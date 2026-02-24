/* Main entry point for WiNTown (Windows NT version)
 * Based on original WiNTown code from WiNTownLegacy project
 */

#include "sim.h"
#include "tiles.h"
#include "sprite.h"
#include "tools.h"
#include "charts.h"
#include "notify.h"
#include "resource.h"
#include "newgame.h"
#include "assets.h"
#include <commdlg.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "gdifix.h"

#define IDM_FILE_NEW 1001
#define IDM_FILE_OPEN 1002
#define IDM_FILE_SAVE 1003
#define IDM_FILE_SAVE_AS 1004
#define IDM_FILE_EXIT 1005
#define IDM_TILESET_BASE 2000
#define IDM_TILESET_MAX 2100
#define IDM_SIM_PAUSE 3001
#define IDM_SIM_SLOW 3002
#define IDM_SIM_MEDIUM 3003
#define IDM_SIM_FAST 3004

/* Scenario menu IDs */
#define IDM_SCENARIO_BASE 4000
#define IDM_SCENARIO_DULLSVILLE 4001
#define IDM_SCENARIO_SANFRANCISCO 4002
#define IDM_SCENARIO_HAMBURG 4003
#define IDM_SCENARIO_BERN 4004
#define IDM_SCENARIO_TOKYO 4005
#define IDM_SCENARIO_DETROIT 4006
#define IDM_SCENARIO_BOSTON 4007
#define IDM_SCENARIO_RIO 4008

/* View menu IDs */
#define IDM_VIEW_INFOWINDOW 4100
#define IDM_VIEW_LOGWINDOW 4101
#define IDM_VIEW_POWER_OVERLAY 4102
#define IDM_VIEW_DEBUG_LOGS 4103
#define IDM_VIEW_MINIMAPWINDOW 4104
#define IDM_VIEW_TILESWINDOW 4105
#define IDM_VIEW_CHARTSWINDOW 4106
#define IDM_VIEW_TILE_DEBUG 4107
#define IDM_VIEW_TEST_SAVELOAD 4108
#define IDM_VIEW_ABOUT 4109

/* Spawn menu IDs */
#define IDM_SPAWN_HELICOPTER 6001
#define IDM_SPAWN_AIRPLANE 6002
#define IDM_SPAWN_TRAIN 6003
#define IDM_SPAWN_SHIP 6004
#define IDM_SPAWN_BUS 6005

/* Cheats menu IDs */
#define IDM_CHEATS_DISABLE_DISASTERS 7001

/* Disaster menu IDs */
#define IDM_DISASTER_BASE 9000
#define IDM_DISASTER_FIRE 9001
#define IDM_DISASTER_FLOOD 9002
#define IDM_DISASTER_TORNADO 9003
#define IDM_DISASTER_EARTHQUAKE 9004
#define IDM_DISASTER_MONSTER 9005
#define IDM_DISASTER_MELTDOWN 9006

/* Settings menu IDs */
#define IDM_SETTINGS_DIALOG 8101
#define IDM_SETTINGS_LEVEL_EASY 8102
#define IDM_SETTINGS_LEVEL_MEDIUM 8103
#define IDM_SETTINGS_LEVEL_HARD 8104
#define IDM_SETTINGS_AUTO_BUDGET 8105
#define IDM_SETTINGS_AUTO_BULLDOZE 8106
#define IDM_SETTINGS_AUTO_GOTO 8107


/* View menu IDs - Budget Window */
#define IDM_VIEW_BUDGET 8001
#define IDM_VIEW_EVALUATION 8002

/* Minimap window definitions */
#define MINIMAP_WINDOW_CLASS "WiNTownMinimapWindow"
#define MINIMAP_WINDOW_WIDTH 360  /* WORLD_X * MINIMAP_SCALE = 120 * 3 */
#define MINIMAP_WINDOW_HEIGHT 320  /* WORLD_Y * MINIMAP_SCALE + space for mode label = 100 * 3 + 20 */
#define MINIMAP_TIMER_ID 3
#define MINIMAP_TIMER_INTERVAL 2000 /* Update minimap every 2 seconds for periodic refresh */
#define CHART_TIMER_ID 4
#define CHART_TIMER_INTERVAL 5000 /* Update charts every 5 seconds for periodic refresh */
#define EARTHQUAKE_TIMER_ID 5
#define MINIMAP_SCALE 3 /* 3x3 pixels per tile */

/* Minimap view modes */
#define MINIMAP_MODE_ALL 0
#define MINIMAP_MODE_RESIDENTIAL 1
#define MINIMAP_MODE_COMMERCIAL 2
#define MINIMAP_MODE_INDUSTRIAL 3
#define MINIMAP_MODE_POWER 4
#define MINIMAP_MODE_TRANSPORT 5
#define MINIMAP_MODE_POPULATION 6
#define MINIMAP_MODE_TRAFFIC 7
#define MINIMAP_MODE_POLLUTION 8
#define MINIMAP_MODE_CRIME 9
#define MINIMAP_MODE_LANDVALUE 10
#define MINIMAP_MODE_FIRE 11
#define MINIMAP_MODE_POLICE 12
#define MINIMAP_MODE_COUNT 13

/* Info window definitions */
#define INFO_WINDOW_CLASS "WiNTownInfoWindow"
#define INFO_WINDOW_WIDTH 300
#define INFO_WINDOW_HEIGHT 320
#define INFO_TIMER_ID 2
#define INFO_TIMER_INTERVAL 2000 /* Update info window every 2 seconds */

/* Log window definitions */
#define LOG_WINDOW_CLASS "WiNTownLogWindow"
#define LOG_WINDOW_WIDTH 500
#define LOG_WINDOW_HEIGHT 400
#define MAX_LOG_LINES 100

/* Tiles debug window definitions */
#define TILES_WINDOW_CLASS "WiNTownTilesWindow"
#define TILES_WINDOW_WIDTH 560  /* 32 tiles * 16 pixels + scrollbar + border */
#define TILES_WINDOW_HEIGHT 520 /* 30 tiles * 16 pixels + title bar + border */

/* Tool menu IDs */
#define IDM_TOOL_BASE 5000
#define IDM_TOOL_BULLDOZER 5001
#define IDM_TOOL_ROAD 5002
#define IDM_TOOL_RAIL 5003
#define IDM_TOOL_WIRE 5004
#define IDM_TOOL_PARK 5005
#define IDM_TOOL_RESIDENTIAL 5006
#define IDM_TOOL_COMMERCIAL 5007
#define IDM_TOOL_INDUSTRIAL 5008
#define IDM_TOOL_FIRESTATION 5009
#define IDM_TOOL_POLICESTATION 5010
#define IDM_TOOL_STADIUM 5011
#define IDM_TOOL_SEAPORT 5012
#define IDM_TOOL_POWERPLANT 5013
#define IDM_TOOL_NUCLEAR 5014
#define IDM_TOOL_AIRPORT 5015
#define IDM_TOOL_QUERY 5016

/* Define needed for older Windows SDK compatibility */
#ifndef LR_CREATEDIBSECTION
#define LR_CREATEDIBSECTION 0x2000
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef LR_DEFAULTCOLOR
#define LR_DEFAULTCOLOR 0
#endif

#ifndef LR_DEFAULTSIZE
#define LR_DEFAULTSIZE 0x0040
#endif

#ifndef LR_LOADFROMFILE
#define LR_LOADFROMFILE 0x0010
#endif

#ifndef IMAGE_BITMAP
#define IMAGE_BITMAP 0
#endif

#define TILE_SIZE 16

/* Main map and history arrays - now defined in simulation.h/c */
/* RISC CPU Optimization: Align critical data structures on 8-byte boundaries
 * Prevents alignment traps on Alpha, MIPS, PowerPC processors
 */
#pragma pack(push, 8)
short Map[WORLD_Y][WORLD_X];
short ResHis[HISTLEN / 2];
short ComHis[HISTLEN / 2];
short IndHis[HISTLEN / 2];
short CrimeHis[HISTLEN / 2];
short PollutionHis[HISTLEN / 2];
short MoneyHis[HISTLEN / 2];
short MiscHis[MISCHISTLEN / 2];
#pragma pack(pop)

HWND hwndMain = NULL; /* Main window handle - used by other modules */
HWND hwndInfo = NULL; /* Info window handle for displaying city stats */
HWND hwndLog = NULL; /* Log window handle for displaying game events */
HWND hwndMinimap = NULL; /* Minimap window handle for overview and navigation */
HWND hwndTiles = NULL; /* Tiles debug window handle for tileset inspection */
HWND hwndCharts = NULL; /* Charts window handle for data visualization */

/* Minimap window variables */
static int minimapMode = MINIMAP_MODE_ALL; /* Current minimap display mode */
static BOOL minimapDragging = FALSE; /* Is user dragging on minimap */

/* Tiles debug window variables */
static BOOL tilesWindowVisible = FALSE; /* Track tiles window visibility */
static int selectedTileX = -1; /* Selected tile X coordinate (-1 = no selection) */
static int selectedTileY = -1; /* Selected tile Y coordinate (-1 = no selection) */
static BOOL tileDebugEnabled = FALSE; /* Track tile debug mode for mouse cursor info */
static int minimapDragX = 0; /* Drag start position */
static int minimapDragY = 0; /* Drag start position */

/* Charts window variables */
static BOOL chartsWindowVisible = TRUE; /* Track charts window visibility */

/* Log window variables */
static BOOL logWindowVisible = TRUE; /* Track log window visibility - on by default */
static char logMessages[MAX_LOG_LINES][256]; /* Circular buffer for log messages */
static int logMessageCount = 0; /* Number of messages in buffer */
static int logScrollPos = 0; /* Current scroll position */
static BOOL autoScrollEnabled = TRUE; /* Auto-scroll when user is at bottom */
int showDebugLogs = 1; /* Flag to control whether debug logs are shown (enabled by default) */
static HBITMAP hbmBuffer = NULL;
static HDC hdcBuffer = NULL;
static HBITMAP hbmTiles = NULL;
static HDC hdcTiles = NULL;
HPALETTE hPalette = NULL;

/* Sprite bitmaps - array indexed by sprite type and frame */
static HBITMAP hbmSprites[9][16] = {0}; /* 9 sprite types, max 16 frames each */
static HDC hdcSprites = NULL;

static int cxClient = 0;
static int cyClient = 0;
static int xOffset = 0;
static int yOffset = 0;
static int toolbarWidth = 132; /* Original Micropolis toolbar width */

int AutoGo = 1; /* Auto-scroll to event locations */

static BOOL isMouseDown = FALSE; /* Used for map dragging */
static int lastMouseX = 0;
static int lastMouseY = 0;

/* Tool drag state for continuous drawing */
BOOL isToolDragging = FALSE;  /* Made non-static so tools.c can access it */
static int lastToolX = 0;
static int lastToolY = 0;

char progPathName[MAX_PATH];
char cityFileName[MAX_PATH]; /* Current city filename - used by other modules */
static HMENU hMenu = NULL;
static HMENU hFileMenu = NULL;
static HMENU hTilesetMenu = NULL;
static HMENU hScenarioMenu = NULL;
static HMENU hToolMenu = NULL;
static HMENU hSpawnMenu = NULL;
static HMENU hDisasterMenu = NULL;
static HMENU hSettingsMenu = NULL;
static char currentTileset[MAX_PATH] = "classic";
static int powerOverlayEnabled = 0; /* Power overlay display toggle */
int disastersDisabled = 0; /* Cheat flag to disable disasters - global for other modules */

/* Game settings - Global variables for speed, difficulty, and auto-settings */
int gameSpeed = SPEED_MEDIUM;     /* Current game speed (0=pause, 1=slow, 2=medium, 3=fast) */
int gameLevel = LEVEL_EASY;       /* Current difficulty level (0=easy, 1=medium, 2=hard) */
int autoBulldoze = 1;             /* Auto-bulldoze enabled flag */
int simTimerDelay = 200;          /* Timer delay in milliseconds */

/* Earthquake shake effect variables */
int shakeNow = 0;                 /* Earthquake shake intensity (0 = no shake) */
unsigned int earthquakeTimer = 0; /* Timer ID for earthquake duration */
#define EARTHQUAKE_DURATION 3000  /* Earthquake duration in milliseconds */

/* External reference to scenario variables (defined in scenarios.c) */
extern short ScenarioID;    /* Current scenario ID (0 = none) */
extern short DisasterEvent; /* Current disaster type */
extern short DisasterWait;  /* Countdown to next disaster */
extern short ScoreType;     /* Score type for scenario */
extern short ScoreWait;     /* Score wait for scenario */

/* WiNTown tile flags - These must match simulation.h */
/* Using LOMASK from simulation.h */
/* Use the constants from simulation.h for consistency */

/* Tile type constants */
#define TILE_DIRT 0
#define TILE_RIVER 2
#define TILE_REDGE 3
#define TILE_CHANNEL 4
#define TILE_FIRSTRIVEDGE 5
#define TILE_LASTRIVEDGE 20
#define TILE_WATER_LOW TILE_RIVER
#define TILE_WATER_HIGH TILE_LASTRIVEDGE

#define TILE_TREEBASE 21
#define TILE_WOODS_LOW TILE_TREEBASE
#define TILE_LASTTREE 36
#define TILE_WOODS 37
#define TILE_UNUSED_TRASH1 38
#define TILE_UNUSED_TRASH2 39
#define TILE_WOODS_HIGH TILE_UNUSED_TRASH2
#define TILE_WOODS2 40
#define TILE_WOODS3 41
#define TILE_WOODS4 42
#define TILE_WOODS5 43

#define TILE_RUBBLE 44
#define TILE_LASTRUBBLE 47

#define TILE_FLOOD 48
#define TILE_LASTFLOOD 51

#define TILE_RADTILE 52

#define TILE_FIRE 56
#define TILE_FIREBASE TILE_FIRE
#define TILE_LASTFIRE 63

#define TILE_ROADBASE 64
#define TILE_HBRIDGE 64
#define TILE_VBRIDGE 65
#define TILE_ROADS 66
#define TILE_INTERSECTION 76
#define TILE_HROADPOWER 77
#define TILE_VROADPOWER 78
#define TILE_LASTROAD 206

#define TILE_POWERBASE 208
#define TILE_HPOWER 208
#define TILE_VPOWER 209
#define TILE_LHPOWER 210
#define TILE_LVPOWER 211
#define TILE_LASTPOWER 222

#define TILE_RAILBASE 224
#define TILE_HRAIL 224
#define TILE_VRAIL 225
#define TILE_LASTRAIL 238

#define TILE_RESBASE 240
#define TILE_RESCLR 244
#define TILE_HOUSE 249
#define TILE_LASTRES 420

#define TILE_COMBASE 423
#define TILE_COMCLR 427
#define TILE_LASTCOM 611

/* Tileset constants - commented out because they're defined elsewhere
   If you need to modify these, update the definitions where they are first defined
 #define TILE_TOTAL_COUNT   1024  Maximum number of tiles in the tileset
 #define TILES_IN_ROW       32    Number of tiles per row in the tileset bitmap */
#ifndef TILE_TOTAL_COUNT
#define TILE_TOTAL_COUNT 1024 /* Maximum number of tiles in the tileset */
#endif
#ifndef TILES_IN_ROW
#define TILES_IN_ROW 32 /* Number of tiles per row in the tileset bitmap */
#endif

/* RISC CPU Optimization: Pre-calculated tile coordinate lookup tables
 * Eliminates expensive division/modulo operations (100+ cycles on RISC CPUs)
 * These replace: srcX = (tileIndex % TILES_IN_ROW) * TILE_SIZE
 *               srcY = (tileIndex / TILES_IN_ROW) * TILE_SIZE
 */
static short tileSourceX[TILE_TOTAL_COUNT];  /* Pre-calculated X coordinates */
static short tileSourceY[TILE_TOTAL_COUNT];  /* Pre-calculated Y coordinates */
static int lookupTablesInitialized = 0;

/* Initialize tile coordinate lookup tables */
static void initTileCoordinateLookup(void) {
    int i;
    if (lookupTablesInitialized) {
        return;
    }
    
    for (i = 0; i < TILE_TOTAL_COUNT; i++) {
        tileSourceX[i] = (i % TILES_IN_ROW) * TILE_SIZE;
        tileSourceY[i] = (i / TILES_IN_ROW) * TILE_SIZE;
    }
    lookupTablesInitialized = 1;
}

#define TILE_INDBASE 612
#define TILE_INDCLR 616
#define TILE_LASTIND 692

#define TILE_PORTBASE 693
#define TILE_PORT 698
#define TILE_LASTPORT 708

#define TILE_AIRPORTBASE 709
#define TILE_AIRPORT 716
#define TILE_LASTAIRPORT 744

#define TILE_COALBASE 745
#define TILE_POWERPLANT 750
#define TILE_LASTPOWERPLANT 760

#define TILE_FIRESTBASE 761
#define TILE_FIRESTATION 765
#define TILE_LASTFIRESTATION 769

#define TILE_POLICESTBASE 770
#define TILE_POLICESTATION 774
#define TILE_LASTPOLICESTATION 778

#define TILE_STADIUMBASE 779
#define TILE_STADIUM 784
#define TILE_LASTSTADIUM 799

#define TILE_NUCLEARBASE 811
#define TILE_NUCLEAR 816
#define TILE_LASTNUCLEAR 826

#ifndef TILE_TOTAL_COUNT
#define TILE_TOTAL_COUNT 960
#endif

#define TILES_IN_ROW 32
#define TILES_PER_ROW 32
LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK infoWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK logWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK minimapWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK tilesWndProc(HWND, UINT, WPARAM, LPARAM);

/* Log window helper functions */
void addToLogWindow(const char *message);
void updateLogWindow(void);

/* Function to add a game log entry */
void addGameLog(const char *format, ...);
/* Function to add a debug log entry */
void addDebugLog(const char *format, ...);
void initializeGraphics(HWND hwnd);
void cleanupGraphics(void);
int loadCity(char *filename);
void loadSpriteBitmaps(void);
void DrawTransparentBitmap(HDC hdcDest, int xDest, int yDest, int width, int height,
                          HDC hdcSrc, int xSrc, int ySrc, COLORREF transparentColor);
int loadFile(char *filename);
int saveFile(char *filename);
int saveCity(void);
int saveCityAs(void);
int testSaveLoad(void);
void drawCity(HDC hdc);
void drawTile(HDC hdc, int x, int y, short tileValue);
void initTileBaseLookup();
/* getBaseFromTile() is now a macro for O(1) performance */
void swapShorts(short *buf, int len);
void resizeBuffer(int cx, int cy);
void scrollView(int dx, int dy);
void openCityDialog(HWND hwnd);
int loadTileset(const char *filename);
HPALETTE createSystemPalette(void);
HMENU createMainMenu(void);
void populateTilesetMenu(HMENU hSubMenu);
void refreshTilesetMenu(void);
int changeTileset(HWND hwnd, const char *tilesetName);
void ForceFullCensus(void);
void createNewMap(HWND hwnd);

/* External functions - defined in simulation.c */
extern int SimRandom(int range);
extern void SetValves(void);
extern const char *GetCityClassName(void);
extern void RandomlySeedRand(void);

/* Forward declarations */
void ShowEvaluationWindow(HWND parent);


