# Tape Delay EQ

Delay tape-style (wow/flutter + saturación) con un EQ híbrido de 12 bandas
que actúa **exclusivamente sobre la señal duplicada (el delay)**, nunca
sobre la señal seca.

## Flujo de señal

```
                    ┌───────────────────────────┐
input ──────┬──────►│           DRY              │──────┐
            │        └───────────────────────────┘      │
            │                                            ▼
            │        ┌─────────────┐   ┌──────────┐   ┌─────┐
            └───────►│ TapeDelay   │──►│ HybridEQ │──►│ MIX │──► output
                     │ (read+sat)  │   │(12 bandas)│   └─────┘
                     └─────┬───────┘   └────┬─────┘
                           │  feedback      │
                           └────────◄───────┘
                        (input + wet*feedback se re-escribe)
```

Como el EQ está **dentro** del loop de feedback, cada repetición del delay
pasa de nuevo por el EQ: el color se va acumulando repetición tras
repetición, igual que en una cinta analógica real (las repeticiones se
oscurecen/calientan progresivamente en vez de sonar todas idénticas).

## Estructura

- `Source/DSP/TapeDelayLine.h` — buffer circular, lectura fraccional,
  modulación wow/flutter, saturación tipo cinta.
- `Source/DSP/HybridEQ.h` — 12 bandas (low shelf + 10 peaking + high shelf)
  + etapa de saturación, igual filosofía que KickForge EQ.
- `Source/PluginProcessor.*` — conecta ambos y hace el split dry/wet.
- `Source/PluginEditor.*` — UI mínima funcional (6 knobs). Las 12 bandas
  del EQ ya están como parámetros en el APVTS, falta agregarles sliders.

## Próximos pasos sugeridos

1. Agregar sliders/faders para las 12 bandas del EQ en el editor.
2. Sincronizar `delayTimeMs` a tempo (notas musicales) si lo quieres.
3. Agregar un control "EQ Amount" que mezcle EQ'd vs sin EQ dentro del wet,
   si en algún momento quieres suavizar el efecto.
4. Probar con JUCE como submódulo en vez de FetchContent si el build
   tarda mucho en CI.
