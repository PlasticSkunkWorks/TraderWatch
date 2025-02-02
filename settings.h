// rename this file to `secrets.h`
#define SSID "Tanode24"
#define WIFI_PASSWORD "guidomeda"


//List here your symbols from Yahoo Finance

String MyTickers[] = {"TSLA","BTC-USD","LDO.MI","AAPL","NVDA","DHL.DE"};


#define DAYLIGHT_OFFSET 3600 //usually 1 hr save 

#define TIMEZONE 3600 //central Europe Time 
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0


#define ENABLE_OTA true //to update the device over WIFI
#define DEVICENAME "TickerClock"
#define DEVICEPASSWORD "TickerClock"

///////////////////Do not modify if it is not clear for you

#define NTP_SERVER "pool.ntp.org"

//TIcker timer, don't modify
#define TickerTImer 120000
#define RebootTimer 14400000 //reboot every 4 hrs
#define TimeUpdateTimer 50000 //update time every 10 seconds
#define ScreenTimer 2000  // scroll tra i ticker

// Calculate the length of the array
#define NumberOfTikers sizeof(MyTickers) / sizeof(MyTickers[0])

