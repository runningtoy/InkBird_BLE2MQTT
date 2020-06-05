
#include "Arduino.h"
#include "loggingHelper.h"
#include <sstream>
#include <string>
#include <cstring>

#include "BLEDevice.h"
#include <WiFi.h>
#include "credentials.h"
#include <PubSubClient.h>



WiFiClient espClient;
PubSubClient mqttclient(espClient);

// static BLEUUID accountVerifyUUID("0000fff2-0000-1000-8000-00805f9b34fb");
// static BLEUUID realtimeData("0000fff4-0000-1000-8000-00805f9b34fb");
// static BLEUUID settings("0000fff5-0000-1000-8000-00805f9b34fb");
// static BLEUUID settingsResult("0000fff1-0000-1000-8000-00805f9b34fb");
// static BLEUUID historyData("0000fff3-0000-1000-8000-00805f9b34fb");

static uint8_t credentials[] = {0x21, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0xb8, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t enableRealTimeData[] = {0x0B, 0x01, 0x00, 0x00, 0x00, 0x00};
static uint8_t unitCelsius[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t batteryLevel[] = {0x08, 0x24, 0x00, 0x00, 0x00, 0x00};

//Device
static BLEUUID serviceUUID("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID RealtimeData(BLEUUID((uint16_t)0xfff4));
static BLEUUID SettingsData(BLEUUID((uint16_t)0xfff5));
static BLEUUID AccountAndVerify(BLEUUID((uint16_t)0xfff2));
static BLEUUID SettingsResults(BLEUUID((uint16_t)0xfff1));


long lastReconnectAttempt = 0;


bool wifiReConnect();
uint16_t littleEndianInt(uint8_t *pData);
// for more detailed instructions on these settings, see the EspMQTTClient github repo

static BLEAddress *pServerAddress;
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLERemoteCharacteristic *pSettingsCharacteristic;
static BLERemoteCharacteristic *pAccountAndVerifyCharacteristic;
static BLERemoteCharacteristic *pSettingsResultsCharacteristic;


boolean mqttIsConnected(){
  return mqttclient.connected();
}

boolean mqttReconnect() {
  if(mqttclient.connect("BBQClient"))
  {
      ESP_LOGI("BBQ", "MQTT Connected");
  }
  else
     {
      ESP_LOGE("BBQ", "MQTT not Connected");
    }
  return mqttIsConnected();
}

void mqttLoop(){
    if (!mqttclient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (mqttReconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    mqttclient.loop();
  }
}


static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  uint8_t probeId = 1;
  for (int i = 0; i < length; i += 2)
  {
    uint16_t val = littleEndianInt(&pData[i]);
    float temp = val / 10;
    ESP_LOGI("BBQ", "Probe %d has value %f", probeId, temp);
    if(mqttIsConnected()){
       char temperature_out[255];
       sprintf(temperature_out, "inkbird/probe%d",probeId);
       mqttclient.publish(temperature_out, String((int)temp).c_str());
       ESP_LOGI("BBQ", "Publish %d to topic %s",(int)temp, temperature_out);
    }
    probeId++;
  }
}

static void notifyResultsCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  uint16_t currentVoltage= littleEndianInt(&pData[1]); // up to maxVoltage
  uint16_t maxVoltage= littleEndianInt(&pData[3]); // if 0 maxVoltage is 6550
  maxVoltage=(maxVoltage==0)?6550:maxVoltage;
  double battery_percent = (100 * (double)currentVoltage) / (double)maxVoltage;
  ESP_LOGI("BBQ", "currentVoltage %d::maxVoltage %d::perc %d",currentVoltage, maxVoltage,(int)battery_percent);
  if(mqttIsConnected()){
    mqttclient.publish("inkbird/batteryLevel", String((int)battery_percent).c_str());
    ESP_LOGI("BBQ", "Publish %f to topic %s",battery_percent, "inkbird/batteryLevel");
  }
}


void getBatteryData(){
  if (pSettingsCharacteristic != nullptr && pSettingsResultsCharacteristic!=nullptr)
  {
    ESP_LOGI("BBQ", " Request BatteryStatus");
    pSettingsCharacteristic->writeValue((uint8_t *)batteryLevel, sizeof(batteryLevel), true);
  }
}

bool connectToServer(BLEAddress pAddress)
{
  ESP_LOGI("BBQ", "Forming a connection to %s", pAddress.toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  ESP_LOGI("BBQ", " - Created client");

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  ESP_LOGI("BBQ", " - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    ESP_LOGE("BBQ", "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
    return false;
  }
  ESP_LOGI("BBQ", " - Found our service");

  pAccountAndVerifyCharacteristic = pRemoteService->getCharacteristic(AccountAndVerify);
  if (pAccountAndVerifyCharacteristic == nullptr)
  {
    ESP_LOGE("BBQ", "Failed to find our characteristic UUID: %s", AccountAndVerify.toString().c_str());
    return false;
  }
  ESP_LOGI("BBQ", " - Found our AccountAndVerify characteristic");
  ESP_LOGI("BBQ", " - AccountAndVerify");
  pAccountAndVerifyCharacteristic->writeValue((uint8_t *)credentials, sizeof(credentials), true);

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(RealtimeData);
  if (pRemoteCharacteristic == nullptr)
  {
    ESP_LOGE("BBQ", "Failed to find our characteristic UUID: %s", RealtimeData.toString().c_str());
    return false;
  }
  ESP_LOGI("BBQ", " - Found our RealtimeData characteristic");

  pSettingsCharacteristic = pRemoteService->getCharacteristic(SettingsData);
  if (pSettingsCharacteristic == nullptr)
  {
    ESP_LOGE("BBQ", "Failed to find our characteristic UUID: %s", SettingsData.toString().c_str());
    return false;
  }

  ESP_LOGI("BBQ", " - Found our SettingsData characteristic");
  ESP_LOGI("BBQ", " - enableRealTimeData");
  pSettingsCharacteristic->writeValue((uint8_t *)enableRealTimeData, sizeof(enableRealTimeData), true);
  ESP_LOGI("BBQ", " - unitCelsius");
  pSettingsCharacteristic->writeValue((uint8_t *)unitCelsius, sizeof(unitCelsius), true);

  ESP_LOGI("BBQ", " - ADD notifyCallback");
  pRemoteCharacteristic->registerForNotify(notifyCallback);

  pSettingsResultsCharacteristic = pRemoteService->getCharacteristic(SettingsResults);
  if (pSettingsResultsCharacteristic == nullptr)
  {
    ESP_LOGE("BBQ", "Failed to find our characteristic UUID: %s", SettingsResults.toString().c_str());
    return false;
  }
  ESP_LOGI("BBQ", " - Found our SettingsResults characteristic");
  ESP_LOGI("BBQ", " - ADD notifyResultsCallback");
  pSettingsResultsCharacteristic->registerForNotify(notifyResultsCallback);

  getBatteryData();
}


/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  /**
        Called for each advertising BLE server.
    */
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    ESP_LOGV("BBQ", "BLE Advertised Device found: %s", advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(serviceUUID))
    {

      //
      ESP_LOGI("BBQ", "Found our device!  address: ");
      advertisedDevice.getScan()->stop();

      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;

    } // Found our server
  }   // onResult
};    // MyAdvertisedDeviceCallbacks


uint16_t littleEndianInt(uint8_t *pData)
{
  uint16_t val = pData[1] << 8;
  val = val | pData[0];
  return val;
}

void setup()
{
  Serial.begin(115200);
  wifiReConnect();
  mqttclient.setServer(MQTTSERVER, MQTTPORT);
  mqttReconnect();
  lastReconnectAttempt = 0;
  setLogLevel();
  ESP_LOGD("BBQ", "Scanning");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop()
{
  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true)
  {
    if (connectToServer(*pServerAddress))
    {
      ESP_LOGI("BBQ", "We are now connected to the BLE Server.");
      connected = true;
    }
    else
    {
      ESP_LOGI("BBQ", "We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }
  
  mqttLoop();
  getBatteryData();
  delay(10000); // Delay a second between loops.
}

bool wifiReConnect()
{
  int wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 5)
  {
    wifi_retry++;
    // Serial.println("WiFi not connected. Try to reconnect");
    ESP_LOGE("BBQ", "WiFi not connected. Try to reconnect");
    WiFi.disconnect();
    delay(100);
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(DEFAULT_SSID, DEFAULT_WIFIPASSWORD);
    delay(200);
  }
  ESP_LOGE("BBQ", "WiFi connected.");
  return (WiFi.status() == WL_CONNECTED);
}