# Hardware

## Placa: LOLIN S3 Mini Pro v1.1.0

| Característica | Valor |
|---|---|
| Microcontrolador | ESP32-S3FH4R2 (4 MB de flash, 2 MB de PSRAM) |
| Pantalla | TFT de 0,85", 128 × 128, ST7789 (existe otra versión con GC9A01) |
| Sensor de movimiento | QMI8658C, 6 ejes, por I2C |
| Botones | 3 |
| Medidas | 34,3 × 25,4 mm |
| Alimentación | 5 V por USB-C o por el pin 5V |

## Pines usados

| Función | Pin |
|---|---|
| I2C SDA | IO12 |
| I2C SCL | IO11 |
| Dirección I2C del sensor | `0x6B` |
| GPS: TX del módulo | IO13 |
| GPS: RX del módulo | IO14 |
| Botón azul | IO0 |
| Botón naranja | IO47 |
| Botón verde | IO48 |
| Pantalla: retroiluminación | IO33 |
| Pantalla: MOSI / SCK | IO38 / IO40 |
| Pantalla: CS / DC / RST | IO35 / IO36 / IO34 |

Los pines de la pantalla los fija la librería TFT_eSPI de WEMOS (archivo `Setup406_LOLIN_S3_MINI_PRO_ST7789.h`).

## Referencias

- [Documentación oficial de la S3 Mini Pro](https://www.wemos.cc/en/latest/s3/s3_mini_pro.html)
- [Ejemplo Arduino oficial](https://github.com/wemos/D1_mini_Examples/tree/master/examples/06.S3_MINI_PRO/s3_mini_pro)
- [TFT_eSPI de WEMOS](https://github.com/wemos/TFT_eSPI)
