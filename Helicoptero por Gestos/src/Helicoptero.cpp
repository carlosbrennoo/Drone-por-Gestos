// ─────────────────────────────────────────────────────────────
//  DRONE (RECEPTOR) - Quadricoptero controlado por gestos
//  Recebe pacotes da luva via nRF24L01 e comanda 4 ESCs.
//
//  SEGURANÇA:
//   - Arming: só libera os motores depois de receber throttle
//     mínimo por 2s seguidos (evita arrancada acidental).
//   - Failsafe: se ficar 500ms sem pacote válido, desarma e
//     corta os motores (1000us).
//
//  PINOS (todos livres do SPI/I2C):
//    Motor 1 (frente-esq) -> D3
//    Motor 2 (frente-dir) -> D4
//    Motor 3 (tras-dir)   -> D5
//    Motor 4 (tras-esq)   -> D6
//    nRF24: CE=D9, CSN=D10, MOSI=D11, MISO=D12, SCK=D13
//    LED status -> D7
// ─────────────────────────────────────────────────────────────
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

// ── Pinos ──────────────────────────────────────────────────
#define M1_PIN  3
#define M2_PIN  4
#define M3_PIN  5
#define M4_PIN  6
#define RF_CE   9
#define RF_CSN  10
#define LED_PIN 7

// ── Parâmetros de segurança ────────────────────────────────
#define THROTTLE_MIN     1000
#define THROTTLE_MAX     2000
#define THROTTLE_IDLE    1000   // motores parados/mínimo
#define ARM_THRESHOLD    1080   // abaixo disso = throttle "no mínimo"
#define ARM_TEMPO_MS     2000   // tempo de throttle mínimo p/ armar
#define FAILSAFE_MS      500    // sem pacote por isso -> desarma

RF24 radio(RF_CE, RF_CSN);
const byte address[6] = "DRN01";
Servo escM1, escM2, escM3, escM4;

// Layout IDÊNTICO ao da luva
struct __attribute__((packed)) PacoteControle {
  uint8_t  magic;
  uint16_t throttle;
  int16_t  pitch;
  int16_t  roll;
  int16_t  yaw;
  uint8_t  armar;
};

const uint8_t MAGIC = 0xA5;

bool     armado          = false;
uint32_t ultimoPacote    = 0;   // millis do último pacote válido
uint32_t inicioArming    = 0;   // quando começou a segurar throttle mínimo

// ── Escreve o mesmo valor nos 4 motores ────────────────────
void escreveMotores(int v) {
  v = constrain(v, THROTTLE_MIN, THROTTLE_MAX);
  escM1.writeMicroseconds(v);
  escM2.writeMicroseconds(v);
  escM3.writeMicroseconds(v);
  escM4.writeMicroseconds(v);
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  // Anexa os ESCs e manda throttle mínimo para ARMAR os ESCs.
  // (ESC precisa ver o mínimo no boot, senão entra em modo programação.)
  escM1.attach(M1_PIN); escM2.attach(M2_PIN);
  escM3.attach(M3_PIN); escM4.attach(M4_PIN);
  escreveMotores(THROTTLE_IDLE);
  delay(3000); // tempo para os ESCs reconhecerem o mínimo e darem o beep

  if (!radio.begin()) {
    Serial.println(F("ERRO: nRF24 nao responde. Verifique 3.3V e capacitor."));
    while (true) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(300); }
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);          // tem que ser igual ao da luva
  radio.openReadingPipe(1, address);
  radio.startListening();

  Serial.println(F("Drone pronto. Aguardando luva (throttle minimo p/ armar)..."));
}

void loop() {
  PacoteControle p;

  // ── Recebe (pode chegar mais de um pacote por loop) ──────
  while (radio.available()) {
    radio.read(&p, sizeof(p));
    if (p.magic != MAGIC) continue;          // descarta lixo de RF
    ultimoPacote = millis();

    // Lógica de ARMAR: throttle no mínimo por ARM_TEMPO_MS
    if (!armado) {
      if (p.armar && p.throttle < ARM_THRESHOLD) {
        if (inicioArming == 0) inicioArming = millis();
        if (millis() - inicioArming >= ARM_TEMPO_MS) {
          armado = true;
          Serial.println(F("ARMADO."));
        }
      } else {
        inicioArming = 0; // soltou o mínimo -> reinicia contagem
      }
      escreveMotores(THROTTLE_IDLE); // enquanto não arma, fica no mínimo
      continue;
    }

    // ── ARMADO: aplica o mixer ─────────────────────────────
    int base = constrain((int)p.throttle, THROTTLE_MIN, THROTTLE_MAX);

    // Throttle muito baixo = mantém em marcha lenta, ignora atitude
    if (base < ARM_THRESHOLD) {
      escreveMotores(THROTTLE_IDLE);
      continue;
    }

    // Mixer X. pitch/roll/yaw são offsets (-300..+300)
    //   M1(frente-esq)  M2(frente-dir)  M3(tras-dir)  M4(tras-esq)
    int m1 = base + p.pitch - p.roll + p.yaw;
    int m2 = base + p.pitch + p.roll - p.yaw;
    int m3 = base - p.pitch + p.roll + p.yaw;
    int m4 = base - p.pitch - p.roll - p.yaw;

    escM1.writeMicroseconds(constrain(m1, THROTTLE_MIN, THROTTLE_MAX));
    escM2.writeMicroseconds(constrain(m2, THROTTLE_MIN, THROTTLE_MAX));
    escM3.writeMicroseconds(constrain(m3, THROTTLE_MIN, THROTTLE_MAX));
    escM4.writeMicroseconds(constrain(m4, THROTTLE_MIN, THROTTLE_MAX));
  }

  // ── FAILSAFE: perdeu o sinal -> corta tudo e desarma ─────
  if (millis() - ultimoPacote > FAILSAFE_MS) {
    if (armado) Serial.println(F("FAILSAFE: sinal perdido, desarmando."));
    armado = false;
    inicioArming = 0;
    escreveMotores(THROTTLE_IDLE);
  }

  // ── LED: aceso=armado, piscando=aguardando/sem link ──────
  digitalWrite(LED_PIN, armado ? HIGH : (millis() / 250) % 2);
}
