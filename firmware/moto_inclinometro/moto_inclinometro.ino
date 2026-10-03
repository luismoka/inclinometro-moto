/*
  INCLINOMETRO PARA MOTO - LOLIN S3 Mini Pro (ESP32-S3 + TFT 0.85" + IMU QMI8658)

  Muestra: inclinacion en directo, maxima a izquierda y derecha,
           maxima aceleracion y maxima frenada (en g).
  Guarda los maximos en memoria y los ensena tambien en el movil por WiFi.

  BOTONES (por el color de la serigrafia)
    AZUL    (IO0)  mantener 3 s  -> calibrar (2 pasos)
    NARANJA (IO47) mantener 1,5 s -> borrar maximos / pulsacion corta: confirmar
    VERDE   (IO48) pulsacion corta -> girar la pantalla 90 grados

  MOVIL: conectate a la WiFi "MotoLean" (clave moto1234) y abre http://192.168.4.1

  Libreria necesaria: TFT_eSPI (version de WEMOS), ver instrucciones.
*/

#include <TFT_eSPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ---------------- Ajustes ----------------
const char *WIFI_NOMBRE = "MotoLean";
const char *WIFI_CLAVE  = "moto1234";      // minimo 8 caracteres

#define PIN_SDA   12
#define PIN_SCL   11
#define PIN_BL    33
#define BTN_AZUL  0
#define BTN_NAR   47
#define BTN_VERDE 48
#define IMU_ADDR  0x6B

const float LEAN_MIN_REG = 8.0f;    // por debajo de esto no se registra como maximo
const float LEAN_MAX_REG = 70.0f;   // por encima se descarta (caida / lectura falsa)
const float G_MAX_REG    = 2.0f;    // aceleraciones mayores se descartan

// ---------------- Vectores ----------------
struct V3 { float x, y, z; };
static inline V3 vadd(V3 a, V3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline V3 vsub(V3 a, V3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline V3 vmul(V3 a, float k) { return { a.x * k, a.y * k, a.z * k }; }
static inline float vdot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 vcross(V3 a, V3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
static inline float vnorm(V3 a) { return sqrtf(vdot(a, a)); }
static inline V3 vunit(V3 a) { float n = vnorm(a); return n > 1e-6f ? vmul(a, 1.0f / n) : a; }

// ---------------- Estado ----------------
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
WebServer web(80);
Preferences nvs;

bool imuOk = false, calibrado = false;
V3 ejeArriba = { 0, 0, 1 }, ejeDelante = { 1, 0, 0 }, ejeIzq = { 0, 1, 0 };
V3 biasGiro = { 0, 0, 0 };
V3 accF = { 0, 0, 1 };          // acelerometro filtrado
float guinoF = 0;               // giro "no de balanceo" filtrado (para saber si va recto)

float lean = 0;                 // grados, positivo = derecha
float accLong = 0;              // g, positivo = acelerando
float maxIzq = 0, maxDer = 0, maxAcel = 0, maxFren = 0;
bool hayQueGuardar = false;
uint32_t tUltimoGuardado = 0;
uint8_t rotacion = 0;

// ---------------- Botones ----------------
struct Boton {
  uint8_t pin; uint16_t msLargo;
  bool antes = false, largoHecho = false; uint32_t t0 = 0;
  bool corta = false, larga = false;
  Boton(uint8_t p, uint16_t ms) : pin(p), msLargo(ms) {}
  void begin() { pinMode(pin, INPUT_PULLUP); }
  void leer() {
    corta = larga = false;
    bool ahora = digitalRead(pin) == LOW;
    uint32_t t = millis();
    if (ahora && !antes) { t0 = t; largoHecho = false; }
    if (ahora && !largoHecho && t - t0 > msLargo) { larga = true; largoHecho = true; }
    if (!ahora && antes && !largoHecho && t - t0 > 30) corta = true;
    antes = ahora;
  }
};
Boton bAzul(BTN_AZUL, 3000), bNar(BTN_NAR, 1500), bVerde(BTN_VERDE, 1500);

// ---------------- IMU QMI8658 ----------------
void imuEscribe(uint8_t reg, uint8_t val) {
  Wire.beginTransmission((uint8_t)IMU_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

bool imuInicia() {
  Wire.beginTransmission((uint8_t)IMU_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)1) != 1) return false;
  if (Wire.read() != 0x05) return false;      // WHO_AM_I
  imuEscribe(0x60, 0xB0); delay(20);          // reinicio
  imuEscribe(0x02, 0x40);                     // auto-incremento, little-endian
  imuEscribe(0x03, 0x25);                     // acelerometro +-8 g, 235 Hz
  imuEscribe(0x04, 0x55);                     // giroscopio +-512 grados/s, 235 Hz
  imuEscribe(0x06, 0x11);                     // filtros paso bajo activados
  imuEscribe(0x08, 0x03);                     // encender acelerometro + giroscopio
  delay(50);
  return true;
}

// a en g, g en grados/segundo
bool imuLee(V3 &a, V3 &g) {
  Wire.beginTransmission((uint8_t)IMU_ADDR);
  Wire.write(0x35);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)12) != 12) return false;
  int16_t r[6];
  for (int i = 0; i < 6; i++) {
    uint8_t lo = Wire.read(), hi = Wire.read();
    r[i] = (int16_t)(lo | (hi << 8));
  }
  a = { r[0] / 4096.0f, r[1] / 4096.0f, r[2] / 4096.0f };
  g = { r[3] / 64.0f, r[4] / 64.0f, r[5] / 64.0f };
  return true;
}