BOOL WINAPI MyPathRemoveFileSpecA(char* path)
{
	char* filespec = path;
	BOOL modified = FALSE;

	if (!path || !*path) { return FALSE; }
	if (*path == '\\') { filespec = ++path; }
	if (*path == '\\') { filespec = ++path; }

	while (*path)
	{
		if (*path == '\\')
		{
			filespec = path;
		}
		else if (*path == ':')
		{
			filespec = ++path;
			if (*path == '\\') { filespec++; }
		}
		path = CharNextA(path);
	}

	if (*filespec)
	{
		*filespec = '\0';
		modified = TRUE;
	}

	return modified;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc, wcInfo;
    MSG msg;
    RECT rect;
    int mainWindowX, mainWindowY;
    FILE *debugFile;
    char debugLogPath[MAX_PATH];

	GetModuleFileName(NULL, progPathName, MAX_PATH);
	MyPathRemoveFileSpecA(progPathName);


    /* Initialize debug log by overwriting existing file */
    wsprintf(debugLogPath, "%s\\debug.log", progPathName);
    debugFile = fopen(debugLogPath, "w");
    if (debugFile) {
        /* Write initial header to verify file is working */
        fprintf(debugFile, "=== WiNTown Debug Log Started ===\n");
        fflush(debugFile);
        if (fclose(debugFile) != 0) {
            /* File close failed - handle error */
            MessageBox(NULL, "Warning: Could not properly close debug log file", "File Warning", MB_OK | MB_ICONWARNING);
        }
        debugFile = NULL;  /* Reset pointer after closing */
    } else {
        /* Could not create debug log file - warn user */
        MessageBox(NULL, "Warning: Could not create debug.log file", "File Warning", MB_OK | MB_ICONWARNING);
    }

    /* Register main window class */
    /*wc.cbSize = sizeof(WNDCLASS);*/
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "WiNTown";
    /*wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);*/

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Register info window class */
    /*wcInfo.cbSize = sizeof(WNDCLASS);*/
    wcInfo.style = CS_HREDRAW | CS_VREDRAW;
    wcInfo.lpfnWndProc = infoWndProc;
    wcInfo.cbClsExtra = 0;
    wcInfo.cbWndExtra = 0;
    wcInfo.hInstance = hInstance;
    wcInfo.hIcon = NULL;
    wcInfo.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcInfo.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcInfo.lpszMenuName = NULL;
    wcInfo.lpszClassName = INFO_WINDOW_CLASS;
    /*wcInfo.hIconSm = NULL;*/

    if (!RegisterClass(&wcInfo)) {
        MessageBox(NULL, "Info Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Register log window class */
    wcInfo.lpfnWndProc = logWndProc;
    wcInfo.lpszClassName = LOG_WINDOW_CLASS;

    if (!RegisterClass(&wcInfo)) {
        MessageBox(NULL, "Log Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Register minimap window class */
    wcInfo.lpfnWndProc = minimapWndProc;
    wcInfo.lpszClassName = MINIMAP_WINDOW_CLASS;

    if (!RegisterClass(&wcInfo)) {
        MessageBox(NULL, "Minimap Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Register tiles debug window class */
    wcInfo.lpfnWndProc = tilesWndProc;
    wcInfo.lpszClassName = TILES_WINDOW_CLASS;

    if (!RegisterClass(&wcInfo)) {
        MessageBox(NULL, "Tiles Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Register chart window class */
    wcInfo.lpfnWndProc = ChartWndProc;
    wcInfo.lpszClassName = CHART_WINDOW_CLASS;

    if (!RegisterClass(&wcInfo)) {
        MessageBox(NULL, "Chart Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hMenu = createMainMenu();

    /* Create main window */
    hwndMain = CreateWindowEx(WS_EX_CLIENTEDGE, "WiNTown", "WiNTown - Tileset: classic",
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                              932, 600, /* Additional 132px width for the original toolbar */
                              NULL, hMenu, hInstance, NULL);

    if (hwndMain == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Get main window position to position info window appropriately */
    GetWindowRect(hwndMain, &rect);
    mainWindowX = rect.left;
    mainWindowY = rect.top;

    /* Create info window */
    hwndInfo = CreateWindowEx(WS_EX_TOOLWINDOW, INFO_WINDOW_CLASS, "WiNTown Info",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE,
        mainWindowX + rect.right - rect.left + 10, mainWindowY, /* Position to right of main window */
        INFO_WINDOW_WIDTH, INFO_WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

    if (hwndInfo == NULL) {
        MessageBox(NULL, "Info Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        /* Continue anyway, just without the info window */
    } else {
        /* Start timer to update info window */
        SetTimer(hwndInfo, INFO_TIMER_ID, INFO_TIMER_INTERVAL, NULL);
    }

    /* Create log window */
    hwndLog = CreateWindowEx(WS_EX_TOOLWINDOW, LOG_WINDOW_CLASS, "WiNTown Message Log",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE | WS_VSCROLL,
        mainWindowX + rect.right - rect.left + 10, mainWindowY + INFO_WINDOW_HEIGHT + 30, /* Position below info window */
        LOG_WINDOW_WIDTH, LOG_WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

    if (hwndLog == NULL) {
        MessageBox(NULL, "Log Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        /* Continue anyway, just without the log window */
    }

    /* Create minimap window */
    {
        RECT desiredClientRect;
        RECT windowRect;
        int windowWidth, windowHeight;
        
        /* Calculate window size from desired client area size */
        desiredClientRect.left = 0;
        desiredClientRect.top = 0;
        desiredClientRect.right = MINIMAP_WINDOW_WIDTH;
        desiredClientRect.bottom = MINIMAP_WINDOW_HEIGHT;
        
        /* Adjust for window style to get actual window size needed */
        windowRect = desiredClientRect;
        AdjustWindowRectEx(&windowRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                           FALSE, /* Menu is a popup menu, not a menu bar */
                           WS_EX_TOOLWINDOW);
        
        windowWidth = windowRect.right - windowRect.left;
        windowHeight = windowRect.bottom - windowRect.top;
        
        hwndMinimap = CreateWindowEx(WS_EX_TOOLWINDOW, MINIMAP_WINDOW_CLASS, "WiNTown Minimap - Right-click for options",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE,
            mainWindowX + rect.right - rect.left + 10, mainWindowY + INFO_WINDOW_HEIGHT + 10, /* Position underneath info window */
            windowWidth, windowHeight, NULL, NULL, hInstance, NULL);
    }

    if (hwndMinimap == NULL) {
        MessageBox(NULL, "Minimap Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        /* Continue anyway, just without the minimap window */
    } else {
        /* Start slower periodic timer for minimap updates */
        SetTimer(hwndMinimap, MINIMAP_TIMER_ID, MINIMAP_TIMER_INTERVAL, NULL);
    }

    /* Create tiles debug window (hidden by default) */
    hwndTiles = CreateWindowEx(WS_EX_TOOLWINDOW, TILES_WINDOW_CLASS, "WiNTown Tile Viewer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        100, 100, /* Simple fixed position for testing */
        TILES_WINDOW_WIDTH, TILES_WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

    if (hwndTiles == NULL) {
        MessageBox(NULL, "Tiles Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        addDebugLog("Tiles window creation failed");
        /* Continue anyway, just without the tiles window */
    } else {
        addDebugLog("Tiles window created successfully: hwnd=%p", hwndTiles);
    }

    /* Create charts window (auto-opens below main window) */
    hwndCharts = CreateWindowEx(WS_EX_TOOLWINDOW, CHART_WINDOW_CLASS, "WiNTown Charts - Right-click for options",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        mainWindowX, mainWindowY + (rect.bottom - rect.top) + 10, /* Position below main window */
        rect.right - rect.left, (rect.bottom - rect.top) * 2 / 5, NULL, NULL, hInstance, NULL);

    if (hwndCharts == NULL) {
        MessageBox(NULL, "Charts Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        addDebugLog("Charts window creation failed");
        /* Continue anyway, just without the charts window */
    } else {
        addDebugLog("Charts window created successfully: hwnd=%p", hwndCharts);
        /* Show charts window by default */
        ShowWindow(hwndCharts, SW_SHOWNORMAL);
    }

    /* Initialize graphics first */
    initializeGraphics(hwndMain);
    
    /* Initialize notification system */
    InitNotificationSystem();
    
    /* Initialize chart system */
    InitChartSystem();
    
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);
    
    /* Show new game dialog on startup */
    {
        NewGameConfig config;
        addGameLog("About to show new game dialog...");
        if (showNewGameDialog(hwndMain, &config)) {
            addGameLog("User selected new game option: type=%d, difficulty=%d", config.gameType, config.difficulty);
            if (initNewGame(&config)) {
                addGameLog("New game started successfully");
                SetGameSpeed(SPEED_MEDIUM);
                InvalidateRect(hwndMain, NULL, TRUE);
            } else {
                addGameLog("Failed to initialize new game");
                /* Fall back to creating empty map */
                createNewMap(hwndMain);
            }
        } else {
            addGameLog("User cancelled new game dialog - creating empty map");
            /* User cancelled - create empty map as fallback */
            createNewMap(hwndMain);
        }
    }

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cleanupGraphics();
    
    /* Clean up chart system */
    CleanupChartSystem();

    /* Clean up info window timer */
    if (hwndInfo) {
        KillTimer(hwndInfo, INFO_TIMER_ID);
    }

    /* Clean up minimap window timer */
    if (hwndMinimap) {
        KillTimer(hwndMinimap, MINIMAP_TIMER_ID);
    }
    
    /* Clean up charts window timer */
    if (hwndCharts) {
        KillTimer(hwndCharts, CHART_TIMER_ID);
    }

    return (int)msg.wParam;
}

void CenterMapOnTile(int tileX, int tileY) {
    if (tileX < 0) tileX = 0;
    if (tileY < 0) tileY = 0;
    if (tileX >= WORLD_X) tileX = WORLD_X - 1;
    if (tileY >= WORLD_Y) tileY = WORLD_Y - 1;

    xOffset = (tileX * TILE_SIZE) - ((cxClient - toolbarWidth) / 2);
    yOffset = (tileY * TILE_SIZE) - (cyClient / 2);

    if (xOffset < 0) xOffset = 0;
    if (yOffset < 0) yOffset = 0;
    if (xOffset > WORLD_X * TILE_SIZE - (cxClient - toolbarWidth))
        xOffset = WORLD_X * TILE_SIZE - (cxClient - toolbarWidth);
    if (yOffset > WORLD_Y * TILE_SIZE - cyClient)
        yOffset = WORLD_Y * TILE_SIZE - cyClient;

    InvalidateRect(hwndMain, NULL, FALSE);
}

char *FormatNumber(long n, char *buf) {
    char tmp[32];
    int len, i, j, neg;

    neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    wsprintf(tmp, "%ld", n);
    len = lstrlen(tmp);

    j = 0;
    if (neg) buf[j++] = '-';
    for (i = 0; i < len; i++) {
        if (i > 0 && ((len - i) % 3) == 0)
            buf[j++] = ',';
        buf[j++] = tmp[i];
    }
    buf[j] = '\0';
    return buf;
}

void addGameLog(const char *format, ...) {
    va_list args;
    char buffer[512];
    char timeBuffer[64];
    char fullMessage[1024];
    SYSTEMTIME st;
    FILE *logFile;

    /* Get current time */
    GetLocalTime(&st);
    wsprintf(timeBuffer, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);

    /* Format message with bounds checking */
    va_start(args, format);
    if (wvsprintf(buffer, format, args) >= sizeof(buffer)) {
        /* Truncate if too long */
        buffer[sizeof(buffer) - 1] = '\0';
    }
    va_end(args);
    
    /* Create full message with timestamp and bounds checking */
    if (strlen(timeBuffer) + strlen(buffer) + 2 < sizeof(fullMessage)) {
        strcpy(fullMessage, timeBuffer);
        strcat(fullMessage, buffer);
        strcat(fullMessage, "\n");
    } else {
        /* Buffer too small, create truncated version */
        strcpy(fullMessage, "[TRUNCATED] ");
        strncat(fullMessage, buffer, sizeof(fullMessage) - strlen(fullMessage) - 2);
        strcat(fullMessage, "\n");
    }

    /* Write to debug.log file */
    {
        char debugLogPath[MAX_PATH];
        wsprintf(debugLogPath, "%s\\debug.log", progPathName);
        logFile = fopen(debugLogPath, "a");
        if (logFile) {
            fputs(fullMessage, logFile);
            fclose(logFile);
        }
    }
    
    /* Add to log window display (without the final newline) */
    addToLogWindow(fullMessage);
}

/**
 * Adds a debug entry to the debug log file
 */
void addDebugLog(const char *format, ...) {
#ifdef DEBUG
    va_list args;
    char buffer[512];
    char timeBuffer[64];
    char fullMessage[1024];
    SYSTEMTIME st;
    FILE *logFile;

    /* Get current time */
    GetLocalTime(&st);
    sprintf(timeBuffer, "[%02d:%02d:%02d] [DEBUG] ", st.wHour, st.wMinute, st.wSecond);

    /* Format message */
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    /* Create full message with timestamp and debug prefix */
    strcpy(fullMessage, timeBuffer);
    strcat(fullMessage, buffer);
    strcat(fullMessage, "\n");

    /* Write ONLY to debug.log file - do NOT call addGameLog */
    {
        char debugLogPath[MAX_PATH];
        wsprintf(debugLogPath, "%s\\debug.log", progPathName);
        logFile = fopen(debugLogPath, "a");
        if (logFile) {
            fputs(fullMessage, logFile);
            fclose(logFile);
        }
    }
#endif
}

BOOL CALLBACK AboutDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            return TRUE;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

LRESULT CALLBACK logWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT rect;
        HFONT hFont, hOldFont;
        int i, y, lineHeight;
        int linesVisible, startIndex, endIndex;
        
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rect);
        
        hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, "Courier New");
        hOldFont = SelectObject(hdc, hFont);
        
        FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        
        lineHeight = 16;
        y = 5;
        
        /* Calculate which messages to show based on scroll position */
        linesVisible = (rect.bottom - 10) / lineHeight;
        startIndex = logScrollPos;
        endIndex = logMessageCount;
        if (endIndex > startIndex + linesVisible) {
            endIndex = startIndex + linesVisible;
        }
        
        /* Draw visible messages */
        for (i = startIndex; i < endIndex; i++) {
            TextOut(hdc, 5, y, logMessages[i], (int)strlen(logMessages[i]));
            y += lineHeight;
        }
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        return 0;
    }
    
    case WM_VSCROLL: {
        RECT rect;
        int linesVisible, maxScroll, newPos;
        
        GetClientRect(hwnd, &rect);
        linesVisible = (rect.bottom - 10) / 16; /* 16 = lineHeight */
        maxScroll = max(0, logMessageCount - linesVisible);
        
        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            newPos = max(0, logScrollPos - 1);
            break;
        case SB_LINEDOWN:
            newPos = min(maxScroll, logScrollPos + 1);
            break;
        case SB_PAGEUP:
            newPos = max(0, logScrollPos - linesVisible);
            break;
        case SB_PAGEDOWN:
            newPos = min(maxScroll, logScrollPos + linesVisible);
            break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            newPos = HIWORD(wParam);
            if (newPos < 0) newPos = 0;
            if (newPos > maxScroll) newPos = maxScroll;
            break;
        case SB_TOP:
            newPos = 0;
            break;
        case SB_BOTTOM:
            newPos = maxScroll;
            break;
        default:
            return 0;
        }
        
        if (newPos != logScrollPos) {
            logScrollPos = newPos;
            SetScrollPos(hwnd, SB_VERT, logScrollPos, TRUE);
            
            /* Update auto-scroll flag - only enable if user scrolled to bottom */
            autoScrollEnabled = (logScrollPos == maxScroll);
            
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }
    
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        logWindowVisible = FALSE;
        if (hwndMain) {
            HMENU hMenu = GetMenu(hwndMain);
            HMENU hViewMenu = GetSubMenu(hMenu, 6);
            CheckMenuItem(hViewMenu, IDM_VIEW_LOGWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
        }
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void addToLogWindow(const char *message) {
    char *newlinePos;
    int i;
    RECT rect;
    int linesVisible, maxScroll;
    
    if (logMessageCount >= MAX_LOG_LINES) {
        /* Shift all messages up by one to make room for new message */
        for (i = 0; i < MAX_LOG_LINES - 1; i++) {
            strcpy(logMessages[i], logMessages[i + 1]);
        }
        logMessageCount = MAX_LOG_LINES - 1;
    }
    
    /* Copy message and remove trailing newline if present */
    strcpy(logMessages[logMessageCount], message);
    newlinePos = strchr(logMessages[logMessageCount], '\n');
    if (newlinePos) *newlinePos = '\0';
    
    logMessageCount++;
    
    /* Update the log window display and scrollbar */
    if (hwndLog && logWindowVisible) {
        
        GetClientRect(hwndLog, &rect);
        linesVisible = (rect.bottom - 10) / 16; /* 16 = lineHeight */
        
        /* Calculate the new scroll range */
        maxScroll = max(0, logMessageCount - linesVisible);
        SetScrollRange(hwndLog, SB_VERT, 0, maxScroll, FALSE);
        
        /* Only auto-scroll if auto-scroll is enabled (user was at bottom) */
        if (autoScrollEnabled && maxScroll > 0) {
            SetScrollPos(hwndLog, SB_VERT, maxScroll, TRUE);
            logScrollPos = maxScroll;
        } else if (maxScroll == 0) {
            /* All messages fit in window, reset scroll position */
            logScrollPos = 0;
            autoScrollEnabled = TRUE; /* Re-enable auto-scroll when all messages fit */
        }
        
        InvalidateRect(hwndLog, NULL, TRUE);
    }
}

void updateLogWindow(void) {
    if (hwndLog && logWindowVisible) {
        InvalidateRect(hwndLog, NULL, TRUE);
    }
}

/**
 * Info window procedure - handles messages for the info window
 */
LRESULT CALLBACK infoWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HMENU hMenu;
    HMENU hViewMenu;

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT clientRect;
        char buffer[256];
        char *baseName;
        char *lastSlash;
        char *lastFwdSlash;
        char *dot;
        char nameBuffer[MAX_PATH];
        int y = 10;

        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &clientRect);

        /* Fill background */
        FillRect(hdc, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

        /* Set text attributes */
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        /* Draw title */
        TextOut(hdc, 10, y, "CITY INFO", 9);
        y += 25;

        /* Draw city name */
        if (cityFileName[0] != '\0') {
            /* Make a copy of the path to work with */
            char tempPath[MAX_PATH];
            lstrcpy(tempPath, cityFileName);

            /* Extract the city name from the path */
            baseName = tempPath;
            lastSlash = strrchr(tempPath, '\\');
            lastFwdSlash = strrchr(tempPath, '/');

            if (lastSlash && lastSlash > baseName) {
                baseName = lastSlash + 1;
            }
            if (lastFwdSlash && lastFwdSlash > baseName) {
                baseName = lastFwdSlash + 1;
            }

            lstrcpy(nameBuffer, baseName);
            dot = strrchr(nameBuffer, '.');
            if (dot) {
                *dot = '\0';
            }
            wsprintf(buffer, "City Name: %s", nameBuffer);
        } else {
            /* New city */
            wsprintf(buffer, "City Name: New City");
        }
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw date and funds */
        wsprintf(buffer, "Year: %d  Month: %d", CityYear, CityMonth + 1);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        {
            char numBuf[32];
            FormatNumber((long)TotalFunds, numBuf);
            wsprintf(buffer, "Funds: $%s", numBuf);
        }
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        {
            char numBuf[32];
            FormatNumber((long)CityPop, numBuf);
            wsprintf(buffer, "Population: %s", numBuf);
        }
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw detailed population breakdown */
        wsprintf(buffer, "Residential: %d", ResPop);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Commercial: %d", ComPop);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Industrial: %d", IndPop);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw demand values */
        wsprintf(buffer, "Demand - R:%d C:%d I:%d", RValve, CValve, IValve);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw city assessment */
        wsprintf(buffer, "Score: %d  Assessment: %s", CityScore, GetCityClassName());
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Approval Rating: %d%%",
                 (CityYes > 0) ? (CityYes * 100 / (CityYes + CityNo)) : 0);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw other stats */
        wsprintf(buffer, "Traffic: %d  Pollution: %d", TrafficAverage, PollutionAverage);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        wsprintf(buffer, "Crime: %d  Land Value: %d", CrimeAverage, LVAverage);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));
        y += 20;

        /* Draw power info */
        wsprintf(buffer, "Power: Powered=%d Unpowered=%d", PwrdZCnt, UnpwrdZCnt);
        TextOut(hdc, 10, y, buffer, lstrlen(buffer));

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER:
        if (wParam == INFO_TIMER_ID) {
            /* Repaint the window to update the stats */
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        break;

    case WM_CLOSE:
        /* Don't destroy, just hide the window */
        ShowWindow(hwnd, SW_HIDE);

        /* Update menu checkmark */
        if (hwndMain) {
            hMenu = GetMenu(hwndMain);
            hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */
            if (hViewMenu) {
                CheckMenuItem(hViewMenu, IDM_VIEW_INFOWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
            }
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, INFO_TIMER_ID);
        hwndInfo = NULL;
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* Minimap window procedure
 * Displays a miniature view of the entire city with various overlay modes
 * Allows panning the main view by clicking and dragging
 */
LRESULT CALLBACK minimapWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HMENU hMenu;
    HMENU hViewMenu;
    RECT rect;
    POINT pt;

    switch (msg) {
    case WM_CREATE: {
        /* Don't create menu here - we'll create it on right-click instead */
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;
        HDC hdcMem;
        HBITMAP hbmMem, hbmOld;
        HBRUSH hBrush;
        int mapWidth, mapHeight, mapX, mapY;
        int density, intensity, level, value, coverage;
        int x, y;
        short tileValue;
        int tileType;
        COLORREF color;
        int viewX, viewY, viewW, viewH;
        int scaled;
        
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rect);

        /* Create memory DC for double buffering */
        hdcMem = CreateCompatibleDC(hdc);
        hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        hbmOld = SelectObject(hdcMem, hbmMem);

        /* Fill background */
        FillRect(hdcMem, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

        /* Calculate minimap dimensions */
        mapWidth = WORLD_X * MINIMAP_SCALE;
        mapHeight = WORLD_Y * MINIMAP_SCALE;
        mapX = 0; /* Position at left edge - no padding */
        mapY = 0; /* Position at top edge - no padding */
        
        /* Draw a white border around where the map should be */
        {
            HPEN hTestPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            HPEN hOldTestPen = SelectObject(hdcMem, hTestPen);
            HBRUSH hOldTestBrush = SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            
            Rectangle(hdcMem, mapX - 2, mapY - 2, mapX + mapWidth + 2, mapY + mapHeight + 2);
            
            SelectObject(hdcMem, hOldTestPen);
            SelectObject(hdcMem, hOldTestBrush);
            DeleteObject(hTestPen);
        }

        /* Draw the minimap based on current mode */
        for (y = 0; y < WORLD_Y; y++) {
            for (x = 0; x < WORLD_X; x++) {
                tileValue = Map[y][x];
                tileType = tileValue & LOMASK;
                color = RGB(0, 0, 0); /* Default black */

                switch (minimapMode) {
                case MINIMAP_MODE_ALL:
                    /* Show basic terrain and zones */
                    if (tileType >= RESBASE && tileType < HOSPITAL) {
                        color = RGB(0, 255, 0); /* Residential - green */
                    } else if (tileType >= COMBASE && tileType < INDBASE) {
                        color = RGB(0, 0, 255); /* Commercial - blue */
                    } else if (tileType >= INDBASE && tileType < PORTBASE) {
                        color = RGB(255, 255, 0); /* Industrial - yellow */
                    } else if (tileType >= ROADBASE && tileType <= LASTROAD) {
                        color = RGB(128, 128, 128); /* Roads - gray */
                    } else if (tileType >= RAILBASE && tileType <= LASTRAIL) {
                        color = RGB(192, 192, 192); /* Rails - light gray */
                    } else if (tileType >= POWERBASE && tileType <= LASTPOWER) {
                        color = RGB(255, 0, 0); /* Power lines - red */
                    } else if (tileType >= RIVER && tileType <= LASTRIVEDGE) {
                        color = RGB(0, 128, 255); /* Water - blue */
                    } else if (tileType >= TREEBASE && tileType <= WOODS5) {
                        color = RGB(0, 128, 0); /* Trees - dark green */
                    } else if (tileType != 0) {
                        /* Any other non-zero tile - show as dim white */
                        color = RGB(64, 64, 64);
                    }
                    break;

                case MINIMAP_MODE_RESIDENTIAL:
                    if (tileType >= RESBASE && tileType < HOSPITAL) {
                        density = calcResPop(tileType);
                        if (density > 0) {
                            intensity = min(255, density * 25);
                            color = RGB(0, intensity, 0);
                        }
                    }
                    break;

                case MINIMAP_MODE_COMMERCIAL:
                    if (tileType >= COMBASE && tileType < INDBASE) {
                        density = calcComPop(tileType);
                        if (density > 0) {
                            intensity = min(255, density * 25);
                            color = RGB(0, 0, intensity);
                        }
                    }
                    break;

                case MINIMAP_MODE_INDUSTRIAL:
                    if (tileType >= INDBASE && tileType < PORTBASE) {
                        density = calcIndPop(tileType);
                        if (density > 0) {
                            intensity = min(255, density * 25);
                            color = RGB(intensity, intensity, 0);
                        }
                    }
                    break;

                case MINIMAP_MODE_POWER:
                    if (tileValue & POWERBIT) {
                        color = RGB(255, 255, 0); /* Powered - yellow */
                    } else if (tileValue & CONDBIT) {
                        color = RGB(128, 0, 0); /* Unpowered conductor - dark red */
                    }
                    break;

                case MINIMAP_MODE_TRANSPORT:
                    if (tileType >= ROADBASE && tileType <= LASTROAD) {
                        color = RGB(255, 255, 255); /* Roads - white */
                    } else if (tileType >= RAILBASE && tileType <= LASTRAIL) {
                        color = RGB(192, 192, 192); /* Rails - light gray */
                    }
                    break;

                case MINIMAP_MODE_POPULATION:
                    /* PopDensity is half-size array, so check bounds and access correctly */
                    if ((y/2) < (WORLD_Y/2) && (x/2) < (WORLD_X/2)) {
                        density = PopDensity[y/2][x/2];
                        if (density > 0) {
                            intensity = min(255, density * 2);
                            color = RGB(intensity, 0, intensity);
                        }
                        /* Debug: show any residential areas in faint color even if no density */
                        else if (tileType >= RESBASE && tileType < HOSPITAL) {
                            color = RGB(32, 0, 32); /* Very faint purple for residential with no density */
                        }
                    }
                    break;

                case MINIMAP_MODE_TRAFFIC:
                    /* TrfDensity is half-size array, so check bounds and access correctly */
                    if ((y/2) < (WORLD_Y/2) && (x/2) < (WORLD_X/2)) {
                        density = TrfDensity[y/2][x/2];
                        if (density > 0) {
                            /* Use bright color gradient for traffic */
                            if (density >= 120) {
                                color = RGB(255, 0, 0);     /* Bright red for heavy traffic */
                            } else if (density >= 80) {
                                color = RGB(255, 128, 0);   /* Bright orange */
                            } else if (density >= 40) {
                                color = RGB(255, 255, 0);   /* Bright yellow */
                            } else if (density >= 20) {
                                color = RGB(128, 255, 0);   /* Yellow-green */
                            } else {
                                color = RGB(0, 255, 128);   /* Light green for low traffic */
                            }
                        }
                    }
                    break;

                case MINIMAP_MODE_POLLUTION:
                    /* PollutionMem is half-size array, so check bounds and access correctly */
                    if ((y/2) < (WORLD_Y/2) && (x/2) < (WORLD_X/2)) {
                        level = PollutionMem[y/2][x/2];
                        if (level > 0) {
                            /* Use bright color gradient for pollution */
                            if (level >= 200) {
                                color = RGB(255, 0, 0);     /* Bright red for high pollution */
                            } else if (level >= 150) {
                                color = RGB(255, 128, 0);   /* Bright orange */
                            } else if (level >= 100) {
                                color = RGB(255, 255, 0);   /* Bright yellow */
                            } else if (level >= 50) {
                                color = RGB(128, 255, 0);   /* Yellow-green */
                            } else {
                                color = RGB(0, 255, 128);   /* Light green for low pollution */
                            }
                        }
                    }
                    break;

                case MINIMAP_MODE_CRIME:
                    /* CrimeMem is half-size array, so check bounds and access correctly */
                    if ((y/2) < (WORLD_Y/2) && (x/2) < (WORLD_X/2)) {
                        level = CrimeMem[y/2][x/2];
                        if (level > 0) {
                            /* Use bright red gradient for crime */
                            if (level >= 200) {
                                color = RGB(255, 0, 0);     /* Bright red for high crime */
                            } else if (level >= 150) {
                                color = RGB(255, 64, 0);    /* Red-orange */
                            } else if (level >= 100) {
                                color = RGB(255, 128, 0);   /* Orange */
                            } else if (level >= 50) {
                                color = RGB(255, 192, 0);   /* Yellow-orange */
                            } else {
                                color = RGB(255, 255, 0);   /* Yellow for low crime */
                            }
                        }
                    }
                    break;

                case MINIMAP_MODE_LANDVALUE:
                    /* LandValueMem is half-size array, so check bounds and access correctly */
                    if ((y/2) < (WORLD_Y/2) && (x/2) < (WORLD_X/2)) {
                        value = LandValueMem[y/2][x/2];
                        if (value > 0) {
                            /* Use bright green gradient for land value */
                            if (value >= 200) {
                                color = RGB(0, 255, 0);     /* Bright green for high value */
                            } else if (value >= 150) {
                                color = RGB(64, 255, 64);   /* Light green */
                            } else if (value >= 100) {
                                color = RGB(128, 255, 128); /* Pale green */
                            } else if (value >= 50) {
                                color = RGB(192, 255, 192); /* Very pale green */
                            } else {
                                color = RGB(255, 255, 192); /* Pale yellow for low value */
                            }
                        }
                    }
                    break;

                case MINIMAP_MODE_FIRE:
                    /* FireRate is quarter-size array, so check bounds and access correctly */
                    if ((x/4) < (WORLD_X/4) && (y/4) < (WORLD_Y/4)) {
                        coverage = FireRate[y/4][x/4];
                        if (coverage > 0) {
                            /* Scale down short values for display - original starts with 1000 */
                            int scaled = coverage / 4;  /* Scale down from short range */
                            if (scaled > 255) scaled = 255;
                            
                            /* Use bright red gradient for fire coverage */
                            if (scaled >= 200) {
                                color = RGB(255, 0, 0);     /* Bright red for high coverage */
                            } else if (scaled >= 150) {
                                color = RGB(255, 64, 64);   /* Red-pink */
                            } else if (scaled >= 100) {
                                color = RGB(255, 128, 128); /* Light red */
                            } else if (scaled >= 50) {
                                color = RGB(255, 192, 192); /* Pink */
                            } else {
                                color = RGB(255, 224, 224); /* Very light pink */
                            }
                        }
                    }
                    break;

                case MINIMAP_MODE_POLICE:
                    /* PoliceMapEffect is quarter-size array, so check bounds and access correctly */
                    if ((x/4) < (WORLD_X/4) && (y/4) < (WORLD_Y/4)) {
                        coverage = PoliceMapEffect[y/4][x/4];
                        if (coverage > 0) {
                            /* Scale down short values for display - original starts with 1000 */
                            scaled = coverage / 4;  /* Scale down from short range */
                            if (scaled > 255) scaled = 255;
                            
                            /* Use bright blue gradient for police coverage */
                            if (scaled >= 200) {
                                color = RGB(0, 0, 255);     /* Bright blue for high coverage */
                            } else if (scaled >= 150) {
                                color = RGB(64, 64, 255);   /* Blue-violet */
                            } else if (scaled >= 100) {
                                color = RGB(128, 128, 255); /* Light blue */
                            } else if (scaled >= 50) {
                                color = RGB(192, 192, 255); /* Very light blue */
                            } else {
                                color = RGB(224, 224, 255); /* Faint blue */
                            }
                        }
                    }
                    break;
                }

                /* Draw the pixel */
                if (color != RGB(0, 0, 0)) {
                    RECT tileRect;
                    tileRect.left = mapX + x * MINIMAP_SCALE;
                    tileRect.top = mapY + y * MINIMAP_SCALE;
                    tileRect.right = tileRect.left + MINIMAP_SCALE;
                    tileRect.bottom = tileRect.top + MINIMAP_SCALE;
                    
                    hBrush = CreateSolidBrush(color);
                    FillRect(hdcMem, &tileRect, hBrush);
                    DeleteObject(hBrush);
                }
                
                /* Always draw something for dirt/empty tiles in ALL mode */
                if (minimapMode == MINIMAP_MODE_ALL && color == RGB(0, 0, 0)) {
                    RECT tileRect;
                    tileRect.left = mapX + x * MINIMAP_SCALE;
                    tileRect.top = mapY + y * MINIMAP_SCALE;
                    tileRect.right = tileRect.left + MINIMAP_SCALE;
                    tileRect.bottom = tileRect.top + MINIMAP_SCALE;
                    
                    hBrush = CreateSolidBrush(RGB(32, 32, 32));
                    FillRect(hdcMem, &tileRect, hBrush);
                    DeleteObject(hBrush);
                }
            }
        }

        /* Draw viewport rectangle showing current view */
        if (hwndMain) {
            HPEN hPen, hOldPen;
            HBRUSH hOldBrush;
            
            /* Calculate viewport position in minimap coordinates */
            viewX = mapX + (xOffset / TILE_SIZE) * MINIMAP_SCALE;
            viewY = mapY + (yOffset / TILE_SIZE) * MINIMAP_SCALE;
            viewW = ((cxClient - toolbarWidth) / TILE_SIZE) * MINIMAP_SCALE;
            viewH = (cyClient / TILE_SIZE) * MINIMAP_SCALE;

            /* Draw white outline */
            hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            hOldPen = SelectObject(hdcMem, hPen);
            hOldBrush = SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            
            Rectangle(hdcMem, viewX - 1, viewY - 1, viewX + viewW + 1, viewY + viewH + 1);
            
            /* Draw yellow inner rectangle */
            DeleteObject(hPen);
            hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
            SelectObject(hdcMem, hPen);
            
            Rectangle(hdcMem, viewX, viewY, viewX + viewW, viewY + viewH);
            
            SelectObject(hdcMem, hOldPen);
            SelectObject(hdcMem, hOldBrush);
            DeleteObject(hPen);
        }

        /* Draw mode label at bottom instead of top to avoid overlap */
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(255, 255, 255));
        
        {
            int labelY = mapY + mapHeight + 5; /* Position below minimap with small gap */
            switch (minimapMode) {
            case MINIMAP_MODE_ALL: TextOut(hdcMem, 5, labelY, "Mode: All", 9); break;
            case MINIMAP_MODE_RESIDENTIAL: TextOut(hdcMem, 5, labelY, "Mode: Residential", 17); break;
            case MINIMAP_MODE_COMMERCIAL: TextOut(hdcMem, 5, labelY, "Mode: Commercial", 16); break;
            case MINIMAP_MODE_INDUSTRIAL: TextOut(hdcMem, 5, labelY, "Mode: Industrial", 16); break;
            case MINIMAP_MODE_POWER: TextOut(hdcMem, 5, labelY, "Mode: Power Grid", 16); break;
            case MINIMAP_MODE_TRANSPORT: TextOut(hdcMem, 5, labelY, "Mode: Transportation", 20); break;
            case MINIMAP_MODE_POPULATION: TextOut(hdcMem, 5, labelY, "Mode: Population Density", 24); break;
            case MINIMAP_MODE_TRAFFIC: TextOut(hdcMem, 5, labelY, "Mode: Traffic Density", 21); break;
            case MINIMAP_MODE_POLLUTION: TextOut(hdcMem, 5, labelY, "Mode: Pollution", 15); break;
            case MINIMAP_MODE_CRIME: TextOut(hdcMem, 5, labelY, "Mode: Crime Rate", 16); break;
            case MINIMAP_MODE_LANDVALUE: TextOut(hdcMem, 5, labelY, "Mode: Land Value", 16); break;
            case MINIMAP_MODE_FIRE: TextOut(hdcMem, 5, labelY, "Mode: Fire Coverage", 19); break;
            case MINIMAP_MODE_POLICE: TextOut(hdcMem, 5, labelY, "Mode: Police Coverage", 21); break;
            }
        }

        /* Removed debug text - minimap is working properly now */
        
        /* Blit to screen */
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);

        /* Cleanup */
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mapWidth, mapHeight, mapX, mapY;
        int tileX, tileY;
        
        /* Start dragging to pan view */
        GetClientRect(hwnd, &rect);
        mapWidth = WORLD_X * MINIMAP_SCALE;
        mapHeight = WORLD_Y * MINIMAP_SCALE;
        mapX = 0; /* Same as in WM_PAINT */
        mapY = 0; /* Same as in WM_PAINT */
        
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        
        /* Check if click is within minimap bounds */
        if (pt.x >= mapX && pt.x < mapX + mapWidth &&
            pt.y >= mapY && pt.y < mapY + mapHeight) {
            minimapDragging = TRUE;
            minimapDragX = pt.x;
            minimapDragY = pt.y;
            SetCapture(hwnd);
            
            /* Pan to clicked location */
            tileX = (pt.x - mapX) / MINIMAP_SCALE;
            tileY = (pt.y - mapY) / MINIMAP_SCALE;
            
            /* Center view on clicked tile */
            xOffset = (tileX * TILE_SIZE) - ((cxClient - toolbarWidth) / 2);
            yOffset = (tileY * TILE_SIZE) - (cyClient / 2);
            
            /* Clamp to valid range */
            if (xOffset < 0) xOffset = 0;
            if (yOffset < 0) yOffset = 0;
            if (xOffset > WORLD_X * TILE_SIZE - (cxClient - toolbarWidth)) {
                xOffset = WORLD_X * TILE_SIZE - (cxClient - toolbarWidth);
            }
            if (yOffset > WORLD_Y * TILE_SIZE - cyClient) {
                yOffset = WORLD_Y * TILE_SIZE - cyClient;
            }
            
            /* Redraw main window */
            InvalidateRect(hwndMain, NULL, FALSE);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (minimapDragging) {
            int mapWidth, mapHeight, mapX, mapY;
            int tileX, tileY;
            
            GetClientRect(hwnd, &rect);
            mapWidth = WORLD_X * MINIMAP_SCALE;
            mapHeight = WORLD_Y * MINIMAP_SCALE;
            mapX = 0; /* Same as in WM_PAINT */
            mapY = 0; /* Same as in WM_PAINT */
            
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            
            /* Clamp to minimap bounds */
            if (pt.x < mapX) pt.x = mapX;
            if (pt.y < mapY) pt.y = mapY;
            if (pt.x >= mapX + mapWidth) pt.x = mapX + mapWidth - 1;
            if (pt.y >= mapY + mapHeight) pt.y = mapY + mapHeight - 1;
            
            /* Pan to dragged location */
            tileX = (pt.x - mapX) / MINIMAP_SCALE;
            tileY = (pt.y - mapY) / MINIMAP_SCALE;
            
            /* Center view on dragged tile */
            xOffset = (tileX * TILE_SIZE) - ((cxClient - toolbarWidth) / 2);
            yOffset = (tileY * TILE_SIZE) - (cyClient / 2);
            
            /* Clamp to valid range */
            if (xOffset < 0) xOffset = 0;
            if (yOffset < 0) yOffset = 0;
            if (xOffset > WORLD_X * TILE_SIZE - (cxClient - toolbarWidth)) {
                xOffset = WORLD_X * TILE_SIZE - (cxClient - toolbarWidth);
            }
            if (yOffset > WORLD_Y * TILE_SIZE - cyClient) {
                yOffset = WORLD_Y * TILE_SIZE - cyClient;
            }
            
            /* Redraw main window */
            InvalidateRect(hwndMain, NULL, FALSE);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (minimapDragging) {
            minimapDragging = FALSE;
            ReleaseCapture();
        }
        return 0;
    }
    
    case WM_RBUTTONDOWN: {
        /* Show popup menu for mode selection */
        HMENU hPopup = CreatePopupMenu();
        POINT ptScreen;
        
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_ALL ? MF_CHECKED : 0), 
                   1000 + MINIMAP_MODE_ALL, "All");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_RESIDENTIAL ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_RESIDENTIAL, "Residential");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_COMMERCIAL ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_COMMERCIAL, "Commercial");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_INDUSTRIAL ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_INDUSTRIAL, "Industrial");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_POWER ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_POWER, "Power Grid");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_TRANSPORT ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_TRANSPORT, "Transportation");
        AppendMenu(hPopup, MF_SEPARATOR, 0, NULL);
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_POPULATION ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_POPULATION, "Population Density");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_TRAFFIC ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_TRAFFIC, "Traffic Density");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_POLLUTION ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_POLLUTION, "Pollution");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_CRIME ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_CRIME, "Crime Rate");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_LANDVALUE ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_LANDVALUE, "Land Value");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_FIRE ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_FIRE, "Fire Coverage");
        AppendMenu(hPopup, MF_STRING | (minimapMode == MINIMAP_MODE_POLICE ? MF_CHECKED : 0),
                   1000 + MINIMAP_MODE_POLICE, "Police Coverage");
                   
        /* Get cursor position for popup menu */
        GetCursorPos(&ptScreen);
        
        /* Show the popup menu */
        TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                       ptScreen.x, ptScreen.y, 0, hwnd, NULL);
                       
        DestroyMenu(hPopup);
        return 0;
    }

    case WM_COMMAND: {
        /* Handle mode selection from menu */
        int mode = LOWORD(wParam) - 1000;
        
        if (mode >= MINIMAP_MODE_ALL && mode < MINIMAP_MODE_COUNT) {
            minimapMode = mode;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == MINIMAP_TIMER_ID) {
            /* Periodic minimap refresh */
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        break;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        /* Set minimum window size to ensure map fits */
        mmi->ptMinTrackSize.x = MINIMAP_WINDOW_WIDTH;
        mmi->ptMinTrackSize.y = MINIMAP_WINDOW_HEIGHT;
        return 0;
    }

    case WM_CLOSE:
        /* Don't destroy, just hide the window */
        KillTimer(hwnd, MINIMAP_TIMER_ID);
        ShowWindow(hwnd, SW_HIDE);

        /* Update menu checkmark */
        if (hwndMain) {
            hMenu = GetMenu(hwndMain);
            hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */
            if (hViewMenu) {
                CheckMenuItem(hViewMenu, IDM_VIEW_MINIMAPWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
            }
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, MINIMAP_TIMER_ID);
        hwndMinimap = NULL;
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* Tiles debug window procedure
 * Displays the current tileset for debugging and inspection
 * Shows tile coordinates on mouse hover in window title
 */
LRESULT CALLBACK tilesWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HMENU hMenu;
    HMENU hViewMenu;
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rect;
    int tileX, tileY, tileIndex;
    int startX, startY, endX, endY;
    int drawX, drawY;
    int newMouseX, newMouseY;
    char titleBuffer[256];
    static int mouseX = -1;
    static int mouseY = -1;

    switch (msg) {
    case WM_CREATE:
        addDebugLog("Tiles window WM_CREATE received");
        return 0;

    case WM_PAINT:
        addDebugLog("Tiles window WM_PAINT received");
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rect);

        addDebugLog("hdcTiles: %p, hbmTiles: %p", hdcTiles, hbmTiles);
        /* Use the existing hdcTiles that's already configured */
        if (hdcTiles && hbmTiles) {
            addDebugLog("Using existing hdcTiles");

            /* Calculate which tiles to draw based on client area */
            startX = 0;
            startY = 0;
            endX = min(TILES_IN_ROW, (rect.right / TILE_SIZE) + 1);
            endY = min(30, (rect.bottom / TILE_SIZE) + 1); /* 30 rows of tiles */

            addDebugLog("Drawing tiles from (%d,%d) to (%d,%d), rect: %dx%d", 
                       startX, startY, endX, endY, rect.right, rect.bottom);

            /* Draw tiles */
            for (tileY = startY; tileY < endY; tileY++) {
                for (tileX = startX; tileX < endX; tileX++) {
                    tileIndex = tileY * TILES_IN_ROW + tileX;
                    if (tileIndex < TILE_TOTAL_COUNT) {
                        drawX = tileX * TILE_SIZE;
                        drawY = tileY * TILE_SIZE;

                        /* Copy tile from tileset bitmap using existing hdcTiles */
                        BitBlt(hdc, drawX, drawY, TILE_SIZE, TILE_SIZE,
                               hdcTiles, tileX * TILE_SIZE, tileY * TILE_SIZE, SRCCOPY);
                    }
                }
            }
            
            /* Draw yellow selection square if a tile is selected */
            if (selectedTileX >= 0 && selectedTileY >= 0 && 
                selectedTileX < TILES_IN_ROW && selectedTileY < 30) {
                HPEN hYellowPen, hOldPen;
                HBRUSH hOldBrush;
                int selX, selY;
                
                selX = selectedTileX * TILE_SIZE;
                selY = selectedTileY * TILE_SIZE;
                
                /* Create yellow pen for selection border */
                hYellowPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
                hOldPen = SelectObject(hdc, hYellowPen);
                hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH)); /* Hollow brush */
                
                /* Draw yellow rectangle around selected tile */
                Rectangle(hdc, selX, selY, selX + TILE_SIZE, selY + TILE_SIZE);
                
                /* Restore original pen and brush */
                SelectObject(hdc, hOldPen);
                SelectObject(hdc, hOldBrush);
                DeleteObject(hYellowPen);
            }
        } else {
            addDebugLog("Cannot draw tiles: hdcTiles=%p, hbmTiles=%p", hdcTiles, hbmTiles);
            /* Fill with a gray background to show the window is working */
            FillRect(hdc, &rect, GetStockObject(GRAY_BRUSH));
        }

        EndPaint(hwnd, &ps);
        return 0;

    case WM_MOUSEMOVE:
        newMouseX = LOWORD(lParam);
        newMouseY = HIWORD(lParam);
        
        /* Only update if mouse position actually changed */
        if (newMouseX != mouseX || newMouseY != mouseY) {
            tileX = newMouseX / TILE_SIZE;
            tileY = newMouseY / TILE_SIZE;
            tileIndex = tileY * TILES_IN_ROW + tileX;

            mouseX = newMouseX;
            mouseY = newMouseY;

            /* Update window title with tile coordinates and selection info */
            if (tileX >= 0 && tileX < TILES_IN_ROW && tileY >= 0 && tileY < 30) {
                if (selectedTileX >= 0 && selectedTileY >= 0) {
                    wsprintf(titleBuffer, "WiNTown Tile Viewer - Hover: %d (X:%d, Y:%d) | Selected: %d (X:%d, Y:%d)", 
                             tileIndex, tileX, tileY, 
                             selectedTileY * TILES_IN_ROW + selectedTileX, selectedTileX, selectedTileY);
                } else {
                    wsprintf(titleBuffer, "WiNTown Tile Viewer - Hover: %d (X:%d, Y:%d) | Click to select", 
                             tileIndex, tileX, tileY);
                }
            } else {
                if (selectedTileX >= 0 && selectedTileY >= 0) {
                    wsprintf(titleBuffer, "WiNTown Tile Viewer - Selected: %d (X:%d, Y:%d)", 
                             selectedTileY * TILES_IN_ROW + selectedTileX, selectedTileX, selectedTileY);
                } else {
                    wsprintf(titleBuffer, "WiNTown Tile Viewer - Click to select a tile");
                }
            }
            SetWindowText(hwnd, titleBuffer);
        }
        return 0;

    case WM_LBUTTONDOWN:
        newMouseX = LOWORD(lParam);
        newMouseY = HIWORD(lParam);
        
        /* Calculate which tile was clicked */
        tileX = newMouseX / TILE_SIZE;
        tileY = newMouseY / TILE_SIZE;
        
        /* Update selection if click is within valid tile area */
        if (tileX >= 0 && tileX < TILES_IN_ROW && tileY >= 0 && tileY < 30) {
            selectedTileX = tileX;
            selectedTileY = tileY;
            
            /* Force repaint to show new selection */
            InvalidateRect(hwnd, NULL, FALSE);
            
            addDebugLog("Selected tile at (%d,%d), index: %d", tileX, tileY, tileY * TILES_IN_ROW + tileX);
        }
        return 0;

    case WM_CLOSE:
        /* Hide window instead of destroying it */
        ShowWindow(hwnd, SW_HIDE);
        tilesWindowVisible = FALSE;
        
        /* Reset selection when closing */
        selectedTileX = -1;
        selectedTileY = -1;
        
        hMenu = GetMenu(hwndMain);
        hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */
        if (hViewMenu) {
            CheckMenuItem(hViewMenu, IDM_VIEW_TILESWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
        }
        return 0;

    case WM_DESTROY:
        hwndTiles = NULL;
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* These constants should match those in simulation.c */
#define SIM_TIMER_ID 1
#define SIM_TIMER_INTERVAL 100  /* Reasonable interval for city simulation */

/* Update menu when simulation speed changes */
void UpdateSimulationMenu(HWND hwnd, int speed) {
    /* Update menu checkmarks */
    CHECK_MENU_RADIO_ITEM(hSettingsMenu, IDM_SIM_PAUSE, IDM_SIM_FAST, IDM_SIM_PAUSE + speed, MF_BYCOMMAND);
}

/* Check if current tool supports drag drawing */
int IsToolDragSupported(void) {
    int currentTool = GetCurrentTool();
    
    /* Only single-tile linear tools support drag drawing */
    switch (currentTool) {
        case roadState:       /* Road */
        case railState:       /* Rail */
        case wireState:       /* Wire/Power */
        case bulldozerState:  /* Bulldozer */
            return 1;
        default:
            return 0;
    }
}

/* Continuous tool drawing during mouse drag */
void ToolDrag(int screenX, int screenY) {
    int mapX, mapY, lastMapX, lastMapY;
    int dx, dy, adx, ady;
    int i, step, tx, ty, dtx, dty, rx, ry;
    int lx, ly;
    
    /* Convert screen coordinates to map coordinates */
    ScreenToMap(screenX, screenY, &mapX, &mapY, xOffset, yOffset);
    ScreenToMap(lastToolX, lastToolY, &lastMapX, &lastMapY, xOffset, yOffset);
    
    /* Bounds check */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return;
    }
    if (lastMapX < 0 || lastMapX >= WORLD_X || lastMapY < 0 || lastMapY >= WORLD_Y) {
        return;
    }
    
    /* Calculate distance to move */
    dx = mapX - lastMapX;
    dy = mapY - lastMapY;
    
    /* No movement - avoid redundant placement */
    if (dx == 0 && dy == 0) {
        return;
    }
    
    adx = (dx < 0) ? -dx : dx;  /* abs(dx) */
    ady = (dy < 0) ? -dy : dy;  /* abs(dy) */
    
    /* Calculate step size using fixed-point arithmetic */
    if (adx > ady) {
        step = (adx > 0) ? (300 / adx) : 300;
    } else {
        step = (ady > 0) ? (300 / ady) : 300;
    }
    
    rx = (dx < 0) ? 1 : 0;
    ry = (dy < 0) ? 1 : 0;
    
    lx = lastMapX;
    ly = lastMapY;
    
    /* Interpolate between last position and current position */
    for (i = 0; i <= 1000 + step; i += step) {
        /* Fixed-point arithmetic: multiply by 1000 for precision */
        tx = (lastMapX * 1000) + (i * dx);
        ty = (lastMapY * 1000) + (i * dy);
        
        /* Convert back to integer coordinates */
        tx = (tx / 1000) + rx;
        ty = (ty / 1000) + ry;
        
        /* Check if we've moved to a new tile */
        dtx = (tx - lx) >= 0 ? (tx - lx) : (lx - tx);  /* abs(tx - lx) */
        dty = (ty - ly) >= 0 ? (ty - ly) : (ly - ty);  /* abs(ty - ly) */
        
        if (dtx >= 1 || dty >= 1) {
            /* For single-tile tools, fill diagonal gaps to ensure continuity */
            if ((dtx >= 1) && (dty >= 1)) {
                if (dtx > dty) {
                    /* Fill horizontal first */
                    ApplyTool(tx, ly);
                } else {
                    /* Fill vertical first */
                    ApplyTool(lx, ty);
                }
            }
            
            /* Apply tool at interpolated position */
            ApplyTool(tx, ty);
            
            lx = tx;
            ly = ty;
        }
    }
    
    /* Update last position for next drag operation */
    lastToolX = screenX;
    lastToolY = screenY;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        /* Initialize toolbar */
        CreateToolbar(hwnd, 0, 0, 108, 600);

        /* Load sprite bitmaps */
        loadSpriteBitmaps();

        /* Initialize simulation */
        DoSimInit();

        /* Refresh tileset menu after initialization to pick up any new tilesets */
        refreshTilesetMenu();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_NEW: {
            NewGameConfig config;
            if (showNewGameDialog(hwnd, &config)) {
                if (initNewGame(&config)) {
                    addGameLog("New game started successfully");
                    SetGameSpeed(SPEED_MEDIUM);
                    InvalidateRect(hwnd, NULL, TRUE);
                } else {
                    MessageBox(hwnd, "Failed to start new game.", "Error", MB_OK | MB_ICONERROR);
                }
            }
            return 0;
        }
            
        case IDM_FILE_OPEN:
            openCityDialog(hwnd);
            return 0;

        case IDM_FILE_SAVE:
            saveCity();
            return 0;

        case IDM_FILE_SAVE_AS:
            saveCityAs();
            return 0;

        case IDM_FILE_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;

        case IDM_SIM_PAUSE:
            SetGameSpeed(SPEED_PAUSED);
            return 0;

        case IDM_SIM_SLOW:
            SetGameSpeed(SPEED_SLOW);
            return 0;

        case IDM_SIM_MEDIUM:
            SetGameSpeed(SPEED_MEDIUM);
            return 0;

        case IDM_SIM_FAST:
            SetGameSpeed(SPEED_FAST);
            return 0;

        /* Scenario menu items */
        case IDM_SCENARIO_DULLSVILLE:
            if (loadScenario(1)) {
                SetGameSpeed(SPEED_MEDIUM);
            }
            return 0;

        case IDM_SCENARIO_SANFRANCISCO:
            if (loadScenario(2)) {
                SetGameSpeed(SPEED_MEDIUM);
            }
            return 0;

        case IDM_SCENARIO_HAMBURG:
            if (loadScenario(3)) {
                SetGameSpeed(SPEED_MEDIUM);
            }
            return 0;

        case IDM_SCENARIO_BERN:
            if (loadScenario(4)) {
                SetGameSpeed(SPEED_MEDIUM);
            }
            return 0;

        case IDM_SCENARIO_TOKYO:
            if (loadScenario(5)) {
                SetGameSpeed(SPEED_MEDIUM);
            }
            return 0;

        case IDM_SCENARIO_DETROIT:
            if (loadScenario(6)) {
                SetGameSpeed(SPEED_MEDIUM);
            }
            return 0;

        case IDM_SCENARIO_BOSTON:
            if (loadScenario(7)) {
                SetGameSpeed(SPEED_MEDIUM);
            }
            return 0;

        case IDM_SCENARIO_RIO:
            if (loadScenario(8)) {
                SetGameSpeed(SPEED_MEDIUM);
            }
            return 0;

        /* View menu items */
        case IDM_VIEW_INFOWINDOW:
            if (hwndInfo) {
                HMENU hMenu = GetMenu(hwnd);
                HMENU hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */
                UINT state = GetMenuState(hViewMenu, IDM_VIEW_INFOWINDOW, MF_BYCOMMAND);

                if (state & MF_CHECKED) {
                    /* Hide info window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_INFOWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
                    KillTimer(hwndInfo, INFO_TIMER_ID);
                    ShowWindow(hwndInfo, SW_HIDE);
                } else {
                    /* Show info window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_INFOWINDOW, MF_BYCOMMAND | MF_CHECKED);
                    SetTimer(hwndInfo, INFO_TIMER_ID, INFO_TIMER_INTERVAL, NULL);
                    ShowWindow(hwndInfo, SW_SHOW);
                    SetFocus(hwnd); /* Keep focus on main window */
                }
            }
            return 0;

        case IDM_VIEW_LOGWINDOW:
            if (hwndLog) {
                HMENU hMenu = GetMenu(hwnd);
                HMENU hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */
                UINT state = GetMenuState(hViewMenu, IDM_VIEW_LOGWINDOW, MF_BYCOMMAND);

                if (state & MF_CHECKED) {
                    /* Hide log window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_LOGWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
                    ShowWindow(hwndLog, SW_HIDE);
                    logWindowVisible = FALSE;
                } else {
                    /* Show log window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_LOGWINDOW, MF_BYCOMMAND | MF_CHECKED);
                    ShowWindow(hwndLog, SW_SHOW);
                    SetFocus(hwnd); /* Keep focus on main window */
                    logWindowVisible = TRUE;
                }
            }
            return 0;


        case IDM_VIEW_POWER_OVERLAY: {
            HMENU hMenu = GetMenu(hwnd);
            HMENU hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */
            
            /* Simply toggle based on current variable state */
            powerOverlayEnabled = !powerOverlayEnabled;
            
            if (powerOverlayEnabled) {
                /* Enable power overlay */
                CheckMenuItem(hViewMenu, IDM_VIEW_POWER_OVERLAY, MF_BYCOMMAND | MF_CHECKED);
                addGameLog("Power overlay enabled");
                /* Power grid is updated regularly by simulation - no immediate update needed */
            } else {
                /* Disable power overlay */
                CheckMenuItem(hViewMenu, IDM_VIEW_POWER_OVERLAY, MF_BYCOMMAND | MF_UNCHECKED);
                addGameLog("Power overlay disabled");
            }

            /* Force a redraw to show/hide the overlay */
            InvalidateRect(hwnd, NULL, TRUE);
        }
            return 0;

        case IDM_VIEW_MINIMAPWINDOW:
            if (hwndMinimap) {
                HMENU hMenu = GetMenu(hwnd);
                HMENU hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */
                UINT state = GetMenuState(hViewMenu, IDM_VIEW_MINIMAPWINDOW, MF_BYCOMMAND);

                if (state & MF_CHECKED) {
                    /* Hide minimap window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_MINIMAPWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
                    KillTimer(hwndMinimap, MINIMAP_TIMER_ID);
                    ShowWindow(hwndMinimap, SW_HIDE);
                } else {
                    /* Show minimap window */
                    CheckMenuItem(hViewMenu, IDM_VIEW_MINIMAPWINDOW, MF_BYCOMMAND | MF_CHECKED);
                    SetTimer(hwndMinimap, MINIMAP_TIMER_ID, MINIMAP_TIMER_INTERVAL, NULL);
                    ShowWindow(hwndMinimap, SW_SHOW);
                    SetFocus(hwnd); /* Keep focus on main window */
                }
            }
            return 0;

        case IDM_VIEW_TILESWINDOW:
            addDebugLog("Tiles window menu clicked, hwndTiles=%p", hwndTiles);
            if (hwndTiles) {
                HMENU hMenu = GetMenu(hwnd);
                HMENU hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */

                addDebugLog("Current tilesWindowVisible: %d", tilesWindowVisible);
                if (tilesWindowVisible) {
                    /* Hide tiles debug window */
                    addDebugLog("Hiding tiles window");
                    tilesWindowVisible = FALSE;
                    CheckMenuItem(hViewMenu, IDM_VIEW_TILESWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
                    ShowWindow(hwndTiles, SW_HIDE);
                } else {
                    /* Show tiles debug window */
                    addDebugLog("Showing tiles window");
                    tilesWindowVisible = TRUE;
                    CheckMenuItem(hViewMenu, IDM_VIEW_TILESWINDOW, MF_BYCOMMAND | MF_CHECKED);
                    ShowWindow(hwndTiles, SW_SHOWNORMAL);
                    SetForegroundWindow(hwndTiles);
                }
            } else {
                addDebugLog("hwndTiles is NULL - cannot show tiles window");
            }
            return 0;

        case IDM_VIEW_CHARTSWINDOW:
            if (hwndCharts) {
                HMENU hMenu = GetMenu(hwnd);
                HMENU hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */

                addDebugLog("Current chartsWindowVisible: %d", chartsWindowVisible);
                if (chartsWindowVisible) {
                    /* Hide charts window */
                    addDebugLog("Hiding charts window");
                    chartsWindowVisible = FALSE;
                    CheckMenuItem(hViewMenu, IDM_VIEW_CHARTSWINDOW, MF_BYCOMMAND | MF_UNCHECKED);
                    ShowChartWindow(0); /* Use existing function which handles timer */
                } else {
                    /* Show charts window */
                    addDebugLog("Showing charts window");
                    chartsWindowVisible = TRUE;
                    CheckMenuItem(hViewMenu, IDM_VIEW_CHARTSWINDOW, MF_BYCOMMAND | MF_CHECKED);
                    ShowChartWindow(1); /* Use existing function which handles timer */
                    SetForegroundWindow(hwndCharts);
                }
            } else {
                addDebugLog("hwndCharts is NULL - cannot show charts window");
            }
            return 0;

        case IDM_VIEW_TILE_DEBUG:
            {
                HMENU hMenu = GetMenu(hwnd);
                HMENU hViewMenu = GetSubMenu(hMenu, 6); /* View is the 7th menu (0-based index) */

                if (tileDebugEnabled) {
                    /* Disable tile debug */
                    tileDebugEnabled = FALSE;
                    CheckMenuItem(hViewMenu, IDM_VIEW_TILE_DEBUG, MF_BYCOMMAND | MF_UNCHECKED);
                    /* Reset window title to remove tile info */
                    SetWindowText(hwnd, "WiNTown NT");
                } else {
                    /* Enable tile debug */
                    tileDebugEnabled = TRUE;
                    CheckMenuItem(hViewMenu, IDM_VIEW_TILE_DEBUG, MF_BYCOMMAND | MF_CHECKED);
                }
            }
            return 0;

        case IDM_VIEW_TEST_SAVELOAD:
            testSaveLoad();
            return 0;

        case IDM_VIEW_ABOUT:
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT_DIALOG), hwnd, (DLGPROC)AboutDialogProc);
            return 0;

        /* Tool menu items */
        case IDM_TOOL_BULLDOZER:
            SelectTool(bulldozerState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_BULLDOZER,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_ROAD:
            SelectTool(roadState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_ROAD,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_RAIL:
            SelectTool(railState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_RAIL,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_WIRE:
            SelectTool(wireState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_WIRE,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_PARK:
            SelectTool(parkState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_PARK,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_RESIDENTIAL:
            SelectTool(residentialState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_RESIDENTIAL,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_COMMERCIAL:
            SelectTool(commercialState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_COMMERCIAL,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_INDUSTRIAL:
            SelectTool(industrialState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_INDUSTRIAL,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_FIRESTATION:
            SelectTool(fireState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_FIRESTATION,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_POLICESTATION:
            SelectTool(policeState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY,
                               IDM_TOOL_POLICESTATION, MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_STADIUM:
            SelectTool(stadiumState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_STADIUM,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_SEAPORT:
            SelectTool(seaportState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_SEAPORT,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_POWERPLANT:
            SelectTool(powerState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_POWERPLANT,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_NUCLEAR:
            SelectTool(nuclearState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_NUCLEAR,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_AIRPORT:
            SelectTool(airportState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_AIRPORT,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        case IDM_TOOL_QUERY:
            SelectTool(queryState);
            isToolActive = TRUE;
            CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_QUERY,
                               MF_BYCOMMAND);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;

        /* Spawn menu items */
        case IDM_SPAWN_HELICOPTER:
            {
                SimSprite *sprite;
                int x, y;
                int spriteCount;
                
                /* Debug info */
                spriteCount = GetSpriteCount();
                addGameLog("DEBUG: Current sprite count: %d", spriteCount);
                
                /* Spawn helicopter at center of visible area */
                x = (xOffset + (cxClient / 2)) & ~15; /* Align to 16-pixel grid */
                y = (yOffset + (cyClient / 2)) & ~15;
                
                addGameLog("DEBUG: Spawning helicopter at x=%d, y=%d", x, y);
                
                sprite = NewSprite(SPRITE_HELICOPTER, x, y);
                if (sprite) {
                    sprite->control = -1;
                    sprite->count = 150;
                    sprite->dest_x = SimRandom(WORLD_X) << 4;
                    sprite->dest_y = SimRandom(WORLD_Y) << 4;
                    addGameLog("SUCCESS: Helicopter spawned at %d,%d dest=%d,%d", x, y, sprite->dest_x, sprite->dest_y);
                } else {
                    addGameLog("FAILED: Could not spawn helicopter");
                    addGameLog("DEBUG: Max sprites=%d, current=%d", MAX_SPRITES, spriteCount);
                }
            }
            return 0;

        case IDM_SPAWN_AIRPLANE:
            {
                SimSprite *sprite;
                int x, y;
                
                /* Spawn airplane at center of visible area */
                x = (xOffset + (cxClient / 2)) & ~15;
                y = (yOffset + (cyClient / 2)) & ~15;
                
                sprite = NewSprite(SPRITE_AIRPLANE, x, y);
                if (sprite) {
                    sprite->control = -10; /* Takeoff mode */
                    addGameLog("Airplane spawned");
                } else {
                    addGameLog("Could not spawn airplane - too many sprites");
                }
            }
            return 0;

        case IDM_SPAWN_TRAIN:
            {
                SimSprite *sprite;
                int tx, ty, cx, cy, found, radius;
                short tile;

                cx = (xOffset + (cxClient / 2)) >> 4;
                cy = (yOffset + (cyClient / 2)) >> 4;
                found = 0;

                for (radius = 0; radius < 60 && !found; radius++) {
                    for (ty = cy - radius; ty <= cy + radius && !found; ty++) {
                        for (tx = cx - radius; tx <= cx + radius && !found; tx++) {
                            if (tx < 0 || tx >= WORLD_X || ty < 0 || ty >= WORLD_Y) continue;
                            if (abs(tx - cx) != radius && abs(ty - cy) != radius) continue;
                            tile = Map[ty][tx] & LOMASK;
                            if (tile >= RAILBASE && tile <= LASTRAIL) {
                                sprite = NewSprite(SPRITE_TRAIN, tx << 4, ty << 4);
                                if (sprite) {
                                    addGameLog("Train spawned on rail at %d,%d", tx, ty);
                                    found = 1;
                                }
                            }
                        }
                    }
                }
                if (!found) {
                    addGameLog("No rail found - cannot spawn train");
                }
            }
            return 0;

        case IDM_SPAWN_SHIP:
            {
                SimSprite *sprite;
                int x, y;
                
                /* Spawn ship at center of visible area */
                x = (xOffset + (cxClient / 2)) & ~15;
                y = (yOffset + (cyClient / 2)) & ~15;
                
                sprite = NewSprite(SPRITE_SHIP, x, y);
                if (sprite) {
                    addGameLog("Ship spawned");
                } else {
                    addGameLog("Could not spawn ship - too many sprites");
                }
            }
            return 0;

        case IDM_SPAWN_BUS:
            {
                SimSprite *sprite;
                int x, y;
                
                /* Spawn bus at center of visible area */
                x = (xOffset + (cxClient / 2)) & ~15;
                y = (yOffset + (cyClient / 2)) & ~15;
                
                sprite = NewSprite(SPRITE_BUS, x, y);
                if (sprite) {
                    addGameLog("Bus spawned");
                } else {
                    addGameLog("Could not spawn bus - too many sprites");
                }
            }
            return 0;
            
        /* Disaster menu items */
        case IDM_DISASTER_FIRE:
            if (!disastersDisabled) {
                makeFire(SimRandom(WORLD_X), SimRandom(WORLD_Y));
                addGameLog("Fire disaster manually triggered");
            } else {
                addGameLog("Disasters are disabled - cannot trigger fire");
            }
            return 0;
            
        case IDM_DISASTER_FLOOD:
            if (!disastersDisabled) {
                makeFlood();
                addGameLog("Flood disaster manually triggered");
            } else {
                addGameLog("Disasters are disabled - cannot trigger flood");
            }
            return 0;
            
        case IDM_DISASTER_TORNADO:
            if (!disastersDisabled) {
                makeTornado();
                addGameLog("Tornado disaster manually triggered");
            } else {
                addGameLog("Disasters are disabled - cannot trigger tornado");
            }
            return 0;
            
        case IDM_DISASTER_EARTHQUAKE:
            if (!disastersDisabled) {
                doEarthquake();
                addGameLog("Earthquake disaster manually triggered");
            } else {
                addGameLog("Disasters are disabled - cannot trigger earthquake");
            }
            return 0;
            
        case IDM_DISASTER_MONSTER:
            if (!disastersDisabled) {
                makeMonster();
                addGameLog("Monster disaster manually triggered");
            } else {
                addGameLog("Disasters are disabled - cannot trigger monster");
            }
            return 0;
            
        case IDM_DISASTER_MELTDOWN:
            if (!disastersDisabled) {
                makeMeltdown();
                addGameLog("Nuclear meltdown disaster manually triggered");
            } else {
                addGameLog("Disasters are disabled - cannot trigger meltdown");
            }
            return 0;
            
        /* Handle disaster enable/disable from Settings menu */
        case IDM_CHEATS_DISABLE_DISASTERS:
            {
                disastersDisabled = !disastersDisabled;
                CheckMenuItem(hSettingsMenu, IDM_CHEATS_DISABLE_DISASTERS, 
                            !disastersDisabled ? MF_CHECKED : MF_UNCHECKED);
                
                addDebugLog("Disasters menu toggled: Enabled=%d, Disabled=%d", !disastersDisabled, disastersDisabled);
                
                if (disastersDisabled) {
                    int x, y;
                    short tile;
                    int firesExtinguished = 0;
                    
                    addGameLog("Disasters disabled");
                    /* Clear any active disasters */
                    DisasterEvent = 0;
                    DisasterWait = 0;
                    
                    /* Extinguish all existing fires */
                    for (y = 0; y < WORLD_Y; y++) {
                        for (x = 0; x < WORLD_X; x++) {
                            tile = Map[y][x] & LOMASK;
                            if (tile >= TILE_FIRE && tile <= TILE_LASTFIRE) {
                                setMapTile(x, y, TILE_RUBBLE, BULLBIT, TILE_SET_REPLACE, "extinguish-fire");
                                firesExtinguished++;
                            }
                        }
                    }
                    
                    if (firesExtinguished > 0) {
                        addGameLog("Extinguished %d fires", firesExtinguished);
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                } else {
                    addGameLog("Disasters enabled");
                }
            }
            return 0;

        /* View menu - Budget Window */
        case IDM_VIEW_BUDGET:
            ShowBudgetWindow(hwnd);
            return 0;

        case IDM_VIEW_EVALUATION:
            ShowEvaluationWindow(hwnd);
            return 0;

        /* Settings menu items */
        case IDM_SETTINGS_LEVEL_EASY:
            SetGameLevel(LEVEL_EASY);
            return 0;
            
        case IDM_SETTINGS_LEVEL_MEDIUM:
            SetGameLevel(LEVEL_MEDIUM);
            return 0;
            
        case IDM_SETTINGS_LEVEL_HARD:
            SetGameLevel(LEVEL_HARD);
            return 0;
            
        case IDM_SETTINGS_AUTO_BUDGET:
            AutoBudget = !AutoBudget;
            CheckMenuItem(hSettingsMenu, IDM_SETTINGS_AUTO_BUDGET, AutoBudget ? MF_CHECKED : MF_UNCHECKED);
            addGameLog("Auto-budget %s", AutoBudget ? "enabled" : "disabled");
            return 0;
            
        case IDM_SETTINGS_AUTO_BULLDOZE:
            autoBulldoze = !autoBulldoze;
            CheckMenuItem(hSettingsMenu, IDM_SETTINGS_AUTO_BULLDOZE, autoBulldoze ? MF_CHECKED : MF_UNCHECKED);
            addGameLog("Auto-bulldoze %s", autoBulldoze ? "enabled" : "disabled");
            return 0;

        case IDM_SETTINGS_AUTO_GOTO:
            AutoGo = !AutoGo;
            CheckMenuItem(hSettingsMenu, IDM_SETTINGS_AUTO_GOTO, AutoGo ? MF_CHECKED : MF_UNCHECKED);
            addGameLog("Auto-goto %s", AutoGo ? "enabled" : "disabled");
            return 0;

        default:
            if (LOWORD(wParam) >= IDM_TILESET_BASE && LOWORD(wParam) < IDM_TILESET_MAX) {
                int index;
                char tilesetName[MAX_PATH];

                index = LOWORD(wParam) - IDM_TILESET_BASE;

                /* Compatible with Windows NT 4.0 */
                GetMenuString(hTilesetMenu, LOWORD(wParam), tilesetName, MAX_PATH - 1,
                              MF_BYCOMMAND);

                if (changeTileset(hwnd, tilesetName)) {
                    CHECK_MENU_RADIO_ITEM(hTilesetMenu, IDM_TILESET_BASE, IDM_TILESET_MAX - 1, 
                                       LOWORD(wParam), MF_BYCOMMAND);
                }
                return 0;
            }
        }
        return 0;

    case WM_TIMER:
        if (wParam == SIM_TIMER_ID) {
            BOOL needRedraw;
            static int minimapUpdateCounter = 0;
            static int chartUpdateCounter = 0;

            /* Run the simulation frame */
            SimFrame();

            /* Always redraw to handle animations */
            needRedraw = TRUE;

            /* Update the display */
            if (needRedraw) {
                InvalidateRect(hwnd, NULL, FALSE);
                /* Update minimap only every 20 frames (2 seconds at 100ms intervals) */
                minimapUpdateCounter++;
                if (minimapUpdateCounter >= 20) {
                    minimapUpdateCounter = 0;
                    if (hwndMinimap && IsWindowVisible(hwndMinimap)) {
                        InvalidateRect(hwndMinimap, NULL, FALSE);
                    }
                }
                /* Update chart only every 50 frames (5 seconds at 100ms intervals) */
                chartUpdateCounter++;
                if (chartUpdateCounter >= 50) {
                    chartUpdateCounter = 0;
                    if (hwndCharts && IsWindowVisible(hwndCharts)) {
                        InvalidateRect(hwndCharts, NULL, FALSE);
                    }
                }
            }
            return 0;
        } else if (wParam == EARTHQUAKE_TIMER_ID) {
            /* Earthquake duration timer expired - stop shaking */
            stopEarthquake();
            return 0;
        }
        return 0;

    case WM_QUERYNEWPALETTE: {
        /* Realize the palette when window gets focus */
        if (hPalette != NULL) {
            HDC hdc = GetDC(hwnd);
            SelectPalette(hdc, hPalette, FALSE);
            RealizePalette(hdc);
            InvalidateRect(hwnd, NULL, FALSE);
            ReleaseDC(hwnd, hdc);
            return TRUE;
        }
        return FALSE;
    }

    case WM_PALETTECHANGED: {
        /* Realize palette if it was changed by another window */
        if ((HWND)wParam != hwnd && hPalette != NULL) {
            HDC hdc = GetDC(hwnd);
            SelectPalette(hdc, hPalette, FALSE);
            RealizePalette(hdc);
            UpdateColors(hdc);
            ReleaseDC(hwnd, hdc);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;
        int shakeX = 0, shakeY = 0;
        int i;

        hdc = BeginPaint(hwnd, &ps);

        /* Select and realize palette for proper 8-bit color rendering */
        if (hPalette) {
            SelectPalette(hdc, hPalette, FALSE);
            RealizePalette(hdc);
        }

        if (hbmBuffer) {
            /* Draw everything to our offscreen buffer with the correct palette */
            if (hPalette) {
                SelectPalette(hdcBuffer, hPalette, FALSE);
                RealizePalette(hdcBuffer);
            }

            /* Draw the city to our buffer */
            drawCity(hdcBuffer);

            /* Calculate earthquake shake offset */
            if (shakeNow > 0) {
                for (i = 0; i < shakeNow; i++) {
                    shakeX += (SimRandom(16) - 8);
                    shakeY += (SimRandom(16) - 8);
                }
                /* Limit shake to reasonable bounds */
                if (shakeX > 20) shakeX = 20;
                if (shakeX < -20) shakeX = -20;
                if (shakeY > 20) shakeY = 20;
                if (shakeY < -20) shakeY = -20;
            }
            
            /* Copy the buffer to the screen with offset for toolbar and earthquake shake */
            BitBlt(hdc, toolbarWidth + shakeX, shakeY, cxClient - toolbarWidth, cyClient, 
                   hdcBuffer, 0, 0, SRCCOPY);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);

        /* Skip toolbar area */
        if (xPos < toolbarWidth) {
            return 0;
        }

        if (isToolActive) {
            /* Apply the tool at this position */
            int result = HandleToolMouse(xPos, yPos, xOffset, yOffset);

            /* Initialize tool dragging for supported tools */
            if (IsToolDragSupported()) {
                isToolDragging = TRUE;
                lastToolX = xPos;
                lastToolY = yPos;
                SetCapture(hwnd);
            }

            /* Display result if needed */
            if (result == TOOLRESULT_NO_MONEY) {
                addGameLog("TOOL ERROR: Not enough money!");
            } else if (result == TOOLRESULT_NEED_BULLDOZE) {
                addGameLog("TOOL ERROR: You need to bulldoze this area first!");
            } else if (result == TOOLRESULT_FAILED) {
                addGameLog("TOOL ERROR: Can't build there!");
            }
        } else {
            /* Regular map dragging */
            isMouseDown = TRUE;
            lastMouseX = xPos;
            lastMouseY = yPos;
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        }

        return 0;
    }

    case WM_MOUSEMOVE: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);
        int mapX, mapY;

        /* Skip toolbar area */
        if (xPos < toolbarWidth) {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return 0;
        }

        /* Tile debug functionality - show tile info under cursor */
        if (tileDebugEnabled) {
            ScreenToMap(xPos, yPos, &mapX, &mapY, xOffset, yOffset);
            if (mapX >= 0 && mapX < WORLD_X && mapY >= 0 && mapY < WORLD_Y) {
                short tileValue = Map[mapY][mapX];
                short baseTile = tileValue & LOMASK;
                short flags = tileValue & ~LOMASK;
                char flagStr[128];
                char titleBuffer[256];
                
                /* Convert flags to readable string */
                flagStr[0] = '\0';
                if (flags & POWERBIT) strcat(flagStr, "PWR ");
                if (flags & CONDBIT) strcat(flagStr, "COND ");
                if (flags & BURNBIT) strcat(flagStr, "BURN ");
                if (flags & BULLBIT) strcat(flagStr, "BULL ");
                if (flags & ANIMBIT) strcat(flagStr, "ANIM ");
                if (flags & ZONEBIT) strcat(flagStr, "ZONE ");
                if (flagStr[0] == '\0') strcpy(flagStr, "NONE");
                
                /* Update window title with tile information */
                wsprintf(titleBuffer, "WiNTown NT - Tile Debug: [%d,%d] = %s (%d/0x%04X) base=%d flags=[%s]", 
                         mapX, mapY, GetZoneName(tileValue), tileValue, tileValue, baseTile, flagStr);
                SetWindowText(hwnd, titleBuffer);
            }
        }

        if (isMouseDown) {
            int dx = lastMouseX - xPos;
            int dy = lastMouseY - yPos;

            lastMouseX = xPos;
            lastMouseY = yPos;

            if (dx != 0 || dy != 0) {
                scrollView(dx, dy);
            }

            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        } else if (isToolDragging) {
            /* Continuous tool drawing during drag */
            ToolDrag(xPos, yPos);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        } else if (isToolActive) {
            /* Convert mouse position to map coordinates for tool hover */
            ScreenToMap(xPos, yPos, &mapX, &mapY, xOffset, yOffset);

            /* Use normal cursor instead of crosshair */
            SetCursor(LoadCursor(NULL, IDC_ARROW));

            /* Force a partial redraw to show hover effect only if needed */
            {
                RECT updateRect;
                updateRect.left = toolbarWidth;
                updateRect.top = 0;
                updateRect.right = cxClient;
                updateRect.bottom = cyClient;
                InvalidateRect(hwnd, &updateRect, FALSE);
            }
        } else {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        isMouseDown = FALSE;
        
        /* Stop tool dragging if it was active */
        if (isToolDragging) {
            isToolDragging = FALSE;
        }
        
        ReleaseCapture();

        /* Always use arrow cursor */
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return 0;
    }

    case WM_RBUTTONDOWN: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);

        /* Skip toolbar area */
        if (xPos < toolbarWidth) {
            return 0;
        }

        /* Start map dragging with right button regardless of tool state */
        isMouseDown = TRUE;
        lastMouseX = xPos;
        lastMouseY = yPos;
        SetCapture(hwnd);
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));

        return 0;
    }

    case WM_RBUTTONUP: {
        isMouseDown = FALSE;
        ReleaseCapture();

        /* Always use arrow cursor */
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return 0;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            if (isMouseDown) {
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
            } else if (isToolActive) {
                SetCursor(LoadCursor(NULL, IDC_ARROW)); /* Use normal cursor instead of cross */
            } else {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            return TRUE;
        }
        return 0;
    }

    case WM_SIZE: {
        HWND hwndToolbarWnd;

        /* Get full client area */
        cxClient = LOWORD(lParam);
        cyClient = HIWORD(lParam);

        /* Get the toolbar window handle */
        hwndToolbarWnd = FindWindow("WiNTownToolbar", NULL);

        /* If the toolbar exists, adjust its position */
        if (hwndToolbarWnd) {
            MoveWindow(hwndToolbarWnd, 0, 0, toolbarWidth, cyClient, TRUE);
        } else {
            /* Create the toolbar if it doesn't exist yet */
            CreateToolbar(hwnd, 0, 0, toolbarWidth, cyClient);
        }

        /* Resize the drawing buffer to the client area less the toolbar */
        resizeBuffer(cxClient - toolbarWidth, cyClient);

        /* Adjust the xOffset to account for the toolbar */
        xOffset = toolbarWidth;

        return 0;
    }

    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_LEFT:
            scrollView(-TILE_SIZE, 0);
            break;

        case VK_RIGHT:
            scrollView(TILE_SIZE, 0);
            break;

        case VK_UP:
            scrollView(0, -TILE_SIZE);
            break;

        case VK_DOWN:
            scrollView(0, TILE_SIZE);
            break;

        case 'O':
            if (GetKeyState(VK_CONTROL) < 0) {
                openCityDialog(hwnd);
            }
            break;

        case 'Q':
            if (GetKeyState(VK_CONTROL) < 0) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            break;

        case 'I':
            if (hbmTiles) {
                BITMAP bm;
                char debugMsg[256];

                if (GetObject(hbmTiles, sizeof(BITMAP), &bm)) {
                    wsprintf(debugMsg, "Bitmap Info:\nDimensions: %dx%d\nBits/pixel: %d",
                             bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
                    MessageBox(hwnd, debugMsg, "Bitmap Information", MB_OK);
                }
            }
            break;

        /* Speed control keyboard shortcuts */
        case VK_SPACE:
            TogglePause();
            break;

        case '1':
            SetGameSpeed(SPEED_SLOW);
            break;

        case '2':
            SetGameSpeed(SPEED_MEDIUM);
            break;

        case '3':
            SetGameSpeed(SPEED_FAST);
            break;

        case '0':
            SetGameSpeed(SPEED_PAUSED);
            break;

        }
        return 0;
    }

    case WM_DESTROY:
        CleanupSimTimer(hwnd);
        cleanupGraphics();

        /* Clean up toolbar */
        if (FindWindow("WiNTownToolbar", NULL)) {
            DestroyWindow(FindWindow("WiNTownToolbar", NULL));
        }

        /* Clean up toolbar bitmaps */
        CleanupToolbarBitmaps();
        
        /* Clean up chart system */
        CleanupChartSystem();

        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void swapShorts(short *buf, int len) {
    int i;

    for (i = 0; i < len; i++) {
        buf[i] = ((buf[i] & 0xFF) << 8) | ((buf[i] & 0xFF00) >> 8);
    }
}

HPALETTE createSystemPalette(void) {
    LOGPALETTE *pLogPal;
    HPALETTE hPal;
    int i, r, g, b;
    int gray;
    PALETTEENTRY sysColors[16];

    /* Setup the standard 16 colors */
    sysColors[0].peRed = 0;
    sysColors[0].peGreen = 0;
    sysColors[0].peBlue = 0;
    sysColors[0].peFlags = 0; /* Black */
    sysColors[1].peRed = 128;
    sysColors[1].peGreen = 0;
    sysColors[1].peBlue = 0;
    sysColors[1].peFlags = 0; /* Dark Red */
    sysColors[2].peRed = 0;
    sysColors[2].peGreen = 128;
    sysColors[2].peBlue = 0;
    sysColors[2].peFlags = 0; /* Dark Green */
    sysColors[3].peRed = 128;
    sysColors[3].peGreen = 128;
    sysColors[3].peBlue = 0;
    sysColors[3].peFlags = 0; /* Dark Yellow */
    sysColors[4].peRed = 0;
    sysColors[4].peGreen = 0;
    sysColors[4].peBlue = 128;
    sysColors[4].peFlags = 0; /* Dark Blue */
    sysColors[5].peRed = 128;
    sysColors[5].peGreen = 0;
    sysColors[5].peBlue = 128;
    sysColors[5].peFlags = 0; /* Dark Magenta */
    sysColors[6].peRed = 0;
    sysColors[6].peGreen = 128;
    sysColors[6].peBlue = 128;
    sysColors[6].peFlags = 0; /* Dark Cyan */
    sysColors[7].peRed = 192;
    sysColors[7].peGreen = 192;
    sysColors[7].peBlue = 192;
    sysColors[7].peFlags = 0; /* Light Gray */
    sysColors[8].peRed = 128;
    sysColors[8].peGreen = 128;
    sysColors[8].peBlue = 128;
    sysColors[8].peFlags = 0; /* Dark Gray */
    sysColors[9].peRed = 255;
    sysColors[9].peGreen = 0;
    sysColors[9].peBlue = 0;
    sysColors[9].peFlags = 0; /* Red */
    sysColors[10].peRed = 0;
    sysColors[10].peGreen = 255;
    sysColors[10].peBlue = 0;
    sysColors[10].peFlags = 0; /* Green */
    sysColors[11].peRed = 255;
    sysColors[11].peGreen = 255;
    sysColors[11].peBlue = 0;
    sysColors[11].peFlags = 0; /* Yellow */
    sysColors[12].peRed = 0;
    sysColors[12].peGreen = 0;
    sysColors[12].peBlue = 255;
    sysColors[12].peFlags = 0; /* Blue */
    sysColors[13].peRed = 255;
    sysColors[13].peGreen = 0;
    sysColors[13].peBlue = 255;
    sysColors[13].peFlags = 0; /* Magenta */
    sysColors[14].peRed = 0;
    sysColors[14].peGreen = 255;
    sysColors[14].peBlue = 255;
    sysColors[14].peFlags = 0; /* Cyan */
    sysColors[15].peRed = 255;
    sysColors[15].peGreen = 255;
    sysColors[15].peBlue = 255;
    sysColors[15].peFlags = 0; /* White */

    /* Allocate memory for 256 color entries */
    pLogPal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) + 255 * sizeof(PALETTEENTRY));
    if (!pLogPal) {
        return NULL;
    }

    pLogPal->palVersion = 0x300;  /* Windows 3.0 */
    pLogPal->palNumEntries = 256; /* 256 colors (8-bit) */

    /* Copy system colors */
    for (i = 0; i < 16; i++) {
        pLogPal->palPalEntry[i] = sysColors[i];
    }

    /* Create a 6x6x6 color cube for the next 216 entries (6 levels each for R, G, B) */
    /* This is similar to the "Web Safe" palette */
    i = 16; /* Start after system colors */
    for (r = 0; r < 6; r++) {
        for (g = 0; g < 6; g++) {
            for (b = 0; b < 6; b++) {
                pLogPal->palPalEntry[i].peRed = r * 51; /* 51 is 255/5 */
                pLogPal->palPalEntry[i].peGreen = g * 51;
                pLogPal->palPalEntry[i].peBlue = b * 51;
                pLogPal->palPalEntry[i].peFlags = 0;
                i++;
            }
        }
    }

    /* Fill the remaining entries with grayscale values */
    for (; i < 256; i++) {
        gray = (i - 232) * 10 + 8; /* 24 grayscale entries, from light gray to near-white */
        if (gray > 255) {
            gray = 255;
        }

        pLogPal->palPalEntry[i].peRed = gray;
        pLogPal->palPalEntry[i].peGreen = gray;
        pLogPal->palPalEntry[i].peBlue = gray;
        pLogPal->palPalEntry[i].peFlags = 0;
    }

    hPal = CreatePalette(pLogPal);
    free(pLogPal);

    return hPal;
}

void initializeGraphics(HWND hwnd) {
    HDC hdc;
    RECT rect;
    char tilePath[MAX_PATH];
    int width;
    int height;
    BITMAPINFOHEADER bi;
    HBITMAP hbmOld;
    char errorMsg[256];
    DWORD error;
    void* bits;
    

    hdc = GetDC(hwnd);
    
    /* Initialize tile base lookup table for getBaseFromTile() macro */
    initTileBaseLookup();

    /* Create our 256-color palette */
    if (hPalette == NULL) {
        hPalette = createSystemPalette();

        if (hPalette) {
            SelectPalette(hdc, hPalette, FALSE);
            RealizePalette(hdc);
        } else {
            OutputDebugString("Failed to create palette!");
        }
    }

    hdcBuffer = CreateCompatibleDC(hdc);

    if (hPalette) {
        SelectPalette(hdcBuffer, hPalette, FALSE);
        RealizePalette(hdcBuffer);
    }

    GetClientRect(hwnd, &rect);
    cxClient = rect.right - rect.left;
    cyClient = rect.bottom - rect.top;

    width = cxClient;
    height = cyClient;

#if NEW32
    /* Setup 8-bit DIB section for our drawing buffer */
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; /* Negative for top-down DIB */
    bi.biPlanes = 1;
    bi.biBitCount = 8; /* 8 bits = 256 colors */
    bi.biCompression = BI_RGB;

    hbmBuffer = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
#else
	/* Setup 8-bit DIB section for our drawing buffer */
    ZeroMemory(&binfo, sizeof(BITMAPINFO));
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; /* Negative for top-down DIB */
    bi.biPlanes = 1;
    bi.biBitCount = 8; /* 8 bits = 256 colors */
    bi.biCompression = BI_RGB;

	hbmBuffer = CreateDIBitmap(hdc, 
		&bi,				/* Pointer to BITMAPINFOHEADER */
		CBM_INIT,			/* Initialize bitmap bits */
		NULL,				/* Pointer to actual bitmap bits (if any) */
		&binfo,				/* Pointer to BITMAPINFO */
		DIB_RGB_COLORS);		/* Color usage */
#endif

    if (hbmBuffer == NULL) {
        error = GetLastError();

        wsprintf(errorMsg, "Failed to create buffer DIB Section: Error %d", error);
        OutputDebugString(errorMsg);
        ReleaseDC(hwnd, hdc);
        return;
    }

    hbmOld = SelectObject(hdcBuffer, hbmBuffer);

    /* Fill with black background */
    FillRect(hdcBuffer, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    /* Use default.bmp tileset by default */
    strcpy(currentTileset, "default");
    wsprintf(tilePath, "%s.bmp", currentTileset);
    wsprintf(errorMsg,"LOADING TILESET %s FROM RESOURCES\n",tilePath);
    OutputDebugString(errorMsg);
    /* Load the tileset from embedded resources */
    if (!loadTileset(tilePath)) {
        OutputDebugString("Failed to load default tileset! Trying classic as fallback...");
        /* Fallback to classic if default is not available */
        strcpy(currentTileset, "classic");
        wsprintf(tilePath, "%s.bmp", currentTileset);
        
        if (!loadTileset(tilePath)) {
            OutputDebugString("Failed to load classic tileset too!");
        }
    }

    ReleaseDC(hwnd, hdc);
}


int loadTileset(const char *filename) {
    HDC hdc;
    char errorMsg[256];
    DWORD error;
    BITMAP bm;
    char debugMsg[256];
    char tilesetName[MAX_PATH];
    char* namePtr;
    int resourceId;

    if (hdcTiles) {
        DeleteDC(hdcTiles);
        hdcTiles = NULL;
    }

    if (hbmTiles) {
        DeleteObject(hbmTiles);
        hbmTiles = NULL;
    }

    /* Extract tileset name from full path */
    namePtr = strrchr(filename, '\\');
    if (namePtr) {
        namePtr++; /* Skip the backslash */
    } else {
        namePtr = (char*)filename;
    }
    strcpy(tilesetName, namePtr);

    /* Try to load from embedded resources first */
    resourceId = findTilesetResourceByName(tilesetName);
    if (resourceId != 0) {
        wsprintf(debugMsg, "Loading tileset %s from embedded resources (ID: %d)\n", tilesetName, resourceId);
        OutputDebugString(debugMsg);
        
        hbmTiles = loadTilesetFromResource(resourceId);
        if (hbmTiles != NULL) {
            wsprintf(debugMsg, "Successfully loaded tileset %s from resources\n", tilesetName);
            OutputDebugString(debugMsg);
        }
    }

    /* If resource not found, report it */
    if (hbmTiles == NULL) {
        wsprintf(debugMsg, "Tileset %s not found in embedded resources\n", tilesetName);
        OutputDebugString(debugMsg);
    }

    if (hbmTiles == NULL) {
        /* Output debug message */
        error = GetLastError();

        wsprintf(errorMsg, "Failed to load tileset: %s, Error: %d", filename, error);
        OutputDebugString(errorMsg);
        return 0;
    }

    /* Verify that the bitmap was loaded properly */
    if (GetObject(hbmTiles, sizeof(BITMAP), &bm)) {
        wsprintf(debugMsg, "Tileset Info: %dx%d, %d bits/pixel", bm.bmWidth, bm.bmHeight,
                 bm.bmBitsPixel);
        OutputDebugString(debugMsg);

        /* Convert all images to uncompressed 8-bit format */
        {
            HBITMAP hConvertedBitmap;
            HDC hdcTemp;
            
            hdcTemp = GetDC(hwndMain);
            hConvertedBitmap = convertTo8Bit(hbmTiles, hdcTemp, hPalette);
            ReleaseDC(hwndMain, hdcTemp);
            
            if (hConvertedBitmap) {
                /* Always replace original with uncompressed bitmap */
                DeleteObject(hbmTiles);
                hbmTiles = hConvertedBitmap;
            }
        }
        
        /* Validate final tileset format */
        validateTilesetFormat(hbmTiles);
    }

    hdc = GetDC(hwndMain);
    hdcTiles = CreateCompatibleDC(hdc);

    /* Apply our palette to the tileset - be more forceful about it */
    if (hPalette) {
        SelectPalette(hdcTiles, hPalette, FALSE);
        RealizePalette(hdcTiles);
    }

    SelectObject(hdcTiles, hbmTiles);

    /* Force a background color update to use our palette */
    SetBkMode(hdcTiles, TRANSPARENT);
    
    ReleaseDC(hwndMain, hdc);
    return 1;
}

int changeTileset(HWND hwnd, const char *tilesetName) {
    char windowTitle[MAX_PATH];
    HDC hdc;
    char errorMsg[256];
    BITMAP bm;
    char debugMsg[256];
    char tilesetFilename[MAX_PATH];
    int resourceId;

    wsprintf(tilesetFilename, "%s.bmp", tilesetName);

    if (hdcTiles) {
        DeleteDC(hdcTiles);
        hdcTiles = NULL;
    }

    if (hbmTiles) {
        DeleteObject(hbmTiles);
        hbmTiles = NULL;
    }

    /* Load from embedded resources only */
    resourceId = findTilesetResourceByName(tilesetFilename);
    if (resourceId != 0) {
        wsprintf(debugMsg, "Changing to tileset %s from embedded resources (ID: %d)\n", tilesetName, resourceId);
        OutputDebugString(debugMsg);
        
        hbmTiles = loadTilesetFromResource(resourceId);
        if (hbmTiles != NULL) {
            wsprintf(debugMsg, "Successfully changed to tileset %s from resources\n", tilesetName);
            OutputDebugString(debugMsg);
        }
    } else {
        wsprintf(debugMsg, "Tileset %s not found in embedded resources\n", tilesetName);
        OutputDebugString(debugMsg);
    }

    if (hbmTiles == NULL) {
        /* Try fallback to default if requested tileset isn't 'default' */
        if (strcmp(tilesetName, "default") != 0) {
            wsprintf(errorMsg, "Trying to fallback to default tileset");
            OutputDebugString(errorMsg);
            
            resourceId = findTilesetResourceByName("default.bmp");
            if (resourceId != 0) {
                hbmTiles = loadTilesetFromResource(resourceId);
            }
                              
            if (hbmTiles == NULL) {
                /* If default also fails, give up */
                wsprintf(errorMsg, "Failed to load default fallback from resources");
                OutputDebugString(errorMsg);
                return 0;
            }
        } else {
            /* If we're already trying to load default and it failed, give up */
            return 0;
        }
    }

    /* Verify that the bitmap was loaded properly */
    if (GetObject(hbmTiles, sizeof(BITMAP), &bm)) {
        wsprintf(debugMsg, "Changed Tileset Info: %dx%d, %d bits/pixel", bm.bmWidth, bm.bmHeight,
                 bm.bmBitsPixel);
        OutputDebugString(debugMsg);

        /* Convert all images to uncompressed 8-bit format */
        {
            HBITMAP hConvertedBitmap;
            HDC hdcTemp;
            
            hdcTemp = GetDC(hwndMain);
            hConvertedBitmap = convertTo8Bit(hbmTiles, hdcTemp, hPalette);
            ReleaseDC(hwndMain, hdcTemp);
            
            if (hConvertedBitmap) {
                /* Always replace original with uncompressed bitmap */
                DeleteObject(hbmTiles);
                hbmTiles = hConvertedBitmap;
            }
        }
        
        /* Validate final tileset format */
        validateTilesetFormat(hbmTiles);
    }

    hdc = GetDC(hwndMain);
    hdcTiles = CreateCompatibleDC(hdc);

    /* Apply our palette to the tileset - be more forceful about it */
    if (hPalette) {
        SelectPalette(hdcTiles, hPalette, FALSE);
        RealizePalette(hdcTiles);
    }

    SelectObject(hdcTiles, hbmTiles);
    
    /* Force a background color update to use our palette */
    SetBkMode(hdcTiles, TRANSPARENT);

    /* Update current tileset name */
    strcpy(currentTileset, tilesetName);

    wsprintf(windowTitle, "WiNTown - Tileset: %s", currentTileset);
    SetWindowText(hwnd, windowTitle);

    /* Force a full redraw */
    InvalidateRect(hwnd, NULL, TRUE);
    
    /* Also invalidate tiles debug window if it exists and is visible */
    if (hwndTiles && tilesWindowVisible) {
        InvalidateRect(hwndTiles, NULL, TRUE);
    }

    ReleaseDC(hwndMain, hdc);
    return 1;
}

void cleanupGraphics(void) {
    if (hbmBuffer) {
        DeleteObject(hbmBuffer);
        hbmBuffer = NULL;
    }

    if (hdcBuffer) {
        DeleteDC(hdcBuffer);
        hdcBuffer = NULL;
    }

    if (hbmTiles) {
        DeleteObject(hbmTiles);
        hbmTiles = NULL;
    }

    if (hdcTiles) {
        DeleteDC(hdcTiles);
        hdcTiles = NULL;
    }

    if (hPalette) {
        DeleteObject(hPalette);
        hPalette = NULL;
    }
}

void resizeBuffer(int cx, int cy) {
    HDC hdc;
    HBITMAP hbmNew;
    RECT rcBuffer;
    BITMAPINFOHEADER bi;
    BITMAPINFO binfo;
    char errorMsg[256];
    DWORD error;

    if (cx <= 0 || cy <= 0) {
        return;
    }

    hdc = GetDC(hwndMain);

    /* Make sure our palette is selected into the DC */
    if (hPalette) {
        SelectPalette(hdc, hPalette, FALSE);
        RealizePalette(hdc);
    }

#if NEW32XX
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = cx;
    bi.biHeight = -cy; /* Negative for top-down DIB */
    bi.biPlanes = 1;
    bi.biBitCount = 8; /* 8 bits = 256 colors */
    bi.biCompression = BI_RGB;

    /* Create DIB section with our palette */
    hbmNew = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
#else
    ZeroMemory(&binfo, sizeof(BITMAPINFO));
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = cx;
    bi.biHeight = -cy; /* Negative for top-down DIB */
    bi.biPlanes = 1;
    bi.biBitCount = 8; /* 8 bits = 256 colors */
    bi.biCompression = BI_RGB;

	hbmNew = CreateDIBitmap(hdc, 
		&bi,				/* Pointer to BITMAPINFOHEADER */
		CBM_INIT,			/* Initialize bitmap bits */
		NULL,				/* Pointer to actual bitmap bits (if any) */
		&binfo,				/* Pointer to BITMAPINFO */
		DIB_RGB_COLORS);	/* Color usage */
#endif

    if (hbmNew == NULL) {
        /* Debug output for DIB creation failure */
        error = GetLastError();

        wsprintf(errorMsg, "Failed to create DIB Section: Error %d", error);
        OutputDebugString(errorMsg);
        ReleaseDC(hwndMain, hdc);
        return;
    }

    if (hbmBuffer) {
        DeleteObject(hbmBuffer);
    }

    hbmBuffer = hbmNew;
    SelectObject(hdcBuffer, hbmBuffer);

    /* Apply the palette to our buffer */
    if (hPalette) {
        SelectPalette(hdcBuffer, hPalette, FALSE);
        RealizePalette(hdcBuffer);
    }

    rcBuffer.left = 0;
    rcBuffer.top = 0;
    rcBuffer.right = cx;
    rcBuffer.bottom = cy;
    FillRect(hdcBuffer, &rcBuffer, (HBRUSH)GetStockObject(BLACK_BRUSH));

    ReleaseDC(hwndMain, hdc);

    InvalidateRect(hwndMain, NULL, FALSE);
}

void scrollView(int dx, int dy) {
    RECT rcClient;
    HRGN hRgn;

    /* Adjust offsets */
    xOffset += dx;
    yOffset += dy;

    /* Enforce bounds */
    if (xOffset < 0) {
        xOffset = 0;
    }
    if (yOffset < 0) {
        yOffset = 0;
    }

    if (xOffset > WORLD_X * TILE_SIZE - cxClient) {
        xOffset = WORLD_X * TILE_SIZE - cxClient;
    }
    if (yOffset > WORLD_Y * TILE_SIZE - cyClient) {
        yOffset = WORLD_Y * TILE_SIZE - cyClient;
    }

    /* Get client area without toolbar */
    GetClientRect(hwndMain, &rcClient);
    rcClient.left = toolbarWidth; /* Skip toolbar area */

    /* Create update region for the map area only */
    hRgn = CreateRectRgnIndirect(&rcClient);

    /* Update only the map region, without erasing the background */
    InvalidateRgn(hwndMain, hRgn, FALSE);
    DeleteObject(hRgn);
    
    /* Update minimap to show new viewport position */
    if (hwndMinimap && IsWindowVisible(hwndMinimap)) {
        InvalidateRect(hwndMinimap, NULL, FALSE);
    }
}

/* Internal function to load file data */
int loadFile(char *filename) {
    FILE *f;
    DWORD size;
    size_t readResult;

    f = fopen(filename, "rb");
    if (f == NULL) {
        return 0;
    }

    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    /* The original WiNTown city files are 27120 bytes */
    if (size != 27120) {
        fclose(f);
        return 0;
    }

    readResult = fread(ResHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(ResHis, HISTLEN / 2);

    readResult = fread(ComHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(ComHis, HISTLEN / 2);

    readResult = fread(IndHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(IndHis, HISTLEN / 2);

    readResult = fread(CrimeHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(CrimeHis, HISTLEN / 2);

    readResult = fread(PollutionHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(PollutionHis, HISTLEN / 2);

    readResult = fread(MoneyHis, sizeof(short), HISTLEN / 2, f);
    if (readResult != HISTLEN / 2) {
        goto read_error;
    }
    swapShorts(MoneyHis, HISTLEN / 2);

    readResult = fread(MiscHis, sizeof(short), MISCHISTLEN / 2, f);
    if (readResult != MISCHISTLEN / 2) {
        goto read_error;
    }
    swapShorts(MiscHis, MISCHISTLEN / 2);

    /* Extract game state data from MiscHis array */
    {
        QUAD l;
        
        /* Extract TotalFunds from MiscHis[50-51] */
        /* Reconstruct QUAD from two shorts with half-word swap (like original HALF_SWAP_LONGS) */
        l = (QUAD)MiscHis[51] | ((QUAD)MiscHis[50] << 16);
        TotalFunds = l;
        
        /* Extract CityTime from MiscHis[8-9] */
        /* Reconstruct QUAD from two shorts with half-word swap (like original HALF_SWAP_LONGS) */
        l = (QUAD)MiscHis[9] | ((QUAD)MiscHis[8] << 16);
        CityTime = l;
        
        EMarket = (float)MiscHis[1];
        ResPop = MiscHis[2];
        ComPop = MiscHis[3];
        IndPop = MiscHis[4];
        RValve = MiscHis[5];
        CValve = MiscHis[6];
        IValve = MiscHis[7];
        CrimeRamp = MiscHis[10];
        PolluteRamp = MiscHis[11];
        LVAverage = MiscHis[12];
        CrimeAverage = MiscHis[13];
        PollutionAverage = MiscHis[14];
        GameLevel = MiscHis[15];
        CityClass = MiscHis[16];
        CityScore = MiscHis[17];

        AutoBulldoze = MiscHis[52];
        AutoBudget = MiscHis[53];
        AutoGo = MiscHis[54];
        TaxRate = MiscHis[56];
        SimSpeed = MiscHis[57];
        
        /* Extract funding percentages from fixed-point values */
        l = *(QUAD *)(MiscHis + 58);
        RoadPercent = l / 65536.0f;
        
        l = *(QUAD *)(MiscHis + 60);
        PolicePercent = l / 65536.0f;
        
        l = *(QUAD *)(MiscHis + 62);
        FirePercent = l / 65536.0f;
        
        /* Validate ranges */
        if (CityTime < 0) {
            CityTime = 0;
        }
        if (TaxRate < 0 || TaxRate > 20) {
            TaxRate = 7;
        }
        if (SimSpeed < 0 || SimSpeed > 3) {
            SimSpeed = 3;
        }
        if (RoadPercent < 0.0f || RoadPercent > 1.0f) {
            RoadPercent = 1.0f;
        }
        if (PolicePercent < 0.0f || PolicePercent > 1.0f) {
            PolicePercent = 1.0f;
        }
        if (FirePercent < 0.0f || FirePercent > 1.0f) {
            FirePercent = 1.0f;
        }
    }

    /* Original WiNTown stores map transposed compared to our array convention */
    {
        short tmpMap[WORLD_X][WORLD_Y];
        int x, y;

        readResult = fread(&tmpMap[0][0], sizeof(short), WORLD_X * WORLD_Y, f);
        if (readResult != WORLD_X * WORLD_Y) {
            goto read_error;
        }

        swapShorts((short *)tmpMap, WORLD_X * WORLD_Y);

        for (x = 0; x < WORLD_X; x++) {
            for (y = 0; y < WORLD_Y; y++) {
                setMapTile(x, y, tmpMap[x][y], 0, TILE_SET_REPLACE, "loadFile-transpose");
            }
        }
    }

    fclose(f);
    return 1;

read_error:
    fclose(f);
    return 0;
}

int saveFile(char *filename) {
    FILE *f;
    size_t writeResult;
    QUAD l;

    f = fopen(filename, "wb");
    if (f == NULL) {
        return 0;
    }

    /* Prepare game state data in MiscHis array before saving */
    /* Store TotalFunds as a QUAD in MiscHis[50-51] */
    l = TotalFunds;
    /* Note: WiNTown uses little-endian, but .cty format expects big-endian */
    /* We'll swap bytes when writing, so store in native format here */
    *(QUAD *)(MiscHis + 50) = l;

    /* Store CityTime as a QUAD in MiscHis[8-9] */
    l = CityTime;
    *(QUAD *)(MiscHis + 8) = l;

    MiscHis[1] = (short)EMarket;
    MiscHis[2] = ResPop;
    MiscHis[3] = ComPop;
    MiscHis[4] = IndPop;
    MiscHis[5] = RValve;
    MiscHis[6] = CValve;
    MiscHis[7] = IValve;
    MiscHis[10] = CrimeRamp;
    MiscHis[11] = PolluteRamp;
    MiscHis[12] = LVAverage;
    MiscHis[13] = CrimeAverage;
    MiscHis[14] = PollutionAverage;
    MiscHis[15] = GameLevel;
    MiscHis[16] = CityClass;
    MiscHis[17] = CityScore;

    MiscHis[52] = AutoBulldoze;
    MiscHis[53] = AutoBudget;
    MiscHis[54] = AutoGo;
    MiscHis[55] = 1;
    MiscHis[56] = TaxRate;
    MiscHis[57] = SimSpeed;

    /* Store funding percentages as fixed-point values */
    l = (int)(RoadPercent * 65536);
    *(QUAD *)(MiscHis + 58) = l;

    l = (int)(PolicePercent * 65536);
    *(QUAD *)(MiscHis + 60) = l;

    l = (int)(FirePercent * 65536);
    *(QUAD *)(MiscHis + 62) = l;

    /* Write history arrays - swap bytes to big-endian format */
    swapShorts(ResHis, HISTLEN / 2);
    writeResult = fwrite(ResHis, sizeof(short), HISTLEN / 2, f);
    swapShorts(ResHis, HISTLEN / 2); /* swap back to native format */
    if (writeResult != HISTLEN / 2) {
        goto write_error;
    }

    swapShorts(ComHis, HISTLEN / 2);
    writeResult = fwrite(ComHis, sizeof(short), HISTLEN / 2, f);
    swapShorts(ComHis, HISTLEN / 2);
    if (writeResult != HISTLEN / 2) {
        goto write_error;
    }

    swapShorts(IndHis, HISTLEN / 2);
    writeResult = fwrite(IndHis, sizeof(short), HISTLEN / 2, f);
    swapShorts(IndHis, HISTLEN / 2);
    if (writeResult != HISTLEN / 2) {
        goto write_error;
    }

    swapShorts(CrimeHis, HISTLEN / 2);
    writeResult = fwrite(CrimeHis, sizeof(short), HISTLEN / 2, f);
    swapShorts(CrimeHis, HISTLEN / 2);
    if (writeResult != HISTLEN / 2) {
        goto write_error;
    }

    swapShorts(PollutionHis, HISTLEN / 2);
    writeResult = fwrite(PollutionHis, sizeof(short), HISTLEN / 2, f);
    swapShorts(PollutionHis, HISTLEN / 2);
    if (writeResult != HISTLEN / 2) {
        goto write_error;
    }

    swapShorts(MoneyHis, HISTLEN / 2);
    writeResult = fwrite(MoneyHis, sizeof(short), HISTLEN / 2, f);
    swapShorts(MoneyHis, HISTLEN / 2);
    if (writeResult != HISTLEN / 2) {
        goto write_error;
    }

    swapShorts(MiscHis, MISCHISTLEN / 2);
    writeResult = fwrite(MiscHis, sizeof(short), MISCHISTLEN / 2, f);
    swapShorts(MiscHis, MISCHISTLEN / 2);
    if (writeResult != MISCHISTLEN / 2) {
        goto write_error;
    }

    /* Write map data - transpose to match original format */
    {
        short tmpMap[WORLD_X][WORLD_Y];
        int x, y;

        /* Copy map data with transposition */
        for (x = 0; x < WORLD_X; x++) {
            for (y = 0; y < WORLD_Y; y++) {
                tmpMap[x][y] = getMapTile(x, y);
            }
        }

        swapShorts((short *)tmpMap, WORLD_X * WORLD_Y);
        writeResult = fwrite(&tmpMap[0][0], sizeof(short), WORLD_X * WORLD_Y, f);
        if (writeResult != WORLD_X * WORLD_Y) {
            goto write_error;
        }
    }

    fclose(f);
    return 1;

write_error:
    fclose(f);
    return 0;
}

int saveCity(void) {
    if (cityFileName[0] == '\0') {
        /* No filename set, call Save As dialog */
        return saveCityAs();
    } else {
        if (saveFile(cityFileName)) {
            addGameLog("City saved to: %s", cityFileName);
            return 1;
        } else {
            addGameLog("ERROR: Failed to save city to: %s", cityFileName);
            return 0;
        }
    }
}

int saveCityAs(void) {
    OPENFILENAME ofn;
    char szFileName[MAX_PATH];
    char *dot;

    ZeroMemory(&ofn, sizeof(ofn));
    szFileName[0] = '\0';

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndMain;
    ofn.lpstrFilter = "City Files (*.cty)\0*.cty\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrTitle = "Save City As";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "cty";

    if (GetSaveFileName(&ofn)) {
        if (saveFile(szFileName)) {
            /* Update current filename */
            lstrcpy(cityFileName, szFileName);
            
            /* Update window title with new city name */
            {
                char windowTitle[MAX_PATH];
                char *baseName;

                baseName = cityFileName;

                if (strrchr(baseName, '\\')) {
                    baseName = strrchr(baseName, '\\') + 1;
                }
                if (strrchr(baseName, '/')) {
                    baseName = strrchr(baseName, '/') + 1;
                }

                lstrcpy(windowTitle, "WiNTown - ");
                lstrcat(windowTitle, baseName);

                /* Remove the extension if present */
                dot = strrchr(windowTitle, '.');
                if (dot) {
                    *dot = '\0';
                }

                SetWindowText(hwndMain, windowTitle);
            }

            addGameLog("City saved as: %s", szFileName);
            return 1;
        } else {
            addGameLog("ERROR: Failed to save city to: %s", szFileName);
            return 0;
        }
    }
    return 0; /* User cancelled */
}

int testSaveLoad(void) {
    char testFileName[MAX_PATH];
    char originalFileName[MAX_PATH];
    short originalMap[WORLD_X][WORLD_Y];
    short originalResHis[HISTLEN / 2];
    short originalComHis[HISTLEN / 2];
    short originalIndHis[HISTLEN / 2];
    short originalCrimeHis[HISTLEN / 2];
    short originalPollutionHis[HISTLEN / 2];
    short originalMoneyHis[HISTLEN / 2];
    short originalMiscHis[MISCHISTLEN / 2];
    QUAD originalTotalFunds;
    int originalCityTime;
    int originalTaxRate;
    int x, y, i;
    int errorsFound;

    /* Store original filename */
    lstrcpy(originalFileName, cityFileName);
    
    /* Save original data for comparison */
    originalTotalFunds = TotalFunds;
    originalCityTime = CityTime;
    originalTaxRate = TaxRate;
    
    for (x = 0; x < WORLD_X; x++) {
        for (y = 0; y < WORLD_Y; y++) {
            originalMap[x][y] = getMapTile(x, y);
        }
    }
    
    for (i = 0; i < HISTLEN / 2; i++) {
        originalResHis[i] = ResHis[i];
        originalComHis[i] = ComHis[i];
        originalIndHis[i] = IndHis[i];
        originalCrimeHis[i] = CrimeHis[i];
        originalPollutionHis[i] = PollutionHis[i];
        originalMoneyHis[i] = MoneyHis[i];
    }
    
    for (i = 0; i < MISCHISTLEN / 2; i++) {
        originalMiscHis[i] = MiscHis[i];
    }

    /* Create test filename */
    lstrcpy(testFileName, "savetest.cty");

    addGameLog("Starting save/load test...");
    
    /* Save current state to test file */
    if (!saveFile(testFileName)) {
        addGameLog("ERROR: Test failed - could not save test file");
        return 0;
    }
    
    /* Modify some data to ensure we're actually loading from file */
    TotalFunds = 12345;
    CityTime = 99999;
    TaxRate = 15;
    setMapTile(0, 0, 999, 0, TILE_SET_REPLACE, "test");
    ResHis[0] = 888;
    
    /* Load the test file */
    if (!loadFile(testFileName)) {
        addGameLog("ERROR: Test failed - could not load test file");
        return 0;
    }
    
    /* Compare the data */
    errorsFound = 0;
    
    if (TotalFunds != originalTotalFunds) {
        addGameLog("ERROR: TotalFunds mismatch - expected %d, got %d", originalTotalFunds, TotalFunds);
        errorsFound++;
    }
    
    if (CityTime != originalCityTime) {
        addGameLog("ERROR: CityTime mismatch - expected %d, got %d", originalCityTime, CityTime);
        errorsFound++;
    }
    
    if (TaxRate != originalTaxRate) {
        addGameLog("ERROR: TaxRate mismatch - expected %d, got %d", originalTaxRate, TaxRate);
        errorsFound++;
    }
    
    /* Check map data */
    for (x = 0; x < WORLD_X && errorsFound < 10; x++) {
        for (y = 0; y < WORLD_Y && errorsFound < 10; y++) {
            if (getMapTile(x, y) != originalMap[x][y]) {
                addGameLog("ERROR: Map tile mismatch at (%d,%d) - expected %d, got %d", 
                          x, y, originalMap[x][y], getMapTile(x, y));
                errorsFound++;
            }
        }
    }
    
    /* Check history arrays */
    for (i = 0; i < HISTLEN / 2 && errorsFound < 10; i++) {
        if (ResHis[i] != originalResHis[i]) {
            addGameLog("ERROR: ResHis[%d] mismatch - expected %d, got %d", i, originalResHis[i], ResHis[i]);
            errorsFound++;
        }
    }
    
    /* Clean up test file */
    DeleteFile(testFileName);
    
    /* Restore original filename */
    lstrcpy(cityFileName, originalFileName);
    
    if (errorsFound == 0) {
        addGameLog("SUCCESS: Save/load test passed - all data matches");
        return 1;
    } else {
        addGameLog("FAILED: Save/load test found %d errors", errorsFound);
        return 0;
    }
}

/* External function declarations */
extern int calcResPop(int zone);     /* Calculate residential zone population - from zone.c */
extern int calcComPop(int zone);     /* Calculate commercial zone population - from zone.c */
extern int calcIndPop(int zone);     /* Calculate industrial zone population - from zone.c */
extern void ClearCensus(void);       /* Reset census counters - from simulation.c */
extern void CityEvaluation(void);    /* Update city evaluation - from evaluation.c */
extern void TakeCensus(void);        /* Take a census - from simulation.c */
extern void CountSpecialTiles(void); /* Count special buildings - from evaluation.c */

/* Force a census calculation of the entire map */
void ForceFullCensus(void) {
    int x, y;
    short tile;
    int zoneTile;

    /* Reset census counts */
    ClearCensus();

    /* Scan entire map to count populations */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            tile = Map[y][x];

            /* Check if this is a zone center */
            if (tile & ZONEBIT) {
                zoneTile = tile & LOMASK;

                /* Population counting is handled exclusively by zone processing system */
                /* Do not count population here to avoid conflicts with zone.c system */

                /* Count other special zones */
                if (zoneTile == FIRESTATION) {
                    FirePop++;
                } else if (zoneTile == POLICESTATION) {
                    PolicePop++;
                } else if (zoneTile == STADIUM) {
                    StadiumPop++;
                    /* Stadium population counted by zone processing */
                } else if (zoneTile == PORT) {
                    PortPop++;
                    /* Port population counted by zone processing */
                } else if (zoneTile == AIRPORT) {
                    APortPop++;
                    /* Airport population counted by zone processing */
                } else if (zoneTile == NUCLEAR) {
                    NuclearPop++;
                }
                /* Note: We don't count power plants here since CountSpecialTiles() in evaluation.c
                 * handles it */

                /* Power zone counting is handled exclusively by DoPowerScan() in power.c */
                /* Do not count power zones here to avoid conflicts */
            }

            /* Count infrastructure */
            if ((tile & LOMASK) >= ROADBASE && (tile & LOMASK) <= LASTROAD) {
                RoadTotal++;
            } else if ((tile & LOMASK) >= RAILBASE && (tile & LOMASK) <= LASTRAIL) {
                RailTotal++;
            }
        }
    }

    /* Calculate total population using unified functions */
    TotalPop = CalculateTotalPopulation(ResPop, ComPop, IndPop);
    CityPop = CalculateCityPopulation(ResPop, ComPop, IndPop);

    /* Determine city class based on population */
    CityClass = 0; /* Village */
    if (CityPop > 2000) {
        CityClass++; /* Town */
    }
    if (CityPop > 10000) {
        CityClass++; /* City */
    }
    if (CityPop > 50000) {
        CityClass++; /* Capital */
    }
    if (CityPop > 100000) {
        CityClass++; /* Metropolis */
    }
    if (CityPop > 500000) {
        CityClass++; /* Megalopolis */
    }

    /* Count special buildings for city evaluation */
    CountSpecialTiles();

    /* Update the city evaluation based on the new population */
    CityEvaluation();

    /* Take census to update history graphs */
    TakeCensus();
}

int loadCity(char *filename) {
    /* Save previous population values in case we need them */
    int oldResPop;
    int oldComPop;
    int oldIndPop;
    QUAD oldCityPop;

    /* Initialize variables at the top of function for C89 compliance */
    oldResPop = ResPop;
    oldComPop = ComPop;
    oldIndPop = IndPop;
    oldCityPop = CityPop;

    /* Reset scenario ID */
    ScenarioID = 0;
    DisasterEvent = 0;
    DisasterWait = 0;

    lstrcpy(cityFileName, filename);

    if (!loadFile(filename)) {
        addGameLog("ERROR: Failed to load city file: %s", filename);
        return 0;
    }

    xOffset = (WORLD_X * TILE_SIZE - cxClient) / 2;
    yOffset = (WORLD_Y * TILE_SIZE - cyClient) / 2;
    if (xOffset < 0) {
        xOffset = 0;
    }
    if (yOffset < 0) {
        yOffset = 0;
    }

    /* First run a full census to calculate initial city population */
    ForceFullCensus();

    /* Check if we got a valid population */
    if (CityPop == 0 && (oldCityPop > 0)) {
        /* If no population detected, use values from previous city */
        ResPop = oldResPop;
        ComPop = oldComPop;
        IndPop = oldIndPop;
        TotalPop = CalculateTotalPopulation(ResPop, ComPop, IndPop);
        CityPop = oldCityPop;

        /* Update city class based on restored population */
        CityClass = 0; /* Village */
        if (CityPop > 2000) {
            CityClass++; /* Town */
        }
        if (CityPop > 10000) {
            CityClass++; /* City */
        }
        if (CityPop > 50000) {
            CityClass++; /* Capital */
        }
        if (CityPop > 100000) {
            CityClass++; /* Metropolis */
        }
        if (CityPop > 500000) {
            CityClass++; /* Megalopolis */
        }
    }

    /* Now we can initialize the simulation but preserve population */
    DoSimInit();

    /* Force a final population census calculation for the loaded city */
    ForceFullCensus();

    /* Unpause simulation at medium speed */
    SetSimulationSpeed(hwndMain, SPEED_MEDIUM);

    /* Update the window title with city name */
    {
        char windowTitle[MAX_PATH];
        char *baseName;
        char *dot;

        baseName = cityFileName;

        if (strrchr(baseName, '\\')) {
            baseName = strrchr(baseName, '\\') + 1;
        }
        if (strrchr(baseName, '/')) {
            baseName = strrchr(baseName, '/') + 1;
        }

        lstrcpy(windowTitle, "WiNTown - ");
        lstrcat(windowTitle, baseName);

        /* Remove the extension if present */
        dot = strrchr(windowTitle, '.');
        if (dot) {
            *dot = '\0';
        }

        SetWindowText(hwndMain, windowTitle);
    }

    InvalidateRect(hwndMain, NULL, FALSE);

    /* Log the city load event */
    {
        char *baseName;

        baseName = cityFileName;

        if (strrchr(baseName, '\\')) {
            baseName = strrchr(baseName, '\\') + 1;
        }
        if (strrchr(baseName, '/')) {
            baseName = strrchr(baseName, '/') + 1;
        }

        /* Log the city loading information */
        addGameLog("City loaded: %s", baseName);
        addGameLog("City stats - Population: %d, Funds: $%d", (int)CityPop, (int)TotalFunds);
        addDebugLog("City class: %s, Year: %d, Month: %d", GetCityClassName(), CityYear,
                    CityMonth + 1);
        addDebugLog("R-C-I population: %d, %d, %d", ResPop, ComPop, IndPop);
    }

    /* Refresh tileset menu to show any new tilesets */
    refreshTilesetMenu();

    return 1;
}

/* Lookup table for getBaseFromTile() - O(1) performance */
static int tileBaseLookup[1024] = {0};
static int tileBaseLookupInit = 0;

void initTileBaseLookup() {
    int i;
    
    if (tileBaseLookupInit) return;
    
    /* Initialize all to TILE_DIRT */
    for (i = 0; i < 1024; i++) {
        tileBaseLookup[i] = TILE_DIRT;
    }
    
    /* Water tiles */
    for (i = TILE_WATER_LOW; i <= TILE_WATER_HIGH; i++) {
        tileBaseLookup[i] = TILE_RIVER;
    }
    
    /* Woods tiles */
    for (i = TILE_WOODS_LOW; i <= TILE_WOODS_HIGH; i++) {
        tileBaseLookup[i] = TILE_TREEBASE;
    }
    
    /* Road tiles */
    for (i = TILE_ROADBASE; i <= TILE_LASTROAD; i++) {
        tileBaseLookup[i] = TILE_ROADBASE;
    }
    
    /* Power tiles */
    for (i = TILE_POWERBASE; i <= TILE_LASTPOWER; i++) {
        tileBaseLookup[i] = TILE_POWERBASE;
    }
    
    /* Rail tiles */
    for (i = TILE_RAILBASE; i <= TILE_LASTRAIL; i++) {
        tileBaseLookup[i] = TILE_RAILBASE;
    }
    
    /* Residential tiles */
    for (i = TILE_RESBASE; i <= TILE_LASTRES; i++) {
        tileBaseLookup[i] = TILE_RESBASE;
    }
    
    /* Commercial tiles */
    for (i = TILE_COMBASE; i <= TILE_LASTCOM; i++) {
        tileBaseLookup[i] = TILE_COMBASE;
    }
    
    /* Industrial tiles */
    for (i = TILE_INDBASE; i <= TILE_LASTIND; i++) {
        tileBaseLookup[i] = TILE_INDBASE;
    }
    
    /* Fire tiles */
    for (i = TILE_FIREBASE; i <= TILE_LASTFIRE; i++) {
        tileBaseLookup[i] = TILE_FIREBASE;
    }
    
    /* Flood tiles */
    for (i = TILE_FLOOD; i <= TILE_LASTFLOOD; i++) {
        tileBaseLookup[i] = TILE_FLOOD;
    }
    
    /* Rubble tiles */
    for (i = TILE_RUBBLE; i <= TILE_LASTRUBBLE; i++) {
        tileBaseLookup[i] = TILE_RUBBLE;
    }
    
    tileBaseLookupInit = 1;
}

/* getBaseFromTile() is now a macro for maximum performance - see below */

/* Fast macro - eliminates 273K function calls per run */
#define getBaseFromTile(tile) tileBaseLookup[(tile) & LOMASK]

void drawTile(HDC hdc, int x, int y, short tileValue) {
    RECT rect;
    HBRUSH hBrush;
    HBRUSH hOldBrush;
    int tileIndex;
    int srcX;
    int srcY;

    /* Don't treat negative values as special - they are valid tile values with the sign bit set
       In the original code, negative values in PowerMap indicated unpowered state,
       but in our implementation we need to still render the tile */

    tileIndex = tileValue & LOMASK;

    if (tileIndex >= TILE_TOTAL_COUNT) {
        tileIndex = 0;
    }

    /* Handle animated traffic tiles */
    if (tileValue & ANIMBIT) {
        /* Use low 2 bits of Fcycle for animation (0-3) */
        int frame = (Fcycle & 3);

        /* Light traffic animation range (80-127) */
        if (tileIndex >= 80 && tileIndex <= 127) {
            /* The way traffic works is:
               - Tiles 80-95 are frame 1 (use when frame=1)
               - Tiles 96-111 are frame 2 (use when frame=2)
               - Tiles 112-127 are frame 3 (use when frame=3)
               - Tiles 128-143 are frame 0 (use when frame=0)
             */
            int baseOffset = tileIndex & 0xF; /* Base road layout (0-15) */

            /* Get correct frame tiles */
            switch (frame) {
            case 0:
                tileIndex = 128 + baseOffset;
                break; /* Frame 0 */
            case 1:
                tileIndex = 80 + baseOffset;
                break; /* Frame 1 */
            case 2:
                tileIndex = 96 + baseOffset;
                break; /* Frame 2 */
            case 3:
                tileIndex = 112 + baseOffset;
                break; /* Frame 3 */
            }
        }

        /* Heavy traffic animation range (144-207) */
        if (tileIndex >= 144 && tileIndex <= 207) {
            /* Heavy traffic works the same way:
               - Tiles 144-159 are frame 1 (use when frame=1)
               - Tiles 160-175 are frame 2 (use when frame=2)
               - Tiles 176-191 are frame 3 (use when frame=3)
               - Tiles 192-207 are frame 0 (use when frame=0)
             */
            int baseOffset = tileIndex & 0xF; /* Base road layout (0-15) */

            /* Get correct frame tiles */
            switch (frame) {
            case 0:
                tileIndex = 192 + baseOffset;
                break; /* Frame 0 */
            case 1:
                tileIndex = 144 + baseOffset;
                break; /* Frame 1 */
            case 2:
                tileIndex = 160 + baseOffset;
                break; /* Frame 2 */
            case 3:
                tileIndex = 176 + baseOffset;
                break; /* Frame 3 */
            }
        }
    }

    rect.left = x;
    rect.top = y;
    rect.right = x + TILE_SIZE;
    rect.bottom = y + TILE_SIZE;

    if (hdcTiles && hbmTiles) {
        /* RISC CPU Optimization: Use lookup tables instead of division/modulo */
        if (!lookupTablesInitialized) {
            initTileCoordinateLookup();
        }
        
        srcX = tileSourceX[tileIndex];
        srcY = tileSourceY[tileIndex];

        BitBlt(hdc, x, y, TILE_SIZE, TILE_SIZE, hdcTiles, srcX, srcY, SRCCOPY);
    } else {
        /* Use pre-created brushes to avoid CreateSolidBrush overhead */
        static HBRUSH hBrushRiver = NULL;
        static HBRUSH hBrushTree = NULL;
        static HBRUSH hBrushRoad = NULL;
        static HBRUSH hBrushRail = NULL;
        static HBRUSH hBrushPower = NULL;
        static HBRUSH hBrushRes = NULL;
        static HBRUSH hBrushCom = NULL;
        static HBRUSH hBrushInd = NULL;
        static HBRUSH hBrushFire = NULL;
        static HBRUSH hBrushFlood = NULL;
        static HBRUSH hBrushRubble = NULL;
        static HBRUSH hBrushDirt = NULL;
        
        /* Initialize brushes on first call */
        if (!hBrushRiver) {
            hBrushRiver = CreateSolidBrush(RGB(0, 0, 128));        /* Dark blue */
            hBrushTree = CreateSolidBrush(RGB(0, 128, 0));         /* Dark green */
            hBrushRoad = CreateSolidBrush(RGB(128, 128, 128));     /* Gray */
            hBrushRail = CreateSolidBrush(RGB(192, 192, 192));     /* Light gray */
            hBrushPower = CreateSolidBrush(RGB(255, 255, 0));      /* Yellow */
            hBrushRes = CreateSolidBrush(RGB(0, 255, 0));          /* Green */
            hBrushCom = CreateSolidBrush(RGB(0, 0, 255));          /* Blue */
            hBrushInd = CreateSolidBrush(RGB(255, 255, 0));        /* Yellow */
            hBrushFire = CreateSolidBrush(RGB(255, 0, 0));         /* Red */
            hBrushFlood = CreateSolidBrush(RGB(0, 128, 255));      /* Light blue */
            hBrushRubble = CreateSolidBrush(RGB(128, 128, 0));     /* Olive */
            hBrushDirt = CreateSolidBrush(RGB(204, 102, 0));       /* Orange-brown */
        }
        
        switch (getBaseFromTile(tileValue)) {
        case TILE_RIVER:
            hBrush = hBrushRiver;
            break;
        case TILE_TREEBASE:
            hBrush = hBrushTree;
            break;
        case TILE_ROADBASE:
            hBrush = hBrushRoad;
            break;
        case TILE_RAILBASE:
            hBrush = hBrushRail;
            break;
        case TILE_POWERBASE:
            hBrush = hBrushPower;
            break;
        case TILE_RESBASE:
            hBrush = hBrushRes;
            break;
        case TILE_COMBASE:
            hBrush = hBrushCom;
            break;
        case TILE_INDBASE:
            hBrush = hBrushInd;
            break;
        case TILE_FIREBASE:
            hBrush = hBrushFire;
            break;
        case TILE_FLOOD:
            hBrush = hBrushFlood;
            break;
        case TILE_RUBBLE:
            hBrush = hBrushRubble;
            break;
        case TILE_DIRT:
        default:
            hBrush = hBrushDirt;
            break;
        }

        hOldBrush = SelectObject(hdc, hBrush);
        FillRect(hdc, &rect, hBrush);
        SelectObject(hdc, hOldBrush);
        /* No DeleteObject needed - brushes are cached */
    }

    /* Removed white frame for all zones to improve visual appearance */

    if ((tileValue & ZONEBIT) && !(tileValue & POWERBIT)) {
        /* Unpowered zones get a yellow frame - use cached brush */
        static HBRUSH hBrushUnpowered = NULL;
        if (!hBrushUnpowered) {
            hBrushUnpowered = CreateSolidBrush(RGB(255, 255, 0));
        }
        FrameRect(hdc, &rect, hBrushUnpowered);
        /* No DeleteObject needed - brush is cached */

        /* Draw the lightning bolt power indicator in the center of the tile */
        if (hdcTiles && hbmTiles) {
            int srcX, srcY;
            
            /* RISC CPU Optimization: Use lookup tables for LIGHTNINGBOLT coordinates */
            if (!lookupTablesInitialized) {
                initTileCoordinateLookup();
            }
            
            srcX = tileSourceX[LIGHTNINGBOLT];
            srcY = tileSourceY[LIGHTNINGBOLT];

            BitBlt(hdc, x, y, TILE_SIZE, TILE_SIZE, hdcTiles, srcX, srcY, SRCCOPY);
        }
    }
    /* Removed green frame for powered zones to improve visual appearance */

    /* Commented out power indicator dots for cleaner display */
    /*
       else if ((tileValue & CONDBIT) && (tileValue & POWERBIT))
       {
        * Power lines and other conductors with power get a cyan dot *
        int dotSize = 4;
        RECT dotRect;

        dotRect.left = rect.left + (TILE_SIZE - dotSize) / 2;
        dotRect.top = rect.top + (TILE_SIZE - dotSize) / 2;
        dotRect.right = dotRect.left + dotSize;
        dotRect.bottom = dotRect.top + dotSize;

        hBrush = CreateSolidBrush(RGB(0, 255, 255));
        FillRect(hdc, &dotRect, hBrush);
        DeleteObject(hBrush);
       }
     */

    /* Commented out traffic visualization dots for cleaner display */
    /*
       {
        int tileBase = tileValue & LOMASK;
        Byte trafficLevel;

        if ((tileBase >= ROADBASE && tileBase <= LASTROAD) ||
            (tileBase >= RAILBASE && tileBase <= LASTRAIL))
        {
            Get the traffic density for this location
            trafficLevel = TrfDensity[y/2][x/2];

            Only display if there's significant traffic
            if (trafficLevel > 40) {
                COLORREF trafficColor;
                RECT trafficRect;
                int trafficSize;

                Scale traffic visualization by density level
                if (trafficLevel < 100) {
                    trafficColor = RGB(255, 255, 0); Yellow for light traffic
                    trafficSize = 2;
                } else if (trafficLevel < 200) {
                    trafficColor = RGB(255, 128, 0); Orange for medium traffic
                    trafficSize = 3;
                } else {
                    trafficColor = RGB(255, 0, 0);   Red for heavy traffic
                    trafficSize = 4;
                }

                Draw traffic indicator
                trafficRect.left = rect.left + (TILE_SIZE - trafficSize) / 2;
                trafficRect.top = rect.top + (TILE_SIZE - trafficSize) / 2;
                trafficRect.right = trafficRect.left + trafficSize;
                trafficRect.bottom = trafficRect.top + trafficSize;

                hBrush = CreateSolidBrush(trafficColor);
                FillRect(hdc, &trafficRect, hBrush);
                DeleteObject(hBrush);
            }
        }
       }
     */
}

void drawCity(HDC hdc) {
    int x;
    int y;
    int screenX;
    int screenY;
    int startX;
    int startY;
    int endX;
    int endY;
    int cityMonth;
    int cityYear;
    int fundValue;
    int popValue;
    RECT rcClient;

    /* Copy simulation values to local variables for display */
    cityMonth = CityMonth;
    cityYear = CityYear;
    fundValue = (int)TotalFunds;

    /* Display the current population */
    popValue = (int)CityPop;

    /* Calculate visible range */
    startX = xOffset / TILE_SIZE;
    startY = yOffset / TILE_SIZE;
    /* Adjust the width of the map view based on toolbar */
    endX = startX + ((cxClient - toolbarWidth) / TILE_SIZE) + 1;
    endY = startY + (cyClient / TILE_SIZE) + 1;

    /* Bounds check */
    if (startX < 0) {
        startX = 0;
    }
    if (startY < 0) {
        startY = 0;
    }
    if (endX > WORLD_X) {
        endX = WORLD_X;
    }
    if (endY > WORLD_Y) {
        endY = WORLD_Y;
    }

    /* Clear the background */
    rcClient.left = 0;
    rcClient.top = 0;
    rcClient.right = cxClient;
    rcClient.bottom = cyClient;
    FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));

    /* Draw the map tiles */
    for (y = startY; y < endY; y++) {
        for (x = startX; x < endX; x++) {
            screenX = x * TILE_SIZE - xOffset;
            screenY = y * TILE_SIZE - yOffset;

            drawTile(hdc, screenX, screenY, Map[y][x]);

            /* If power overlay is enabled, show power status with a transparent color overlay */
            if (powerOverlayEnabled) {
                RECT tileRect;
                /* Use cached brushes and pens for power overlay to reduce GDI overhead */
                static HBRUSH hBrushPoweredZone = NULL;
                static HBRUSH hBrushUnpoweredZone = NULL;
                static HBRUSH hBrushPoweredTile = NULL;
                static HPEN hPenPoweredLine = NULL;
                
                /* Initialize cached GDI objects on first use */
                if (!hBrushPoweredZone) {
                    hBrushPoweredZone = CreateSolidBrush(RGB(0, 255, 0));    /* Bright green */
                    hBrushUnpoweredZone = CreateSolidBrush(RGB(255, 0, 0));  /* Red */
                    hBrushPoweredTile = CreateSolidBrush(RGB(0, 200, 0));    /* Green */
                    hPenPoweredLine = CreatePen(PS_SOLID, 1, RGB(0, 255, 0)); /* Green pen */
                }

                tileRect.left = screenX;
                tileRect.top = screenY;
                tileRect.right = screenX + TILE_SIZE;
                tileRect.bottom = screenY + TILE_SIZE;

                /* Skip power plants themselves */
                if ((Map[y][x] & LOMASK) != POWERPLANT && (Map[y][x] & LOMASK) != NUCLEAR) {
                    /* Show power status - use cached brushes */
                    if (Map[y][x] & ZONEBIT) {
                        if (Map[y][x] & POWERBIT) {
                            /* Powered zones - bright green border */
                            FrameRect(hdc, &tileRect, hBrushPoweredZone);
                            /* Add a small green power indicator in the corner */
                            Rectangle(hdc, tileRect.left + 2, tileRect.top + 2, tileRect.left + 6,
                                      tileRect.top + 6);
                        } else {
                            /* Unpowered zones - red overlay */
                            FrameRect(hdc, &tileRect, hBrushUnpoweredZone);
                            /* Add an X in the corner to indicate no power */
                            MoveToEx(hdc, tileRect.left + 2, tileRect.top + 2, NULL);
                            LineTo(hdc, tileRect.left + 6, tileRect.top + 6);
                            MoveToEx(hdc, tileRect.left + 6, tileRect.top + 2, NULL);
                            LineTo(hdc, tileRect.left + 2, tileRect.top + 6);
                        }
                    } else if (Map[y][x] & POWERBIT) {
                        /* Show power conducting elements (power lines, roads, etc.) clearly */
                        if ((Map[y][x] & LOMASK) >= POWERBASE &&
                            (Map[y][x] & LOMASK) < POWERBASE + 12) {
                            /* Power lines - make them bright */
                            HPEN hOldPen = SelectObject(hdc, hPenPoweredLine);
                            /* Draw a cross through the tile to indicate power flow */
                            MoveToEx(hdc, tileRect.left, tileRect.top, NULL);
                            LineTo(hdc, tileRect.right, tileRect.bottom);
                            MoveToEx(hdc, tileRect.right, tileRect.top, NULL);
                            LineTo(hdc, tileRect.left, tileRect.bottom);
                            SelectObject(hdc, hOldPen);
                        } else {
                            /* Other conductive tiles - highlight them */
                            Rectangle(hdc, tileRect.left + (TILE_SIZE / 2) - 1,
                                      tileRect.top + (TILE_SIZE / 2) - 1,
                                      tileRect.left + (TILE_SIZE / 2) + 2,
                                      tileRect.top + (TILE_SIZE / 2) + 2);
                        }
                    }
                }

                /* Mark power plants with a yellow circle - use cached pen */
                if ((Map[y][x] & LOMASK) == POWERPLANT || (Map[y][x] & LOMASK) == NUCLEAR) {
                    static HPEN hPenPowerPlant = NULL;
                    HPEN hOldPen;
                    HBRUSH hOldBrush;

                    /* Initialize cached pen on first use */
                    if (!hPenPowerPlant) {
                        hPenPowerPlant = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
                    }

                    hOldPen = SelectObject(hdc, hPenPowerPlant);
                    hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

                    /* Draw a circle around the power plant */
                    Ellipse(hdc, screenX + 2, screenY + 2, screenX + TILE_SIZE - 2,
                            screenY + TILE_SIZE - 2);

                    SelectObject(hdc, hOldPen);
                    SelectObject(hdc, hOldBrush);
                    /* No DeleteObject needed - pen is cached */
                }
            }
        }
    }

    /* Show legend for power overlay if enabled - use cached brushes */
    if (powerOverlayEnabled) {
        RECT legendRect;
        RECT legendBackground;
        COLORREF oldTextColor;
        int legendX;
        int legendY;
        int legendWidth;
        int legendHeight;
        
        /* Cached brushes and pens for legend to reduce GDI overhead */
        static HBRUSH hBackgroundBrush = NULL;
        static HBRUSH hLegendBrushPowered = NULL;
        static HBRUSH hLegendBrushUnpowered = NULL; 
        static HBRUSH hLegendBrushPowerLine = NULL;
        static HPEN hLegendPenPowerLine = NULL;

        /* Initialize cached GDI objects on first use */
        if (!hBackgroundBrush) {
            hBackgroundBrush = CreateSolidBrush(RGB(50, 50, 50));
            hLegendBrushPowered = CreateSolidBrush(RGB(0, 255, 0));
            hLegendBrushUnpowered = CreateSolidBrush(RGB(255, 0, 0));
            hLegendBrushPowerLine = CreateSolidBrush(RGB(0, 200, 0));
            hLegendPenPowerLine = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
        }
        
        legendX = (cxClient - toolbarWidth) - 300;
        legendY = 10;
        legendWidth = 150;
        legendHeight = 120;

        /* Create semi-transparent background for the legend */
        legendBackground.left = legendX - 5;
        legendBackground.top = legendY - 5;
        legendBackground.right = legendX + legendWidth;
        legendBackground.bottom = legendY + legendHeight;

        FillRect(hdc, &legendBackground, hBackgroundBrush);
        /* No DeleteObject needed - brush is cached */

        /* Set up text for legend */
        SetBkMode(hdc, TRANSPARENT);
        oldTextColor = SetTextColor(hdc, RGB(255, 255, 255));

        /* Header */
        TextOut(hdc, legendX, legendY, "Power Overlay Legend:", 21);
        legendY += 20;

        /* Powered zones */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        FrameRect(hdc, &legendRect, hLegendBrushPowered);
        /* Add the green power indicator in corner */
        Rectangle(hdc, legendX + 2, legendY + 2, legendX + 6, legendY + 6);
        /* No DeleteObject needed - brush is cached */
        TextOut(hdc, legendX + 20, legendY, "Powered Zones", 13);
        legendY += 20;

        /* Unpowered zones */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        FrameRect(hdc, &legendRect, hLegendBrushUnpowered);
        /* Add an X to match visualization */
        MoveToEx(hdc, legendX + 2, legendY + 2, NULL);
        LineTo(hdc, legendX + 6, legendY + 6);
        MoveToEx(hdc, legendX + 6, legendY + 2, NULL);
        LineTo(hdc, legendX + 2, legendY + 6);
        /* No DeleteObject needed - brush is cached */
        TextOut(hdc, legendX + 20, legendY, "Unpowered Zones", 15);
        legendY += 20;

        /* Power lines */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        FrameRect(hdc, &legendRect, hLegendBrushPowerLine);

        /* Draw a cross through the tile to indicate power flow - use cached pen */
        {
            HPEN hOldPen = SelectObject(hdc, hLegendPenPowerLine);
            MoveToEx(hdc, legendX, legendY, NULL);
            LineTo(hdc, legendX + 15, legendY + 15);
            MoveToEx(hdc, legendX + 15, legendY, NULL);
            LineTo(hdc, legendX, legendY + 15);
            SelectObject(hdc, hOldPen);
            /* No DeleteObject needed - pen is cached */
        }

        /* No DeleteObject needed - brush is cached */
        TextOut(hdc, legendX + 20, legendY, "Power Lines", 11);
        legendY += 20;

        /* Other powered grid connections - use cached brush */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        FrameRect(hdc, &legendRect, hLegendBrushPowerLine);
        /* Draw a dot in the center */
        Rectangle(hdc, legendX + 6, legendY + 6, legendX + 10, legendY + 10);
        /* No DeleteObject needed - brush is cached */
        TextOut(hdc, legendX + 20, legendY, "Power Grid", 10);
        legendY += 20;

        /* Power plants - use cached brushes and pens */
        legendRect.left = legendX;
        legendRect.top = legendY;
        legendRect.right = legendX + 15;
        legendRect.bottom = legendY + 15;
        /* Use cached black brush for frame */
        {
            static HBRUSH hLegendBrushBlack = NULL;
            if (!hLegendBrushBlack) {
                hLegendBrushBlack = CreateSolidBrush(RGB(0, 0, 0));
            }
            FrameRect(hdc, &legendRect, hLegendBrushBlack);
            /* No DeleteObject needed - brush is cached */
        }

        /* Draw a yellow circle for power plants - use cached pen */
        {
            static HPEN hLegendPenYellow = NULL;
            HPEN hOldPen;
            HBRUSH hOldBrush;

            if (!hLegendPenYellow) {
                hLegendPenYellow = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
            }
            
            hOldPen = SelectObject(hdc, hLegendPenYellow);
            hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

            Ellipse(hdc, legendX + 2, legendY + 2, legendX + 13, legendY + 13);

            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            /* No DeleteObject needed - pen is cached */
        }

        TextOut(hdc, legendX + 20, legendY, "Power Plants", 12);

        /* Restore text color */
        SetTextColor(hdc, oldTextColor);
    }

    /* Draw tool hover highlight if a tool is active */
    if (isToolActive) {
        /* Get mouse position */
        POINT mousePos;
        int mapX, mapY;

        GetCursorPos(&mousePos);
        ScreenToClient(hwndMain, &mousePos);

        /* Skip if mouse is outside client area or in toolbar */
        if (mousePos.x >= toolbarWidth && mousePos.y >= 0 && mousePos.x < cxClient &&
            mousePos.y < cyClient) {
            /* Convert to map coordinates */
            ScreenToMap(mousePos.x, mousePos.y, &mapX, &mapY, xOffset, yOffset);

            /* Draw the highlight box */
            DrawToolHover(hdc, mapX, mapY, GetCurrentTool(), xOffset, yOffset);
        }
    }

    /* Draw sprites (helicopters, planes, trains, ships, buses) */
    {
        int i;
        HBRUSH hSpriteBrush;
        HPEN hSpritePen;
        HPEN hOldPen;
        HBRUSH hOldBrush;
        
        for (i = 0; i < MAX_SPRITES; i++) {
            SimSprite *sprite = GetSprite(i);
            if (sprite != NULL) {
                int spriteScreenX = sprite->x - xOffset;
                int spriteScreenY = sprite->y - yOffset;
                
                /* Check if sprite is visible on screen */
                if (spriteScreenX >= -32 && spriteScreenX < cxClient &&
                    spriteScreenY >= -32 && spriteScreenY < cyClient) {
                    
                    /* Different colors for different sprite types */
                    switch (sprite->type) {
                        case SPRITE_HELICOPTER:
                            hSpriteBrush = CreateSolidBrush(RGB(0, 255, 0)); /* Green */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(0, 128, 0));
                            break;
                        case SPRITE_AIRPLANE:
                            hSpriteBrush = CreateSolidBrush(RGB(255, 255, 255)); /* White */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
                            break;
                        case SPRITE_TRAIN:
                            hSpriteBrush = CreateSolidBrush(RGB(128, 64, 0)); /* Brown */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(64, 32, 0));
                            break;
                        case SPRITE_SHIP:
                            hSpriteBrush = CreateSolidBrush(RGB(0, 128, 255)); /* Blue */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(0, 64, 128));
                            break;
                        case SPRITE_BUS:
                            hSpriteBrush = CreateSolidBrush(RGB(255, 255, 0)); /* Yellow */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(128, 128, 0));
                            break;
                        case SPRITE_EXPLOSION:
                            hSpriteBrush = CreateSolidBrush(RGB(255, 128, 0)); /* Orange */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
                            break;
                        case SPRITE_MONSTER:
                            hSpriteBrush = CreateSolidBrush(RGB(128, 255, 128)); /* Light green */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
                            break;
                        case SPRITE_TORNADO:
                            hSpriteBrush = CreateSolidBrush(RGB(64, 64, 64)); /* Dark gray */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
                            break;
                        default:
                            hSpriteBrush = CreateSolidBrush(RGB(255, 0, 255)); /* Magenta */
                            hSpritePen = CreatePen(PS_SOLID, 1, RGB(128, 0, 128));
                            break;
                    }
                    
                    hOldBrush = SelectObject(hdc, hSpriteBrush);
                    hOldPen = SelectObject(hdc, hSpritePen);
                    
                    /* Draw sprite bitmap with transparency */
                    if ((sprite->type >= SPRITE_TRAIN && sprite->type <= SPRITE_BUS) ||
                        sprite->type == SPRITE_MONSTER || sprite->type == SPRITE_TORNADO) {
                        HBITMAP hbmSprite;
                        int frameIndex;
                        
                        /* Get appropriate frame for the sprite */
                        frameIndex = sprite->frame - 1;
                        if (frameIndex < 0) frameIndex = 0;
                        
                        /* Adjust frame index based on sprite type */
                        switch (sprite->type) {
                            case SPRITE_HELICOPTER:
                                if (frameIndex > 7) frameIndex = frameIndex % 8;
                                break;
                            case SPRITE_AIRPLANE:
                                if (frameIndex > 10) frameIndex = frameIndex % 11;
                                break;
                            case SPRITE_SHIP:
                                if (frameIndex > 7) frameIndex = frameIndex % 8;
                                break;
                            case SPRITE_TRAIN:
                                if (frameIndex > 4) frameIndex = frameIndex % 5;
                                break;
                            case SPRITE_BUS:
                                if (frameIndex > 3) frameIndex = frameIndex % 4;
                                break;
                            case SPRITE_EXPLOSION:
                                if (frameIndex > 5) frameIndex = frameIndex % 6;
                                break;
                            case SPRITE_MONSTER:
                                if (frameIndex > 15) frameIndex = frameIndex % 16;
                                break;
                            case SPRITE_TORNADO:
                                if (frameIndex > 2) frameIndex = frameIndex % 3;
                                break;
                        }
                        
                        /* Get the sprite bitmap */
                        hbmSprite = hbmSprites[sprite->type][frameIndex];
                        
                        if (hbmSprite && hdcSprites) {
                            HBITMAP hOldBitmap;
                            HPALETTE hOldPalette = NULL;
                            
                            /* Select sprite bitmap into DC */
                            hOldBitmap = SelectObject(hdcSprites, hbmSprite);
                            
                            /* Ensure palette is selected in sprite DC */
                            if (hPalette) {
                                hOldPalette = SelectPalette(hdcSprites, hPalette, FALSE);
                                RealizePalette(hdcSprites);
                            }
                            
                            /* Draw sprite with transparency (magenta is transparent) */
                            DrawTransparentBitmap(hdc, spriteScreenX - sprite->width / 2, spriteScreenY - sprite->height / 2,
                                                sprite->width, sprite->height, hdcSprites, 0, 0, RGB(255, 0, 255));
                            
                            /* Restore palette and DC */
                            if (hOldPalette) {
                                SelectPalette(hdcSprites, hOldPalette, FALSE);
                            }
                            SelectObject(hdcSprites, hOldBitmap);
                        } else {
                            /* Fallback if sprite not loaded */
                            Rectangle(hdc, spriteScreenX - 8, spriteScreenY - 8,
                                     spriteScreenX + 8, spriteScreenY + 8);
                        }
                    } else {
                        /* Draw simple rectangle for sprites without bitmaps */
                        Rectangle(hdc, spriteScreenX - 8, spriteScreenY - 8,
                                 spriteScreenX + 8, spriteScreenY + 8);
                    }
                    
                    /* Cleanup GDI objects */
                    SelectObject(hdc, hOldBrush);
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hSpriteBrush);
                    DeleteObject(hSpritePen);
                }
            }
        }
    }

    /* Setup text drawing */
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
}

void openCityDialog(HWND hwnd) {
    OPENFILENAME ofn;
    char szFileName[MAX_PATH];

    szFileName[0] = '\0';

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "City Files (*.cty)\0*.cty\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = NULL; /* Use current directory instead of hardcoded cities path */
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = "cty";

    if (GetOpenFileName(&ofn)) {
        loadCity(szFileName);
        SetGameSpeed(gameSpeed ? gameSpeed : SPEED_MEDIUM);
    }
}

HMENU createMainMenu(void) {
    HMENU hMainMenu;
    HMENU hViewMenu;

    hMainMenu = CreateMenu();

    hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_NEW, "&New...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPEN, "&Open City...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE, "&Save City");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE_AS, "Save City &As...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, "E&xit");

    hTilesetMenu = CreatePopupMenu();
    populateTilesetMenu(hTilesetMenu);

    /* Speed menu moved to Settings menu */

    /* Create scenario menu */
    hScenarioMenu = CreatePopupMenu();
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_DULLSVILLE, "&Dullsville (1900): Boredom");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_SANFRANCISCO,
               "&San Francisco (1906): Earthquake");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_HAMBURG, "&Hamburg (1944): Bombing");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_BERN, "&Bern (1965): Traffic");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_TOKYO, "&Tokyo (1957): Monster Attack");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_DETROIT, "&Detroit (1972): Crime");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_BOSTON, "&Boston (2010): Nuclear Meltdown");
    AppendMenu(hScenarioMenu, MF_STRING, IDM_SCENARIO_RIO,
               "&Rio de Janeiro (2047): Coastal Flooding");

    /* Create tools menu */
    hToolMenu = CreatePopupMenu();

    /* Transportation Tools */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_BULLDOZER, "&Bulldozer ($1)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_ROAD, "&Road ($10)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_RAIL, "Rail&road ($20)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_WIRE, "&Wire ($5)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Zone Tools */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_RESIDENTIAL, "&Residential Zone ($100)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_COMMERCIAL, "&Commercial Zone ($100)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_INDUSTRIAL, "&Industrial Zone ($100)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_PARK, "Par&k ($10)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Public Services */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_POLICESTATION, "Police &Station ($500)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_FIRESTATION, "&Fire Station ($500)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Special Buildings */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_STADIUM, "S&tadium ($5000)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_SEAPORT, "Sea&port ($3000)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_AIRPORT, "&Airport ($10000)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Power */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_POWERPLANT, "&Coal Power Plant ($3000)");
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_NUCLEAR, "&Nuclear Power Plant ($5000)");
    AppendMenu(hToolMenu, MF_SEPARATOR, 0, NULL);

    /* Query */
    AppendMenu(hToolMenu, MF_STRING, IDM_TOOL_QUERY, "&Query");

    /* Default tool is bulldozer */
    CHECK_MENU_RADIO_ITEM(hToolMenu, IDM_TOOL_BULLDOZER, IDM_TOOL_QUERY, IDM_TOOL_BULLDOZER,
                       MF_BYCOMMAND);

    /* Create View menu */
    hViewMenu = CreatePopupMenu();
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_INFOWINDOW, "&Info Window");
    /* Check it by default since the info window is shown on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_INFOWINDOW, MF_CHECKED);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_LOGWINDOW, "&Message Log");
    /* Check it by default since the log window is shown on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_LOGWINDOW, MF_CHECKED);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_MINIMAPWINDOW, "&Minimap Window");
    /* Check it by default since the minimap window is shown on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_MINIMAPWINDOW, MF_CHECKED);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_BUDGET, "&Budget Window");
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_EVALUATION, "&Evaluation Window");
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_CHARTSWINDOW, "&Charts Window");
    /* Check it by default since the charts window is shown on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_CHARTSWINDOW, MF_CHECKED);
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_POWER_OVERLAY, "&Power Overlay");
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_TILESWINDOW, "Tile &Viewer");
    /* Leave unchecked by default since the tiles window is hidden on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_TILESWINDOW, MF_UNCHECKED);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_TILE_DEBUG, "Tile &Debug");
    /* Leave unchecked by default since tile debug is disabled on startup */
    CheckMenuItem(hViewMenu, IDM_VIEW_TILE_DEBUG, MF_UNCHECKED);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_TEST_SAVELOAD, "Test Save/&Load");
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_ABOUT, "&About");

    /* Spawn Menu */
    hSpawnMenu = CreatePopupMenu();
    AppendMenu(hSpawnMenu, MF_STRING, IDM_SPAWN_HELICOPTER, "&Helicopter");
    AppendMenu(hSpawnMenu, MF_STRING, IDM_SPAWN_AIRPLANE, "&Airplane");
    AppendMenu(hSpawnMenu, MF_STRING, IDM_SPAWN_TRAIN, "&Train");
    AppendMenu(hSpawnMenu, MF_STRING, IDM_SPAWN_SHIP, "&Ship");
    AppendMenu(hSpawnMenu, MF_STRING, IDM_SPAWN_BUS, "&Bus");
    
    /* Disaster Menu */
    hDisasterMenu = CreatePopupMenu();
    AppendMenu(hDisasterMenu, MF_STRING, IDM_DISASTER_FIRE, "&Fire");
    AppendMenu(hDisasterMenu, MF_STRING, IDM_DISASTER_FLOOD, "Fl&ood");
    AppendMenu(hDisasterMenu, MF_STRING, IDM_DISASTER_TORNADO, "&Tornado");
    AppendMenu(hDisasterMenu, MF_STRING, IDM_DISASTER_EARTHQUAKE, "&Earthquake");
    AppendMenu(hDisasterMenu, MF_STRING, IDM_DISASTER_MONSTER, "&Monster");
    AppendMenu(hDisasterMenu, MF_STRING, IDM_DISASTER_MELTDOWN, "Nuclear &Meltdown");
    
    /* Settings Menu */
    hSettingsMenu = CreatePopupMenu();
    
    /* Speed controls */
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SIM_PAUSE, "Speed: &Pause\t0");
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SIM_SLOW, "Speed: &Slow\t1");
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SIM_MEDIUM, "Speed: &Medium\t2");
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SIM_FAST, "Speed: &Fast\t3");
    AppendMenu(hSettingsMenu, MF_SEPARATOR, 0, NULL);
    
    /* Difficulty Level submenu */
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SETTINGS_LEVEL_EASY, "Difficulty: &Easy");
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SETTINGS_LEVEL_MEDIUM, "Difficulty: &Medium");
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SETTINGS_LEVEL_HARD, "Difficulty: &Hard");
    AppendMenu(hSettingsMenu, MF_SEPARATOR, 0, NULL);
    
    /* Auto Settings */
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SETTINGS_AUTO_BUDGET, "Auto &Budget");
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SETTINGS_AUTO_BULLDOZE, "Auto B&ulldoze");
    AppendMenu(hSettingsMenu, MF_STRING, IDM_SETTINGS_AUTO_GOTO, "Auto &Goto");
    AppendMenu(hSettingsMenu, MF_STRING, IDM_CHEATS_DISABLE_DISASTERS, "Enable &Disasters");
    
    /* Set default checkmarks */
    CheckMenuItem(hSettingsMenu, IDM_SIM_MEDIUM, MF_CHECKED); /* Default speed */
    CheckMenuItem(hSettingsMenu, IDM_SETTINGS_LEVEL_EASY, MF_CHECKED);
    CheckMenuItem(hSettingsMenu, IDM_SETTINGS_AUTO_BUDGET, AutoBudget ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSettingsMenu, IDM_SETTINGS_AUTO_BULLDOZE, autoBulldoze ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSettingsMenu, IDM_SETTINGS_AUTO_GOTO, AutoGo ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSettingsMenu, IDM_CHEATS_DISABLE_DISASTERS, !disastersDisabled ? MF_CHECKED : MF_UNCHECKED);

    AppendMenu(hMainMenu, MF_POPUP, (UINT)hFileMenu, "&File");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hScenarioMenu, "&Scenarios");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hTilesetMenu, "&Tileset");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hSpawnMenu, "S&pawn");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hDisasterMenu, "&Disasters");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hSettingsMenu, "Se&ttings");
    AppendMenu(hMainMenu, MF_POPUP, (UINT)hViewMenu, "&View");

    return hMainMenu;
}

