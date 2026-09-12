#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// ============================================================================
// GTA San Andreas Engine Structures & Constants (v1.0 US)
// ============================================================================

typedef struct {
    signed short LeftStickX;          // 0x00 (-128 to 127)
    signed short LeftStickY;          // 0x02 (-128 to 127)
    signed short RightStickX;         // 0x04
    signed short RightStickY;         // 0x06
    signed short LeftShoulder1;       // 0x08 (L1)
    signed short LeftShoulder2;       // 0x0A (L2)
    signed short RightShoulder1;      // 0x0C (R1)
    signed short RightShoulder2;      // 0x0E (R2)
    signed short DPadUp;              // 0x10
    signed short DPadDown;            // 0x12
    signed short DPadLeft;            // 0x14
    signed short DPadRight;           // 0x16
    signed short Start;               // 0x18
    signed short Select;              // 0x1A
    signed short ButtonSquare;        // 0x1C
    signed short ButtonTriangle;      // 0x1E
    signed short ButtonCross;         // 0x20
    signed short ButtonCircle;        // 0x22
    signed short ShockButtonL;        // 0x24 (L3)
    signed short ShockButtonR;        // 0x26 (R3)
    signed short m_bChatIndicated;    // 0x28
    signed short m_bPedWalk;          // 0x2A
    signed short m_bVehicleMouseLook; // 0x2C
    signed short m_bRadioTrackSkip;   // 0x2E
} CControllerState;

typedef struct {
    CControllerState NewState;        // 0x00 (0x30 bytes)
    CControllerState OldState;        // 0x30 (0x30 bytes)
    short SteeringLeftRightBuffer[10];// 0x60
    int DrunkDrivingBufferUsed;       // 0x74
    CControllerState PCTempKeyState;  // 0x78
    CControllerState PCTempJoyState;  // 0xA8
    CControllerState PCTempMouseState;// 0xD8
    char Phase;                       // 0x108
    char _pad109;
    short Mode;                       // 0x10A
    short ShakeDur;                   // 0x10C
    unsigned short DisablePlayerControls;// 0x10E
    char ShakeFreq;                   // 0x110
    char bHornHistory[5];             // 0x111
    char iCurrHornHistory;            // 0x116
    char JustOutOfFrontEnd;           // 0x117
    char bApplyBrakes;                // 0x118
    char bDisablePlayerEnterCar;      // 0x119
    char bDisablePlayerDuck;          // 0x11A
    char bDisablePlayerFireWeapon;    // 0x11B
    char bDisablePlayerFireWeaponWithL1;// 0x11C
    char bDisablePlayerCycleWeapon;   // 0x11D
    char bDisablePlayerJump;          // 0x11E
    char bDisablePlayerDisplayVitalStats;// 0x11F
    int LastTimeTouched;              // 0x120
    int AverageWeapon;                // 0x124
    int AverageEntries;               // 0x128
    int NoShakeBeforeThis;            // 0x12C
    char NoShakeFreq;                 // 0x130
    char _pad131[3];
} CPad;

typedef struct {
    unsigned char lmb;
    unsigned char rmb;
    unsigned char mmb;
    unsigned char wheelUp;
    unsigned char wheelDown;
    unsigned char bmx1;
    unsigned char bmx2;
    char __align;
    float z;
    float x;                          // Horizontal delta
    float y;                          // Vertical delta
} CMouseControllerState;

// Engine Functions
typedef void* (*tFindPlayerVehicle)(int playerId, BOOL bIncludeRemote);
#define FUNC_FindPlayerVehicle ((tFindPlayerVehicle)0x0056E0D0)

#define ADDR_MOUSE_STATE         0x00B73418

#define ADDR_GAME_INVERTMOUSE_Y  0x00BA6745
#define ADDR_GAME_INVERTPAD_Y    0x00BA67FA

// FrontEnd & Menu Engine (CMenuManager)
#define ADDR_FRONTEND_MENU_MANAGER 0x00BA6748
#define ADDR_MENU_ACTIVE           0x00BA67A4 // 0x00BA6748 + 0x5C
#define ADDR_CURRENT_MENU_PAGE     0x00BA68A5 // 0x00BA6748 + 0x15D
#define ADDR_MAP_ZOOM              0x00BA67AC // 0x00BA6748 + 0x64
#define ADDR_MAP_BASE_X            0x00BA67B0 // 0x00BA6748 + 0x68
#define ADDR_MAP_BASE_Y            0x00BA67B4 // 0x00BA6748 + 0x6C

typedef void (__attribute__((thiscall)) *tSwitchMenuOnAndOff)(void* thisMgr);
#define FUNC_SwitchMenuOnAndOff ((tSwitchMenuOnAndOff)0x00576B70)

typedef char (__attribute__((thiscall)) *tSwitchToNewScreen)(void* thisMgr, char page);
#define FUNC_SwitchToNewScreen ((tSwitchToNewScreen)0x00573680)

typedef void (__attribute__((thiscall)) *tProcessUserInput)(void* thisMgr, char down, char up, char enter, char exit, char input);
#define FUNC_ProcessUserInput ((tProcessUserInput)0x0057B480)

static void LogMsg(const char* fmt, ...);

// ============================================================================
// RenderWare & CFont Definitions for PS5 Button Icons
// ============================================================================

typedef int (__cdecl *tAddTxdSlot)(const char* name);
typedef bool (__cdecl *tLoadTxd)(int slot, const char* filename);
typedef void (__cdecl *tAddRef)(int slot);
typedef void (__cdecl *tPushCurrentTxd)(void);
typedef void (__cdecl *tPopCurrentTxd)(void);
typedef void (__cdecl *tSetCurrentTxd)(int slot);
typedef void (__attribute__((thiscall)) *tSetTexture)(void* thisSprite, const char* name, const char* mask);

#define FUNC_CTxdStore_AddTxdSlot      ((tAddTxdSlot)0x00731A00)
#define FUNC_CTxdStore_LoadTxd         ((tLoadTxd)0x007320B0)
#define FUNC_CTxdStore_AddRef          ((tAddRef)0x00731CD0)
#define FUNC_CTxdStore_PushCurrentTxd  ((tPushCurrentTxd)0x007316A0)
#define FUNC_CTxdStore_PopCurrentTxd   ((tPopCurrentTxd)0x007316B0)
#define FUNC_CTxdStore_SetCurrentTxd   ((tSetCurrentTxd)0x007319C0)
#define FUNC_CSprite2d_SetTexture      ((tSetTexture)0x007272B0)

// Array of 16 CSprite2d structs (each CSprite2d is 4 bytes: void* m_pTexture)
static void* g_ps5ButtonSprites[16] = { 0 };

static const char* s_psButtonNames[16] = {
    NULL,          // 0: Unused
    "cross",       // 1: ~N~
    "circle",      // 2: ~A~
    "square",      // 3: ~<~
    "triangle",    // 4: ~=~
    "up",          // 5: ~Q~
    "down",        // 6: ~H~
    "left",        // 7: ~J~
    "right",       // 8: ~M~
    "l1",          // 9: ~E~
    "l2",          // 10: ~F~
    "r1",          // 11: ~>~
    "r2",          // 12: ~D~
    "l3",          // 13: ~O~
    "r3",          // 14: ~@~
    NULL           // 15
};

typedef struct {
    const char* token;
    const char* replacement;
} KeyTokenMap;

