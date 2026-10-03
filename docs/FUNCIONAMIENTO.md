# Cómo funciona

## El problema de medir la inclinación en una moto

En parado, un acelerómetro mide la inclinación sin problema: basta ver hacia dónde apunta la gravedad. En curva no sirve. Una moto tumbada en equilibrio nota la suma de gravedad y fuerza centrífuga exactamente alineada con su eje vertical, así que el acelerómetro "cree" que va recta.

Por eso la inclinación se calcula con el **giroscopio**, integrando la velocidad de giro alrededor del eje longitudinal de la moto. El giroscopio acumula un pequeño error con el tiempo (deriva), y ese error se corrige con el acelerómetro **solo cuando la moto va recta o está parada**, que es cuando el acelerómetro sí dice la verdad.

## Pasos del cálculo

1. **Lectura** del sensor QMI8658 a unas 235 muestras por segundo: acelerómetro (±8 g) y giroscopio (±512 °/s).
2. **Cambio de ejes**: con la calibración, las lecturas se pasan a los ejes de la moto (arriba, delante, izquierda).
3. **Integración** del giro alrededor del eje "delante" para obtener la inclinación.
4. **Corrección de deriva** cuando se cumplen a la vez: aceleración total cercana a 1 g, casi sin giro de guiñada ni cabeceo, y casi sin velocidad de balanceo.
5. **Aceleración longitudinal**: componente del acelerómetro (filtrado a 0,25 s) sobre el eje "delante".

## Registro de máximos

| Máximo | Condición para registrarlo |
|---|---|
| Inclinación | Entre 8° y 70°, y solo si el acelerómetro no coincide con el ángulo. Así se descartan la moto apoyada en la pata o tumbada en parado |
| Aceleración y frenada | Valores por debajo de 2 g, para descartar golpes |

Estos límites están al principio del código (`LEAN_MIN_REG`, `LEAN_MAX_REG`, `G_MAX_REG`).

## Limitaciones

- **Precisión orientativa**, de unos pocos grados.
- **Curvas largas y enlazadas**: al no haber tramos rectos, la deriva no se corrige y puede acumularse algo de error, que desaparece al volver a ir recto.
- **Cuestas**: la aceleración longitudinal incluye la componente de la gravedad, así que una pendiente fuerte suma o resta a la lectura.
- **Vibraciones**: un montaje rígido falsea sobre todo la aceleración. Conviene interponer goma.
- **Ancho de neumático**: se mide la inclinación del chasis, no el ángulo real respecto al punto de contacto.
