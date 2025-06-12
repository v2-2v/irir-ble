#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define DEVICE_NAME         "device_1"
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-1234-1234-abcdefabcdef"

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("✅ クライアントが接続されました");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("🔌 クライアントが切断されました");

    // BLEスタックは初期化済みなので、広告だけ再開する
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
  if (deviceConnected) {
    static unsigned long lastTime = 0;
    if (millis() - lastTime > 3000) {
      lastTime = millis();
      String message = "⏱ 通知: " + String(millis() / 1000) + "秒";
      pCharacteristic->setValue(message.c_str());
      pCharacteristic->notify();
      Serial.println("📤 Notify送信: " + message);
    }
  }
}