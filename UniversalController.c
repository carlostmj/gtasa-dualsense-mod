#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
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

typedef void* (*tFindPlayerPed)(int playerId);
#define FUNC_FindPlayerPed ((tFindPlayerPed)0x0056E210)

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
    // Disabled to prevent TXD loading crash during startup
    return;
}

typedef struct {
    float x;
    float y;
} CVector2D_t;

// Hook para CTaskSimpleUseGun::ControlGunMove (0x0061E0C0)
// Corrige o bug original do GTA SA onde m_MoveCmd interpolado com 0.07 era zerado
// no mesmo frame por SetMoveAnim (moveCmdAbsSum < 0.1f), travando CJ no lugar
// se o jogador ja estivesse andando ao apertar L2 para mirar.
static BOOL __attribute__((thiscall)) Hooked_ControlGunMove(void* thisTask, const CVector2D_t* moveDir) {
    if (thisTask && moveDir) {
        *(float*)((BYTE*)thisTask + 0x14) = moveDir->x;
        *(float*)((BYTE*)thisTask + 0x18) = moveDir->y;
        *(BYTE*)((BYTE*)thisTask + 0x0A) = 1; // m_HasMoveControl = true
    }
    return TRUE;
}

static void EnsureMoveWhileAiming(void) {
    static DWORD s_lastCheck = 0;
    DWORD now = GetTickCount();
    if (now - s_lastCheck < 1000) return; // Verifica a cada 1 segundo
    s_lastCheck = now;

    DWORD oldProtect;
    if (VirtualProtect((LPVOID)0x00C8AAB8, 80 * 0x70, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        for (int i = 0; i < 80; i++) {
            BYTE* pWInfo = (BYTE*)(0x00C8AAB8 + i * 0x70);
            int modelId = *(int*)(pWInfo + 0x0C);
            DWORD* pFlags = (DWORD*)(pWInfo + 0x18);
            float* pMoveSpeed = (float*)(pWInfo + 0x3C);

            // Ativa: bCanAim (0x01) | bMoveAim (0x10) | bMoveFire (0x20)
            *pFlags |= 0x31;

            // NÃO remove b1stPerson de armas com mira telescópica ou câmera (Sniper 358, RPG 359, Heatseeker 360, Câmera 367)
            if (modelId == 358 || modelId == 359 || modelId == 360 || modelId == 367) {
                *pFlags |= 0x04; // Mantém o Scope telescópico da Sniper e RPG com retícula e zoom 100% funcionais!
            } else {
                *pFlags &= ~0x04; // Armas convencionais usam mira livre em 3ª pessoa sobre o ombro
            }

            // Armas pesadas e rifles de duas mãos (M4, AK-47, Shotguns, MP5, etc.)
            // NÃO podem ter bAimWithArm (0x02), senão a engine não chama PlayerControlZeldaWeapon!
            // Somente armas de uma mão e a Sniper mantêm suas configurações originais.
            if (modelId != 346 && modelId != 350 && modelId != 352 && modelId != 372 && modelId != 358) {
                *pFlags &= ~0x02;
            }

            // Garante velocidade de movimento para a animação mover o personagem
            if (*pMoveSpeed < 0.85f) {
                *pMoveSpeed = 0.85f;
            }
        }
        VirtualProtect((LPVOID)0x00C8AAB8, 80 * 0x70, oldProtect, &oldProtect);
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
static DWORD g_outputReportLength = 0;

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
                            PHIDP_PREPARSED_DATA pPreparsed = NULL;
                            if (HidD_GetPreparsedData(h, &pPreparsed)) {
                                HIDP_CAPS caps;
                                if (HidP_GetCaps(pPreparsed, &caps) == HIDP_STATUS_SUCCESS) {
                                    g_outputReportLength = caps.OutputReportByteLength;
                                    LogMsg("[SonyHID] Caps: OutputReportByteLength = %u\n", g_outputReportLength);
                                }
                                HidD_FreePreparsedData(pPreparsed);
                            }

                            if (attr.ProductID == 0x0CE6 || attr.ProductID == 0x0DF2) {
                                g_sonyDevType = SONY_DEV_DUALSENSE;
                                if (strstr(path, "{e0cbf06c") || strstr(path, "bth") || strstr(path, "BTH")) {
                                    g_sonyIsBluetooth = TRUE;
                                }
                                LogMsg("[SonyHID] Auto-detected Sony DualSense (PS5) (VID=0x%04X, PID=0x%04X, Conn=%s)\n",
                                       attr.VendorID, attr.ProductID, g_sonyIsBluetooth ? "Bluetooth" : "USB");
                                hFound = h;
                                free(pDetail);
                                break;
                            } else if (attr.ProductID == 0x05C4 || attr.ProductID == 0x09CC) {
                                g_sonyDevType = SONY_DEV_DUALSHOCK4;
                                if (strstr(path, "{e0cbf06c") || strstr(path, "bth") || strstr(path, "BTH")) {
                                    g_sonyIsBluetooth = TRUE;
                                }
                                LogMsg("[SonyHID] Auto-detected Sony DualShock 4 (PS4) (VID=0x%04X, PID=0x%04X, Conn=%s)\n",
                                       attr.VendorID, attr.ProductID, g_sonyIsBluetooth ? "Bluetooth" : "USB");
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

static volatile BYTE g_triggerR2Mode = 0;
static volatile BYTE g_triggerR2Param1 = 0;
static volatile BYTE g_triggerR2Param2 = 0;
static volatile BYTE g_triggerL2Mode = 0;
static volatile BYTE g_triggerL2Param1 = 0;
static volatile BYTE g_triggerL2Param2 = 0;

static volatile BYTE g_lastSentR2Mode = 0xFF;
static volatile BYTE g_lastSentR2P1 = 0xFF;
static volatile BYTE g_lastSentR2P2 = 0xFF;
static volatile BYTE g_lastSentL2Mode = 0xFF;
static volatile BYTE g_lastSentL2P1 = 0xFF;
static volatile BYTE g_lastSentL2P2 = 0xFF;
static volatile BYTE g_lastSentRed = 0xFF;
static volatile BYTE g_lastSentGreen = 0xFF;
static volatile BYTE g_lastSentBlue = 0xFF;

static void SendDualSenseHardwareReport(HANDLE hDev, BYTE leftMotor, BYTE rightMotor) {
    if (g_sonyDevType == SONY_DEV_DUALSENSE) {
        if (!g_sonyIsBluetooth) {
            // ================================================================
            // DUALSENSE USB MODE (Report ID 0x02, 64 bytes standard)
            // ================================================================
            BYTE report[64];
            memset(report, 0, sizeof(report));

            report[0] = 0x02; // Report ID
            report[1] = 0xFF; // valid_flag0 (motors + triggers)
            report[2] = 0x1 | 0x2 | 0x4 | 0x10 | 0x40; // valid_flag1 (LEDs + lightbar)
            report[3] = rightMotor; // High-frequency weak rumble (0-255)
            report[4] = leftMotor;  // Low-frequency strong rumble (0-255)

            // R2 (Right Trigger):
            report[11] = g_triggerR2Mode;
            report[12] = g_triggerR2Param1;
            report[13] = g_triggerR2Param2;

            // L2 (Left Trigger):
            report[22] = g_triggerL2Mode;
            report[23] = g_triggerL2Param1;
            report[24] = g_triggerL2Param2;

            // Lightbar & LEDs:
            report[39] = 0x02; // Uninterruptable LED
            report[42] = 0x00; // Pulse options
            report[43] = 0x00; // Brightness High
            report[44] = 0x04; // Player 1 LED
            report[45] = g_lightbarRed;
            report[46] = g_lightbarGreen;
            report[47] = g_lightbarBlue;

            DWORD targetLen = (g_outputReportLength >= 48) ? g_outputReportLength : 64;
            DWORD written = 0;
            if (!WriteFile(hDev, report, targetLen, &written, NULL)) {
                HidD_SetOutputReport(hDev, report, targetLen);
            }
        } else {
            // ================================================================
            // DUALSENSE BLUETOOTH MODE (Report ID 0x31, 78 bytes with CRC)
            // ================================================================
            BYTE report[78];
            memset(report, 0, sizeof(report));

            report[0] = 0x31; // Report ID
            report[1] = 0x02; // Tag / Sequence
            report[2] = 0xFF; // valid_flag0 (motors + triggers)
            report[3] = 0x1 | 0x2 | 0x4 | 0x10 | 0x40; // valid_flag1 (LEDs + lightbar)
            report[4] = rightMotor; // High-frequency weak rumble
            report[5] = leftMotor;  // Low-frequency strong rumble

            // R2 (Right Trigger):
            report[12] = g_triggerR2Mode;
            report[13] = g_triggerR2Param1;
            report[14] = g_triggerR2Param2;

            // L2 (Left Trigger):
            report[23] = g_triggerL2Mode;
            report[24] = g_triggerL2Param1;
            report[25] = g_triggerL2Param2;

            // Lightbar & LEDs:
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
            if (!WriteFile(hDev, report, 78, &written, NULL)) {
                HidD_SetOutputReport(hDev, report, 78);
            }
        }
    } else if (g_sonyDevType == SONY_DEV_DUALSHOCK4) {
        if (g_sonyIsBluetooth) {
            BYTE report[78];
            memset(report, 0, sizeof(report));
            report[0] = 0x11;
            report[1] = 0xC0 | 0x04;
            report[3] = 0x03; // Enable rumble + lightbar
            report[6] = rightMotor;
            report[7] = leftMotor;
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
            if (!WriteFile(hDev, report, 78, &written, NULL)) {
                HidD_SetOutputReport(hDev, report, 78);
            }
        } else {
            BYTE report[32];
            memset(report, 0, sizeof(report));
            report[0] = 0x05;
            report[1] = 0x07;
            report[4] = rightMotor;
            report[5] = leftMotor;
            report[6] = g_lightbarRed;
            report[7] = g_lightbarGreen;
            report[8] = g_lightbarBlue;

            DWORD targetLen = (g_outputReportLength >= 32) ? g_outputReportLength : 32;
            DWORD written = 0;
            if (!WriteFile(hDev, report, targetLen, &written, NULL)) {
                HidD_SetOutputReport(hDev, report, targetLen);
            }
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
                g_lastSentR2Mode = 0xFF;
                g_lastSentL2Mode = 0xFF;
                g_lastSentRed = 0xFF;
                g_lastOutputTick = GetTickCount();
                SendDualSenseHardwareReport(g_hDualSense, 0, 0);
            } else {
                Sleep(1000);
                continue;
            }
        }

        // 1. Output update (Rumble, Triggers & Lightbar heartbeat)
        DWORD now = GetTickCount();
        BOOL motorChanged = (g_targetLeftMotor != g_lastSentLeftMotor || g_targetRightMotor != g_lastSentRightMotor);
        BOOL triggerChanged = (g_triggerR2Mode != g_lastSentR2Mode || g_triggerR2Param1 != g_lastSentR2P1 ||
                               g_triggerR2Param2 != g_lastSentR2P2 || g_triggerL2Mode != g_lastSentL2Mode ||
                               g_triggerL2Param1 != g_lastSentL2P1 || g_triggerL2Param2 != g_lastSentL2P2);
        BOOL lightbarChanged = (g_lightbarRed != g_lastSentRed || g_lightbarGreen != g_lastSentGreen || g_lightbarBlue != g_lastSentBlue);
        BOOL heartbeat = (now - g_lastOutputTick) >= 150;

        if (motorChanged || triggerChanged || lightbarChanged || heartbeat) {
            g_lastSentLeftMotor = g_targetLeftMotor;
            g_lastSentRightMotor = g_targetRightMotor;
            g_lastSentR2Mode = g_triggerR2Mode;
            g_lastSentR2P1 = g_triggerR2Param1;
            g_lastSentR2P2 = g_triggerR2Param2;
            g_lastSentL2Mode = g_triggerL2Mode;
            g_lastSentL2P1 = g_triggerL2Param1;
            g_lastSentL2P2 = g_triggerL2Param2;
            g_lastSentRed = g_lightbarRed;
            g_lastSentGreen = g_lightbarGreen;
            g_lastSentBlue = g_lightbarBlue;
            g_lastOutputTick = now;
            SendDualSenseHardwareReport(g_hDualSense, g_lastSentLeftMotor, g_lastSentRightMotor);
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
        SendDualSenseHardwareReport(g_hDualSense, 0, 0);
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

static bool s_customSniperZoomIn = false;
static bool s_customSniperZoomOut = false;

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
    static DWORD s_rumbleUntil = 0;
    static BYTE s_rumbleLeft = 0;
    static BYTE s_rumbleRight = 0;
    static BYTE s_lastR2 = 0;

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

    // ========================================================================
    // SISTEMA DE LIGHTBAR DINÂMICA (PS5 DUALSENSE & DUALSHOCK 4)
    // ========================================================================
    void* pPlayerPed = FUNC_FindPlayerPed(-1);
    if (pPlayerPed) {
        DWORD wantedLevel = 0;
        BYTE* pPlayerData = *(BYTE**)((BYTE*)pPlayerPed + 0x480);
        if (pPlayerData) {
            BYTE* pWanted = *(BYTE**)pPlayerData;
            if (pWanted) {
                wantedLevel = *(DWORD*)(pWanted + 0x2C);
            }
        }

        if (wantedLevel > 0) {
            // Perseguição Policial: Giroflex piscando alternado Vermelho / Azul!
            if ((now / 180) % 2 == 0) {
                g_lightbarRed = 255; g_lightbarGreen = 0; g_lightbarBlue = 0;
            } else {
                g_lightbarRed = 0; g_lightbarGreen = 40; g_lightbarBlue = 255;
            }
        } else {
            // Status de Saúde do CJ
            float health = *(float*)((BYTE*)pPlayerPed + 0x540);
            if (health > 70.0f) {
                // Vida Cheia / Saudável: Verde Grove Street
                g_lightbarRed = 30; g_lightbarGreen = 220; g_lightbarBlue = 40;
            } else if (health > 35.0f) {
                // Vida Média / Ferido: Laranja / Amarelo
                g_lightbarRed = 255; g_lightbarGreen = 140; g_lightbarBlue = 0;
            } else if (health > 15.0f) {
                // Vida Baixa / Perigo: Vermelho
                g_lightbarRed = 255; g_lightbarGreen = 20; g_lightbarBlue = 20;
            } else {
                // Vida Crítica (< 15%): Batimento cardíaco pulsando em vermelho
                if ((now / 250) % 2 == 0) {
                    g_lightbarRed = 255; g_lightbarGreen = 0; g_lightbarBlue = 0;
                } else {
                    g_lightbarRed = 40; g_lightbarGreen = 0; g_lightbarBlue = 0;
                }
            }
        }
    } else {
        // Padrão PS5: Azul Suave
        g_lightbarRed = 0; g_lightbarGreen = 120; g_lightbarBlue = 255;
    }

    // ========================================================================
    // PAUSE MENU (START / OPTIONS / PLAY) & MAPA (SHARE / TOUCHPAD)
    // ========================================================================
    static BOOL s_lastStartBtn = FALSE;
    BOOL startEdge = (gp.btnStart && !s_lastStartBtn);
    s_lastStartBtn = gp.btnStart;

    static BOOL s_lastShareBtn = FALSE;
    BOOL shareBtn = (gp.btnShare || gp.btnTouch);
    BOOL shareEdge = (shareBtn && !s_lastShareBtn);
    s_lastShareBtn = shareBtn;

    // START (Options / Play) -> Alterna entre o Menu Principal de Opções e o Jogo
    if (startEdge) {
        LogMsg("[Menu] START pressed! Active=%d, Page=%d\n", isMenuActive, curMenuPage);
        if (!isMenuActive) {
            *(BYTE*)0x00BA677B = 1; // m_bStartUpFrontEndRequested = true (Abre Menu)
            *(char*)0x00BA68A5 = 0; // m_nCurrentMenuPage = 0 (Opções)
        } else {
            *(BYTE*)0x00BA677A = 1; // m_bShutDownFrontEndRequested = true (Fecha Menu)
        }
        pad->NewState.Start = 255;
    }

    // SHARE (Create / Touchpad) -> Abre diretamente o Mapa do Jogo
    if (shareEdge) {
        LogMsg("[Menu] SHARE/TOUCHPAD pressed! Active=%d, Page=%d\n", isMenuActive, curMenuPage);
        if (!isMenuActive) {
            *(BYTE*)0x00BA677B = 1; // m_bStartUpFrontEndRequested = true
            *(char*)0x00BA68A5 = 5; // m_nCurrentMenuPage = 5 (Mapa Direto)
            *(BYTE*)0x00BA67A1 = 1; // m_bDrawRadarOrMap = true
        } else {
            if (curMenuPage == 5) {
                *(BYTE*)0x00BA677A = 1; // Fecha o mapa e volta ao jogo
            } else {
                *(char*)0x00BA68A5 = 5; // Pula de outra aba direto para o Mapa
                *(BYTE*)0x00BA67A1 = 1;
            }
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
        g_triggerR2Mode = 0; g_triggerR2Param1 = 0; g_triggerR2Param2 = 0;
        g_triggerL2Mode = 0; g_triggerL2Param1 = 0; g_triggerL2Param2 = 0;
        
        // Mantém suporte para fechar o menu com Start
        if (gp.btnStart) pad->NewState.Start = 255;

        // Navegação precisa de itens do Menu via CMenuManager::ProcessUserInput
        // NOTA: Não escrevemos em pad->NewState.DPad* para evitar pulo duplo de opções nos submenus!
        static DWORD s_nextNavRepeat = 0;
        static int s_activeDir = 0;

        int curDir = 0;
        if (gp.dpadUp || gp.ly < -60) curDir = 1;       // CIMA
        else if (gp.dpadDown || gp.ly > 60) curDir = 2; // BAIXO
        else if (gp.dpadLeft || gp.lx < -60) curDir = 3;// ESQUERDA
        else if (gp.dpadRight || gp.lx > 60) curDir = 4;// DIREITA

        char down = 0, up = 0, input = 0;
        if (curDir != 0) {
            if (curDir != s_activeDir) {
                s_activeDir = curDir;
                s_nextNavRepeat = now + 350;     // Delay inicial de 350ms para evitar pulo acidental
                if (curDir == 1) up = 1;         // CIMA -> decrementa índice (sobe 1 item)
                else if (curDir == 2) down = 1;  // BAIXO -> incrementa índice (desce 1 item)
                else if (curDir == 3) input = -1;// ESQUERDA -> slider / opção anterior
                else if (curDir == 4) input = 1; // DIREITA -> slider / próxima opção
            } else if (now >= s_nextNavRepeat) {
                s_nextNavRepeat = now + 160;     // Taxa de repetição contínua (160ms) ao segurar
                if (curDir == 1) up = 1;
                else if (curDir == 2) down = 1;
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
        s_customSniperZoomIn = false;
        s_customSniperZoomOut = false;
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

    // 3. Mantém suporte simultâneo a Teclado + Mouse e Controle
    *(BYTE*)0x00B6EC2E = 1;            // CCamera::m_bUseMouse3rdPerson: 1 = Câmera livre do mouse sempre ativa
    pad->Mode = 0;                     // Mode 0: Layout padrão
    *(BYTE*)(0x00BA6748 + 0xD0) = 1;   // CMenuManager: 1 = Mouse + Teclas
    *(BYTE*)0x00BA6818 = 1;            // ControlsManager: 1 = Mouse + Teclas

    void* pVeh = FUNC_FindPlayerVehicle(-1, FALSE);
    BOOL isVehicle = (pVeh != NULL);
    BOOL isBicycle = FALSE;

    if (isVehicle) {
        // Checa se o veiculo e bicicleta (BMX, Bike, Mountain Bike)
        int subClass = *(int*)((BYTE*)pVeh + 0x5A0);
        WORD modelId = *(WORD*)((BYTE*)pVeh + 0x22);
        if (subClass == 10 || modelId == 481 || modelId == 509 || modelId == 510) {
            isBicycle = TRUE;
        }
    }

    // ANALOG STICK MOVEMENT (Left Stick) vs TECLADO (WASD):
    // Deadzone de 25 para evitar que ruído do controle na mesa anule as teclas W, A, S, D
    int deadzone = 25;
    if (abs(gp.lx) > deadzone) {
        if (abs(gp.lx) >= abs(pad->NewState.LeftStickX)) {
            pad->NewState.LeftStickX = gp.lx;
        }
    }
    if (abs(gp.ly) > deadzone) {
        if (abs(gp.ly) >= abs(pad->NewState.LeftStickY)) {
            pad->NewState.LeftStickY = gp.ly;
        }
    }

    // ANALOG CAMERA LOOK (Right Stick -> Mouse Deltas & Pad Stick)
    // Só envia deltas para o mouse se o analógico direito estiver sendo movido intencionalmente
    if (abs(gp.rx) > 15 || abs(gp.ry) > 15) {
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

    // PAUSE MENU (Start / Options) - handled via startEdge above

    // CAMERA VIEW CHANGE (PS Logo Button)
    if (gp.btnPS) {
        pad->NewState.Select = 255;
    }

    if (!isVehicle) {
        // ====================================================================
        // A PÉ (ON FOOT)
        // ====================================================================
        
        // --------------------------------------------------------------------
        // MIRA 100% LIVRE (L2 no Controle OU Botão Direito do Mouse):
        // Puxa a mira sobre o ombro com a retícula (crosshair) na tela.
        // O jogador tem controle total da mira pelo Analógico Direito e pelo Mouse.
        // --------------------------------------------------------------------
        if (gp.l2 > 30) {
            pad->NewState.RightShoulder1 = 255;
        }

        // R2: ATIRAR / DISPARAR (Fire Weapon)
        if (gp.r2 > 30) {
            pad->NewState.ButtonCircle = 255;
        }

        // REMOVE 100% A MIRA AUTOMÁTICA (Sem cones/triângulos verdes girando e sem lock-on)
        // Garante que CJ nunca trave mira em pedestres e a câmera nunca fique presa
        void* pPlayer = FUNC_FindPlayerPed(-1);
        if (pPlayer) {
            void* pTarget = *(void**)((BYTE*)pPlayer + 0x79C);
            if (pTarget) {
                ((void (__attribute__((thiscall)) *)(void*))0x0060D5A0)(pPlayer); // ClearWeaponTarget
            }
        }

        // --------------------------------------------------------------------
        // GATILHOS ADAPTÁVEIS A PÉ:
        // --------------------------------------------------------------------
        float timeCanRun = 10.0f;
        if (pPlayerPed) {
            BYTE* pPlayerData = *(BYTE**)((BYTE*)pPlayerPed + 0x480);
            if (pPlayerData) {
                timeCanRun = *(float*)(pPlayerData + 0x18);
            }
        }

        if (IsPlayerHoldingFirearm()) {
            // Arma de fogo: Gatilho mecânico com duplo estágio (resistência aos 25% e clique aos 175)
            g_triggerR2Mode = 0x02; // Section Resistance
            g_triggerR2Param1 = 25; // Ponto do clique
            g_triggerR2Param2 = 175;// Força mecânica do gatilho
            g_triggerL2Mode = 0x00; // L2 100% livre para puxar a mira livremente!
        } else if (timeCanRun <= 1.0f) {
            // CJ Exausto / Sem fôlego (Stamina zerada): Gatilhos pesados e trêmulos simulando cansaço físico
            BOOL tiredPulse = ((now / 150) % 2 == 0);
            g_triggerR2Mode = 0x01; // Continuous Resistance
            g_triggerR2Param1 = 10;
            g_triggerR2Param2 = tiredPulse ? 190 : 70;
            g_triggerL2Mode = 0x01;
            g_triggerL2Param1 = 10;
            g_triggerL2Param2 = tiredPulse ? 190 : 70;
            if (tiredPulse) {
                if (s_rumbleLeft < 80) s_rumbleLeft = 80;
                if (now + 75 > s_rumbleUntil) s_rumbleUntil = now + 75;
            }
        } else {
            // Desarmado / Faca / Normal: Gatilhos 100% livres e macios
            g_triggerR2Mode = 0x00; g_triggerR2Param1 = 0; g_triggerR2Param2 = 0;
            g_triggerL2Mode = 0x00; g_triggerL2Param1 = 0; g_triggerL2Param2 = 0;
        }

        // BOTÕES DE FACE (Preserva teclado se pressionado)
        if (gp.btnCross)    pad->NewState.ButtonCross    = 255; // Correr / Sprint
        if (gp.btnSquare)   pad->NewState.ButtonSquare   = 255; // Pulo / Escalar
        if (gp.btnTriangle) pad->NewState.ButtonTriangle = 255; // Entrar no veículo
        if (gp.btnCircle && gp.r2 <= 30) pad->NewState.ButtonCircle = 255; // Soco / Combate corpo a corpo

        // --------------------------------------------------------------------
        // ZOOM DA SNIPER / CÂMERA / ROCKET LAUNCHER COM D-PAD (SETAS CIMA / BAIXO)
        // --------------------------------------------------------------------
        BYTE activeCam = *(BYTE*)0x00B6F081;
        if (activeCam > 2) activeCam = 0;
        BYTE* pCam = (BYTE*)(0x00B6F19C + activeCam * 0x238);
        short camMode = *(short*)(pCam + 0x0C);
        BOOL isScopedCam = (camMode == 39 || camMode == 46 || camMode == 7);

        BOOL isScopedWeapon = FALSE;
        BYTE* pPedAim = *(BYTE**)0x00B7CD98; // CWorld::Players[0].m_pPed
        if (pPedAim) {
            BYTE slot = *(BYTE*)(pPedAim + 0x718);
            if (slot <= 12) {
                DWORD weaponType = *(DWORD*)(pPedAim + 0x5A0 + slot * 0x1C);
                if (weaponType == 34 || weaponType == 43 || weaponType == 35 || weaponType == 36) {
                    isScopedWeapon = TRUE;
                }
            }
        }

        BOOL isAimingScoped = isScopedCam || (isScopedWeapon && (gp.l2 > 30 || pad->NewState.RightShoulder1 > 0));

        if (isAimingScoped) {
            if (gp.dpadUp) {
                s_customSniperZoomIn = true;
                s_customSniperZoomOut = false;
                // Feedback tátil no limite de zoom máximo (15.0 graus)
                float curFov = *(float*)(pCam + 0xB4);
                if (curFov <= 15.2f) {
                    if (s_rumbleRight < 90) s_rumbleRight = 90;
                    if (now + 40 > s_rumbleUntil) s_rumbleUntil = now + 40;
                }
            } else if (gp.dpadDown) {
                s_customSniperZoomIn = false;
                s_customSniperZoomOut = true;
                // Feedback tátil no limite de zoom mínimo (70.0 graus)
                float curFov = *(float*)(pCam + 0xB4);
                if (curFov >= 69.8f) {
                    if (s_rumbleLeft < 90) s_rumbleLeft = 90;
                    if (now + 40 > s_rumbleUntil) s_rumbleUntil = now + 40;
                }
            } else {
                s_customSniperZoomIn = false;
                s_customSniperZoomOut = false;
            }
        } else {
            s_customSniperZoomIn = false;
            s_customSniperZoomOut = false;
        }

        // D-PAD:
        // Ao mirar com arma telescópica, NÃO passa Up/Down para a engine para não falar comandos de gangue
        if (!isAimingScoped) {
            if (gp.dpadUp)    pad->NewState.DPadUp    = 255;
            if (gp.dpadDown)  pad->NewState.DPadDown  = 255;
        }
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
        // ====================================================================
        // EM VEÍCULO (IN VEHICLE)
        // ====================================================================
        s_customSniperZoomIn = false;
        s_customSniperZoomOut = false;
        float vehHealth = *(float*)((BYTE*)pVeh + 0x4C0);
        BYTE curGear = *(BYTE*)((BYTE*)pVeh + 0x4B4) & 7; // VALIDATE_OFFSET(CVehicle, m_nCurrentGear, 0x4B4)
        WORD modelId = *(WORD*)((BYTE*)pVeh + 0x22);

        // Velocidade real do veículo para cálculo dinâmico de ABS:
        float vx = *(float*)((BYTE*)pVeh + 0x44);
        float vy = *(float*)((BYTE*)pVeh + 0x48);
        float vz = *(float*)((BYTE*)pVeh + 0x4C);
        float speedSq = (vx * vx) + (vy * vy) + (vz * vz);
        BOOL isMovingFast = (speedSq > 0.035f); // Acima de ~20 km/h

        // A. TROCA DE MARCHA (GEAR SHIFT SNAP NO R2 + TRANCO MECÂNICO)
        static void* s_lastVeh = NULL;
        static BYTE s_lastGear = 0;
        static DWORD s_gearSnapUntil = 0;
        if (pVeh != s_lastVeh) {
            s_lastVeh = pVeh;
            s_lastGear = curGear;
        }

        if (!isBicycle && curGear != s_lastGear) {
            if (curGear >= 1 && s_lastGear >= 1) {
                s_gearSnapUntil = now + 140; // Tranco de 140ms no R2
                if (s_rumbleRight < 190) s_rumbleRight = 190;
                if (s_rumbleLeft < 120) s_rumbleLeft = 120;
                if (now + 100 > s_rumbleUntil) s_rumbleUntil = now + 100;
            }
            s_lastGear = curGear;
        }

        // B. DANO CRÍTICO DO MOTOR (HP < 300: MOTOR FALHANDO / BATENDO BIELA)
        if (vehHealth > 0.0f && vehHealth < 300.0f) {
            // Vibração irregular intermitente simulando falha de ignição
            int phase = (now / 150) % 4;
            if (phase == 0) {
                if (s_rumbleLeft < 140) s_rumbleLeft = 140;
                if (s_rumbleRight < 70) s_rumbleRight = 70;
                if (now + 80 > s_rumbleUntil) s_rumbleUntil = now + 80;
            } else if (phase == 2) {
                if (s_rumbleLeft < 85) s_rumbleLeft = 85;
                if (now + 60 > s_rumbleUntil) s_rumbleUntil = now + 60;
            }
        }

        // C. CONFIGURAÇÃO DE GATILHOS ADAPTÁVEIS NO VEÍCULO:
        if (isBicycle) {
            float timeCanRun = 10.0f;
            if (pPlayerPed) {
                BYTE* pPlayerData = *(BYTE**)((BYTE*)pPlayerPed + 0x480);
                if (pPlayerData) timeCanRun = *(float*)(pPlayerData + 0x18);
            }
            if (timeCanRun <= 1.0f) {
                BOOL tiredPulse = ((now / 150) % 2 == 0);
                g_triggerR2Mode = 0x01; g_triggerR2Param1 = 10; g_triggerR2Param2 = tiredPulse ? 180 : 70;
                g_triggerL2Mode = 0x01; g_triggerL2Param1 = 10; g_triggerL2Param2 = tiredPulse ? 180 : 70;
                if (tiredPulse) {
                    if (s_rumbleLeft < 80) s_rumbleLeft = 80;
                    if (now + 75 > s_rumbleUntil) s_rumbleUntil = now + 75;
                }
            } else {
                g_triggerR2Mode = 0; g_triggerR2Param1 = 0; g_triggerR2Param2 = 0;
                g_triggerL2Mode = 0; g_triggerL2Param1 = 0; g_triggerL2Param2 = 0;
            }

            // BICICLETA (BMX, Mountain Bike, Bike):
            // Pedalar e X, e O para frear/voltar! R2 e L2 NAO atuam na bicicleta!
            if (gp.btnCross) {
                pad->NewState.ButtonCross = 255;  // Pedalar / Acelerar bicicleta
            }
            if (gp.btnCircle) {
                pad->NewState.ButtonSquare = 255; // Freio / Ré da bicicleta
            }
            if (gp.btnSquare) {
                pad->NewState.RightShoulder1 = 255; // Bunny Hop / Pulo da bike
            }
            if (gp.btnTriangle) {
                pad->NewState.ButtonTriangle = 255; // Sair da bike
            }
            if (gp.btnL3) {
                pad->NewState.ShockButtonL = 255;   // Campainha da bike
            }
        } else {
            // R2 (Aceleração):
            if (now < s_gearSnapUntil) {
                // Snap mecânico de troca de marcha no R2!
                g_triggerR2Mode = 0x02; // Rigid Stop
                g_triggerR2Param1 = 10;
                g_triggerR2Param2 = 230;
            } else {
                // Aceleração suave e progressiva
                g_triggerR2Mode = 0x00;
                g_triggerR2Param1 = 0;
                g_triggerR2Param2 = 0;
            }

            // L2 (Freio / ABS):
            // Veículos pesados (Caminhões, Ônibus, Tanques, etc.)
            BOOL isHeavy = (modelId == 403 || modelId == 406 || modelId == 407 || modelId == 408 ||
                            modelId == 431 || modelId == 432 || modelId == 437 || modelId == 443 ||
                            modelId == 514 || modelId == 515 || modelId == 524 || modelId == 573);

            if (gp.l2 > 70 && isMovingFast) {
                // Frenagem forte em movimento: Simulação de ABS pulsante no pedal/gatilho L2!
                BOOL absPulse = ((now / 60) % 2 == 0);
                g_triggerL2Mode = 0x01; // Continuous Resistance
                g_triggerL2Param1 = absPulse ? 35 : 10;
                g_triggerL2Param2 = absPulse ? 230 : 60;
                if (absPulse) {
                    if (s_rumbleLeft < 130) s_rumbleLeft = 130;
                    if (now + 50 > s_rumbleUntil) s_rumbleUntil = now + 50;
                }
            } else if (isHeavy && gp.l2 > 10) {
                // Caminhão pesado: freio mais rígido e com maior resistência mecânica
                g_triggerL2Mode = 0x01;
                g_triggerL2Param1 = 15;
                g_triggerL2Param2 = 180;
            } else {
                // Carro normal: freio suave e macio
                g_triggerL2Mode = 0x00;
                g_triggerL2Param1 = 0;
                g_triggerL2Param2 = 0;
            }

            // CARROS E MOTOS:
            // ACELERAÇÃO PROGRESSIVA ANALÓGICA COM R2:
            if (gp.r2 > 15) {
                short accel = (short)(((int)(gp.r2 - 15) * 255) / (255 - 15));
                if (accel > 255) accel = 255;
                if (accel > pad->NewState.ButtonCross) {
                    pad->NewState.ButtonCross = accel;
                }
            }

            // FREIO E RÉ PROGRESSIVOS ANALÓGICOS COM L2:
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
    }

    // ========================================================================
    // 5. VIBRAÇÃO HÁPTICA / RUMBLE REAL NO DUALSENSE
    // ========================================================================
    // Disparo de tiro com R2: pulso de recuo seco e rápido (95ms)
    if (!isVehicle && gp.r2 > 50 && s_lastR2 <= 50) {
        s_rumbleLeft = 180;
        s_rumbleRight = 255;
        s_rumbleUntil = now + 95;
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


// ============================================================================
// Sniper Rifle & Camera Zoom Hooks (D-Pad UP / DOWN Zooming)
// ============================================================================
#define ADDR_HOOK_SNIPER_ZOOM_IN   0x00540B30
#define ADDR_HOOK_SNIPER_ZOOM_OUT  0x00540B80


static bool Orig_SniperZoomIn(CPad* pad) {
    if (!pad) return false;
    if (pad->DisablePlayerControls != 0) return false;
    short mode = pad->Mode;
    if (mode == 2) {
        return (pad->NewState.ButtonTriangle != 0);
    }
    if (mode >= 0 && mode <= 3) {
        return (pad->NewState.LeftShoulder2 != 0 || pad->NewState.ButtonSquare != 0);
    }
    return false;
}

static bool Orig_SniperZoomOut(CPad* pad) {
    if (!pad) return false;
    if (pad->DisablePlayerControls != 0) return false;
    short mode = pad->Mode;
    if (mode == 2) {
        return (pad->NewState.ButtonSquare != 0);
    }
    if (mode >= 0 && mode <= 3) {
        return (pad->NewState.RightShoulder2 != 0 || pad->NewState.ButtonCross != 0);
    }
    return false;
}

static bool __attribute__((thiscall)) Hooked_SniperZoomIn(void* thisPad) {
    CPad* pad = (CPad*)thisPad;
    if (pad == (CPad*)0x00B73458 && s_customSniperZoomIn) {
        return true;
    }
    return Orig_SniperZoomIn(pad);
}

static bool __attribute__((thiscall)) Hooked_SniperZoomOut(void* thisPad) {
    CPad* pad = (CPad*)thisPad;
    if (pad == (CPad*)0x00B73458 && s_customSniperZoomOut) {
        return true;
    }
    return Orig_SniperZoomOut(pad);
}

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

    // 2. Hook JMP em CTaskSimpleUseGun::ControlGunMove (0x0061E0C0)
    // 5-byte JMP direto substituindo a interpolacao com atraso da engine
    if (VirtualProtect((LPVOID)0x0061E0C0, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        DWORD myAddr = (DWORD)Hooked_ControlGunMove;
        DWORD rel    = myAddr - (0x0061E0C0 + 5);
        *(BYTE* )0x0061E0C0       = 0xE9;
        *(DWORD*)(0x0061E0C0 + 1) = rel;
        VirtualProtect((LPVOID)0x0061E0C0, 5, oldProtect, &oldProtect);
        LogMsg("[Hook] ControlGunMove JMP hook OK: 0x0061E0C0 -> 0x%08X\n", myAddr);
    } else {
        LogMsg("[Hook] FAILED VirtualProtect at 0x0061E0C0\n");
    }

    // 3. Desativa permanentemente a busca de alvos para auto-aim no engine (CPlayerPed::FindWeaponLockOnTarget - 0x0060DC50)
    // Isso garante que NUNCA seja criado um cone/triângulo 3D girando sobre a cabeça de nenhum ped,
    // e o CJ nunca trave a mira automaticamente em ninguém. 100% Mira Livre (Free Aim) moderna!
    if (VirtualProtect((LPVOID)0x0060DC50, 3, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        BYTE noAutoAim[] = { 0x31, 0xC0, 0xC3 }; // xor eax, eax; ret
        memcpy((void*)0x0060DC50, noAutoAim, sizeof(noAutoAim));
        VirtualProtect((LPVOID)0x0060DC50, 3, oldProtect, &oldProtect);
        LogMsg("[Hook] CPlayerPed::FindWeaponLockOnTarget permanently disabled (100%% Free Aim)!\n");
    } else {
        LogMsg("[Hook] FAILED VirtualProtect at 0x0060DC50\n");
    }

    // 4. Hook JMP em CPad::SniperZoomIn (0x00540B30)
    if (VirtualProtect((LPVOID)ADDR_HOOK_SNIPER_ZOOM_IN, 7, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        DWORD myAddr = (DWORD)Hooked_SniperZoomIn;
        DWORD rel    = myAddr - (ADDR_HOOK_SNIPER_ZOOM_IN + 5);
        *(BYTE* )ADDR_HOOK_SNIPER_ZOOM_IN       = 0xE9;
        *(DWORD*)(ADDR_HOOK_SNIPER_ZOOM_IN + 1) = rel;
        *(BYTE* )(ADDR_HOOK_SNIPER_ZOOM_IN + 5) = 0x90;
        *(BYTE* )(ADDR_HOOK_SNIPER_ZOOM_IN + 6) = 0x90;
        VirtualProtect((LPVOID)ADDR_HOOK_SNIPER_ZOOM_IN, 7, oldProtect, &oldProtect);
        LogMsg("[Hook] CPad::SniperZoomIn JMP hook OK: 0x%08X -> our fn 0x%08X\n", ADDR_HOOK_SNIPER_ZOOM_IN, myAddr);
    } else {
        LogMsg("[Hook] FAILED VirtualProtect at 0x%08X\n", ADDR_HOOK_SNIPER_ZOOM_IN);
    }

    // 5. Hook JMP em CPad::SniperZoomOut (0x00540B80)
    if (VirtualProtect((LPVOID)ADDR_HOOK_SNIPER_ZOOM_OUT, 7, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        DWORD myAddr = (DWORD)Hooked_SniperZoomOut;
        DWORD rel    = myAddr - (ADDR_HOOK_SNIPER_ZOOM_OUT + 5);
        *(BYTE* )ADDR_HOOK_SNIPER_ZOOM_OUT       = 0xE9;
        *(DWORD*)(ADDR_HOOK_SNIPER_ZOOM_OUT + 1) = rel;
        *(BYTE* )(ADDR_HOOK_SNIPER_ZOOM_OUT + 5) = 0x90;
        *(BYTE* )(ADDR_HOOK_SNIPER_ZOOM_OUT + 6) = 0x90;
        VirtualProtect((LPVOID)ADDR_HOOK_SNIPER_ZOOM_OUT, 7, oldProtect, &oldProtect);
        LogMsg("[Hook] CPad::SniperZoomOut JMP hook OK: 0x%08X -> our fn 0x%08X\n", ADDR_HOOK_SNIPER_ZOOM_OUT, myAddr);
    } else {
        LogMsg("[Hook] FAILED VirtualProtect at 0x%08X\n", ADDR_HOOK_SNIPER_ZOOM_OUT);
    }

}

// ============================================================================
// DLL Entry Point
// ============================================================================

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        LogMsg("====================================================\n");
        LogMsg(" Universal Controller Mod v2.8.0 (DualSense & DualShock 4 Direct)\n");
        LogMsg(" 100%% Pure Free Aim + PS5 Icons + Fixed Menu Navigation\n");
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