/* Load sprite bitmaps from embedded resources */
void loadSpriteBitmaps(void) {
    int type, frame;
    int maxFrames[9] = {5, 8, 11, 8, 4, 16, 3, 6, 0}; /* Max frames per sprite type */
    char *prefix[9] = {"train", "helicopter", "airplane", "ship", "bus", "monster", "tornado", "explosion", NULL};
    
    /* Create sprite DC if not exists */
    if (!hdcSprites) {
        HDC hdcScreen = GetDC(NULL);
        hdcSprites = CreateCompatibleDC(hdcScreen);
        ReleaseDC(NULL, hdcScreen);
        
        /* Apply palette to sprite DC if we have one */
        if (hPalette) {
            SelectPalette(hdcSprites, hPalette, FALSE);
            RealizePalette(hdcSprites);
        }
    }
    
    /* Load each sprite type from embedded resources */
    for (type = 0; type < 8; type++) {
        for (frame = 0; frame < maxFrames[type]; frame++) {
            int resourceId;
            extern int findSpriteResourceByName(const char* spriteType, int frameNumber);
            extern HBITMAP loadTilesetFromResource(int resourceId);
            
            /* Find the resource ID for this sprite frame */
            resourceId = findSpriteResourceByName(prefix[type], frame);
            
            if (resourceId != 0) {
                /* Load the sprite from embedded resource */
                hbmSprites[type + 1][frame] = loadTilesetFromResource(resourceId);
                
                if (hbmSprites[type + 1][frame]) {
                    addDebugLog("Loaded sprite from resource: %s-%d (ID: %d)", prefix[type], frame, resourceId);
                } else {
                    addDebugLog("Failed to load sprite from resource: %s-%d (ID: %d)", prefix[type], frame, resourceId);
                }
            } else {
                addDebugLog("Resource not found for sprite: %s-%d", prefix[type], frame);
                hbmSprites[type + 1][frame] = NULL;
            }
        }
    }
}

