#pragma once

// NOTE: RED pin of thermocouple is NEGATIVE
#define CS_PIN 7
#define SPI_CLK_PIN 13
#define SPI_MISO_PIN 12

class Thermocouple
{   
private:
    SPISettings settings = SPISettings(4000000, MSBFIRST, SPI_MODE1);
public:
    void init(void);
    float readCelsius(void);
};

void Thermocouple::init(void) {
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    SPI.begin();
}

float Thermocouple::readCelsius(void) {
    SPI.beginTransaction(settings);
    digitalWrite(CS_PIN, LOW);
    // Max6675 outputs 16 bits, MSB first; we clock out 0s while reading
    uint16_t v = SPI.transfer16(0x0000);
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();

    if (v & 0x0004) {           // D2 = OC (open thermocouple)
      return NAN;
    }
    
    v >>= 3;                    // drop [2:0] status bits
    return v * 0.25f;
}
