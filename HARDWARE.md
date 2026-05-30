# Hardware, Ligações Elétricas e Montagem Física

Guia completo de como montar a **luva (transmissor)** e o **drone (receptor)**, incluindo alimentação por bateria (sem tomada).

> ⚠️ **Antes de tudo:** o nRF24L01 é a peça mais sensível do projeto. 90% dos
> problemas de "não conecta" são por **alimentação ruim no 3.3V**. Sempre solde
> um **capacitor de 10µF (até 100µF) entre VCC e GND do módulo**, o mais perto
> possível dos pinos. Use uma fonte 3.3V dedicada (AMS1117-3.3), **não** o pino
> 3V3 do Nano (ele só entrega ~50mA, o rádio puxa picos maiores).

---

## 1. Tabela de pinos (firmware atual)

### Luva (HandControl)
| Sinal | Pino Nano | Vai para |
|-------|-----------|----------|
| RF24 CE | D9 | nRF24 CE |
| RF24 CSN | D10 | nRF24 CSN |
| RF24 MOSI | D11 | nRF24 MOSI |
| RF24 MISO | D12 | nRF24 MISO |
| RF24 SCK | D13 | nRF24 SCK |
| MPU SDA | A4 | MPU6050 SDA |
| MPU SCL | A5 | MPU6050 SCL |
| LED status | D7 | LED + resistor 220Ω |

### Drone (Helicoptero)
| Sinal | Pino Nano | Vai para |
|-------|-----------|----------|
| Motor 1 (frente-esq) | D3 | ESC 1 (fio de sinal) |
| Motor 2 (frente-dir) | D4 | ESC 2 (fio de sinal) |
| Motor 3 (trás-dir) | D5 | ESC 3 (fio de sinal) |
| Motor 4 (trás-esq) | D6 | ESC 4 (fio de sinal) |
| RF24 CE | D9 | nRF24 CE |
| RF24 CSN | D10 | nRF24 CSN |
| RF24 MOSI | D11 | nRF24 MOSI |
| RF24 MISO | D12 | nRF24 MISO |
| RF24 SCK | D13 | nRF24 SCK |
| LED status | D7 | LED + resistor 220Ω |

> Os pinos D11/D12/D13 são o barramento **SPI** — agora **só** o rádio os usa.
> O conflito antigo (Motor 4 no D11) foi eliminado.

---

## 2. Esquema elétrico — LUVA (transmissor)

```
                    BATERIA 1S LiPo 3.7V
                    (ex.: 503035 ~500mAh)
                         │      │
                       (+)│      │(-)
                         │      └──────────────┬──── GND (comum)
                         │                      │
                  ┌──────┴───────┐              │
                  │ BOOST MT3608 │              │
                  │  3.7V -> 5V  │              │
                  └──────┬───────┘              │
                       5V│                      │
              ┌──────────┼──────────────────────┤
              │          │                      │
              │   ┌──────┴──────┐               │
              │   │ Arduino Nano│               │
              │   │             │               │
              │ 5V│ VIN/5V   GND├───────────────┤
              │   │             │               │
              │   │ A4 ───────────► SDA ┐       │
              │   │ A5 ───────────► SCL ┤ MPU6050
              │   │ 5V ───────────► VCC ┤  (módulo)
              │   │ GND──────────► GND ─┘───────┤
              │   │             │               │
              │   │ D9  ──► CE  │               │
              │   │ D10 ──► CSN │               │
              │   │ D11 ──► MOSI│   nRF24L01+    │
              │   │ D12 ──► MISO│               │
              │   │ D13 ──► SCK │               │
              │   │ D7 ─[220Ω]─►│LED──┐         │
              │   └─────────────┘     └─────────┤
              │                                 │
              │   ┌──────────────┐              │
              └───┤ AMS1117-3.3  │              │
                 5V│  -> 3.3V    │3.3V─► VCC nRF24
                  │   GND        ├──────────────┘
                  └──────┬───────┘
                         │   ┌──┤├──┐  capacitor 10µF
                      VCC├───┤      ├── GND   (colado no nRF24)
                         nRF24L01+
```

