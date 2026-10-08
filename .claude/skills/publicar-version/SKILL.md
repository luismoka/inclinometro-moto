---
name: publicar-version
description: Publica una versión nueva del firmware MotoLean - sube el número en todos los sitios donde aparece, añade la entrada al CHANGELOG, regenera las imágenes y prepara el commit.
argument-hint: "[169|lolin] [número de versión] [resumen de cambios]"
disable-model-invocation: true
---

# Publicar una versión

Argumentos recibidos: `$ARGUMENTS`

## 1. Aclarar qué se publica

Si falta algún dato, pregúntalo antes de tocar nada:

- **Firmware**: `169` (Waveshare, serie 3.x, el habitual) o `lolin` (LOLIN S3 Mini Pro, serie 2.x). Sin indicación, propón `169`.
- **Número nuevo**: si no se da, lee el actual y propón el siguiente. Función nueva → sube el segundo número (3.2 → 3.3). Solo correcciones → sube el tercero (3.2.0 → 3.2.1).
- **Cambios**: si no se dan, dedúcelos de `git log` y `git diff` desde el commit de la versión anterior y enséñaselos al autor para que los confirme.

## 2. Cambiar el número

El `#define VERSION` lleva dos cifras (`"3.3"`); el CHANGELOG lleva tres (`3.3.0`). En una versión de solo correcciones (3.2.1) el `#define` y los títulos no cambian: solo se añade la entrada al CHANGELOG.

**Firmware 169**

| Archivo | Qué cambiar |
|---|---|
| `firmware/moto_inclinometro_169/moto_inclinometro_169.ino` | `#define VERSION "X.Y"` |
| `README.md` | línea `**Versión X.Y**` |
| `docs/PLACA_169.md` | título: `(firmware X.Y)` |

**Firmware LOLIN**

| Archivo | Qué cambiar |
|---|---|
| `firmware/moto_inclinometro/moto_inclinometro.ino` | `#define VERSION "X.Y"` |
| `README.md` | la mención `(firmware X.Y)` del aviso inicial |

Después busca el número antiguo en todo el repositorio para comprobar que no queda en ningún otro sitio (sin contar las entradas anteriores del CHANGELOG):

```
git grep -n "<número antiguo>" -- . ":!CHANGELOG.md"
```

## 3. CHANGELOG

Añade la entrada arriba del todo en `CHANGELOG.md`, con el formato de las existentes:

```
## X.Y.Z – AAAA-MM-DD

- Cambio descrito para quien usa el aparato, no para quien programa.
```

Usa la fecha de hoy. Si el cambio no se ha probado en la placa, dilo en la entrada ("Compila; sin probar en placa real").

## 4. Documentación e imágenes

- Si cambian botones, pantalla, pines o páginas web, actualiza el documento de `docs/` que corresponda.
- Si cambia el aspecto de la pantalla, actualiza el simulador (`herramientas/simular_pantalla_169.py` o `simular_pantalla.py`), ejecútalo desde la raíz del repositorio y **enseña la imagen al autor** antes de seguir.
- Actualiza la tabla "Estado del proyecto" de `README.md` si algo ha cambiado de estado.

## 5. Binario

`firmware/bin/` debe contener el binario de la versión que se publica (`MotoLean_169_firmware.bin` o `MotoLean_firmware.bin`).

- Si hay `arduino-cli` y `esptool` en la sesión, compila y une el binario con los comandos de `CLAUDE.md` y `docs/COMPILAR.md`.
- Si no, **no inventes ni reutilices el binario anterior**: pide al autor que lo exporte desde Arduino IDE (*Programa → Exportar binario compilado*) y lo copie a `firmware/bin/` con el nombre correcto. Mientras tanto, deja preparado todo lo demás y avisa de que el binario sigue siendo el de la versión anterior.

## 6. Revisión y commit

1. Enseña `git status` y un resumen de `git diff`.
2. Comprueba que el número coincide en todos los archivos de la tabla del paso 2.
3. Propón el mensaje de commit, en español y corto: `Versión X.Y: <resumen>`.
4. **Espera la confirmación del autor** antes de hacer commit y antes de subir a GitHub.
5. Tras subir, recuérdale que actualice su copia local con `git pull`.
