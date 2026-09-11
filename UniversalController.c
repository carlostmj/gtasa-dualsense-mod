#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <setupapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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

static const char* KNOWN_DS_PATH = "\\\\?\\hid#{00001124-0000-1000-8000-00805f9b34fb}_vid&0002054c_pid&0ce6#8&2a285c49&1&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}";

static HANDLE FindAndOpenDualSenseDevice(void) {
    HANDLE hFast = CreateFileA(KNOWN_DS_PATH,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFast != INVALID_HANDLE_VALUE) {
        LogMsg("[DualSense] Connected via direct hardware path!\n");
        return hFast;
    }

    HDEVINFO devInfo = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    SP_DEVICE_INTERFACE_DATA devData;
    devData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    HANDLE hFound = INVALID_HANDLE_VALUE;

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, NULL, &GUID_DEVINTERFACE_HID, i, &devData); i++) {
        DWORD detailSize = 0;
        SetupDiGetDeviceInterfaceDetailA(devInfo, &devData, NULL, 0, &detailSize, NULL);
        if (detailSize == 0) continue;

        PSP_DEVICE_INTERFACE_DETAIL_DATA_A pDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)malloc(detailSize);
        pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        if (SetupDiGetDeviceInterfaceDetailA(devInfo, &devData, pDetail, detailSize, NULL, NULL)) {
            if ((strstr(pDetail->DevicePath, "054c") || strstr(pDetail->DevicePath, "054C")) &&
                (strstr(pDetail->DevicePath, "0ce6") || strstr(pDetail->DevicePath, "0CE6") ||
                 strstr(pDetail->DevicePath, "0df2") || strstr(pDetail->DevicePath, "0DF2"))) {

                HANDLE h = CreateFileA(pDetail->DevicePath,
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_EXISTING, 0, NULL);

                if (h != INVALID_HANDLE_VALUE) {
                    hFound = h;
                    free(pDetail);
                    break;
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

static void SendDualSenseHardwareReport(HANDLE hDev, BYTE leftMotor, BYTE rightMotor) {
    BYTE report[78];
    memset(report, 0, sizeof(report));

    report[0] = 0x31; // Report ID
    report[1] = 0x02; // Config tag
    report[2] = 0xFF; // Enable motors + triggers
    report[3] = 0x1 | 0x2 | 0x4 | 0x10 | 0x40; // Enable LEDs and lightbar
    report[4] = rightMotor; // High-frequency weak rumble
    report[5] = leftMotor;  // Low-frequency strong rumble

    // R2: Mode 0x02 (Section Resistance / Rigid Stop: CURTO E FORTE!)
    report[11] = 0x02;
    report[12] = 0x02;
    report[13] = 20;  // Start position (batente mecânico curto imediato!)
    report[14] = 255; // Força máxima!

    // L2: Mode 0x02 (Batente tátil para mira e freio)
    report[22] = 0x02;
    report[23] = 0x02;
    report[24] = 25;  // Start position
    report[25] = 240; // Força alta

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
}

static DWORD WINAPI DualSenseWorkerThread(LPVOID lpParam) {
    LogMsg("[DualSense] Native HID Worker Thread started.\n");

    BYTE buf[78];
    DWORD readBytes = 0;

    while (g_dsRunning) {
        if (g_hDualSense == INVALID_HANDLE_VALUE) {
            g_hDualSense = FindAndOpenDualSenseDevice();
            if (g_hDualSense != INVALID_HANDLE_VALUE) {
                LogMsg("[DualSense] Connected successfully via Native HID Bluetooth!\n");
                // Send initial trigger lock ("Curto e Forte")
                SendDualSenseHardwareReport(g_hDualSense, 0, 0);
                g_lastSentLeftMotor = 0;
                g_lastSentRightMotor = 0;
                g_lastOutputTick = GetTickCount();
            } else {
                Sleep(1000);
                continue;
            }
        }

        // 1. Output update (Rumble & Trigger heartbeat)
        DWORD now = GetTickCount();
        BOOL motorChanged = (g_targetLeftMotor != g_lastSentLeftMotor || g_targetRightMotor != g_lastSentRightMotor);
        BOOL heartbeat = (now - g_lastOutputTick) >= 400;

        if (motorChanged || heartbeat) {
            g_lastSentLeftMotor = g_targetLeftMotor;
            g_lastSentRightMotor = g_targetRightMotor;
            g_lastOutputTick = now;
            SendDualSenseHardwareReport(g_hDualSense, g_lastSentLeftMotor, g_lastSentRightMotor);
        }

        // 2. Read hardware input stream
        if (ReadFile(g_hDualSense, buf, 78, &readBytes, NULL) && readBytes >= 11) {
            g_dsInput.lx = (short)((int)buf[2] - 128);
            g_dsInput.ly = (short)((int)buf[3] - 128);
            g_dsInput.rx = (short)((int)buf[4] - 128);
            g_dsInput.ry = (short)((int)buf[5] - 128);
            g_dsInput.l2 = buf[6];
            g_dsInput.r2 = buf[7];

            BYTE b9 = buf[9];
            // Face buttons
            g_dsInput.btnSquare   = (b9 & 0x10) != 0;
            g_dsInput.btnCross    = (b9 & 0x20) != 0;
            g_dsInput.btnCircle   = (b9 & 0x40) != 0;
            g_dsInput.btnTriangle = (b9 & 0x80) != 0;

            // D-Pad
            BYTE dpad = b9 & 0x0F;
            g_dsInput.dpadUp    = (dpad == 0 || dpad == 1 || dpad == 7);
            g_dsInput.dpadRight = (dpad == 1 || dpad == 2 || dpad == 3);
            g_dsInput.dpadDown  = (dpad == 3 || dpad == 4 || dpad == 5);
            g_dsInput.dpadLeft  = (dpad == 5 || dpad == 6 || dpad == 7);

            BYTE b10 = buf[10];
            g_dsInput.btnL1     = (b10 & 0x01) != 0;
            g_dsInput.btnR1     = (b10 & 0x02) != 0;
            g_dsInput.btnShare  = (b10 & 0x10) != 0; // Share / Create button
            g_dsInput.btnStart  = (b10 & 0x20) != 0; // Options / Start button
            g_dsInput.btnL3     = (b10 & 0x40) != 0;
            g_dsInput.btnR3     = (b10 & 0x80) != 0;

            BYTE b11 = (readBytes > 11) ? buf[11] : 0;
            g_dsInput.btnPS     = (b11 & 0x01) != 0; // PS Logo button
            g_dsInput.btnTouch  = (b11 & 0x02) != 0; // Touchpad click
            g_dsInput.btnSelect = g_dsInput.btnShare || g_dsInput.btnTouch;

            g_dsInput.connected = TRUE;
        } else {
            Sleep(5);
        }
    }

    if (g_hDualSense != INVALID_HANDLE_VALUE) {
        SendDualSenseHardwareReport(g_hDualSense, 0, 0); // Release triggers
        CloseHandle(g_hDualSense);
        g_hDualSense = INVALID_HANDLE_VALUE;
    }
    return 0;
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
// Controller Processing & Engine Injection
// ============================================================================

static DWORD g_lastInputLogTime = 0;

static void ProcessCustomController(CPad* pad) {
    if (!pad) return;

    // *** INPUT STRATEGY ***
    // Primary: joyGetPosEx (WinMM) - proven reliable for DualSense on Windows BT.
    //          The DualSense registers as a standard joystick - axes + main buttons work.
    // Overlay: g_dsInput (HID thread) - provides EXTRA buttons only (Share/Options/PS/Touch)
    //          that joyGetPosEx doesn't expose. HID thread also handles ALL output (rumble/triggers).
    DualSenseInputState gp = { 0 };

    // Step 1: Read axes + main buttons via joyGetPosEx (the proven path)
    JOYINFOEX jie;
    memset(&jie, 0, sizeof(jie));
    jie.dwSize = sizeof(jie);
    jie.dwFlags = JOY_RETURNALL;
    if (joyGetPosEx(JOYSTICKID1, &jie) == JOYERR_NOERROR) {
        gp.connected   = TRUE;
        gp.lx          = (short)(((int)jie.dwXpos - 32768) / 256);
        gp.ly          = (short)(((int)jie.dwYpos - 32768) / 256);
        gp.rx          = (short)(((int)jie.dwZpos - 32768) / 256);
        gp.ry          = (short)(((int)jie.dwRpos - 32768) / 256);
        // L2/R2 analog from U/V axes
        gp.l2          = (BYTE)(jie.dwUpos / 257);
        gp.r2          = (BYTE)(jie.dwVpos / 257);
        // Main face buttons
        gp.btnCross    = (jie.dwButtons & (1 << 0)) != 0;
        gp.btnCircle   = (jie.dwButtons & (1 << 1)) != 0;
        gp.btnSquare   = (jie.dwButtons & (1 << 2)) != 0;
        gp.btnTriangle = (jie.dwButtons & (1 << 3)) != 0;
        gp.btnL1       = (jie.dwButtons & (1 << 4)) != 0;
        gp.btnR1       = (jie.dwButtons & (1 << 5)) != 0;
        gp.btnL2       = (jie.dwButtons & (1 << 6)) != 0;  // digital L2
        gp.btnR2       = (jie.dwButtons & (1 << 7)) != 0;  // digital R2 (unused)
        gp.btnShare    = (jie.dwButtons & (1 << 8)) != 0;
        gp.btnStart    = (jie.dwButtons & (1 << 9)) != 0;
        gp.btnL3       = (jie.dwButtons & (1 << 10)) != 0;
        gp.btnR3       = (jie.dwButtons & (1 << 11)) != 0;
        gp.btnPS       = (jie.dwButtons & (1 << 12)) != 0;
        gp.btnTouch    = (jie.dwButtons & (1 << 13)) != 0;
        gp.btnSelect   = gp.btnShare || gp.btnTouch;
        // D-Pad from POV hat
        if (jie.dwPOV != 0xFFFF) {
            gp.dpadUp    = (jie.dwPOV == 0    || jie.dwPOV == 4500  || jie.dwPOV == 31500);
            gp.dpadRight = (jie.dwPOV == 4500 || jie.dwPOV == 9000  || jie.dwPOV == 13500);
            gp.dpadDown  = (jie.dwPOV == 13500|| jie.dwPOV == 18000 || jie.dwPOV == 22500);
            gp.dpadLeft  = (jie.dwPOV == 22500|| jie.dwPOV == 27000 || jie.dwPOV == 31500);
        }
    }

    // Step 2: Overlay extra HID-only buttons (Share/Options/PS) if HID thread is connected
    // These buttons are not exposed by joyGetPosEx on all systems
    if (g_dsInput.connected) {
        if (g_dsInput.btnShare) gp.btnShare  = TRUE;
        if (g_dsInput.btnStart) gp.btnStart  = TRUE;
        if (g_dsInput.btnPS)    gp.btnPS     = TRUE;
        if (g_dsInput.btnTouch) gp.btnTouch  = TRUE;
        gp.btnSelect = gp.btnShare || gp.btnTouch;
        // Use analog L2/R2 from HID (more accurate than joyGetPosEx axes)
        if (g_dsInput.l2 > 0 || g_dsInput.r2 > 0) {
            gp.l2 = g_dsInput.l2;
            gp.r2 = g_dsInput.r2;
        }
    }

    if (!gp.connected) {
        return;
    }

    gp.lx = ApplyDeadzoneVal(gp.lx, g_cfg.deadzoneLeft);
    gp.ly = ApplyDeadzoneVal(gp.ly, g_cfg.deadzoneLeft);
    gp.rx = ApplyDeadzoneVal(gp.rx, g_cfg.deadzoneRight);
    gp.ry = ApplyDeadzoneVal(gp.ry, g_cfg.deadzoneRight);

    DWORD now = GetTickCount();

    // ========================================================================
    // GLOBAL BUTTON EDGES: START (OPTIONS) & SHARE / TOUCHPAD (MAP)
    // ========================================================================
    static BOOL s_lastStart = FALSE;
    BOOL startEdge = (gp.btnStart && !s_lastStart);
    s_lastStart = gp.btnStart;

    static BOOL s_lastMapBtn = FALSE;
    BOOL mapBtn = (gp.btnShare || gp.btnTouch);
    BOOL mapEdge = (mapBtn && !s_lastMapBtn);
    s_lastMapBtn = mapBtn;

    BYTE isMenuActive = *(BYTE*)ADDR_MENU_ACTIVE;
    char curMenuPage = *(char*)ADDR_CURRENT_MENU_PAGE;

    // 1. START / OPTIONS BUTTON: Toggles Pause / Settings Menu
    if (startEdge) {
        LogMsg("[Menu] Start/Options toggled! Current Active=%d\n", isMenuActive);
        FUNC_SwitchMenuOnAndOff((void*)ADDR_FRONTEND_MENU_MANAGER);
        return;
    }

    // 2. SHARE / TOUCHPAD: Direct Map Shortcut
    if (mapEdge) {
        LogMsg("[Menu] Share/Touchpad toggled! Active=%d, Page=%d\n", isMenuActive, curMenuPage);
        if (!isMenuActive) {
            // Open menu directly into MAP page (5)
            FUNC_SwitchMenuOnAndOff((void*)ADDR_FRONTEND_MENU_MANAGER);
            FUNC_SwitchToNewScreen((void*)ADDR_FRONTEND_MENU_MANAGER, 5);
        } else {
            if (curMenuPage == 5) {
                // Already in Map -> exit straight to game
                FUNC_SwitchMenuOnAndOff((void*)ADDR_FRONTEND_MENU_MANAGER);
            } else {
                // In another menu -> switch to Map
                FUNC_SwitchToNewScreen((void*)ADDR_FRONTEND_MENU_MANAGER, 5);
            }
        }
        return;
    }

    // ========================================================================
    // 3. FRONTEND / PAUSE MENU NAVIGATION (D-PAD, STICK, CROSS, CIRCLE)
    // ========================================================================
    if (isMenuActive != 0) {
        // Map Page (5) Controls: Left Stick pans map, R2/L2 or R1/L1 zooms map
        if (curMenuPage == 5) {
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

        // Navigation (D-Pad or Left Stick) with repeat timer
        static DWORD s_nextNavRepeat = 0;
        static int s_activeDir = 0; // 1=Up, 2=Down, 3=Left, 4=Right

        int curDir = 0;
        if (gp.dpadUp || gp.ly < -60) curDir = 1;
        else if (gp.dpadDown || gp.ly > 60) curDir = 2;
        else if (gp.dpadLeft || gp.lx < -60) curDir = 3;
        else if (gp.dpadRight || gp.lx > 60) curDir = 4;

        char down = 0, up = 0, input = 0;

        if (curDir != 0) {
            if (curDir != s_activeDir) {
                s_activeDir = curDir;
                s_nextNavRepeat = now + 300; // 300ms initial delay
                if (curDir == 1) up = 1;
                else if (curDir == 2) down = 1;
                else if (curDir == 3) input = -1;
                else if (curDir == 4) input = 1;
            } else if (now >= s_nextNavRepeat) {
                s_nextNavRepeat = now + 120; // 120ms repeat rate
                if (curDir == 1) up = 1;
                else if (curDir == 2) down = 1;
                else if (curDir == 3) input = -1;
                else if (curDir == 4) input = 1;
            }
        } else {
            s_activeDir = 0;
        }

        // Buttons in Menu
        static BOOL s_lastCross = FALSE;
        static BOOL s_lastBack = FALSE;

        char enter = 0;
        if (gp.btnCross && !s_lastCross) {
            enter = 1;
        }
        s_lastCross = gp.btnCross;

        char exit = 0;
        if ((gp.btnCircle || gp.btnTriangle) && !s_lastBack) {
            exit = 1;
        }
        s_lastBack = (gp.btnCircle || gp.btnTriangle);

        if (down || up || enter || exit || input) {
            FUNC_ProcessUserInput((void*)ADDR_FRONTEND_MENU_MANAGER, down, up, enter, exit, input);
        }

        // Silence rumble while in menu
        g_targetLeftMotor = 0;
        g_targetRightMotor = 0;
        g_lightbarRed = 160;
        g_lightbarGreen = 30;
        g_lightbarBlue = 255; // Violet in Menu
        return; // End menu frame processing
    }

    // Update Lightbar based on CJ's Health in Gameplay
    void** pPlayerPedPtr = (void**)0x00B7CD98;
    if (pPlayerPedPtr && *pPlayerPedPtr) {
        float hp = *(float*)((char*)(*pPlayerPedPtr) + 0x540);
        if (hp <= 30.0f) {
            g_lightbarRed = 255;
            g_lightbarGreen = 0;
            g_lightbarBlue = 0; // Red (Critical Health)
        } else if (hp <= 60.0f) {
            g_lightbarRed = 255;
            g_lightbarGreen = 120;
            g_lightbarBlue = 0; // Orange (Medium Health)
        } else {
            g_lightbarRed = 0;
            g_lightbarGreen = 120;
            g_lightbarBlue = 255; // PlayStation Blue (Healthy)
        }
    } else {
        g_lightbarRed = 0;
        g_lightbarGreen = 120;
        g_lightbarBlue = 255;
    }

    // ========================================================================
    // 4. GAMEPLAY CONTROLS (ON FOOT & IN VEHICLE)
    // ========================================================================
    BOOL hasInput = (gp.lx != 0 || gp.ly != 0 || gp.rx != 0 || gp.ry != 0 ||
                     gp.l2 > 30 || gp.r2 > 30 || gp.btnCross || gp.btnSquare ||
                     gp.btnTriangle || gp.btnCircle || gp.btnL1 || gp.btnR1 ||
                     gp.dpadUp || gp.dpadDown || gp.dpadLeft || gp.dpadRight ||
                     gp.btnStart || gp.btnSelect || gp.btnPS);

    if (hasInput && (now - g_lastInputLogTime > 3000)) {
        g_lastInputLogTime = now;
        LogMsg("[Pad0] Active Gameplay Input: LX=%d, LY=%d, RX=%d, RY=%d, L2=%d, R2=%d\n",
               gp.lx, gp.ly, gp.rx, gp.ry, gp.l2, gp.r2);
    }

    void* pVeh = FUNC_FindPlayerVehicle(-1, FALSE);
    BOOL isVehicle = (pVeh != NULL);

    // ANALOG STICK MOVEMENT (Left Stick ONLY)
    // CRITICAL: D-Pad NEVER moves CJ!
    if (!isVehicle) {
        pad->NewState.DPadUp    = 0;
        pad->NewState.DPadDown  = 0;
        pad->NewState.DPadLeft  = 0;
        pad->NewState.DPadRight = 0;

        if (gp.lx != 0) pad->NewState.LeftStickX = gp.lx;
        if (gp.ly != 0) pad->NewState.LeftStickY = gp.ly;
    } else {
        if (gp.lx != 0) pad->NewState.LeftStickX = gp.lx;
        if (gp.ly != 0) pad->NewState.LeftStickY = gp.ly;
    }

    // ANALOG CAMERA LOOK (Right Stick -> Mouse Deltas)
    if (gp.rx != 0 || gp.ry != 0) {
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

    // CAMERA VIEW CHANGE (PS Logo Button)
    if (gp.btnPS) {
        pad->NewState.Select = 255;
    }

    if (!isVehicle) {
        // ==========================================
        // A PÉ (ON FOOT)
        // ==========================================
        // L2: MIRA (Target Lock-on / Free Aim) -> RightShoulder1
        if (gp.l2 > 30) {
            pad->NewState.RightShoulder1 = 255;
        }

        // R2: ATIRAR / DISPARAR (Fire Weapon)
        if (gp.r2 > 30) {
            pad->NewState.ButtonCircle = 255;
        }

        // BOTÕES DE FACE
        if (gp.btnCross)    pad->NewState.ButtonCross    = 255; // Correr / Sprint
        if (gp.btnSquare)   pad->NewState.ButtonSquare   = 255; // Pulo / Escalar
        if (gp.btnTriangle) pad->NewState.ButtonTriangle = 255; // Entrar no veículo
        if (gp.btnCircle && gp.r2 <= 30) pad->NewState.ButtonCircle = 255; // Soco / Combate corpo a corpo

        // TROCA DE ARMA RÁPIDA (CYCLE WEAPONS)
        // EXCLUSIVAMENTE L1 E R1! (DPAD NÃO TROCA ARMA!)
        if (gp.btnL1) {
            pad->NewState.LeftShoulder2 = 255;  // Ciclo arma anterior
        }
        if (gp.btnR1) {
            pad->NewState.RightShoulder2 = 255; // Ciclo arma seguinte
        }

        // D-PAD AÇÕES A PÉ (NÃO ANDA E NÃO TROCA ARMA):
        if (gp.dpadUp) {
            pad->NewState.LeftShoulder1 = 255;  // Atender telefone / stats
        }
        if (gp.dpadDown) {
            pad->NewState.ShockButtonR = 255;   // Cancelar / parar gangue
        }
        if (gp.dpadLeft) {
            pad->NewState.m_bChatIndicated = 1; // Resposta Não
        }
        if (gp.dpadRight) {
            pad->NewState.m_bChatIndicated = 2; // Resposta Sim / Recrutar
        }

        // ANALÓGICOS PRESSIONADOS
        if (gp.btnL3) pad->NewState.ShockButtonL = 255; // Agachar (Duck)
        if (gp.btnR3) pad->NewState.ShockButtonR = 255; // Olhar para trás

    } else {
        // ==========================================
        // EM VEÍCULO (IN VEHICLE)
        // ==========================================
        if (gp.r2 > 30) {
            pad->NewState.ButtonCross = 255;    // Acelerar
        }
        if (gp.l2 > 30) {
            pad->NewState.ButtonSquare = 255;   // Frear / Ré
        }
        if (gp.btnR1 || gp.btnCross) {
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

        // D-PAD EM VEÍCULO: MUDAR RÁDIO!
        if (gp.dpadLeft) {
            pad->NewState.DPadDown = 255;       // Rádio anterior
        }
        if (gp.dpadRight) {
            pad->NewState.DPadUp = 255;         // Próxima rádio
        }
        if (gp.dpadUp || gp.dpadDown) {
            pad->NewState.LeftShoulder1 = 255;  // Pular viagem táxi
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
// Strategy: hook the CALL instruction at 0x0054120B (pad0) in UpdatePads
// Our function gets: ecx=thisPad (thiscall), [esp+4]=padNum
// Then we call the original function directly (its entry is untouched)
// ============================================================================

#define ADDR_HOOK_PAD0      0x0054120B  // call CPad::Update (pad0, ecx=0xB73458)
#define FUNC_CPAD_UPDATE    0x00541040  // original CPad::Update function entry

static BYTE g_origPad0[5];

typedef void (__attribute__((thiscall)) *tCPadUpdate)(void* thisPad, int padNum);

// Replacement for the CALL to CPad::Update(pad0)
// Called with thiscall convention: ecx=thisPad, [esp+4]=padNum
static void __attribute__((thiscall)) Hooked_CPadUpdate0(void* thisPad, int padNum) {
    // Call original CPad::Update (its entry is untouched, we only patched the CALL site)
    ((tCPadUpdate)FUNC_CPAD_UPDATE)(thisPad, padNum);

    // Custom DualSense processing only for pad 0
    if (padNum == 0) {
        ProcessCustomController((CPad*)thisPad);
    }
}

static void InstallHook(void) {
    DWORD oldProtect;
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
        LogMsg(" Universal Controller Mod v2.2.0 (DualSense Master + Menu Engine)\n");
        LogMsg(" Built exclusively for GTA San Andreas\n");
        LogMsg("====================================================\n");
        LoadConfig();
        // Start DualSense HID thread immediately on DLL load (not lazy)
        LogMsg("[Init] Starting DualSense HID worker thread...\n");
        CreateThread(NULL, 0, DualSenseWorkerThread, NULL, 0, NULL);
        // Install game hook
        InstallHook();
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        g_dsRunning = FALSE;
        Sleep(50);
    }
    return TRUE;
}
