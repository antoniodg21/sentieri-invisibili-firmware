# Sentieri Invisibili – firmware del radiofaro

Firmware del radiofaro Bluetooth del prototipo di Sentieri Invisibili, progetto presentato alla challenge Beyond the Museum (programma Square Heritage 2026).

Il faro trasmette un annuncio Bluetooth Low Energy continuo e non connettibile, con il proprio nome (`SENT-01`, `SENT-02`, …) e un service UUID a 128 bit. L'app sul telefono lo riconosce da quel service UUID e usa la forza del segnale ricevuto per guidare la ricerca. Non è in formato iBeacon: è un annuncio normale, leggibile sia da iOS sia da Android.

## Hardware e strumenti

- Scheda: DevKit ESP32-C3 generico (`esp32:esp32:esp32c3`). È la scelta del prototipo.
- Core ESP32 di Espressif 3.3 o successivo (verificato con la versione 3.3.12), con la libreria BLE inclusa nel core. Nessuna libreria esterna.
- Arduino IDE oppure arduino-cli.

## Parametri

Sono tutti in testa a `sent_beacon.ino`:

- `BEACON_NUMBER`: numero del faro, da cui deriva il nome annunciato. Per programmare un altro faro si cambia solo questo valore.
- `BEACON_TYPE`: 0 per una tappa principale, 1 per un faro nascosto. L'app per ora non lo usa.
- `SERVICE_UUID`: deve essere identico in tutti i fari e nell'app.
- `ADV_INTERVAL_MS`: intervallo tra un annuncio e l'altro, 50 ms.
- `TX_POWER_LEVEL`: potenza di trasmissione. Nel prototipo è impostata al massimo; la conformità ai limiti di legge è ancora da verificare. Cambiarla sposta le soglie dell'app, che vanno ritarate.

## Stato

Prototipo, collaudato con uno strumento di scansione Bluetooth e con l'app iOS. Autonomia a batteria e potenza di trasmissione sono ancora da misurare e verificare.

## Team

Antonio Damiano Giuffrè e Marco Gorgone.
