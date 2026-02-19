#include <Wire.h>
#include<WiFi.h>
#include<PubSubClient.h>
#include<NTPClient.h>
#include<WiFiUdp.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <esp_sleep.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

/*
#define WiFi_SSID "............"
#define WiFi_PASSWORD "kkkkoooo"
#define WiFi_SSID "TP-Link_DAE7"
#define WiFi_PASSWORD "50696382"*/
#define WiFi_SSID "TP-Link_FA5C"
#define WiFi_PASSWORD "47845862"
const int sleepDuration = 180 * 1000000;

int Rsoil1=34;
int Rsoil2=36;
float soil1,soil2;
int smap=0;
int se=0;
int combined_soil_moisture1, combined_soil_moisture2,res;

const char *mqtt_broker = "broker.emqx.io";
const int mqtt_Port = 1883;

const char *topic_soil1 = "soil1";
const char *topic_soil2 = "soil2";

  WiFiClient espClient;
  PubSubClient client(espClient);

void setup() {
  res=0;
  Serial.begin(115200);
client.setServer(mqtt_broker, 1883);
lcd.init();
lcd.backlight();
delay(500);
}

float predictTree1(float sensor_value) {
  if (sensor_value <= 1524.76) {
    if (sensor_value <= 1446.0) {
      if (sensor_value <= 1352.34) {
        return 81.5006;
      } else {
        return 74.5023;
      }
    } else {
      if (sensor_value <= 1509.26) {
        return 49.2241;
      } else {
        return 40.0;
      }
    }
  } else {
    if (sensor_value <= 2783.14) {
      if (sensor_value <= 2055.5) {
        return 34.9195;
      } else {
        return 24.5245;
      }
    } else {
      if (sensor_value <= 2949.06) {
        return 11.3068;
      } else {
        return 1.5033;
      }
    }
  }
}

float predictTree2(float sensor_value) {
  if (sensor_value <= 1529.26) {
    if (sensor_value <= 1437.5) {
      if (sensor_value <= 1352.34) {
        return 81.7073;
      } else {
        return 76.0693;
      }
    } else {
      if (sensor_value <= 1508.76) {
        return 50.9582;
      } else {
        return 40.0;
      }
    }
  } else {
    if (sensor_value <= 2783.14) {
      if (sensor_value <= 2055.5) {
        return 34.6555;
      } else {
        return 24.4377;
      }
    } else {
      if (sensor_value <= 2949.06) {
        return 9.5853;
      } else {
        return 1.5797;
      }
    }
  }
}

float predictTree3(float sensor_value) {
  if (sensor_value <= 1525.26) {
    if (sensor_value <= 1410.5) {
      if (sensor_value <= 1392.32) {
        return 81.2642;
      } else {
        return 76.8768;
      }
    } else {
      if (sensor_value <= 1447.5) {
        return 60.6818;
      } else {
        return 49.7;
      }
    }
  } else {
    if (sensor_value <= 2784.14) {
      if (sensor_value <= 2055.5) {
        return 34.8724;
      } else {
        return 24.5015;
      }
    } else {
      if (sensor_value <= 2949.06) {
        return 9.4156;
      } else {
        return 1.5869;
      }
    }
  }
}

// Random Forest Prediction
float randomForestPredict(float sensor_value) {
  float tree1 = predictTree1(sensor_value);
  float tree2 = predictTree2(sensor_value);
  float tree3 = predictTree3(sensor_value);
  return (tree1 + tree2 + tree3) / 3.0;
}

// SVR Prediction
float svrPredict(float sensor_value) {
  if (sensor_value >= 1000 && sensor_value <= 1500) {
    return -0.0758 * sensor_value + 157.4269;
  } else if (sensor_value >= 1501 && sensor_value <= 2500) {
    return -0.0219 * sensor_value + 75.4338;
  } else if (sensor_value >= 2501 && sensor_value <= 3500) {
    return -0.0322 * sensor_value + 99.3774;
  } else {
    return 0.0; // Out of range
  }
}

// Combined Prediction
float combinedPredict(float sensor_value) {
  float rfPrediction = randomForestPredict(sensor_value);
  float svrPrediction = svrPredict(sensor_value);
  return (rfPrediction + svrPrediction) / 2.0;
}

void sleep(){
  esp_sleep_enable_timer_wakeup(sleepDuration);
  lcd.clear();
  lcd.setCursor(1,1);
  lcd.print("Going to deep sleep...");
  delay(1000);
  lcd.noBacklight();
  esp_deep_sleep_start(); 
}
void mq(){
  client.publish(topic_soil1,String(soil1).c_str());
  client.publish(topic_soil2,String(soil2).c_str());
}

void checkWiFi(){
WiFi.begin(WiFi_SSID,WiFi_PASSWORD);
while(WiFi.status() != WL_CONNECTED)
  {
delay(1000);
  Serial.print(".w");
  
  res=res+1;
  Serial.println(res);
  if(res>=10){
    Serial.println("res");
    ESP.restart(); }

  if(res<7){
  lcd.clear();
  lcd.setCursor(1,1);
  lcd.print("no internet");}

  if(res>=7){
  lcd.clear();
  lcd.setCursor(1,1);
  lcd.print("Restart");}}

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("lklsjdiwkjjmoo")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds"); 
      delay(5000);
   }}}

void aggg(){
    HTTPClient http;
        String url = "https://script.google.com/macros/s/AKfycbx8epk8-NIs751vmMJWxTk1z1fH17LN6obBdw__1KjgpT0yMj9VZk_NLeyCMWBQCpAU/exec?data3="+String(soil1)+"&data4="+String(soil2);
        Serial.println("Making a request");
        http.begin(url.c_str()); //Specify the URL and certificate
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET();
        String payload;
        if (httpCode > 0) { //Check for the returning code
          payload = http.getString();
          Serial.println(httpCode);
          Serial.println(payload);
        }
        else {
          Serial.println("Error on HTTP request");
        }
        http.end();
}

void LLCd(){
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("Moisture");
  lcd.setCursor(1,1);
  lcd.print("15cm = ");
  lcd.setCursor(10,1);
  lcd.print(soil1);

  lcd.setCursor(1,2);
  lcd.print("30cm = ");
  lcd.setCursor(10,2);
  lcd.print(soil2);
}

void loop() { 
checkWiFi();

  combined_soil_moisture1=analogRead(Rsoil1);
  combined_soil_moisture2=analogRead(Rsoil2);
  soil1 = combinedPredict(combined_soil_moisture1);
  soil2 = combinedPredict(combined_soil_moisture2);
  smap=map(soil1,0,4025,100,0);
  se=smap-25;
  if(se<0){
    se=0;
  }
  LLCd();
  client.loop();
  reconnect();
  Serial.println(soil1);
  aggg();
  mq();
  sleep();
  }

