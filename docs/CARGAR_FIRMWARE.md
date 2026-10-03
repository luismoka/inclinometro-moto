# Cargar el firmware en la placa

El archivo [`firmware/bin/MotoLean_firmware.bin`](../firmware/bin/MotoLean_firmware.bin) contiene el programa completo (arranque, tabla de particiones y aplicación en un solo archivo). Se graba desde el navegador, sin instalar Arduino ni controladores.

## Qué necesitas

- Un ordenador con **Chrome o Edge** (Firefox y Safari no sirven: no tienen acceso al puerto serie).
- Un cable USB-C **de datos** (algunos cables solo cargan).

## Pasos

1. Descarga `MotoLean_firmware.bin`.
2. Mantén pulsado el **botón azul (IO0)** de la placa mientras la conectas por USB, y suéltalo después. Así entra en modo de carga.
3. Abre <https://espressif.github.io/esptool-js/>.
4. Pulsa **Connect** y elige el puerto que aparezca ("USB JTAG/serial debug unit" o "COM…").
5. En **Flash Address** escribe `0x0`.
6. Elige el archivo `MotoLean_firmware.bin` y pulsa **Program**.
7. Cuando termine, desconecta y vuelve a conectar el USB.

La pantalla debe mostrar **"SIN CALIBRAR – Mantén AZUL 3 s"** la primera vez.

> No pulses **Erase Flash** al actualizar: borraría la calibración y los máximos guardados.

## Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| No aparece ningún puerto | La placa no está en modo de carga, o el cable es solo de carga | Repite el paso 2 o cambia de cable |
| Pantalla negra o colores raros | La placa lleva la otra pantalla (GC9A01) | Compilar con `Setup404_LOLIN_S3_MINI_PRO.h`, ver [COMPILAR.md](COMPILAR.md) |
| "ERROR – No encuentro el sensor IMU" | El sensor no responde en la dirección `0x6B` | Revisar `IMU_ADDR` en el código y recompilar |
| Se queda en "Connecting…" | El puerto está ocupado por otro programa | Cierra Arduino IDE u otros monitores serie |

## Datos del archivo

| Dato | Valor |
|---|---|
| Dirección de grabado | `0x0` |
| Chip | ESP32-S3, flash de 4 MB |
| Modo y frecuencia de flash | DIO, 80 MHz |
| Contenido | bootloader (`0x0`), particiones (`0x8000`), boot_app0 (`0xE000`), aplicación (`0x10000`) |
