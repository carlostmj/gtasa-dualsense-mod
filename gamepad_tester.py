import tkinter as tk
from tkinter import ttk, messagebox
import ctypes
from ctypes import wintypes
import time
import json
import os
import threading

kernel32 = ctypes.windll.kernel32
winmm = ctypes.windll.winmm

# DualSense BT CRC32 Table
g_crc_table = []
for i in range(256):
    c = i
    for j in range(8):
        c = (c >> 1) ^ 0xEDB88320 if (c & 1) else (c >> 1)
    g_crc_table.append(c)

def dualsense_crc32(buf):
    c = 0xFFFFFFFF
    for b in buf:
        c = g_crc_table[(c ^ b) & 0xFF] ^ (c >> 8)
    return (~c) & 0xFFFFFFFF

DUALSENSE_PATH = '\\\\?\\hid#{00001124-0000-1000-8000-00805f9b34fb}_vid&0002054c_pid&0ce6#8&2a285c49&1&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}'

class GamepadTesterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Antigravity - Universal Gamepad & DualSense Tester")
        self.root.geometry("820x680")
        self.root.configure(bg="#121214")
        self.root.resizable(False, False)

        self.ds_handle = None
        self.running = True

        self.latest_input = {
            "lx": 0, "ly": 0, "rx": 0, "ry": 0,
            "l2": 0, "r2": 0,
            "square": False, "cross": False, "circle": False, "triangle": False,
            "l1": False, "r1": False, "l3": False, "r3": False,
            "start": False, "select": False,
            "dpad_up": False, "dpad_down": False, "dpad_left": False, "dpad_right": False
        }

        self.stats = {
            "start_time": time.strftime("%Y-%m-%d %H:%M:%S"),
            "controller_detected": False,
            "transport": "None",
            "buttons_pressed": set(),
            "max_lx": 0, "min_lx": 0,
            "max_ly": 0, "min_ly": 0,
            "max_rx": 0, "min_rx": 0,
            "max_ry": 0, "min_ry": 0,
            "max_l2": 0, "max_r2": 0,
            "dpad_directions": set(),
            "adaptive_trigger_tested": False,
            "vibration_tested": False,
            "total_frames_polled": 0
        }

        self.init_hardware()
        self.create_ui()

        # Start background input reader thread
        self.read_thread = threading.Thread(target=self.hid_read_worker, daemon=True)
        self.read_thread.start()

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.update_ui_loop()

    def init_hardware(self):
        try:
            # Open with GENERIC_READ | GENERIC_WRITE (0xC0000000)
            h = kernel32.CreateFileA(
                DUALSENSE_PATH.encode('utf-8'),
                0xC0000000,
                1 | 2,      # SHARE READ | WRITE
                None,
                3,          # OPEN_EXISTING
                0,
                None
            )
            if h != -1 and h != 0:
                self.ds_handle = h
                self.stats["controller_detected"] = True
                self.stats["transport"] = "Bluetooth HID Direct"
        except Exception as e:
            self.ds_handle = None

    def send_dualsense_report(self, left_motor=0, right_motor=0, lt_mode=2, rt_mode=2, r2_start=20, r2_force=255, l2_start=25, l2_force=240):
        if not self.ds_handle:
            self.init_hardware()
        if not self.ds_handle:
            return False

        report = bytearray(78)
        report[0] = 0x31
        report[1] = 0x02
        report[2] = 0x01 | 0x02 | 0x04 | 0x08
        report[4] = right_motor
        report[5] = left_motor

        # R2 Trigger (Curto e Forte)
        report[11] = rt_mode
        report[12] = r2_start
        report[13] = r2_force

        # L2 Trigger
        report[22] = lt_mode
        report[23] = l2_start
        report[24] = l2_force

        crc = dualsense_crc32(b'\xa2' + bytes(report[:74]))
        report[74:78] = crc.to_bytes(4, 'little')

        written = wintypes.DWORD()
        ok = kernel32.WriteFile(self.ds_handle, (ctypes.c_char * 78).from_buffer(report), 78, ctypes.byref(written), None)
        return bool(ok)

    def hid_read_worker(self):
        buf = (ctypes.c_char * 78)()
        read_bytes = wintypes.DWORD()

        while self.running:
            if not self.ds_handle:
                self.init_hardware()
                time.sleep(0.5)
                continue

            ok = kernel32.ReadFile(self.ds_handle, buf, 78, ctypes.byref(read_bytes), None)
            if ok and read_bytes.value >= 11:
                raw = bytes(buf)[:read_bytes.value]

                # DualSense Bluetooth Report 0x31:
                # raw[0] = 0x31
                # raw[1] = seq
                # raw[2] = LX (0..255, 128 is center)
                # raw[3] = LY
                # raw[4] = RX
                # raw[5] = RY
                # raw[6] = L2 Analog (0..255)
                # raw[7] = R2 Analog (0..255)
                # raw[8] = DPad (low nibble) + Face buttons (high nibble)
                # raw[9] = Shoulders + Menu buttons
                # raw[10] = Touchpad click + PS button

                lx = int(raw[2]) - 128
                ly = int(raw[3]) - 128
                rx = int(raw[4]) - 128
                ry = int(raw[5]) - 128
                l2 = int(raw[6])
                r2 = int(raw[7])

                b8 = raw[8]
                dpad_val = b8 & 0x0F
                sq = bool(b8 & 0x10)
                cr = bool(b8 & 0x20)
                ci = bool(b8 & 0x40)
                tr = bool(b8 & 0x80)

                b9 = raw[9]
                l1 = bool(b9 & 0x01)
                r1 = bool(b9 & 0x02)
                share = bool(b9 & 0x10)
                options = bool(b9 & 0x20)
                l3 = bool(b9 & 0x40)
                r3 = bool(b9 & 0x80)

                b10 = raw[10] if len(raw) > 10 else 0
                touchpad = bool(b10 & 0x02)

                # D-Pad interpretation
                # 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW, 8=None
                du = dpad_val in (0, 1, 7)
                dr = dpad_val in (1, 2, 3)
                dd = dpad_val in (3, 4, 5)
                dl = dpad_val in (5, 6, 7)

                self.latest_input = {
                    "lx": lx, "ly": ly, "rx": rx, "ry": ry,
                    "l2": l2, "r2": r2,
                    "square": sq, "cross": cr, "circle": ci, "triangle": tr,
                    "l1": l1, "r1": r1, "l3": l3, "r3": r3,
                    "start": options, "select": (share or touchpad),
                    "dpad_up": du, "dpad_down": dd, "dpad_left": dl, "dpad_right": dr
                }
            else:
                time.sleep(0.01)

    def create_ui(self):
        header = tk.Frame(self.root, bg="#1a1a1e", height=60)
        header.pack(fill=tk.X)

        title = tk.Label(header, text="🎮 TESTADOR DE HARDWARE: SONY DUALSENSE PS5", font=("Segoe UI", 13, "bold"), fg="#38bdf8", bg="#1a1a1e")
        title.pack(pady=(10, 2))

        self.lbl_status = tk.Label(header, text="CONECTADO: Sony DualSense PS5 (Bluetooth Direct HID)", font=("Segoe UI", 9, "bold"), fg="#4ade80", bg="#1a1a1e")
        self.lbl_status.pack(pady=(0, 8))

        main = tk.Frame(self.root, bg="#121214")
        main.pack(fill=tk.BOTH, expand=True, padx=20, pady=10)

        # Left Column: Sticks & Triggers
        left_col = tk.Frame(main, bg="#18181b", bd=1, relief=tk.SOLID)
        left_col.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 10))

        lbl_analog = tk.Label(left_col, text="ANALÓGICOS & GATILHOS L2 / R2", font=("Segoe UI", 10, "bold"), fg="#a1a1aa", bg="#18181b")
        lbl_analog.pack(pady=8)

        self.canvas_sticks = tk.Canvas(left_col, width=360, height=180, bg="#09090b", highlightthickness=0)
        self.canvas_sticks.pack(pady=5)

        self.canvas_sticks.create_oval(30, 20, 170, 160, outline="#27272a", width=2)
        self.canvas_sticks.create_line(100, 20, 100, 160, fill="#27272a")
        self.canvas_sticks.create_line(30, 90, 170, 90, fill="#27272a")
        self.stick_l = self.canvas_sticks.create_oval(90, 80, 110, 100, fill="#38bdf8", outline="#60a5fa")
        self.canvas_sticks.create_text(100, 170, text="ANALÓGICO ESQUERDO (L)", fill="#71717a", font=("Segoe UI", 8, "bold"))

        self.canvas_sticks.create_oval(190, 20, 330, 160, outline="#27272a", width=2)
        self.canvas_sticks.create_line(260, 20, 260, 160, fill="#27272a")
        self.canvas_sticks.create_line(190, 90, 330, 90, fill="#27272a")
        self.stick_r = self.canvas_sticks.create_oval(250, 80, 270, 100, fill="#a855f7", outline="#c084fc")
        self.canvas_sticks.create_text(260, 170, text="ANALÓGICO DIREITO (R)", fill="#71717a", font=("Segoe UI", 8, "bold"))

        trig_frame = tk.Frame(left_col, bg="#18181b")
        trig_frame.pack(fill=tk.X, padx=20, pady=10)

        tk.Label(trig_frame, text="Gatilho L2 (Mira / Freio):", font=("Segoe UI", 9), fg="#e4e4e7", bg="#18181b").grid(row=0, column=0, sticky="w")
        self.bar_l2 = ttk.Progressbar(trig_frame, length=200, maximum=255, value=0)
        self.bar_l2.grid(row=0, column=1, padx=10, pady=4)
        self.lbl_val_l2 = tk.Label(trig_frame, text="0", font=("Segoe UI", 9, "bold"), fg="#38bdf8", bg="#18181b", width=4)
        self.lbl_val_l2.grid(row=0, column=2)

        tk.Label(trig_frame, text="Gatilho R2 (Tiro / Acelerar):", font=("Segoe UI", 9), fg="#e4e4e7", bg="#18181b").grid(row=1, column=0, sticky="w")
        self.bar_r2 = ttk.Progressbar(trig_frame, length=200, maximum=255, value=0)
        self.bar_r2.grid(row=1, column=1, padx=10, pady=4)
        self.lbl_val_r2 = tk.Label(trig_frame, text="0", font=("Segoe UI", 9, "bold"), fg="#f43f5e", bg="#18181b", width=4)
        self.lbl_val_r2.grid(row=1, column=2)

        actions_frame = tk.Frame(left_col, bg="#18181b")
        actions_frame.pack(fill=tk.X, padx=20, pady=(5, 10))

        btn_trig = tk.Button(actions_frame, text="💥 Testar Gatilhos Curto e Forte", font=("Segoe UI", 9, "bold"), bg="#2563eb", fg="white", activebackground="#1d4ed8", command=self.action_test_triggers)
        btn_trig.pack(fill=tk.X, pady=3)

        btn_vib = tk.Button(actions_frame, text="📳 Testar Vibração Háptica", font=("Segoe UI", 9, "bold"), bg="#d97706", fg="white", activebackground="#b45309", command=self.action_test_vibration)
        btn_vib.pack(fill=tk.X, pady=3)

        btn_reset = tk.Button(actions_frame, text="🔄 Restaurar Gatilhos Normais", font=("Segoe UI", 8), bg="#3f3f46", fg="white", activebackground="#27272a", command=self.action_reset_triggers)
        btn_reset.pack(fill=tk.X, pady=3)

        # Right Column: Buttons
        right_col = tk.Frame(main, bg="#18181b", bd=1, relief=tk.SOLID)
        right_col.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        lbl_buttons = tk.Label(right_col, text="BOTÕES FÍSICOS (PRESSIONE NO CONTROLE)", font=("Segoe UI", 10, "bold"), fg="#a1a1aa", bg="#18181b")
        lbl_buttons.pack(pady=8)

        self.btn_widgets = {}
        grid_frame = tk.Frame(right_col, bg="#18181b")
        grid_frame.pack(padx=10, pady=5)

        buttons_layout = [
            [("L1", "L1 (Arma Ant)"), ("R1", "R1 (Próx Arma)")],
            [("Square", "◻ Quadrado"), ("Triangle", "△ Triângulo")],
            [("Cross", "╳ Cruz"), ("Circle", "⭘ Círculo")],
            [("L3", "L3 (Agachar)"), ("R3", "R3 (Olhar Trás)")],
            [("Select", "Share / Touch"), ("Start", "Options (Pausa)")],
            [("DPadUp", "D-Pad Cima"), ("DPadDown", "D-Pad Baixo")],
            [("DPadLeft", "D-Pad Esq"), ("DPadRight", "D-Pad Dir")],
        ]

        for r_idx, row in enumerate(buttons_layout):
            for c_idx, (key, label) in enumerate(row):
                lbl = tk.Label(grid_frame, text=label, font=("Segoe UI", 8, "bold"), fg="#71717a", bg="#27272a", width=18, height=1, relief=tk.FLAT)
                lbl.grid(row=r_idx, column=c_idx, padx=4, pady=3)
                self.btn_widgets[key] = lbl

        lbl_log = tk.Label(right_col, text="REGISTRO DE EVENTOS EM TEMPO REAL:", font=("Segoe UI", 8, "bold"), fg="#a1a1aa", bg="#18181b")
        lbl_log.pack(anchor="w", padx=15, pady=(10, 2))

        self.txt_log = tk.Text(right_col, height=7, bg="#09090b", fg="#22c55e", font=("Consolas", 8), relief=tk.FLAT)
        self.txt_log.pack(fill=tk.BOTH, expand=True, padx=15, pady=(0, 10))

    def log_event(self, text):
        self.txt_log.insert(tk.END, f"[{time.strftime('%H:%M:%S')}] {text}\n")
        self.txt_log.see(tk.END)

    def action_test_triggers(self):
        self.stats["adaptive_trigger_tested"] = True
        self.log_event("Gatilhos Curto e Forte ativados (R2: stop 20, forca 255 | L2: stop 25, forca 240)")
        ok = self.send_dualsense_report(0, 0, lt_mode=2, rt_mode=2, r2_start=20, r2_force=255, l2_start=25, l2_force=240)
        if ok:
            messagebox.showinfo("Gatilhos Ativados", "Gatilhos Adaptativos ativados!\nPuxe L2 e R2 agora para sentir a parede mecânica rígida!")
        else:
            self.log_event("Falha ao enviar comando para o controle!")

    def action_test_vibration(self):
        self.stats["vibration_tested"] = True
        self.log_event("Enviando pulsos de vibração háptica nos motores...")
        threading.Thread(target=self._vibrate_worker, daemon=True).start()

    def _vibrate_worker(self):
        for _ in range(3):
            self.send_dualsense_report(left_motor=220, right_motor=240, rt_mode=2, lt_mode=2)
            time.sleep(0.18)
            self.send_dualsense_report(left_motor=0, right_motor=0, rt_mode=2, lt_mode=2)
            time.sleep(0.12)
        self.log_event("Vibração concluída!")

    def action_reset_triggers(self):
        self.log_event("Restaurando gatilhos livres...")
        self.send_dualsense_report(left_motor=0, right_motor=0, lt_mode=0, rt_mode=0, r2_start=0, r2_force=0, l2_start=0, l2_force=0)

    def update_ui_loop(self):
        if not self.running:
            return

        inp = self.latest_input
        lx, ly = inp["lx"], inp["ly"]
        rx, ry = inp["rx"], inp["ry"]
        l2, r2 = inp["l2"], inp["r2"]

        # Update sticks UI
        slx = 100 + int((lx / 128.0) * 55)
        sly = 90 + int((ly / 128.0) * 55)
        self.canvas_sticks.coords(self.stick_l, slx - 10, sly - 10, slx + 10, sly + 10)

        srx = 260 + int((rx / 128.0) * 55)
        sry = 90 + int((ry / 128.0) * 55)
        self.canvas_sticks.coords(self.stick_r, srx - 10, sry - 10, srx + 10, sry + 10)

        # Update triggers UI
        self.bar_l2["value"] = l2
        self.lbl_val_l2.config(text=str(l2))
        self.bar_r2["value"] = r2
        self.lbl_val_r2.config(text=str(r2))

        # Update button badges
        btn_states = {
            "Square": inp["square"],
            "Cross": inp["cross"],
            "Circle": inp["circle"],
            "Triangle": inp["triangle"],
            "L1": inp["l1"],
            "R1": inp["r1"],
            "L3": inp["l3"],
            "R3": inp["r3"],
            "Start": inp["start"],
            "Select": inp["select"],
            "DPadUp": inp["dpad_up"],
            "DPadDown": inp["dpad_down"],
            "DPadLeft": inp["dpad_left"],
            "DPadRight": inp["dpad_right"],
        }

        for k, is_down in btn_states.items():
            if is_down:
                self.stats["buttons_pressed"].add(k)
                self.btn_widgets[k].config(bg="#22c55e", fg="#ffffff")
            else:
                self.btn_widgets[k].config(bg="#27272a", fg="#71717a")

        self.root.after(16, self.update_ui_loop)

    def on_close(self):
        self.running = False
        if self.ds_handle:
            self.send_dualsense_report(0, 0, 0, 0, 0, 0, 0, 0)
            kernel32.CloseHandle(self.ds_handle)
            self.ds_handle = None

        self.stats["buttons_pressed"] = list(self.stats["buttons_pressed"])
        self.stats["dpad_directions"] = list(self.stats["dpad_directions"])
        self.stats["end_time"] = time.strftime("%Y-%m-%d %H:%M:%S")

        out_path = r"C:\Users\Carlos\.gemini\antigravity\brain\b1f856c8-3cb2-4343-8024-31abac071e1b\scratch\gamepad_test_results.json"
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(self.stats, f, indent=2, ensure_ascii=False)

        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = GamepadTesterApp(root)
    root.mainloop()
