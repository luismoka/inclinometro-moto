/*
  INCLINOMETRO PARA MOTO - Waveshare ESP32-S3-LCD-1.69 (pantalla de 240x280 + IMU QMI8658)

  Muestra: inclinacion en directo, maxima a izquierda y derecha, y maxima aceleracion y frenada (en g).
  Con un GPS NEO-6M graba las rutas (posicion, velocidad, inclinacion y g) para verlas en un mapa.
  Los maximos se ponen a cero al empezar cada ruta.

  BOTONES
    BOOT  pulsacion corta  -> girar la pantalla 180 grados
          mantener 3 s     -> calibracion completa (2 pasos: recta y apoyada en la pata)
    PWR   pulsacion corta  -> ajuste de centro: lo que marca pasa a ser 0 (solo con menos de 10 grados)
          mantener 1,5 s   -> terminar la ruta y parar la grabacion (otra vez: reanudar)

  PANTALLA
    Icono GPS (arriba izquierda): verde fijo con posicion, rojo parpadeando sin ella. REC parpadeando arriba en el centro = grabando.
    Icono WiFi (arriba derecha): verde con conexion, gris sin ella.
    Por encima de 40 grados el fondo parpadea en rojo. Por debajo de 4 grados marca RECTA.

  MOVIL: conectate a la WiFi "MotoLean" (clave moto1234) y abre http://192.168.4.1

  Libreria necesaria: TFT_eSPI, configurada con Setup_Waveshare_169.h (ver docs/PLACA_169.md).
*/

#include <TFT_eSPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <time.h>

#define VERSION "3.2"

// ---------------- Ajustes ----------------
const char *WIFI_NOMBRE = "MotoLean";
const char *WIFI_CLAVE  = "moto1234";      // minimo 8 caracteres

#define PIN_SDA   11
#define PIN_SCL   10
#define PIN_BL    15
#define BTN_BOOT  0       // activo a nivel bajo
#define BTN_PWR   40      // SYS_OUT: a nivel alto mientras se pulsa
#define PIN_SYS_EN 41     // mantiene la alimentacion cuando va con bateria
#define PIN_ZUMB  42      // zumbador (sin uso)
#define IMU_ADDR  0x6B
// Botonera de membrana de 2 botones y 3 hilos. Los tres hilos van a tres pines, en cualquier orden:
// el programa aprende que hilos une cada boton, asi que no hace falta saber cual es el comun.
const uint8_t KB_PIN[3] = { 16, 2, 3 };

#define ANCHO 240
#define ALTO  280
const float ZONA_RECTA   = 4.0f;    // por debajo de esto la moto se considera recta
const float ANGULO_AVISO = 40.0f;   // a partir de aqui el fondo parpadea en rojo
const float CENTRO_MAX   = 10.0f;   // el ajuste de centro solo se acepta por debajo de esto

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
float maxIzq = 0, maxDer = 0, maxAcel = 0, maxFren = 0, maxVel = 0;
bool hayQueGuardar = false;
uint32_t tUltimoGuardado = 0;
uint8_t rotacion = 0;

// ---------------- Botones ----------------
// ---------------- Botonera externa ----------------
uint8_t kbPar[2] = { 255, 255 };      // par de hilos de cada boton (0..2); 255 = sin configurar
uint32_t kbTDos = 0;
// Devuelve una mascara con los pares de hilos que estan unidos: bit0 = 0-1, bit1 = 0-2, bit2 = 1-2
uint8_t kbLee() {
  static const uint8_t A[3] = { 0, 0, 1 }, B[3] = { 1, 2, 2 };
  uint8_t m = 0;
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(KB_PIN[A[i]], OUTPUT); digitalWrite(KB_PIN[A[i]], LOW);
    delayMicroseconds(40);
    if (digitalRead(KB_PIN[B[i]]) == LOW) m |= 1 << i;
    pinMode(KB_PIN[A[i]], INPUT_PULLUP);
    delayMicroseconds(40);
  }
  return m;
}

struct Boton {
  uint8_t pin; uint16_t msLargo; bool activoAlto;
  bool antes = false, largoHecho = false; uint32_t t0 = 0;
  bool corta = false, larga = false;
  int8_t kb = -1;                     // -1: boton de la placa; 0 amarillo, 1 rojo, 2 los dos a la vez
  Boton(uint8_t p, uint16_t ms, bool alto = false) : pin(p), msLargo(ms), activoAlto(alto) {}
  Boton(int8_t k, uint16_t ms, int) : pin(0), msLargo(ms), activoAlto(false), kb(k) {}
  void begin() { if (kb < 0) pinMode(pin, activoAlto ? INPUT : INPUT_PULLUP); antes = pulsado(); largoHecho = antes; }
  bool pulsado() {
    if (kb < 0) return digitalRead(pin) == (activoAlto ? HIGH : LOW);
    uint8_t m = kbLee();
    bool dos = (m & (m - 1)) != 0;    // mas de un par unido: los dos botones
    if (dos) kbTDos = millis();
    if (kb == 2) return dos;
    if (dos || kbPar[kb] > 2) return false;
    return m == (1 << kbPar[kb]);
  }
  void leer() {
    corta = larga = false;
    bool ahora = pulsado();
    uint32_t t = millis();
    if (ahora && !antes) { t0 = t; largoHecho = false; }
    if (ahora && !largoHecho && t - t0 > msLargo) { larga = true; largoHecho = true; }
    if (!ahora && antes && !largoHecho && t - t0 > 30) corta = true;
    if (kb >= 0 && kb < 2 && t - kbTDos < 700) corta = larga = false;   // se estaban soltando los dos
    antes = ahora;
  }
};
Boton bBoot(BTN_BOOT, 3000), bPwr(BTN_PWR, 1500, true);
Boton bAma((int8_t)0, 3000, 0), bRojo((int8_t)1, 1500, 0), bDos((int8_t)2, 1500, 0);

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
  nvs.putFloat("mVel", maxVel);
  hayQueGuardar = false; tUltimoGuardado = millis();
}