/* Draw bitmap with transparency (magenta = transparent) */
void DrawTransparentBitmap(HDC hdcDest, int xDest, int yDest, int width, int height,
                          HDC hdcSrc, int xSrc, int ySrc, COLORREF transparentColor) {
    HDC hdcMask, hdcImage;
    HBITMAP hbmMask, hbmImage;
    HBITMAP hbmOldMask, hbmOldImage;
    
    /* Create mask DC */
    hdcMask = CreateCompatibleDC(hdcDest);
    hbmMask = CreateBitmap(width, height, 1, 1, NULL);
    hbmOldMask = SelectObject(hdcMask, hbmMask);
    
    /* Create image DC with full color support */
    hdcImage = CreateCompatibleDC(hdcDest);
    hbmImage = CreateCompatibleBitmap(hdcDest, width, height);
    hbmOldImage = SelectObject(hdcImage, hbmImage);
    
    /* Ensure palettes match */
    if (hPalette) {
        SelectPalette(hdcImage, hPalette, FALSE);
        RealizePalette(hdcImage);
    }
    
    /* Copy source image to our image DC */
    BitBlt(hdcImage, 0, 0, width, height, hdcSrc, xSrc, ySrc, SRCCOPY);
    
    /* Create mask: transparent color becomes white (1), rest black (0) */
    SetBkColor(hdcImage, transparentColor);
    BitBlt(hdcMask, 0, 0, width, height, hdcImage, 0, 0, SRCCOPY);
    
    /* Make transparent areas black in the source image */
    SetBkColor(hdcImage, RGB(0,0,0));
    SetTextColor(hdcImage, RGB(255,255,255));
    BitBlt(hdcImage, 0, 0, width, height, hdcMask, 0, 0, SRCAND);
    
    /* Make opaque areas white in the mask (invert it) */
    BitBlt(hdcMask, 0, 0, width, height, hdcMask, 0, 0, NOTSRCCOPY);
    
    /* Apply mask to destination (AND operation - preserves destination where mask is white) */
    BitBlt(hdcDest, xDest, yDest, width, height, hdcMask, 0, 0, SRCAND);
    
    /* Paint image over destination (OR operation - adds sprite to punched hole) */
    BitBlt(hdcDest, xDest, yDest, width, height, hdcImage, 0, 0, SRCPAINT);
    
    /* Cleanup */
    SelectObject(hdcMask, hbmOldMask);
    DeleteObject(hbmMask);
    DeleteDC(hdcMask);
    SelectObject(hdcImage, hbmOldImage);
    DeleteObject(hbmImage);
    DeleteDC(hdcImage);
}

