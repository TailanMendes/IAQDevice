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
#define SSID     "KZMNDS_2G"
#define WIFI_PASSWD "mnds190518"

/** Collect Sensor Interval = 3 min **/
#define COLLECT_TIME 180000

//INFURA
#define INFURA_HOST "rinkeby.infura.io"
#define INFURA_PATH "/v3/ea9cd517fe4c4782a5f7ee526a578ec5"

/** METAMASK ADDRESS **/
#define MY_ADDRESS "0x8047490dA302959AA294e9d096f2B0803140D63C"

/** Contract Address **/
#define CONTRACT_ADDRESS "0x495498a9F628Ce581AedA1B5bb6e598090717Df7"

/** Smart Contract Functions Signature **/
#define SET_MEASURE "setMeasure(string)"

/** MetaMask Private Key **/
const char* private_key = "8dafbc49423c21e71c778421e7a6b30fac6702390b47bd9e37e2969f5df1fd1d";

int pinVout2 = 32; // Vout 2 (default output) (Pin #2) from DSM501A - particles over 1 micrometer
int pinVout1 = 27; // Vout 1 (Pin #4) from DSM501A - particles over 2.5 micrometer // using this for PM10
unsigned long pm25_pulseInLow; // the length of the pulse in LOW in microseconds 
unsigned long pm10_pulseInLow;

unsigned long starttime;
unsigned long sampletime_ms = 60000; // 1min seconds
unsigned long pm25_sumTimeOfLow = 0;
unsigned long pm10_sumTimeOfLow = 0;
float pm25_low_ratio = 0;
float pm10_low_ratio = 0;
float pm25_concentration = 0; // concentration in mg/m3
float pm10_concentration = 0; // concentration in mg/m3

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

  pinMode(pinVout2,INPUT);
  pinMode(pinVout1,INPUT);

  //setupWiFI();

  //configTime(0, 0, ntpServer);

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

  starttime = millis(); // current time in milli seconds
}

void loop() 
{

  pm25_pulseInLow = pulseIn(pinVout2, LOW);
  pm10_pulseInLow = pulseIn(pinVout1, LOW);

  pm25_sumTimeOfLow += pm25_pulseInLow;
  pm10_sumTimeOfLow += pm10_pulseInLow;

  if ((millis()-starttime) > sampletime_ms)
  {
    pm25_low_ratio = pm25_sumTimeOfLow/(sampletime_ms*10.0);
    pm10_low_ratio = pm10_sumTimeOfLow/(sampletime_ms*10.0);

    //Serial.print("Sum time in low: ");
    //Serial.println(pm25_sumTimeOfLow);
    //Serial.print("Low ratio: ");
    //Serial.println(pm25_low_ratio);

    pm25_concentration = 0.001915*pow(pm25_low_ratio,2)+0.09522*pm25_low_ratio-0.04884; // using spec sheet curve for mg/m3
    pm10_concentration = 0.001915*pow(pm10_low_ratio,2)+0.09522*pm10_low_ratio-0.04884; // using spec sheet curve for mg/m3

    pm25_concentration = pm25_concentration - pm10_concentration; // Descartando particulas maiores de 2.5 micrometro para ter o PM25 (Vout2-Vout1)

    sensorCollectData();

    Serial.println(formatDataToSend().c_str());
    
    sendDataToSamartContract(formatDataToSend().c_str());

    //epochTime = getTime();

    pm25_sumTimeOfLow = 0;
    pm10_concentration = 0;
    starttime = millis();
  }

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
  fmeasure << std::fixed << std::setprecision(2) << temperature << "|" << humidity << "|" << co2 << "|" << tvoc << "|" << pm25_concentration << "|" << pm10_concentration << "|" << to_string(getTime());

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
  uint32_t  gasLimitVal = 400000;

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