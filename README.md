# medAlert

Applicazione desktop in C++ e Qt6 per tenere traccia delle medicine presenti in casa, aggiornare giornalmente le scorte e avvisare quando un farmaco sta finendo.

## Funzioni principali

- scheda per ogni farmaco con nome, pastiglie per confezione e dosaggio giornaliero
- stato corrente espresso in numero di pastiglie residue
- scorta di magazzino espressa in numero di confezioni
- possibilità di mettere un farmaco in stand-by, sospendendo il consumo automatico delle 23:00
- aggiornamento automatico una volta al giorno alle 23:00
- ricarico automatico dallo stock quando le pastiglie correnti terminano
- notifica quando restano al massimo 10 pastiglie e non ci sono confezioni di riserva
- selezione multipla di righe con i meccanismi standard della tabella Qt, incluso Shift+clic
- pulsante che mostra in una finestra modale i nomi dei farmaci selezionati, uno per riga
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
- `medicines`: elenco dei farmaci, incluso il flag `standby`, e del loro stato corrente

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Avvio

```bash
./build/medAlert
```