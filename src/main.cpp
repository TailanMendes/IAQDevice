#include <Arduino.h>
#include <WiFi.h>
#include <Web3.h>
#include "ClosedCube_HDC1080.h"

//WIFI
#define SSID     "2G_KZMNDS"
#define WIFI_PASSWD "mnds190518"

#define COLLECT_TIME 3000

//INFURA
#define INFURA_HOST "rinkeby.infura.io"
#define INFURA_PATH "/v3/ea9cd517fe4c4782a5f7ee526a578ec5"

/** METAMASK ADDRESS **/
#define MY_ADDRESS "0x8047490dA302959AA294e9d096f2B0803140D63C"

/** Contract Address **/
#define CONTRACT_ADDRESS "0x194E4A079f507208B544eB8EAd2821e3Daa6a8C5"

/** MetaMask Private Key **/
const char* private_key = "8dafbc49423c21e71c778421e7a6b30fac6702390b47bd9e37e2969f5df1fd1d";

ClosedCube_HDC1080 hdc1080;
int wifi_counter = 0;

double temperature;
double humidity;

void setupWiFI();
void sensorCollectData();

void setup() 
{
  Serial.begin(115200);

  setupWiFI();

  // Default settings: 
	//  - Heater off
	//  - 14 bit Temperature and Humidity Measurement Resolutions
	hdc1080.begin(0x40);

}

void loop() 
{
  sensorCollectData();

  Serial.print("T=");
  Serial.print(temperature);
  Serial.print("C, RH=");
	Serial.print(humidity);
	Serial.println("%");

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
}