void populateTilesetMenu(HMENU hSubMenu) {
    char tilesetNames[IDM_TILESET_MAX - IDM_TILESET_BASE][MAX_PATH];
    char fileName[MAX_PATH];
    char *dot;
    int menuId = IDM_TILESET_BASE;
    UINT menuFlags;
    int count = 0;
    int i, j;
    char temp[MAX_PATH];
    GameAssetInfo* tilesetInfo;
    
    /* Get embedded tilesets */
    for (i = 0; i < getTilesetCount() && count < (IDM_TILESET_MAX - IDM_TILESET_BASE); i++) {
        tilesetInfo = getTilesetInfo(i);
        if (tilesetInfo && tilesetInfo->filename) {
            strcpy(fileName, tilesetInfo->filename);
            dot = strrchr(fileName, '.');
            if (dot != NULL) {
                *dot = '\0';
            }
            strcpy(tilesetNames[count], fileName);
            count++;
        }
    }
    
    if (count > 0) {
        /* Sort the tileset names alphabetically using bubble sort */
        for (i = 0; i < count - 1; i++) {
            for (j = 0; j < count - i - 1; j++) {
                if (strcmp(tilesetNames[j], tilesetNames[j + 1]) > 0) {
                    strcpy(temp, tilesetNames[j]);
                    strcpy(tilesetNames[j], tilesetNames[j + 1]);
                    strcpy(tilesetNames[j + 1], temp);
                }
            }
        }

        /* Add sorted names to menu */
        for (i = 0; i < count; i++) {
            menuFlags = MF_STRING;
            if (strcmp(tilesetNames[i], currentTileset) == 0) {
                menuFlags |= MF_CHECKED;
            }

            AppendMenu(hSubMenu, menuFlags, menuId++, tilesetNames[i]);
        }
    } else {
        AppendMenu(hSubMenu, MF_GRAYED, 0, "No tilesets found");
    }
}

