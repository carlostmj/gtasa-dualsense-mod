import tkinter as tk
from tkinter import ttk, messagebox
import time
import threading
try:
    import hid
except ImportError:
    hid = None

# ─────────────────────────────────────────────────────────────────────────────
# DualSense BT CRC32
# ─────────────────────────────────────────────────────────────────────────────
_CRC_TABLE = []
for _i in range(256):
    _c = _i
    for _ in range(8):
        _c = (_c >> 1) ^ 0xEDB88320 if (_c & 1) else (_c >> 1)
    _CRC_TABLE.append(_c)

def _crc32(data: bytes) -> int:
    c = 0xFFFFFFFF
    for b in data:
        c = _CRC_TABLE[(c ^ b) & 0xFF] ^ (c >> 8)
    return (~c) & 0xFFFFFFFF

# ─────────────────────────────────────────────────────────────────────────────
# DualSense Hardware Driver
# ─────────────────────────────────────────────────────────────────────────────
SONY_VID = 0x054C
SONY_PIDS = {
    0x0CE6: "DualSense PS5 (Wireless/BT)",
    0x0DF2: "DualSense Edge PS5",
    0x05C4: "DualShock 4 v1",
    0x09CC: "DualShock 4 v2"
}

class DualSenseDriver:
    def __init__(self):
        self.dev = None
        self.is_bt = False
        self.pid = 0
        self.name = "Nenhum controle detectado"
        self.transport = "—"

    def connect(self) -> bool:
        if not hid:
            return False
        try:
            devs = hid.enumerate(SONY_VID)
            for d in devs:
                pid = d.get('product_id', 0)
                if pid in SONY_PIDS:
                    try:
                        dev = hid.device()
                        dev.open_path(d['path'])
                        dev.set_nonblocking(True)
                        self.dev = dev
                        self.pid = pid
                        self.name = SONY_PIDS.get(pid, "Sony Controller")
                        # 0x0CE6 over BT has report 0x31 (78 bytes)
                        self.is_bt = (pid == 0x0CE6 and d.get('interface_number', -1) == -1)
                        self.transport = "Bluetooth" if self.is_bt else "USB / Direct"
                        return True
                    except Exception:
                        pass
        except Exception:
            pass
        return False

    def read(self) -> bytes | None:
        if not self.dev:
            return None
        try:
            data = self.dev.read(100)
            return bytes(data) if data else None
        except Exception:
            return None

    def send_output(self, *, left_motor=0, right_motor=0,
                    r2_mode=2, r2_start=20, r2_force=255,
                    l2_mode=2, l2_start=25, l2_force=240,
                    r=0, g=120, b=255):
        if not self.dev:
            return False
        try:
            if self.is_bt:
                report = bytearray(78)
                report[0]  = 0x31
                report[1]  = 0x02
                report[2]  = 0xFF
                report[3]  = 0x57
                report[4]  = right_motor
                report[5]  = left_motor
                report[11] = r2_mode
                report[12] = 0x02
                report[13] = r2_start
                report[14] = r2_force
                report[22] = l2_mode
                report[23] = 0x02
                report[24] = l2_start
                report[25] = l2_force
                report[40] = 0x02
                report[44] = 0x00
                report[45] = 0x04
                report[46] = r
                report[47] = g
                report[48] = b
                crc = _crc32(b'\xa2' + bytes(report[:74]))
                report[74] = crc & 0xFF
                report[75] = (crc >> 8) & 0xFF
                report[76] = (crc >> 16) & 0xFF
                report[77] = (crc >> 24) & 0xFF
                self.dev.write(bytes(report))
            else:
                report = bytearray(48)
                report[0]  = 0x02
                report[1]  = 0xFF
                report[2]  = 0x57
                report[3]  = right_motor
                report[4]  = left_motor
                report[10] = r2_mode
                report[11] = 0x02
                report[12] = r2_start
                report[13] = r2_force
                report[21] = l2_mode
                report[22] = 0x02
                report[23] = l2_start
                report[24] = l2_force
                report[39] = 0x02
                report[43] = 0x00
                report[44] = 0x04
                report[45] = r
                report[46] = g
                report[47] = b
                self.dev.write(bytes([0x02]) + bytes(report))
            return True
        except Exception:
            return False

    def parse(self, raw: bytes) -> dict:
        if not raw or len(raw) < 10:
            return {}
        bt = (raw[0] == 0x31)
        base = 2 if bt else 1
        def byte(i):
            idx = base + i
            return raw[idx] if idx < len(raw) else 0

        lx = byte(0) - 128
        ly = byte(1) - 128
        rx = byte(2) - 128
        ry = byte(3) - 128
        l2 = byte(4)
        r2 = byte(5)

        b0 = byte(7)
        b1 = byte(8)
        b2 = byte(9)

        dpad = b0 & 0x0F
        square   = bool(b0 & 0x10)
        cross    = bool(b0 & 0x20)
        circle   = bool(b0 & 0x40)
        triangle = bool(b0 & 0x80)

        l1      = bool(b1 & 0x01)
        r1      = bool(b1 & 0x02)
        l2d     = bool(b1 & 0x04)
        r2d     = bool(b1 & 0x08)
        share   = bool(b1 & 0x10)
        options = bool(b1 & 0x20)
        l3      = bool(b1 & 0x40)
        r3      = bool(b1 & 0x80)

        ps_btn  = bool(b2 & 0x01)
        touch   = bool(b2 & 0x02)
        mute    = bool(b2 & 0x04)

        # Touchpad XY
        tp_base = base + 31
        tp0 = raw[tp_base] if tp_base < len(raw) else 0
        tp1 = raw[tp_base+1] if tp_base+1 < len(raw) else 0
        tp2 = raw[tp_base+2] if tp_base+2 < len(raw) else 0
        tp_x = ((tp1 & 0x0F) << 8) | tp0
        tp_y = (tp2 << 4) | ((tp1 & 0xF0) >> 4)

        return {
            "lx": lx, "ly": ly, "rx": rx, "ry": ry,
            "l2": l2, "r2": r2,
            "square": square, "cross": cross, "circle": circle, "triangle": triangle,
            "l1": l1, "r1": r1, "l2d": l2d, "r2d": r2d,
            "l3": l3, "r3": r3,
            "share": share, "options": options,
            "ps": ps_btn, "touch": touch, "mute": mute,
            "dpad_up":    dpad in (0, 1, 7),
            "dpad_right": dpad in (1, 2, 3),
            "dpad_down":  dpad in (3, 4, 5),
            "dpad_left":  dpad in (5, 6, 7),
            "tp_x": tp_x, "tp_y": tp_y,
            "bt": bt
        }

    def close(self):
        if self.dev:
            try:
                self.dev.close()
            except Exception:
                pass
            self.dev = None