// Media durante "ms". Devuelve cuanto se ha movido el giroscopio (para saber si estaba quieta)
float imuMedia(uint16_t ms, V3 &aM, V3 &gM) {
  V3 sa = { 0, 0, 0 }, sg = { 0, 0, 0 }, a, g, gMin = { 1e9, 1e9, 1e9 }, gMax = { -1e9, -1e9, -1e9 };
  int n = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    if (imuLee(a, g)) {
      sa = vadd(sa, a); sg = vadd(sg, g); n++;
      gMin = { min(gMin.x, g.x), min(gMin.y, g.y), min(gMin.z, g.z) };
      gMax = { max(gMax.x, g.x), max(gMax.y, g.y), max(gMax.z, g.z) };
    }
    delay(5);
  }
  if (n == 0) return 1e9;
  aM = vmul(sa, 1.0f / n); gM = vmul(sg, 1.0f / n);
  return vnorm(vsub(gMax, gMin));
}

// ---------------- Memoria ----------------
void guardaV3(const char *k, V3 v) { nvs.putBytes(k, &v, sizeof(V3)); }
V3 leeV3(const char *k, V3 def) { V3 v = def; nvs.getBytes(k, &v, sizeof(V3)); return v; }

void guardaMaximos() {
  nvs.putFloat("mIzq", maxIzq); nvs.putFloat("mDer", maxDer);
  nvs.putFloat("mAce", maxAcel); nvs.putFloat("mFre", maxFren);
  hayQueGuardar = false; tUltimoGuardado = millis();
}

void borraMaximos() {
  maxIzq = maxDer = maxAcel = maxFren = 0;
  guardaMaximos();
}

void cargaTodo() {
  calibrado = nvs.getBool("cal", false);
  ejeArriba = leeV3("up", ejeArriba);
  ejeDelante = leeV3("fwd", ejeDelante);
  ejeIzq = leeV3("izq", ejeIzq);
  biasGiro = leeV3("bias", biasGiro);
  maxIzq = nvs.getFloat("mIzq", 0); maxDer = nvs.getFloat("mDer", 0);
  maxAcel = nvs.getFloat("mAce", 0); maxFren = nvs.getFloat("mFre", 0);
  rotacion = nvs.getUChar("rot", 0);
}

// ---------------- Pantalla ----------------
void mensaje(const char *l1, const char *l2 = "", const char *l3 = "", const char *l4 = "", uint16_t color = TFT_YELLOW) {
  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(color);
  spr.drawString(l1, 64, 22, 2);
  spr.setTextColor(TFT_WHITE);
  spr.drawString(l2, 64, 50, 2);
  spr.drawString(l3, 64, 70, 2);
  spr.setTextColor(TFT_ORANGE);
  spr.drawString(l4, 64, 104, 2);
  spr.pushSprite(0, 0);
}

#define C_GRIS   0x31A7   // gris oscuro (fondo de arco y barras)
#define C_GRISC  0x8C92   // gris claro (rotulos)
#define C_AZUL   0x2C7F
#define C_AMAR   0xFE80
#define C_ROJO   0xF985
#define C_VERDE  0x3EEB
#define C_NAR    0xFC60
#define C_CIAN   0x06FF

// Marca radial sobre el arco. "grados" desde la vertical, + = derecha
void marca(float grados, int rExt, int rInt, float ancho, uint16_t color) {
  float a = grados * DEG_TO_RAD, s = sinf(a), c = cosf(a);
  spr.drawWideLine(64 + rExt * s, 66 - rExt * c, 64 + rInt * s, 66 - rInt * c, ancho, color, TFT_BLACK);
}

