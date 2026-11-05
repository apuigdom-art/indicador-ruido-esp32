/*
  Indicador de nivel de ruido ambiental
  ESP32 (ESP-WROOM-32) + MAX4466 + 74HC595 + 8 LEDs
  Autora: Ariadna Puigdomenech Chaves
*/
#include <math.h>

// Pines
const uint8_t PIN_MIC   = 4;    // ADC2_CH0 (válido porque no se usa el Wi-Fi)
const uint8_t PIN_DATA  = 23;   // -> DS    (pin 14 del 74HC595)
const uint8_t PIN_CLOCK = 18;   // -> SH_CP (pin 11)
const uint8_t PIN_LATCH = 19;   // -> ST_CP (pin 12)

// Parámetros de medida
const uint32_t FS_HZ = 10000;                 // frecuencia de muestreo
const uint32_t PERIODO_US = 1000000 / FS_HZ;  // 100 us entre muestras
const uint16_t N_MUESTRAS = 500;              // 500 muestras = ventana de 50 ms
const float NIVEL_MIN = -45.0;  // dBFS con 0 LEDs (calibrado: silencio ~ -50 dBFS)
const float NIVEL_MAX = -25.0;  // dBFS con 8 LEDs (calibrado: música ~ -36 dBFS)
const float ALFA = 0.3;         // suavizado exponencial (0..1)
const float HISTERESIS = 0.3;   // fracción de escalón para apagar un LED

float nivelSuavizado = NIVEL_MIN;
int leds = 0;                   // LEDs encendidos actualmente

// Envía 8 bits al 74HC595 y los pasa a las salidas Q0..Q7
void escribirLeds(uint8_t patron) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, patron);
  digitalWrite(PIN_LATCH, HIGH);
}

// Toma N_MUESTRAS a FS_HZ y devuelve el nivel RMS en dBFS
float medirNivelDb() {
  uint32_t suma = 0;
  uint64_t sumaCuadrados = 0;
  uint32_t t = micros();
  for (uint16_t i = 0; i < N_MUESTRAS; i++) {
    while (micros() - t < PERIODO_US) {}   // espera al instante de muestreo
    t += PERIODO_US;
    uint32_t x = analogRead(PIN_MIC);
    suma += x;
    sumaCuadrados += (uint64_t)x * x;
  }

  // Varianza = media de los cuadrados - cuadrado de la media (quita la continua)
  double media = (double)suma / N_MUESTRAS;
  double varianza = (double)sumaCuadrados / N_MUESTRAS - media * media;
  double rms = sqrt(varianza > 0 ? varianza : 0);
  if (rms < 0.5) rms = 0.5;                  // evita log10(0)
  return 20.0 * log10(rms / 2048.0);         // 2048 = mitad del rango de 12 bits
}

void setup() {
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  analogReadResolution(12);          // 0..4095
  analogSetAttenuation(ADC_11db);    // rango de entrada ~0..3,1 V
  Serial.begin(115200);

  // Prueba de arranque: la barra se llena y se vacía
  for (uint8_t i = 0; i <= 8; i++) {
    escribirLeds((1 << i) - 1);
    delay(100);
  }
  escribirLeds(0);
  medirNivelDb();                    // descarta la perturbación del último cambio
}

void loop() {
  float nivel = medirNivelDb();
  nivelSuavizado = ALFA * nivel + (1 - ALFA) * nivelSuavizado;

  // Posición en la barra (0..8); para apagar un LED el nivel debe bajar
  // HISTERESIS escalones por debajo, así no parpadea en el límite
  float posicion = (nivelSuavizado - NIVEL_MIN) / (NIVEL_MAX - NIVEL_MIN) * 8;
  int objetivo = constrain((int)floor(posicion), 0, 8);
  if (objetivo != leds && (objetivo > leds || posicion < leds - HISTERESIS)) {
    leds = objetivo;
    escribirLeds((1 << leds) - 1);   // enciende Q0..Q(leds-1)
    // Al conmutar, los LEDs perturban la medida del micrófono:
    // se descarta la siguiente ventana
    medirNivelDb();
  }

  // Formato compatible con el Serial Plotter del IDE de Arduino
  Serial.print("nivel_dBFS:");
  Serial.print(nivelSuavizado, 1);
  Serial.print(" leds:");
  Serial.println(leds);
}
