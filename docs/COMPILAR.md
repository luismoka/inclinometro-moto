# Compilar desde el código fuente

Solo hace falta si se quiere modificar el programa. Para usarlo tal cual basta con [cargar el `.bin`](CARGAR_FIRMWARE.md).

## Con Arduino IDE

1. Instala Arduino IDE 2.
2. En *Herramientas → Placa → Gestor de placas*, instala **esp32** de Espressif.
3. Descarga la librería **TFT_eSPI de WEMOS** desde <https://github.com/wemos/TFT_eSPI> (*Code → Download ZIP*) y añádela con *Programa → Incluir biblioteca → Añadir biblioteca .ZIP*.
4. En la carpeta de esa librería, edita `User_Setup_Select.h`:
   - comenta `#include <User_Setup.h>`
   - descomenta `#include <User_Setups/Setup406_LOLIN_S3_MINI_PRO_ST7789.h>`
   - si la placa lleva pantalla GC9A01, usa `Setup404_LOLIN_S3_MINI_PRO.h` en su lugar.
5. Abre `firmware/moto_inclinometro/moto_inclinometro.ino`.
6. Elige la placa **LOLIN S3 Mini** (o **LOLIN S3 Mini Pro** si tu versión del paquete la incluye) y el puerto.
7. En *Herramientas*, pon **USB CDC On Boot: Enabled** para ver el puerto serie por USB.
8. Pulsa **Subir**. Si no aparece el puerto, conecta la placa con el botón azul pulsado.

## Entorno con el que se generó el `.bin` publicado

| Componente | Versión |
|---|---|
| arduino-cli | 1.1.1 |
| Paquete esp32 (Espressif) | 2.0.9 |
| Placa (FQBN) | `esp32:esp32:lolin_s3_mini:CDCOnBoot=cdc` (puerto serie por USB) |
| TFT_eSPI (WEMOS) | 2.5.44, con `Setup406` |

Comandos equivalentes:

```bash
arduino-cli compile -b esp32:esp32:lolin_s3_mini:CDCOnBoot=cdc --output-dir build firmware/moto_inclinometro

esptool.py --chip esp32s3 merge_bin -o MotoLean_firmware.bin \
  --flash_mode dio --flash_freq 80m --flash_size 4MB \
  0x0     build/moto_inclinometro.ino.bootloader.bin \
  0x8000  build/moto_inclinometro.ino.partitions.bin \
  0xe000  <paquete esp32>/tools/partitions/boot_app0.bin \
  0x10000 build/moto_inclinometro.ino.bin
```

No hacen falta más librerías: el sensor y el GPS se leen directamente, y el resto (WiFi, WebServer, Preferences, LittleFS, ESPmDNS, Wire) viene con el paquete esp32.

## Ajustes rápidos en el código

| Constante | Para qué sirve |
|---|---|
| `WIFI_NOMBRE`, `WIFI_CLAVE` | Red WiFi que crea la placa |
| `LEAN_MIN_REG`, `LEAN_MAX_REG` | Rango de inclinación que cuenta como máximo |
| `G_MAX_REG` | Aceleración máxima que se acepta como válida |
| `IMU_ADDR` | Dirección I2C del sensor |