# ─────────────────────────────────────────────────────────────────────────────
# UI Application
# ─────────────────────────────────────────────────────────────────────────────
C_BG     = "#0f0f13"
C_PANEL  = "#181820"
C_BORDER = "#272736"
C_TEXT   = "#e2e2f0"
C_DIM    = "#64748b"
C_BLUE   = "#38bdf8"
C_GREEN  = "#22c55e"
C_RED    = "#f43f5e"
C_YELLOW = "#facc15"
C_PURPLE = "#c084fc"

class GamepadTesterApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Antigravity — Testador Profissional de Controle (DualSense / PS4)")
        self.root.configure(bg=C_BG)
        self.root.geometry("880x640")
        self.root.resizable(False, False)

        self.driver = DualSenseDriver()
        self.running = True
        self.inp = {}

        self._build_ui()
        self._connect()

        self._thread = threading.Thread(target=self._worker, daemon=True)
        self._thread.start()

        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self._update_loop()

    def _connect(self):
        ok = self.driver.connect()
        if ok:
            self.lbl_status.config(
                text=f"🟢 CONECTADO: {self.driver.name} [{self.driver.transport}]",
                fg=C_GREEN)
            self.driver.send_output()
        else:
            self.lbl_status.config(text="🔴 Nenhum controle detectado — Conecte via Bluetooth ou Cabo USB", fg=C_RED)

    def _worker(self):
        while self.running:
            if not self.driver.dev:
                time.sleep(1.0)
                self._connect()
                continue
            raw = self.driver.read()
            if raw:
                self.inp = self.driver.parse(raw)
            else:
                time.sleep(0.01)

    def _build_ui(self):
        hdr = tk.Frame(self.root, bg="#13131c", pady=8)
        hdr.pack(fill=tk.X)
        tk.Label(hdr, text="🎮 TESTADOR DE CONTROLE — SONY DUALSENSE PS5 / PS4",
                 font=("Segoe UI", 12, "bold"), fg=C_BLUE, bg="#13131c").pack()
        self.lbl_status = tk.Label(hdr, text="Detectando hardware...",
                                   font=("Segoe UI", 9, "bold"), fg=C_DIM, bg="#13131c")
        self.lbl_status.pack()

        body = tk.Frame(self.root, bg=C_BG)
        body.pack(fill=tk.BOTH, expand=True, padx=12, pady=8)
        body.columnconfigure(0, weight=1)
        body.columnconfigure(1, weight=1)
        body.rowconfigure(0, weight=1)

        # Left Column: Analogs, Triggers, Test Buttons
        left = tk.LabelFrame(body, text=" ANALÓGICOS & GATILHOS ADAPTATIVOS ",
                             font=("Segoe UI", 9, "bold"), fg=C_DIM, bg=C_PANEL,
                             bd=1, relief=tk.SOLID, labelanchor="n")
        left.grid(row=0, column=0, sticky="nsew", padx=6, pady=4)

        cv = tk.Canvas(left, width=380, height=180, bg="#09090f", highlightthickness=0)
        cv.pack(pady=8)
        self._cv = cv

        # Left stick
        cv.create_oval(30, 20, 170, 160, outline=C_BORDER, width=2)
        cv.create_line(100, 20, 100, 160, fill=C_BORDER)
        cv.create_line(30, 90, 170, 90, fill=C_BORDER)
        self._sl = cv.create_oval(90, 80, 110, 100, fill=C_BLUE, outline="#60a5fa", width=2)
        cv.create_text(100, 170, text="ANALÓGICO ESQ (L)", fill=C_DIM, font=("Segoe UI", 8, "bold"))
        self._lbl_ls = cv.create_text(100, 15, text="0, 0", fill=C_DIM, font=("Consolas", 8))

        # Right stick
        cv.create_oval(210, 20, 350, 160, outline=C_BORDER, width=2)
        cv.create_line(280, 20, 280, 160, fill=C_BORDER)
        cv.create_line(210, 90, 350, 90, fill=C_BORDER)
        self._sr = cv.create_oval(270, 80, 290, 100, fill=C_PURPLE, outline="#c084fc", width=2)
        cv.create_text(280, 170, text="ANALÓGICO DIR (R)", fill=C_DIM, font=("Segoe UI", 8, "bold"))
        self._lbl_rs = cv.create_text(280, 15, text="0, 0", fill=C_DIM, font=("Consolas", 8))

        # Triggers
        trig = tk.Frame(left, bg=C_PANEL)
        trig.pack(fill=tk.X, padx=16, pady=6)

        tk.Label(trig, text="L2 (Mira / Freio):", font=("Segoe UI", 8, "bold"),
                 fg=C_TEXT, bg=C_PANEL, width=18, anchor="w").grid(row=0, column=0)
        self._bar_l2 = ttk.Progressbar(trig, length=180, maximum=255)
        self._bar_l2.grid(row=0, column=1, padx=6, pady=3)
        self._lbl_l2 = tk.Label(trig, text="0", font=("Consolas", 9, "bold"), fg=C_BLUE, bg=C_PANEL, width=4)
        self._lbl_l2.grid(row=0, column=2)

        tk.Label(trig, text="R2 (Tiro / Acelerar):", font=("Segoe UI", 8, "bold"),
                 fg=C_TEXT, bg=C_PANEL, width=18, anchor="w").grid(row=1, column=0)
        self._bar_r2 = ttk.Progressbar(trig, length=180, maximum=255)
        self._bar_r2.grid(row=1, column=1, padx=6, pady=3)
        self._lbl_r2 = tk.Label(trig, text="0", font=("Consolas", 9, "bold"), fg=C_RED, bg=C_PANEL, width=4)
        self._lbl_r2.grid(row=1, column=2)

        # Action test buttons
        act = tk.Frame(left, bg=C_PANEL)
        act.pack(fill=tk.X, padx=16, pady=8)
        tk.Button(act, text="💥 Testar Gatilho Curto e Forte (R2 Parede Mecânica)",
                  font=("Segoe UI", 9, "bold"), bg="#1d4ed8", fg="white", relief=tk.FLAT, pady=5,
                  command=self._test_triggers).pack(fill=tk.X, pady=3)
        tk.Button(act, text="📳 Testar Vibração Háptica (Motores DualSense)",
                  font=("Segoe UI", 9, "bold"), bg="#d97706", fg="white", relief=tk.FLAT, pady=5,
                  command=self._test_vibration).pack(fill=tk.X, pady=3)
        tk.Button(act, text="🔄 Restaurar Gatilhos Livres",
                  font=("Segoe UI", 8), bg="#334155", fg="white", relief=tk.FLAT, pady=4,
                  command=self._reset_triggers).pack(fill=tk.X, pady=3)

        # Right Column: Buttons & Extras
        right = tk.LabelFrame(body, text=" BOTÕES FÍSICOS & RECURSOS ",
                              font=("Segoe UI", 9, "bold"), fg=C_DIM, bg=C_PANEL,
                              bd=1, relief=tk.SOLID, labelanchor="n")
        right.grid(row=0, column=1, sticky="nsew", padx=6, pady=4)

        grid = tk.Frame(right, bg=C_PANEL)
        grid.pack(padx=12, pady=6)

        layout = [
            [("l1", "L1 (Arma Ant)"), ("r1", "R1 (Próx Arma)")],
            [("l2d", "L2 Digital"), ("r2d", "R2 Digital")],
            [("square", "◻ Quadrado"), ("triangle", "△ Triângulo")],
            [("cross", "✕ Cruz"), ("circle", "○ Círculo")],
            [("l3", "L3 (Agachar)"), ("r3", "R3 (Olhar Trás)")],
            [("share", "Share / Create"), ("options", "Options (Pause)")],
            [("ps", "🔵 Botão PS"), ("touch", "Touchpad Click")],
            [("mute", "Mute"), ("tp_pos", "Touch: —")],
            [("dpad_up", "D-Pad ↑"), ("dpad_down", "D-Pad ↓")],
            [("dpad_left", "D-Pad ←"), ("dpad_right", "D-Pad →")],
        ]

        self._btn_w = {}
        for r, row in enumerate(layout):
            for c, (key, label) in enumerate(row):
                lbl = tk.Label(grid, text=label, font=("Segoe UI", 8, "bold"),
                               fg=C_DIM, bg=C_BORDER, width=18, height=1, relief=tk.FLAT, pady=3)
                lbl.grid(row=r, column=c, padx=3, pady=2)
                self._btn_w[key] = lbl

        # Log
        tk.Label(right, text="REGISTRO DE EVENTOS:", font=("Segoe UI", 8, "bold"),
                 fg=C_DIM, bg=C_PANEL).pack(anchor="w", padx=12, pady=(6, 2))
        self._log = tk.Text(right, height=6, bg="#09090f", fg=C_GREEN,
                            font=("Consolas", 8), relief=tk.FLAT, state=tk.DISABLED)
        self._log.pack(fill=tk.BOTH, expand=True, padx=12, pady=(0, 8))

    def _log_msg(self, msg):
        self._log.config(state=tk.NORMAL)
        self._log.insert(tk.END, f"[{time.strftime('%H:%M:%S')}] {msg}\n")
        self._log.see(tk.END)
        self._log.config(state=tk.DISABLED)

    def _test_triggers(self):
        self._log_msg("Gatilho R2: Batente mecânico curto ativado!")
        self.driver.send_output(r2_mode=2, r2_start=20, r2_force=255,
                                l2_mode=2, l2_start=25, l2_force=240)
        messagebox.showinfo("Gatilhos Ativados", "Gatilho R2 bloqueado como batente mecânico curto!\nPuxe o R2 agora para sentir a resistência.")

    def _test_vibration(self):
        self._log_msg("Pulsos de vibração háptica...")
        def _v():
            for _ in range(3):
                self.driver.send_output(left_motor=220, right_motor=240)
                time.sleep(0.18)
                self.driver.send_output(left_motor=0, right_motor=0)
                time.sleep(0.12)
            self._log_msg("Vibração concluída!")
        threading.Thread(target=_v, daemon=True).start()

    def _reset_triggers(self):
        self._log_msg("Gatilhos livres restaurados.")
        self.driver.send_output(r2_mode=0, r2_start=0, r2_force=0,
                                l2_mode=0, l2_start=0, l2_force=0)

    _prev_pressed = set()

    def _update_loop(self):
        if not self.running:
            return
        inp = self.inp
        if inp:
            lx, ly = inp.get("lx", 0), inp.get("ly", 0)
            rx, ry = inp.get("rx", 0), inp.get("ry", 0)
            l2, r2 = inp.get("l2", 0), inp.get("r2", 0)

            slx = 100 + int((lx / 128.0) * 55)
            sly = 90 + int((ly / 128.0) * 55)
            self._cv.coords(self._sl, slx - 9, sly - 9, slx + 9, sly + 9)
            self._cv.itemconfig(self._lbl_ls, text=f"{lx:+4d}, {ly:+4d}")

            srx = 280 + int((rx / 128.0) * 55)
            sry = 90 + int((ry / 128.0) * 55)
            self._cv.coords(self._sr, srx - 9, sry - 9, srx + 9, sry + 9)
            self._cv.itemconfig(self._lbl_rs, text=f"{rx:+4d}, {ry:+4d}")

            self._bar_l2["value"] = l2
            self._lbl_l2.config(text=str(l2))
            self._bar_r2["value"] = r2
            self._lbl_r2.config(text=str(r2))

            curr_pressed = set()
            for k, w in self._btn_w.items():
                if k == "tp_pos":
                    if inp.get("touch"):
                        w.config(text=f"X={inp.get('tp_x',0)} Y={inp.get('tp_y',0)}", bg=C_BLUE, fg="white")
                    else:
                        w.config(text="Touch: —", bg=C_BORDER, fg=C_DIM)
                    continue
                if inp.get(k, False):
                    curr_pressed.add(k)
                    w.config(bg=C_GREEN, fg="white")
                else:
                    w.config(bg=C_BORDER, fg=C_DIM)

            new_keys = curr_pressed - self._prev_pressed
            for k in new_keys:
                self._log_msg(f"Pressionado: {k.upper()}")
            self._prev_pressed = curr_pressed

        self.root.after(16, self._update_loop)

    def _on_close(self):
        self.running = False
        self.driver.close()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = GamepadTesterApp(root)
    root.mainloop()
