#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include "time.h"
#include <SimpleTimer.h>
#include "settings.h" // WiFi Configuration (WiFi name and Password)
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = SSID;
const char* password = WIFI_PASSWORD;

const int httpsPort = 443;

//NTP server coniguration
const char* ntpServer = NTP_SERVER;
const long  gmtOffset_sec = TIMEZONE;
const int   daylightOffset_sec = DAYLIGHT_OFFSET;

//Yhoo finance configuration
const String urlbase = "https://query1.finance.yahoo.com/v8/finance/chart/";
const String terminal="?&range=1d&interval=1mo";

WiFiClient client;
HTTPClient http;

#define TFT_CS        10
#define TFT_RST       9 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

String orario = String("");
String calendario = String("");
struct tm timeinfo;

//to manage recconnect
unsigned long previousMillis = 0;
unsigned long interval = 30000;

//to manage the Various Processes
SimpleTimer TickerUpdateTimer;
SimpleTimer SimpleRebootTimer;
SimpleTimer SimpleTimeUpdateTimer;
SimpleTimer SimpleScreenUpdate;

String shortname[NumberOfTikers]; // = String("");
String Symbol[NumberOfTikers]; // = String("");
String Price[NumberOfTikers]; // = String("");
String Currency[NumberOfTikers]; // = String("");
String tickerString[NumberOfTikers]; // = String("");

int tickerScroll = 0;

void setup() {
  //Debug
  Serial.begin(115200);

  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  //Conect The Wifi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  drawText("Connecting to WiFi...", ST77XX_WHITE);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();


  //configure the timers
  TickerUpdateTimer.setInterval(TickerTImer);
  SimpleRebootTimer.setInterval(RebootTimer);
  SimpleTimeUpdateTimer.setInterval(TimeUpdateTimer);
  SimpleScreenUpdate.setInterval(ScreenTimer);

  // Initialize a NTPClient to get time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //boot update
  printLocalTime();
  updateTickers();
  updateScreen(tickerScroll);

  //OTA Update
  if (ENABLE_OTA){
    StartOTAService();
  }
  
}


void loop() {
    ArduinoOTA.handle();
    unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }

//NTP Synch time :)

  if (SimpleTimeUpdateTimer.isReady()) {                   
    Serial.println("Time Update");
    printLocalTime();
    SimpleTimeUpdateTimer.reset();                       
  }

//Update tickers from Yahoo Finance

  if (TickerUpdateTimer.isReady()) {                    
    Serial.println("Ticker Update");
  //This is printing the ticker 
    updateTickers();
    TickerUpdateTimer.reset();                     
  }
//Update Screen
  if (SimpleScreenUpdate.isReady()) {
    Serial.println("Screen Update " + String(tickerScroll));
    updateScreen(tickerScroll);
    tickerScroll += 1;
    if (tickerScroll >= NumberOfTikers){
      tickerScroll=0;
    }
    SimpleScreenUpdate.reset();   
  }
//Reboot every 4 Hrs
  if (SimpleRebootTimer.isReady()) {                    // Check is ready a second timer
    Serial.println("restarting ESP32"); //cosi va sempre e non gestiamo gli errori
    ESP.restart();
    SimpleRebootTimer.reset();                        // Reset a second timer
  }

  
}

void updateTickers(){
  for (int i = 0; i < NumberOfTikers; i++) {
      Serial.println("Collecting: "+MyTickers[i]);
      printTicker(MyTickers[i],i);
      delay(10);
    }
}
//Print A Ticker
void printTicker(String ticker, int i){
  String url=urlbase+ticker+terminal;
  Serial.print("Connecting to ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();
  StaticJsonDocument<2000000> doc;
  String a=http.getString();

  DeserializationError error = deserializeJson(doc, a);
  
  if (error) {

    Serial.print(F("deserializeJson Failed, probable buffere error trying to fix the json"));
    Serial.println(error.f_str());
      
    int pos = a.indexOf("timestamp");
    Serial.println(pos); 
    a=a.substring(0, pos-2);
    a=a+"}]}}";
    delay(250);
    DeserializationError error2 = deserializeJson(doc, a);
    if (error2) {
      Serial.print(F("deserializeJson Failed with error: "));
      Serial.println(error2.f_str());
      delay(2500);
      return;
    }
  }

  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  shortname[i] = doc["chart"]["result"][0]["meta"]["shortName"].as<String>();
  Symbol[i] = doc["chart"]["result"][0]["meta"]["symbol"].as<String>();
  Price[i] = doc["chart"]["result"][0]["meta"]["regularMarketPrice"].as<String>();
  Currency[i] = doc["chart"]["result"][0]["meta"]["currency"].as<String>();
 
  http.end();

  if (shortname[i].length() > 20) {         //formatta la lunghezza del nome
  shortname[i].remove(20, shortname[i].length()-20);
  shortname[i].trim();
  }
  Serial.print(shortname[i]);
  Serial.print(" ");
  Serial.print(Symbol[i]);
  Serial.print(" ");
  Serial.print(Price[i]);
  Serial.println(Currency[i]);

  // riformatta la stringa del ticker 
  
  // tickerString += String(shortname);
  // tickerString += String(" ");
  // tickerString += String(Symbol);
  // tickerString += String(" ");
  tickerString[i] = String(Price[i]);
  tickerString[i] += String(" ");
  tickerString[i] += String(Currency[i]);

}

//Print Local Time
void printLocalTime(){
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }


  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);

  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);
  
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  
  char timeDay[3];
  strftime(timeDay,3, "%d", &timeinfo);

  char timeMonth[10];
  strftime(timeMonth,10, "%B", &timeinfo);
  
  char timeYear[10];
  strftime(timeYear,10, "%Y", &timeinfo);
  
  Serial.println("Time variables");
  Serial.println(timeHour);
  Serial.println(timeMinute);
  Serial.println(timeWeekDay);
  Serial.println(timeDay);
  Serial.println(timeMonth);
  Serial.println(timeYear);
  Serial.println();

orario = String(timeHour);
orario += String(":");
orario += String(timeMinute);

calendario = String(timeWeekDay);
calendario += String(", ");
calendario += String(timeDay);
calendario += String(" ");
calendario += String(timeMonth);
calendario += String(" ");
calendario += String(timeYear);
}

void updateScreen(int i){
  /* stampa a schermo */
  int posx0 = (160 - shortname[i].length()*6)/2;     // nome titolo
  int posx1 = (160 - Symbol[i].length()*12)/2;      // simbolo ticker
  int posx2 = (160 - tickerString[i].length()*12)/2;  // prezzo
  int posx3 = (160 - orario.length()*12)/2;
  int posx4 = (160 - calendario.length()*6)/2;
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  //tft.setTextWrap(true);
  tft.setTextSize(1);
  tft.setCursor(posx0, 10);
  tft.println(shortname[i]);
  tft.setTextWrap(false);
  tft.setTextSize(2);
  tft.setCursor(posx1, 28);
  tft.setTextSize(2);
  tft.println(Symbol[i]);
  tft.setCursor(posx2, 52);
  tft.println(tickerString[i]);
  tft.setTextSize(2);
  tft.setCursor(posx3, 84);
  tft.println(orario);
  tft.setTextSize(1);
  tft.setCursor(posx4, 108);
  tft.println(calendario);
}

void drawText(String text, uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setTextWrap(true);
  tft.print(text);
 
}

void StartOTAService(){
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(DEVICENAME);

  // No authentication by default
  ArduinoOTA.setPassword(DEVICEPASSWORD);

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();


}
