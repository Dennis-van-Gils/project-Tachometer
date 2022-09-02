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
  02-09-2022
*******************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "DvG_StreamCommand.h"
#include "avdweb_Switch.h"

// Tacho settings
const uint8_t PIN_TACHO = 10;
const uint8_t N_SLITS_ON_DISK = 25;

// OLED display
const uint8_t PIN_BUTTON_A = 9;
const uint8_t PIN_BUTTON_B = 6;
const uint8_t PIN_BUTTON_C = 5;
Switch button_A = Switch(PIN_BUTTON_A, INPUT_PULLUP);
Switch button_B = Switch(PIN_BUTTON_B, INPUT_PULLUP);
Switch button_C = Switch(PIN_BUTTON_C, INPUT_PULLUP);

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
const uint16_t T_DISPLAY = 500;       // [ms] Display refresh rate
const uint16_t T_SCREENSAVER = 20000; // [ms] Turn display off when at 0 RPM

// Instantiate serial port listener for receiving ASCII commands
const uint8_t CMD_BUF_LEN = 16;  // Length of the ASCII command buffer
char cmd_buf[CMD_BUF_LEN]{'\0'}; // The ASCII command buffer
DvG_StreamCommand sc(Serial, cmd_buf, CMD_BUF_LEN);

/*------------------------------------------------------------------------------
  Frequency detector
------------------------------------------------------------------------------*/

const uint16_t N_UPFLANKS = 25;    // Number of up-flanks to average over
const uint16_t ISR_TIMEOUT = 4000; // [ms] Timeout to stop waiting for the ISR

volatile bool isr_done = false;
volatile uint8_t isr_counter = 0;
volatile uint32_t T_upflanks = 0; // [us] Measured duration for N_UPFLANKS

double freq = NAN; // [Hz]  Measured frequency
double RPM = NAN;  // [RPM] Measured RPM

void isr_rising() {
  // Interrupt service routine for when an up-flank is detected on the input pin
  static uint32_t micros_start = 0;

  if (!isr_done) {
    if (isr_counter == 0) {
      micros_start = micros();
    }
    isr_counter++;
    if (isr_counter > N_UPFLANKS) {
      T_upflanks = micros() - micros_start;
      isr_done = true;
    }
  }
}

/*------------------------------------------------------------------------------
  setup
------------------------------------------------------------------------------*/

void setup() {
  Serial.begin(9600);

  // Tacho input
  pinMode(PIN_TACHO, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PIN_TACHO), isr_rising, RISING);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.clearDisplay();                    // Clear the buffer
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();
}

/*------------------------------------------------------------------------------
  loop
------------------------------------------------------------------------------*/

void loop() {
  uint32_t now = millis();
  static uint32_t tick = now;
  static uint32_t tick_isr = now;
  static bool alive_blinker = true;
  static bool update_anim = false;
  static uint8_t anim = 0;

  // Listen for commands on the serial port
  if (sc.available()) {
    char *str_cmd = sc.getCommand();

    if (strcmp(str_cmd, "id?") == 0) {
      Serial.println("Arduino, Tachometer");
    } else {
      Serial.println(RPM);
    }
  }

  // Read the buttons
  /*
  button_A.poll();
  button_B.poll();
  button_C.poll();
  if (button_A.pushed()) {}
  if (button_B.pushed()) {}
  if (button_C.pushed()) {}
  */

  if (isr_done) {
    freq = 1000000. / T_upflanks * N_UPFLANKS;
    RPM = freq / N_SLITS_ON_DISK * 60.;
    update_anim = true;
    isr_counter = 0;
    isr_done = false;
    tick_isr = now;
  }

  if (now - tick_isr > ISR_TIMEOUT) {
    freq = NAN;
    RPM = NAN;
  }

  // Refresh display
  if (now - tick_isr > T_SCREENSAVER) {
    // Screensaver engaged
    display.clearDisplay();
    display.display();
    delay(100);

  } else {
    if (now - tick >= T_DISPLAY) {
      tick = now;
      display.clearDisplay();

      // Draw RPM value
      display.setCursor(0, 0);
      display.setTextSize(3);
      if (!isnan(RPM)) {
        display.print(RPM, 2);
      } else {
        display.print("<");
        display.print(
            1. / (ISR_TIMEOUT / 1000. / N_UPFLANKS) / N_SLITS_ON_DISK * 60, 1);
      }

      // Draw "RPM"
      display.setTextSize(1);
      display.setCursor(110, 0); // Upper-right
      display.print("RPM");

      // Draw alive blinker
      alive_blinker = !alive_blinker;
      if (alive_blinker) {
        display.fillRect(0, 26, 6, 6, SSD1306_WHITE);
      }

      // Draw new readout animation
      display.setTextSize(2);
      display.setCursor(112, 19);
      if (update_anim) {
        update_anim = false;
        anim = ((anim + 1) % 4);
      }
      if (anim == 0) {
        display.print("|");
      } else if (anim == 1) {
        display.print("/");
      } else if (anim == 2) {
        display.print("-");
      } else if (anim == 3) {
        display.print("\\");
      }

      display.display();
    }
  }
}