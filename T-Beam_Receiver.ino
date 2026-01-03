// Software for TBeam AXP-2101 v1.2 TBEAM on Arduino Board
// 29_12_2025
 

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>


// ---------- LoRa pins (T-Beam V1.2 - setup tipico) ----------
static const int PIN_LORA_SCK  = 5;
static const int PIN_LORA_MISO = 19;
static const int PIN_LORA_MOSI = 27;
static const int PIN_LORA_CS   = 18;
static const int PIN_LORA_DIO0 = 26;
static const int PIN_LORA_RST  = 23;
static const int PIN_LORA_DIO1 = RADIOLIB_NC;

// ---------- Radio params ----------
static const float LORA_FREQ_MHZ = 868.1;
static const int   LORA_SF       = 9;
static const float LORA_BW_KHZ   = 125.0;
static const int   LORA_CR       = 5;
static const int   LORA_SYNCWORD = 0x34;
int Counter =0;
// ---------- SPI instance ----------
SPIClass spiLoRa(VSPI);
Module loraModule(PIN_LORA_CS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1,
                  spiLoRa, SPISettings(8000000, MSBFIRST, SPI_MODE0));
SX1276 radio(&loraModule);

// ---------- RX flag ----------
volatile bool receivedFlag = false;
void setFlag(void) { receivedFlag = true; }

static void printPayload(const uint8_t* buf, size_t len) {
  Serial.print("hex=");
  for (size_t i = 0; i < len; i++) {
    if (buf[i] < 0x10) Serial.print('0');
    Serial.print(buf[i], HEX);
  }
  Serial.print(" ascii=\"");
  for (size_t i = 0; i < len; i++) {
    char c = (char)buf[i];
    Serial.print((c >= 32 && c <= 126) ? c : '.');
  }
  Serial.print('"');
}

void setup() {
  Serial.begin(115200);
  delay(200);

  spiLoRa.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_CS);

  int state = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR); //Inizializza l'oggetto RADIO 
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("radio.begin failed, code="); Serial.println(state);
    while (true) delay(1000);
  }

  radio.setSyncWord(LORA_SYNCWORD);
  radio.setCRC(true);

  // FIX: ora servono 2 argomenti (callback + edge)
  radio.setDio0Action(setFlag, RISING);

  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("startReceive failed, code="); Serial.println(state);
    while (true) delay(1000);
  }

  Serial.println("LoRa RX ready.");


}

void loop() {
  
  
  if (!receivedFlag) return;
  if (receivedFlag) Counter =Counter +1;
  receivedFlag = false;

  int packetLen = radio.getPacketLength();
  if (packetLen < 0) packetLen = 0;

  uint8_t buf[256];
  size_t len = (size_t)packetLen;
  if (len > sizeof(buf)) len = sizeof(buf);

  int state = radio.readData(buf, len);

  Serial.print("t="); Serial.print(millis());
  Serial.print(" len="); Serial.print(len);
  Serial.print(" rssi="); Serial.print(radio.getRSSI(), 1);
  Serial.print(" snr=");  Serial.print(radio.getSNR(), 1);
  Serial.print(" fe=");   Serial.print(radio.getFrequencyError());
  Serial.print(" ");



   //Inizializzazione del display grafico------------------
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,      // rotazione (R0 = normale)
  U8X8_PIN_NONE // reset (non collegato)
  );
  Wire.begin(21, 22);   // SDA, SCL (ESP32)
  u8g2.begin();         // inizializza display
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf); 

  u8g2.drawStr(0, 12, "    LoRa RX    ");
  String s = "RSSI: " + String(radio.getRSSI(), 1);
  u8g2.drawStr(0, 26, s.c_str());
  s = "SNR: " + String(radio.getSNR(), 1);
  u8g2.drawStr(0, 40, s.c_str());
  s = "Counter: " + String(Counter);
  u8g2.drawStr(0, 54, s.c_str());
  u8g2.sendBuffer();
  //Fine gestione del display---------------------


  if (state == RADIOLIB_ERR_NONE) {
    printPayload(buf, len);
    Serial.println();
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("CRC_MISMATCH");
  } else {
    Serial.print("readData failed, code="); Serial.println(state);
  }

  radio.startReceive();
}
