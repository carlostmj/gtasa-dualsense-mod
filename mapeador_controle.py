import tkinter as tk
from tkinter import ttk, messagebox
import ctypes
from ctypes import wintypes
import time
import json
import os
import threading

kernel32 = ctypes.windll.kernel32

DUALSENSE_PATH = '\\\\?\\hid#{00001124-0000-1000-8000-00805f9b34fb}_vid&0002054c_pid&0ce6#8&2a285c49&1&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}'

STEPS = [
    {"id": "cross", "name": "╳  CRUZ (X do PlayStation)", "type": "button", "hint": "Pressione o botão ╳ (Cruz / A)"},
    {"id": "circle", "name": "⭘  CÍRCULO (Bola do PlayStation)", "type": "button", "hint": "Pressione o botão ⭘ (Círculo / B)"},
    {"id": "square", "name": "◻  QUADRADO", "type": "button", "hint": "Pressione o botão ◻ (Quadrado / X)"},
    {"id": "triangle", "name": "△  TRIÂNGULO", "type": "button", "hint": "Pressione o botão △ (Triângulo / Y)"},
    {"id": "l1", "name": "L1  (Botão de Ombro Esquerdo)", "type": "button", "hint": "Pressione o botão L1"},
    {"id": "r1", "name": "R1  (Botão de Ombro Direito)", "type": "button", "hint": "Pressione o botão R1"},
    {"id": "l2", "name": "L2  (Gatilho Esquerdo)", "type": "trigger", "hint": "Puxe o gatilho L2 até o final"},
    {"id": "r2", "name": "R2  (Gatilho Direito)", "type": "trigger", "hint": "Puxe o gatilho R2 até o final"},
    {"id": "dpad_up", "name": "D-PAD CIMA (Seta para Cima)", "type": "dpad_or_btn", "hint": "Pressione a seta para CIMA"},
    {"id": "dpad_down", "name": "D-PAD BAIXO (Seta para Baixo)", "type": "dpad_or_btn", "hint": "Pressione a seta para BAIXO"},
    {"id": "dpad_left", "name": "D-PAD ESQUERDA (Seta para Esquerda)", "type": "dpad_or_btn", "hint": "Pressione a seta para ESQUERDA"},
    {"id": "dpad_right", "name": "D-PAD DIREITA (Seta para Direita)", "type": "dpad_or_btn", "hint": "Pressione a seta para DIREITA"},
    {"id": "l3", "name": "L3  (Clique do Analógico Esquerdo)", "type": "button", "hint": "Pressione para baixo o analógico esquerdo (clique L3)"},
    {"id": "r3", "name": "R3  (Clique do Analógico Direito)", "type": "button", "hint": "Pressione para baixo o analógico direito (clique R3)"},
    {"id": "start", "name": "OPTIONS / START (Menu)", "type": "button", "hint": "Pressione o botão Options / Start"},
    {"id": "select", "name": "SHARE / TOUCHPAD (Câmera / Mapa)", "type": "button", "hint": "Pressione o botão Share / Create ou clique no Touchpad"},
    {"id": "stick_lx", "name": "ANALÓGICO ESQUERDO (Mover para Direita)", "type": "axis", "hint": "Empurre o analógico esquerdo todo para a DIREITA"},
    {"id": "stick_ly", "name": "ANALÓGICO ESQUERDO (Mover para Frente)", "type": "axis", "hint": "Empurre o analógico esquerdo todo para FRENTE"},
    {"id": "stick_rx", "name": "ANALÓGICO DIREITO (Mover para Direita)", "type": "axis", "hint": "Empurre o analógico direito todo para a DIREITA"},
    {"id": "stick_ry", "name": "ANALÓGICO DIREITO (Mover para Frente)", "type": "axis", "hint": "Empurre o analógico direito todo para FRENTE"}
]

