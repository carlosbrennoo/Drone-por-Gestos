#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(9, 10);
const byte address[6] = "00001";
Servo escMotor1, escMotor2, escMotor3, escMotor4;

#define LED_PIN 7

struct DadosControle {
  int throttle;
  int pitch;
  int roll;
  int yaw;
  bool calibrado;
};

void setup() {
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

  escMotor1.attach(3);
  escMotor2.attach(5);
  escMotor3.attach(6);
  escMotor4.attach(11);

  pinMode(LED_PIN, OUTPUT);

  // Drone aguarda sinal calibrado antes de aceitar comandos
  Serial.println("Aguardando calibração da luva...");
}

void loop() {
  if (radio.available()) {
    DadosControle dados;
    radio.read(&dados, sizeof(dados));

    // Só age se a luva já foi calibrada
    if (!dados.calibrado) {
      digitalWrite(LED_PIN, LOW);
      return;
    }

    digitalWrite(LED_PIN, HIGH);

    // Mixer: combina os 4 eixos para cada motor
    //        M1(frente-esq)  M2(frente-dir)  M3(trás-dir)  M4(trás-esq)
    int m1 = dados.throttle + dados.pitch - dados.roll + dados.yaw;
    int m2 = dados.throttle + dados.pitch + dados.roll - dados.yaw;
    int m3 = dados.throttle - dados.pitch + dados.roll + dados.yaw;
    int m4 = dados.throttle - dados.pitch - dados.roll - dados.yaw;

    escMotor1.writeMicroseconds(constrain(m1, 1000, 2000));
    escMotor2.writeMicroseconds(constrain(m2, 1000, 2000));
    escMotor3.writeMicroseconds(constrain(m3, 1000, 2000));
    escMotor4.writeMicroseconds(constrain(m4, 1000, 2000));
  }
}