# Glasklar Amp

Cleaner Gitarren-Amp im Fender-Blackface-Stil mit Tremolo, Delay und Reverb – als VST3 / AU / Standalone (JUCE).

**Signalweg:** Bright → Vorstufe (Röhren-Sättigung, 2× Oversampling) → Tonestack → Endstufe
→ Tremolo → Box-Simulation → Delay → Reverb → Master

## Regler

### Amp
| Regler   | Wirkung |
|----------|---------|
| Volume   | Vorstufen-Gain. Bis ca. 5 glasklar, darüber leichter Breakup wie ein aufgedrehter Fender |
| Treble   | Höhen-Shelf ab ca. 2,2 kHz |
| Bass     | Bass-Shelf um 110 Hz |
| Middle   | Mitten bei 500 Hz – auf 5 typisch ausgehöhlt („Scoop“), auf 10 fast neutral |
| Master   | Ausgangspegel in dB |
| Bright   | Höhen-Boost; wirkt wie beim Original bei leisem Volume stärker |
| Box-Sim  | Lautsprecher-Simulation. Ausschalten, wenn du einen eigenen IR-Loader nutzt |

### Tremolo
| Regler    | Wirkung |
|-----------|---------|
| An        | Schaltet das Tremolo ein/aus |
| Speed     | Geschwindigkeit, 1 bis 10 Hz |
| Intensity | Tiefe – von leichtem Pulsieren bis zum harten „Hacken“ |

### Delay
| Regler   | Wirkung |
|----------|---------|
| An       | Schaltet das Delay ein/aus – vorhandene Echos klingen dabei natürlich aus |
| Time     | Verzögerung, 40 bis 1200 ms (Drehen während des Spielens gibt einen leichten Tape-Effekt) |
| Feedback | Anzahl der Wiederholungen; jede Wiederholung wird etwas dunkler, wie bei einem Analog-Delay |
| Mix      | Lautstärke der Echos |

### Reverb
| Regler | Wirkung |
|--------|---------|
| An     | Schaltet den Hall ein/aus – die Hallfahne klingt aus |
| Mix    | Hall-Anteil |
| Decay  | Länge der Hallfahne |
| Tone   | Klangfarbe des Halls, von dunkel bis hell |

Doppelklick auf einen Knopf setzt ihn auf den Startwert zurück. Alle Einstellungen werden mit dem DAW-Projekt gespeichert und lassen sich automatisieren.

---

## Bauen

### Was du brauchst
- **Windows:** Visual Studio 2022 (Community reicht, Workload „Desktopentwicklung mit C++“)
- **macOS:** Xcode
- **CMake** ab 3.22 (cmake.org) und **Git**

### Schritte
1. Im Projektordner JUCE 8 klonen:
   ```sh
   git clone --depth 1 https://github.com/juce-framework/JUCE.git
   ```
2. Projekt erzeugen und bauen:
   ```sh
   cmake -B build
   cmake --build build --config Release
   ```
   Der erste Build dauert ein paar Minuten.

3. Fertige Dateien liegen unter `build/GlasklarAmp_artefacts/Release/`:
   - `VST3/Glasklar Amp.vst3`
   - `Standalone/` – eigenständige App, gut zum schnellen Testen ohne DAW
   - `AU/` (nur Mac)

4. VST3 in den Plugin-Ordner kopieren und in der DAW neu scannen:
   - Windows: `C:\Program Files\Common Files\VST3\`
   - macOS: `~/Library/Audio/Plug-Ins/VST3/`

## Tipps für den Sound
- Gitarre direkt (DI) ins Interface, Gain am Interface so, dass die Peaks bei etwa −12 dBFS liegen.
- Klassischer Clean-Start: Volume 4, Treble 6, Middle 5, Bass 4–5, Reverb Mix 2–3.
- Surf-Sound: Reverb Mix 6–7, Decay 6, Tone 7, dazu Tremolo mit Speed 5 und Intensity 4.
- Ambient-Clean: Delay an, Time ca. 450 ms, Feedback 5, Mix 3, dazu Reverb Decay 8.
- Mehr Glanz bei Single Coils: Bright an, Volume niedrig lassen, Lautstärke über Master holen.
