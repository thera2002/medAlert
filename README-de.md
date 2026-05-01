# medAlert

C++- und Qt6-Desktop-Anwendung zum Verwalten von Medikamenten zu Hause, zum taglichen Aktualisieren des Bestands und zum Warnen, wenn ein Medikament knapp wird.

Verfugbare Dokumentation:

- Englisch: `README.md`
- Italienisch: `README-it.md`
- Franzosisch: `README-fr.md`
- Deutsch: `README-de.md`

## Hauptfunktionen

- ein Eintrag pro Medikament mit Name, Einheiten pro Packung und Tagesdosis
- aktueller Stand als Anzahl verbleibender Einheiten
- Lagerbestand als Anzahl vorhandener Reservepackungen
- Stand-by-Modus fur ein Medikament, der das automatische Update um 23:00 aussetzt
- automatisches tagliches Update um 23:00
- automatisches Nachfullen aus dem Lager, wenn die aktuellen Einheiten aufgebraucht sind
- konfigurierbare Warnschwelle fur jedes Medikament, standardmassig 10 Einheiten
- Benachrichtigung, wenn die verbleibenden Einheiten unter die konfigurierte Schwelle fallen und keine Reservepackungen mehr vorhanden sind
- Mehrfachauswahl von Zeilen mit dem Standardverhalten der Qt-Tabelle einschliesslich Umschalt+Klick
- Pfeiltasten zum Verschieben der ausgewahlten Zeile um genau eine Position nach oben oder unten
- Schaltflache, die die Namen der ausgewahlten Medikamente in einem modalen Fenster zeilenweise anzeigt
- Schaltflache `Settings`, die ein Fenster zur Wahl der Sprache und der taglichen Aktualisierungszeit offnet
- die manuelle Prufung sendet immer eine Benachrichtigung, auch wenn nichts bestellt werden muss, um den Benachrichtigungsweg zu testen
- Headless-Modus `--check`, um die tagliche Prufung ohne GUI auszufuhren
- GUI-Schaltflache zum automatischen Installieren und Aktivieren eines `systemd --user`-Timers fur die konfigurierte Uhrzeit
- vollstandige Persistenz in einer JSON-Datei, die nach jeder Anderung aktualisiert wird
- englische Quelloberflache mit Laufzeitlokalisierung fur Italienisch, Franzosisch und Deutsch anhand der Systemsprache

## Benachrichtigungen unter GNOME 3 und Wayland

Die universellste Losung auf dem Linux-Desktop ist die D-Bus-Schnittstelle `org.freedesktop.Notifications`.
Qt6 nutzt sie uber das Modul `Qt6::DBus`, und sie funktioniert gut unter GNOME 3, auch in Wayland-Sitzungen.

Wenn der Benachrichtigungsdienst nicht verfugbar ist, verwendet die Anwendung als Fallback einen lokalen Qt-Warndialog.

## JSON-Statusdatei

Die JSON-Datei wird unter `QStandardPaths::GenericDataLocation` gespeichert, zum Beispiel:

```text
~/.local/share/medAlert/medicines.json
```

Sie enthalt:

- `lastProcessedDate`: letztes Datum, fur das der tagliche Verbrauch bereits angewendet wurde
- `medicines`: Medikamentenliste einschliesslich `standby`-Flag, Warnschwelle und aktuellem Bestandsstatus

Die Anwendung speichert ausserdem Benutzereinstellungen in:

```text
~/.local/share/medAlert/settings.json
```

Diese Datei enthalt:

- `preferredLanguage`: ausgewahlte Sprache der Oberflache
- `updateTime`: tagliche Aktualisierungszeit im Format `HH:mm`

## Lokalisierung

Die Quellsprache der Anwendung ist Englisch.

Beim Start ladt die Anwendung automatisch eine Ubersetzung, wenn die Systemsprache einer dieser Sprachen entspricht:

- Italienisch
- Franzosisch
- Deutsch

Wenn keine unterstutzte Ubersetzung verfugbar ist, bleibt die Oberflache auf Englisch.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Starten

```bash
./build/medAlert
```

## Headless-Prufung

```bash
./build/medAlert --check
```

Der Modus `--check` liest die JSON-Datei, wendet das tagliche Update an und sendet nur dann eine Benachrichtigung, wenn ein oder mehrere Medikamente unter der Schwelle liegen.

## systemd-Benutzertimer

Die Anwendung kann diese Benutzerdateien automatisch installieren:

- `~/.config/systemd/user/medalert.service`
- `~/.config/systemd/user/medalert.timer`

Der Timer fuhrt `medAlert --check` jeden Tag zur konfigurierten Uhrzeit mit `Persistent=true` aus.

Das Repository enthalt ausserdem Beispieldateien in [systemd/medalert.service](systemd/medalert.service) und [systemd/medalert.timer](systemd/medalert.timer).