// Barra vertical de 10 segmentos (0,1 g cada uno) con raya de maximo
void barra(int x, float val, float maxi, uint16_t color) {
  int n = (int)lroundf(constrain(val, 0.0f, 1.0f) * 10);
  for (int i = 0; i < 10; i++)
    spr.fillRect(x, 20 + i * 7, 10, 6, (9 - i) < n ? color : C_GRIS);
  int ym = 90 - (int)(constrain(maxi, 0.0f, 1.0f) * 70);
  spr.fillRect(x - 2, ym - 1, 14, 2, TFT_WHITE);
}

void dibuja() {
  char buf[16];
  const int cx = 64, cy = 66, R = 46, RI = 39;
  spr.fillSprite(TFT_BLACK);

  float al = fabsf(lean);
  uint16_t col = al < 25 ? C_AZUL : (al < 40 ? C_AMAR : C_ROJO);

  // Arco de inclinacion (escala +-65 grados). En drawSmoothArc, 180 = arriba
  spr.drawSmoothArc(cx, cy, R, RI, 115, 245, C_GRIS, TFT_BLACK, false);
  int l = constrain((int)lroundf(lean), -65, 65);
  if (l > 0) spr.drawSmoothArc(cx, cy, R, RI, 180, 180 + l, col, TFT_BLACK, false);
  if (l < 0) spr.drawSmoothArc(cx, cy, R, RI, 180 + l, 180, col, TFT_BLACK, false);
  marca(-60, R + 3, R + 1, 1, C_GRISC); marca(-30, R + 3, R + 1, 1, C_GRISC);
  marca(30, R + 3, R + 1, 1, C_GRISC);  marca(60, R + 3, R + 1, 1, C_GRISC);
  marca(0, R + 4, RI - 1, 2, TFT_WHITE);
  marca(-min(maxIzq, 65.0f), R + 2, RI - 1, 2, C_CIAN);
  marca(min(maxDer, 65.0f), R + 2, RI - 1, 2, C_CIAN);

  // Numero grande con el simbolo de grados dibujado a mano
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE);
  spr.setFreeFont(&FreeSansBold18pt7b);
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(al));
  int w = spr.textWidth(buf);
  spr.drawString(buf, cx - 3, 55);
  spr.drawCircle(cx - 3 + w / 2 + 6, 45, 3, TFT_WHITE);
  spr.drawCircle(cx - 3 + w / 2 + 6, 45, 4, TFT_WHITE);
  spr.setTextFont(2);

  spr.setTextColor(al > 1.5f ? col : C_VERDE);
  spr.drawString(lean < -1.5f ? "<< IZQ" : (lean > 1.5f ? "DER >>" : "RECTA"), cx, 78, 2);

  // Maximos de inclinacion
  spr.setTextColor(C_GRISC);
  spr.drawString("MAX", cx, 91, 1);
  spr.setTextColor(C_CIAN);
  spr.setTextDatum(MR_DATUM);
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(maxIzq));
  spr.drawString(buf, cx - 12, 103, 2);
  spr.drawCircle(cx - 9, 98, 2, C_CIAN);
  spr.setTextDatum(ML_DATUM);
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(maxDer));
  int w2 = spr.drawString(buf, cx + 6, 103, 2);
  spr.drawCircle(cx + 6 + w2 + 3, 98, 2, C_CIAN);

  // Barras de frenada (izquierda) y aceleracion (derecha)
  barra(3, -accLong, maxFren, C_NAR);
  barra(115, accLong, maxAcel, C_VERDE);

  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(C_NAR);
  spr.drawString("FRENO", 2, 12, 1);
  snprintf(buf, sizeof(buf), "%.2f", maxFren);
  spr.drawString(buf, 2, 98, 1);
  spr.setTextColor(C_GRISC);
  spr.drawString("g max", 2, 108, 1);

  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(C_VERDE);
  spr.drawString("ACEL", 126, 12, 1);
  snprintf(buf, sizeof(buf), "%.2f", maxAcel);
  spr.drawString(buf, 126, 98, 1);
  spr.setTextColor(C_GRISC);
  spr.drawString("g max", 126, 108, 1);

  // Aceleracion en directo
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE);
  snprintf(buf, sizeof(buf), "%+.2f g", accLong);
  spr.drawString(buf, cx, 120, 2);

  spr.pushSprite(0, 0);
}

// ---------------- Calibracion ----------------
bool esperaNaranja() {            // true = confirmado, false = cancelado con el azul
  delay(400);
  while (true) {
    bNar.leer(); bAzul.leer();
    if (bNar.corta || bNar.larga) return true;
    if (bAzul.corta) return false;
    delay(10);
  }
}

