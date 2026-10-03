# GPS, rutas y WiFi de casa

> Esta parte está compilada pero **todavía no se ha probado con el GPS conectado**.

## Conexión del GPS

Módulo **GY-GPS6MV2** (u-blox NEO-6M) con su antena cerámica. Los cuatro agujeros que usa están seguidos en el mismo borde de la placa:

| Módulo GPS | Placa S3 Mini Pro |
|---|---|
| VCC | 5V |
| GND | G |
| TX | 13 |
| RX | 14 |

La antena debe mirar al cielo y no tener metal encima. El NEO-6M tarda entre 30 segundos y varios minutos en tener señal al encender.

## Qué cambia en la pantalla

- Abajo aparece la **velocidad** en km/h, o «sin GPS» mientras no hay señal.
- Un **punto rojo** junto a la velocidad indica que se está grabando una ruta.

## LED de estado

El LED de colores de la placa indica el estado del GPS. Se ve por la mirilla de 3,5 mm de la tapa, a la derecha de la pantalla, en su mitad inferior.

| Color | Significado |
|---|---|
| Rojo | El GPS no contesta |
| Amarillo | Contesta, pero aún no tiene posición |
| Verde | Tiene posición |
| Verde con un guiño cada 2 s | Tiene posición y está grabando la ruta |

El brillo se ajusta con `LED_BRILLO` al principio del código. El módulo GPS tiene además su propio LED, que parpadea una vez por segundo cuando hay posición, pero queda tapado dentro de la caja.

## Grabación de rutas

- La ruta empieza sola cuando hay señal GPS y la moto pasa de 5 km/h.
- Se guarda un punto por segundo: hora, posición, velocidad, mayor inclinación del segundo y mayor aceleración y frenada del segundo.
- Parado más de 5 segundos deja de guardar puntos, para no gastar memoria.
- Cada vez que se enciende la placa se crea una ruta nueva.
- Caben unas 19 horas en movimiento. Cuando falta sitio se borra la ruta más antigua.
- Los datos se vuelcan a la memoria cada 10 segundos: si se corta la corriente se pierden como mucho esos segundos.

## Descargar y ver una ruta

1. Entra en la página de la placa y pulsa **Rutas grabadas**.
2. Pulsa **Descargar** en la ruta: baja un archivo `.csv`.
3. Abre [`mapa/index.html`](../mapa/index.html) en un navegador con internet y carga ese archivo con **Abrir ruta**.

El mapa muestra el trazado coloreado por inclinación o por velocidad, marca las frenadas fuertes (desde 0,5 g) y los acelerones (desde 0,4 g), y resume distancia, tiempo, velocidades y máximos. Pulsando sobre el trazado se ven los datos de ese punto. En `mapa/ruta_de_ejemplo.csv` hay una ruta inventada para probarlo.

Columnas del archivo: `t_utc` (segundos desde 1970, hora UTC), `lat`, `lon`, `kmh`, `inclinacion` (grados, positivo a la derecha), `acel_max_g`, `acel_min_g`.

## WiFi de casa

La placa mantiene siempre su propia WiFi (`MotoLean`). Además puede entrar en la WiFi de casa para acceder a ella desde cualquier ordenador de la red:

1. Conéctate a `MotoLean`, abre <http://192.168.4.1> y pulsa **WiFi de casa**.
2. Escribe el nombre y la clave de tu WiFi y guarda. La placa se reinicia.
3. Desde un ordenador de casa entra en <http://motolean.local>. Si no responde, usa la dirección IP que muestra esa misma página o la lista de dispositivos del router.

Detalles:

- La WiFi de casa solo se busca durante los 15 primeros segundos tras encender. En ruta, pasado ese tiempo, la placa se queda solo con su propia red.
- Solo funciona con redes de 2,4 GHz.
- La clave se guarda en la memoria de la placa, no en el código ni en este repositorio.
