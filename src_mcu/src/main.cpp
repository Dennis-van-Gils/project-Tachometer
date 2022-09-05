/*******************************************************************************
  Tachometer

  A stand-alone Arduino tachometer using a transmissive photointerrupter with an
  optical encoder disk.

  The rotation rate is displayed on an OLED screen. The units can be switched
  between RPM, rev/s and rad/s by pressing one of the OLED screen buttons. The
  display will go blank when no rotation has been detected after a certain
  timeout period.

  Hardware:
  - Adafruit Feather M4 Express (Adafruit #3857)
  - Adafruit FeatherWing OLED - 128x32 OLED Add-on For Feather (Adafruit #2900)
  - OMRON EE-SX1041 Transmissive Photomicrosensor
    Emitter on pin 10

  https://github.com/Dennis-van-Gils/project-Tachometer
  Dennis van Gils
  05-09-2022
*******************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "DvG_StreamCommand.h"
#include "avdweb_Switch.h"

// Tacho settings
enum class TACHO_UNIT {
  RPM,   // rounds per minute
  REVPS, // revolutions per second
  RADPS, // rad per second
  EOL    // end-of-list
};

const uint8_t PIN_TACHO = 10;
const uint8_t N_SLITS_ON_DISK = 25; // Optical encoder disk
TACHO_UNIT unit = TACHO_UNIT::RPM;

// An interrupt service routine (ISR) will execute once an up-flank on the
// digital input of pin PIN_TACHO is detected. A single up-flank corresponds to
// light hitting the photodiode after having been dark.
const uint16_t N_UPFLANKS = 25;    // Number of up-flanks to average over
const uint16_t ISR_TIMEOUT = 4000; // [ms] Timeout to stop waiting for the ISR

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

// Minimum detectable rotation rates
const double MIN_REVPS = 1000. * N_UPFLANKS / ISR_TIMEOUT / N_SLITS_ON_DISK;
const double MIN_RPM = MIN_REVPS * 60;
const double MIN_RADPS = MIN_REVPS * TWO_PI;

volatile bool isr_done = false;
volatile uint8_t isr_counter = 0;
volatile uint32_t T_upflanks = 0; // [us] Measured duration for N_UPFLANKS
double freq_upflanks = NAN;       // [Hz] Measured up-flank frequency

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
  display.setTextSize(1);
  display.print("GITHUB.COM/");
  display.setCursor(0, 12);
  display.print("DENNIS-VAN-GILS/");
  display.setCursor(0, 24);
  display.print("PROJECT-TACHOMETER");
  display.display();
  delay(4000);
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

  if (isr_done) {
    freq_upflanks = 1000000. / T_upflanks * N_UPFLANKS;
    update_anim = true;
    isr_counter = 0;
    isr_done = false;
    tick_isr = now;
  }

  if (now - tick_isr > ISR_TIMEOUT) {
    freq_upflanks = NAN;
  }

  double tacho_revps = freq_upflanks / N_SLITS_ON_DISK;
  double tacho_rpm = tacho_revps * 60.;
  double tacho_radps = tacho_revps * TWO_PI;

  // Listen for commands on the serial port
  if (sc.available()) {
    char *str_cmd = sc.getCommand();

    if (strcmp(str_cmd, "id?") == 0) {
      // Reply identity string
      Serial.println("Arduino, Tachometer v1.0");

    } else if (strncmp(str_cmd, "u", 1) == 0) {
      // Change unit
      uint8_t new_unit = parseIntInString(str_cmd, 1);
      if (new_unit == int(TACHO_UNIT::REVPS)) {
        unit = TACHO_UNIT::REVPS;
      } else if (new_unit == int(TACHO_UNIT::RADPS)) {
        unit = TACHO_UNIT::RADPS;
      } else {
        unit = TACHO_UNIT::RPM;
      }

    } else {
      // Report rotation rate
      if (unit == TACHO_UNIT::RPM) {
        Serial.print(tacho_rpm, tacho_rpm < 100 ? 2 : 1);
        Serial.println(" rpm");

      } else if (unit == TACHO_UNIT::REVPS) {
        Serial.print(tacho_revps, tacho_revps < 10 ? 3 : 2);
        Serial.println(" rev/s");

      } else if (unit == TACHO_UNIT::RADPS) {
        Serial.print(tacho_radps, tacho_radps < 10 ? 3 : 2);
        Serial.println(" rad/s");
      }
    }
  }

  // Read the buttons
  button_A.poll();
  button_B.poll();
  button_C.poll();
  if (button_A.pushed() || button_B.pushed() || button_C.pushed()) {
    unit = static_cast<TACHO_UNIT>((int(unit) + 1) % int(TACHO_UNIT::EOL));
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

      // Draw rotation rate value
      display.setCursor(0, 0);
      display.setTextSize(3);

      if (unit == TACHO_UNIT::RPM) {
        if (!isnan(freq_upflanks)) {
          display.print(tacho_rpm, tacho_rpm < 100 ? 2 : 1);
        } else {
          display.print("<");
          display.print(MIN_RPM);
        }
        display.setTextSize(1);
        display.setCursor(110, 0);
        display.print("RPM");

      } else if (unit == TACHO_UNIT::REVPS) {
        if (!isnan(freq_upflanks)) {
          display.print(tacho_revps, tacho_revps < 10 ? 3 : 2);
        } else {
          display.print("<");
          display.print(MIN_REVPS);
        }
        display.setTextSize(1);
        display.setCursor(110, 0);
        display.print("REV");
        display.setCursor(110, 8);
        display.print("/S");

      } else if (unit == TACHO_UNIT::RADPS) {
        if (!isnan(freq_upflanks)) {
          display.print(tacho_radps, tacho_radps < 10 ? 3 : 2);
        } else {
          display.print("<");
          display.print(MIN_RADPS);
        }
        display.setTextSize(1);
        display.setCursor(110, 0);
        display.print("RAD");
        display.setCursor(110, 8);
        display.print("/S");
      }

      // Draw alive blinker
      alive_blinker = !alive_blinker;
      if (alive_blinker) {
        display.fillRect(0, 30, 2, 2, SSD1306_WHITE);
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