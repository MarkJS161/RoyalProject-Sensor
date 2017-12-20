#include <DHT.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <WifiLocation.h>

const char* googleApiKey = "AIzaSyDfl9Lfghlhk6dxqNBrMYh9Lae0xGT_05Y";
WifiLocation location(googleApiKey);
location_t loc;

#define DHTR_PIN D3
#define DHTL_PIN D4

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dhtL(DHTL_PIN, DHTTYPE);
DHT dhtR(DHTR_PIN, DHTTYPE);

// Config Firebase
/*
#define FIREBASE_HOST "fir-test-8527e.firebaseio.com"
#define FIREBASE_AUTH "pzjiq7rjhrcdp2gxAFmbIglyF26oXOvlzEqjOz3i"
*/
#define FIREBASE_HOST "realtime-b3259.firebaseio.com"
#define FIREBASE_AUTH "WWacdVqgqE3c7lrV7TzjoOeToD1DHDVNjkdAYLoR"

// Config connect WiFi

#define WIFI_SSID "WiFi_Name"
#define WIFI_PASSWORD "WiFi_PASSWORD"

boolean wifi_status;

// Config time
int timezone = 7;
int dst = 0;
char ntp_server1[20] = "ntp.ku.ac.th";
char ntp_server2[20] = "fw.eng.ku.ac.th";
char ntp_server3[20] = "time.uni.net.th";
String licensePlate = "81-7591";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
  }

  configTime(timezone * 3600, dst, ntp_server1, ntp_server2, ntp_server3);
  Serial.println("Waiting for time");
  while(!time(nullptr)){
    Serial.print(".");
    delay(500);
  }

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  dhtL.begin();
  dhtR.begin();  
}

void loop() {
  // put your main code here, to run repeatedly:
  float humid_L = dhtL.readHumidity();
  float temp_L = dhtL.readTemperature();
  float humid_R = dhtR.readHumidity();
  float temp_R = dhtR.readTemperature();
  loc = location.getGeoFromWiFi();
  
  if (isnan(humid_L) || isnan(temp_L)|| isnan(humid_R) || isnan(temp_R)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  pushSensorValue(temp_L, humid_L, temp_R, humid_R, loc.lat, loc.lon);
  delay(90000); 
}

void pushSensorValue(float tL, float hL, float tR, float hR, float lati, float longit){
  
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root_R = jsonBuffer.createObject();
  JsonObject& root_L = jsonBuffer.createObject();
  JsonObject& root_GPS = jsonBuffer.createObject();
  
  if(NowDate() == "1970-01-01"){
    return;
  }  
    // Sensor-L
    root_L["temperature"] = tL;
    root_L["humidity"] = hL;
    root_L["time"] = NowTime();
    root_L["date"] = NowDate();
    root_L["license"] = licensePlate;
    // Sensor-R
    root_R["temperature"] = tR;
    root_R["humidity"] = hR;
    root_R["time"] = NowTime();
    root_R["date"] = NowDate();
    root_R["license"] = licensePlate;
    // Current Position
    root_GPS["latitude"] = String(lati, 7);
    root_GPS["longitude"] = String(longit, 7);
    root_GPS["date"] = NowDate();
    root_GPS["license"] = licensePlate;

    Firebase.setString("Realtime/date", NowDate());
    Firebase.setString("Realtime/time", NowTime());

    Firebase.setFloat("Realtime/temperature", (tL+tR)/2);
    Firebase.setFloat("Realtime/humidity", (hL+hR)/2);
    Firebase.setFloat("Realtime/temperatureL", tL);
    Firebase.setFloat("Realtime/humidityL", hL);
    Firebase.setFloat("Realtime/temperatureR", tR);
    Firebase.setFloat("Realtime/humidityR", hR);

    Firebase.setString("Realtime/latitude", String(lati, 7));
    Firebase.setString("Realtime/longitude", String(longit, 7));

    Firebase.setFloat("Realtime2/temperature", (tL+tR)/2);
    Firebase.setFloat("Realtime2/humidity", (hL+hR)/2);
    
    Firebase.push("Realtime2/logSensor/sensor_R", root_L);
    Firebase.push("Realtime2/logSensor/sensor_L", root_R);
    Firebase.push("Realtime2/logGPS", root_GPS);
    Serial.println("Push done");
    if (Firebase.failed()) {
        Serial.print("pushing /logs failed:");
        Serial.println(Firebase.error());  
        return;
    }
}

String NowDate(){
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);
  String day = String(newtime->tm_mday);
  String month = String(newtime->tm_mon + 1);

  String tmpNow = "";
  tmpNow += String(newtime->tm_year + 1900);
  tmpNow += "-";
  tmpNow += month.length() == 1 ? "0" + month : month;
  tmpNow += "-";
  tmpNow += day.length() == 1 ? "0" + day : day;
  return tmpNow; 
}

String NowTime(){
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now); 
  String tmpNow = "";
  tmpNow += String(newtime->tm_hour);
  tmpNow += ":";
  tmpNow += String(newtime->tm_min);
  tmpNow += ":";
  tmpNow += String(newtime->tm_sec);
  return tmpNow;
}

