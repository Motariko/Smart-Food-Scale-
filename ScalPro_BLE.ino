#include <EEPROM.h>
#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "driver/rtc_io.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// HX711 circuit wiring
#define HX711_DOUT_PIN 16
#define HX711_SCK_PIN  4
#define BUTTON_PINSend 13
#define BUTTON_PINTare 18
#define BUTTON_PINSleep 32
//SDA 21
//SCL 22

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

unsigned long lastLCDUpdate = 0;
bool lastSendState = HIGH;
bool lastTareState = HIGH;
bool lastSleepSate = HIGH;
long offset = 0;
float scale = 0;
float Whight_new = 0;
float Whight_current = 0;
float Whight_previous = 0;
float Alpha = 0.2;
float diff = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

#define CHECK_CONFIG_ADDRESS  10
#define OFFSET_ADDRESS        20
#define SCALE_ADDRESS         30

HX711 hx711;

String wait_enter() {
  Serial.flush();
  while(Serial.available() == 0) delay(10);
  String str = Serial.readString();
  Serial.print(str);
  return str;
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100);
  EEPROM.begin(512);
  pinMode(BUTTON_PINSend, INPUT_PULLUP);
  pinMode(BUTTON_PINTare, INPUT_PULLUP);
  pinMode(BUTTON_PINSleep, INPUT_PULLUP);


  lcd.init();
  lcd.backlight ();

  hx711.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  hx711.tare();

  BLEDevice::init("ScalPro BLE");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEDevice::getAdvertising()->start();
  
  if (EEPROM.read(CHECK_CONFIG_ADDRESS) == 0xF5) {
    EEPROM.get(OFFSET_ADDRESS, offset);
    EEPROM.get(SCALE_ADDRESS, scale);
  }
  
  hx711.set_offset(offset);
  hx711.set_scale(scale);
}

void loop() {
  // แสดงค่าน้ำหนักปัจจุบัน (ตัดค่าลบออกด้วย max)
  Whight_current = hx711.get_units(3);
  diff = abs(Whight_current -Whight_previous);
  if(diff > 2){
    Alpha = 0.8;
  }else{
    Alpha = 0.2;
  }
  Whight_new = (Alpha * Whight_current) + ((1-Alpha) * Whight_previous);

  Serial.println(max(Whight_new, 0.0f),1);

  bool currenSendState = digitalRead(BUTTON_PINSend);
  bool currenTareState = digitalRead(BUTTON_PINTare);
  bool currenSleepState = digitalRead(BUTTON_PINSleep);

  if(lastSleepSate == HIGH && currenSleepState == LOW){
    unsigned long debuonceTime = millis();
    while(digitalRead(BUTTON_PINSleep) ==LOW){
      if(millis() - debuonceTime > 3000 ){
        break;
      }
      delay(10);
    }

    rtc_gpio_pullup_en((gpio_num_t)BUTTON_PINSleep);
    rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PINSleep);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PINSleep,0);

    delay(100);
    
    lcd.clear();
    lcd.print("Good Bye...");
    delay(1000);
    lcd.noBacklight();
    lcd.noDisplay();
    esp_deep_sleep_start();
  }
  lastSleepSate = currenSleepState;

  if(Whight_new < -1.0){
      hx711.tare(10);
    }

  if(deviceConnected && lastSendState == HIGH && currenSendState == LOW){
    char txString[10];
    dtostrf(max(round(Whight_new), 0.0f), 1, 0, txString); // แปลงเลขเป็นข้อความ
    pCharacteristic->setValue(txString);
    pCharacteristic->notify(); // ส่งไป iPad เลยเหมี่ยว! 🚀
  }
    lastSendState = currenSendState;

  if(lastTareState == HIGH && currenTareState == LOW){
    hx711.tare(10);
    Serial.println("เซต0เรียบร้อย");
  }
  lastTareState = currenTareState;

  if(millis() - lastLCDUpdate >= 200){
    lcd.setCursor(0, 0);
    lcd.print(max(round(Whight_new), 0.0f));
    lcd.print("      ");
    lcd.setCursor(8, 0);
    lcd.print("g   ");

    lastLCDUpdate = millis();
  }
  
  
  Whight_previous = Whight_new;
 
  if (Serial.available()) {
    char c = Serial.read();
    
    // กด '1' เพื่อเข้าสู่โหมด Calibrate
    if (c == '1') {
      Serial.println("ยกทุกอย่างบนเครื่องชั่งออก แล้วส่งค่าใด ๆ เพื่อทำงานต่อ");
      wait_enter();
      
      Serial.println("กำลังคำนวณค่า...");
      offset = hx711.read_average(10);
      Serial.print("ขณะไม่มีสิ่งของวาง ค่าที่อ่านได้ คือ ");
      Serial.println(offset);
      
      Serial.println("วางสิ่งของที่ทราบน้ำหนัก แล้วกรอกตัวเลขน้ำหนักของสิ่งของนั้น");
      String str = wait_enter();
      float weight = str.toFloat();
      
      Serial.println("กำลังคำนวณค่า...");
      long weight_value = hx711.read_average(10);
      
      Serial.print("สิ่งของน้ำหนัก ");
      Serial.print(weight);
      Serial.println(" กรัม");
      Serial.print("ค่าที่อ่านได้ คือ ");
      Serial.println(weight_value);
      Serial.println("ทำงานเสร็จสิ้น");
      
      // สูตรคำนวณหาค่า Scale
      scale = ((float)weight_value - (float)offset) / (float)weight;
      
      // บันทึกค่าลง EEPROM
      EEPROM.put(OFFSET_ADDRESS, offset);
      EEPROM.put(SCALE_ADDRESS, scale);
      EEPROM.write(CHECK_CONFIG_ADDRESS, 0xF5);

      EEPROM.commit();
      
      hx711.set_offset(offset);
      hx711.set_scale(scale);
      
      delay(3000);
    } 
    // กด '0' เพื่อ Reset ค่าที่บันทึกไว้ทั้งหมด
    else if (c == '0') {
      for (int i = 0 ; i < EEPROM.length() ; i++) { 
        EEPROM.write(i, 0);
      }
      offset = 0;
      scale = 0;
      hx711.set_offset(offset);
      hx711.set_scale(scale);
      Serial.println("EEPROM Reset Complete!");
    }
  }
}