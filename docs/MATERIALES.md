# Materiales

Lista de todo lo que se utiliza en el proyecto.

## Electrónica

| Cantidad | Pieza | Detalle |
|---|---|---|
| 1 | Placa LOLIN (WEMOS) S3 Mini Pro v1.1.0 | ESP32-S3 con pantalla de 0,85" (versión ST7789), sensor de movimiento QMI8658 y 3 botones |
| 1 | Módulo GPS GY-GPS6MV2 | u-blox NEO-6M, con su antena cerámica de 25 × 25 mm y el cable que trae |
| 4 | Cables finos de unos 8 cm | Para unir el GPS a la placa (VCC, GND, TX, RX) |
| 1 | Cable USB-C de datos | Para cargar el firmware y para alimentar la placa en la moto |
| 1 | Toma USB de 5 V en la moto | Alimentación |

El GPS es opcional: sin él no hay velocidad ni rutas.

## Caja y fijación

| Cantidad | Pieza | Detalle |
|---|---|---|
| — | Filamento PETG | Aguanta sol y calor mejor que el PLA |
| 4 | Tornillos M2 × 8–10 mm | Cierran la tapa; roscan directamente en el plástico |
| 1 | Tornillo M3 × 30 mm o brida fina | Seguro opcional del módulo desmontable |
| 2 | Bridas de hasta 4,8 mm de ancho | Sujetan la base al manillar |
| 1 | Tira de goma de 0,5–1 mm | Entre la base y el manillar: evita que resbale y amortigua vibraciones |
| 1 | Cinta de doble cara de espuma | Fija la antena dentro de la caja |
| 1 | Lámina transparente (opcional) | Para cerrar la ventana de la pantalla frente a la lluvia |

## Piezas impresas

Caja desmontable con GPS:

| Archivo | Pieza |
|---|---|
| `soporte/gps_base.stl` | Base de manillar con carril y pestaña |
| `soporte/gps_modulo.stl` | Módulo de la electrónica |
| `soporte/gps_tapa.stl` | Tapa |
| `soporte/soporte_pulsadores_x3.stl` | Tres pulsadores |

Soporte sencillo, sin GPS:

| Archivo | Pieza |
|---|---|
| `soporte/soporte_cuerpo.stl` | Cuerpo |
| `soporte/soporte_tapa.stl` | Tapa |
| `soporte/soporte_pulsadores_x3.stl` | Tres pulsadores |

## Herramientas

- Impresora 3D y laminador (en este proyecto, Bambu Studio).
- Soldador y estaño, para el GPS.
- Calibre, para comprobar medidas.
- Destornillador pequeño.
- Ordenador con Chrome o Edge, para cargar el firmware.

## Programas y servicios

| Para qué | Qué se usa |
|---|---|
| Cargar el firmware | [esptool-js](https://espressif.github.io/esptool-js/), desde el navegador |
| Compilar el firmware | arduino-cli 1.1.1 o Arduino IDE 2, con el paquete esp32 de Espressif 2.0.9 |
| Pantalla | Librería [TFT_eSPI de WEMOS](https://github.com/wemos/TFT_eSPI) 2.5.44 |
| WiFi, web, memoria, rutas y `motolean.local` | Librerías incluidas en el paquete esp32: WiFi, WebServer, Preferences, LittleFS, ESPmDNS, Wire |
| Mapa | [Leaflet](https://leafletjs.com) 1.9.4 |
| Fondos del mapa | IGN España (CC BY 4.0, ign.es) y Esri |
| Diseño de las cajas | Python con trimesh, manifold3d y numpy |
| Imágenes de la documentación | Python con Pillow |
