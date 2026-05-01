# medAlert

C++ and Qt6 desktop application for tracking medications at home, updating stock once per day, and warning when a medication is running low.

Available documentation:

- English: `README.md`
- Italian: `README-it.md`
- French: `README-fr.md`
- German: `README-de.md`

## Main features

- one record per medication with name, units per box, and daily dosage
- current state stored as remaining units
- warehouse stock stored as number of spare boxes
- standby mode for a medication, suspending the automatic 23:00 consumption update
- automatic daily update at 23:00
- automatic refill from stock when current units run out
- configurable alert threshold for each medication, with a default of 10 units
- notification when remaining units fall below the configured threshold and no spare boxes are left
- multi-row selection through standard Qt table behavior, including Shift+click
- arrow buttons to move the selected row up or down by one position
- button that shows the selected medication names in a modal window, one per line
- `Settings` button that opens a dialog for choosing the UI language and the daily update time
- manual check always sends a notification, even when nothing needs to be ordered, so the notification path can be tested
- headless `--check` mode to run the daily check without opening the GUI
- GUI button that installs and enables a `systemd --user` timer for the configured update time
- full persistence on a JSON file updated after every change
- English source UI with runtime localization for Italian, French, and German based on the system locale

## Notifications on GNOME 3 and Wayland

The most universal solution on Linux desktop is the `org.freedesktop.Notifications` D-Bus interface.
Qt6 integrates with it through the `Qt6::DBus` module and it works well on GNOME 3, including Wayland sessions.

If the notification service is unavailable, the app falls back to a local Qt warning dialog.

## JSON state file

The JSON file is stored under `QStandardPaths::GenericDataLocation`, for example:

```text
~/.local/share/medAlert/medicines.json
```

It contains:

- `lastProcessedDate`: last date for which daily consumption has already been applied
- `medicines`: medication list, including the `standby` flag, the alert threshold, and current stock state

The application also stores preferences in:

```text
~/.local/share/medAlert/settings.json
```

This file contains:

- `preferredLanguage`: selected UI language
- `updateTime`: daily update time in `HH:mm` format

## Localization

The application source language is English.

At startup the app loads translations for these locales when they match the system language:

- Italian
- French
- German

If no supported translation is available, the app stays in English.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Launch

```bash
./build/medAlert
```

## Headless check

```bash
./build/medAlert --check
```

The `--check` mode reads the JSON file, applies the daily update, and sends a notification only when one or more medications are below threshold.

## User systemd timer

The app can automatically install these user files:

- `~/.config/systemd/user/medalert.service`
- `~/.config/systemd/user/medalert.timer`

The timer runs `medAlert --check` every day at the configured time with `Persistent=true`.

The repository also includes example files in [systemd/medalert.service](systemd/medalert.service) and [systemd/medalert.timer](systemd/medalert.timer).