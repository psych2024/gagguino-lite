#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include "lcd/lcd.h"
#include "PID_v1.h"
#include "thermocouple/thermocouple.h"

#define RELAY_OUTPUT_PIN 6

// PID Constants
#define HEATER_KP 3.5
#define HEATER_KI 0.2
#define HEATER_KD 30.0

// Timing Constants
#define PID_PERIOD_MS 500 // 2Hz PWM frequency
#define REPORTING_PERIOD_MS 1000

// Variables
double temp_setpoint, temp_reading, pwm_output;
unsigned long pwmPreviousMillis = 0;
unsigned long tempPreviousMillis = 0;
Thermocouple thermocouple;

unsigned long relayOnSince = 0;
bool relayIsOn = false;
unsigned long heaterOnTime = 0;

enum RoastLevel
{
  DARK,
  MEDIUM,
  LIGHT,
  GENERAL
};
RoastLevel level = GENERAL;
GagguinoMode mode = BREW;

// PID Controller
PID pid(&temp_reading, &pwm_output, &temp_setpoint, HEATER_KP, HEATER_KI, HEATER_KD, P_ON_E, DIRECT);

float get_temp_setpoint_for_roast(RoastLevel level)
{
  switch (level)
  {
  case DARK:
    return 90.0f;
  case MEDIUM:
    return 93.0f;
  case LIGHT:
    return 96.0f;
  default:
    return 94.0f;
  }
}

LCD lcd;

void setup()
{
  Serial.begin(9600);

  lcd.init();
  thermocouple.init();

  // Configure relay output
  pinMode(RELAY_OUTPUT_PIN, OUTPUT);
  digitalWrite(RELAY_OUTPUT_PIN, LOW); // Ensure relay starts OFF

  // Initialize PID
  temp_setpoint = get_temp_setpoint_for_roast(GENERAL);
  pid.SetOutputLimits(0, 255);
  pid.SetSampleTime(PID_PERIOD_MS);
  pid.SetMode(AUTOMATIC);

  Serial.println("Setup Complete");
}

void loop()
{
  unsigned long currentMillis = millis();


  // // Compute PID output at regular intervals
  if (currentMillis - pwmPreviousMillis >= PID_PERIOD_MS)
  {
    pwmPreviousMillis = currentMillis;

    pid.Compute();

    // Calculate how long to keep the relay ON (burst fire)
    heaterOnTime = static_cast<unsigned long>((pwm_output / 255.0f) * PID_PERIOD_MS);

    if (heaterOnTime > 0)
    {
      digitalWrite(RELAY_OUTPUT_PIN, HIGH);
      relayOnSince = currentMillis;
      relayIsOn = true;
    }
    else
    {
      digitalWrite(RELAY_OUTPUT_PIN, LOW);
      relayIsOn = false;
    }
  }

  // If the relay is ON and it's been on long enough, turn it OFF
  if (relayIsOn && (currentMillis - relayOnSince >= heaterOnTime))
  {
    digitalWrite(RELAY_OUTPUT_PIN, LOW);
    relayIsOn = false;
  }

  // Periodic reporting
  if (currentMillis - tempPreviousMillis >= REPORTING_PERIOD_MS)
  {
    tempPreviousMillis = currentMillis;

    temp_reading = thermocouple.readCelsius();
    if (isnan(temp_reading)) {
      Serial.println("Error temp reading!");
    } else {
      Serial.print("Temp: ");
      Serial.print(temp_reading);
      Serial.print("°C | Setpoint: ");
      Serial.print(temp_setpoint);
      Serial.print("°C | PID Output: ");
      Serial.println(pwm_output);
    }
    
    GagguinoMode curr_mode = lcd.get_gagguino_mode();
    if (curr_mode != mode) {
      if (curr_mode == BREW) {
        temp_setpoint = 93.f;
      } else {
        temp_setpoint = 123.f;
      }
    }

    // lcd.plot_temperature_reading(temp_reading, (pwm_output / 255.0f));
  }

  // lcd.poll_touchscreen();
}