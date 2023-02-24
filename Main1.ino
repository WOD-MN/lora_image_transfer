#include <SPI.h>
#include <LoRa.h>
#include <SD.h>
#include "JPEGDecoder.h"

#define SCK 5    // define the pins used by the SD card module and LoRa module
#define MISO 19
#define MOSI 27
#define SS_SD 18
#define SS_LoRa 14
#define RST_LoRa 23
#define DI0 26

File myFile;
unsigned char jpg_buf[JPEG_FILE_BUFFER_SIZE];
unsigned long jpg_len = 0;
bool jpg_ready = false;
unsigned long last_transmission_time = 0;
const unsigned long TRANSMISSION_DELAY = 120000; // 2 minute delay in milliseconds

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // initialize SD card
  pinMode(SS_SD, OUTPUT);
  digitalWrite(SS_SD, HIGH);
  SPI.begin(SCK, MISO, MOSI, SS_SD);
  if (!SD.begin(SS_SD)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized successfully.");

  // initialize LoRa module
  pinMode(SS_LoRa, OUTPUT);
  digitalWrite(SS_LoRa, HIGH);
  pinMode(RST_LoRa, OUTPUT);
  digitalWrite(RST_LoRa, LOW);
  delay(10);
  digitalWrite(RST_LoRa, HIGH);
  LoRa.setPins(SS_LoRa, RST_LoRa, DI0);
  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa initialization failed!");
    return;
  }
  Serial.println("LoRa initialized successfully.");

  // read the image file from the SD card
  myFile = SD.open("/test.jpg");
  if (!myFile) {
    Serial.println("Failed to open file");
    return;
  }
  Serial.println("File opened successfully.");
  jpg_len = myFile.size();
  Serial.print("Image size: ");
  Serial.println(jpg_len);

  // decode the image and save it to jpg_buf
  JPEGDecoder decoder;
  decoder.setPurgeOnLoad(true);
  decoder.setBuffer(jpg_buf, JPEG_FILE_BUFFER_SIZE);
  if (decoder.decode(&myFile)) {
    jpg_ready = true;
    Serial.println("Image decoded successfully.");
  } else {
    Serial.println("Image decoding failed!");
    return;
  }
  myFile.close();

  // set up LoRa transmission parameters
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125000);
}

void loop() {
  unsigned long current_time = millis();
  if (jpg_ready && current_time - last_transmission_time >= TRANSMISSION_DELAY) {
    // transmit the image data over LoRa
    int transmission_attempt = 1;
    while (transmission_attempt <= 3) { // try up to 3 times
      LoRa.beginPacket();
      LoRa.write(jpg_buf, jpg_len);
      if (LoRa.endPacket()) {
        Serial.print("Image transmitted successfully on attempt ");
        Serial.println(transmission_attempt);
        last_transmission_time = current_time; // set the last transmission time to the current time
        break; // break out of the retry loop if successful
      } else {
        Serial.print("Image transmission failed on attempt ");
        Serial.println(transmission_attempt);
        transmission_attempt++;
        delay(500); // wait for a short time before retrying
      }
    }
    if (transmission_attempt > 3) {
      Serial.println("Image transmission failed after 3 attempts. Please check the LoRa connection.");
    }
  }
}