static const KeyTokenMap s_keyMap[] = {
    // A pe (On foot)
    { "~k~~PED_FIREWEAPON~",              "~D~" }, // R2 (Disparar)
    { "~k~~PED_FIREWEAPON_ALT~",          "~D~" }, // R2
    { "~k~~PED_LOCK_TARGET~",             "~F~" }, // L2 (Mirar / Lock-on)
    { "~k~~PED_SPRINT~",                  "~N~" }, // Cross (Correr)
    { "~k~~PED_JUMPING~",                 "~<~" }, // Square (Pular)
    { "~k~~PED_DUCK~",                    "~O~" }, // L3 (Agachar)
    { "~k~~PED_LOOKBEHIND~",              "~@~" }, // R3 (Olhar para tras)
    { "~k~~PED_CYCLE_WEAPON_LEFT~",       "~E~" }, // L1 (Arma anterior)
    { "~k~~PED_CYCLE_WEAPON_RIGHT~",      "~>~" }, // R1 (Arma seguinte)
    { "~k~~PED_CYCLE_TARGET_LEFT~",       "~E~" }, // L1
    { "~k~~PED_CYCLE_TARGET_RIGHT~",      "~>~" }, // R1
    { "~k~~PED_ANSWER_PHONE~",            "~=~" }, // Triangle (Atender telefone)
    { "~k~~PED_SNIPER_ZOOM_IN~",          "~<~" }, // Square
    { "~k~~PED_SNIPER_ZOOM_OUT~",         "~N~" }, // Cross
    { "~k~~PED_1RST_PERSON_LOOK_LEFT~",   "~J~" }, // D-Pad Esquerda
    { "~k~~PED_1RST_PERSON_LOOK_RIGHT~",  "~M~" }, // D-Pad Direita
    { "~k~~PED_1RST_PERSON_LOOK_UP~",     "~Q~" }, // D-Pad Cima
    { "~k~~PED_1RST_PERSON_LOOK_DOWN~",   "~H~" }, // D-Pad Baixo
    { "~k~~PED_CENTER_CAMERA_BEHIND_PLAYER~", "~@~" }, // R3

    // Em veiculo (In Vehicle)
    { "~k~~VEHICLE_ACCELERATE~",          "~D~" }, // R2 (Acelerar)
    { "~k~~VEHICLE_BRAKE~",               "~F~" }, // L2 (Freio / Marcha a re)
    { "~k~~VEHICLE_HANDBRAKE~",           "~>~" }, // R1 (Freio de mao)
    { "~k~~VEHICLE_FIREWEAPON~",          "~A~" }, // Circle (Atirar do veiculo)
    { "~k~~VEHICLE_FIREWEAPON_ALT~",      "~A~" }, // Circle
    { "~k~~VEHICLE_ENTER_EXIT~",          "~=~" }, // Triangle (Sair/Entrar)
    { "~k~~VEHICLE_HORN~",                "~O~" }, // L3 (Buzina)
    { "~k~~VEHICLE_LOOKBEHIND~",          "~E~" }, // L1 (Olhar tras)
    { "~k~~VEHICLE_LOOKLEFT~",            "~J~" }, // D-Pad Esquerda
    { "~k~~VEHICLE_LOOKRIGHT~",           "~M~" }, // D-Pad Direita
    { "~k~~VEHICLE_RADIO_STATION_UP~",    "~Q~" }, // D-Pad Cima (Mudar radio)
    { "~k~~VEHICLE_RADIO_STATION_DOWN~",  "~H~" }, // D-Pad Baixo
    { "~k~~VEHICLE_TURRETLEFT~",          "~J~" }, // D-Pad Esquerda
    { "~k~~VEHICLE_TURRETRIGHT~",         "~M~" }, // D-Pad Direita
    { "~k~~VEHICLE_TURRETUP~",            "~Q~" }, // D-Pad Cima
    { "~k~~VEHICLE_TURRETDOWN~",          "~H~" }, // D-Pad Baixo

    // Conversas e Grupo (Gang / Conversation)
    { "~k~~CONVERSATION_YES~",            "~M~" }, // D-Pad Direita (Sim)
    { "~k~~CONVERSATION_NO~",             "~J~" }, // D-Pad Esquerda (Nao)
    { "~k~~GROUP_CONTROL_FWD~",           "~Q~" }, // D-Pad Cima (Avancar)
    { "~k~~GROUP_CONTROL_BWD~",           "~H~" }  // D-Pad Baixo (Recuar)
};



