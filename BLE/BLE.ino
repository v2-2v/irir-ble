#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define DEVICE_NAME         "ESP32"
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-1234-1234-abcdefabcdef"

const int sensorPin = 15;

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

bool deviceConnected = false;
bool measuring = false;
unsigned long measureStartTime = 0;

const int MAX_BUFFER_SIZE = 1024;
int bufferIndex = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("✅ クライアントが接続されました");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("🔌 クライアントが切断されました");
    BLEDevice::startAdvertising();
    Serial.println("🔄 広告を再開しました");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.print("📥 受信データ: ");
      Serial.println(rxValue);

      if (rxValue == "start") {
        measuring = true;
        measureStartTime = millis();
        Serial.println("🟢 データ計測を開始します");
        pCharacteristic->setValue("🟢 計測開始");
        pCharacteristic->notify();
      } else if (rxValue == "stop") {
        measuring = false;
        unsigned long elapsed = millis() - measureStartTime;
        Serial.println("🛑 データ計測を終了します");
        Serial.print("📦 測定データ: ");
        Serial.println();
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init(DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("初期値");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("🚀 BLE広告開始");
}

void loop() {
  if (deviceConnected && measuring) {
    static unsigned long lastSampleTime = 0;
    if (millis() - lastSampleTime > 50) {
      lastSampleTime = millis();

      if (bufferIndex < MAX_BUFFER_SIZE) {
        int data = analogRead(sensorPin);
        String dataStr = String(data);
        pCharacteristic->setValue(dataStr.c_str());
        pCharacteristic->notify();
      }
    }
  }
}