void calibrar() {
  V3 a0, a1, g0, g1;
  mensaje("CALIBRAR 1/2", "Moto RECTA", "y quieta", "pulsa NARANJA");
  if (!esperaNaranja()) return;
  mensaje("Midiendo...", "no la muevas");
  delay(500);
  imuMedia(1500, a0, g0);

  mensaje("CALIBRAR 2/2", "Apoyala en la", "PATA DE CABRA", "pulsa NARANJA");
  if (!esperaNaranja()) return;
  mensaje("Midiendo...", "no la muevas");
  delay(500);
  imuMedia(1500, a1, g1);

  V3 u0 = vunit(a0), u1 = vunit(a1);
  V3 eje = vcross(u0, u1);          // eje de balanceo = direccion de avance (la pata esta a la izquierda)
  if (vnorm(eje) < 0.07f) {         // menos de ~4 grados de diferencia
    mensaje("ERROR", "Poca diferencia", "entre los 2 pasos", "repite", TFT_RED);
    delay(3000);
    return;
  }
  ejeArriba = u0;
  ejeDelante = vunit(eje);
  ejeIzq = vcross(ejeArriba, ejeDelante);
  biasGiro = g1;
  calibrado = true;

  nvs.putBool("cal", true);
  guardaV3("up", ejeArriba); guardaV3("fwd", ejeDelante);
  guardaV3("izq", ejeIzq); guardaV3("bias", biasGiro);

  accF = a1;
  lean = atan2f(vdot(a1, ejeIzq), vdot(a1, ejeArriba)) * RAD_TO_DEG;
  mensaje("CALIBRADO", "Todo listo", "", "", TFT_GREEN);
  delay(1500);
}

// ---------------- Calculo ----------------
void calcula(float dt, V3 a, V3 g) {
  g = vsub(g, biasGiro);

  float kA = dt / (0.25f + dt);                       // filtro del acelerometro (~0,25 s)
  accF = vadd(accF, vmul(vsub(a, accF), kA));

  float p = vdot(g, ejeDelante);                      // velocidad de balanceo, + = hacia la derecha
  float q = vdot(g, ejeIzq), r = vdot(g, ejeArriba);
  float kG = dt / (0.5f + dt);
  guinoF += (sqrtf(q * q + r * r) - guinoF) * kG;

  lean += p * dt;                                     // el giroscopio manda

  // En curva el acelerometro NO sirve para medir la inclinacion (la fuerza centrifuga lo engana),
  // asi que solo corrige la deriva del giroscopio cuando la moto va recta o esta parada.
  float leanAcc = atan2f(vdot(accF, ejeIzq), vdot(accF, ejeArriba)) * RAD_TO_DEG;
  bool recta = fabsf(vnorm(accF) - 1.0f) < 0.06f && guinoF < 2.5f && fabsf(p) < 4.0f;
  if (recta) lean += (leanAcc - lean) * (dt / 2.0f);
  lean = constrain(lean, -90.0f, 90.0f);

  accLong = vdot(accF, ejeDelante);

  if (!calibrado) return;

  // Maximos de inclinacion: solo cuentan las inclinaciones "de curva"
  // (apoyada en la pata o tumbada en parado, el acelerometro coincide con el angulo y no se registra)
  float al = fabsf(lean);
  if (al > LEAN_MIN_REG && al < LEAN_MAX_REG && fabsf(lean - leanAcc) > 0.5f * al) {
    if (lean < 0 && al > maxIzq) { maxIzq = al; hayQueGuardar = true; }
    if (lean > 0 && al > maxDer) { maxDer = al; hayQueGuardar = true; }
  }
  // Maximos de aceleracion y frenada
  if (fabsf(accLong) < G_MAX_REG) {
    if (accLong > maxAcel + 0.01f) { maxAcel = accLong; hayQueGuardar = true; }
    if (-accLong > maxFren + 0.01f) { maxFren = -accLong; hayQueGuardar = true; }
  }
}

