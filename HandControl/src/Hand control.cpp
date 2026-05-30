// ─────────────────────────────────────────────────────────────
//  LUVA (TRANSMISSOR) - Controle de drone por gestos
//  Lê o MPU6050, gera comandos e envia via nRF24L01.
//
//  PROTOCOLO (precisa ser IDÊNTICO ao do drone):
//    magic    : byte fixo 0xA5 para rejeitar lixo de RF
//    throttle : 1000..2000 (absoluto)
//    pitch    : -300..+300 (offset, 0 = neutro)
//    roll     : -300..+300 (offset, 0 = neutro)
//    yaw      : -300..+300 (offset, 0 = neutro)
//    armar    : 1 = pedido de armar (throttle no mínimo)
// ─────────────────────────────────────────────────────────────
#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ── Pinos ──────────────────────────────────────────────────
// SPI fixo do Nano: 11=MOSI, 12=MISO, 13=SCK (não usar p/ outra coisa)
#define RF_CE   9
#define RF_CSN  10
#define LED_PIN 7
// MPU6050 usa I2C: A4=SDA, A5=SCL (fixos no Nano)

// ── Parâmetros ─────────────────────────────────────────────
#define AMOSTRAS_CALIBRACAO 300   // leituras para achar o "zero" da mão
#define DEADZONE            1200  // ignora ruído pequeno (em unidades do MPU)
#define FILTRO_ALFA         0.30f // 0..1  (menor = mais suave, mais atraso)
#define TAXA_ENVIO_MS       20    // ~50 Hz

MPU6050 mpu;
RF24 radio(RF_CE, RF_CSN);
const byte address[6] = "DRN01";

// Mesmo layout nos dois lados. __attribute__((packed)) evita
// que o compilador insira bytes de alinhamento diferentes.
struct __attribute__((packed)) PacoteControle {
  uint8_t  magic;
  uint16_t throttle;
  int16_t  pitch;
  int16_t  roll;
  int16_t  yaw;
  uint8_t  armar;
};

const uint8_t MAGIC = 0xA5;

// Offsets capturados na calibração
int16_t offsetAX = 0, offsetAY = 0, offsetAZ = 0;
int16_t offsetGZ = 0;

// Valores filtrados (estado do passa-baixa)
float fPitch = 0, fRoll = 0, fYaw = 0, fThrottle = 1000;

// ── Aplica deadzone: zera variações pequenas ───────────────
int16_t aplicaDeadzone(int16_t v) {
  if (v > -DEADZONE && v < DEADZONE) return 0;
  return v;
}

// ── Filtro passa-baixa (suaviza tremor da mão) ─────────────
float filtra(float anterior, float novo) {
  return anterior + FILTRO_ALFA * (novo - anterior);
}

// ─────────────────────────────────────────────────────────────
//  CALIBRAÇÃO: média de N leituras com a mão parada = "zero"
// ─────────────────────────────────────────────────────────────
void calibrar() {
  Serial.println(F("Calibrando... mantenha a mao parada e nivelada!"));

  // Pisca avisando que vai calibrar
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, i % 2 == 0 ? HIGH : LOW);
    delay(400);
  }

  long sAX = 0, sAY = 0, sAZ = 0, sGZ = 0;
  for (int i = 0; i < AMOSTRAS_CALIBRACAO; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sAX += ax; sAY += ay; sAZ += az; sGZ += gz;
    delay(3);
  }

  offsetAX = sAX / AMOSTRAS_CALIBRACAO;
  offsetAY = sAY / AMOSTRAS_CALIBRACAO;
  offsetAZ = sAZ / AMOSTRAS_CALIBRACAO;
  offsetGZ = sGZ / AMOSTRAS_CALIBRACAO;

  digitalWrite(LED_PIN, HIGH); // LED fixo = pronto
  Serial.println(F("Calibracao concluida."));
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  Wire.begin();
  Wire.setClock(400000); // I2C rápido = leituras mais frescas
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println(F("ERRO: MPU6050 nao responde. Verifique A4/A5."));
    // Pisca rápido para sempre sinalizando falha de hardware
    while (true) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(120); }
  }

  if (!radio.begin()) {
    Serial.println(F("ERRO: nRF24 nao responde. Verifique alimentacao 3.3V e capacitor."));
    while (true) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(300); }
  }
  radio.setPALevel(RF24_PA_LOW);   // alcance curto, menos consumo (suba se precisar)
  radio.setDataRate(RF24_250KBPS); // mais alcance e robustez
  radio.setChannel(108);           // canal fora do WiFi comum
  radio.setRetries(3, 5);
  radio.openWritingPipe(address);
  radio.stopListening();

  calibrar();
}

void loop() {
  static uint32_t ultimoEnvio = 0;
  if (millis() - ultimoEnvio < TAXA_ENVIO_MS) return;
  ultimoEnvio = millis();

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Movimento relativo ao neutro
  int16_t rAX = aplicaDeadzone(ax - offsetAX);
  int16_t rAY = aplicaDeadzone(ay - offsetAY);
  int16_t rAZ = aplicaDeadzone(az - offsetAZ);
  int16_t rGZ = aplicaDeadzone(gz - offsetGZ);

  // Atitude vira OFFSET centrado em zero (-300..+300)
  float pitchAlvo = constrain(map(rAY, -8000, 8000, -300, 300), -300, 300);
  float rollAlvo  = constrain(map(rAX, -8000, 8000, -300, 300), -300, 300);
  float yawAlvo   = constrain(map(rGZ, -8000, 8000, -300, 300), -300, 300);
  // Throttle é absoluto (1000..2000)
  float throttleAlvo = constrain(map(rAZ, -8000, 8000, 1000, 2000), 1000, 2000);

  // Suaviza tudo
  fPitch    = filtra(fPitch, pitchAlvo);
  fRoll     = filtra(fRoll, rollAlvo);
  fYaw      = filtra(fYaw, yawAlvo);
  fThrottle = filtra(fThrottle, throttleAlvo);

  PacoteControle p;
  p.magic    = MAGIC;
  p.throttle = (uint16_t) fThrottle;
  p.pitch    = (int16_t) fPitch;
  p.roll     = (int16_t) fRoll;
  p.yaw      = (int16_t) fYaw;
  // Pede para armar somente quando a mão está no mínimo (throttle baixo)
  p.armar    = (p.throttle < 1080) ? 1 : 0;

  bool ok = radio.write(&p, sizeof(p));

  // LED: aceso normal; pisca curto se o pacote não foi confirmado (sem link)
  digitalWrite(LED_PIN, ok ? HIGH : (millis() / 100) % 2);
}
