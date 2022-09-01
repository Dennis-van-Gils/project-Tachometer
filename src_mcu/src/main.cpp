/*******************************************************************************
  Tachometer

  A stand-alone Arduino tachometer using a transmissive photointerrupter with a
  slotted disk. The RPMs are displayed on an OLED screen.

  Hardware:
  - Adafruit Feather M4 Express (Adafruit #3857)
  - Adafruit FeatherWing OLED - 128x32 OLED Add-on For Feather (Adafruit #2900)
  - OMRON EE-SX1041 Transmissive Photomicrosensor
    Emitter on pin 10

  https://github.com/Dennis-van-Gils/project-Tachometer
  Dennis van Gils
  01-09-2022
*******************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_BUTTON_A 9
#define PIN_BUTTON_B 6
#define PIN_BUTTON_C 5
#define PIN_TACHO 10

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

void setup() {
  Serial.begin(9600);

  // OLED buttons
  pinMode(PIN_BUTTON_A, INPUT_PULLUP);
  pinMode(PIN_BUTTON_B, INPUT_PULLUP);
  pinMode(PIN_BUTTON_C, INPUT_PULLUP);

  // Tacho input
  pinMode(PIN_TACHO, INPUT_PULLDOWN);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.display(); // Show Adafruit splashscreen from buffer
  delay(1000);

  display.clearDisplay(); // Clear the buffer
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();
}

void loop() {
  bool fLight;

  fLight = digitalRead(PIN_TACHO);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(fLight);

  if (!digitalRead(PIN_BUTTON_A)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("A");
  }
  if (!digitalRead(PIN_BUTTON_B)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("B");
  }
  if (!digitalRead(PIN_BUTTON_C)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("C");
  }
  delay(10);
  yield();
  display.display();
}