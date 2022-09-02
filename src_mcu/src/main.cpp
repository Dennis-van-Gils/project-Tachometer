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
#include <avdweb_Switch.h>

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
const uint32_t T_display = 500; // Display refresh rate [ms]

/*------------------------------------------------------------------------------
  Frequency detector
------------------------------------------------------------------------------*/

const uint16_t N_AVG = 10;     // Number of light up-flanks to average over
const uint16_t TIMEOUT = 2000; // [ms] Timeout to stop detecting the frequency

volatile bool isr_done = false;
volatile uint8_t isr_counter = 0;
volatile uint32_t T_upflanks = 0; // [us] Measured duration for N_AVG up-flanks

double frequency = 0; // [Hz] Measured average frequency of light up-flanks
double RPM = 0;       // [RPM] Measured RPM

void isr_rising() {
  /* Interrupt service routine for when an up-flank is detected on the input pin
   */
  static uint32_t micros_start = 0;

  if (!isr_done) {
    if (isr_counter == 0) {
      micros_start = micros();
    }
    isr_counter++;
    if (isr_counter > N_AVG) {
      T_upflanks = micros() - micros_start;
      isr_done = true;
    }
  }
}

bool measure_frequency() {
  uint32_t t_0 = millis();
  isr_counter = 0;
  isr_done = false;
  while ((!isr_done) && ((millis() - t_0) < TIMEOUT)) {}

  if (isr_done) {
    frequency = 1000000. * N_AVG / T_upflanks;
    RPM = frequency / N_SLITS_ON_DISK * 60.;
  } else {
    frequency = 0;
    RPM = 0;
  }

  return isr_done;
}

/*------------------------------------------------------------------------------
  setup
------------------------------------------------------------------------------*/

void setup() {
  //Serial.begin(9600);

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
  static bool alive_blinker = true;

  // Read the buttons
  /*
  button_A.poll();
  button_B.poll();
  button_C.poll();
  if (button_A.pushed()) {}
  if (button_B.pushed()) {}
  if (button_C.pushed()) {}
  */

  // Refresh screen
  if (now - tick >= T_display) {
    tick = now;
    display.clearDisplay();

    // Draw RPM value
    display.setCursor(0, 0);
    display.setTextSize(3);
    if (measure_frequency()) {
      display.print(RPM, 2);
    }

    // Draw "RPM"
    display.setTextSize(1);
    // display.setCursor(110, 25); // Lower-right
    display.setCursor(110, 0); // Upper-right
    display.print("RPM");

    // Draw alive blinker
    alive_blinker = !alive_blinker;
    if (alive_blinker) {
      display.fillRect(0, 26, 6, 6, SSD1306_WHITE);
    }

    display.display();
  }

  delay(10);
}