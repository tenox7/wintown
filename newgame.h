/* newgame.h - New Game Dialog Interface for WiNTown
 * Based on original WiNTown scenarios and new game functionality
 */

#ifndef NEWGAME_H
#define NEWGAME_H

#include <windows.h>

/* Dialog resource IDs */
#define IDD_NEWGAME 100
#define IDD_SCENARIO_SELECT 101

/* Control IDs for New Game dialog */
#define IDC_NEW_CITY 1001
#define IDC_LOAD_CITY 1002
#define IDC_SCENARIO 1003
#define IDC_DIFFICULTY_EASY 1004
#define IDC_DIFFICULTY_MEDIUM 1005
#define IDC_DIFFICULTY_HARD 1006
#define IDC_CITY_NAME 1007
#define IDC_OK 1008
#define IDC_CANCEL 1009

/* Control IDs for Scenario Selection dialog */
#define IDC_SCENARIO_LIST 2001
#define IDC_SCENARIO_PREVIEW 2002
#define IDC_SCENARIO_DESC 2003
#define IDC_SCENARIO_OK 2004
#define IDC_SCENARIO_CANCEL 2005

/* Control IDs for Map Generation */
#define IDC_MAP_ISLAND 10001
#define IDC_WATER_PERCENT 10003
#define IDC_WATER_LABEL 10004
#define IDC_FOREST_PERCENT 10005
#define IDC_FOREST_LABEL 10006
#define IDC_MAP_PREVIEW 10007
#define IDC_GENERATE_PREVIEW 10008

/* New Game options */
#define NEWGAME_NEW_CITY 0
#define NEWGAME_LOAD_CITY_BUILTIN 1
#define NEWGAME_LOAD_CITY_FILE 2
#define NEWGAME_SCENARIO 3

/* Difficulty levels */
#define DIFFICULTY_EASY 0
#define DIFFICULTY_MEDIUM 1
#define DIFFICULTY_HARD 2

/* Scenario definitions based on original WiNTown */
typedef struct {
    int id;
    char name[64];
    char description[512];
    char filename[32];
    int year;
    int funds;
} ScenarioInfo;

/* New game configuration */
typedef struct {
    int gameType;        /* NEWGAME_NEW_CITY, NEWGAME_LOAD_CITY, or NEWGAME_SCENARIO */
    int difficulty;      /* DIFFICULTY_EASY, DIFFICULTY_MEDIUM, or DIFFICULTY_HARD */
    int scenarioId;      /* 1-8 for scenarios, 0 for new city */
    char cityName[64];   /* City name */
    char loadFile[MAX_PATH]; /* File to load (for NEWGAME_LOAD_CITY) */
    int mapType;         /* Map generation type (rivers/island) */
    int waterPercent;    /* Water coverage percentage (0-100) */
    int forestPercent;   /* Forest coverage percentage (0-100) */
    int loadFromInternal; /* 1 for internal resources, 0 for external files */
} NewGameConfig;

/* Function prototypes */
int showNewGameDialog(HWND parent, NewGameConfig *config);
int showScenarioDialog(HWND parent, int *selectedScenario);
BOOL CALLBACK newGameDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK scenarioDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Initialize new game system */
int initNewGame(NewGameConfig *config);

/* Scenario management */
int getScenarioCount(void);
ScenarioInfo* getScenarioInfo(int index);
int loadScenarioById(int scenarioId);

/* New city generation */
int generateNewCity(char *cityName, int difficulty);
int generateNewCityWithTerrain(char *cityName, int difficulty, int mapType, int waterPercent, int forestPercent);

/* City loading */
int loadCityFile(char *filename);

#endif /* NEWGAME_H */