**Resumo da alimentação da luva:**
- Bateria **1S LiPo 3.7V** → **boost MT3608** ajustado para **5V** → alimenta o Nano (pino 5V/VIN) e o MPU6050.
- Um **AMS1117-3.3** tira 3.3V dos 5V para o **nRF24** (com o capacitor de 10µF).
- Todos os GND juntos (massa comum).

> 💡 Alternativa mais simples (porém mais pesada): usar **2x 18650 em série
> (7.4V)** no pino **VIN** do Nano — o regulador interno faz os 5V. Ainda assim
> use o AMS1117-3.3 para o rádio.

---

## 3. Esquema elétrico — DRONE (receptor)

```
            BATERIA DE VOO 2S LiPo 7.4V (ex.: 850–1300mAh 30C)
                         │              │
                      (+)│              │(-)
            ┌────────────┼──────────────┼────────────┐
            │            │              │            │
       ┌────┴────┐  ┌────┴────┐   ┌────┴────┐  ┌────┴────┐
       │  ESC 1  │  │  ESC 2  │   │  ESC 3  │  │  ESC 4  │
       │  30A    │  │  30A    │   │  30A    │  │  30A    │
       └─┬──┬──┬─┘  └─┬────┬──┘   └─┬────┬──┘  └─┬────┬──┘
      sinal│GND│5V  sinal GND     sinal GND    sinal GND
         │  │  │(BEC)
         │  │  └───────────────────────► 5V Nano (só UM BEC ligado ao 5V!)
         │  └──────────────────────────► GND comum
         │
         │  Motores:                        Arduino Nano
         │  ESC1 sinal ──► D3            ┌────────────────┐
         │  ESC2 sinal ──► D4            │                │
         │  ESC3 sinal ──► D5         5V►│ 5V         GND ►│ GND comum
         │  ESC4 sinal ──► D6            │                │
         │                              │ D3 ──► ESC1     │
         │                              │ D4 ──► ESC2     │
         │                              │ D5 ──► ESC3     │
         │                              │ D6 ──► ESC4     │
         │                              │                │
         │            nRF24L01+         │ D9  ──► CE      │
         │            ┌───────┐         │ D10 ──► CSN     │
         │      VCC◄3.3V      │         │ D11 ──► MOSI    │
         │ (AMS1117-3.3 dos 5V)         │ D12 ──► MISO    │
         │      GND── capacitor 10µF    │ D13 ──► SCK     │
         │                              │ D7 ─[220Ω]► LED │
         │                              └────────────────┘
```

**Regras de ouro do drone:**
1. **Bateria principal** alimenta os **4 ESCs** direto (fios grossos / power distribution board).
2. **Apenas UM** fio de 5V (BEC) de **um** ESC vai ao pino 5V do Nano. **Corte o fio +5V (vermelho) dos outros 3 ESCs** para não brigarem entre si — deixe só sinal e GND deles.
3. **Todos os GND** no mesmo ponto (bateria, ESCs, Nano, rádio).
4. nRF24 recebe **3.3V** do AMS1117 + capacitor 10µF.

---

## 4. Sentido de rotação dos motores e hélices

Para um quadricóptero estável em "X", os motores giram em pares opostos:

```
        FRENTE
   M1(CW)     M2(CCW)
      \         /
       \       /
        +-----+
        |NANO |
        +-----+
       /       \
      /         \
   M4(CCW)    M3(CW)
        TRÁS
```

- **M1 e M3:** sentido horário (CW) → hélices CW
- **M2 e M4:** sentido anti-horário (CCW) → hélices CCW
- Se um motor girar ao contrário do esperado, **troque dois dos três fios** entre o motor e o ESC.
- Use o par de hélices correto (CW/CCW). Hélice errada = drone não sobe / capota.

> O **mixer do firmware** assume exatamente esse arranjo (M1 frente-esq,
> M2 frente-dir, M3 trás-dir, M4 trás-esq). Monte fisicamente nessa ordem.

---

## 5. Baterias recomendadas (sem tomada)

