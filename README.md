# Drone por Gestos 🚁

Controle de um quadricóptero por **gestos da mão**: uma luva com acelerômetro/giroscópio
(MPU6050) lê o movimento e envia os comandos por rádio (nRF24L01+) para o drone, que
mistura os 4 eixos e comanda os motores via ESC. Sem controle remoto convencional.

- **Comunicação:** nRF24L01+ 2.4GHz (canal 108, 250kbps)
- **Eixos:** Throttle, Pitch, Roll, Yaw
- **Placas:** 2x Arduino Nano (ATmega328P)

---

## Estrutura do repositório

| Pasta | O que é | Roda em |
|-------|---------|---------|
| [`HandControl/`](HandControl/src/Hand%20control.cpp) | Firmware da **luva** (transmissor): lê MPU6050 e envia comandos | Arduino Nano |
| [`Helicoptero por Gestos/`](Helicoptero%20por%20Gestos/src/Helicoptero.cpp) | Firmware do **drone** (receptor): recebe e comanda os 4 ESCs | Arduino Nano |

📐 **Esquema elétrico completo, baterias e montagem física:** veja **[HARDWARE.md](HARDWARE.md)**.

---

## Como funciona (visão rápida)

```
   LUVA (transmissor)                         DRONE (receptor)
 ┌───────────────────┐                     ┌────────────────────┐
 │ MPU6050 (gesto)   │      nRF24L01+      │ nRF24L01+          │
 │  -> Nano          │ ───── 2.4GHz ─────► │  -> Nano           │
 │  -> nRF24L01+     │   pacote 50Hz       │  -> Mixer -> 4 ESC │
 └───────────────────┘                     └────────────────────┘
```

**Protocolo** (idêntico nos dois lados, `struct __attribute__((packed))`):

| Campo | Tipo | Faixa | Significado |
|-------|------|-------|-------------|
| `magic` | uint8 | 0xA5 | rejeita lixo de RF |
| `throttle` | uint16 | 1000–2000 | aceleração (absoluto) |
| `pitch` | int16 | −300..+300 | inclinação frente/trás (offset) |
| `roll` | int16 | −300..+300 | inclinação esq/dir (offset) |
| `yaw` | int16 | −300..+300 | giro (offset) |
| `armar` | uint8 | 0/1 | pedido de armar (throttle no mínimo) |

---

## Segurança embutida no firmware

- **Arming:** o drone só libera os motores depois de receber **throttle mínimo por 2s**.
- **Failsafe:** se passar **500ms sem pacote válido**, o drone **desarma e corta os motores**.
- **Arming dos ESCs:** no boot o drone manda throttle mínimo por 3s (senão o ESC não inicializa).
- **Deadzone + filtro** na luva para o drone não ficar nervoso com o tremor da mão.
- **Checagem de hardware:** se MPU ou rádio não responderem, o LED pisca em código de erro.

---

## Pinagem resumida

**Luva:** RF24 CE=D9, CSN=D10 (SPI: D11/D12/D13) · MPU6050 SDA=A4, SCL=A5 · LED=D7
**Drone:** Motores D3,D4,D5,D6 · RF24 CE=D9, CSN=D10 (SPI: D11/D12/D13) · LED=D7

> Detalhes, diagramas e alimentação por bateria: **[HARDWARE.md](HARDWARE.md)**.

---

## Compilar e gravar (PlatformIO)

```bash
# Luva
cd "HandControl"
pio run -e nanoatmega328new            # compila
pio run -e nanoatmega328new -t upload  # grava no Nano

# Drone
cd "Helicoptero por Gestos"
pio run -e nanoatmega328new -t upload
```

---

## ⚠️ Primeiro teste — SEMPRE sem hélices

1. Ligue a **luva**, deixe **calibrar** (mão parada e nivelada → LED fixo).
2. Ligue o **drone** (ESCs bipam, LED pisca).
3. Mão **parada e baixa** ~2s → drone **arma** (LED fixo).
4. Mova devagar e confira o sentido de cada motor.
5. **Teste o failsafe:** desligue a luva → motores param em ~0,5s.
6. Só então, em área livre, instale as hélices.

Passo a passo completo e checklist de problemas: **[HARDWARE.md](HARDWARE.md)**.

---

## Avisos

⚠️ Drones causam ferimentos — opere em área segura, sem pessoas por perto.
⚠️ Baterias LiPo podem incendiar — carregue em balanceador, nunca abaixo de 3.0V/célula.
⚠️ Projeto educacional, sem estabilização por PID — exige mão firme e área controlada.

## Licença

Veja [LICENSE](LICENSE).
