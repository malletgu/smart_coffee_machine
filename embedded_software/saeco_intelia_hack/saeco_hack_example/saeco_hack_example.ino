/******************************************************************************
 * @file    saeco_hack_example.ino
 * @author  Rémi Pincent - INRIA
 * @date    27/05/2016
 *
 * @brief Make your coffee from anywhere on a SEACO INTELIA coffee machine. 
 * In order to be functional, coffee machine must be hacked : 
 * https://github.com/OpHaCo/smart_coffee_machine
 * remark : data echanges over MQTT not optimized - transmitted strings
 *
 * Project : smart_coffee_machine - saaeco intelia hack
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 *
 * Revision History:
 * https://github.com/OpHaCo/smart_coffee_machine
 * 
 * LICENSE :
 * project_name (c) by Rémi Pincent
 * project_name is licensed under a
 * Creative Commons Attribution-NonCommercial 3.0 Unported License.
 *
 * You should have received a copy of the license along with this
 * work.  If not, see <http://creativecommons.org/licenses/by-nc/3.0/>.
 *****************************************************************************/

/**************************************************************************
 * Include Files
 **************************************************************************/
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "saeco_intelia.h"
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

/**************************************************************************
 * Manifest Constants
 **************************************************************************/
static const char* _ssid = "wifi_ssid";
static const char* _password = "wifi_pass";
static const char* _mqttServer = "mosquitto_serv";

/**************************************************************************
 * Type Definitions
 **************************************************************************/

/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Local Functions Declarations
 **************************************************************************/
static void onMQTTMsgReceived(char* topic, byte* payload, unsigned int length);
static void onSmallCoffeeBtnPress(uint32_t dur);
static void onPowerBtnPress(uint32_t dur);
static String macToStr(const uint8_t* mac);
static void setupWifi(void);
static void setupOTA(void);
static void setupSaecoCoffeeMachine(void);
static void setupMQTT(void);
static void setupMQTTTopics(void);
static void setupMQTTSubs(void);
static void mqttSubscribe(const char* topic);
static void setupMQTTConnection(void);

/**************************************************************************
 * Static Variables
 **************************************************************************/
/** filled lated with mac address */
static String _coffeeMachineName = "saeco_intelia";
static String _coffeeMachineTopicPrefix = "/amiqual4home/machine_place/";
static String _onBtnPressSuffixTopic = "_on_butn_press";
static String _coffeeMachinePowerTopic;
static String _coffeeMachineSmallCupTopic;
static String _coffeeMachineBigCupTopic;
static String _coffeeMachineTeaCupTopic;
static String _coffeeMachineCleanTopic;
static String _coffeeMachineBrewTopic;
static String _coffeeMachineOnBtnPressTopic;
static String _coffeeMachineStatusTopic;

static Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, A0, NEO_GRB + NEO_KHZ400);
static WiFiClient _wifiClient;
static PubSubClient _mqttClient(_mqttServer, 1883, onMQTTMsgReceived, _wifiClient);

/**************************************************************************
 * Macros
 **************************************************************************/

/**************************************************************************
 * Global Functions Defintions
 **************************************************************************/
void setup() {
  strip.begin();
  strip.show(); // Initialise tous les pixels à 'off' (éteint)
  strip.setPixelColor(0, 255, 0, 255);
  strip.show(); // Initialise tous les pixels à 'off' (éteint)
  
  Serial.begin(115200);
  delay(10);
  setupWifi();
  setupMQTT();
  setupOTA();
  setupSaecoCoffeeMachine();
}

void loop() {
  /** TODO OK to call it before checking connectin??? */
  _mqttClient.loop();
  saecoIntelia_update();
  
  if (!_mqttClient.connected()){
    connectMQTT();
  }
  ArduinoOTA.handle();
  
}
 /**************************************************************************
 * Local Functions Definitions
 **************************************************************************/
static void onMQTTMsgReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message on topic ");
  Serial.print(topic);
  Serial.print(" - length = ");
  Serial.println(length);

  if(strcmp(topic, _coffeeMachinePowerTopic.c_str()) == 0)
  {
    saecoIntelia_power();
  }
  else if(strcmp(topic, _coffeeMachineSmallCupTopic.c_str()) == 0)
  {
    saecoIntelia_smallCup();
  }
  else if(strcmp(topic, _coffeeMachineBigCupTopic.c_str()) == 0)
  {
    saecoIntelia_bigCup();
  }
  else if(strcmp(topic, _coffeeMachineTeaCupTopic.c_str()) == 0)
  {
    saecoIntelia_teaCup();
  }
  else if(strcmp(topic, _coffeeMachineCleanTopic.c_str()) == 0)
  {
    saecoIntelia_clean();
  }
  else if(strcmp(topic, _coffeeMachineBrewTopic.c_str()) == 0)
  {
    saecoIntelia_brew();
  }
}



static void onBrewBtnPress(uint32_t dur)
{
  String topic = _coffeeMachineOnBtnPressTopic;
  topic += "/brew";
  
  Serial.print("brew button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(topic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_coffeeMachineOnBtnPressTopic.c_str());
  }
}

static void onSmallCoffeeBtnPress(uint32_t dur)
{
  String topic = _coffeeMachineOnBtnPressTopic;
  topic += "/smallCoffee";
  
  Serial.print("small coffee button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(topic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_coffeeMachineOnBtnPressTopic.c_str());
  }
}

