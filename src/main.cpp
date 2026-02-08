#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>
#include "lcd/lcd.h"
#include "PID_v1.h"
#include "globals.h"
#include "thermocouple/thermocouple.h"

#define RELAY_OUTPUT_PIN 6
#define RELAY_LOW LOW
#define RELAY_HIGH HIGH

GagguinoMode MODE = INVALID;

// Variable
double temp_reading = -1;
double temp_setpoint, pwm_output;
unsigned long relayPrevMills = 0;
unsigned long reportingPrevMills = 0;
unsigned long pidPrevMills = 0;
unsigned long pwmPrevMills = 0;
Thermocouple thermocouple;

unsigned long relayOnTime = 0;
bool relayOn = false;

enum RoastLevel
{
  DARK,
  MEDIUM,
  LIGHT,
  GENERAL
};
RoastLevel level = GENERAL;

// PID Controller
PID pid(&temp_reading, &pwm_output, &temp_setpoint, 0, 0, 0, P_ON_E, DIRECT);

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

void setGlobalMode(GagguinoMode new_mode) {
  if (MODE == new_mode) return;

  if (new_mode == INVALID) {
    lcd.clear_screen();
    
    pid.SetMode(MANUAL);
    pwm_output = 0;

  } else if (new_mode == WARMUP) {
    lcd.draw_loading_screen();
    lcd.update_warmup_status(0);

    pid.SetMode(MANUAL);

  } else if (new_mode == BREW) {
    lcd.plot_temp_graph(BREW);
    temp_setpoint = get_temp_setpoint_for_roast(GENERAL);

    pid.SetMode(MANUAL);
    pwm_output = 0;
    pid.SetTunings(BREW_KP, BREW_KI, BREW_KD);

  } else if (new_mode == STEAM) {
    lcd.plot_temp_graph(STEAM);
    temp_setpoint = STEAM_TEMP_SETPOINT;

    pid.SetMode(MANUAL);
    pwm_output = 0;
    pid.SetTunings(STEAM_KP, STEAM_KI, STEAM_KD);
  }
  
  MODE = new_mode;
}

void update_temperature(void) {
  double new_reading = thermocouple.readCelsius();
  if (isnan(new_reading)) {
    setGlobalMode(INVALID);
    return;
  }

  if (MODE == INVALID) {
    setGlobalMode(WARMUP);
  }

  if (temp_reading < 0) {
    temp_reading = new_reading;
    return;
  }

  temp_reading = EMA_ALPHA * new_reading + (1.0 - EMA_ALPHA) * temp_reading;
}

void setup()
{
  Serial.begin(115200);

  thermocouple.init();
  lcd.init();

  // Configure relay output
  pinMode(RELAY_OUTPUT_PIN, OUTPUT);
  digitalWrite(RELAY_OUTPUT_PIN, RELAY_LOW); // Ensure relay starts OFF

  // Initialize PID
  pid.SetOutputLimits(0, PWM_WINDOW_MS);
  pid.SetSampleTime(PID_PERIOD_MS);

  setGlobalMode(WARMUP);

  Serial.println("Setup Complete");
}

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - pidPrevMills >= PID_PERIOD_MS) {
    pidPrevMills = currentMillis;
    update_temperature();
    
    if (temp_reading >= WARMUP_THRESHOLD_CELCIUS && MODE == WARMUP) {
      setGlobalMode(BREW);
    }
    pid.Compute();
  }


  if (currentMillis - pwmPrevMills >= PWM_WINDOW_MS) {
    pwmPrevMills = currentMillis;

    if (MODE == INVALID) {
      relayOnTime = 0;
    } else if (MODE == WARMUP) {
      relayOnTime = PWM_WINDOW_MS;
    } else {
      // add 0.3 as hysteresis
      // if (temp_setpoint - temp_reading >= PID_BLEND_DELTA_LO + 0.3) {
      //   pid.SetMode(MANUAL);
      // }

      if (temp_setpoint - temp_reading < PID_BLEND_DELTA_LO) {
        pid.SetMode(AUTOMATIC);
      }

      if (temp_reading >= temp_setpoint - PID_BLEND_DELTA_HI) {
          relayOnTime = static_cast<unsigned long>(pwm_output);
      } else {
        float alpha = ((temp_setpoint - PID_BLEND_DELTA_HI) - temp_reading) / float(PID_BLEND_DELTA_LO - PID_BLEND_DELTA_HI);
        alpha = min(alpha, 1.0);
        alpha = max(0, alpha);

        relayOnTime = alpha * PWM_WINDOW_MS + (1.0f - alpha) * pwm_output;
      }
    }

    relayPrevMills = currentMillis;
    if (relayOnTime > 0) {
      digitalWrite(RELAY_OUTPUT_PIN, RELAY_HIGH);
      relayOn = true;
    } else {
      digitalWrite(RELAY_OUTPUT_PIN, RELAY_LOW);
      relayOn = false;
    }
  }

  // If the relay is ON and it's been on long enough, turn it OFF
  if (relayOn && (currentMillis - relayPrevMills >= relayOnTime)) {
    digitalWrite(RELAY_OUTPUT_PIN, RELAY_LOW);
    relayOn = false;
  }

  // Periodic reporting
  if (currentMillis - reportingPrevMills >= REPORTING_PERIOD_MS) {
    reportingPrevMills = currentMillis;
    
    if (temp_reading == -1) {
      Serial.println(F("Error temp reading!"));
    } else {
      Serial.print(temp_reading);
      Serial.print(" ");
      Serial.println(float(relayOnTime / PWM_WINDOW_MS) * 100);
    }
    
    if (lcd.check_toggled_mode()) {
      Serial.println(F("MODE CHANGE"));
      lcd.display_mode(MODE, ILI9341_BLACK);
      if (MODE == BREW)
          setGlobalMode(STEAM);
      else
          setGlobalMode(BREW);
      lcd.display_mode(MODE, ILI9341_WHITE);
    }

    if (MODE == INVALID) {
      lcd.clear_screen();
    } else if (MODE == WARMUP) {
      float status = (temp_reading / WARMUP_THRESHOLD_CELCIUS) * 100;
      lcd.update_warmup_status(static_cast<uint8_t>(status));
    } else {
      lcd.plot_temperature_reading(MODE, temp_reading, static_cast<uint16_t>((relayOnTime / PWM_WINDOW_MS) * 100));
    }
  }

  if (MODE > WARMUP) lcd.poll_touchscreen();
}