void borraMaximos() {
  maxIzq = maxDer = maxAcel = maxFren = maxVel = 0;
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
  maxVel = nvs.getFloat("mVel", 0);
  rotacion = nvs.getUChar("rot", 0);
}

// ---------------- GPS (NEO-6M, modulo GY-GPS6MV2) ----------------
// Cableado: VCC->5V (o 3V3), GND->GND, TX del GPS->GPIO17, RX del GPS->GPIO18
#define GPS_RX 17
#define GPS_TX 18

bool gpsFix = false, gpsNueva = false;
uint32_t gpsUltimo = 0;          // millis() de la ultima posicion valida
double gpsLat = 0, gpsLon = 0;
float gpsKmh = 0;
uint32_t gpsUnix = 0;            // hora UTC en segundos desde 1970
char nmea[100];
uint8_t nmeaN = 0;
// Diagnostico: lo que llega del GPS, para verlo en /gps y por el puerto serie
uint32_t gpsBytes = 0, gpsFrases = 0, gpsMalas = 0;
int gpsSat = -1;
char gpsUltimas[6][84];
uint8_t gpsUltN = 0;

double nmeaCoord(const char *v, const char *h) {
  if (!*v) return 0;
  double x = atof(v);
  int grados = (int)(x / 100);
  double r = grados + (x - grados * 100) / 60.0;
  return (*h == 'S' || *h == 'W') ? -r : r;
}

uint32_t aUnix(int y, int m, int d, int hh, int mm, int ss) {
  y -= m <= 2;
  long era = y / 400;
  unsigned yoe = y - era * 400;
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long dias = era * 146097L + (long)doe - 719468L;
  return (uint32_t)dias * 86400UL + hh * 3600UL + mm * 60UL + ss;
}

static int dosDig(const char *c) { return (c[0] - '0') * 10 + (c[1] - '0'); }

// Solo se usa la frase RMC: hora, posicion, velocidad y fecha
void nmeaProcesa(char *l) {
  char *ast = strchr(l, '*');
  if (!ast) return;
  uint8_t cs = 0;
  for (char *q = l + 1; q < ast; q++) cs ^= (uint8_t)*q;
  if (cs != (uint8_t)strtol(ast + 1, NULL, 16)) { gpsMalas++; return; }
  gpsFrases++;
  strlcpy(gpsUltimas[gpsUltN % 6], l, sizeof(gpsUltimas[0])); gpsUltN++;
  Serial.println(l);                       // eco al puerto serie USB (115200)
  *ast = 0;
  if (strlen(l) >= 6 && strncmp(l + 3, "GGA", 3) == 0) {   // numero de satelites
    int coma = 0;
    for (char *q = l; *q; q++) if (*q == ',' && ++coma == 7) { gpsSat = atoi(q + 1); break; }
    return;
  }
  if (strlen(l) < 6 || strncmp(l + 3, "RMC", 3) != 0) return;

  char *c[14]; int n = 0;
  c[n++] = l;
  for (char *q = l; *q && n < 14; q++) if (*q == ',') { *q = 0; c[n++] = q + 1; }
  if (n < 10) return;
  if (c[2][0] != 'A') { gpsFix = false; return; }
  if (strlen(c[1]) < 6 || strlen(c[9]) < 6) return;

  gpsLat = nmeaCoord(c[3], c[4]);
  gpsLon = nmeaCoord(c[5], c[6]);
  gpsKmh = atof(c[7]) * 1.852f;
  gpsUnix = aUnix(2000 + dosDig(c[9] + 4), dosDig(c[9] + 2), dosDig(c[9]),
                  dosDig(c[1]), dosDig(c[1] + 2), dosDig(c[1] + 4));
  gpsFix = true; gpsNueva = true; gpsUltimo = millis();
  if (gpsKmh > maxVel + 0.5f && gpsKmh < 300) { maxVel = gpsKmh; hayQueGuardar = true; }
}

void gpsLee() {
  while (Serial1.available()) {
    char ch = Serial1.read();
    gpsBytes++;
    if (ch == '$') nmeaN = 0;
    if (ch == '\r' || ch == '\n') {
      if (nmeaN > 6) { nmea[nmeaN] = 0; nmeaProcesa(nmea); }
      nmeaN = 0;
    } else if (nmeaN < sizeof(nmea) - 1) nmea[nmeaN++] = ch;
  }
  if (gpsFix && millis() - gpsUltimo > 3000) gpsFix = false;
}

// ---------------- Grabacion de rutas ----------------
// Un registro por segundo, 20 bytes. En los 9,9 MB de esta placa caben mas de 100 horas en movimiento.
struct __attribute__((packed)) Reg {
  uint32_t t;            // hora UTC (segundos desde 1970)
  int32_t lat, lon;      // grados x 1e7
  uint16_t v;            // km/h x 100
  int16_t lean;          // grados x 10: la mayor inclinacion del segundo (+ derecha)
  int16_t aMax, aMin;    // g x 1000: mayor aceleracion y mayor frenada del segundo
};

bool fsOk = false, grabando = false;
bool rutaPausa = false;      // grabacion parada a mano (boton PWR largo o web) hasta reanudarla o apagar
fs::File fRuta;
char nombreRuta[40] = "";
uint8_t sinVolcar = 0, parado = 0;
float sLean = 0, sAMax = 0, sAMin = 0;      // picos del segundo en curso

// Borra la ruta mas antigua mientras falte sitio
void haceSitio() {
  for (int i = 0; i < 20 && LittleFS.totalBytes() - LittleFS.usedBytes() < 60000; i++) {
    String viejo = "";
    fs::File d = LittleFS.open("/");
    for (fs::File f = d.openNextFile(); f; f = d.openNextFile()) {
      String nom = String("/") + f.name();
      if (nom.startsWith("/r_") && nom != nombreRuta && (viejo == "" || nom < viejo)) viejo = nom;
    }
    if (viejo == "") return;
    LittleFS.remove(viejo);
  }
}

