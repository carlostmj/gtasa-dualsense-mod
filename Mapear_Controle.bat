@echo off
title Mapeador Passo a Passo DualSense
cd /d "C:\Users\Carlos\.gemini\antigravity\brain\b1f856c8-3cb2-4343-8024-31abac071e1b\scratch"
python mapeador_controle.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Ocorreu um erro ao abrir o mapeador!
    pause
)
