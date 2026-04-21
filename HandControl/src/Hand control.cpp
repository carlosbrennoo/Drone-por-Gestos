#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

MPU6050 mpu;
RF24 radio(9, 10);
const byte address[6] = "00001";

#define LED_PIN 7
#define AMOSTRAS_CALIBRACAO 200  // quantidade de leituras pra calcular o neutro

struct DadosControle {
  int throttle;
  int pitch;
  int roll;
  int yaw;
  bool calibrado;
};

// Offsets capturados na calibração
int16_t offsetAX = 0, offsetAY = 0, offsetAZ = 0;
int16_t offsetGX = 0, offsetGY = 0, offsetGZ = 0;

// ─────────────────────────────────────────
// CALIBRAÇÃO: tira a média de N leituras
// com a mão parada na posição neutra
// ─────────────────────────────────────────
void calibrar() {
  Serial.println("Iniciando calibração... Mantenha a mão parada!");

  // Pisca LED enquanto aguarda o usuário se preparar
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, i % 2 == 0 ? HIGH : LOW);
    delay(500);
  }

  long somaAX = 0, somaAY = 0, somaAZ = 0;
  long somaGX = 0, somaGY = 0, somaGZ = 0;

  for (int i = 0; i < AMOSTRAS_CALIBRACAO; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    somaAX += ax;  somaAY += ay;  somaAZ += az;
    somaGX += gx;  somaGY += gy;  somaGZ += gz;

    delay(5);
  }

  // Calcula a média → esse é o "zero" da mão
  offsetAX = somaAX / AMOSTRAS_CALIBRACAO;
  offsetAY = somaAY / AMOSTRAS_CALIBRACAO;
  offsetAZ = somaAZ / AMOSTRAS_CALIBRACAO;
  offsetGX = somaGX / AMOSTRAS_CALIBRACAO;
  offsetGY = somaGY / AMOSTRAS_CALIBRACAO;
  offsetGZ = somaGZ / AMOSTRAS_CALIBRACAO;

  // LED fixo = calibração concluída
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Calibração concluída!");
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  mpu.initialize();
  pinMode(LED_PIN, OUTPUT);

  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();

  // Calibra logo ao ligar
  calibrar();
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Subtrai o offset → movimento RELATIVO à posição neutra
  int16_t rAX = ax - offsetAX;
  int16_t rAY = ay - offsetAY;
  int16_t rAZ = az - offsetAZ;
  int16_t rGZ = gz - offsetGZ;

  DadosControle dados;
  dados.calibrado = true;

  // Movimento relativo ao neutro mapeado para PWM (1000~2000µs)
  dados.pitch    = map(rAY, -8000, 8000, 1000, 2000);
  dados.roll     = map(rAX, -8000, 8000, 1000, 2000);
  dados.yaw      = map(rGZ, -8000, 8000, 1000, 2000);
  dados.throttle = map(rAZ, -8000, 8000, 1000, 2000);

  // Garante que os valores ficam dentro do range
  dados.pitch    = constrain(dados.pitch,    1000, 2000);
  dados.roll     = constrain(dados.roll,     1000, 2000);
  dados.yaw      = constrain(dados.yaw,      1000, 2000);
  dados.throttle = constrain(dados.throttle, 1000, 2000);

  radio.write(&dados, sizeof(dados));
  delay(20); // ~50Hz de atualização
}