// Cierra la ruta en curso y la deja guardada
void terminaRuta() {
  if (!grabando) return;
  fRuta.close();
  grabando = false; nombreRuta[0] = 0; sinVolcar = 0;
}

void registra() {
  gpsNueva = false;
  if (!fsOk) return;
  if (!grabando) {
    if (rutaPausa || gpsKmh < 5) return;                 // la ruta empieza al echar a andar
    haceSitio();
    borraMaximos();                         // los maximos son de cada ruta
    time_t tt = gpsUnix; struct tm *g = gmtime(&tt);
    snprintf(nombreRuta, sizeof(nombreRuta), "/r_%04d%02d%02d_%02d%02d%02d.bin",
             g->tm_year + 1900, g->tm_mon + 1, g->tm_mday, g->tm_hour, g->tm_min, g->tm_sec);
    fRuta = LittleFS.open(nombreRuta, "w");
    if (!fRuta) { nombreRuta[0] = 0; return; }
    grabando = true; parado = 0;
  }
  // Parado mas de 5 s: no se gastan registros
  if (gpsKmh < 3) { if (parado < 255) parado++; } else parado = 0;
  if (parado <= 5) {
    Reg r;
    r.t = gpsUnix;
    r.lat = (int32_t)llround(gpsLat * 1e7); r.lon = (int32_t)llround(gpsLon * 1e7);
    r.v = (uint16_t)constrain(gpsKmh * 100.0f, 0.0f, 65000.0f);
    r.lean = (int16_t)lroundf(sLean * 10);
    r.aMax = (int16_t)lroundf(constrain(sAMax, -3.0f, 3.0f) * 1000);
    r.aMin = (int16_t)lroundf(constrain(sAMin, -3.0f, 3.0f) * 1000);
    fRuta.write((uint8_t *)&r, sizeof(r));
    if (++sinVolcar >= 10) { fRuta.flush(); sinVolcar = 0; haceSitio(); }
  }
  sLean = lean; sAMax = sAMin = accLong;
}

// ---------------- Pantalla ----------------
#define C_GRIS   0x31A7   // gris oscuro (fondo de arco y barras)
#define C_GRISC  0x8C92   // gris claro (rotulos)
#define C_AZUL   0x2C7F
#define C_AMAR   0xFE80
#define C_ROJO   0xF985
#define C_VERDE  0x3EEB
#define C_NAR    0xFC60
#define C_CIAN   0x06FF
#define C_AVISO  0x9000   // rojo oscuro del fondo de aviso

uint16_t cFondo = TFT_BLACK;
String wifiCasa = "", claveCasa = "";
bool casaBuscando = false, casaConectada = false;

void mensaje(const char *l1, const char *l2 = "", const char *l3 = "", const char *l4 = "", uint16_t color = TFT_YELLOW) {
  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(color);
  spr.drawString(l1, ANCHO / 2, 62, 4);
  spr.setTextColor(TFT_WHITE);
  spr.drawString(l2, ANCHO / 2, 122, 4);
  spr.drawString(l3, ANCHO / 2, 156, 4);
  spr.setTextColor(TFT_ORANGE);
  spr.drawString(l4, ANCHO / 2, 226, 4);
  spr.pushSprite(0, 0);
}

// Marca radial sobre el arco. "grados" desde la vertical, + = derecha
void marca(float grados, int rExt, int rInt, float ancho, uint16_t color) {
  float a = grados * DEG_TO_RAD, s = sinf(a), c = cosf(a);
  spr.drawWideLine(120 + rExt * s, 150 - rExt * c, 120 + rInt * s, 150 - rInt * c, ancho, color, cFondo);
}

// Barra vertical de 10 segmentos (0,1 g cada uno) con raya de maximo
void barra(int x, float val, float maxi, uint16_t color, uint16_t gris) {
  int n = (int)lroundf(constrain(val, 0.0f, 1.0f) * 10);
  for (int i = 0; i < 10; i++)
    spr.fillRect(x, 70 + i * 13, 13, 10, (9 - i) < n ? color : gris);
  int ym = 200 - (int)(constrain(maxi, 0.0f, 1.0f) * 130);
  spr.fillRect(x - 3, ym - 2, 19, 3, TFT_WHITE);
}

// Simbolo de grados: un aro
void grado(int x, int y, int r, int grosor, uint16_t color) {
  spr.fillCircle(x, y, r, color);
  spr.fillCircle(x, y, r - grosor, cFondo);
}

void iconoGps(int x, int y, uint16_t c) {
  spr.fillCircle(x, y - 2, 9, c);
  spr.fillTriangle(x - 7, y + 3, x + 7, y + 3, x, y + 15, c);
  spr.fillCircle(x, y - 2, 3, cFondo);
}

void iconoWifi(int x, int y, uint16_t c) {
  spr.drawSmoothArc(x, y + 8, 17, 14, 135, 225, c, cFondo, true);
  spr.drawSmoothArc(x, y + 8, 11, 8, 135, 225, c, cFondo, true);
  spr.fillCircle(x, y + 7, 3, c);
}

