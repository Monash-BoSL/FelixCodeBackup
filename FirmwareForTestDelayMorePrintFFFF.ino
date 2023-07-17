#include <Wire.h>

void getData() {
  uint32_t val[6];
  uint32_t pressure, temperature;
  float pres, temp, dept, ec;

    Wire.begin();
    Wire.beginTransmission(0x78);
    Wire.write(0xAC);
    byte error = Wire.endTransmission();
    delay(600);
    byte len = Wire.requestFrom(0x78, 6);
    Serial.print("len:"); Serial.print(len); Serial.println();
    for (int i = 0; i < 6; i++) {
      val[i] = Wire.read();
      Serial.print(val[i], HEX);
      Serial.print(" ");
    }
		Serial.println();
    Wire.end();

  pressure = val[1] << 16 | val[2] << 8 | val[3];
  pres = float(pressure) / 16777216;
  pres = ((pres-0.15)/0.7)*35*10; //convert to the actual mbar
  
  temperature = val[4] << 8 | val[5];
  temp = ((long)val[4] << 8) + (val[5]);
  temp = temp/65536 * (150+40) - 40;
  
  String str = String(pres) + "mbar, " + String(temp) + "C";
  Serial.println(str);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.setTimeout(3000);
}

void loop() {
  getData();
  delay(2000);
}
