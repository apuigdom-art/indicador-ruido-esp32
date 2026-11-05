// Diagnóstico: mide el nivel del micrófono con distintos estados de los LEDs
// fase 0: LEDs apagados | fase 1: 8 LEDs encendidos fijos
// fase 2: 4 LEDs encendidos fijos | fase 3: LEDs conmutando cada 50 ms
const uint8_t PIN_MIC   = 4;
const uint8_t PIN_DATA  = 23;
const uint8_t PIN_CLOCK = 18;
const uint8_t PIN_LATCH = 19;

void escribirLeds(uint8_t patron) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, patron);
  digitalWrite(PIN_LATCH, HIGH);
}

float medirNivelDb() {
  uint32_t suma = 0;
  uint64_t sumaCuadrados = 0;
  uint32_t t = micros();
  for (uint16_t i = 0; i < 500; i++) {
    while (micros() - t < 100) {}
    t += 100;
    uint32_t x = analogRead(PIN_MIC);
    suma += x;
    sumaCuadrados += (uint64_t)x * x;
  }
  double media = (double)suma / 500;
  double varianza = (double)sumaCuadrados / 500 - media * media;
  double rms = sqrt(varianza > 0 ? varianza : 0);
  if (rms < 0.5) rms = 0.5;
  return 20.0 * log10(rms / 2048.0);
}

void setup() {
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  Serial.begin(115200);
}

void loop() {
  for (int fase = 0; fase < 4; fase++) {
    unsigned long inicio = millis();
    bool encendido = false;
    while (millis() - inicio < 3000) {
      if (fase == 0) escribirLeds(0x00);
      if (fase == 1) escribirLeds(0xFF);
      if (fase == 2) escribirLeds(0x0F);
      if (fase == 3) { encendido = !encendido; escribirLeds(encendido ? 0xFF : 0x00); }
      float db = medirNivelDb();
      Serial.print("fase:"); Serial.print(fase);
      Serial.print(" db:");  Serial.println(db, 1);
    }
  }
}