void dibuja() {
  char buf[16];
  const int cx = 120, cy = 150, R = 100, RI = 85;
  float al = fabsf(lean);
  uint32_t ms = millis();

  // Por encima del angulo de aviso, el fondo parpadea en rojo
  bool aviso = al >= ANGULO_AVISO && (ms / 250) % 2 == 0;
  cFondo = aviso ? C_AVISO : TFT_BLACK;
  uint16_t gris = aviso ? 0x5945 : C_GRIS;
  spr.fillSprite(cFondo);

  // Iconos de estado
  if (gpsFix) iconoGps(38, 30, C_VERDE);
  else if ((ms / 400) % 2 == 0) iconoGps(38, 30, C_ROJO);
  if (grabando && (ms / 500) % 2 == 0) {                           // REC parpadeando = grabando ruta
    uint16_t cRec = aviso ? TFT_WHITE : C_ROJO;                    // sobre el fondo rojo de aviso, en blanco
    spr.fillCircle(92, 21, 8, cRec);
    spr.setTextDatum(ML_DATUM); spr.setTextColor(cRec, cFondo);
    spr.drawString("REC", 105, 22, 4);
  }
  bool hayWifi = casaConectada || WiFi.softAPgetStationNum() > 0;
  iconoWifi(202, 28, hayWifi ? C_VERDE : gris);

  // Arco de inclinacion (escala +-65 grados). En drawSmoothArc, 180 = arriba
  uint16_t col = al < 25 ? C_AZUL : (al < ANGULO_AVISO ? C_AMAR : (aviso ? TFT_WHITE : C_ROJO));
  spr.drawSmoothArc(cx, cy, R, RI, 115, 245, gris, cFondo, false);
  int l = constrain((int)lroundf(lean), -65, 65);
  if (al >= ZONA_RECTA) {
    if (l > 0) spr.drawSmoothArc(cx, cy, R, RI, 180, 180 + l, col, cFondo, false);
    if (l < 0) spr.drawSmoothArc(cx, cy, R, RI, 180 + l, 180, col, cFondo, false);
  }
  marca(-60, R + 6, R + 2, 2, C_GRISC); marca(-30, R + 6, R + 2, 2, C_GRISC);
  marca(30, R + 6, R + 2, 2, C_GRISC);  marca(60, R + 6, R + 2, 2, C_GRISC);
  marca(0, R + 7, RI - 2, 3, TFT_WHITE);
  marca(-min(maxIzq, 65.0f), R + 4, RI - 2, 3, C_CIAN);
  marca(min(maxDer, 65.0f), R + 4, RI - 2, 3, C_CIAN);

  // Numero grande
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE);
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(al));
  int w = spr.textWidth(buf, 8);
  spr.drawString(buf, cx - 10, 128, 8);
  grado(cx - 10 + w / 2 + 11, 101, 8, 3, TFT_WHITE);

  // Lado
  bool recta = al < ZONA_RECTA;
  spr.setFreeFont(&FreeSansBold12pt7b);
  spr.setTextColor(recta ? C_VERDE : col);
  spr.drawString(recta ? "RECTA" : (lean < 0 ? "<< IZQ" : "DER >>"), cx, 188);

  // Maximos de inclinacion
  spr.setTextFont(2);
  spr.setTextColor(C_GRISC);
  spr.drawString("MAX", cx, 214, 2);
  spr.setFreeFont(&FreeSansBold18pt7b);
  spr.setTextColor(C_CIAN);
  spr.setTextDatum(MR_DATUM);
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(maxIzq));
  spr.drawString(buf, cx - 22, 246);
  grado(cx - 15, 236, 5, 2, C_CIAN);
  spr.setTextDatum(ML_DATUM);
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(maxDer));
  int w2 = spr.drawString(buf, cx + 14, 246);
  grado(cx + 14 + w2 + 7, 236, 5, 2, C_CIAN);
  spr.setTextFont(2);

  // Barras de frenada (izquierda) y aceleracion (derecha), con su maximo debajo
  barra(5, -accLong, maxFren, C_NAR, gris);
  barra(222, accLong, maxAcel, C_VERDE, gris);
  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(C_NAR);
  snprintf(buf, sizeof(buf), "%.2f", maxFren);
  spr.drawString(buf, 4, 212, 2);
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(C_VERDE);
  snprintf(buf, sizeof(buf), "%.2f", maxAcel);
  spr.drawString(buf, 236, 212, 2);

  spr.pushSprite(0, 0);
}

// ---------------- Calibracion ----------------
// Completa, en dos pasos: moto recta y moto apoyada en la pata (a la izquierda).
V3 calA0 = { 0, 0, 1 };
bool calPaso1Hecho = false;

void calPaso1() {
  V3 g;
  imuMedia(1500, calA0, g);
  calPaso1Hecho = true;
}

bool calPaso2() {
  if (!calPaso1Hecho) return false;
  V3 a1, g1;
  imuMedia(1500, a1, g1);
  V3 u0 = vunit(calA0), u1 = vunit(a1);
  V3 eje = vcross(u0, u1);          // eje de balanceo = direccion de avance (la pata esta a la izquierda)
  if (vnorm(eje) < 0.07f) return false;   // menos de ~4 grados de diferencia entre los dos pasos
  ejeArriba = u0;
  ejeDelante = vunit(eje);
  ejeIzq = vcross(ejeArriba, ejeDelante);
  biasGiro = g1;
  calibrado = true; calPaso1Hecho = false;

  nvs.putBool("cal", true);
  guardaV3("up", ejeArriba); guardaV3("fwd", ejeDelante);
  guardaV3("izq", ejeIzq); guardaV3("bias", biasGiro);

  accF = a1;
  lean = atan2f(vdot(a1, ejeIzq), vdot(a1, ejeArriba)) * RAD_TO_DEG;
  return true;
}

// Ajuste de centro: la posicion actual pasa a ser 0 grados. No cambia las direcciones.
// Solo se acepta con la moto casi vertical, para que un toque en plena curva no descoloque el cero.
bool ajustaCentro() {
  if (!calibrado) return false;
  float la = atan2f(vdot(accF, ejeIzq), vdot(accF, ejeArriba));
  if (fabsf(la * RAD_TO_DEG) >= CENTRO_MAX || fabsf(lean) >= CENTRO_MAX) return false;
  float c = cosf(la), s = sinf(la);
  V3 arriba = vadd(vmul(ejeArriba, c), vmul(ejeIzq, s));
  V3 izq = vadd(vmul(ejeArriba, -s), vmul(ejeIzq, c));
  ejeArriba = vunit(arriba); ejeIzq = vunit(izq);
  guardaV3("up", ejeArriba); guardaV3("izq", ejeIzq);
  lean = 0;
  return true;
}

