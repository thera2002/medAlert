# medAlert

Applicazione desktop in C++ e Qt6 per tenere traccia delle medicine presenti in casa, aggiornare giornalmente le scorte e avvisare quando un farmaco sta finendo.

## Funzioni principali

- scheda per ogni farmaco con nome, unità per confezione e dosaggio giornaliero
- stato corrente espresso in numero di unità residue
- scorta di magazzino espressa in numero di confezioni
- possibilità di mettere un farmaco in stand-by, sospendendo il consumo automatico delle 23:00
- aggiornamento automatico una volta al giorno alle 23:00
- ricarico automatico dallo stock quando le unità correnti terminano
- soglia di notifica configurabile per ogni farmaco, con default a 10 unità
- notifica quando le unità residue scendono sotto la soglia configurata e non ci sono confezioni di riserva
- selezione multipla di righe con i meccanismi standard della tabella Qt, incluso Shift+clic
- pulsante che mostra in una finestra modale i nomi dei farmaci selezionati, uno per riga
- il controllo manuale invia sempre una notifica, anche quando non c'è nulla da ordinare, per testare il sistema
- modalità headless `--check` per eseguire il controllo giornaliero senza aprire la GUI
- pulsante GUI per installare e abilitare automaticamente un timer `systemd --user` alle 23:00
- persistenza completa su file JSON aggiornato dall'app dopo ogni modifica

## Notifiche su GNOME 3 e Wayland

La soluzione più universale su Linux desktop è usare l'interfaccia D-Bus `org.freedesktop.Notifications`.
Qt6 la usa facilmente tramite il modulo `Qt6::DBus` e funziona bene su GNOME 3, anche in sessioni Wayland.

Se il servizio di notifiche non è disponibile, l'app usa un fallback locale con finestra di avviso Qt.

## File di stato JSON

Il file JSON viene salvato nel percorso restituito da `QStandardPaths::AppDataLocation`, ad esempio:

```text
~/.local/share/medAlert/medicines.json
```

Contiene:

- `lastProcessedDate`: ultima data per cui è stato applicato il consumo giornaliero
- `medicines`: elenco dei farmaci, incluso il flag `standby`, la soglia di notifica e il loro stato corrente

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Avvio

```bash
./build/medAlert
```

## Controllo headless

```bash
./build/medAlert --check
```

La modalità `--check` legge il file JSON, applica l'aggiornamento giornaliero e invia una notifica solo se ci sono farmaci sotto soglia.

## Timer systemd utente

L'app può installare automaticamente i file utente:

- `~/.config/systemd/user/medalert.service`
- `~/.config/systemd/user/medalert.timer`

Il timer esegue `medAlert --check` ogni giorno alle 23:00 con `Persistent=true`.

Nel repository sono presenti anche esempi in [systemd/medalert.service](/home/lfabio/git/medAlert/systemd/medalert.service) e [systemd/medalert.timer](/home/lfabio/git/medAlert/systemd/medalert.timer).