static void InitPS5Buttons(void) {
    static BOOL s_done = FALSE;
    if (s_done) return;
    s_done = TRUE;

    LogMsg("[PS5 Buttons] Loading models\\ps3btns.txd...\n");
    int slot = FUNC_CTxdStore_AddTxdSlot("ps3btns");
    if (slot >= 0) {
        if (FUNC_CTxdStore_LoadTxd(slot, "models\\ps3btns.txd")) {
            FUNC_CTxdStore_AddRef(slot);
            FUNC_CTxdStore_PushCurrentTxd();
            FUNC_CTxdStore_SetCurrentTxd(slot);
            for (int i = 1; i <= 14; i++) {
                if (s_psButtonNames[i]) {
                    FUNC_CSprite2d_SetTexture(&g_ps5ButtonSprites[i], s_psButtonNames[i], NULL);
                }
            }
            FUNC_CTxdStore_PopCurrentTxd();
            LogMsg("[PS5 Buttons] Successfully loaded 14 PlayStation button textures!\n");

            // Patch CFont::PrintChar displacement at 0x00718AE1
            DWORD oldProtect;
            if (VirtualProtect((LPVOID)0x00718AE1, 4, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                *(DWORD*)0x00718AE1 = (DWORD)g_ps5ButtonSprites;
                VirtualProtect((LPVOID)0x00718AE1, 4, oldProtect, &oldProtect);
                LogMsg("[PS5 Buttons] CFont::PrintChar table patched to g_ps5ButtonSprites!\n");
            }
        } else {
            LogMsg("[PS5 Buttons] Failed to load models\\ps3btns.txd\n");
        }
    } else {
        LogMsg("[PS5 Buttons] Failed to add TXD slot ps3btns\n");
    }
}

static void EnsureMoveWhileAiming(void) {
    DWORD* pFlagsM4 = (DWORD*)(0x00C8AAB8 + 30 * 0x70 + 0x18);
    if (!(*pFlagsM4 & 0x10)) {
        DWORD oldProtect;
        if (VirtualProtect((LPVOID)0x00C8AAB8, 80 * 0x70, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            for (int i = 0; i < 80; i++) {
                DWORD* pFlags = (DWORD*)(0x00C8AAB8 + i * 0x70 + 0x18);
                *pFlags |= 0x30; // bMoveAim (0x10) | bMoveFire (0x20)
            }
            VirtualProtect((LPVOID)0x00C8AAB8, 80 * 0x70, oldProtect, &oldProtect);
            LogMsg("[MoveWhileAim] bMoveAim and bMoveFire applied to all 80 weapons!\n");
        }
    }
}

// ============================================================================
// Mod Configuration & Logging
// ============================================================================

typedef struct {
    int deadzoneLeft;
    int deadzoneRight;
    float camSensX;
    float camSensY;
    int invertY;
    int controllerType;
} ModConfig;

static ModConfig g_cfg = { 18, 20, 0.12f, 0.10f, 0, 1 };
static FILE* g_logFile = NULL;

static void LogMsg(const char* fmt, ...) {
    if (!g_logFile) {
        char logPath[MAX_PATH];
        GetModuleFileNameA(NULL, logPath, MAX_PATH);
        char* p = strrchr(logPath, '\\');
        if (p) *p = '\0';
        strcat(logPath, "\\UniversalController.log");
        g_logFile = fopen(logPath, "a");
    }
    if (g_logFile) {
        va_list args;
        va_start(args, fmt);
        vfprintf(g_logFile, fmt, args);
        va_end(args);
        fflush(g_logFile);
    }
}

static void LoadConfig(void) {
    char iniPath[MAX_PATH];
    GetModuleFileNameA(NULL, iniPath, MAX_PATH);
    char* p = strrchr(iniPath, '\\');
    if (p) *p = '\0';
    strcat(iniPath, "\\modloader\\UniversalController\\UniversalController.ini");

    g_cfg.deadzoneLeft = GetPrivateProfileIntA("Settings", "DeadzoneLeft", 18, iniPath);
    g_cfg.deadzoneRight = GetPrivateProfileIntA("Settings", "DeadzoneRight", 20, iniPath);
    g_cfg.invertY = GetPrivateProfileIntA("Settings", "InvertY", 0, iniPath);
    g_cfg.controllerType = GetPrivateProfileIntA("Settings", "ControllerType", 1, iniPath);

    char sensBuf[32];
    GetPrivateProfileStringA("Settings", "CamSensX", "0.12", sensBuf, sizeof(sensBuf), iniPath);
    g_cfg.camSensX = (float)atof(sensBuf);
    GetPrivateProfileStringA("Settings", "CamSensY", "0.10", sensBuf, sizeof(sensBuf), iniPath);
    g_cfg.camSensY = (float)atof(sensBuf);

    LogMsg("[Config] Loaded from %s\n", iniPath);
    LogMsg("[Config] ControllerType=%d, DeadzoneL=%d, DeadzoneR=%d, SensX=%.3f, SensY=%.3f, InvertY=%d\n",
           g_cfg.controllerType, g_cfg.deadzoneLeft, g_cfg.deadzoneRight, g_cfg.camSensX, g_cfg.camSensY, g_cfg.invertY);
}

// ============================================================================
// DualSense Hardware Engine: Native HID Reader & Output (Adaptive Triggers/Rumble)
// ============================================================================

static const GUID GUID_DEVINTERFACE_HID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

static unsigned int g_crc32_table[256];
static int g_crc32_init = 0;

static void InitCrc32(void) {
    if (g_crc32_init) return;
    for (unsigned int i = 0; i < 256; i++) {
        unsigned int crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
        g_crc32_table[i] = crc;
    }
    g_crc32_init = 1;
}

static unsigned int ComputeCrc32(const unsigned char* data, size_t len) {
    InitCrc32();
    unsigned int crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = g_crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

typedef struct {
    BOOL connected;
    short lx, ly;           // -128 to 127
    short rx, ry;           // -128 to 127
    BYTE l2, r2;            // 0 to 255 (analog)
    BOOL btnCross;
    BOOL btnCircle;
    BOOL btnSquare;
    BOOL btnTriangle;
    BOOL btnL1;
    BOOL btnR1;
    BOOL btnL2;             // digital L2
    BOOL btnR2;             // digital R2
    BOOL btnL3;
    BOOL btnR3;
    BOOL btnSelect;
    BOOL btnStart;
    BOOL btnShare;
    BOOL btnTouch;
    BOOL btnPS;
    BOOL dpadUp, dpadDown, dpadLeft, dpadRight;
} DualSenseInputState;

static volatile DualSenseInputState g_dsInput = { 0 };
static HANDLE g_hDualSense = INVALID_HANDLE_VALUE;
static volatile BOOL g_dsRunning = TRUE;

// Output state
static unsigned char g_targetLeftMotor = 0;
static unsigned char g_targetRightMotor = 0;
static unsigned char g_lastSentLeftMotor = 0xFF;
static unsigned char g_lastSentRightMotor = 0xFF;
static volatile BOOL g_triggerGunActive = FALSE;
static volatile BOOL g_lastSentTriggerGun = FALSE;
static DWORD g_lastOutputTick = 0;

// ============================================================================
// Dynamic HID & Controller Detection (Zero Hardcoded Keys/Paths)
// ============================================================================

typedef enum {
    SONY_DEV_NONE = 0,
    SONY_DEV_DUALSENSE,  // PS5: 0x0CE6, 0x0DF2
    SONY_DEV_DUALSHOCK4  // PS4: 0x05C4, 0x09CC
} SonyDeviceType;

static volatile SonyDeviceType g_sonyDevType = SONY_DEV_NONE;
static volatile BOOL g_sonyIsBluetooth = FALSE;

static HANDLE FindAndOpenDualSenseDevice(void) {
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO devInfo = SetupDiGetClassDevsA(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }

    SP_DEVICE_INTERFACE_DATA devData;
    devData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    HANDLE hFound = INVALID_HANDLE_VALUE;

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, NULL, &hidGuid, i, &devData); i++) {
        DWORD detailSize = 0;
        SetupDiGetDeviceInterfaceDetailA(devInfo, &devData, NULL, 0, &detailSize, NULL);
        if (detailSize == 0) continue;

        PSP_DEVICE_INTERFACE_DETAIL_DATA_A pDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)malloc(detailSize);
        if (!pDetail) continue;

        pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        if (SetupDiGetDeviceInterfaceDetailA(devInfo, &devData, pDetail, detailSize, NULL, NULL)) {
            const char* path = pDetail->DevicePath;
            if (strstr(path, "054c") || strstr(path, "054C") || strstr(path, "vid_054c") || strstr(path, "VID_054C")) {
                HANDLE h = CreateFileA(path,
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_EXISTING, 0, NULL);
                if (h != INVALID_HANDLE_VALUE) {
                    HIDD_ATTRIBUTES attr;
                    attr.Size = sizeof(HIDD_ATTRIBUTES);
                    if (HidD_GetAttributes(h, &attr)) {
                        if (attr.VendorID == 0x054C) {
                            if (attr.ProductID == 0x0CE6 || attr.ProductID == 0x0DF2) {
                                g_sonyDevType = SONY_DEV_DUALSENSE;
                                LogMsg("[SonyHID] Auto-detected Sony DualSense (PS5) (VID=0x%04X, PID=0x%04X)\n",
                                       attr.VendorID, attr.ProductID);
                                hFound = h;
                                free(pDetail);
                                break;
                            } else if (attr.ProductID == 0x05C4 || attr.ProductID == 0x09CC) {
                                g_sonyDevType = SONY_DEV_DUALSHOCK4;
                                LogMsg("[SonyHID] Auto-detected Sony DualShock 4 (PS4) (VID=0x%04X, PID=0x%04X)\n",
                                       attr.VendorID, attr.ProductID);
                                hFound = h;
                                free(pDetail);
                                break;
                            }
                        }
                    }
                    CloseHandle(h);
                }
            }
        }
        free(pDetail);
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return hFound;
}

static volatile BYTE g_lightbarRed = 0;
static volatile BYTE g_lightbarGreen = 120;
static volatile BYTE g_lightbarBlue = 255;

static void SendDualSenseHardwareReport(HANDLE hDev, BYTE leftMotor, BYTE rightMotor, BOOL gunTrigger) {
    if (g_sonyDevType == SONY_DEV_DUALSENSE) {
        BYTE report[78];
        memset(report, 0, sizeof(report));

        report[0] = 0x31; // Report ID
        report[1] = 0x02; // Config tag
        report[2] = 0xFF; // Enable motors + triggers
        report[3] = 0x1 | 0x2 | 0x4 | 0x10 | 0x40; // Enable LEDs and lightbar
        report[4] = rightMotor; // High-frequency weak rumble
        report[5] = leftMotor;  // Low-frequency strong rumble

        // R2 (Gatilho de Disparo): Resistência mecânica ATIVADA SOMENTE COM ARMA DE FOGO!
        // Sem arma / Em veículo / Menus: 100% suave e livre, sem resistência desnecessária!
        if (gunTrigger) {
            report[11] = 0x02; // Section Resistance / Rigid Stop
            report[12] = 0x02;
            report[13] = 25;   // Ponto onde o gatilho começa a oferecer resistência
            report[14] = 175;  // Resistência tátil de peso de gatilho
        } else {
            report[11] = 0x00; // Desativado
            report[12] = 0x00;
            report[13] = 0;
            report[14] = 0;
        }

        // L2 (ONDE MIRA): NUNCA TEM RESISTÊNCIA! 100% suave e livre conforme pedido do usuário!
        report[22] = 0x00;
        report[23] = 0x00;
        report[24] = 0;
        report[25] = 0;

        // Lightbar & Player LEDs
        report[40] = 0x02; // Uninterruptable LED
        report[43] = 0x00;
        report[44] = 0x00; // Brightness High
        report[45] = 0x04; // Player 1 LED
        report[46] = g_lightbarRed;
        report[47] = g_lightbarGreen;
        report[48] = g_lightbarBlue;

        // CRC32 of 0xA2 + report[0..73]
        BYTE crcBuf[75];
        crcBuf[0] = 0xA2;
        memcpy(crcBuf + 1, report, 74);
        DWORD crc = ComputeCrc32(crcBuf, 75);

        report[74] = (BYTE)(crc & 0xFF);
        report[75] = (BYTE)((crc >> 8) & 0xFF);
        report[76] = (BYTE)((crc >> 16) & 0xFF);
        report[77] = (BYTE)((crc >> 24) & 0xFF);

        DWORD written = 0;
        WriteFile(hDev, report, 78, &written, NULL);
    } else if (g_sonyDevType == SONY_DEV_DUALSHOCK4) {
        if (g_sonyIsBluetooth) {
            // DS4 Bluetooth Output Report 0x11 (78 bytes)
            BYTE report[78];
            memset(report, 0, sizeof(report));
            report[0] = 0x11;
            report[1] = 0xC0 | 0x04;
            report[3] = 0x03; // Enable rumble + lightbar
            report[6] = rightMotor; // High-frequency weak rumble
            report[7] = leftMotor;  // Low-frequency strong rumble
            report[8] = g_lightbarRed;
            report[9] = g_lightbarGreen;
            report[10] = g_lightbarBlue;

            BYTE crcBuf[75];
            crcBuf[0] = 0xA2;
            memcpy(crcBuf + 1, report, 74);
            DWORD crc = ComputeCrc32(crcBuf, 75);

            report[74] = (BYTE)(crc & 0xFF);
            report[75] = (BYTE)((crc >> 8) & 0xFF);
            report[76] = (BYTE)((crc >> 16) & 0xFF);
            report[77] = (BYTE)((crc >> 24) & 0xFF);

            DWORD written = 0;
            WriteFile(hDev, report, 78, &written, NULL);
        } else {
            // DS4 USB Output Report 0x05 (32 bytes)
            BYTE report[32];
            memset(report, 0, sizeof(report));
            report[0] = 0x05;
            report[1] = 0x07; // Enable rumble right, left, LED
            report[4] = rightMotor;
            report[5] = leftMotor;
            report[6] = g_lightbarRed;
            report[7] = g_lightbarGreen;
            report[8] = g_lightbarBlue;

            DWORD written = 0;
            WriteFile(hDev, report, 32, &written, NULL);
        }
    }
}

static DWORD WINAPI DualSenseWorkerThread(LPVOID lpParam) {
    LogMsg("[SonyHID] Worker Thread started (DualSense & DualShock 4 Support).\n");

    BYTE buf[78];
    DWORD readBytes = 0;

    while (g_dsRunning) {
        if (g_hDualSense == INVALID_HANDLE_VALUE) {
            g_hDualSense = FindAndOpenDualSenseDevice();
            if (g_hDualSense != INVALID_HANDLE_VALUE) {
                LogMsg("[SonyHID] Controller connected! Type: %s\n",
                       g_sonyDevType == SONY_DEV_DUALSHOCK4 ? "DualShock 4 (PS4)" : "DualSense (PS5)");
                g_lastSentLeftMotor = 0;
                g_lastSentRightMotor = 0;
                g_lastSentTriggerGun = FALSE;
                g_lastOutputTick = GetTickCount();
                SendDualSenseHardwareReport(g_hDualSense, 0, 0, FALSE);
            } else {
                Sleep(1000);
                continue;
            }
        }

        // 1. Output update (Rumble, Triggers & Lightbar heartbeat)
        DWORD now = GetTickCount();
        BOOL motorChanged = (g_targetLeftMotor != g_lastSentLeftMotor || g_targetRightMotor != g_lastSentRightMotor);
        BOOL triggerChanged = (g_triggerGunActive != g_lastSentTriggerGun);
        BOOL heartbeat = (now - g_lastOutputTick) >= 400;

        if (motorChanged || triggerChanged || heartbeat) {
            g_lastSentLeftMotor = g_targetLeftMotor;
            g_lastSentRightMotor = g_targetRightMotor;
            g_lastSentTriggerGun = g_triggerGunActive;
            g_lastOutputTick = now;
            SendDualSenseHardwareReport(g_hDualSense, g_lastSentLeftMotor, g_lastSentRightMotor, g_lastSentTriggerGun);
        }

        // 2. Read hardware input stream
        if (ReadFile(g_hDualSense, buf, 78, &readBytes, NULL) && readBytes >= 10) {
            if (g_sonyDevType == SONY_DEV_DUALSHOCK4) {
                if (buf[0] == 0x11 && readBytes >= 12) {
                    // DS4 Bluetooth (Report 0x11)
                    g_sonyIsBluetooth = TRUE;
                    g_dsInput.lx = (short)((int)buf[3] - 128);
                    g_dsInput.ly = (short)((int)buf[4] - 128);
                    g_dsInput.rx = (short)((int)buf[5] - 128);
                    g_dsInput.ry = (short)((int)buf[6] - 128);

                    BYTE b0 = buf[7];
                    g_dsInput.btnSquare   = (b0 & 0x10) != 0;
                    g_dsInput.btnCross    = (b0 & 0x20) != 0;
                    g_dsInput.btnCircle   = (b0 & 0x40) != 0;
                    g_dsInput.btnTriangle = (b0 & 0x80) != 0;

                    BYTE dpad = b0 & 0x0F;
                    g_dsInput.dpadUp    = (dpad == 0 || dpad == 1 || dpad == 7);
                    g_dsInput.dpadRight = (dpad == 1 || dpad == 2 || dpad == 3);
                    g_dsInput.dpadDown  = (dpad == 3 || dpad == 4 || dpad == 5);
                    g_dsInput.dpadLeft  = (dpad == 5 || dpad == 6 || dpad == 7);

                    BYTE b1 = buf[8];
                    g_dsInput.btnL1     = (b1 & 0x01) != 0;
                    g_dsInput.btnR1     = (b1 & 0x02) != 0;
                    g_dsInput.btnL2     = (b1 & 0x04) != 0;
                    g_dsInput.btnR2     = (b1 & 0x08) != 0;
                    g_dsInput.btnShare  = (b1 & 0x10) != 0;
                    g_dsInput.btnStart  = (b1 & 0x20) != 0;
                    g_dsInput.btnL3     = (b1 & 0x40) != 0;
                    g_dsInput.btnR3     = (b1 & 0x80) != 0;

                    BYTE b2 = buf[9];
                    g_dsInput.btnPS     = (b2 & 0x01) != 0;
                    g_dsInput.btnTouch  = (b2 & 0x02) != 0;
                    g_dsInput.btnSelect = g_dsInput.btnShare || g_dsInput.btnTouch;

                    g_dsInput.l2 = buf[10];
                    g_dsInput.r2 = buf[11];

                    g_dsInput.connected = TRUE;
                } else {
                    // DS4 USB / Standard (Report 0x01)
                    g_sonyIsBluetooth = FALSE;
                    int base = (buf[0] == 0x01) ? 1 : 0;
                    g_dsInput.lx = (short)((int)buf[base + 0] - 128);
                    g_dsInput.ly = (short)((int)buf[base + 1] - 128);
                    g_dsInput.rx = (short)((int)buf[base + 2] - 128);
                    g_dsInput.ry = (short)((int)buf[base + 3] - 128);

                    BYTE b0 = buf[base + 4];
                    g_dsInput.btnSquare   = (b0 & 0x10) != 0;
                    g_dsInput.btnCross    = (b0 & 0x20) != 0;
                    g_dsInput.btnCircle   = (b0 & 0x40) != 0;
                    g_dsInput.btnTriangle = (b0 & 0x80) != 0;

                    BYTE dpad = b0 & 0x0F;
                    g_dsInput.dpadUp    = (dpad == 0 || dpad == 1 || dpad == 7);
                    g_dsInput.dpadRight = (dpad == 1 || dpad == 2 || dpad == 3);
                    g_dsInput.dpadDown  = (dpad == 3 || dpad == 4 || dpad == 5);
                    g_dsInput.dpadLeft  = (dpad == 5 || dpad == 6 || dpad == 7);

                    BYTE b1 = buf[base + 5];
                    g_dsInput.btnL1     = (b1 & 0x01) != 0;
                    g_dsInput.btnR1     = (b1 & 0x02) != 0;
                    g_dsInput.btnL2     = (b1 & 0x04) != 0;
                    g_dsInput.btnR2     = (b1 & 0x08) != 0;
                    g_dsInput.btnShare  = (b1 & 0x10) != 0;
                    g_dsInput.btnStart  = (b1 & 0x20) != 0;
                    g_dsInput.btnL3     = (b1 & 0x40) != 0;
                    g_dsInput.btnR3     = (b1 & 0x80) != 0;

                    BYTE b2 = buf[base + 6];
                    g_dsInput.btnPS     = (b2 & 0x01) != 0;
                    g_dsInput.btnTouch  = (b2 & 0x02) != 0;
                    g_dsInput.btnSelect = g_dsInput.btnShare || g_dsInput.btnTouch;

                    g_dsInput.l2 = buf[base + 7];
                    g_dsInput.r2 = buf[base + 8];

                    g_dsInput.connected = TRUE;
                }
            } else {
                // DualSense PS5 parsing (Report 0x31 BT or 0x01 USB)
                int base = (buf[0] == 0x31) ? 2 : 1;
                g_sonyIsBluetooth = (buf[0] == 0x31);

                g_dsInput.lx = (short)((int)buf[base + 0] - 128);
                g_dsInput.ly = (short)((int)buf[base + 1] - 128);
                g_dsInput.rx = (short)((int)buf[base + 2] - 128);
                g_dsInput.ry = (short)((int)buf[base + 3] - 128);
                g_dsInput.l2 = buf[base + 4];
                g_dsInput.r2 = buf[base + 5];

                BYTE b0 = buf[base + 7];
                g_dsInput.btnSquare   = (b0 & 0x10) != 0;
                g_dsInput.btnCross    = (b0 & 0x20) != 0;
                g_dsInput.btnCircle   = (b0 & 0x40) != 0;
                g_dsInput.btnTriangle = (b0 & 0x80) != 0;

                BYTE dpad = b0 & 0x0F;
                g_dsInput.dpadUp    = (dpad == 0 || dpad == 1 || dpad == 7);
                g_dsInput.dpadRight = (dpad == 1 || dpad == 2 || dpad == 3);
                g_dsInput.dpadDown  = (dpad == 3 || dpad == 4 || dpad == 5);
                g_dsInput.dpadLeft  = (dpad == 5 || dpad == 6 || dpad == 7);

                BYTE b1 = buf[base + 8];
                g_dsInput.btnL1     = (b1 & 0x01) != 0;
                g_dsInput.btnR1     = (b1 & 0x02) != 0;
                g_dsInput.btnL2     = (b1 & 0x04) != 0;
                g_dsInput.btnR2     = (b1 & 0x08) != 0;
                g_dsInput.btnShare  = (b1 & 0x10) != 0;
                g_dsInput.btnStart  = (b1 & 0x20) != 0;
                g_dsInput.btnL3     = (b1 & 0x40) != 0;
                g_dsInput.btnR3     = (b1 & 0x80) != 0;

                BYTE b2 = (readBytes > (DWORD)(base + 9)) ? buf[base + 9] : 0;
                g_dsInput.btnPS     = (b2 & 0x01) != 0;
                g_dsInput.btnTouch  = (b2 & 0x02) != 0;
                g_dsInput.btnSelect = g_dsInput.btnShare || g_dsInput.btnTouch;

                g_dsInput.connected = TRUE;
            }
        } else {
            DWORD err = GetLastError();
            if (err == ERROR_DEVICE_NOT_CONNECTED || err == ERROR_GEN_FAILURE || err == ERROR_INVALID_HANDLE) {
                LogMsg("[SonyHID] Controller disconnected, scanning for reconnect...\n");
                CloseHandle(g_hDualSense);
                g_hDualSense = INVALID_HANDLE_VALUE;
                g_sonyDevType = SONY_DEV_NONE;
                g_dsInput.connected = FALSE;
            }
            Sleep(5);
        }
    }

    if (g_hDualSense != INVALID_HANDLE_VALUE) {
        SendDualSenseHardwareReport(g_hDualSense, 0, 0, FALSE);
        CloseHandle(g_hDualSense);
        g_hDualSense = INVALID_HANDLE_VALUE;
        g_sonyDevType = SONY_DEV_NONE;
    }
    return 0;
}

// ============================================================================
// Dynamic XInput & DirectInput Universal Fallback
// ============================================================================

typedef struct {
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
} XINPUT_GAMEPAD_S;

typedef struct {
    DWORD dwPacketNumber;
    XINPUT_GAMEPAD_S Gamepad;
} XINPUT_STATE_S;

typedef struct {
    WORD wLeftMotorSpeed;
    WORD wRightMotorSpeed;
} XINPUT_VIBRATION_S;

typedef DWORD (WINAPI *tXInputGetState)(DWORD dwUserIndex, XINPUT_STATE_S* pState);
typedef DWORD (WINAPI *tXInputSetState)(DWORD dwUserIndex, XINPUT_VIBRATION_S* pVibration);

static HMODULE g_hXInput = NULL;
static tXInputGetState g_pfnXInputGetState = NULL;
static tXInputSetState g_pfnXInputSetState = NULL;
static BOOL g_xinputAttempted = FALSE;

static void EnsureXInputLoaded(void) {
    if (g_xinputAttempted) return;
    g_xinputAttempted = TRUE;
    const char* dlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    for (int i = 0; i < 3; i++) {
        g_hXInput = LoadLibraryA(dlls[i]);
        if (g_hXInput) {
            g_pfnXInputGetState = (tXInputGetState)GetProcAddress(g_hXInput, "XInputGetState");
            g_pfnXInputSetState = (tXInputSetState)GetProcAddress(g_hXInput, "XInputSetState");
            if (g_pfnXInputGetState) {
                LogMsg("[XInput] Dynamic fallback loader active (%s)\n", dlls[i]);
                break;
            }
        }
    }
}

static BOOL PollXInput(DualSenseInputState* outState) {
    EnsureXInputLoaded();
    if (!g_pfnXInputGetState) return FALSE;

    XINPUT_STATE_S xi;
    memset(&xi, 0, sizeof(xi));
    for (DWORD i = 0; i < 4; i++) {
        if (g_pfnXInputGetState(i, &xi) == 0) {
            outState->connected = TRUE;
            outState->lx = (short)(xi.Gamepad.sThumbLX / 256);
            outState->ly = (short)(-xi.Gamepad.sThumbLY / 256);
            outState->rx = (short)(xi.Gamepad.sThumbRX / 256);
            outState->ry = (short)(-xi.Gamepad.sThumbRY / 256);
            outState->l2 = xi.Gamepad.bLeftTrigger;
            outState->r2 = xi.Gamepad.bRightTrigger;
            outState->btnCross    = (xi.Gamepad.wButtons & 0x1000) != 0; // A
            outState->btnCircle   = (xi.Gamepad.wButtons & 0x2000) != 0; // B
            outState->btnSquare   = (xi.Gamepad.wButtons & 0x4000) != 0; // X
            outState->btnTriangle = (xi.Gamepad.wButtons & 0x8000) != 0; // Y
            outState->btnL1       = (xi.Gamepad.wButtons & 0x0100) != 0; // LB
            outState->btnR1       = (xi.Gamepad.wButtons & 0x0200) != 0; // RB
            outState->btnShare    = (xi.Gamepad.wButtons & 0x0020) != 0; // Back
            outState->btnStart    = (xi.Gamepad.wButtons & 0x0010) != 0; // Start
            outState->btnL3       = (xi.Gamepad.wButtons & 0x0040) != 0; // LS
            outState->btnR3       = (xi.Gamepad.wButtons & 0x0080) != 0; // RS
            outState->btnSelect   = outState->btnShare;
            outState->dpadUp      = (xi.Gamepad.wButtons & 0x0001) != 0;
            outState->dpadDown    = (xi.Gamepad.wButtons & 0x0002) != 0;
            outState->dpadLeft    = (xi.Gamepad.wButtons & 0x0004) != 0;
            outState->dpadRight   = (xi.Gamepad.wButtons & 0x0008) != 0;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL PollDirectInput(DualSenseInputState* outState) {
    JOYINFOEX jie;
    memset(&jie, 0, sizeof(jie));
    jie.dwSize = sizeof(jie);
    jie.dwFlags = JOY_RETURNALL;

    for (UINT id = 0; id < 4; id++) {
        if (joyGetPosEx(id, &jie) == JOYERR_NOERROR) {
            short lx = (short)(((int)jie.dwXpos - 32768) / 256);
            short ly = (short)(((int)jie.dwYpos - 32768) / 256);
            short rx = (short)(((int)jie.dwZpos - 32768) / 256);
            short ry = (short)(((int)jie.dwRpos - 32768) / 256);

            // Filter phantom zeroed devices
            if (jie.dwButtons == 0 && abs(lx) < 2 && abs(ly) < 2 && abs(rx) < 2 && abs(ry) < 2 && jie.dwPOV == 0xFFFF) {
                continue;
            }

            outState->connected = TRUE;
            outState->lx = lx;
            outState->ly = ly;
            outState->rx = rx;
            outState->ry = ry;
            outState->l2 = (BYTE)(jie.dwUpos / 257);
            outState->r2 = (BYTE)(jie.dwVpos / 257);
            outState->btnCross    = (jie.dwButtons & (1 << 0)) != 0;
            outState->btnCircle   = (jie.dwButtons & (1 << 1)) != 0;
            outState->btnSquare   = (jie.dwButtons & (1 << 2)) != 0;
            outState->btnTriangle = (jie.dwButtons & (1 << 3)) != 0;
            outState->btnL1       = (jie.dwButtons & (1 << 4)) != 0;
            outState->btnR1       = (jie.dwButtons & (1 << 5)) != 0;
            outState->btnShare    = (jie.dwButtons & (1 << 8)) != 0;
            outState->btnStart    = (jie.dwButtons & (1 << 9)) != 0;
            outState->btnL3       = (jie.dwButtons & (1 << 10)) != 0;
            outState->btnR3       = (jie.dwButtons & (1 << 11)) != 0;
            outState->btnSelect   = outState->btnShare;
            if (jie.dwPOV != 0xFFFF) {
                outState->dpadUp    = (jie.dwPOV == 0    || jie.dwPOV == 4500  || jie.dwPOV == 31500);
                outState->dpadRight = (jie.dwPOV == 4500 || jie.dwPOV == 9000  || jie.dwPOV == 13500);
                outState->dpadDown  = (jie.dwPOV == 13500|| jie.dwPOV == 18000 || jie.dwPOV == 22500);
                outState->dpadLeft  = (jie.dwPOV == 22500|| jie.dwPOV == 27000 || jie.dwPOV == 31500);
            }
            return TRUE;
        }
    }
    return FALSE;
}

// ============================================================================
// Deadzone Calculator
// ============================================================================

static short ApplyDeadzoneVal(short val, int percent) {
    int maxVal = 128;
    int threshold = (maxVal * percent) / 100;
    if (val > -threshold && val < threshold) return 0;
    if (val > 0) return (short)(((val - threshold) * 128) / (128 - threshold));
    return (short)(((val + threshold) * 128) / (128 - threshold));
}

// ============================================================================
// Weapon Recognition (Adaptive Triggers Active ONLY for Firearms!)
// ============================================================================

static BOOL IsPlayerHoldingFirearm(void) {
    void* pVeh = FUNC_FindPlayerVehicle(-1, FALSE);
    if (pVeh) return FALSE; // Em veículo: gatilho de arma desativado! Aceleração analógica suave.

    BYTE* pPed = *(BYTE**)0x00B7CD98; // CWorld::Players[0].m_pPed
    if (!pPed) return FALSE;

    BYTE slot = *(BYTE*)(pPed + 0x718); // m_nActiveWeaponSlot
    // Slots de armas de fogo:
    // 2: Pistolas (Pistol, Silenced, Desert Eagle)
    // 3: Espingardas (Shotgun, Sawn-off, SPAS-12)
    // 4: Submetralhadoras (Micro Uzi, MP5, Tec-9)
    // 5: Fuzis de Assalto (AK-47, M4)
    // 6: Rifles de Precisão (Country Rifle, Sniper Rifle)
    // 7: Armas Pesadas (Rocket Launcher, Heat Seeker, Flamethrower, Minigun)
    if (slot >= 2 && slot <= 7) {
        DWORD weaponType = *(DWORD*)(pPed + 0x5A0 + slot * 0x1C);
        if (weaponType >= 22 && weaponType <= 38) {
            return TRUE;
        }
    }
    return FALSE;
}

// ============================================================================
// Controller Processing & Engine Injection
// ============================================================================

static DWORD g_lastInputLogTime = 0;

static void ProcessCustomController(CPad* pad) {
    if (!pad) return;

    DualSenseInputState gp = { 0 };

    // Multi-tier Universal Detection:
    // 1. Native Sony HID (DualSense PS5, DualShock 4)
    if (g_dsInput.connected) {
        gp = g_dsInput;
    }
    // 2. XInput (Xbox, DS4Windows, Steam Input, DualSenseX)
    else if (PollXInput(&gp)) {
        // Active via XInput
    }
    // 3. DirectInput (Generic USB gamepads)
    else if (PollDirectInput(&gp)) {
        // Active via DirectInput
    }
    else {
        return; // No controller connected
    }

    DWORD now = GetTickCount();

    gp.lx = ApplyDeadzoneVal(gp.lx, g_cfg.deadzoneLeft);
    gp.ly = ApplyDeadzoneVal(gp.ly, g_cfg.deadzoneLeft);
    gp.rx = ApplyDeadzoneVal(gp.rx, g_cfg.deadzoneRight);
    gp.ry = ApplyDeadzoneVal(gp.ry, g_cfg.deadzoneRight);

    // Tell engine that pad is active and touched
    pad->LastTimeTouched = now;

    // Log active gameplay input periodically
    BOOL hasInput = (gp.lx != 0 || gp.ly != 0 || gp.rx != 0 || gp.ry != 0 ||
                     gp.l2 > 30 || gp.r2 > 30 || gp.btnCross || gp.btnSquare ||
                     gp.btnTriangle || gp.btnCircle || gp.btnL1 || gp.btnR1 ||
                     gp.dpadUp || gp.dpadDown || gp.dpadLeft || gp.dpadRight ||
                     gp.btnStart || gp.btnSelect || gp.btnPS);

    if (hasInput && (now - g_lastInputLogTime > 2500)) {
        g_lastInputLogTime = now;
        LogMsg("[Pad0 Input] Active: LX=%d, LY=%d, RX=%d, RY=%d, L2=%d, R2=%d, X=%d, Sq=%d, Tri=%d, Cir=%d\n",
               gp.lx, gp.ly, gp.rx, gp.ry, gp.l2, gp.r2,
               gp.btnCross, gp.btnSquare, gp.btnTriangle, gp.btnCircle);
    }

    BYTE isMenuActive = *(BYTE*)ADDR_MENU_ACTIVE;
    char curMenuPage = *(char*)ADDR_CURRENT_MENU_PAGE;

    // GATILHOS ADAPTATIVOS DUALSENSE:
    // Resistência no R2 ATIVA SOMENTE SE ESTIVER EMPUNHANDO UMA ARMA DE FOGO!
    // Se estiver desarmado, socos, faca, veículos ou menus -> 100% livre e macio!
    // L2 (MIRA): NUNCA TEM RESISTÊNCIA! 100% livre e macio!
    g_triggerGunActive = (!isMenuActive && IsPlayerHoldingFirearm());

    // SHARE / TOUCHPAD: Direct Map Shortcut
    static BOOL s_lastMapBtn = FALSE;
    BOOL mapBtn = (gp.btnShare || gp.btnTouch);
    BOOL mapEdge = (mapBtn && !s_lastMapBtn);
    s_lastMapBtn = mapBtn;

    if (mapEdge) {
        LogMsg("[Menu] Share/Touchpad toggled! Active=%d, Page=%d\n", isMenuActive, curMenuPage);
        if (!isMenuActive) {
            FUNC_SwitchMenuOnAndOff((void*)ADDR_FRONTEND_MENU_MANAGER);
            FUNC_SwitchToNewScreen((void*)ADDR_FRONTEND_MENU_MANAGER, 5);
            return;
        } else {
            if (curMenuPage == 5) {
                FUNC_SwitchMenuOnAndOff((void*)ADDR_FRONTEND_MENU_MANAGER);
            } else {
                FUNC_SwitchToNewScreen((void*)ADDR_FRONTEND_MENU_MANAGER, 5);
            }
            return;
        }
    }

    // MAP PAGE CONTROLS (Page 5)
    if (isMenuActive && curMenuPage == 5) {
        float* pMapX = (float*)ADDR_MAP_BASE_X;
        float* pMapY = (float*)ADDR_MAP_BASE_Y;
        if (pMapX && pMapY && (gp.lx != 0 || gp.ly != 0)) {
            *pMapX += ((float)gp.lx / 128.0f) * 14.0f;
            *pMapY += ((float)gp.ly / 128.0f) * 14.0f;
        }
        float* pZoom = (float*)ADDR_MAP_ZOOM;
        if (pZoom) {
            if (gp.r2 > 50 || gp.btnR1) *pZoom += 0.015f;
            if (gp.l2 > 50 || gp.btnL1) *pZoom -= 0.015f;
            if (*pZoom < 0.2f) *pZoom = 0.2f;
            if (*pZoom > 8.0f) *pZoom = 8.0f;
        }
    }

    // ========================================================================
    // FRONTEND / MENU NAVIGATION
    // ========================================================================
    if (isMenuActive != 0) {
        g_triggerGunActive = FALSE;
        // Inversao corrigida para navegacao de menu: D-Pad Cima SOBE o cursor, D-Pad Baixo DESCE!
        if (gp.dpadUp || gp.ly < -50)          pad->NewState.DPadDown = 255;
        if (gp.dpadDown || gp.ly > 50)        pad->NewState.DPadUp = 255;
        if (gp.dpadLeft || gp.lx < -50)       pad->NewState.DPadLeft = 255;
        if (gp.dpadRight || gp.lx > 50)      pad->NewState.DPadRight = 255;
        if (gp.btnCross)                      pad->NewState.ButtonCross = 255;
        if (gp.btnTriangle || gp.btnCircle)   pad->NewState.ButtonTriangle = 255;
        if (gp.btnStart)                      pad->NewState.Start = 255;
        if (abs(gp.lx) > 8) pad->NewState.LeftStickX = gp.lx;
        if (abs(gp.ly) > 8) pad->NewState.LeftStickY = -gp.ly;

        // Also trigger ProcessUserInput for instant responsive cursor/item navigation
        static DWORD s_nextNavRepeat = 0;
        static int s_activeDir = 0;

        int curDir = 0;
        if (gp.dpadUp || gp.ly < -60) curDir = 1;
        else if (gp.dpadDown || gp.ly > 60) curDir = 2;
        else if (gp.dpadLeft || gp.lx < -60) curDir = 3;
        else if (gp.dpadRight || gp.lx > 60) curDir = 4;

        char down = 0, up = 0, input = 0;
        if (curDir != 0) {
            if (curDir != s_activeDir) {
                s_activeDir = curDir;
                s_nextNavRepeat = now + 300;
                if (curDir == 1) down = 1;        // Invertido: curDir 1 (CIMA) move a selecao para CIMA!
                else if (curDir == 2) up = 1;    // Invertido: curDir 2 (BAIXO) move a selecao para BAIXO!
                else if (curDir == 3) input = -1;
                else if (curDir == 4) input = 1;
            } else if (now >= s_nextNavRepeat) {
                s_nextNavRepeat = now + 120;
                if (curDir == 1) down = 1;
                else if (curDir == 2) up = 1;
                else if (curDir == 3) input = -1;
                else if (curDir == 4) input = 1;
            }
        } else {
            s_activeDir = 0;
        }

        static BOOL s_lastCross = FALSE;
        static BOOL s_lastBack = FALSE;
        char enter = (gp.btnCross && !s_lastCross) ? 1 : 0;
        char exit = ((gp.btnCircle || gp.btnTriangle) && !s_lastBack) ? 1 : 0;
        s_lastCross = gp.btnCross;
        s_lastBack = (gp.btnCircle || gp.btnTriangle);

        if (down || up || enter || exit || input) {
            FUNC_ProcessUserInput((void*)ADDR_FRONTEND_MENU_MANAGER, down, up, enter, exit, input);
        }

        // Silence rumble while in menu
        g_targetLeftMotor = 0;
        g_targetRightMotor = 0;
        return;
    }

    // ========================================================================
    // GAMEPLAY CONTROLS (ON FOOT & IN VEHICLE)
    // NOTE: We do NOT wipe NewState with memset, so Keyboard and Controller
    // work together seamlessly at the same time!
    // ========================================================================
    
    // 1. Inicializa os icones de botoes PS5 a partir de models\ps3btns.txd
    InitPS5Buttons();

    // 2. Garante que todas as 80 armas tenham bMoveAim e bMoveFire (andar e esquivar mirando/atirando)
    EnsureMoveWhileAiming();

    // 3. Ativa modo Joypad do console para habilitar mira automatica (Auto-Aim / Lock-On)
    *(BYTE*)0x00B6EC2E = 1;            // m_bJoypadControls = 1 (ativa CPlayerPed::FindWeaponTargetJoypad)
    pad->Mode = 1;                     // Joypad mode no CPad
    *(BYTE*)(0x00BA6748 + 0xD0) = 0;   // CMenuManager: 0 = Joypad

    void* pVeh = FUNC_FindPlayerVehicle(-1, FALSE);
    BOOL isVehicle = (pVeh != NULL);

    // ANALOG STICK MOVEMENT (Left Stick)
    // Se o controle for movido alem da deadzone, aplica a direcao do controle.
    // Se estiver neutro, mantem o teclado (WASD / setas) intacto!
    if (abs(gp.lx) > 8) {
        pad->NewState.LeftStickX = gp.lx;
    }
    if (abs(gp.ly) > 8) {
        pad->NewState.LeftStickY = gp.ly;
    }

    // ANALOG CAMERA LOOK & TARGET SWITCHING (Right Stick -> Mouse Deltas & Pad Stick)
    if (gp.rx != 0 || gp.ry != 0) {
        pad->NewState.RightStickX = gp.rx;
        pad->NewState.RightStickY = gp.ry;

        CMouseControllerState* mouseState = (CMouseControllerState*)ADDR_MOUSE_STATE;
        if (mouseState) {
            BOOL bGameInvert = (*(BYTE*)ADDR_GAME_INVERTMOUSE_Y != 0) || 
                               (*(BYTE*)ADDR_GAME_INVERTPAD_Y != 0) || 
                               (g_cfg.invertY != 0);

            float dx = (float)gp.rx * g_cfg.camSensX;
            float dy = (float)gp.ry * (bGameInvert ? -g_cfg.camSensY : g_cfg.camSensY);
            mouseState->x += dx;
            mouseState->y += dy;
        }
    }

    // PAUSE MENU (Start / Options)
    if (gp.btnStart) {
        pad->NewState.Start = 255;
    }

    // CAMERA VIEW CHANGE (PS Logo Button)
    if (gp.btnPS) {
        pad->NewState.Select = 255;
    }

    if (!isVehicle) {
        // A PÉ (ON FOOT)
        // L2: MIRA (Target Lock-on / Free Aim) -> RightShoulder1 (SEM resistência no gatilho!)
        if (gp.l2 > 30) {
            pad->NewState.RightShoulder1 = 255;
        }

        // R2: ATIRAR / DISPARAR (Fire Weapon)
        if (gp.r2 > 30) {
            pad->NewState.ButtonCircle = 255;
        }

        // BOTÕES DE FACE (Preserva teclado se pressionado)
        if (gp.btnCross)    pad->NewState.ButtonCross    = 255; // Correr / Sprint
        if (gp.btnSquare)   pad->NewState.ButtonSquare   = 255; // Pulo / Escalar
        if (gp.btnTriangle) pad->NewState.ButtonTriangle = 255; // Entrar no veículo
        if (gp.btnCircle && gp.r2 <= 30) pad->NewState.ButtonCircle = 255; // Soco / Combate corpo a corpo

        // D-PAD
        if (gp.dpadUp)    pad->NewState.DPadUp    = 255;
        if (gp.dpadDown)  pad->NewState.DPadDown  = 255;
        if (gp.dpadLeft)  pad->NewState.DPadLeft  = 255;
        if (gp.dpadRight) pad->NewState.DPadRight = 255;

        // TROCA DE ARMA RÁPIDA (L1 e R1)
        if (gp.btnL1) {
            pad->NewState.LeftShoulder2 = 255;  // Ciclo arma anterior
        }
        if (gp.btnR1) {
            pad->NewState.RightShoulder2 = 255; // Ciclo arma seguinte
        }

        // ANALÓGICOS PRESSIONADOS
        if (gp.btnL3) pad->NewState.ShockButtonL = 255; // Agachar (Duck)
        if (gp.btnR3) pad->NewState.ShockButtonR = 255; // Olhar para trás

    } else {
        // EM VEÍCULO (IN VEHICLE)
        // ACELERAÇÃO PROGRESSIVA ANALÓGICA COM R2:
        // Pressionar leve = velocidade baixa / cruzeiro.
        // Pressionar tudo = aceleração máxima!
        if (gp.r2 > 15) {
            short accel = (short)(((int)(gp.r2 - 15) * 255) / (255 - 15));
            if (accel > 255) accel = 255;
            if (accel > pad->NewState.ButtonCross) {
                pad->NewState.ButtonCross = accel;
            }
        }

        // FREIO E RÉ PROGRESSIVOS ANALÓGICOS COM L2:
        // Pressionar leve = frenagem suave.
        // Pressionar tudo = frenagem total / ré rápida!
        if (gp.l2 > 15) {
            short brake = (short)(((int)(gp.l2 - 15) * 255) / (255 - 15));
            if (brake > 255) brake = 255;
            if (brake > pad->NewState.ButtonSquare) {
                pad->NewState.ButtonSquare = brake;
            }
        }

        if (gp.btnR1) {
            pad->NewState.RightShoulder1 = 255; // Freio de mão
        }
        if (gp.btnCircle) {
            pad->NewState.ButtonCircle = 255;   // Atirar do carro
        }
        if (gp.btnTriangle) {
            pad->NewState.ButtonTriangle = 255; // Sair do carro
        }
        if (gp.btnL1) {
            pad->NewState.LeftShoulder1 = 255;  // Olhar para trás
        }
        if (gp.btnL3) {
            pad->NewState.ShockButtonL = 255;   // Buzina
        }
        if (gp.btnR3) {
            pad->NewState.ShockButtonR = 255;   // Missão veículo
        }
    }

    // ========================================================================
    // 5. VIBRAÇÃO HÁPTICA / RUMBLE REAL NO DUALSENSE
    // ========================================================================
    static DWORD s_rumbleUntil = 0;
    static BYTE s_rumbleLeft = 0;
    static BYTE s_rumbleRight = 0;
    static BYTE s_lastR2 = 0;

    // Disparo de tiro com R2: pulso de recuo seco e rápido (85ms)
    if (!isVehicle && gp.r2 > 50 && s_lastR2 <= 50) {
        s_rumbleLeft = 140;
        s_rumbleRight = 240;
        s_rumbleUntil = now + 85;
    }
    s_lastR2 = gp.r2;

    // Tremor da engine (dano, explosões, etc):
    if (pad->ShakeDur > 0) {
        int freq = (unsigned char)pad->ShakeFreq;
        BYTE engLeft = (freq > 0) ? (BYTE)min(255, freq * 4) : 180;
        BYTE engRight = (BYTE)min(255, (int)pad->ShakeDur * 4);
        if (engLeft > s_rumbleLeft) s_rumbleLeft = engLeft;
        if (engRight > s_rumbleRight) s_rumbleRight = engRight;
        if (now + 100 > s_rumbleUntil) s_rumbleUntil = now + 100;

        // Decai ShakeDur até 0 para nunca ficar travado
        int rem = (int)pad->ShakeDur - 30;
        pad->ShakeDur = (rem > 0) ? (short)rem : 0;
    }

    BYTE leftM = 0;
    BYTE rightM = 0;

    if (now < s_rumbleUntil) {
        leftM = s_rumbleLeft;
        rightM = s_rumbleRight;
    } else {
        s_rumbleLeft = 0;
        s_rumbleRight = 0;
    }

    g_targetLeftMotor = leftM;
    g_targetRightMotor = rightM;
}

// ============================================================================
// Hook: CALL-site patch at CPad::Update(pad0) call in UpdatePads
// Strategy: hook the CALL instruction at 0x00541E0B (pad0) in UpdatePads
// Our function gets: ecx=thisPad (thiscall), [esp+4]=padNum
// Then we call the original function directly (its entry is untouched)
// ============================================================================

#define ADDR_HOOK_PAD0      0x00541E0B  // call CPad::Update (pad0, ecx=0xB73458) in UpdatePads
#define FUNC_CPAD_UPDATE    0x00541C40  // original CPad::Update function entry (v1.0 US)

static BYTE g_origPad0[5];

typedef void (__attribute__((thiscall)) *tCPadUpdate)(void* thisPad, int padNum);

// Replacement for the CALL to CPad::Update(pad0)
// Called with thiscall convention: ecx=thisPad, [esp+4]=padNum
static void __attribute__((thiscall)) Hooked_CPadUpdate0(void* thisPad, int padNum) {
    static BOOL s_hookAnnounced = FALSE;
    if (!s_hookAnnounced) {
        s_hookAnnounced = TRUE;
        LogMsg("[Hook] CPad::Update(padNum=%d) ACTIVELY CALLED BY GTA ENGINE! Hook is alive!\n", padNum);
    }

    // Call original CPad::Update (its entry is untouched, we only patched the CALL site)
    ((tCPadUpdate)FUNC_CPAD_UPDATE)(thisPad, padNum);

    // Custom DualSense processing only for pad 0
    if (padNum == 0) {
        ProcessCustomController((CPad*)thisPad);
    }
}

static void InstallHook(void) {
    DWORD oldProtect;
    
    // 1. Hook CALL em CPad::Update (pad 0)
    if (VirtualProtect((LPVOID)ADDR_HOOK_PAD0, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(g_origPad0, (void*)ADDR_HOOK_PAD0, 5);
        DWORD myAddr = (DWORD)Hooked_CPadUpdate0;
        DWORD rel    = myAddr - (ADDR_HOOK_PAD0 + 5);
        *(BYTE* )ADDR_HOOK_PAD0       = 0xE8;
        *(DWORD*)(ADDR_HOOK_PAD0 + 1) = rel;
        VirtualProtect((LPVOID)ADDR_HOOK_PAD0, 5, oldProtect, &oldProtect);
        LogMsg("[Hook] Pad0 CALL hook OK: 0x%08X -> our fn 0x%08X\n", ADDR_HOOK_PAD0, myAddr);
    } else {
        LogMsg("[Hook] FAILED VirtualProtect at 0x%08X\n", ADDR_HOOK_PAD0);
    }
}

// ============================================================================
// DLL Entry Point
// ============================================================================

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        LogMsg("====================================================\n");
        LogMsg(" Universal Controller Mod v2.6.0 (DualSense & DualShock 4 Engine Direct)\n");
        LogMsg(" PS5 Icons + Auto-Aim Lock-On + Move While Aiming\n");
        LogMsg(" Built exclusively for GTA San Andreas\n");
        LogMsg("====================================================\n");
        LoadConfig();
        // Start DualSense HID thread immediately on DLL load (not lazy)
        LogMsg("[Init] Starting DualSense HID worker thread...\n");
        CreateThread(NULL, 0, DualSenseWorkerThread, NULL, 0, NULL);
        // Install game hooks
        InstallHook();
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        g_dsRunning = FALSE;
        Sleep(50);
    }
    return TRUE;
}