bool esperaPulsacion() {            // true = confirmado; false = cancelado (BOOT largo) o sin respuesta en 2 min
  delay(400);
  uint32_t t0 = millis();
  while (millis() - t0 < 120000UL) {
    bBoot.leer(); bPwr.leer(); bAma.leer(); bRojo.leer();
    if (bBoot.corta || bPwr.corta || bAma.corta) return true;
    if (bBoot.larga || bRojo.corta || bRojo.larga) return false;
    delay(10);
  }
  return false;
}

// Aprende que hilos une cada boton de la botonera. Se guarda en memoria.
int8_t kbEsperaPar(int8_t distinto) {
  uint32_t t0 = millis();
  while (millis() - t0 < 30000UL) {
    uint8_t m = kbLee();
    if (m == 1 || m == 2 || m == 4) {
      int8_t par = m == 1 ? 0 : m == 2 ? 1 : 2;
      delay(40);
      if (kbLee() == m && par != distinto) {
        while (kbLee()) delay(10);
        delay(150);
        return par;
      }
    }
    delay(10);
  }
  return -1;
}

bool kbAprende() {
  mensaje("BOTONERA 1/2", "Suelta todo y", "pulsa el", "AMARILLO");
  while (kbLee()) delay(10);
  int8_t a = kbEsperaPar(-1);
  if (a < 0) { mensaje("BOTONERA", "sin respuesta", "", "", TFT_RED); delay(1500); return false; }
  mensaje("BOTONERA 2/2", "Ahora pulsa", "el", "ROJO");
  int8_t r = kbEsperaPar(a);
  if (r < 0) { mensaje("BOTONERA", "sin respuesta", "", "", TFT_RED); delay(1500); return false; }
  kbPar[0] = a; kbPar[1] = r;
  nvs.putUChar("kbA", a); nvs.putUChar("kbR", r);
  bAma.begin(); bRojo.begin(); bDos.begin();
  mensaje("BOTONERA", "configurada", "", "", TFT_GREEN);
  delay(1200);
  return true;
}

void calibrar() {
  mensaje("CALIBRAR 1/2", "Moto RECTA", "y quieta", "AMARILLO: seguir");
  if (!esperaPulsacion()) return;
  mensaje("Midiendo...", "no la muevas");
  delay(500);
  calPaso1();

  mensaje("CALIBRAR 2/2", "Apoyala en la", "PATA DE CABRA", "AMARILLO: seguir");
  if (!esperaPulsacion()) return;
  mensaje("Midiendo...", "no la muevas");
  delay(500);
  if (!calPaso2()) {
    mensaje("ERROR", "Poca diferencia", "entre los 2 pasos", "repite", TFT_RED);
    delay(3000);
    return;
  }
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
  if (fabsf(lean) > fabsf(sLean)) sLean = lean;
  if (accLong > sAMax) sAMax = accLong;
  if (accLong < sAMin) sAMin = accLong;

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
<div class="c"><span>Velocidad</span><b id="v">-</b></div>
<div class="c"><span>Max velocidad</span><b id="mv">-</b></div>
<div class="c" style="grid-column:1/3"><span>Aceleracion ahora</span><b id="al">-</b></div></div>
<p id="g" style="color:#999"></p><p><a href="/rutas" style="color:#6cf;font-size:20px">Rutas grabadas</a> &nbsp; <a href="/wifi" style="color:#6cf;font-size:20px">WiFi de casa</a> &nbsp; <a href="/gps" style="color:#6cf;font-size:20px">GPS</a> &nbsp; <a href="/ajustes" style="color:#6cf;font-size:20px">Ajustes</a></p>
<p style="color:#666;font-size:13px">MotoLean 3.0</p>
<button onclick="if(confirm('Borrar maximos?'))fetch('/reset')">Borrar maximos</button>
<script>async function t(){try{const d=await(await fetch('/datos')).json();
l.textContent=Math.abs(d.lean).toFixed(0)+'\u00b0';
s.textContent=d.cal?(d.lean<-1.5?'\u25c0 IZQUIERDA':d.lean>1.5?'DERECHA \u25b6':'RECTA'):'SIN CALIBRAR';
mi.textContent=d.mi.toFixed(0)+'\u00b0';md.textContent=d.md.toFixed(0)+'\u00b0';
ma.textContent=d.ma.toFixed(2)+' g';mf.textContent=d.mf.toFixed(2)+' g';
al.textContent=d.al.toFixed(2)+' g';v.textContent=d.gps?d.v.toFixed(0)+' km/h':'--';mv.textContent=d.mv.toFixed(0)+' km/h';g.textContent=(d.gps?'GPS con se\u00f1al':'GPS sin se\u00f1al')+(d.rec?' \u00b7 grabando ruta':'');}catch(e){}setTimeout(t,250)}t()</script></body></html>)HTML";

// Nombre de ruta pedido por la web: solo se aceptan los ficheros de ruta
bool rutaValida(String &f) {
  f = web.arg("f");
  return f.startsWith("r_") && f.endsWith(".bin") && f.indexOf('/') < 0 && f.length() < 36;
}

