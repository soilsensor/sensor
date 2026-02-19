#include<WiFi.h> //ใช้ในการเชื่อมต่อ WiFi
#include<PubSubClient.h> //ใช้ในการสื่อสารกับโปโตคอล mqtt
#include <HTTPClient.h> //ใช้ในการส่งข้อมูลขึ้น Google Sheet
#include <LiquidCrystal_I2C.h> //ใช้ในการแสดงข้อความขึ้น LCD (หาก .init() ขึ้น Eror ให้เปลี่ยนเป็น .begin() )
#include <esp_sleep.h> //ใช้ในการทำให้ตัวบอร์ด esp ทำการ Sleep
#include <ArduinoOTA.h>//ใช้ในการอัพโหลดโปรแกรมลงบอร์ดแบบไร้สาย
#include <Preferences.h>//ใช้ในการจดจำข้อมูลบางอย่างแม้จะเข้าโหมด Sleep หรือ Reset
#include <WiFiClientSecure.h>//ใช้ในการส่งข้อมูลไปยัง Line

int otaNO = 1;//ลำดับของบอร์ดที่จะทำการอัพโหลด([แปลงที่ 1 ใช้เลข 1])
const char *topic_soil1 = "soil1";//ชื่อของ topic ที่ต้องการส่งขึ้น broker ของ 15 cm
const char *topic_soil2 = "soil2";//ชื่อของ topic ที่ต้องการส่งขึ้น broker ของ 30 cm
LiquidCrystal_I2C lcd(0x27, 20, 4); 

Preferences prefs;//สร้างตัวแปรเพื่อใช้ในการเขียนฟังชันให้กับการจดจำข้อมูลบางอย่าง

const char* host = "api.line.me";//เป็น host ที่ใช้ในการเชื่อมต่อกับ line (เป็นชื่อแบบฟิก)
const int httpsPort = 443;//port ที่ใช้ในการเชื่อมต่อกับ line (เป็นชื่อแบบฟิก)

String accessToken = "LINE Access Token";// ใส่ LINE Access Token ของคุณ
String User_ID = "User ID";//ใส่ User ID ของ Line developer

float BatA;//ตัวแปรในการเก็บค่าแบตเตอร์รี่
float percent,vBat; //ตัวแปรในการเก็บค่าเปอร์เซนและแรงดันของแบตเตอร์รี่
//ทำการเชื่อมต่อกับ LCD แบบ I2C โดยที่ 0x27 คือแอดเดรสของ LCD (ตัว LCD จะมีอยู่ประมาณสองแอดเดรสหาก 0x27 ไม่สามารถใช้งานได้ให้เปลี่ยนเป็น 0x3F)

//#define WiFi_SSID "TP-Link_DAE7"//ssid หรือชื่อของ WiFi ที่ต้องการทำการเชื่อมต่อ
//#define WiFi_PASSWORD "50696382" //password หรือรหัสในการเชื่อมต่อ WiFi
#define WiFi_SSID "TP-Link_FA5C"//ssid หรือชื่อของ WiFi ที่ต้องการทำการเชื่อมต่อ
#define WiFi_PASSWORD "47845862" //password หรือรหัสในการเชื่อมต่อ WiFi //password หรือรหัสในการเชื่อมต่อ WiFi
const int sleepDuration = 170 * 1000000; //ตั้งเวลาในการ Sleep โดยมีหน่วยเวลาเป็นวินาทีในโปรแกรมนี้ตัเงเวลาเป็น 180 วินาทีหรือ 3 นาที
#define BTPIN 33 //ขา pin ของ Battery

int Rsoil1=34; //ขา pin ในการอ่านค่าเซ็นเซอร์ความชื้นในดินขาที่ 1
int Rsoil2=35; //ขา pin ในการอ่านค่าเซ็นเซอร์ความชื้นในดินขาที่ 2
float soil1,soil2; //ตัวแปรในการเก็บค่าคาริเบทเซนเซอร์
float Csoil1,Csoil2,SumSoil1,SumSoil2,Ssoil1,Ssoil2;
int res; //ตัวแปรในการเคาท์เพื่อทำการรีเซ็ตบอร์ต

unsigned long Time,currentMills;//ตัวแปรเทียบเวลาและตัวแปรเวลา
int otaUp;//ตัวแปรในการเข้าโหมดอัพโหลดโปรแกรมแบบไร้สาย
int otabegin;//ตัวแปรในทำให้การการเริ่มทำงานการอัพโหลดโปรแกรมทำงานเพียงครั้งเดียว
const char *mqtt_broker = "broker.emqx.io"; //ชื่อของ broker ที่ต้องเชื่อมต่อ
const int mqtt_Port = 1883; //port ของ mqtt

