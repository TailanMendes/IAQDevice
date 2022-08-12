#include <Arduino.h>
#include <WiFi.h>
#include <Web3.h>
#include "ClosedCube_HDC1080.h"
#include <iomanip>
#include <sstream>
#include "time.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_CCS811.h"

//WIFI
#define SSID     "2G_KZMNDS"
#define WIFI_PASSWD "mnds190518"

/** Collect Sensor Interval = 10 min **/
#define COLLECT_TIME 180000

//INFURA
#define INFURA_HOST "rinkeby.infura.io"
#define INFURA_PATH "/v3/ea9cd517fe4c4782a5f7ee526a578ec5"

/** METAMASK ADDRESS **/
#define MY_ADDRESS "0x8047490dA302959AA294e9d096f2B0803140D63C"

/** Contract Address **/
#define CONTRACT_ADDRESS "0x194E4A079f507208B544eB8EAd2821e3Daa6a8C5"

/** Smart Contract Functions Signature **/
#define SET_MEASURE "setMeasure(string)"

/** MetaMask Private Key **/
const char* private_key = "8dafbc49423c21e71c778421e7a6b30fac6702390b47bd9e37e2969f5df1fd1d";

ClosedCube_HDC1080 hdc1080;
Adafruit_CCS811 ccs;
int wifi_counter = 0;

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime;

double temperature;
double humidity;
uint16_t co2;
uint16_t tvoc;

void setupWiFI();
void sensorCollectData();
void sendDataToSamartContract(string measure);
string formatDataToSend();
unsigned long getTime(); 

Web3 web3(INFURA_HOST, INFURA_PATH);

void setup() 
{
  Serial.begin(115200);

  setupWiFI();

  configTime(0, 0, ntpServer);

  // Default settings: 
	//  - Heater off
	//  - 14 bit Temperature and Humidity Measurement Resolutions
	hdc1080.begin(0x40);

  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  }

  // Wait for the sensor to be ready
  while(!ccs.available());
}

void loop() 
{
  sensorCollectData();

  sendDataToSamartContract(formatDataToSend().c_str());

  Serial.println(formatDataToSend().c_str());

  epochTime = getTime();
  //Serial.print("Epoch Time: ");
  //Serial.println(epochTime);

  delay(COLLECT_TIME);
}

void setupWiFI()
{
  if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(SSID);

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.persistent(false);
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);

        WiFi.begin(SSID, WIFI_PASSWD);
    }

    wifi_counter = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_counter < 10)
    {
        for (int i = 0; i < 500; i++)
        {
            delay(1);
        }
        Serial.print(".");
        wifi_counter++;
    }

    if (wifi_counter >= 10)
    {
        Serial.println("Restarting ...");
        ESP.restart(); 
    }

    delay(10);

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void sensorCollectData()
{
    temperature = hdc1080.readTemperature();
    humidity = hdc1080.readHumidity();
    
    if(ccs.available())
    {
      if(!ccs.readData()){
        
        co2 = ccs.geteCO2();
        
        tvoc = ccs.getTVOC();
      }
      else{
        Serial.println("ERROR!");
        //while(1); // revisar se é necessário
      }
    } 
}

string formatDataToSend()
{
  stringstream fmeasure;

  // Converting double to string and setting precision 2 decimal
  fmeasure << std::fixed << std::setprecision(2) << temperature << "|" << humidity << "|" << co2 << "|" << tvoc << "|" << to_string(getTime());

  //sensor_data = to_string(temperature) + "|" + to_string(humidity); // timestamp

  return fmeasure.str();;
}

void sendDataToSamartContract(string measure)
{
  string _measure = measure;

  Contract contract(&web3, CONTRACT_ADDRESS);
  contract.SetPrivateKey(private_key);
  string addr = MY_ADDRESS;
  uint32_t nonceVal = (uint32_t)web3.EthGetTransactionCount(&addr);

  /** Gas settings - REVIEW **/
  unsigned long long gasPriceVal = 2500000008ULL;
  uint32_t  gasLimitVal = 80000;

  //string p = contract.SetupContractData("setMeasure(string)", measure);
  string p = contract.SetupContractData(SET_MEASURE, _measure);

  string contractAddr = CONTRACT_ADDRESS;
  uint256_t weiValue = 000000000000000;

  string result = contract.SendTransaction(nonceVal, gasPriceVal, gasLimitVal, &contractAddr, &weiValue, &p);

}

/** Get EPOCH time **/
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}