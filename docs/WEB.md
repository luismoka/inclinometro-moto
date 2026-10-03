# Web

El proyecto tiene dos partes web: las páginas que sirve la propia placa y la página del mapa, que se abre en un ordenador.

## Cómo entrar en la placa

| Situación | Red WiFi | Dirección |
|---|---|---|
| En cualquier sitio | `MotoLean` (clave `moto1234`) | <http://192.168.4.1> |
| En casa, con la WiFi de casa configurada | La de casa | <http://motolean.local> |

Hay que escribir `http://` delante: con `https` no funciona. Si el móvil avisa de que la red `MotoLean` no tiene internet, es normal; hay que elegir mantener la conexión.

## Páginas de la placa

### Inicio (`/`)

Datos en directo, actualizados cuatro veces por segundo:

- Inclinación actual y lado.
- Máxima a izquierda y derecha.
- Máxima aceleración y frenada.
- Velocidad y velocidad máxima.
- Aceleración en directo.
- Estado del GPS y si se está grabando una ruta.
- Botón para borrar los máximos.
- Enlaces a **Rutas grabadas** y **WiFi de casa**.

### Rutas grabadas (`/rutas`)

Lista de las rutas guardadas, con fecha y hora (UTC), minutos en movimiento y memoria libre. Cada ruta se puede **descargar** en CSV o **borrar**.

### WiFi de casa (`/wifi`)

Formulario para escribir el nombre y la clave de la WiFi doméstica. Al guardar, la placa se reinicia. Dejando el nombre vacío deja de usarla. Los detalles están en [GPS_Y_RUTAS.md](GPS_Y_RUTAS.md#wifi-de-casa).

### Diagnóstico del GPS (`/gps`)

Dice si el módulo GPS contesta: bytes y frases recibidos, satélites en uso, posición y las últimas frases tal como llegan. Se actualiza sola cada 2 segundos.

| Lo que muestra | Qué significa |
|---|---|
| No llega nada | Sin alimentación, o el TX del GPS no está en el 13 |
| Llegan datos pero no se entienden | Mal contacto u otra velocidad de comunicación |
| Contesta, sin posición | Funciona; le falta ver el cielo o más tiempo |
| Contesta y tiene posición | Todo correcto |

Las mismas frases salen también por el puerto serie USB, a 115200 baudios.

### Direcciones internas

| Dirección | Qué devuelve |
|---|---|
| `/datos` | Los datos en directo, en JSON |
| `/reset` | Borra los máximos |
| `/ruta?f=NOMBRE` | Una ruta en CSV |
| `/borrar?f=NOMBRE` | Borra una ruta |

## Página del mapa

Archivo [`mapa/index.html`](../mapa/index.html). Se abre con doble clic en un ordenador con internet; no hay que instalar nada.

1. Pulsa **Abrir ruta (.csv)** y elige un archivo descargado de la placa.
2. Elige el color del trazado: por **inclinación** o por **velocidad**.
3. Pulsa sobre el trazado para ver hora, velocidad, inclinación, aceleración y frenada de ese punto.

Qué muestra:

- **Resumen**: fecha, distancia, tiempo en movimiento, velocidad máxima y media, inclinación máxima a cada lado, y máxima aceleración y frenada.
- **Trazado**: gris, azul, amarillo y rojo, con los mismos umbrales que la pantalla de la placa (10°, 25° y 40°), o por tramos de velocidad (50, 90 y 120 km/h).
- **Marcas**: naranja en las frenadas desde 0,5 g y verde en los acelerones desde 0,4 g.
- **Fondo**: tres mapas a elegir arriba a la derecha (IGN España, calles de Esri y satélite de Esri).

En `mapa/ruta_de_ejemplo.csv` hay una ruta inventada para probar la página.

> El fondo de mapa está pendiente de confirmar: los servidores de OpenStreetMap y CARTO rechazaron la página abierta desde un archivo local, y los fondos actuales no se han podido comprobar todavía.