int TGS = otaNO*2;//ใช้ในการส่งขึ้น Google Sheet และ node red
WiFiClient espClient; //ทำการสร้าง Client ของ WiFi
PubSubClient client(espClient); //ทำการสร้าง Client ของ mqtt 

void setup() {
Serial.begin(115200);//ใช้การสื่อสารแบบ port อนุกรมเพื่อใช้ในการสื่อสารระหว่างบอร์ดและคอมพิวเตอร์(สามารถตั้งค่าได้)
client.setServer(mqtt_broker, mqtt_Port);//ทำการตั้งค่าการเชื่อมต่อกับ Broker
lcd.init();//ทำการเริ่มใช้งานหน้าจอ LCD (หากคุณทำการติดตั้งไลบรารี่ของหน้าจอ LCD แล้ว เกิดการ Error ในบรรทัดนี้ให้เปลี่ยนจาก lcd.init(); เป็น lcd.begin();)
lcd.backlight();//ทำการเปิดใช้งานโหมด backlight เพื่อให้หน้าจอ LCD สว่าง
client.setCallback(callback);//ตั้งค่าการรับข้อมูลกลับมาจาก Node red
delay(500);
}

//ฟังชั่นในการเข้าโหมด deep sleep
void sleep(){
  esp_sleep_enable_timer_wakeup(sleepDuration);//ทำการตั้งค่าเวลาเวลาในการ sleep
  lcd.clear();//เครียหน้าจอ LCD (ลบข้อความทั้งหมด)
  lcd.setCursor(1,1);//เริ่มแสดงข้อความที่ 1 บรรทัดที่ 1
  lcd.print("Going to deep sleep...");//ทำการแสดงข้อความ
  delay(1000);
  lcd.clear();//เครียหน้าจอ LCD (ลบข้อความทั้งหมด)
  lcd.noBacklight();//ทำการปิดโหมด Backlight
  esp_deep_sleep_start(); //ทำการเริ่มเข้าสู่โหมด deep sleep
}

//ฟังชั่นในการการแปลงค่าที่อ่านมาจากแบตเตอร์รี่เป็นค่า เปอร์เซ็น
float getBatteryPercent() {
  int adc = analogRead(BTPIN);//ทำการอ่านค่าจากแบตเตอร์รี่
  float vADC = (adc / 4095.0) * 3.3;//ทำการเปลี่ยนค่าอนาล็อกที่ได้อ่านมาจากแบตเตอร์รี่ เป็น 0 ถึง 3.3
  Serial.println("BV="+String(vADC));//แสดงค่าที่แปลงจากค่าอนาล็อกเป็น 0 ถึง 3.3
  float vper = ((0.3-(2.78-(vADC+0.11)))/0.3)*100;//เปลี่ยนค่าจาก 3.3 เป็นค่าเปอร์เซ็น
  Serial.println("BP="+String(vper));
  return vper;//ส่งค่าเปอเซ็นกลับไปใช้
}

//ฟังชั่นในการเปลี่ยนค่าเปอร์เซ็นเป็นแรงดันไฟฟ้า
float readBatteryVoltage(float vper) {
  float vADC = (vper/100)*1.4;
  float Vbb = 11.7+vADC;
  return Vbb;
}

//ฟังชั่นในการนำค่าต่างๆไปยัง node red 
void mq(){
  percent = getBatteryPercent();//ดึงค่าเปอร์เซ็นแบตเตอร์รี่
  vBat = readBatteryVoltage(percent);//ดึงค่าแรงดันแบตเตอร์รี่
  if(percent>=100){percent=100;}//ทำให้ค่าเปอร์เซ็นแบตเตอรี่ไม่มากกว่า 100
  if(percent<=0){percent=0;}//ทำให้ค่าเปอร์เซ็นแบตเตอรี่ไม่น้อยกว่า 0
  Serial.print("Battery Voltage: ");//แสดงข้อความบน Serial moniter 
  Serial.print(vBat, 2);//แสดงค่าแรงดันแบตเตอร์รี่บน Serial moniter โดยมีทศนิยม 2 ตำแหน่ง
  Serial.print(" V | Battery %: ");//แสดงข้อความบน Serial moniter 
  Serial.print(percent);//แสดงค่าเปอร์เซ็นแบตเตอร์รี่บน Serial moniter
  Serial.println("%");//แสดงข้อความบน Serial moniter
  client.publish(("BV"+String(otaNO)).c_str(),String(vBat).c_str());//ส่งค่าแรงดันของแบตเตอร์รี่ไปยัง node red
  client.publish(("BP"+String(otaNO)).c_str(),String(percent).c_str());//ส่งค่าเปอร์เซ็นของแบตเตอร์รี่ไปยัง node red
  client.publish(topic_soil1,String(soil1).c_str());//ส่งค่าความชื้นในดินที่ 15 cm
  client.publish(topic_soil2,String(soil2).c_str());//ส่งค่าความชื้นในดินที่ 30 cm
}