static void onCoffeeBtnPress(uint32_t dur)
{
  String topic = _coffeeMachineOnBtnPressTopic;
  topic += "/Coffee";
  
  Serial.print("coffee button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(topic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_coffeeMachineOnBtnPressTopic.c_str());
  }
}

static void onPowerBtnPress(uint32_t dur)
{
  String topic = _coffeeMachineOnBtnPressTopic;
  topic += _onBtnPressSuffixTopic;
  topic += "/powerButton";
  
  Serial.print("power button pressed during "); 
  Serial.print(dur);
  Serial.println("ms");

  if (!_mqttClient.publish(topic.c_str(), String(dur).c_str())) {
    Serial.print("Publish failed on topic ");
    Serial.println(_coffeeMachineOnBtnPressTopic.c_str());
  }
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

static void setupWifi(void)
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(_ssid);
  
  WiFi.begin(_ssid, _password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printf(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }  
}

static void setupOTA(void)
{
  //Set actions for OTA
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA initialized");
  
}

static void setupSaecoCoffeeMachine()
{
  TsCoffeeBtnPins coffeePins =
  {
    ._u8_smallCupBtnPin = 16,
    ._u8_bigCupBtnPin   = 14,
    ._u8_teaCupBtnPin= NOT_USED_COFFEE_PIN,
    ._u8_powerBtnPin = 15,
    ._u8_coffeeBrewBtnPin= 10,
    ._u8_cleanBtnPin= 4,
    ._u8_onSmallCupBtnPin= 5,
    ._u8_onBigCupBtnPin= 12,
    ._u8_onTeaCupBtnPin= NOT_USED_COFFEE_PIN,
    ._u8_onPowerBtnPin= 13,
    ._u8_onCoffeeBrewBtnPin= 9,
    ._u8_onCleanBtnPin= NOT_USED_COFFEE_PIN,
  };
  
  
  TsButtonPressCb coffeeBtnPressCb = {
    ._pf_onSmallCupBtnPress = &onSmallCoffeeBtnPress,
    ._pf_onBigCupBtnPress = &onCoffeeBtnPress,
    ._pf_onTeaCupBtnPress= NULL,
    ._pf_onPowerBtnPress= &onPowerBtnPress,
    ._pf_onCoffeeBrewBtnPress= &onBrewBtnPress,
    ._pf_onCleanBtnPress = NULL,
  };
  saecoIntelia_init(&coffeePins, &coffeeBtnPressCb); 
  Serial.println("Saeco coffee machine initialized");
}

static void setupMQTT(void)
{
  setupMQTTTopics();
  connectMQTT();
}

static void setupMQTTTopics(void)
{
  _coffeeMachineTopicPrefix += _coffeeMachineName;

  _coffeeMachinePowerTopic = _coffeeMachineTopicPrefix + '/' + "power";
  _coffeeMachineSmallCupTopic = _coffeeMachineTopicPrefix + '/' + "small_cup";
  _coffeeMachineBigCupTopic = _coffeeMachineTopicPrefix + '/' + "big_cup";
  _coffeeMachineTeaCupTopic = _coffeeMachineTopicPrefix + '/' + "tea_cup";
  _coffeeMachineCleanTopic = _coffeeMachineTopicPrefix + '/' + "clean";
  _coffeeMachineBrewTopic = _coffeeMachineTopicPrefix + '/' + "brew";
  _coffeeMachineStatusTopic= _coffeeMachineTopicPrefix + '/' + "status";
  _coffeeMachineOnBtnPressTopic= _coffeeMachineTopicPrefix + '/' + "on_button_press";
}

static void setupMQTTSubs(void)
{
  mqttSubscribe(_coffeeMachinePowerTopic.c_str());
  mqttSubscribe(_coffeeMachineSmallCupTopic.c_str());
  mqttSubscribe(_coffeeMachineBigCupTopic.c_str());
  mqttSubscribe(_coffeeMachineTeaCupTopic.c_str());
  mqttSubscribe(_coffeeMachineCleanTopic.c_str());
  mqttSubscribe(_coffeeMachineBrewTopic.c_str());
}

static void mqttSubscribe(const char* topic)
{
  if(_mqttClient.subscribe (topic))
  {
    Serial.print("Subscribe ok - to ");
    Serial.println(topic);
  }
  else {
    Serial.print("Subscribe ko to ");
    Serial.println(topic);
  }
}

static void connectMQTT(void)
{
  Serial.print("Connecting to ");
  Serial.print(_mqttServer);
  Serial.print(" as ");
  Serial.println(_coffeeMachineName);

  while (true)
  {
    if (_mqttClient.connect(_coffeeMachineName.c_str())) {
      setupMQTTSubs();
      Serial.println("Connected to MQTT broker");
      Serial.print("send alive message to topic ");
      Serial.println(_coffeeMachineStatusTopic.c_str());
      if (!_mqttClient.publish(_coffeeMachineStatusTopic.c_str(), "alive")) {
        Serial.println("Publish failed");
      }
      return;
    }
    else
    {
      Serial.println("connection to MQTT failed\n");
    }
    delay(2000);
  }
}