/**
 * Refreshes the tileset menu by clearing and repopulating it
 */
void refreshTilesetMenu(void) {
    int count;
    int i;
    
    if (hTilesetMenu == NULL) {
        return;
    }
    
    /* Remove all existing menu items */
    count = GetMenuItemCount(hTilesetMenu);
    for (i = count - 1; i >= 0; i--) {
        RemoveMenu(hTilesetMenu, i, MF_BYPOSITION);
    }
    
    /* Repopulate the menu with current tilesets */
    populateTilesetMenu(hTilesetMenu);
}

/**
 * Creates a new empty map
 */
void createNewMap(HWND hwnd) {
    int x, y;
    
    /* Clear city filename since this is a new map */
    cityFileName[0] = '\0';
    
    /* Set default tileset for new maps */
    changeTileset(hwnd, "default");
    
    /* Update window title (override the tileset title) */
    SetWindowText(hwnd, "WiNTown - New City");
    
    /* Fill map with dirt */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            setMapTile(x, y, TILE_DIRT, 0, TILE_SET_REPLACE, "createNewMap-init");
        }
    }
    
    /* Reset scenario values */
    ScenarioID = 0;
    DisasterEvent = 0;
    DisasterWait = 0;
    
    /* Use fixed seed for consistent appearance */
    RandomlySeedRand();
    
    /* Reset simulation */
    DoSimInit();
    
    /* Reset funds to starting amount */
    TotalFunds = 50000;
    
    /* Start with medium speed */
    SetSimulationSpeed(hwnd, SPEED_MEDIUM);
    
    RValve = 0;
    CValve = 0;
    IValve = 0;
    ValveFlag = 1;
    
    /* Add logging */
    addGameLog("Created new empty city");
    
    /* Force display update */
    InvalidateRect(hwnd, NULL, TRUE);
    
    /* Update info window */
    if (hwndInfo) {
        InvalidateRect(hwndInfo, NULL, TRUE);
    }
    
    /* Refresh tileset menu to show any new tilesets */
    refreshTilesetMenu();
}