//ฟังชั่นในการตรวจสอบ WiFi
void checkWiFi(){
WiFi.begin(WiFi_SSID,WiFi_PASSWORD);//ทำการเซ็ตค่า WiFi
while(WiFi.status() != WL_CONNECTED)//ตรวจสอบ WiFi
  {
delay(1000);//ทำการดีเรย์ 1 วินาที
  Serial.print(".");//ทำการแสดง . ใน Serial moniter เมื่อเชื่อมต่อ WiFi ไม่ได้
  
  res=res+1;//ทำการบวกค่า 1 ทุกๆ 1 วินาทีเพื่อใช้ในการเข้าเงื่อนไขรีเซตบอร์ด
  Serial.println(res);//แสดงค่าวินาทีในการรีเซตบอร์ดเมื่อค่าเป็น 10 จะทำการรีเซตบอร์ด
  //เมื่อไม่สามารถเชื่อมต่อ WiFi ได้จะทำการรีเซ็ตบอร์ดเมื่อเชื่อมต่อไม่ได้เป็นเวลา 10 วิ
  if(res>=10){//เงื่อนไขในการ รีเซตบอรืด
    ESP.restart(); //ทำการรีเซ็ตบอร์ด
    }

  if(res<7){//เงื่อนไขในการแสดงข้อความบนหน้าจอ LCD เมื่อเชื่อมต้อ WiFi ไม่ได้
  lcd.clear();//เครียหน้าจอ LCD (ลบข้อความทั้งหมด)
  lcd.setCursor(1,1);//เริ่มแสดงข้อความที่ 1 บรรทัดที่ 1
  lcd.print("no internet");}//ทำการแสดงข้อความบนหน้าจอ LCD

  if(res>=7){//เงื่อนไขในการแสดงข้อความบนหน้าจอ LCD เมื่อจะเริ่มทำการรีเซจบอร์ด
  lcd.clear();//เครียหน้าจอ LCD (ลบข้อความทั้งหมด)
  lcd.setCursor(1,1);//เริ่มแสดงข้อความที่ 1 บรรทัดที่ 1
  lcd.print("Restart");}}//ทำการแสดงข้อความบนหน้าจอ LCD
  
  res=0;//เมื่อสามารถเชื่อมต่อ WiFi ได้ จะทำการเปลี่ยนให้เงื่อนไขในการรีเซตบอร์ดมีค่าเป็น 0
  Serial.println("WiFi connected");//แสดงข้อความบน Serial moniter เมื่อสามารถเชื่อมต่อ WiFi ได้
  Serial.print("IP address: ");//แสดงข้อความบน Serial moniter 
  Serial.println(WiFi.localIP());//แสดง IP ของ WiFi บน Serial moniter
  /*ความรู้เพิ่มเติม
    ถึงแม้จะเขียนฟังชั้นในการเชื่อมต่อ WiFi ได้ถูกต้อง และทำการเปิด WiFi จากอุปกรณ์ต่างๆแล้ว แต่ในบางครั้งบอร์ดไมโครคอนโทรเลอร์ก็ไม่สามารถเชื่อมต่อกับ WiFi ได้
    จึงจำเป็นต้องทำการรีเซ็ตบอร์ดเพื่อที่จะให้ตัวบอร์ดสามารถเชื่อมต่อ WiFi ได้ การใช้ฟังชั่นในการรีเซ็ตบอร์ดจึงจะทำให้ไม่จำเป็นต้องกดรีเซ็ตที่บอร์ดด้วยตนเอง*/
}

