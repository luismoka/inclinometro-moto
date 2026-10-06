# Historial de cambios

## 3.1.0 – 2026-10-06

- Botonera externa de membrana de 2 botones en GPIO16, GPIO2 y GPIO3, con aprendizaje automático de los hilos.
- Amarillo: centro (breve) y calibración completa (3 s). Rojo: borrar máximos (breve) y terminar o reanudar ruta (1,5 s). Los dos: girar pantalla.
- Documentados los colores del conector de 12 hilos.

## 3.0.0 – 2026-10-04

Versión para la placa Waveshare ESP32-S3-LCD-1.69 (`firmware/moto_inclinometro_169`). Compila; sin probar en placa real.

- Pantalla de 240×280: número de inclinación más grande, sin velocidad.
- Fondo rojo parpadeante al pasar de 40°. Zona central de ±4° marcada como RECTA.
- Iconos de GPS (rojo parpadeante / verde) y WiFi (gris / verde).
- BOOT breve gira la pantalla; BOOT 3 s calibra en dos pasos; PWR breve ajusta el centro (solo si el desvío es menor de ±10°); PWR 1,5 s termina o reanuda la ruta.
- Los máximos se ponen a cero al empezar cada ruta.
- Página web `/ajustes` con calibración, ajuste de centro y giro de pantalla.

## 2.3.0 – 2026-10-03

- Terminar la ruta a mano: botón verde mantenido 1,5 s o enlace en la página de rutas. La misma acción reanuda la grabación.
- LED: corregidos el rojo y el verde, que salían intercambiados; azul cuando la grabación está parada a mano.

## 2.2.0 – 2026-10-03

- El LED de colores de la placa indica el estado del GPS: rojo, amarillo, verde y guiño al grabar.
- Tapas con una mirilla de 3,5 mm para ver ese LED.

## 2.1.0 – 2026-10-03

- Página `/gps` de diagnóstico: indica si el GPS contesta, con contadores, satélites y las últimas frases recibidas.
- Las frases del GPS salen también por el puerto serie USB (115200 baudios).

## 2.0.1 – 2026-10-03

- Tapas: ventana de pantalla 0,6 mm más ancha por el lado de los botones y por el opuesto, tras probar la tapa impresa.
- Tapas exportadas con la cara exterior hacia la cama, para que el rebaje interior salga limpio.
- Enlace de donación.

## 2.0.0 – 2026-10-03

Versión que reúne todo lo añadido desde la 0.2: GPS, rutas, web ampliada y caja desmontable.

Firmware:

- GPS NEO-6M: velocidad en pantalla (sustituye a la aceleración en directo) y velocidad máxima.
- Grabación automática de rutas en la memoria de la placa, un punto por segundo, con unas 19 horas de capacidad.
- Web: páginas de rutas (descarga en CSV y borrado) y de WiFi de casa; velocidad y estado del GPS en la página de inicio.
- Conexión opcional a la WiFi doméstica, con acceso por `http://motolean.local`.
- El número de versión aparece al arrancar y en la web.

Mapa:

- Página `mapa/index.html` para ver una ruta: trazado por inclinación o velocidad, marcas de frenadas y acelerones, resumen y tres fondos de mapa.

Cajas:

- Caja desmontable con GPS: base fija con carril en cola de milano y pestaña, módulo con placa, GPS y antena, y seguro opcional con tornillo M3.
- Pulsadores para la tapa, que sobresalen 3 mm.
- Tapas y huecos ajustados con medidas de calibre: botones cada 7,1 mm, pantalla a 0,5 mm de la tapa, apoyos para que la placa quede recta y ventana en bisel.

Documentación:

- Nuevos: `MATERIALES.md`, `WEB.md`, `GPS_Y_RUTAS.md` y `CAJA_GPS.md`.
- README reorganizado.

## 0.5.1 – 2026-10-03

- Las dos cajas: hueco de la placa de 9 a 7,45 mm para acercar la pantalla a la tapa, y apoyos para que la placa no quede torcida.
- Tapas con ventana en bisel y rebaje interior para los pulsadores; pulsadores con ala de 0,8 mm.

## 0.5.0 – 2026-10-03

- Caja desmontable con GPS: base fija con carril y pestaña, y módulo con placa, GPS y antena.
- Mapa: fondos de IGN y Esri.

## 0.4.0 – 2026-10-03

- GPS NEO-6M: velocidad en pantalla y velocidad máxima.
- Grabación de rutas en la memoria de la placa y descarga en CSV.
- Página `mapa/index.html` para ver la ruta sobre el mapa.
- Conexión opcional a la WiFi de casa (`http://motolean.local`).

## 0.3.0 – 2026-10-03

- Pulsadores para la tapa, que sobresalen 3 mm.
- Agujeros de la tapa reducidos a 5,0 mm para retener mejor los pulsadores.

## 0.2.1 – 2026-10-03

- Tapa del soporte corregida con medidas de calibre: botones cada 7,1 mm y ventana de 16,5 mm recentrada. El cuerpo no cambia.

## 0.2.0 – 2026-10-03

- Nueva pantalla: arco de inclinación con marcas de máximo y barras laterales de frenada y aceleración.
- Aceleración longitudinal en directo.
- Soporte de manillar a 45° para imprimir en 3D (cuerpo y tapa).
- Documentación del proyecto.

## 0.1.0 – 2026-10-03

- Primera versión: inclinación en directo, máximos a cada lado, máximos de aceleración y frenada.
- Calibración en dos pasos para cualquier posición de montaje.
- Máximos y calibración guardados en memoria.
- Página web para el móvil por WiFi propia.
