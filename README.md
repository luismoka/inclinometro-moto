# Inclinómetro para moto (MotoLean)

**Versión 3.0**

> **Nuevo:** versión para la placa Waveshare ESP32-S3-LCD-1.69 (pantalla de 1,69"), todavía sin probar en placa real. Ver [docs/PLACA_169.md](docs/PLACA_169.md). El resto de este documento describe la versión para la LOLIN S3 Mini Pro (firmware 2.3).

Inclinómetro casero para moto basado en una placa **LOLIN S3 Mini Pro** (ESP32-S3 con pantalla de 0,85" y sensor de movimiento integrado) y un GPS **NEO-6M**. Muestra la inclinación, la aceleración y la velocidad, graba las rutas y las enseña después sobre un mapa.

![Pantalla del inclinómetro](docs/img/pantalla.png)

## Qué hace

- **Inclinación en directo**, con arco de color: azul hasta 25°, amarillo hasta 40°, rojo a partir de ahí.
- **Máxima inclinación** a izquierda y derecha.
- **Aceleración y frenada** en g, con barra en directo y máximo alcanzado.
- **Velocidad GPS** en pantalla y velocidad máxima.
- **Grabación de rutas**: posición, velocidad, inclinación, aceleración y frenada, un punto por segundo.
- **Mapa**: la ruta se ve después coloreada por inclinación o velocidad, con las frenadas y los acelerones marcados.
- **Web propia**: datos en directo, descarga de rutas y configuración desde el móvil o el ordenador, sin instalar ninguna app.
- **WiFi de casa**: en casa se une a la red doméstica y se entra en `http://motolean.local`.
- **Memoria**: máximos, calibración y rutas se conservan al apagar.
- **Montaje libre**: una calibración de dos pasos permite colocar la placa en cualquier posición.
- **Caja desmontable**: la base se queda en la moto y la electrónica se quita y se pone sin herramientas.

Sin GPS el inclinómetro funciona igual, sin velocidad ni rutas, y no hace falta soldar nada.

## Novedades de la versión 2.0

| Novedad | Dónde se explica |
|---|---|
| GPS: velocidad y grabación de rutas | [docs/GPS_Y_RUTAS.md](docs/GPS_Y_RUTAS.md) |
| Página de mapa para ver las rutas | [docs/WEB.md](docs/WEB.md) |
| Web ampliada: rutas y WiFi de casa | [docs/WEB.md](docs/WEB.md) |
| Caja desmontable con GPS | [docs/CAJA_GPS.md](docs/CAJA_GPS.md) |
| Pulsadores para la tapa | [docs/SOPORTE.md](docs/SOPORTE.md) |
| Cajas ajustadas con medidas de calibre | [docs/SOPORTE.md](docs/SOPORTE.md) |

El detalle completo está en el [CHANGELOG.md](CHANGELOG.md).

## Puesta en marcha

1. **Reunir el material**: [docs/MATERIALES.md](docs/MATERIALES.md).
2. **Cargar el firmware** ya compilado desde el navegador: [docs/CARGAR_FIRMWARE.md](docs/CARGAR_FIRMWARE.md).
3. **Soldar el GPS** (opcional): [docs/GPS_Y_RUTAS.md](docs/GPS_Y_RUTAS.md).
4. **Imprimir y montar la caja**: [docs/CAJA_GPS.md](docs/CAJA_GPS.md) con GPS, o [docs/SOPORTE.md](docs/SOPORTE.md) sin él.
5. **Calibrar y usar**: [docs/USO.md](docs/USO.md).

## Documentación

| Documento | Contenido |
|---|---|
| [docs/MATERIALES.md](docs/MATERIALES.md) | Lista de todo lo que se utiliza: componentes, tornillería, herramientas y programas |
| [docs/CARGAR_FIRMWARE.md](docs/CARGAR_FIRMWARE.md) | Cómo grabar el `.bin` en la placa sin instalar nada |
| [docs/USO.md](docs/USO.md) | Botones, calibración y pantalla |
| [docs/WEB.md](docs/WEB.md) | Páginas web de la placa y página del mapa |
| [docs/GPS_Y_RUTAS.md](docs/GPS_Y_RUTAS.md) | Conexión del GPS, grabación de rutas y WiFi de casa |
| [docs/CAJA_GPS.md](docs/CAJA_GPS.md) | Caja desmontable con GPS: base fija y módulo de quita y pon |
| [docs/SOPORTE.md](docs/SOPORTE.md) | Soporte sencillo sin GPS y pulsadores |
| [docs/FUNCIONAMIENTO.md](docs/FUNCIONAMIENTO.md) | Cómo se calcula la inclinación y qué limitaciones tiene |
| [docs/HARDWARE.md](docs/HARDWARE.md) | Pines y componentes de la placa |
| [docs/COMPILAR.md](docs/COMPILAR.md) | Cómo compilar el programa desde el código fuente |
| [CHANGELOG.md](CHANGELOG.md) | Historial de versiones |

## Estructura del repositorio

```
firmware/
  moto_inclinometro/   Código fuente (Arduino)
  bin/                 Firmware compilado, listo para grabar
soporte/               STL de las cajas y scripts que los generan
mapa/                  Página para ver una ruta grabada sobre el mapa
herramientas/          Scripts que generan las imágenes de la documentación
docs/                  Documentación e imágenes
```

## Estado del proyecto

| Parte | Estado |
|---|---|
| Inclinación, aceleración y pantalla | Cargado y funcionando en la placa. Pendiente de probar en marcha |
| GPS, rutas y WiFi de casa | Compilado; **sin probar** con el GPS conectado |
| Página del mapa | Probada con una ruta inventada; el fondo de mapa está pendiente de confirmar |
| Soporte sencillo (sin GPS) | Primera versión impresa; la versión corregida con medidas de calibre está sin imprimir |
| Caja desmontable con GPS | Diseñada; **sin imprimir** |
| Medida del manillar | Calculada (tubo de 28,6 mm), sin medir en la moto |

## Aviso de seguridad

Es un proyecto de aficionado y la medida es orientativa. No mires la pantalla en plena curva ni la uses como referencia para apurar la inclinación.

## Apoyar el proyecto

El proyecto es libre y gratuito. Si te ha sido útil y quieres invitarme a un café, puedes hacerlo en [paypal.me/luismoka](https://paypal.me/luismoka).

## Licencia

Proyecto publicado bajo licencia [MIT](LICENSE).