/* Budget window dialog resource IDs */
#define IDD_BUDGET 1001
#define IDC_TAX_RATE_SLIDER 1002
#define IDC_TAX_RATE_LABEL 1003
#define IDC_ROAD_SLIDER 1004
#define IDC_ROAD_LABEL 1005
#define IDC_FIRE_SLIDER 1006
#define IDC_FIRE_LABEL 1007
#define IDC_POLICE_SLIDER 1008
#define IDC_POLICE_LABEL 1009
#define IDC_TAXES_COLLECTED 1010
#define IDC_CASH_FLOW 1011
#define IDC_PREVIOUS_FUNDS 1012
#define IDC_CURRENT_FUNDS 1013
#define IDC_AUTO_BUDGET 1014
#define IDC_RESET_BUDGET 1015
#define IDOK_BUDGET 1016
#define IDCANCEL_BUDGET 1017
#define IDC_ROAD_PERCENT 1018
#define IDC_FIRE_PERCENT 1019
#define IDC_POLICE_PERCENT 1020

/* Budget window state variables */
static int tempTaxRate = 7;
static float tempRoadPercent = 1.0f;
static float tempFirePercent = 1.0f;
static float tempPolicePercent = 1.0f;
static int tempAutoBudget = 1;

/* Budget dialog procedure */
BOOL CALLBACK BudgetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static int tempTaxRate;
    static float tempRoadPercent, tempFirePercent, tempPolicePercent;
    static int tempAutoBudget;
    static int savedRoadTotal, savedFirePop, savedPolicePop; /* Preserve counts during dialog session */
    HWND hSlider;
    int sliderPos;
    char buffer[64];
    QUAD roadRequirement, fireRequirement, policeRequirement;
    QUAD roadSpending, fireSpending, policeSpending;
    QUAD totalExpenses, cashFlow;
    
    switch (message) {
    case WM_INITDIALOG:
        /* Force a census to update infrastructure counts */
        ForceFullCensus();
        
        /* Save the counts for this dialog session */
        savedRoadTotal = RoadTotal;
        savedFirePop = FirePop;
        savedPolicePop = PolicePop;
        
        /* Initialize sliders and values */
        tempTaxRate = TaxRate;
        tempRoadPercent = RoadPercent;
        tempFirePercent = FirePercent;
        tempPolicePercent = PolicePercent;
        tempAutoBudget = AutoBudget;
        
        /* Tax Rate Slider (0-20%) */
        hSlider = GetDlgItem(hDlg, IDC_TAX_RATE_SLIDER);
        SetScrollRange(hSlider, SB_CTL, 0, 20, FALSE);
        SetScrollPos(hSlider, SB_CTL, tempTaxRate, TRUE);
        wsprintf(buffer, "Tax Rate: %d%%", tempTaxRate);
        SetDlgItemText(hDlg, IDC_TAX_RATE_LABEL, buffer);
        
        /* Road Funding Slider (0-100%) */
        hSlider = GetDlgItem(hDlg, IDC_ROAD_SLIDER);
        SetScrollRange(hSlider, SB_CTL, 0, 100, FALSE);
        SetScrollPos(hSlider, SB_CTL, (int)(tempRoadPercent * 100), TRUE);
        wsprintf(buffer, "Road Funding: %d%%", (int)(tempRoadPercent * 100));
        SetDlgItemText(hDlg, IDC_ROAD_LABEL, buffer);
        
        /* Fire Funding Slider (0-100%) */
        hSlider = GetDlgItem(hDlg, IDC_FIRE_SLIDER);
        SetScrollRange(hSlider, SB_CTL, 0, 100, FALSE);
        SetScrollPos(hSlider, SB_CTL, (int)(tempFirePercent * 100), TRUE);
        wsprintf(buffer, "Fire Funding: %d%%", (int)(tempFirePercent * 100));
        SetDlgItemText(hDlg, IDC_FIRE_LABEL, buffer);
        
        /* Police Funding Slider (0-100%) */
        hSlider = GetDlgItem(hDlg, IDC_POLICE_SLIDER);
        SetScrollRange(hSlider, SB_CTL, 0, 100, FALSE);
        SetScrollPos(hSlider, SB_CTL, (int)(tempPolicePercent * 100), TRUE);
        wsprintf(buffer, "Police Funding: %d%%", (int)(tempPolicePercent * 100));
        SetDlgItemText(hDlg, IDC_POLICE_LABEL, buffer);
        
        /* Auto-Budget checkbox */
        CheckDlgButton(hDlg, IDC_AUTO_BUDGET, tempAutoBudget ? BST_CHECKED : BST_UNCHECKED);
        
        /* Update financial display */
        SendMessage(hDlg, WM_USER + 1, 0, 0);
        return TRUE;
        
    case WM_USER + 1:
        /* Calculate current funding requirements using saved counts */
        roadRequirement = savedRoadTotal * 1;     /* $1 per road tile */
        fireRequirement = savedFirePop * 100;     /* $100 per fire station */
        policeRequirement = savedPolicePop * 100; /* $100 per police station */
        
        roadSpending = (QUAD)(roadRequirement * tempRoadPercent);
        fireSpending = (QUAD)(fireRequirement * tempFirePercent);
        policeSpending = (QUAD)(policeRequirement * tempPolicePercent);
        
        totalExpenses = roadSpending + fireSpending + policeSpending;
        cashFlow = TaxFund - totalExpenses;
        
        {
            char numBuf[32];
            FormatNumber((long)TaxFund, numBuf);
            wsprintf(buffer, "Taxes Collected: $%s", numBuf);
            SetDlgItemText(hDlg, IDC_TAXES_COLLECTED, buffer);

            wsprintf(buffer, "Cash Flow: %s$%s", cashFlow >= 0 ? "+" : "",
                     FormatNumber((long)(cashFlow < 0 ? -cashFlow : cashFlow), numBuf));
            SetDlgItemText(hDlg, IDC_CASH_FLOW, buffer);

            FormatNumber((long)TotalFunds, numBuf);
            wsprintf(buffer, "Previous Funds: $%s", numBuf);
            SetDlgItemText(hDlg, IDC_PREVIOUS_FUNDS, buffer);

            FormatNumber((long)(TotalFunds + cashFlow), numBuf);
            wsprintf(buffer, "Current Funds: $%s", numBuf);
            SetDlgItemText(hDlg, IDC_CURRENT_FUNDS, buffer);

            FormatNumber((long)roadSpending, numBuf);
            wsprintf(buffer, "$%s", numBuf);
            SetDlgItemText(hDlg, IDC_ROAD_PERCENT, buffer);

            FormatNumber((long)fireSpending, numBuf);
            wsprintf(buffer, "$%s", numBuf);
            SetDlgItemText(hDlg, IDC_FIRE_PERCENT, buffer);

            FormatNumber((long)policeSpending, numBuf);
            wsprintf(buffer, "$%s", numBuf);
            SetDlgItemText(hDlg, IDC_POLICE_PERCENT, buffer);
        }
        return TRUE;
        
    case WM_HSCROLL:
        /* Handle scrollbar changes */
        hSlider = (HWND)lParam;
        sliderPos = GetScrollPos(hSlider, SB_CTL);
        
        /* Handle thumb drag */
        if (LOWORD(wParam) == SB_THUMBTRACK || LOWORD(wParam) == SB_THUMBPOSITION) {
            sliderPos = HIWORD(wParam);
        } else if (LOWORD(wParam) == SB_LINELEFT) {
            sliderPos = max(0, sliderPos - 1);
        } else if (LOWORD(wParam) == SB_LINERIGHT) {
            int minPos, maxPos;
            GetScrollRange(hSlider, SB_CTL, &minPos, &maxPos);
            sliderPos = min(maxPos, sliderPos + 1);
        } else if (LOWORD(wParam) == SB_PAGELEFT) {
            sliderPos = max(0, sliderPos - 10);
        } else if (LOWORD(wParam) == SB_PAGERIGHT) {
            int minPos, maxPos;
            GetScrollRange(hSlider, SB_CTL, &minPos, &maxPos);
            sliderPos = min(maxPos, sliderPos + 10);
        }
        
        SetScrollPos(hSlider, SB_CTL, sliderPos, TRUE);
        
        if (hSlider == GetDlgItem(hDlg, IDC_TAX_RATE_SLIDER)) {
            tempTaxRate = sliderPos;
            wsprintf(buffer, "Tax Rate: %d%%", tempTaxRate);
            SetDlgItemText(hDlg, IDC_TAX_RATE_LABEL, buffer);
        } else if (hSlider == GetDlgItem(hDlg, IDC_ROAD_SLIDER)) {
            tempRoadPercent = sliderPos / 100.0f;
            wsprintf(buffer, "Road Funding: %d%%", sliderPos);
            SetDlgItemText(hDlg, IDC_ROAD_LABEL, buffer);
        } else if (hSlider == GetDlgItem(hDlg, IDC_FIRE_SLIDER)) {
            tempFirePercent = sliderPos / 100.0f;
            wsprintf(buffer, "Fire Funding: %d%%", sliderPos);
            SetDlgItemText(hDlg, IDC_FIRE_LABEL, buffer);
        } else if (hSlider == GetDlgItem(hDlg, IDC_POLICE_SLIDER)) {
            tempPolicePercent = sliderPos / 100.0f;
            wsprintf(buffer, "Police Funding: %d%%", sliderPos);
            SetDlgItemText(hDlg, IDC_POLICE_LABEL, buffer);
        }
        
        /* Update financial display */
        SendMessage(hDlg, WM_USER + 1, 0, 0);
        return TRUE;
        
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_AUTO_BUDGET:
            tempAutoBudget = IsDlgButtonChecked(hDlg, IDC_AUTO_BUDGET) == BST_CHECKED ? 1 : 0;
            return TRUE;
            
        case IDC_RESET_BUDGET:
            /* Reset to current values */
            tempTaxRate = TaxRate;
            tempRoadPercent = RoadPercent;
            tempFirePercent = FirePercent;
            tempPolicePercent = PolicePercent;
            tempAutoBudget = AutoBudget;
            
            /* Update controls */
            SetScrollPos(GetDlgItem(hDlg, IDC_TAX_RATE_SLIDER), SB_CTL, tempTaxRate, TRUE);
            SetScrollPos(GetDlgItem(hDlg, IDC_ROAD_SLIDER), SB_CTL, (int)(tempRoadPercent * 100), TRUE);
            SetScrollPos(GetDlgItem(hDlg, IDC_FIRE_SLIDER), SB_CTL, (int)(tempFirePercent * 100), TRUE);
            SetScrollPos(GetDlgItem(hDlg, IDC_POLICE_SLIDER), SB_CTL, (int)(tempPolicePercent * 100), TRUE);
            CheckDlgButton(hDlg, IDC_AUTO_BUDGET, tempAutoBudget ? BST_CHECKED : BST_UNCHECKED);
            
            /* Update labels and display */
            wsprintf(buffer, "Tax Rate: %d%%", tempTaxRate);
            SetDlgItemText(hDlg, IDC_TAX_RATE_LABEL, buffer);
            wsprintf(buffer, "Road Funding: %d%%", (int)(tempRoadPercent * 100));
            SetDlgItemText(hDlg, IDC_ROAD_LABEL, buffer);
            wsprintf(buffer, "Fire Funding: %d%%", (int)(tempFirePercent * 100));
            SetDlgItemText(hDlg, IDC_FIRE_LABEL, buffer);
            wsprintf(buffer, "Police Funding: %d%%", (int)(tempPolicePercent * 100));
            SetDlgItemText(hDlg, IDC_POLICE_LABEL, buffer);
            
            SendMessage(hDlg, WM_USER + 1, 0, 0);
            return TRUE;
            
        case IDOK_BUDGET:
            /* Apply changes */
            TaxRate = tempTaxRate;
            RoadPercent = tempRoadPercent;
            FirePercent = tempFirePercent;
            PolicePercent = tempPolicePercent;
            AutoBudget = tempAutoBudget;
            
            /* Recalculate budget */
            DoBudget();
            
            addGameLog("Budget updated: Tax %d%%, Road %d%%, Fire %d%%, Police %d%%",
                      TaxRate, (int)(RoadPercent * 100), (int)(FirePercent * 100), (int)(PolicePercent * 100));
            
            EndDialog(hDlg, IDOK);
            return TRUE;
            
        case IDCANCEL_BUDGET:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
        
    case WM_CLOSE:
        /* Handle X button - same as Cancel */
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

/* Show budget window function */
void ShowBudgetWindow(HWND parent) {
    /* Create budget dialog using Windows API */
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_BUDGET), parent, (DLGPROC)BudgetDlgProc);
}

