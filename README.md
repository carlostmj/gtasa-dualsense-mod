# 🎮 GTA San Andreas — DualSense PS5 Mod (Universal Controller v3.0.0)

> Suporte nativo completo e direto aos controles **PlayStation 5 DualSense** e **PlayStation 4 DualShock 4** para Grand Theft Auto: San Andreas (PC).
> Comunicação direta via HID (USB e Bluetooth), mira por giroscópio (Gyro Aiming), Roda de Armas com Bullet Time, gestos no Touchpad, gatilhos adaptáveis contextuais, vibração de alta fidelidade e lightbar dinâmica RGB em tempo real.

---

## 🌟 Principais Funcionalidades

### 🎯 1. Gyro Aiming (Mira por Giroscópio no L2)
- **Precisão Cirúrgica**: Ao segurar **L2** (ou mirar com qualquer arma telescópica/pesada), o giroscópio de 6 eixos do DualSense faz o micro-ajuste fino da mira apenas inclinando o controle nas mãos.
- **Sensação Idêntica ao PS5**: Ajuste suave sem jitter no analógico direito, permitindo acertar tiros na cabeça à distância com a precisão de um mouse.
- Totalmente configurável no UniversalController.ini (GyroAim=1, GyroSensX=0.0035, GyroSensY=0.0035, GyroInvertY=0).

---

### 🔫 2. Weapon Wheel com Câmera Lenta (Segurar L1 estilo GTA V / RDR2)
- **Bullet Time Cinematográfico**: Ao **segurar L1** por mais de 200ms, o tempo do jogo desacelera para **20% da velocidade normal**.
- **Seleção Rápida com o Analógico Direito**: Aponte o analógico direito para escolher instantaneamente entre os 8 slots de armas principais do CJ.
- **Feedback Háptico**: O DualSense dá um clique mecânico seco na mão a cada arma apontada na roda.
- **Troca Instantânea**: Ao soltar o L1, o tempo volta ao normal com a arma equipada na hora!
- **Toque Rápido no L1 (Tap < 200ms)**: Mantém o ciclo tradicional de troca de arma anterior.

---

### 👆 3. Gestos no Touchpad do DualSense (Swipe Gestures)
- **Deslizar para Cima (Swipe Up)**: Atende chamadas do celular ou exibe a tela de estatísticas do CJ (equivalente à tecla TAB / Ação).
- **Deslizar para a Direita (Swipe Right)**: Próxima estação de rádio ao dirigir.
- **Deslizar para a Esquerda (Swipe Left)**: Estação de rádio anterior ao dirigir.
- **Clique no Touchpad**: Abre o Mapa do jogo em tela cheia instantaneamente.

---

### 🚗 4. Física de Terreno & Derrapagem nos Gatilhos (R2)
- **Burnout / Pneu Cantando em Falso**: Ao acelerar parado ou em baixa velocidade com alto RPM, o gatilho R2 trepida com vibração mecânica de alta frequência simulando o pneu patinando.
- **Freio de Mão / Drift em Curva**: Ao puxar o freio de mão em velocidade, o gatilho R2 alivia a resistência e vibra suavemente traduzindo a perda de aderência traseira.
- **Câmbio & Troca de Marchas**: Tranco seco mecânico no milissegundo em que a marcha engata.
- **Freio ABS no L2**: Trepidação rápida de freio antitravamento em alta velocidade.
- **Veículos Pesados**: Freio mais rígido e pesado em caminhões, ônibus e blindados.

---

### 🌊 5. Mergulho & Oxigênio Subaquático (Pressão nos Gatilhos)
- O mod monitora a capacidade pulmonar do CJ (m_fBreath).
- **Fôlego Baixo (< 40%)**: Os gatilhos L2 e R2 ficam rígidos e pesados, simulando a pressão da água e o esforço do mergulho.
- **Sufocamento Iminente (< 15%)**: Pulsos violentos de batimento cardíaco nos gatilhos e motores de vibração alertando o perigo de afogamento antes de perder vida.

---

### 🌈 6. Lightbar Dinâmica RGB em Tempo Real
- **Saúde Cheia (> 70%)**: Verde clássico Grove Street (#1EDC28).
- **Saúde Média (35% - 70%)**: Amarelo/Laranja de alerta (#FF8C00).
- **Saúde Baixa (15% - 35%)**: Vermelho de perigo (#FF1414).
- **Saúde Crítica (< 15%)**: Batimento cardíaco pulsante em vermelho vivo.
- **Perseguição Policial (Nível de Procurado > 0)**: Giroflex policial estroboscópico alternando em alta velocidade entre **Vermelho e Azul**!
- **Menus & Cutscenes**: Azul suave PlayStation (#0078FF).

---

### 🎯 7. Mira 100% Livre (Pure Free Aim) & Zoom no D-Pad
- Mira livre desvinculada de auto-aim / lock-on magnético.
- Andar e esquivar mirando com todas as armas pesadas.
- Zoom de Sniper / Câmera nas setas do D-pad (Cima = Zoom In, Baixo = Zoom Out).
- Navegação de 1 em 1 nos menus sem pular opções.

---

## 📂 Arquivos do Repositório

| Arquivo | Descrição |
|---|---|
| UniversalController.c | Código-fonte completo em C (hooks de motor GTA SA, Direct HID, Gyro Aim, CRC32) |
| UniversalController.asi | Binário compilado pronto para uso no ModLoader |
| UniversalController.ini | Arquivo de configurações de sensibilidade, giroscópio e deadzones |
| gamepad_tester.py | Utilitário em Python para testes de feedback háptico, gatilhos e botões |
| mapeador_controle.py | Ferramenta visual interativa para teste e mapeamento de controles |

---

## 💻 Instalação

1. Baixe o pacote **UniversalController-v3.0.0.zip** na aba de [Releases](https://github.com/carlostmj/gtasa-dualsense-mod/releases).
2. Extraia o conteúdo na pasta raiz do seu **GTA San Andreas** (onde fica o gta_sa.exe e a pasta modloader).
3. O mod ficará em:
   `
   GTA San Andreas\modloader\UniversalController\UniversalController.asi
   GTA San Andreas\modloader\UniversalController\UniversalController.ini
   `
4. Conecte o controle DualSense (PS5) ou DualShock 4 (PS4) via **Bluetooth** ou **USB** e jogue!

---

## ⚙️ Configurações (UniversalController.ini)

`ini
[Settings]
ActivePreset=01_PlayStation5_DualSense
PresetName=PlayStation 5 DualSense
DeadzoneLeft=18
DeadzoneRight=20
CamSensX=0.08
CamSensY=0.06
InvertY=0
ControllerType=1
GyroAim=1
GyroSensX=0.0035
GyroSensY=0.0035
GyroInvertY=0
`