void webRutas() {
  String h = F("<!DOCTYPE html><html lang='es'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>Rutas</title><style>body{background:#111;color:#eee;font-family:sans-serif;padding:16px}a{color:#6cf}"
               "td{padding:10px 8px;border-bottom:1px solid #333}</style></head><body><h2>Rutas grabadas</h2>");
  if (!fsOk) h += F("<p>La memoria de rutas no esta disponible.</p>");
  else {
    if (grabando) fRuta.flush();
    h += F("<table>");
    int n = 0;
    fs::File d = LittleFS.open("/");
    for (fs::File f = d.openNextFile(); f; f = d.openNextFile()) {
      String nom = f.name();
      if (!nom.startsWith("r_")) continue;
      n++;
      // r_AAAAMMDD_HHMMSS.bin
      String fecha = nom.substring(8, 10) + "/" + nom.substring(6, 8) + "/" + nom.substring(2, 6) + " " +
                     nom.substring(11, 13) + ":" + nom.substring(13, 15) + " UTC";
      h += "<tr><td>" + fecha + "<br><small>" + String(f.size() / sizeof(Reg) / 60) + " min en movimiento" +
           ((String("/") + nom) == nombreRuta ? " &middot; grabando" : "") + "</small></td>"
           "<td><a href='/ruta?f=" + nom + "'>Descargar</a></td>"
           "<td><a href='/borrar?f=" + nom + "' onclick=\"return confirm('Borrar esta ruta?')\">Borrar</a></td></tr>";
    }
    h += F("</table>");
    h += rutaPausa ? F("<p>Grabaci&oacute;n <b>parada</b>. <a href='/fin?r=1'>Reanudar grabaci&oacute;n</a></p>")
                   : F("<p>Grabaci&oacute;n activa. <a href='/fin'>Terminar ruta y parar la grabaci&oacute;n</a></p>");
    if (n == 0) h += F("<p>Todavia no hay rutas. Se graban solas al circular con se&ntilde;al GPS.</p>");
    h += "<p><small>Memoria libre: " + String((LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024) + " KB</small></p>";
  }
  h += F("<p><a href='/'>Volver</a></p></body></html>");
  web.send(200, "text/html", h);
}

// Descarga de una ruta en CSV (lo que abre la pagina del mapa)
void webRuta() {
  String f;
  if (!fsOk || !rutaValida(f)) { web.send(400, "text/plain", "ruta no valida"); return; }
  if (grabando) fRuta.flush();
  fs::File r = LittleFS.open("/" + f, "r");
  if (!r) { web.send(404, "text/plain", "no existe"); return; }
  String csv = f.substring(0, f.length() - 4) + ".csv";
  web.sendHeader("Content-Disposition", "attachment; filename=\"" + csv + "\"");
  web.setContentLength(CONTENT_LENGTH_UNKNOWN);
  web.send(200, "text/csv", "");
  web.sendContent("t_utc,lat,lon,kmh,inclinacion,acel_max_g,acel_min_g\n");
  Reg reg; char lin[96]; String bloque;
  bloque.reserve(4200);
  while (r.read((uint8_t *)&reg, sizeof(reg)) == sizeof(reg)) {
    snprintf(lin, sizeof(lin), "%lu,%.7f,%.7f,%.1f,%.1f,%.3f,%.3f\n", (unsigned long)reg.t,
             reg.lat / 1e7, reg.lon / 1e7, reg.v / 100.0, reg.lean / 10.0, reg.aMax / 1000.0, reg.aMin / 1000.0);
    bloque += lin;
    if (bloque.length() > 4000) { web.sendContent(bloque); bloque = ""; }
  }
  if (bloque.length()) web.sendContent(bloque);
  web.sendContent("");
  r.close();
}

void webBorrar() {
  String f;
  if (!fsOk || !rutaValida(f)) { web.send(400, "text/plain", "ruta no valida"); return; }
  if ((String("/") + f) == nombreRuta) {      // la que se esta grabando: se cierra antes de borrarla
    terminaRuta();
  }
  LittleFS.remove("/" + f);
  web.sendHeader("Location", "/rutas");
  web.send(303, "text/plain", "");
}

// ---------------- WiFi de casa ----------------
// Ademas de su propia WiFi, la placa puede entrar en la WiFi de casa (se configura en /wifi).
// Si a los 15 s de encender no la encuentra (en ruta), deja de buscarla para no molestar a su propia red.
uint32_t tCasa = 0;

void wifiCasaInicia() {
  wifiCasa = nvs.getString("wSsid", "");
  claveCasa = nvs.getString("wClave", "");
  if (wifiCasa == "") { WiFi.mode(WIFI_AP); return; }
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname("motolean");
  WiFi.setAutoReconnect(false);
  WiFi.begin(wifiCasa.c_str(), claveCasa.c_str());
  casaBuscando = true; tCasa = millis();
}

void wifiCasaVigila() {
  if (casaBuscando) {
    if (WiFi.status() == WL_CONNECTED) {
      casaBuscando = false; casaConectada = true;
      MDNS.begin("motolean");
      MDNS.addService("http", "tcp", 80);
    } else if (millis() - tCasa > 15000) {
      casaBuscando = false;
      WiFi.disconnect(true);
      WiFi.mode(WIFI_AP);
    }
  } else if (casaConectada && WiFi.status() != WL_CONNECTED) {
    casaConectada = false;
    MDNS.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
  }
}

void webWifi() {
  if (web.method() == HTTP_POST) {
    nvs.putString("wSsid", web.arg("s"));
    nvs.putString("wClave", web.arg("c"));
    web.send(200, "text/html", F("<meta charset='utf-8'><body style='background:#111;color:#eee;font-family:sans-serif;padding:16px'>"
                                 "<p>Guardado. La placa se reinicia.</p></body>"));
    if (grabando) fRuta.close();
    delay(800);
    ESP.restart();
    return;
  }
  String h = F("<!DOCTYPE html><html lang='es'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>WiFi de casa</title><style>body{background:#111;color:#eee;font-family:sans-serif;padding:16px}a{color:#6cf}"
               "input{font-size:18px;padding:8px;margin:6px 0;width:100%;box-sizing:border-box}button{font-size:18px;padding:10px 20px}</style></head><body>"
               "<h2>WiFi de casa</h2>");
  if (casaConectada) h += "<p>Conectada a <b>" + wifiCasa + "</b>.<br>Direcci&oacute;n: http://" + WiFi.localIP().toString() + " &middot; http://motolean.local</p>";
  else if (wifiCasa != "") h += "<p>Configurada <b>" + wifiCasa + "</b>, ahora mismo sin conexi&oacute;n.</p>";
  else h += F("<p>Sin configurar.</p>");
  h += F("<form method='post'>Nombre de la WiFi<input name='s' maxlength='32'>Clave<input name='c' type='password' maxlength='63'>"
         "<button>Guardar y reiniciar</button></form><p><small>Deja el nombre vac&iacute;o para dejar de usar la WiFi de casa.</small></p>"
         "<p><a href='/'>Volver</a></p></body></html>");
  web.send(200, "text/html", h);
}

// Diagnostico del GPS: sirve para saber si el modulo contesta
void webGps() {
  String h = F("<!DOCTYPE html><html lang='es'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<meta http-equiv='refresh' content='2'><title>GPS</title><style>body{background:#111;color:#eee;font-family:sans-serif;padding:16px}"
               "a{color:#6cf}td{padding:6px 12px 6px 0}pre{background:#000;padding:10px;overflow:auto;font-size:12px}b.ok{color:#6f6}b.mal{color:#f66}b.reg{color:#fc3}</style></head><body><h2>Diagn&oacute;stico del GPS</h2>");
  if (gpsBytes == 0) h += F("<p><b class='mal'>No llega nada del GPS.</b> Revisa la alimentaci&oacute;n (VCC a 5V, GND a G) y que el TX del GPS va al 13.</p>");
  else if (gpsFrases == 0) h += F("<p><b class='mal'>Llegan datos pero no se entienden.</b> Puede ser un mal contacto o un m&oacute;dulo configurado a otra velocidad.</p>");
  else if (!gpsFix) h += F("<p><b class='reg'>El GPS contesta, pero a&uacute;n no tiene posici&oacute;n.</b> Necesita ver el cielo; puede tardar unos minutos.</p>");
  else h += F("<p><b class='ok'>El GPS contesta y tiene posici&oacute;n.</b></p>");
  h += "<table><tr><td>Bytes recibidos</td><td>" + String(gpsBytes) + "</td></tr>"
       "<tr><td>Frases correctas</td><td>" + String(gpsFrases) + "</td></tr>"
       "<tr><td>Frases con error</td><td>" + String(gpsMalas) + "</td></tr>"
       "<tr><td>Sat&eacute;lites en uso</td><td>" + (gpsSat < 0 ? String("-") : String(gpsSat)) + "</td></tr>";
  if (gpsFix) h += "<tr><td>Posici&oacute;n</td><td>" + String(gpsLat, 6) + ", " + String(gpsLon, 6) + "</td></tr>"
                   "<tr><td>Velocidad</td><td>" + String(gpsKmh, 1) + " km/h</td></tr>";
  h += F("</table><p>&Uacute;ltimas frases recibidas:</p><pre>");
  int n = gpsUltN < 6 ? gpsUltN : 6;
  for (int i = 0; i < n; i++) { h += gpsUltimas[(gpsUltN - n + i) % 6]; h += "\n"; }
  if (n == 0) h += "(ninguna)";
  h += F("</pre><p><small>La p&aacute;gina se actualiza sola cada 2 segundos.</small></p><p><a href='/'>Volver</a></p></body></html>");
  web.send(200, "text/html", h);
}

// Ajustes desde la web: centro, calibracion completa, giro de pantalla y borrado de maximos
void paginaAjustes(const String &aviso) {
  String h = F("<!DOCTYPE html><html lang='es'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>Ajustes</title><style>body{background:#111;color:#eee;font-family:sans-serif;padding:16px}a{color:#6cf}"
               "a.b{display:block;background:#222;border-radius:10px;padding:14px;margin:10px 0;font-size:18px;text-decoration:none}"
               "p.m{background:#263;padding:12px;border-radius:10px}</style></head><body><h2>Ajustes</h2>");
  if (aviso.length()) h += "<p class='m'>" + aviso + "</p>";
  h += F("<a class='b' href='/centro'>Ajustar el centro<br><small>Con la moto recta: lo que marca ahora pasa a ser 0&deg;</small></a>"
         "<a class='b' href='/cal1'>Calibraci&oacute;n completa, paso 1<br><small>Pon la moto recta y quieta, y pulsa aqu&iacute;</small></a>"
         "<a class='b' href='/cal2'>Calibraci&oacute;n completa, paso 2<br><small>Ap&oacute;yala en la pata de cabra, y pulsa aqu&iacute;</small></a>"
         "<a class='b' href='/girar'>Girar la pantalla 180&deg;</a>"
         "<a class='b' href='/botonera'>Configurar la botonera<br><small>Despu&eacute;s de pulsar aqu&iacute;, sigue las instrucciones de la pantalla</small></a>"
         "<a class='b' href='/reset2' onclick=\"return confirm('Borrar maximos?')\">Borrar los m&aacute;ximos</a>"
         "<p><a href='/'>Volver</a></p></body></html>");
  web.send(200, "text/html", h);
}

void giraPantalla() {
  rotacion = rotacion == 0 ? 2 : 0;
  tft.setRotation(rotacion);
  nvs.putUChar("rot", rotacion);
}

void webInicia() {
  web.on("/", []() { web.send_P(200, "text/html", PAGINA); });
  web.on("/datos", []() {
    char b[240];
    snprintf(b, sizeof(b), "{\"lean\":%.1f,\"mi\":%.1f,\"md\":%.1f,\"ma\":%.2f,\"mf\":%.2f,\"al\":%.2f,\"cal\":%d,"
             "\"v\":%.1f,\"mv\":%.1f,\"gps\":%d,\"rec\":%d}",
             lean, maxIzq, maxDer, maxAcel, maxFren, accLong, calibrado ? 1 : 0,
             gpsKmh, maxVel, gpsFix ? 1 : 0, grabando ? 1 : 0);
    web.send(200, "application/json", b);
  });
  web.on("/reset", []() { borraMaximos(); web.send(200, "text/plain", "ok"); });
  web.on("/rutas", webRutas);
  web.on("/ruta", webRuta);
  web.on("/borrar", webBorrar);
  web.on("/wifi", webWifi);
  web.on("/gps", webGps);
  web.on("/ajustes", []() { paginaAjustes(""); });
  web.on("/centro", []() {
    paginaAjustes(ajustaCentro() ? F("Centro ajustado: ahora marca 0&deg;.")
                                 : F("No ajustado: la moto debe estar calibrada y a menos de 10&deg; de la vertical."));
  });
  web.on("/cal1", []() { calPaso1(); paginaAjustes(F("Paso 1 hecho. Ahora apoya la moto en la pata de cabra y pulsa el paso 2.")); });
  web.on("/cal2", []() {
    paginaAjustes(calPaso2() ? F("Calibraci&oacute;n completa hecha.")
                             : F("No se pudo calibrar: haz antes el paso 1 y comprueba que la moto queda bien inclinada sobre la pata."));
  });
  web.on("/botonera", []() {
    web.sendHeader("Location", "/ajustes"); web.send(302, "text/plain", "");
    kbAprende();
  });
  web.on("/girar", []() { giraPantalla(); paginaAjustes(F("Pantalla girada.")); });
  web.on("/reset2", []() { borraMaximos(); paginaAjustes(F("M&aacute;ximos borrados.")); });
  web.on("/fin", []() {
    rutaPausa = !web.hasArg("r");
    if (rutaPausa) terminaRuta();
    web.sendHeader("Location", "/rutas");
    web.send(303, "text/plain", "");
  });
  web.begin();
}

// ---------------- Arranque ----------------
void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);            // sin ordenador conectado, el eco no debe frenar el programa
#endif
  pinMode(PIN_SYS_EN, OUTPUT); digitalWrite(PIN_SYS_EN, HIGH);   // mantiene la alimentacion con bateria
  pinMode(PIN_ZUMB, OUTPUT); digitalWrite(PIN_ZUMB, LOW);
  bBoot.begin(); bPwr.begin();

  nvs.begin("moto", false);
  cargaTodo();
  for (uint8_t i = 0; i < 3; i++) pinMode(KB_PIN[i], INPUT_PULLUP);
  kbPar[0] = nvs.getUChar("kbA", 255); kbPar[1] = nvs.getUChar("kbR", 255);
  bAma.begin(); bRojo.begin(); bDos.begin();
  if (rotacion != 2) rotacion = 0;      // esta pantalla solo se usa en vertical: normal o girada 180

  pinMode(PIN_BL, OUTPUT); digitalWrite(PIN_BL, HIGH);
  tft.init();
  tft.setRotation(rotacion);
  tft.fillScreen(TFT_BLACK);
  spr.setColorDepth(16);
  if (!spr.createSprite(ANCHO, ALTO)) {  // sin memoria para 16 bits: se usa la version de 8 bits
    spr.setColorDepth(8);
    spr.createSprite(ANCHO, ALTO);
  }

  Wire.setPins(PIN_SDA, PIN_SCL);
  Wire.begin();
  Wire.setClock(400000);
  imuOk = imuInicia();
  if (!imuOk) {
    mensaje("ERROR", "No encuentro", "el sensor IMU", "", TFT_RED);
    while (true) delay(1000);
  }

  Serial1.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  // Memoria de rutas: la particion grande de esta placa; la primera vez se le da formato
  fsOk = LittleFS.begin(true, "/littlefs", 10, "ffat") || LittleFS.begin(true);

  wifiCasaInicia();
  WiFi.softAP(WIFI_NOMBRE, WIFI_CLAVE);
  webInicia();

  // Al encender: si la moto esta quieta, afinamos el cero del giroscopio
  mensaje("MotoLean " VERSION, "Arrancando...", "", "WiFi: MotoLean", TFT_GREEN);
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

  bBoot.leer(); bPwr.leer();
  if (bBoot.larga) { calibrar(); tAnt = micros(); }
  if (bBoot.corta) giraPantalla();
  bAma.leer(); bRojo.leer(); bDos.leer();
  if (kbPar[0] > 2 && kbLee()) { kbAprende(); tAnt = micros(); }   // primera pulsacion: configurar la botonera
  if (bAma.larga) { calibrar(); tAnt = micros(); }
  if (bDos.larga) giraPantalla();
  if (bRojo.corta) {
    borraMaximos();
    mensaje("MAXIMOS", "borrados", "", "", TFT_GREEN);
    delay(800); tAnt = micros();
  }
  if (bPwr.corta || bAma.corta) {           // ajuste de centro
    if (ajustaCentro()) mensaje("CENTRO", "ajustado", "ahora marca 0", "", TFT_GREEN);
    else mensaje("CENTRO", "no ajustado", "mas de 10 grados", "", TFT_RED);
    delay(1000); tAnt = micros();
  }
  if (bPwr.larga || bRojo.larga) {                         // parar / reanudar la grabacion de rutas
    rutaPausa = !rutaPausa;
    if (rutaPausa) terminaRuta();
    if (rutaPausa) mensaje("RUTA", "terminada", "y guardada", "ROJO 1,5 s: reanudar", TFT_GREEN);
    else mensaje("RUTA", "grabacion", "activada", "", TFT_GREEN);
    delay(1200); tAnt = micros();
  }

  gpsLee();
  if (gpsNueva) registra();

  web.handleClient();
  wifiCasaVigila();

  uint32_t ms = millis();
  if (ms - tDib > 70) {
    tDib = ms;
    if (calibrado) dibuja();
    else mensaje("SIN CALIBRAR", "Manten BOOT 3 s", "para calibrar");
  }
  // Guardado automatico (como mucho cada 5 s, y solo si ha cambiado algun maximo)
  if (hayQueGuardar && ms - tUltimoGuardado > 5000) guardaMaximos();

  delay(3);
}
