/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

/** NimBLE differences highlighted in comment blocks **/

/*******original********
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
***********************/
#include <NimBLEDevice.h>
NimBLECharacteristic *pCharacteristic;
void Clienting();
void HandleSerialPort(NimBLECharacteristic *pCharacteristic);


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

struct Message
{
  String msg;
  String user;

  std::string serialize() const
  {
    return "@b@" + std::string(user.c_str()) + '|' + std::string(msg.c_str()) + '\0';
  }
  // Deserialize a byte array to the struct
  static Message deserialize(const std::string &data)
  {
    Message message;
    if(data.compare(0, 3, "@b@") != 0){
      Serial.println("Invalid message format");
      return message;
    }
    std::string trimmed = data.substr(3);
    size_t delim = data.find('|');
    message.user = String(trimmed.substr(0, delim).c_str());
    message.msg = String(trimmed.substr(delim + 1).c_str());
    return message;
  }
};

// server stuff down below

#define SERVICE_UUID "b5880dbd-bdff-42de-8fa5-c9850c030346"
#define CHARACTERISTIC_UUID "47e97e19-5ac9-4faf-8607-7110ae14a478"

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  NimBLEDevice::init("HELLO ITS ME2");
  NimBLEServer *pServer = NimBLEDevice::createServer();
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ |
          NIMBLE_PROPERTY::WRITE);
  Message msgtest;
  msgtest.user = "user1";
  msgtest.msg = "this is testing structs being set in characteristic's :3";
  std::string msgserial = msgtest.serialize();
  pCharacteristic->setValue(msgserial);
  pService->start();
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  //----- server ---
}

void loop()
{
  // put your main code here, to run repeatedly:
  Clienting();
  delay(1000);
  HandleSerialPort(pCharacteristic);
  delay(2000);
}
void HandleSerialPort(NimBLECharacteristic *pCharacteristic)
{
  if (Serial.available())
  {
    String rec = Serial.readStringUntil('\n');
    Serial.print("Received: ");
    Serial.println(rec);

    if(rec == "request"){
      if(pCharacteristic != nullptr){
        Serial.println("Characteristic is not null");

        std::string sendmsg = pCharacteristic->getValue();
        Serial.println(sendmsg.c_str());
      }
      else{
        Serial.println("Characteristic is null");
      }
    }else if(rec == "post"){
      if(pCharacteristic != nullptr){
        Serial.println("Characteristic is not null");
        String raw = Serial.readStringUntil('\n');
        Message msg = Message::deserialize(raw.c_str());
        std::string sendmsg = msg.serialize();
        pCharacteristic->setValue(sendmsg);

      }
      else{
        Serial.println("Characteristic is null");
      }
    }
  }
}
void Clienting()
{
  Serial.println("Starting NimBLE client");
  NimBLEScan *pScan = NimBLEDevice::getScan();

  pScan->start(5, false);

  NimBLEScanResults results = pScan->getResults();

  for (int i = 0; i < results.getCount(); i++)
  {
    NimBLEAdvertisedDevice device = results.getDevice(i);
    if (device.isAdvertisingService(NimBLEUUID(SERVICE_UUID)))
    {
      NimBLEClient *pClient = NimBLEDevice::createClient();
      Serial.println("Found our device, connecting");
      if (pClient->connect(&device))
      {
        NimBLERemoteService *pRemoteService = pClient->getService(SERVICE_UUID);

        if (pRemoteService != nullptr)
        {
          NimBLERemoteCharacteristic *pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
          if (pRemoteCharacteristic != nullptr)
          {

            if (pRemoteCharacteristic->canRead())
            {
              std::string value = pRemoteCharacteristic->readValue();
              Message msg = Message::deserialize(value);
              Serial.print("Received message: ");
              Serial.print(msg.user);
              Serial.print(" - ");
              Serial.println(msg.msg);
            }

            /*             if(pRemoteCharacteristic->canWrite()) {
                          String sendmsg = "writing";
                          pRemoteCharacteristic->writeValue(sendmsg);
                          Serial.printf("writing to the characteristic: %s\n", sendmsg);

                        } */
          }
        }
      }
      else
      {
        Serial.println("Failed to connect");
      }
      NimBLEDevice::deleteClient(pClient);
    }
  }
}