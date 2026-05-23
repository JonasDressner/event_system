# Architektur des Event-Systems

Dieses Projekt besteht aus drei Schichten:

- `include/`
  - Enthält die Header-Dateien für `Event`, `IPC`, `Producer` und `Consumer`.
- `src/`
  - `main.cpp` startet die Anwendung und wählt den Modus `--producer` oder `--consumer`.
  - `producer.cpp` fährt Events hoch und schreibt sie über IPC in eine Pipe.
  - `consumer.cpp` liest Events aus der Pipe, verarbeitet Warnungen und Fehler und erstellt Statistiken.
- `build/`
  - Externer Build-Ordner für Compiler-Ausgaben und temporäre Dateien.

## Laufzeitverhalten

- Der Producer erzeugt periodisch Ereignisse und serialisiert sie als JSON-ähnliche Strings.
- Der Consumer liest die Ereignisse, filtert INFO-Nachrichten heraus und zeigt WARNUNG/ERROR an.
- Die Pipe wird plattformabhängig über Named Pipes unter Windows oder FIFO-Dateien unter Linux umgesetzt.
