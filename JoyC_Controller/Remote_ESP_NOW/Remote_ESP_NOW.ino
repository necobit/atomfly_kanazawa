#include <M5StickCPlus.h>
#include <WiFi.h>
#include <esp_now.h>
#include "EEPROM.h"

#define EEPROM_SIZE 64

TFT_eSprite Disbuff = TFT_eSprite(&M5.Lcd);

#define SYSNUM 3

uint64_t realTime[4], time_count = 0;
bool k_ready       = false;
uint32_t key_count = 0;

uint32_t send_count  = 0;
uint8_t system_state = 0;

esp_now_peer_info_t slave;

// 送信コールバック
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void I2CWrite1Byte(uint8_t Addr, uint8_t Data) {
    Wire.beginTransmission(0x38);
    Wire.write(Addr);
    Wire.write(Data);
    Wire.endTransmission();
}

void I2CWritebuff(uint8_t Addr, uint8_t *Data, uint16_t Length) {
    Wire.beginTransmission(0x38);
    Wire.write(Addr);
    for (int i = 0; i < Length; i++) {
        Wire.write(Data[i]);
    }
    Wire.endTransmission();
}

uint8_t I2CRead8bit(uint8_t Addr) {
    Wire.beginTransmission(0x38);
    Wire.write(Addr);
    Wire.endTransmission();
    Wire.requestFrom(0x38, 1);
    return Wire.read();
}

uint16_t I2CRead16bit(uint8_t Addr) {
    uint16_t ReData = 0;
    Wire.beginTransmission(0x38);
    Wire.write(Addr);
    Wire.endTransmission();
    Wire.requestFrom(0x38, 2);
    for (int i = 0; i < 2; i++) {
        ReData <<= 8;
        ReData |= Wire.read();
    }
    return ReData;
}

bool joys_l      = false;
uint8_t color[3] = {0, 100, 0};

uint8_t SendBuff[9] = {0xAA, 0x55, SYSNUM, 0x00,0x00, 0x00, 0x00, 0x00, 0xee};

char APName[20];
String WfifAPBuff[16];
uint32_t count_bn_a = 0, choose = 0;
String ssidname;

void setup() {
    // put your setup code here, to run once:
    M5.begin();
    Wire.begin(0, 26, 100000UL);
    EEPROM.begin(EEPROM_SIZE);

    M5.Lcd.setRotation(4);
    M5.Lcd.setSwapBytes(false);
    Disbuff.createSprite(135, 240);
    Disbuff.setSwapBytes(true);

    Disbuff.fillRect(0, 0, 135, 20, Disbuff.color565(50, 50, 50));
    Disbuff.setTextSize(2);
    Disbuff.setTextColor(GREEN);
    Disbuff.setCursor(55, 6);
    // Disbuff.printf("%03d",SYSNUM);

    uint8_t res = I2CRead8bit(0x32);
    Serial.printf("Res0 = %02X \r\n", res);
    /*
    if( res & 0x02 )
    {
        I2CWrite1Byte(0x10,0x02);
    }
    else
    {
        Disbuff.setTextSize(1);
        Disbuff.setTextColor(GREEN);
        Disbuff.fillRect(0,0,80,20,Disbuff.color565(50,50,50));
        Disbuff.setCursor(5,20);
        Disbuff.printf("calibration");
        Disbuff.pushSprite(0,0);
        I2CWrite1Byte( 0x32 , 0x01 );
    }
    */
    M5.update();
      // ESP-NOW初期化
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }

  // マルチキャスト用Slave登録
  memset(&slave, 0, sizeof(slave));
  for (int i = 0; i < 6; ++i) {
    slave.peer_addr[i] = (uint8_t)0xff;
  }
  
  esp_err_t addStatus = esp_now_add_peer(&slave);
  if (addStatus == ESP_OK) {
    // Pair success
    Serial.println("Pair success");
  }
  // ESP-NOWコールバック登録
  esp_now_register_send_cb(OnDataSent);
//  esp_now_register_recv_cb(OnDataRecv);
}

uint8_t adc_value[5] = {0};
uint16_t AngleBuff[5];
uint32_t count = 0;
void loop() {
    for (int i = 0; i < 5; i++) {
        adc_value[i] = I2CRead8bit(0x60 + i);
    }

    for (int i = 0; i < 5; i++) {
        AngleBuff[i] = I2CRead16bit(0x50 + i * 2);
    }

    delay(10);

        SendBuff[3] = map(AngleBuff[0], 0, 4000, 0, 255);
        SendBuff[4] = map(AngleBuff[1], 0, 4000, 0, 255);
        SendBuff[5] = map(AngleBuff[2], 0, 4000, 0, 255);
        SendBuff[6] = map(AngleBuff[3], 0, 4000, 0, 255);
        SendBuff[7] = adc_value[4];

  esp_err_t result = esp_now_send(slave.peer_addr, SendBuff, sizeof(SendBuff));



    Disbuff.fillRect(0, 30, 80, 130, BLACK);
    Disbuff.setCursor(10, 30);
    Disbuff.printf("%04d", SendBuff[3]);
    Disbuff.setCursor(10, 45);
    Disbuff.printf("%04d", SendBuff[4]);
    Disbuff.setCursor(10, 60);
    Disbuff.printf("%04d", SendBuff[5]);
    Disbuff.setCursor(10, 75);
    Disbuff.printf("%04d", SendBuff[6]);
    Disbuff.setCursor(10, 100);
    Disbuff.printf("%04X", adc_value[4]);
    Disbuff.pushSprite(0, 0);
}