class ControllerMapperWizard:
    def __init__(self, root):
        self.root = root
        self.root.title("Mapeador Automático de Controle - GTA San Andreas")
        self.root.geometry("680x480")
        self.root.configure(bg="#0f172a")
        self.root.resizable(False, False)

        self.ds_handle = None
        self.running = True
        self.current_step_idx = 0
        self.results = {}
        self.baseline_packet = None
        self.capture_ready = False

        self.init_device()
        self.create_ui()

        self.read_thread = threading.Thread(target=self.device_reader_thread, daemon=True)
        self.read_thread.start()

    def init_device(self):
        try:
            h = kernel32.CreateFileA(
                DUALSENSE_PATH.encode('utf-8'),
                0xC0000000, # READ | WRITE
                1 | 2,
                None,
                3,
                0,
                None
            )
            if h != -1 and h != 0:
                self.ds_handle = h
        except Exception:
            self.ds_handle = None

    def create_ui(self):
        header = tk.Frame(self.root, bg="#1e293b", height=70)
        header.pack(fill=tk.X)

        title = tk.Label(header, text="MAPEADOR DE CONTROLE: PASSO A PASSO", font=("Segoe UI", 13, "bold"), fg="#38bdf8", bg="#1e293b")
        title.pack(pady=(12, 2))

        sub = tk.Label(header, text="Aperte cada botão indicado no seu controle. O sistema grava os códigos automaticamente!", font=("Segoe UI", 8), fg="#94a3b8", bg="#1e293b")
        sub.pack(pady=(0, 10))

        # Main Card
        self.card = tk.Frame(self.root, bg="#1e293b", bd=2, relief=tk.RIDGE)
        self.card.pack(fill=tk.BOTH, expand=True, padx=30, pady=25)

        self.lbl_progress = tk.Label(self.card, text="", font=("Segoe UI", 9, "bold"), fg="#a855f7", bg="#1e293b")
        self.lbl_progress.pack(pady=(20, 5))

        self.lbl_action = tk.Label(self.card, text="", font=("Segoe UI", 16, "bold"), fg="#f8fafc", bg="#1e293b")
        self.lbl_action.pack(pady=5)

        self.lbl_hint = tk.Label(self.card, text="", font=("Segoe UI", 11), fg="#38bdf8", bg="#1e293b")
        self.lbl_hint.pack(pady=5)

        self.lbl_detected = tk.Label(self.card, text="Aguardando toque no controle...", font=("Consolas", 10, "bold"), fg="#e2e8f0", bg="#0f172a", width=50, height=2)
        self.lbl_detected.pack(pady=20)

        # Progress bar
        self.pbar = ttk.Progressbar(self.card, length=500, maximum=len(STEPS), value=0)
        self.pbar.pack(pady=10)

        # Bottom Controls
        bottom = tk.Frame(self.card, bg="#1e293b")
        bottom.pack(fill=tk.X, padx=20, pady=10)

        btn_skip = tk.Button(bottom, text="Pular este botão >>", font=("Segoe UI", 8), bg="#334155", fg="white", command=self.skip_step)
        btn_skip.pack(side=tk.RIGHT)

        self.show_step()

    def show_step(self):
        if self.current_step_idx >= len(STEPS):
            self.finish_mapping()
            return

        step = STEPS[self.current_step_idx]
        self.lbl_progress.config(text=f"PASSO {self.current_step_idx + 1} DE {len(STEPS)}")
        self.lbl_action.config(text=step["name"])
        self.lbl_hint.config(text=step["hint"])
        self.lbl_detected.config(text="Aguardando toque no controle...", fg="#94a3b8")
        self.pbar["value"] = self.current_step_idx

        # Let baseline settle
        self.capture_ready = False
        self.root.after(300, self.enable_capture)

    def enable_capture(self):
        self.capture_ready = True

    def skip_step(self):
        step = STEPS[self.current_step_idx]
        self.results[step["id"]] = None
        self.current_step_idx += 1
        self.show_step()

    def device_reader_thread(self):
        buf = (ctypes.c_char * 78)()
        read_bytes = wintypes.DWORD()

        while self.running:
            if not self.ds_handle:
                self.init_device()
                time.sleep(0.5)
                continue

            ok = kernel32.ReadFile(self.ds_handle, buf, 78, ctypes.byref(read_bytes), None)
            if ok and read_bytes.value >= 12:
                raw = list(bytes(buf)[:read_bytes.value])

                if self.baseline_packet is None or not self.capture_ready:
                    self.baseline_packet = raw
                    continue

                if self.current_step_idx >= len(STEPS):
                    time.sleep(0.1)
                    continue

                step = STEPS[self.current_step_idx]
                step_type = step["type"]

                # Detect changes in first 16 bytes (buttons, sticks, triggers)
                # Ignore gyro/accel (bytes >= 13)
                detected = None

                if step_type == "button":
                    for b_idx in (8, 9, 10, 11, 12):
                        if b_idx < len(raw):
                            xor = raw[b_idx] ^ self.baseline_packet[b_idx]
                            if xor != 0 and raw[b_idx] != 0:
                                detected = {
                                    "byte_index": b_idx,
                                    "bitmask": xor,
                                    "val": raw[b_idx],
                                    "baseline": self.baseline_packet[b_idx]
                                }
                                break

                elif step_type == "trigger":
                    for b_idx in (6, 7, 8, 9):
                        if b_idx < len(raw):
                            diff = raw[b_idx] - self.baseline_packet[b_idx]
                            if diff > 100:
                                detected = {
                                    "byte_index": b_idx,
                                    "val": raw[b_idx],
                                    "baseline": self.baseline_packet[b_idx]
                                }
                                break

                elif step_type == "dpad_or_btn":
                    for b_idx in (8, 9, 10):
                        if b_idx < len(raw):
                            if raw[b_idx] != self.baseline_packet[b_idx]:
                                detected = {
                                    "byte_index": b_idx,
                                    "val": raw[b_idx],
                                    "baseline": self.baseline_packet[b_idx]
                                }
                                break

                elif step_type == "axis":
                    for b_idx in (2, 3, 4, 5):
                        if b_idx < len(raw):
                            diff = abs(raw[b_idx] - self.baseline_packet[b_idx])
                            if diff > 40:
                                detected = {
                                    "byte_index": b_idx,
                                    "val": raw[b_idx],
                                    "baseline": self.baseline_packet[b_idx],
                                    "direction": "positive" if raw[b_idx] > self.baseline_packet[b_idx] else "negative"
                                }
                                break

                if detected and self.capture_ready:
                    self.capture_ready = False
                    self.results[step["id"]] = detected
                    self.root.after(0, lambda d=detected: self.on_detected(d))
                    time.sleep(0.4)

            else:
                time.sleep(0.01)

    def on_detected(self, d):
        self.lbl_detected.config(text=f"✔ RECONHECIDO! Byte [{d['byte_index']}] = 0x{d['val']:02X}", fg="#22c55e")
        self.root.after(400, self.next_step)

    def next_step(self):
        self.current_step_idx += 1
        self.show_step()

    def finish_mapping(self):
        self.running = False
        if self.ds_handle:
            kernel32.CloseHandle(self.ds_handle)
            self.ds_handle = None

        self.lbl_progress.config(text="MAPEAMENTO CONCLUÍDO!")
        self.lbl_action.config(text="🎉 Sucesso! Todos os botões gravados!")
        self.lbl_hint.config(text="O arquivo de configuração foi salvo automaticamente.")
        self.lbl_detected.config(text="Pode fechar esta janela agora!", fg="#38bdf8")

        # Save to JSON
        out_path = r"C:\Users\Carlos\.gemini\antigravity\brain\b1f856c8-3cb2-4343-8024-31abac071e1b\scratch\dualsense_hardware_map.json"
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(self.results, f, indent=2, ensure_ascii=False)

        messagebox.showinfo("Mapeamento Concluído", "Parabéns! O mapeamento foi salvo com sucesso!\nFeche o programa e o assistente irá aplicar tudo no seu jogo.")

if __name__ == "__main__":
    root = tk.Tk()
    app = ControllerMapperWizard(root)
    root.mainloop()
