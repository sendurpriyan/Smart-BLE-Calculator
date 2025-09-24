#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>

static BLEUUID serviceUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455");
static BLEUUID charUUID("49535343-8841-43f4-a8d4-ecbe34729bb3");

static BLEAdvertisedDevice* printerDevice;
static BLEClient* client;
static BLERemoteCharacteristic* remoteChar;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == "Printer001") {
      printerDevice = new BLEAdvertisedDevice(advertisedDevice);
      BLEDevice::getScan()->stop();
    }
  }
};

void sendToPrinter(String message) {
  if (!remoteChar || !remoteChar->canWrite()) {
    Serial.println("Can't write to characteristic.");
    return;
  }

  const int CHUNK_SIZE = 20;
  int msgLen = message.length();

  for (int i = 0; i < msgLen; i += CHUNK_SIZE) {
    String chunk = message.substring(i, i + CHUNK_SIZE);
    remoteChar->writeValue((uint8_t*)chunk.c_str(), chunk.length(), false);
    delay(50); // Give BLE time to process
  }

  Serial.println("Text sent.");
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  BLEScan* scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  scan->setActiveScan(true);
  scan->start(10);

  if (printerDevice == nullptr) {
    Serial.println("Printer not found.");
    return;
  }

  client = BLEDevice::createClient();
  client->connect(printerDevice);

  BLERemoteService* service = client->getService(serviceUUID);
  if (!service) {
    Serial.println("Service not found.");
    return;
  }

  remoteChar = service->getCharacteristic(charUUID);
  if (!remoteChar) {
    Serial.println("Characteristic not found.");
    return;
  }

  // ESC @
sendToPrinter("\x1B\x40"); // ESC @ init
sendToPrinter("=== AZAM ===\n");
sendToPrinter("Azam Sathik\n");
sendToPrinter("Mobile - 0766989428\n");
sendToPrinter("Email - azamsathik@gmail.com\n");
sendToPrinter("\n\n");

}

void loop() {}