//ฟังชั่นในการเชื่อมต่อกับ broker ใหม่เมื่อไม่สามารถเชื่อมต่อกับ broker ได้
void reconnect() {
  if (!client.connected()) {//เงื่อนไขเมื่อเชื่อมต่อกับ mqtt ไม่ได้
    Serial.print("Attempting MQTT connection...");//แสดงข้อความเมื่อเชื่อมต่อกับ mqtt ไม่ได้
    if (client.connect("soil_analog_1")) {//ใส่ข้อความอะไรก็ได้ที่ท่านคิดว่าจะไม่ซ้ำกับผู้อื่นหรือไม่ซ้ำกันในจำนวนมาก(หากไม่สามารถเชื่อมต่อกับ mqtt ได้ลองทำการเปลี่ยนข้อความ)
      Serial.println("connected");//แสดงข้อความบน Serial moniter
      client.subscribe(("ota"+String(otaNO)).c_str());//ทำการติดตาม topic ที่จะถูกส่งกลับมาจาก node red (เป็นชื่อของ topic ใน mqtt out จาก node red)
    } else {//หากไม่สามารถเชื่อมต่อกับ mqtt ได้จะเข้ามาในเงื่อนไข else 
      Serial.print("failed, rc=");//แสดงข้อความบน Serial moniter
      Serial.print(client.state());//แสดงสเตตัสหรือสาเหตุที่ไม่สามารถเชื่อมต่อกับ mqtt ได้
      /*สเตตัสทั้งหมดมีดังนี้
           0 : สามารถเชื่อมต่อ mqtt ได้
          -1 : รอการตอบกลับนานเกินไป (ตัว broker อาจมีปัญหา หรือ ไม่เสถียร ลองทำการเปลี่ยน broker)
          -2 : การเชื่อมต่อถูกตัดขาด (คุณอาจจะหลุดการเชื่อมต่อระหว่างตัว PC ของคุณกับตัว Broker)
          -3 : ไม่สามารถเชื่อมต่อกับ broker ได้ (คุณอาจจะใส่ broker ใน code ผิด)
          -4 : ถูกตัดการเชื่อมต่อ(ในฟังชั่น client.connect ข้อความอาจจะซ้ำกันกับผู้อื่นมากเกินไป)
          -5 : ไม่ได้รับสิทธิ์ในการเชื่อมต่อ (ในกรณีที่คุณใช้ Broker ที่สามารถใส่ User กับ password ได้ ตัว User ไม่ได้รับสิทธิในการเข้าถึงการใช้งาน อาจเกิดจากหมดอายุการใช้งาน ในกรณีดังกล่าวโดยส่วนใหญ่จะเป็นการใช้ broker แบบเสียค่าใช้จ่าย)
          -6 : user หรือ password ผิด (ในกรณีที่คุณใช้ Broker ที่สามารถใส่ User กับ password ได้ ตัว User หรือ password ใน code อาจจะผิด ในกรณีดังกล่าวโดยส่วนใหญ่จะเป็นการใช้ broker แบบเสียค่าใช้จ่าย)
          -7 : เซิร์ฟเวอร์ไม่พร้อมใช้งาน (เซิร์ฟเวอร์อาจกำลังถูกปรับปรุง)*/
      Serial.println(" try again in 5 seconds"); //แสดงข้อความบน Serial moniter
      delay(5000);//ทำการดีเลย์เพื่อทำการเชื่อมต่อกับ broker ใหม่(การเชื่อมต่อ broker ใหม่ควรมี ดีเลย์อย่างน้อย 5 วินาที)
   }}}

//ฟังชั่นในการส่งข้อมูลไปยังเซ็นเซอร์ไปยัง Google sheet
void aggg(){
    HTTPClient http;//สร้างตัวแปรที่จะใช้ในการเขียนฟังชั่นในการส่งขึ้น google sheet
        //สร้างตัวแปรในการเก็บ url ของ app scrip และข้อมูลที่ต้องการส่งไปยัง google sheet (นำ url ของ app scrip ตามด้วย ? ตามด้วยชื่อข้อมูลที่ทำการสร้างไว้ใน App scrip ตามด้วย = และข้อมูลที่ต้องการส่งและหากมีข้อมูลมากกว่าหนึ้งให้ตามด้วย & ตามด้วยชื่อ= ข้อมูล)
        String url = "https://script.google.com/macros/s/AKfycbx8epk8-NIs751vmMJWxTk1z1fH17LN6obBdw__1KjgpT0yMj9VZk_NLeyCMWBQCpAU/exec?data"+String(TGS-1)+"="+String(soil1)+"&data"+String(TGS)+"="+String(soil2);
        Serial.println("Making a request");//แสดงข้อความบน Serial moniter เมื่อทำการส่งขึ้น Google Sheet
        http.begin(url.c_str()); //ทำการส่งข้อมูลไปยัง Google Sheet   
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);//ทำการติดตามข้อมูลที่ส่งไปยัง Google Sheet 
        int httpCode = http.GET();//ทำการรับสเตต้สของข้อมูลที่ทำการติดตาม
        String payload;//สร้างตัวแปรเก็บข้อความที่ถูกส่งกลับมา
        if (httpCode > 0) { //ตรวจสอบสเตตัสการส่งข้อมูล
          payload = http.getString();//รับข้อมูลที่ได้ทำการติดตาม
          Serial.println(httpCode);//แสดงเสตัสบน Serial moniter
          Serial.println(payload);//แสดงข้อมูลที่ได้ทำการติดตามบน Serial moniter
        }
        else {//เงื่อนไขเมื่อไม่สามารถส่งข้อมูลไปยัง Google Sheet ได้
          Serial.println("Error on HTTP request");//แสดงข้อความบน Serial moniter เมื่อไม่สามารถส่วข้อมูลไปยัง Google Sheet ได้
        }
        http.end();//สิ้นสุดการใช้งานการส่งข้อมูลขึ้น Google Sheet (ควรจะสิ้นสุดการใช้งานทุกครั้งเมื่อการส่งเสร็จสิ้นเพื่อที่จะสามารถส่งข้อมูลไปยัง Google Sheet ได้ในครั้งถัดไป)
        }

