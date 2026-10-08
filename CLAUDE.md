# MotoLean — inclinómetro para moto

Inclinómetro casero con ESP32-S3: firmware Arduino, cajas impresas en 3D generadas con scripts de Python, página de mapa y documentación. Todo el proyecto (código, comentarios, documentación y mensajes de commit) está en **español**.

## Estructura

| Carpeta | Contenido |
|---|---|
| `firmware/moto_inclinometro_169/` | Firmware actual, para la Waveshare ESP32-S3-LCD-1.69 (240×280). Serie de versiones 3.x |
| `firmware/moto_inclinometro/` | Firmware anterior, para la LOLIN S3 Mini Pro (0,85"). Serie 2.x |
| `firmware/bin/` | Binarios compilados listos para grabar (**generados**) |
| `soporte/` | Scripts `generar_*.py` y los STL/STEP que producen (**generados**) |
| `herramientas/` | Scripts que dibujan las imágenes de `docs/img/` |
| `mapa/` | Página HTML que muestra una ruta grabada (CSV) sobre un mapa |
| `docs/` | Documentación para el usuario final |

## Dos firmwares casi iguales

Los dos `.ino` comparten la mayor parte del código (sensor, GPS, rutas, web, WiFi) y difieren en pantalla, pines y botones. Al corregir un fallo o añadir algo en la parte común de uno, comprueba si corresponde también en el otro y dilo expresamente: o se aplica en los dos, o se explica por qué no.

Cada firmware tiene su propio `#define VERSION`. No se igualan entre sí.

## Archivos generados: no editar a mano

- `soporte/**/*.stl`, `soporte/**/*.step` → se regeneran con `python soporte/generar_<nombre>.py`
- `docs/img/*.png` → se regeneran con los scripts de `herramientas/` y de `soporte/`
- `firmware/bin/*.bin` → salen de compilar; nunca se modifican

Todos los scripts se ejecutan **desde la raíz del repositorio** (usan rutas relativas). Necesitan `numpy`, `trimesh`, `cadquery` y `Pillow`. Los de `herramientas/` usan la fuente DejaVu Sans Bold con una ruta de Linux; en Windows hay que ajustarla o ejecutarlos en un entorno Linux.

Tras regenerar una caja, comprueba que cada STL es una malla cerrada (`trimesh.load(f).is_watertight`) antes de darla por buena.

## Compilar

El autor compila y graba con **Arduino IDE 2** en Windows; no des por hecho que `arduino-cli` está instalado en su ordenador. La configuración completa está en `docs/COMPILAR.md` (LOLIN) y `docs/PLACA_169.md` (Waveshare). Resumen:

| Firmware | Placa en el IDE | Opciones |
|---|---|---|
| `moto_inclinometro_169` | ESP32S3 Dev Module | USB CDC On Boot activado, Flash 16MB, partición `16M Flash (3MB APP/9.9MB FATFS)`, PSRAM OPI. TFT_eSPI con `Setup_Waveshare_169.h` |
| `moto_inclinometro` | LOLIN S3 Mini | USB CDC On Boot activado. TFT_eSPI de WEMOS con `Setup406` |

Si en la sesión hay `arduino-cli` (por ejemplo en la nube), los comandos equivalentes son:

```
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi firmware/moto_inclinometro_169
arduino-cli compile -b esp32:esp32:lolin_s3_mini:CDCOnBoot=cdc --output-dir build firmware/moto_inclinometro
```

El `.bin` publicado es un único fichero unido (bootloader + particiones + aplicación) que se graba en la dirección `0x0`; el `merge_bin` está descrito en `docs/COMPILAR.md`.

No afirmes que algo "funciona" si solo se ha compilado. La documentación distingue siempre entre *compila*, *cargado en la placa* y *probado en marcha*; mantén esa distinción.

## Versiones

Para publicar una versión usa la skill `/publicar-version`, que conoce todos los sitios donde aparece el número.

## Documentación

- Cualquier cambio visible para el usuario (botones, pantalla, pines, páginas web, piezas) se refleja en el documento correspondiente de `docs/` en el mismo commit.
- Cada cambio de firmware lleva su entrada en `CHANGELOG.md`.
- Estilo: frases cortas, tablas para pines y botones, sin jerga. Va dirigida a alguien que monta el aparato, no a un programador.
- La tabla "Estado del proyecto" de `README.md` y la sección "Pendiente de comprobar" de `docs/PLACA_169.md` se actualizan cuando algo pasa de compilado a probado.

## Forma de trabajar

- El autor quiere **ver el diseño dibujado** (pantalla, caja, soporte) antes de recibir los archivos o de que se suban: genera primero la imagen y enséñasela.
- Mensajes de commit en español, cortos, con el estilo del historial (`Caja 1,69: mallas STL cerradas`).
- Tras subir cambios a GitHub, el autor actualiza su copia local con `git pull`.