/* Show budget window during budget cycle and wait for user input */
int ShowBudgetWindowAndWait(HWND parent) {
    return (int)DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_BUDGET), parent, (DLGPROC)BudgetDlgProc);
}

/* Evaluation dialog */
extern const char *GetProblemText(int problemIndex);
extern void GetTopProblems(short problems[4]);
extern int GetProblemVotes(int problemIndex);
extern QUAD GetCityAssessedValue(void);

BOOL CALLBACK EvalDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    char buffer[128];
    char numBuf[32];
    short problems[4];
    int i;

    static char *levelStr[3] = {"Easy", "Medium", "Hard"};

    switch (message) {
    case WM_INITDIALOG:
        wsprintf(buffer, "City Evaluation  %d", CityYear);
        SetDlgItemText(hDlg, IDC_EVAL_TITLE, buffer);

        wsprintf(buffer, "Yes: %d%%", CityYes);
        SetDlgItemText(hDlg, IDC_EVAL_YES, buffer);
        wsprintf(buffer, "No: %d%%", CityNo);
        SetDlgItemText(hDlg, IDC_EVAL_NO, buffer);

        GetTopProblems(problems);
        for (i = 0; i < 4; i++) {
            int votes = GetProblemVotes(problems[i]);
            if (votes > 0) {
                SetDlgItemText(hDlg, IDC_EVAL_PROB0 + i, GetProblemText(problems[i]));
                wsprintf(buffer, "%d%%", votes);
                SetDlgItemText(hDlg, IDC_EVAL_PROBPCT0 + i, buffer);
            }
        }

        FormatNumber((long)CityPop, numBuf);
        SetDlgItemText(hDlg, IDC_EVAL_POP, numBuf);

        {
            long delta = (long)(CityPop - PrevCityPop);
            wsprintf(buffer, "%s%s", delta >= 0 ? "+" : "",
                     FormatNumber(delta < 0 ? -delta : delta, numBuf));
            if (delta < 0) {
                wsprintf(buffer, "-%s", numBuf);
            }
        }
        SetDlgItemText(hDlg, IDC_EVAL_DELTA, buffer);

        {
            QUAD val = GetCityAssessedValue();
            FormatNumber((long)(val / 1000), numBuf);
            wsprintf(buffer, "$%s", numBuf);
        }
        SetDlgItemText(hDlg, IDC_EVAL_ASSESSED, buffer);

        SetDlgItemText(hDlg, IDC_EVAL_CATEGORY, GetCityClassName());

        if (GameLevel >= 0 && GameLevel <= 2)
            SetDlgItemText(hDlg, IDC_EVAL_GAMELEVEL, levelStr[GameLevel]);

        wsprintf(buffer, "%d", CityScore);
        SetDlgItemText(hDlg, IDC_EVAL_SCORE, buffer);

        wsprintf(buffer, "%s%d", deltaCityScore >= 0 ? "+" : "", deltaCityScore);
        SetDlgItemText(hDlg, IDC_EVAL_SCORECHANGE, buffer);

        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, IDOK);
        return TRUE;
    }
    return FALSE;
}

void ShowEvaluationWindow(HWND parent) {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EVALUATION), parent, (DLGPROC)EvalDlgProc);
}

/* Enhanced speed control functions */
void SetGameSpeed(int speed) {
    /* Speed delays in milliseconds based on original WiNTown timings */
    static int speedDelays[] = { 0, 500, 200, 100 }; /* pause, slow, medium, fast */
    
    if (speed < 0) speed = 0;
    if (speed > 3) speed = 3;
    
    gameSpeed = speed;
    simTimerDelay = speedDelays[speed];

    /* Update simulation speed */
    SetSimulationSpeed(hwndMain, speed);

    /* Ensure sim timer is running for non-paused speeds */
    if (speed != SPEED_PAUSED && hwndMain) {
        SetTimer(hwndMain, SIM_TIMER_ID, SIM_TIMER_INTERVAL, NULL);
    }

    /* Update menu checkmarks */
    CheckMenuItem(hSettingsMenu, IDM_SIM_PAUSE, speed == SPEED_PAUSED ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSettingsMenu, IDM_SIM_SLOW, speed == SPEED_SLOW ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSettingsMenu, IDM_SIM_MEDIUM, speed == SPEED_MEDIUM ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSettingsMenu, IDM_SIM_FAST, speed == SPEED_FAST ? MF_CHECKED : MF_UNCHECKED);
}

void SetGameLevel(int level) {
    if (level < 0) level = 0;
    if (level > 2) level = 2;
    
    gameLevel = level;
    GameLevel = level; /* Update simulation variable */
    
    /* Log difficulty change - don't change funds for existing cities */
    switch (level) {
        case LEVEL_EASY:
            addGameLog("Difficulty set to Easy");
            break;
        case LEVEL_MEDIUM:
            addGameLog("Difficulty set to Medium");
            break;
        case LEVEL_HARD:
            addGameLog("Difficulty set to Hard");
            break;
    }
    
    addGameLog("Note: Starting funds only apply to new cities");
    
    /* Update menu checkmarks */
    CheckMenuItem(hSettingsMenu, IDM_SETTINGS_LEVEL_EASY, level == LEVEL_EASY ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSettingsMenu, IDM_SETTINGS_LEVEL_MEDIUM, level == LEVEL_MEDIUM ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hSettingsMenu, IDM_SETTINGS_LEVEL_HARD, level == LEVEL_HARD ? MF_CHECKED : MF_UNCHECKED);
}

void PauseSimulation() {
    if (gameSpeed != SPEED_PAUSED) {
        SetGameSpeed(SPEED_PAUSED);
        addGameLog("Simulation paused");
    }
}

void ResumeSimulation() {
    if (gameSpeed == SPEED_PAUSED) {
        SetGameSpeed(SPEED_MEDIUM);
        addGameLog("Simulation resumed");
    }
}

void TogglePause() {
    if (gameSpeed == SPEED_PAUSED) {
        ResumeSimulation();
    } else {
        PauseSimulation();
    }
}

/* Earthquake screen shake effect functions */
void startEarthquake(void) {
    shakeNow = 1; /* Start shake effect */
    
    /* Kill any existing earthquake timer */
    if (earthquakeTimer != 0) {
        KillTimer(hwndMain, EARTHQUAKE_TIMER_ID);
        earthquakeTimer = 0;
    }
    
    /* Start timer to stop earthquake after duration */
    earthquakeTimer = (unsigned int)SetTimer(hwndMain, EARTHQUAKE_TIMER_ID, EARTHQUAKE_DURATION, NULL);
    
    /* Force screen redraw to show shake immediately */
    InvalidateRect(hwndMain, NULL, FALSE);
    
    addGameLog("Earthquake screen shake started");
}

void stopEarthquake(void) {
    shakeNow = 0; /* Stop shake effect */
    
    /* Kill earthquake timer */
    if (earthquakeTimer != 0) {
        KillTimer(hwndMain, EARTHQUAKE_TIMER_ID);
        earthquakeTimer = 0;
    }
    
    /* Force screen redraw to stop shake */
    InvalidateRect(hwndMain, NULL, FALSE);
    
    addGameLog("Earthquake screen shake stopped");
}