//ฟังชั่นในการแสดงข้อความขึ้นหน้าจอ LCD
void LLCd(){
  lcd.clear();//เครียหน้าจอ LCD (ลบข้อความทั้งหมด)
  lcd.setCursor(6,0);//เครียหน้าจอ LCD (ลบข้อความทั้งหมด)
  lcd.print("Moisture");//แสดงข้อความขึ้นหน้าจอ LCD
  lcd.setCursor(1,1);//เริ่มแสดงข้อความที่ 1 บรรทัดที่ 1
  lcd.print("Soil1 = ");//แสดงข้อความขึ้นหน้าจอ LCD
  lcd.setCursor(11,1);//เริ่มแสดงข้อความที่ 11 บรรทัดที่ 1
  lcd.print(soil1);//แสดงค่าความชื้นในดินที่ 15 cm ขึ้นหน้าจอ LCD

  lcd.setCursor(1,2);//เริ่มแสดงข้อความที่ 1 บรรทัดที่ 2
  lcd.print("Soil2 = ");//แสดงข้อความขึ้นหน้าจอ LCD
  lcd.setCursor(11,2);//เริ่มแสดงข้อความที่ 11 บรรทัดที่ 2
  lcd.print(soil2);//แสดงค่าความชื้นในดินที่ 30 cm ขึ้นหน้าจอ LCD
}

//ฟังชั่นมรการอ่านค่าความชื้นในดิน
void Get_soil() {
    int combined_soil_moisture1=analogRead(Rsoil1);//ทำการอ่านค่าความชื้นในดินแบบอนาล็อกที่ 15 cm
    int combined_soil_moisture2=analogRead(Rsoil2);//ทำการอ่านค่าความชื้นในดินแบบอนาล็อกที่ 30 cm
    Csoil1 = (combined_soil_moisture1 * 3.3) / (4095.00);//ทำการแปลงค่าจากอนาล็อกเป็น 0 ถึง 3.3 ที่ 15 cm
    Csoil2 = (combined_soil_moisture2 * 3.3) / (4095.00);//ทำการแปลงค่าจากอนาล็อกเป็น 0 ถึง 3.3 ที่ 30 cm
    Ssoil1 = ((-58.82) * Csoil1) + 123.52;//ทำการคาริเบจจากค่า 0 ถึง 3.3 เป็น 0 ถึง 100 ที่ 15 cm
    Ssoil2 = ((-58.82) * Csoil2) + 123.52;//ทำการคาริเบจจากค่า 0 ถึง 3.3 เป็น 0 ถึง 100 ที่ 30 cm
    //ทำให้ค่าความชื้นในดินที่ 15 cm มีค่าไม่น้อยกว่า 0 และมากกว่า 100
    if (Ssoil1 <= 0) {Ssoil1 = 0;}
    else if (Ssoil1 >= 100) {Ssoil1 = 100;}
    //ทำให้ค่าความชื้นในดินที่ 30 cm มีค่าไม่น้อยกว่า 0 และมากกว่า 100
    if (Ssoil2 <= 0) {Ssoil2 = 0;}
    else if (Ssoil2 >= 100) {Ssoil2 = 100;}
    }

/*ฟังชั่นในการรับข้อมูลกลับมาจาก node red
  การใช้การรับข้อมูลกลับมาจาก node red จำเป็นต้องมีตัวแปรสามตัวได้แก่ 
  1. topic จำเป็นต้องเป็นตัวแปรประเภท char เป็นตัวแปรในการรับชื่อของข้อมูลที่ถูกส่งกลับมา
  2. paylode จำเป็นต้องเป็นตัวแปรประเภท byte เป็นตัวแปรในการรับข้อมูลกลับมา
  3. length จำเป็นต้องเป็นตัวแปรประเภท unsigned int เป็นตัวแปรในการเก็บจำนวนตัวอักษรของข้อมูลที่ถูกส่งกลับมา*/
