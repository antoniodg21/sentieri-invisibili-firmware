// Sentieri Invisibili – radiofaro BLE (prototipo, Passo 1)
//
// Advertising BLE continuo, NON connectable, NON iBeacon: un advertising "normale"
// con un service UUID custom a 128 bit, così CoreBluetooth (iOS) lo vede e riporta
// l'RSSI a ogni pacchetto, e lo stesso formato resta leggibile da Android.
//
// Scheda: DevKit ESP32-C3 generico  –  fqbn: esp32:esp32:esp32c3
// Libreria: BLE inclusa nel core ESP32 di Espressif (sul C3 usa lo stack NimBLE);
// nessuna libreria esterna. Verificato con il core esp32:esp32 3.3.12.

#include <BLEDevice.h>
#include <BLEAdvertising.h>

// ───────────────────────── PARAMETRI (unico punto da modificare) ─────────────────────────

// Numero del faro (1–99). Determina il nome annunciato: SENT-01, SENT-02, ...
// Cambiare SOLO questo valore per programmare un altro faro con lo stesso sketch.
static const uint8_t BEACON_NUMBER = 1;

// Tipo di faro: 0 = tappa principale, 1 = faro nascosto.
// Trasmesso nei manufacturer data; l'app per ora non lo usa.
static const uint8_t BEACON_TYPE = 0;

// Service UUID a 128 bit: è il filtro di scansione dell'app iOS.
// Deve essere identico in tutti i fari e nell'app (vedi README).
static const char *SERVICE_UUID = "CD5E48FF-59D6-4F6A-8142-B53A3C4DEBEB";

// Intervallo di advertising in millisecondi. Il telefono ne riceve circa la metà
// (misurato il 16/11/2026: ~5 pacchetti/s con 100 ms), quindi 50 ms per averne ~10/s.
// Minimo ammesso per un advertising non connectable: 20 ms.
static const uint16_t ADV_INTERVAL_MS = 50;

// Potenza di trasmissione.
// TX_POWER_OVERRIDE = false → si lascia il valore di default dello stack (scelta attuale).
// Mettere true per imporre TX_POWER_LEVEL (es. ESP_PWR_LVL_N0 = 0 dBm, ESP_PWR_LVL_P9 = +9 dBm).
// Cambiare la potenza sposta tutte le soglie RSSI dell'app: ritarare dopo ogni modifica.
static const bool TX_POWER_OVERRIDE = true;
static const esp_power_level_t TX_POWER_LEVEL = ESP_PWR_LVL_P20;

// Scan response con i manufacturer data (numero faro + tipo).
// false → advertising ADV_NONCONN_IND puro: il faro trasmette e basta, non ascolta né
//         risponde alle richieste di scansione. È la forma più semplice e regolare.
// true  → advertising ADV_SCAN_IND: il faro risponde alle scan request con i manufacturer data.
// In prova a false dal 16/11/2026: con true i telefoni Apple ricevevano pochi pacchetti.
static const bool USE_SCAN_RESPONSE = false;

// Company ID dei manufacturer data: 0xFFFF = identificativo riservato a test/prototipi.
static const uint16_t MANUFACTURER_ID = 0xFFFF;

// Ogni quanti millisecondi stampare una riga di stato sulla seriale (115200 baud).
static const uint32_t SERIAL_HEARTBEAT_MS = 5000;

// ──────────────────────────────────────────────────────────────────────────────────────────

static char beaconName[8];  // "SENT-NN" + terminatore

void setup() {
  Serial.begin(115200);
  delay(300);

  snprintf(beaconName, sizeof(beaconName), "SENT-%02u", (unsigned)(BEACON_NUMBER % 100));

  BLEDevice::init(beaconName);
  if (TX_POWER_OVERRIDE) {
    BLEDevice::setPower(TX_POWER_LEVEL, ESP_BLE_PWR_TYPE_DEFAULT);
    BLEDevice::setPower(TX_POWER_LEVEL, ESP_BLE_PWR_TYPE_ADV);
  }

  // Pacchetto di advertising (max 31 byte):
  //   flags (3) + service UUID 128 bit (18) + nome completo "SENT-NN" (9) = 30 byte.
  BLEAdvertisementData advData;
  advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  advData.setCompleteServices(BLEUUID(SERVICE_UUID));
  advData.setName(beaconName);

  // Scan response: manufacturer data = company ID (2 byte, little endian) + numero faro + tipo.
  // Il pacchetto principale è già pieno, quindi questi 4 byte viaggiano nella scan response.
  const char mfg[4] = {
    (char)(MANUFACTURER_ID & 0xFF), (char)(MANUFACTURER_ID >> 8),
    (char)BEACON_NUMBER, (char)BEACON_TYPE
  };
  BLEAdvertisementData scanData;
  scanData.setManufacturerData(String(mfg, sizeof(mfg)));

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  // Non connectable in entrambi i casi. Con la scan response lo stack NimBLE trasmette
  // ADV_SCAN_IND, senza trasmette ADV_NONCONN_IND.
  adv->setAdvertisementType(BLE_GAP_CONN_MODE_NON);
  adv->setScanResponse(USE_SCAN_RESPONSE);
  const bool advOk = adv->setAdvertisementData(advData);
  const bool rspOk = USE_SCAN_RESPONSE ? adv->setScanResponseData(scanData) : true;
  // L'intervallo BLE si esprime in unità da 0,625 ms.
  const uint16_t advUnits = (uint16_t)((ADV_INTERVAL_MS * 1000UL) / 625UL);
  adv->setMinInterval(advUnits);
  adv->setMaxInterval(advUnits);
  const bool startOk = adv->start();

  Serial.println();
  Serial.println("Sentieri Invisibili – radiofaro BLE");
  Serial.printf("  nome:        %s\n", beaconName);
  Serial.printf("  tipo:        %u (0 = tappa principale, 1 = faro nascosto)\n", BEACON_TYPE);
  Serial.printf("  service:     %s\n", SERVICE_UUID);
  Serial.printf("  intervallo:  %u ms\n", ADV_INTERVAL_MS);
  Serial.printf("  scan resp.:  %s\n", USE_SCAN_RESPONSE ? "attiva (ADV_SCAN_IND)" : "assente (ADV_NONCONN_IND)");
  // Livello effettivo letto dal controller: indice della enum esp_power_level_t
  // (8 = 0 dBm, 11 = +9 dBm, 15 = +20 dBm; un passo = 3 dB, tranne l'ultimo).
  Serial.printf("  potenza TX:  %s, livello effettivo %d\n",
                TX_POWER_OVERRIDE ? "impostata da TX_POWER_LEVEL" : "default dello stack",
                (int)esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV));
  Serial.printf("  indirizzo:   %s\n", BLEDevice::getAddress().toString().c_str());
  if (advOk && rspOk && startOk) {
    Serial.println("Advertising avviato (non connectable).");
  } else {
    Serial.printf("ERRORE: advertising non avviato (dati=%d, scan response=%d, avvio=%d)\n", advOk, rspOk, startOk);
  }
}

void loop() {
  // L'advertising è gestito dallo stack BLE: qui solo un segno di vita sulla seriale.
  delay(SERIAL_HEARTBEAT_MS);
  Serial.printf("[%s] in trasmissione, acceso da %lu s\n", beaconName, millis() / 1000UL);
}
