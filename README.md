# 🎮 GTA San Andreas — DualSense PS5 Mod

> Full DualSense PS5 controller support for Grand Theft Auto: San Andreas PC
> Adaptive triggers · Haptic rumble · Lightbar HP colors · D-Pad menu navigation

---

## Features

### ⚡ Adaptive Triggers (R2 / L2)
| Situation | Effect |
|---|---|
| Walking / running | Soft resistance on R2 |
| Driving (acceleration) | Progressive resistance on R2 |
| Shooting | Short snap on R2 trigger |
| Heavy weapons (rocket, minigun) | Full stiff resistance on R2 |
| Braking (L2) | Strong resistance feedback |

- **R2 curto** → disparo rápido (pistola, SMG)
- **R2 forte / segurado** → armas pesadas (rocket launcher, minigun, sniper)

### 💥 Haptic Rumble
- Vibração ao receber dano
- Rumble proporcional à velocidade do veículo / colisão
- Feedback ao atirar (baixa e alta intensidade por arma)
- Explosões próximas geram pulso forte

### 🔴 Lightbar — Cor por HP
| HP | Cor |
|---|---|
| 100% | Azul |
| ~75% | Verde |
| ~50% | Amarelo |
| ~25% | Laranja |
| Crítico | Vermelho piscando |
| Morto | Apagado |

### 🎯 Mira Automática de Console (Auto-Aim / Lock-On)
- **Mira no L2**: Pressione L2 para travar a mira automaticamente no alvo mais próximo com o retículo clássico colorido de saúde do GTA dos consoles.
- **Troca de Alvo**: Mova o analógico direito para alternar rapidamente entre diferentes inimigos no campo de visão.

### 🏃 Andar & Esquivar Mirando (Move & Strafe While Aiming)
- Permite que o CJ ande, corra e faça esquiva lateral enquanto mira e atira com **qualquer arma pesada** (M4, AK-47, Shotguns, Minigun, Sniper, etc.), removendo o travamento de pés original do GTA San Andreas de 2004.

### ⌨️🎮 Controle + Teclado Simultâneo
- Jogabilidade híbrida perfeita: use o DualSense junto com teclado e mouse ao mesmo tempo sem conflito de inputs.

### 🗺️ Navegação de Menu com D-Pad
- **D-Pad ↑↓** → navegação nos menus do jogo (pause, inventário, opções)
- **D-Pad ←→** → seleção lateral / alternar abas
- **Share button** → abre o Mapa do jogo diretamente
- **Options button** → abre o menu de pause

---

## Files

| File | Description |
|---|---|
| `UniversalController.c` | Source code (C) — main mod logic |
| `UniversalController.asi` | Compiled binary — drop into GTA SA folder |
| `UniversalController.ini` | Configuration file — modloader path |
| `gamepad_tester.py` | Python tool to test DualSense inputs and rumble |
| `mapeador_controle.py` | Python tool to map controller buttons interactively |
| `Testar_Controle.bat` | Shortcut to launch gamepad tester |
| `Mapear_Controle.bat` | Shortcut to launch button mapper |

---

## Installation

### Requirements
- GTA San Andreas PC (original or Steam)
- [CLEO 4](https://cleo.li/) or [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
- [Modloader](https://github.com/thelink2012/modloader) (recommended)
- DualSense controller connected via USB or Bluetooth

### Steps
1. Copy `UniversalController.asi` to your GTA SA install directory
   (e.g. `C:\Program Files (x86)\Rockstar Games\GTA San Andreas\`)
2. Copy `UniversalController.ini` to:
   `GTA SA\modloader\UniversalController\UniversalController.ini`
3. Connect your DualSense via USB or Bluetooth
4. Launch the game — the mod loads automatically

### Testing Tools (Python)
```bash
# Install dependency
pip install pygame

# Test triggers, rumble and buttons
Testar_Controle.bat

# Map and identify all buttons
Mapear_Controle.bat
```

---

## Configuration (UniversalController.ini)

```ini
[DualSense]
Enabled=1
LightbarHP=1
AdaptiveTriggers=1
HapticRumble=1
DPadMenu=1
ShareOpensMap=1
```

Edit `UniversalController.ini` to enable/disable individual features.

---

## Building from Source

Requires **CLEO SDK** and **MinGW** or **MSVC**:

```bash
gcc -shared -o UniversalController.asi UniversalController.c -ldinput8 -luser32
```

Or open in Visual Studio with the CLEO 4 SDK headers configured.

---

## Author

**carlostmj**
GTA modding enthusiast — PS5 DualSense integration for classic GTA titles.

- [gta5-mods](https://github.com/carlostmj/gta5-mods) — GTA V PC Singleplayer Mods (Ynix Trainer)

---

## License

MIT — free to use, modify and distribute with attribution.