void callback(char* topic, byte* payload, unsigned int length) {
  String message;//สร้างตัวแปรในการเก็บข้อมูล
  Serial.print("Message arrived [");//แสดงข้อความบน Serial moniter
  Serial.print(topic);//แสดง topic ขึ้น Serial moniter
  Serial.print("] = ");//แสดงข้อความบน Serial moniter
  for (int i = 0; i < length; i++) {//ทำการเรียงข้อมูลจากจำนวนตัวอักษรของข้อมูลที่ถูกส่งกลับมา (length)
    message += (char)payload[i];//เรียงข้อมูลเก็บไว้ในตัวแปร
  }
  otaUp = message.toInt();//เปลี่ยนข้อมูลที่ถูกส่งกลับมาเป็น เลขจำนวนเต็ม
  if(topic==("ota"+String(otaNO)).c_str()){//เงื่อนไขในการตรวจสอบชื่อของข้อมูล (topic) 
    if(otaUp==1){Serial.println("ota="+String(otaUp));}//แสดงข้อมูลที่ถูกส่งกลับมาบน Serial moniter
  }
}

//ฟังชั่นในการส่งค่าของแบตเตอร์รี่ไปยัง Google Sheet
void GSBatery(){
  BatA=prefs.getInt("BatA",0);//ทำการดึงค่าแบตเตอร์รี่ที่ได้ทำการจดจำไว้และเมื่อยังไม่มีค่าที่ได้ทำการจดจำไว้จะให้จดจำค่าแรกคือ 0 ไว้ใน "BatA"
  for(int j=0;j<=20;j++){//ลูปในการเข้าเงื่อนเพื่อใช้ในการส่งเปอร์เซ็นแบตเตอรี่ไปยัง Google Sheet
    if(BatA!=j*5){//เงื่อนไขในการไม่ให้ทำการส่งซ้ำค่าเปอร์เซ็นเดิมไปยัง Google Sheet
      if(percent>=(j*5)-1 && percent<=(j*5)+1){//เงื่อนเพื่อใช้ในการส่งเปอร์เซ็นแบตเตอรี่ไปยัง Google Sheet
        BatA=j*5;//ทำการเก็บค่าเปอเซ็นแบตเตอร์รี่เพื่อทำการจดจำและส่งขึ้น Google Sheet
        //sendLineMessage("Battery"+String(otaNO)+" = "+String(j*5)+" %");//ทำการส่งข้อมูลไปยัง line
        prefs.putInt("BatA",BatA);//ทำการจดจำค่าแบตเตอร์รี่เก็บไว้ใน "BatA"
        HTTPClient http;//ทำการสร้างตัวแปรเพื่อใช้ในการส่งค่าแบตเตอรี่ไปยัง Google Sheet
        //สร้างตัวแปรในการเก็บ url ของ app scrip และข้อมูลที่ต้องการส่งไปยัง google sheet (นำ url ของ app scrip ตามด้วย ? ตามด้วยชื่อข้อมูลที่ทำการสร้างไว้ใน App scrip ตามด้วย = และข้อมูลที่ต้องการส่งและหากมีข้อมูลมากกว่าหนึ้งให้ตามด้วย & ตามด้วยชื่อ= ข้อมูล)
        String url = "https://script.google.com/macros/s/AKfycbwhVUVgQ4lxc6ckUyK2_jsk0Xm2vLFTLDnEhvqlHSVWC2JF0kleMNcdByEF9ey4oxTttQ/exec?bt"+String(otaNO)+"="+String(vBat)+"&bv"+String(otaNO)+"="+String(j*5);
        Serial.println("Making a request");//แสดงข้อความบน Serial moniter
        http.begin(url.c_str());//ทำการส่งข้อมูลไปยัง Google Sheet    
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);//ทำการติดตามข้อมูลที่ส่งไปยัง Google Sheet 
        int httpCode = http.GET();//ทำการรับสเตต้สของข้อมูลที่ทำการติดตาม
        String payload;//สร้างตัวแปรเก็บข้อความที่ถูกส่งกลับมา
        if (httpCode > 0) {//ตรวจสอบสเตตัสการส่งข้อมูล
          payload = http.getString();//รับข้อมูลที่ได้ทำการติดตาม
          Serial.println(httpCode);//แสดงเสตัสบน Serial moniter
          Serial.println(payload);//แสดงข้อมูลที่ได้ทำการติดตามบน Serial moniter
        }
        else {//เงื่อนไขเมื่อไม่สามารถส่งข้อมูลไปยัง Google Sheet ได้
          Serial.println("Error on HTTP request");//แสดงข้อความบน Serial moniter เมื่อไม่สามารถส่วข้อมูลไปยัง Google Sheet ได้
        }
        http.end();//สิ้นสุดการใช้งานการส่งข้อมูลขึ้น Google Sheet (ควรจะสิ้นสุดการใช้งานทุกครั้งเมื่อการส่งเสร็จสิ้นเพื่อที่จะสามารถส่งข้อมูลไปยัง Google Sheet ได้ในครั้งถัดไป)
      }
    }
  }
}