| Onde | Bateria | Por quê |
|------|---------|---------|
| **Luva** | 1S LiPo 3.7V 400–600mAh (ex.: 503035) + boost MT3608 | Leve, plana, cabe na luva. Dura horas (consumo baixo). |
| **Luva (alt.)** | 2x 18650 (7.4V) no VIN | Mais barato/robusto, porém pesado para a mão. |
| **Drone** | 2S LiPo 7.4V 850–1300mAh 25–30C | Padrão de mini-quad; entrega a corrente de pico dos motores. |
| **Drone (maior)** | 3S LiPo 11.1V | Só se os motores/ESCs forem 3S. **Confira o KV e a tensão dos ESCs.** |

⚠️ **Segurança LiPo:**
- Nunca deixe descarregar abaixo de **3.0V por célula**.
- Carregue em carregador balanceador próprio de LiPo, em superfície não inflamável.
- Não fure, não dobre, não deixe no calor.

---

## 6. Lista de compras (módulos prontos — caminho mais fácil)

**Luva:**
- 1x Arduino Nano (com cabo USB p/ gravar)
- 1x MPU6050 (módulo GY-521)
- 1x nRF24L01+ (de preferência o módulo com socket/adaptador que já traz regulador 3.3V)
- 1x boost MT3608 (ou regulador AMS1117-3.3 avulso)
- 1x bateria 1S LiPo + capacitor 10µF + LED + resistor 220Ω
- Fio fino, luva, fita/velcro

**Drone:**
- 1x Arduino Nano
- 1x nRF24L01+ (idem, com adaptador regulado ajuda muito)
- 4x ESC 30A + 4x motor brushless (KV compatível com a bateria)
- 2 pares de hélices (CW e CCW)
- 1x frame de quadricóptero (250–450mm)
- 1x bateria 2S LiPo + power distribution board (ou solda em estrela)
- LED + resistor 220Ω + capacitor 10µF

---

## 7. Organização física

### Luva
1. Fixe o **MPU6050 nas costas da mão** (palma para baixo, chip nivelado) — é a referência da calibração.
2. Nano + rádio + bateria num **pequeno case no antebraço/pulso** (menos peso na mão).
3. Deixe o **LED visível** (status de calibração/link).
4. **Calibre sempre com a mão parada e nivelada** ao ligar.

### Drone
1. Nano + rádio **no centro do frame**, longe dos motores (ruído elétrico).
2. **Antena do nRF24 para fora**, sem metal/fibra de carbono em volta.
3. Bateria embaixo, no centro de gravidade; prenda com cinta de velcro.
4. ESCs nos braços, perto de cada motor; fios curtos.
5. **Capacitor do rádio colado nos pinos VCC/GND** do módulo.

---

## 8. Primeiro teste seguro (SEM hélices!)

> 🚨 **Faça TODO o primeiro teste sem as hélices instaladas.**

1. Ligue **primeiro a luva** e deixe calibrar (LED fixo).
2. Ligue o drone. Os ESCs vão **bipar** (sequência de arming). LED do drone pisca.
3. Com a mão **parada e baixa** (throttle no mínimo), espere ~2s → drone **arma** (LED fixo).
4. Mova a mão devagar e confira se cada motor acelera no sentido certo do mixer.
5. **Teste o failsafe:** desligue a luva → os motores devem **parar em ~0,5s**.
6. Só depois de tudo OK, com bateria carregada e área livre, instale as hélices.

---

## 9. Checklist de problemas comuns

| Sintoma | Causa provável | Ação |
|---------|----------------|------|
| nRF24 não inicia (LED pisca rápido) | 3.3V fraco / sem capacitor | AMS1117-3.3 + capacitor 10µF |
| Conecta mas perde pacotes | Canal/PA/datarate diferentes | Confirme `setChannel(108)` e `RF24_250KBPS` nos dois |
| ESC não arma (bip contínuo) | Não viu throttle mínimo no boot | Garanta `THROTTLE_IDLE` no `setup` (já está) |
| Motor gira ao contrário | Fase trocada | Inverta 2 dos 3 fios motor↔ESC |
| Drone "puxa" parado | Pitch/roll não estão em zero | Recalibre a luva nivelada |
| Drone foge sozinho | (era o bug antigo, já corrigido) | Confirme failsafe ativo no Serial |
| Nano reinicia ao acelerar | BEC fraco / GND mal feito | 5V de um BEC bom, GND em estrela |
