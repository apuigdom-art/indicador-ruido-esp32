/*
  Prueba del 74HC595: alterna todos los LEDs encendidos / apagados
  y luego enciende un LED cada vez. El LED azul de la placa (GPIO2)
  parpadea a la vez para confirmar que el programa corre.
*/
const uint8_t PIN_DATA  = 23;   // -> DS    (pin 14 del 74HC595)
const uint8_t PIN_CLOCK = 18;   // -> SH_CP (pin 11)
const uint8_t PIN_LATCH = 19;   // -> ST_CP (pin 12)
const uint8_t PIN_LED_PLACA = 2;

void escribirLeds(uint8_t patron) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, patron);
  digitalWrite(PIN_LATCH, HIGH);
  Serial.print("patron: ");
  Serial.println(patron, BIN);
}

void setup() {
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_LED_PLACA, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // 3 parpadeos de todos los LEDs
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED_PLACA, HIGH);
    escribirLeds(0b11111111);
    delay(700);
    digitalWrite(PIN_LED_PLACA, LOW);
    escribirLeds(0b00000000);
    delay(700);
  }
  // Un LED cada vez, de Q0 a Q7
  for (int i = 0; i < 8; i++) {
    escribirLeds(1 << i);
    delay(500);
  }
  escribirLeds(0);
  delay(1000);
}