// ---------------- Web para el movil ----------------
const char PAGINA[] PROGMEM = R"HTML(<!DOCTYPE html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>MotoLean</title>
<style>body{background:#111;color:#eee;font-family:sans-serif;text-align:center;margin:0;padding:16px}
#l{font-size:96px;font-weight:700;line-height:1}#s{font-size:22px;color:#8f8;height:28px}
.g{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin:18px 0}
.c{background:#222;border-radius:12px;padding:12px}.c b{display:block;font-size:34px}
.c span{color:#999;font-size:14px}button{font-size:18px;padding:12px 22px;border:0;border-radius:10px;background:#c40;color:#fff}
</style></head><body><div id="l">--</div><div id="s"></div>
<div class="g"><div class="c"><span>Max izquierda</span><b id="mi">-</b></div>
<div class="c"><span>Max derecha</span><b id="md">-</b></div>
<div class="c"><span>Max aceleracion</span><b id="ma">-</b></div>
<div class="c"><span>Max frenada</span><b id="mf">-</b></div>
<div class="c" style="grid-column:1/3"><span>Aceleracion ahora</span><b id="al">-</b></div></div>
<button onclick="if(confirm('Borrar maximos?'))fetch('/reset')">Borrar maximos</button>
<script>async function t(){try{const d=await(await fetch('/datos')).json();
l.textContent=Math.abs(d.lean).toFixed(0)+'\u00b0';
s.textContent=d.cal?(d.lean<-1.5?'\u25c0 IZQUIERDA':d.lean>1.5?'DERECHA \u25b6':'RECTA'):'SIN CALIBRAR';
mi.textContent=d.mi.toFixed(0)+'\u00b0';md.textContent=d.md.toFixed(0)+'\u00b0';
ma.textContent=d.ma.toFixed(2)+' g';mf.textContent=d.mf.toFixed(2)+' g';
al.textContent=d.al.toFixed(2)+' g';}catch(e){}setTimeout(t,250)}t()</script></body></html>)HTML";

void webInicia() {
  web.on("/", []() { web.send_P(200, "text/html", PAGINA); });
  web.on("/datos", []() {
    char b[160];
    snprintf(b, sizeof(b), "{\"lean\":%.1f,\"mi\":%.1f,\"md\":%.1f,\"ma\":%.2f,\"mf\":%.2f,\"al\":%.2f,\"cal\":%d}",
             lean, maxIzq, maxDer, maxAcel, maxFren, accLong, calibrado ? 1 : 0);
    web.send(200, "application/json", b);
  });
  web.on("/reset", []() { borraMaximos(); web.send(200, "text/plain", "ok"); });
  web.begin();
}

// ---------------- Arranque ----------------
void setup() {
  Serial.begin(115200);
  bAzul.begin(); bNar.begin(); bVerde.begin();

  nvs.begin("moto", false);
  cargaTodo();

  pinMode(PIN_BL, OUTPUT); digitalWrite(PIN_BL, HIGH);
  tft.init();
  tft.setRotation(rotacion);
  tft.fillScreen(TFT_BLACK);
  spr.setColorDepth(16);
  spr.createSprite(128, 128);

  Wire.setPins(PIN_SDA, PIN_SCL);
  Wire.begin();
  Wire.setClock(400000);
  imuOk = imuInicia();
  if (!imuOk) {
    mensaje("ERROR", "No encuentro", "el sensor IMU", "", TFT_RED);
    while (true) delay(1000);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_NOMBRE, WIFI_CLAVE);
  webInicia();

  // Al encender: si la moto esta quieta, afinamos el cero del giroscopio
  mensaje("MotoLean", "Arrancando...", "", "WiFi: MotoLean", TFT_GREEN);
  V3 a, g;
  float movimiento = imuMedia(1000, a, g);
  if (movimiento < 3.0f) biasGiro = g;
  accF = a;
  if (calibrado) lean = atan2f(vdot(a, ejeIzq), vdot(a, ejeArriba)) * RAD_TO_DEG;
}

// ---------------- Bucle ----------------
void loop() {
  static uint32_t tAnt = micros(), tDib = 0;
  V3 a, g;
  if (imuLee(a, g)) {
    uint32_t t = micros();
    float dt = (t - tAnt) * 1e-6f;
    tAnt = t;
    if (dt > 0 && dt < 0.1f) calcula(dt, a, g);
  }

  bAzul.leer(); bNar.leer(); bVerde.leer();
  if (bAzul.larga) { calibrar(); tAnt = micros(); }
  if (bNar.larga) {
    borraMaximos();
    mensaje("MAXIMOS", "borrados", "", "", TFT_GREEN);
    delay(800); tAnt = micros();
  }
  if (bVerde.corta) {
    rotacion = (rotacion + 1) % 4;
    tft.setRotation(rotacion);
    nvs.putUChar("rot", rotacion);
  }

  web.handleClient();

  uint32_t ms = millis();
  if (ms - tDib > 60) {
    tDib = ms;
    if (calibrado) dibuja();
    else mensaje("SIN CALIBRAR", "Manten AZUL 3 s", "para calibrar");
  }
  // Guardado automatico (como mucho cada 5 s, y solo si ha cambiado algun maximo)
  if (hayQueGuardar && ms - tUltimoGuardado > 5000) guardaMaximos();

  delay(3);
}
