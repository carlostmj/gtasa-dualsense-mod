# 🎮 GTA San Andreas — DualSense PS5 Mod (Universal Controller v2.8.0)

> Suporte nativo completo e direto aos controles **PlayStation 5 DualSense** e **PlayStation 4 DualShock 4** para Grand Theft Auto: San Andreas (PC).
> Comunicação direta via HID (USB e Bluetooth), gatilhos adaptáveis contextuais, vibração de alta fidelidade, lightbar dinâmica RGB em tempo real e mira livre aprimorada.

---

## 🌟 Principais Funcionalidades

### 🌈 1. Lightbar Dinâmica RGB em Tempo Real
A barra de luz do DualSense e DualShock 4 reage dinamicamente aos acontecimentos e estado físico do CJ no jogo:
- **Saúde Cheia (> 70%)**: Verde clássico Grove Street (#1EDC28).
- **Saúde Média (35% - 70%)**: Amarelo/Laranja de alerta (#FF8C00).
- **Saúde Baixa (15% - 35%)**: Vermelho de perigo (#FF1414).
- **Saúde Crítica (< 15%)**: Batimento cardíaco pulsante em vermelho vivo.
- **Perseguição Policial (Nível de Procurado > 0)**: Giroflex policial estroboscópico alternando em alta velocidade entre **Vermelho e Azul**, transformando o controle numa sirene física nas perseguições!
- **Menus & Cutscenes**: Azul suave PlayStation (#0078FF).

---

### ⚡ 2. Gatilhos Adaptáveis Contextuais (R2 & L2)
- **Câmbio & Troca de Marchas (R2)**: O mod monitora a física e a transmissão dos veículos em tempo real. No milissegundo exato em que o carro troca de marcha, o gatilho de aceleração dá um tranco mecânico seco de embreagem acoplando.
- **Freio com Sistema ABS (L2)**: Ao frear bruscamente em alta velocidade, o gatilho L2 vibra com pulsos rápidos de resistência mecânica simulando o pedal de freio trepidando contra o travamento das rodas.
- **Veículos Pesados (Caminhões, Ônibus e Blindados)**: Resistência e peso aumentados no freio (L2) para veículos pesados como Linerunner, Tanker, Flatbed, Bus e Rhino.
- **Cansaço e Fadiga do CJ (Stamina / Fôlego)**: Ao esgotar o fôlego correndo, nadando ou pedalando, os gatilhos L2 e R2 ficam pesados e começam a tremer, transmitindo a exaustão física do personagem diretamente para as suas mãos.
- **Resistência por Categoria de Arma**:
  - Pistolas e Submetralhadoras: Puxada leve com recuo mecânico rápido.
  - Espingardas e Fuzis: Resistência progressiva firme.
  - Armas Pesadas (Minigun, Rocket Launcher): Gatilho rígido e firme.

---

### 🎯 3. Mira 100% Livre (Pure Free Aim) & Movimentação
- **Mira Livre Profissional**: Remoção completa do sistema de mira magnética automática (Auto-Aim Lock-On) original do GTA SA, proporcionando controle total, suave e cirúrgico da mira pelo analógico direito.
- **Andar e Esquivar Mirando (Move While Aiming)**: Permite que o CJ ande, corra e faça esquiva lateral mirando e disparando com qualquer arma pesada (Shotguns, M4, AK-47, Minigun, Sniper), sem o congelamento de pés original de 2004.
- **Zoom de Precisão de Sniper no D-Pad**: Ajuste de zoom in (Seta para Cima) e zoom out (Seta para Baixo) diretamente no direcional digital enquanto mira com rifle de precisão.

---

### 🕹️ 4. Interface, Menus & Atalhos de Navegação
- **Navegação Cirúrgica nos Menus**: D-Pad navega perfeitamente de 1 em 1 opção, sem pular itens acidentalmente.
- **Botão Start / Options**: Abre e fecha instantaneamente o menu de pausa / opções do jogo.
- **Botão Share / Touchpad**: Abre diretamente a tela cheia do Mapa do jogo em qualquer momento da gameplay.
- **Ícones Nativos de PlayStation**: Tutoriais e dicas na tela exibem os botões clássicos do PlayStation (Cruz, Quadrado, Triângulo, Círculo, L1/R1, L2/R2).

---

## 📂 Arquivos do Repositório

| Arquivo | Descrição |
|---|---|
| UniversalController.c | Código-fonte completo em C (hooks de motor GTA SA, Direct HID, CRC32 e threads) |
| UniversalController.asi | Binário compilado pronto para uso no ModLoader |
| UniversalController.ini | Arquivo de configurações de sensibilidade, deadzones e eixos |
| gamepad_tester.py | Utilitário em Python para testes de feedback háptico, gatilhos e botões |
| mapeador_controle.py | Ferramenta visual interativa para teste e mapeamento de controles |
| Testar_Controle.bat | Atalho de inicialização do testador |
| Mapear_Controle.bat | Atalho de inicialização do mapeador |

---

## 💻 Instalação

### Pré-requisitos
- GTA San Andreas para PC (v1.0 US recomendada).
- [CLEO 4](https://cleo.li/) ou [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).
- [Modloader](https://github.com/thelink2012/modloader) instalado na raiz do jogo.
- Controle PlayStation 5 DualSense ou DualShock 4 conectado via **USB** ou **Bluetooth**.

### Instruções
1. Coloque a pasta UniversalController com o arquivo UniversalController.asi e UniversalController.ini dentro do diretório modloader\ do seu GTA:
   `
   GTA San Andreas\modloader\UniversalController\UniversalController.asi
   GTA San Andreas\modloader\UniversalController\UniversalController.ini
   `
2. Inicie o jogo com o controle conectado via Bluetooth ou cabo USB.
3. O mod detectará automaticamente o dispositivo e ativará todas as funções sem necessidade de drivers externos ou emuladores adicionais.

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
`

---

## 🛠️ Compilação

Para compilar o código-fonte a partir do MinGW (GCC 32-bit):
`ash
i686-w64-mingw32-gcc -shared -O3 -s -o UniversalController.asi UniversalController.c -lwinmm -lsetupapi -lhid
`