//ฟังชั่นในการส่งข้อมูลไปยัง Line 
void sendLineMessage(String message) {
  WiFiClientSecure client;
  client.setInsecure();  

  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }

  String url = "/v2/bot/message/push";
  String payload = "{\"to\":\""+User_ID+"\",\"messages\":[{\"type\":\"text\",\"text\":\"" + message + "\"}]}";
  
  String request = "POST " + url + " HTTP/1.1\r\n" +
                   "Host: " + String(host) + "\r\n" +
                   "Authorization: Bearer " + accessToken + "\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + String(payload.length()) + "\r\n\r\n" +
                   payload;
  
  client.print(request);
  Serial.println("Message Sent!");
}


void loop() { 
if (otaUp==0){//เงื่อนไขในเพื่อใช้ในการอัพโหลดโปรแกรมแบบไร้สาย(0=ทำงานปกติและไม่พร้อมอัพโหลดโปรแกรมแบบไร้สาย)
checkWiFi();//ฟังชั่นในการตรวจสอบ WiFi
reconnect();//ฟังชั่นในการตรวบสอบ mqtt
client.loop();//เป็นฟังชั่นที่ช่วยให้การรับข้อมูลกลับมาจาก node red (การจะรับข้อมูลกลับมาได้จำเป็นต้องมีฟังชั่นดังกล่าวแต่หากไม่มีการรับข้อมูลกลับมาจาก node red ไม่จำเป็นต้องใช้ฟังชั่นดังกล่าว)
client.publish(("ledota"+String(otaNO)).c_str(),String(0).c_str());//ส่งข้อมูลไปยัง Node red เพื่อให้ led บน node red แสดงสถานะไม่พร้อมอัพโหลดโปรแกรมแบบไร้สาย
currentMills = millis();//ทำการใช้ค่าเวลาจากบอร์ด
lcd.clear();//เครียหน้าจอ LCD (ลบข้อความทั้งหมด)
lcd.setCursor(1,1);//เริ่มแสดงข้อความที่ 1 บรรทัดที่ 1
lcd.print("Collect data");//ทำการแสดงข้อความบนหน้าจอ LCD
lcd.setCursor(1,2);//เริ่มแสดงข้อความที่ 1 บรรทัดที่ 2
lcd.print("in 10 seconds");//ทำการแสดงข้อความ
  for (int i=1;i<=10;i++){//ทำการวนลูปเพื่อเก็บค่าเซ็นเซอร์ 10 ครั้งใน 10 วินาที
    reconnect();//ฟังชั่นในการตรวบสอบ mqtt
    client.loop();//เป็นฟังชั่นที่ช่วยให้การรับข้อมูลกลับมาจาก node red (การจะรับข้อมูลกลับมาได้จำเป็นต้องมีฟังชั่นดังกล่าวแต่หากไม่มีการรับข้อมูลกลับมาจาก node red ไม่จำเป็นต้องใช้ฟังชั่นดังกล่าว)
    Time=currentMills;//ทำให้ตัวแปรเท่ากับเวลาในขณะนั้นเพื่อนำไปเข้าเงื่อนไข
    while(currentMills-Time<=1000){//ลูปเงื่อนไลในการจับเวลา 1 วินาทีเพื่อใช้ในการเก็บข้อมูลในทุกๆ 1 วิ
          currentMills = millis();
    }
    Get_soil();//ทำการอ่านค่าเซ็นเซอร์ให้เป็นเปอร์เซ็น
    SumSoil1 = SumSoil1+Ssoil1;//นำค่าความชื้นในดินในแต่ละวิของ 15 cm มารวมเพื่อนำไปหาค่าเฉลี่ย
    SumSoil2 = SumSoil2+Ssoil2;//นำค่าความชื้นในดินในแต่ละวิของ 30 cm มารวมเพื่อนำไปหาค่าเฉลี่ย
    Serial.println("soil1="+String(Ssoil1));//แสดงค่าความชื้นในดินที่ 15 cm บน Serial moniter
    Serial.println("soil2="+String(Ssoil1));//แสดงค่าความชื้นในดินที่ 30 cm บน Serial moniter
    Serial.println(i);}}//แสดงจำนวนการวนรอบ

if (otaUp==1) {//เงื่อนไขในเพื่อใช้ในการอัพโหลดโปรแกรมแบบไร้สาย(1=พร้อมอัพโหลดโปรแกรมแบบไร้สาย)
    if(otabegin==0){//เงื่อนไขในการทำให้การเริ่มใช้การอัพโหลดโปรแกรมแบบไร้สายทำงานเพียงครั้งเดียว(ฟังชั่นในการอับโหลดแบบไร้สายบางฟังชั่นจำเป็นต้องทำงานครั้งเดียวแต่หากเขียนใน Setup จะไม่สามารถใช้งานได้เนื่องจากติดฟังชั่น Deep Sleep)
    ArduinoOTA.setHostname(("esp32-ota"+String(otaNO)).c_str());//ทำการตั้งชื่อ ip ของ pot อัพโหลดแบบไร้สายเพื่อที่จะสามารถเลือกอัพโหลดบอร์ดได้ถูกต้อง
    ArduinoOTA.begin();//ทำการใช้งานการอัพโหลดแบบไร้สาย
    lcd.clear();//เครียหน้าจอ LCD (ลบข้อความทั้งหมด)
    lcd.setCursor(1,1);//เริ่มแสดงข้อความที่ 1 บรรทัดที่ 1
    lcd.print("Uploading Firmware");//ทำการแสดงข้อความบนหน้าจอ LCD
    otabegin=1;}//ทำการเปลี่ยนค่าตัวแปรให้เป็น 1 เพื่อที่จะทำให้ดงื่อนไขในการใช้งานการอัพโหลดแบบไร้สายทำงานเพียงครั้งเดียว
    ArduinoOTA.handle();//ทำการเริ่มใช้งานการอัพโหลดโปรแกรมแบบไร้สาย
    client.publish(("otab"+String(otaNO)).c_str(),String(0).c_str());//ทำการส่งค่า 0 ไปยัง node red เพื่อที่จะทำการปิดการใช้งานสวิตใน Node Red
    client.publish(("ledota"+String(otaNO)).c_str(),String(1).c_str());//ส่งค่า 1 ไปยัง LED ของ Node Red เพื่อทำให้รู้ว่าสามารถอัพโหลดโปรแกรมได้
    }

if (otaUp==0) {//เงื่อนไขในเพื่อใช้ในการอัพโหลดโปรแกรมแบบไร้สาย(0=ทำงานปกติและไม่พร้อมอัพโหลดโปรแกรมแบบไร้สาย)
  soil1=SumSoil1/10;//ทำการหาค่าเฉลี่ยของค่าความชื้นในดินที่ 15 cm
  soil2=SumSoil2/10;//ทำการหาค่าเฉลี่ยของค่าความชื้นในดินที่ 30 cm
  //ทำให้ค่าเซ็นเซอร์ที่ทำการเฉลี่ยไม่ให้น้อยกว่า 0 และมากกว่า 100 ที่ 15 cm
  if(soil1<=0){soil1=0;}
  else if(soil1>=100){soil1=100;}
  //ทำให้ค่าเซ็นเซอร์ที่ทำการเฉลี่ยไม่ให้น้อยกว่า 0 และมากกว่า 100 ที่ 30 cm
  if(soil2<=0){soil2=0;}
  else if(soil2>=100){soil2=100;}

  LLCd();//ฟังชั่นในการแสดงค่าความชื้นในดินบนหน้าจอ LCD
  delay(3000);
  client.loop();//เป็นฟังชั่นที่ช่วยให้การรับข้อมูลกลับมาจาก node red (การจะรับข้อมูลกลับมาได้จำเป็นต้องมีฟังชั่นดังกล่าวแต่หากไม่มีการรับข้อมูลกลับมาจาก node red ไม่จำเป็นต้องใช้ฟังชั่นดังกล่าว)
  reconnect();//ฟังชั่นในการเชื่อมต่อกับ mqtt ใหม่
  aggg();//ทำการส่งค่าความชื้นในดินไปยัง Google Sheet
  mq();//ส่งข้อมูลต่างๆไปยัง node red
  prefs.begin("store", false);//ทำการหยุดการจดจำข้อมูล
  GSBatery();//ทำการส่งค่าแบตเตอร์รี่ไปยัง Google Sheet
  prefs.end();//ทำการสิ้นสุดการจดจำข้อมูล
  sleep();//ฟังชั่นในการเริ่มเข้าสู่โหมด Deep Sleep
}}

