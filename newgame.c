/* newgame.c - New Game Dialog Implementation for WiNTown
 * Based on original WiNTown scenarios and new game functionality
 */

#include "resource.h"
#include "newgame.h"
#include "mapgen.h"
#include "sim.h"
#include "tiles.h"
#include "assets.h"
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);
extern int loadFile(char *filename);
extern int loadScenario(int scenarioId);

/* External simulation functions */
extern void DoSimInit(void);
extern void SetValves(void);

/* External variables */
extern long TotalFunds;
extern int ResPop;
extern int ComPop;
extern int IndPop;
extern int TotalPop;
extern QUAD CityPop;
extern int CityClass;
extern int LastTotalPop;
extern int GameLevel;

/* Scenario data based on original WiNTown */
static ScenarioInfo scenarios[] = {
    {1, "DULLSVILLE", "DULLSVILLE, USA 1900\n\nThings haven't changed much around here in the last hundred years or so and the residents are beginning to get bored. They think Dullsville could be the next great city with the right leader.\n\nIt is your job to attract new growth and development, turning Dullsville into a Metropolis within 30 years.", "dullsville.scn", 1900, 5000},
    {2, "EARTHQUAKE", "SAN FRANCISCO, CA. 1906\n\nDamage from the earthquake was minor compared to that of the ensuing fires, which took days to control. 1500 people died.\n\nControlling the fires should be your initial concern. Then clear the rubble and start rebuilding. You have 5 years.", "sanfrancisco.scn", 1906, 20000},
    {3, "FIRE", "HAMBURG, GERMANY 1944\n\nAllied fire-bombing of German cities in WWII caused tremendous damage and loss of life. People living in the inner cities were at greatest risk.\n\nYou must control the firestorms during the bombing and then rebuild the city after the war. You have 5 years.", "hamburg.scn", 1944, 20000},
    {4, "TRAFFIC", "BERN, SWITZERLAND 1965\n\nThe roads here are becoming more congested every day, and the residents are upset. They demand that you do something about it.\n\nSome have suggested a mass transit system as the answer, but this would require major rezoning in the downtown area. You have 10 years.", "bern.scn", 1965, 20000},
    {5, "MONSTER", "TOKYO, JAPAN 1957\n\nA large reptilian creature has been spotted heading for Tokyo bay. It seems to be attracted to the heavy levels of industrial pollution there.\n\nTry to control the fires, then rebuild the industrial center. You have 5 years.", "tokyo.scn", 1957, 20000},
    {6, "CRIME", "DETROIT, MI. 1972\n\nBy 1970, competition from overseas and other economic factors pushed the once \"automobile capital of the world\" into recession. Plummeting land values and unemployment then increased crime in the inner-city to chronic levels.\n\nYou have 10 years to reduce crime and rebuild the industrial base of the city.", "detroit.scn", 1972, 20000},
    {7, "MELTDOWN", "BOSTON, MA. 2010\n\nA major meltdown is about to occur at one of the new downtown nuclear reactors. The area in the vicinity of the reactor will be severly contaminated by radiation, forcing you to restructure the city around it.\n\nYou have 5 years to get the situation under control.", "boston.scn", 2010, 20000},
    {8, "FLOOD", "RIO DE JANEIRO, BRAZIL 2047\n\nIn the mid-21st century, the greenhouse effect raised global temperatures 6 degrees F. Polar icecaps melted and raised sea levels worldwide. Coastal areas were devastated by flood and erosion.\n\nYou have 10 years to turn this swamp back into a city again.", "rio.scn", 2047, 20000}
};

static int scenarioCount = 8;
static NewGameConfig *currentConfig = NULL;
static HBITMAP currentPreviewBitmap = NULL;
static int currentListMode = NEWGAME_SCENARIO; /* Track what's in the listbox */

/* Load selected scenario/city map and render it into the preview control */
static void updateSelectionPreview(HWND hwnd) {
    static int scenarioResIds[8] = {
        IDR_SCENARIO_DULLSVILLE,
        IDR_SCENARIO_SANFRANCISCO,
        IDR_SCENARIO_HAMBURG,
        IDR_SCENARIO_BERN,
        IDR_SCENARIO_TOKYO,
        IDR_SCENARIO_DETROIT,
        IDR_SCENARIO_BOSTON,
        IDR_SCENARIO_RIO
    };
    HWND listBox, previewCtrl;
    RECT previewRect;
    int previewWidth, previewHeight;
    int sel, resId;
    GameAssetInfo *cityInfo;

    listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
    sel = (int)SendMessage(listBox, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) return;

    previewCtrl = GetDlgItem(hwnd, IDC_MAP_PREVIEW);
    GetClientRect(previewCtrl, &previewRect);
    previewWidth = previewRect.right - previewRect.left;
    previewHeight = previewRect.bottom - previewRect.top;

    if (currentListMode == NEWGAME_SCENARIO) {
        if (sel < 0 || sel >= scenarioCount) return;
        resId = scenarioResIds[sel];
        loadScenarioFromResource(resId, scenarios[sel].name);
    } else if (currentListMode == NEWGAME_LOAD_CITY_BUILTIN) {
        if (sel < 0 || sel >= getCityCount()) return;
        cityInfo = getCityInfo(sel);
        if (!cityInfo) return;
        loadCityFromResource(cityInfo->resourceId, cityInfo->description);
    } else {
        return;
    }

    SetGameSpeed(SPEED_PAUSED);

    if (currentPreviewBitmap) {
        DeleteObject(currentPreviewBitmap);
        currentPreviewBitmap = NULL;
    }

    if (renderMapPreview(&currentPreviewBitmap, previewWidth, previewHeight)) {
        SendMessage(previewCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)currentPreviewBitmap);
        addGameLog("Selection preview rendered (%dx%d)", previewWidth, previewHeight);
    }
}

/* Helper function to generate preview map */
static void generateDefaultPreview(HWND hwnd) {
    MapGenParams params;
    HWND previewCtrl;
    RECT previewRect;
    int previewWidth, previewHeight;
    int waterPos, forestPos;
    int isIslandChecked;
    
    /* Get current slider values */
    waterPos = GetScrollPos(GetDlgItem(hwnd, IDC_WATER_PERCENT), SB_CTL);
    forestPos = GetScrollPos(GetDlgItem(hwnd, IDC_FOREST_PERCENT), SB_CTL);
    
    /* Get preview control dimensions */
    previewCtrl = GetDlgItem(hwnd, IDC_MAP_PREVIEW);
    GetClientRect(previewCtrl, &previewRect);
    previewWidth = previewRect.right - previewRect.left;
    previewHeight = previewRect.bottom - previewRect.top;
    
    /* Set up parameters - read checkbox state directly */
    isIslandChecked = (IsDlgButtonChecked(hwnd, IDC_MAP_ISLAND) == BST_CHECKED);
    params.mapType = isIslandChecked ? MAPTYPE_ISLAND : MAPTYPE_RIVERS;
    params.waterPercent = waterPos;
    params.forestPercent = forestPos;
    
    /* Clean up previous bitmap */
    if (currentPreviewBitmap) {
        DeleteObject(currentPreviewBitmap);
        currentPreviewBitmap = NULL;
    }
    
    /* Generate new preview with dynamic sizing */
    if (generateMapPreview(&params, &currentPreviewBitmap, previewWidth, previewHeight)) {
        SendMessage(previewCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)currentPreviewBitmap);
        addGameLog("Default map preview generated successfully (%dx%d)", previewWidth, previewHeight);
    } else {
        addGameLog("ERROR: Failed to generate default map preview");
    }
}

/* Show the main new game dialog */
int showNewGameDialog(HWND parent, NewGameConfig *config) {
    int result;
    HINSTANCE hInst;
    
    if (!config) {
        addGameLog("ERROR: showNewGameDialog called with NULL config");
        return 0;
    }
    
    /* Initialize default values */
    config->gameType = NEWGAME_NEW_CITY;
    config->difficulty = DIFFICULTY_MEDIUM;
    config->scenarioId = 0;
    strcpy(config->cityName, "New City");
    config->loadFile[0] = '\0';
    config->mapType = MAPTYPE_RIVERS; /* Default: unchecked = rivers */
    config->waterPercent = 25;
    config->forestPercent = 30;
    config->loadFromInternal = 1;
    
    currentConfig = config;
    
    hInst = GetModuleHandle(NULL);
    addGameLog("About to call DialogBox with resource ID %d, hInst=0x%p", IDD_NEWGAME, hInst);
    
    result = (int)DialogBox(hInst, MAKEINTRESOURCE(IDD_NEWGAME), parent, (DLGPROC)newGameDialogProc);
    
    addGameLog("DialogBox returned: %d", result);
    if (result == -1) {
        DWORD error = GetLastError();
        addGameLog("ERROR: DialogBox failed with error %lu", error);
        currentConfig = NULL;
        return 0;
    }
    
    currentConfig = NULL;
    return result;
}

/* Show scenario selection dialog */
int showScenarioDialog(HWND parent, int *selectedScenario) {
    int result;
    
    if (!selectedScenario) {
        return 0;
    }
    
    result = (int)DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SCENARIO_SELECT), 
                           parent, (DLGPROC)scenarioDialogProc, (LPARAM)selectedScenario);
    
    return result;
}

/* Main new game dialog procedure */
BOOL CALLBACK newGameDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        HWND listBox;
        int i;
        char labelText[64];
        
        addGameLog("New game dialog WM_INITDIALOG - dialog is being created");
        /* Set default selections */
        CheckRadioButton(hwnd, IDC_NEW_CITY, IDC_SCENARIO, IDC_NEW_CITY);
        CheckRadioButton(hwnd, IDC_DIFFICULTY_EASY, IDC_DIFFICULTY_HARD, IDC_DIFFICULTY_MEDIUM);
        SetDlgItemText(hwnd, IDC_CITY_NAME, "New City");
        
        /* Initialize map generation controls */
        CheckDlgButton(hwnd, IDC_MAP_ISLAND, BST_UNCHECKED);
        SetScrollRange(GetDlgItem(hwnd, IDC_WATER_PERCENT), SB_CTL, 0, 100, FALSE);
        SetScrollPos(GetDlgItem(hwnd, IDC_WATER_PERCENT), SB_CTL, 25, TRUE);
        SetScrollRange(GetDlgItem(hwnd, IDC_FOREST_PERCENT), SB_CTL, 0, 100, FALSE);
        SetScrollPos(GetDlgItem(hwnd, IDC_FOREST_PERCENT), SB_CTL, 30, TRUE);
        
        /* Set initial labels */
        sprintf(labelText, "Water: %d%%", 25);
        SetDlgItemText(hwnd, IDC_WATER_LABEL, labelText);
        sprintf(labelText, "Forest: %d%%", 30);
        SetDlgItemText(hwnd, IDC_FOREST_LABEL, labelText);
        
        /* Populate scenario list - start with internal scenarios by default */
        listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
        /* CRITICAL: Clear any existing items first to prevent corruption */
        SendMessage(listBox, LB_RESETCONTENT, 0, 0);
        addGameLog("DEBUG: Cleared and populating NEW GAME scenario listbox with %d scenarios", scenarioCount);
        for (i = 0; i < scenarioCount; i++) {
            int addResult = (int)SendMessage(listBox, LB_ADDSTRING, 0, (LPARAM)scenarios[i].name);
            int dataResult = (int)SendMessage(listBox, LB_SETITEMDATA, i, i);
            addGameLog("DEBUG: Adding scenario %d: ID=%d, Name='%s' (add=%d, data=%d)", 
                      i, scenarios[i].id, scenarios[i].name, addResult, dataResult);
        }
        
        /* Select first scenario by default */
        SendMessage(listBox, LB_SETCURSEL, 0, 0);
        SetDlgItemText(hwnd, IDC_SCENARIO_DESC, scenarios[0].description);
        currentListMode = NEWGAME_SCENARIO;
        
        /* Show appropriate controls for New City (default selection) */
        EnableWindow(GetDlgItem(hwnd, IDC_CITY_NAME), TRUE);
        ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_LIST), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_DESC), SW_HIDE);
        
        /* Enable/disable controls based on selection */
        EnableWindow(GetDlgItem(hwnd, IDC_CITY_NAME), TRUE);
        
        /* Generate default preview for "New City" selection */
        generateDefaultPreview(hwnd);
        
        addGameLog("New game dialog initialized with defaults");
        return TRUE;
    }
    
    case WM_COMMAND: {
        HWND listBox;
        int i, sel, itemData;
        GameAssetInfo* cityInfo;
        OPENFILENAME ofn;
        char fileName[MAX_PATH];
        MapGenParams params;
        HWND previewCtrl;
        RECT previewRect;
        int previewWidth, previewHeight;
        int waterPos, forestPos;
        int isIslandChecked;
        
        switch (LOWORD(wParam)) {
        case IDC_NEW_CITY:
            if (HIWORD(wParam) == BN_CLICKED) {
                /* Show new city controls */
                EnableWindow(GetDlgItem(hwnd, IDC_CITY_NAME), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_EASY), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_MEDIUM), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_HARD), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_MAP_ISLAND), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_WATER_PERCENT), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_FOREST_PERCENT), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_GENERATE_PREVIEW), TRUE);
                /* Hide scenario controls */
                ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_LIST), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_DESC), SW_HIDE);
                if (currentConfig) {
                    currentConfig->gameType = NEWGAME_NEW_CITY;
                }
            }
            break;
            
        case IDC_LOAD_CITY_BUILTIN:
            if (HIWORD(wParam) == BN_CLICKED) {
                /* Disable new city controls */
                EnableWindow(GetDlgItem(hwnd, IDC_CITY_NAME), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_EASY), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_MEDIUM), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_HARD), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_MAP_ISLAND), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_WATER_PERCENT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_FOREST_PERCENT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_GENERATE_PREVIEW), FALSE);
                /* Show scenario list for builtin cities */
                ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_LIST), SW_SHOW);
                ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_DESC), SW_SHOW);
                
                /* Populate with builtin cities */
                listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
                SendMessage(listBox, LB_RESETCONTENT, 0, 0);
                for (i = 0; i < getCityCount(); i++) {
                    GameAssetInfo* cityInfo = getCityInfo(i);
                    if (cityInfo) {
                        SendMessage(listBox, LB_ADDSTRING, 0, (LPARAM)cityInfo->description);
                        SendMessage(listBox, LB_SETITEMDATA, i, i);
                    }
                }
                if (SendMessage(listBox, LB_GETCOUNT, 0, 0) > 0) {
                    SendMessage(listBox, LB_SETCURSEL, 0, 0);
                    cityInfo = getCityInfo(0);
                    if (cityInfo) {
                        SetDlgItemText(hwnd, IDC_SCENARIO_DESC, cityInfo->description);
                    }
                }
                currentListMode = NEWGAME_LOAD_CITY_BUILTIN;
                updateSelectionPreview(hwnd);
                
                if (currentConfig) {
                    currentConfig->gameType = NEWGAME_LOAD_CITY_BUILTIN;
                }
            }
            break;
            
        case IDC_LOAD_CITY_FILE:
            if (HIWORD(wParam) == BN_CLICKED) {
                /* Disable new city controls */
                EnableWindow(GetDlgItem(hwnd, IDC_CITY_NAME), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_EASY), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_MEDIUM), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_HARD), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_MAP_ISLAND), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_WATER_PERCENT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_FOREST_PERCENT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_GENERATE_PREVIEW), FALSE);
                /* Hide scenario controls */
                ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_LIST), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_DESC), SW_HIDE);
                if (currentConfig) {
                    currentConfig->gameType = NEWGAME_LOAD_CITY_FILE;
                }
            }
            break;
            
        case IDC_SCENARIO:
            if (HIWORD(wParam) == BN_CLICKED) {
                /* Disable new city controls */
                EnableWindow(GetDlgItem(hwnd, IDC_CITY_NAME), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_EASY), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_MEDIUM), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_DIFFICULTY_HARD), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_MAP_ISLAND), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_WATER_PERCENT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_FOREST_PERCENT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_GENERATE_PREVIEW), FALSE);
                /* Show scenario controls */
                ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_LIST), SW_SHOW);
                ShowWindow(GetDlgItem(hwnd, IDC_SCENARIO_DESC), SW_SHOW);
                
                /* Repopulate with scenarios */
                listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
                SendMessage(listBox, LB_RESETCONTENT, 0, 0);
                for (i = 0; i < scenarioCount; i++) {
                    SendMessage(listBox, LB_ADDSTRING, 0, (LPARAM)scenarios[i].name);
                    SendMessage(listBox, LB_SETITEMDATA, i, i);
                }
                SendMessage(listBox, LB_SETCURSEL, 0, 0);
                SetDlgItemText(hwnd, IDC_SCENARIO_DESC, scenarios[0].description);
                currentListMode = NEWGAME_SCENARIO;
                updateSelectionPreview(hwnd);

                if (currentConfig) {
                    currentConfig->gameType = NEWGAME_SCENARIO;
                }
            }
            break;
            
        case IDC_DIFFICULTY_EASY:
            if (HIWORD(wParam) == BN_CLICKED && currentConfig) {
                currentConfig->difficulty = DIFFICULTY_EASY;
            }
            break;
            
        case IDC_DIFFICULTY_MEDIUM:
            if (HIWORD(wParam) == BN_CLICKED && currentConfig) {
                currentConfig->difficulty = DIFFICULTY_MEDIUM;
            }
            break;
            
        case IDC_DIFFICULTY_HARD:
            if (HIWORD(wParam) == BN_CLICKED && currentConfig) {
                currentConfig->difficulty = DIFFICULTY_HARD;
            }
            break;
            
        case IDC_MAP_ISLAND:
            if (HIWORD(wParam) == BN_CLICKED && currentConfig) {
                int isChecked = (IsDlgButtonChecked(hwnd, IDC_MAP_ISLAND) == BST_CHECKED);
                currentConfig->mapType = isChecked ? MAPTYPE_ISLAND : MAPTYPE_RIVERS;
            }
            break;
            
            
        case IDC_GENERATE_PREVIEW:
            if (HIWORD(wParam) == BN_CLICKED && currentConfig) {
                /* Get current slider values */
                waterPos = GetScrollPos(GetDlgItem(hwnd, IDC_WATER_PERCENT), SB_CTL);
                forestPos = GetScrollPos(GetDlgItem(hwnd, IDC_FOREST_PERCENT), SB_CTL);
                
                /* Get preview control dimensions */
                previewCtrl = GetDlgItem(hwnd, IDC_MAP_PREVIEW);
                GetClientRect(previewCtrl, &previewRect);
                previewWidth = previewRect.right - previewRect.left;
                previewHeight = previewRect.bottom - previewRect.top;
                
                /* Set up parameters - read checkbox state directly */
                isIslandChecked = (IsDlgButtonChecked(hwnd, IDC_MAP_ISLAND) == BST_CHECKED);
                params.mapType = isIslandChecked ? MAPTYPE_ISLAND : MAPTYPE_RIVERS;
                params.waterPercent = waterPos;
                params.forestPercent = forestPos;
                
                
                /* Clean up previous bitmap */
                if (currentPreviewBitmap) {
                    DeleteObject(currentPreviewBitmap);
                    currentPreviewBitmap = NULL;
                }
                
                /* Generate new preview with dynamic sizing */
                if (generateMapPreview(&params, &currentPreviewBitmap, previewWidth, previewHeight)) {
                    SendMessage(previewCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)currentPreviewBitmap);
                    addGameLog("Map preview generated successfully (%dx%d)", previewWidth, previewHeight);
                } else {
                    addGameLog("ERROR: Failed to generate map preview");
                }
            }
            break;
            
        case IDC_SCENARIO_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
                sel = (int)SendMessage(listBox, LB_GETCURSEL, 0, 0);
                itemData = (int)SendMessage(listBox, LB_GETITEMDATA, sel, 0);
                
                addGameLog("DEBUG: Listbox selection changed - sel=%d, itemData=%d, mode=%d", sel, itemData, currentListMode);
                
                if (currentListMode == NEWGAME_SCENARIO) {
                    if (sel != LB_ERR && sel >= 0 && sel < scenarioCount) {
                        SetDlgItemText(hwnd, IDC_SCENARIO_DESC, scenarios[sel].description);
                        updateSelectionPreview(hwnd);
                    }
                } else if (currentListMode == NEWGAME_LOAD_CITY_BUILTIN) {
                    int cityCount = getCityCount();
                    if (sel != LB_ERR && sel >= 0 && sel < cityCount) {
                        GameAssetInfo* cityInfo = getCityInfo(sel);
                        if (cityInfo) {
                            SetDlgItemText(hwnd, IDC_SCENARIO_DESC, cityInfo->description);
                            updateSelectionPreview(hwnd);
                        }
                    }
                }
            }
            break;
            
        case IDC_OK: {
            addGameLog("User clicked OK in new game dialog");
            if (!currentConfig) {
                EndDialog(hwnd, 0);
                return TRUE;
            }
            
            /* Get city name */
            GetDlgItemText(hwnd, IDC_CITY_NAME, currentConfig->cityName, 63);
            addGameLog("City name entered: '%s'", currentConfig->cityName);
            
            /* Handle different game types */
            switch (currentConfig->gameType) {
            case NEWGAME_NEW_CITY:
                if (strlen(currentConfig->cityName) == 0) {
                    MessageBox(hwnd, "Please enter a city name.", "Error", MB_OK | MB_ICONERROR);
                    return TRUE;
                }
                break;
                
            case NEWGAME_LOAD_CITY_BUILTIN: {
                /* Load from builtin resources */
                listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
                sel = (int)SendMessage(listBox, LB_GETCURSEL, 0, 0);
                itemData = (int)SendMessage(listBox, LB_GETITEMDATA, sel, 0);
                
                if (sel != LB_ERR && itemData >= 0 && itemData < getCityCount()) {
                    cityInfo = getCityInfo(itemData);
                    if (cityInfo) {
                        currentConfig->scenarioId = cityInfo->resourceId;
                        currentConfig->loadFromInternal = 1;
                        strcpy(currentConfig->loadFile, cityInfo->filename);
                    }
                } else {
                    MessageBox(hwnd, "Please select a builtin city to load.", "Error", MB_OK | MB_ICONERROR);
                    return TRUE;
                }
                break;
            }
            
            case NEWGAME_LOAD_CITY_FILE: {
                /* Load from external file */
                fileName[0] = '\0';
                
                /* Setup file dialog */
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = sizeof(fileName);
                ofn.lpstrFilter = "City Files (*.cty)\0*.cty\0All Files (*.*)\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.lpstrTitle = "Load City";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                
                if (GetOpenFileName(&ofn)) {
                    strcpy(currentConfig->loadFile, fileName);
                    currentConfig->loadFromInternal = 0;
                } else {
                    return TRUE; /* User cancelled */
                }
                break;
            }
            
            case NEWGAME_SCENARIO: {
                listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
                sel = (int)SendMessage(listBox, LB_GETCURSEL, 0, 0);
                
                if (sel != LB_ERR && sel >= 0 && sel < scenarioCount) {
                    currentConfig->scenarioId = scenarios[sel].id;
                    addGameLog("New game dialog: Starting scenario %d (%s)", 
                              scenarios[sel].id, scenarios[sel].name);
                } else {
                    MessageBox(hwnd, "Please select a scenario.", "Error", MB_OK | MB_ICONERROR);
                    return TRUE;
                }
                break;
            }
            }
            
            addGameLog("New game dialog completed successfully");
            EndDialog(hwnd, 1);
            return TRUE;
        }
        
        case IDC_CANCEL:
            addGameLog("User clicked Cancel in new game dialog");
            /* Clean up preview bitmap */
            if (currentPreviewBitmap) {
                DeleteObject(currentPreviewBitmap);
                currentPreviewBitmap = NULL;
            }
            EndDialog(hwnd, 0);
            return TRUE;
        }
        break;
    }
    
    case WM_HSCROLL: {
        int scrollId = GetDlgCtrlID((HWND)lParam);
        int pos = GetScrollPos((HWND)lParam, SB_CTL);
        char labelText[64];
        
        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            pos = max(0, pos - 1);
            break;
        case SB_LINEDOWN:
            pos = min(100, pos + 1);
            break;
        case SB_PAGEUP:
            pos = max(0, pos - 10);
            break;
        case SB_PAGEDOWN:
            pos = min(100, pos + 10);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            pos = HIWORD(wParam);
            break;
        }
        
        SetScrollPos((HWND)lParam, SB_CTL, pos, TRUE);
        
        /* Update labels and config */
        if (scrollId == IDC_WATER_PERCENT) {
            sprintf(labelText, "Water: %d%%", pos);
            SetDlgItemText(hwnd, IDC_WATER_LABEL, labelText);
            if (currentConfig) {
                currentConfig->waterPercent = pos;
            }
        } else if (scrollId == IDC_FOREST_PERCENT) {
            sprintf(labelText, "Forest: %d%%", pos);
            SetDlgItemText(hwnd, IDC_FOREST_LABEL, labelText);
            if (currentConfig) {
                currentConfig->forestPercent = pos;
            }
        }
        
        return TRUE;
    }
    
    case WM_CLOSE:
        /* Clean up preview bitmap */
        if (currentPreviewBitmap) {
            DeleteObject(currentPreviewBitmap);
            currentPreviewBitmap = NULL;
        }
        EndDialog(hwnd, 0);
        return TRUE;
    }
    
    return FALSE;
}

/* Scenario selection dialog procedure */
BOOL CALLBACK scenarioDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int *selectedScenario = NULL;
    
    switch (msg) {
    case WM_INITDIALOG: {
        HWND listBox;
        int i;
        
        selectedScenario = (int*)lParam;
        
        /* Populate scenario list */
        listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
        for (i = 0; i < scenarioCount; i++) {
            SendMessage(listBox, LB_ADDSTRING, 0, (LPARAM)scenarios[i].name);
            SendMessage(listBox, LB_SETITEMDATA, i, i);
        }
        
        /* Select first scenario by default */
        SendMessage(listBox, LB_SETCURSEL, 0, 0);
        
        /* Update description */
        SetDlgItemText(hwnd, IDC_SCENARIO_DESC, scenarios[0].description);
        
        return TRUE;
    }
    
    case WM_COMMAND: {
        HWND listBox;
        int sel;
        
        switch (LOWORD(wParam)) {
        case IDC_SCENARIO_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
                sel = (int)SendMessage(listBox, LB_GETCURSEL, 0, 0);
                
                if (sel != LB_ERR && sel >= 0 && sel < scenarioCount) {
                    addGameLog("Scenario dialog: Selected scenario %d (%s): %s", 
                              sel, scenarios[sel].name, scenarios[sel].description);
                    SetDlgItemText(hwnd, IDC_SCENARIO_DESC, scenarios[sel].description);
                }
            } else if (HIWORD(wParam) == LBN_DBLCLK) {
                /* Double-click selects and closes dialog */
                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_SCENARIO_OK, BN_CLICKED), 0);
            }
            break;
            
        case IDC_SCENARIO_OK: {
            listBox = GetDlgItem(hwnd, IDC_SCENARIO_LIST);
            sel = (int)SendMessage(listBox, LB_GETCURSEL, 0, 0);
            
            if (sel != LB_ERR && sel >= 0 && sel < scenarioCount && selectedScenario) {
                *selectedScenario = scenarios[sel].id;
                addGameLog("Scenario dialog: Selected scenario %d (%s) for loading", 
                          scenarios[sel].id, scenarios[sel].name);
                EndDialog(hwnd, 1);
            }
            return TRUE;
        }
        
        case IDC_SCENARIO_CANCEL:
            EndDialog(hwnd, 0);
            return TRUE;
        }
        break;
    }
    
    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;
    }
    
    return FALSE;
}

/* Initialize new game based on configuration */
int initNewGame(NewGameConfig *config) {
    if (!config) {
        return 0;
    }
    
    switch (config->gameType) {
    case NEWGAME_NEW_CITY:
        return generateNewCityWithTerrain(config->cityName, config->difficulty, 
                                        config->mapType, config->waterPercent, 
                                        config->forestPercent);
        
    case NEWGAME_LOAD_CITY_BUILTIN: {
        /* Load city from internal resources */
        GameAssetInfo* cityInfo = NULL;
        int i;
        /* Find the city info by resource ID */
        for (i = 0; i < getCityCount(); i++) {
            GameAssetInfo* info = getCityInfo(i);
            if (info && info->resourceId == config->scenarioId) {
                cityInfo = info;
                break;
            }
        }
        
        if (cityInfo) {
            return loadCityFromResource(config->scenarioId, cityInfo->description);
        } else {
            addGameLog("ERROR: Could not find city info for resource ID %d", config->scenarioId);
            return loadCityFromResource(config->scenarioId, "Unknown City");
        }
    }
        
    case NEWGAME_LOAD_CITY_FILE:
        /* Load city from external file */
        return loadCityFile(config->loadFile);
        
    case NEWGAME_SCENARIO:
        return loadScenarioById(config->scenarioId);
        
    default:
        addGameLog("ERROR: Unknown game type in new game configuration.");
        return 0;
    }
}

/* Clear map for new city */
static void clearMapForNewCity(void) {
    int x, y;
    
    addGameLog("Clearing map for new city");
    
    /* Clear the main map - set all tiles to empty dirt */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            setMapTile(x, y, DIRT, 0, TILE_SET_REPLACE, "clearMapForNewCity");
        }
    }
    
    /* Reset all population values to 0 for new city */
    ResPop = 0;
    ComPop = 0;
    IndPop = 0;
    TotalPop = 0;
    CityPop = 0;
    CityClass = 0;
    LastTotalPop = 0;
}

/* Generate a new city */
int generateNewCity(char *cityName, int difficulty) {
    if (!cityName) {
        return 0;
    }
    
    addGameLog("Generating new city: %s", cityName);
    
    /* Set difficulty level */
    GameLevel = difficulty;
    
    /* Set starting funds based on difficulty */
    switch (difficulty) {
    case DIFFICULTY_EASY:
        TotalFunds = 20000;
        break;
    case DIFFICULTY_MEDIUM:
        TotalFunds = 10000;
        break;
    case DIFFICULTY_HARD:
        TotalFunds = 5000;
        break;
    default:
        TotalFunds = 10000;
        break;
    }
    
    /* Clear the map completely for a fresh start */
    clearMapForNewCity();
    
    /* Initialize simulation (but preserve the funds we just set) */
    DoSimInit();
    
    /* Restore the funds since DoSimInit overwrites them */
    switch (difficulty) {
    case DIFFICULTY_EASY:
        TotalFunds = 20000;
        break;
    case DIFFICULTY_MEDIUM:
        TotalFunds = 10000;
        break;
    case DIFFICULTY_HARD:
        TotalFunds = 5000;
        break;
    default:
        TotalFunds = 10000;
        break;
    }
    
    RValve = 0;
    CValve = 0;
    IValve = 0;
    ValveFlag = 1;
    
    addGameLog("New city '%s' generated successfully. Difficulty: %s, Funds: $%ld", 
              cityName, 
              (difficulty == DIFFICULTY_EASY) ? "Easy" : 
              (difficulty == DIFFICULTY_MEDIUM) ? "Medium" : "Hard",
              TotalFunds);
    
    return 1;
}

/* Generate a new city with terrain */
int generateNewCityWithTerrain(char *cityName, int difficulty, int mapType, int waterPercent, int forestPercent) {
    MapGenParams params;
    
    if (!cityName) {
        return 0;
    }
    
    addGameLog("Generating new city with terrain: %s (Type: %s, Water: %d%%, Forest: %d%%)", 
              cityName, (mapType == MAPTYPE_RIVERS) ? "Rivers" : "Island", 
              waterPercent, forestPercent);
    
    /* Set difficulty level */
    GameLevel = difficulty;
    
    /* Set starting funds based on difficulty */
    switch (difficulty) {
    case DIFFICULTY_EASY:
        TotalFunds = 20000;
        break;
    case DIFFICULTY_MEDIUM:
        TotalFunds = 10000;
        break;
    case DIFFICULTY_HARD:
        TotalFunds = 5000;
        break;
    default:
        TotalFunds = 10000;
        break;
    }
    
    /* Initialize simulation first */
    DoSimInit();
    
    /* Generate terrain based on parameters */
    params.mapType = mapType;
    params.waterPercent = waterPercent;
    params.forestPercent = forestPercent;
    
    if (!generateTerrainMap(&params)) {
        addGameLog("ERROR: Failed to generate terrain map");
        /* Fall back to blank map */
        clearMapForNewCity();
    }
    
    /* Restore the funds since DoSimInit overwrites them */
    switch (difficulty) {
    case DIFFICULTY_EASY:
        TotalFunds = 20000;
        break;
    case DIFFICULTY_MEDIUM:
        TotalFunds = 10000;
        break;
    case DIFFICULTY_HARD:
        TotalFunds = 5000;
        break;
    default:
        TotalFunds = 10000;
        break;
    }
    
    RValve = 0;
    CValve = 0;
    IValve = 0;
    ValveFlag = 1;
    
    addGameLog("New city '%s' with terrain generated successfully. Difficulty: %s, Funds: $%ld", 
              cityName, 
              (difficulty == DIFFICULTY_EASY) ? "Easy" : 
              (difficulty == DIFFICULTY_MEDIUM) ? "Medium" : "Hard",
              TotalFunds);
    
    return 1;
}

/* Load city from file */
int loadCityFile(char *filename) {
    /* Use the same loading function as File->Open City to ensure consistency */
    extern int loadCity(char *filename);
    
    if (!filename || strlen(filename) == 0) {
        return 0;
    }
    
    addGameLog("Loading city from file: %s", filename);
    
    if (loadCity(filename)) {
        addGameLog("City loaded successfully from %s", filename);
        return 1;
    } else {
        addGameLog("ERROR: Failed to load city from %s", filename);
        return 0;
    }
}

/* Get scenario count */
int getScenarioCount(void) {
    return scenarioCount;
}

/* Get scenario info by index */
ScenarioInfo* getScenarioInfo(int index) {
    if (index < 0 || index >= scenarioCount) {
        return NULL;
    }
    return &scenarios[index];
}


/* Load scenario by ID */
int loadScenarioById(int scenarioId) {
    if (scenarioId < 1 || scenarioId > scenarioCount) {
        addGameLog("ERROR: Invalid scenario ID: %d", scenarioId);
        return 0;
    }
    
    addGameLog("Loading scenario: %s", scenarios[scenarioId - 1].name);
    
    if (loadScenario(scenarioId)) {
        addGameLog("Scenario '%s' loaded successfully", scenarios[scenarioId - 1].name);
        return 1;
    } else {
        addGameLog("ERROR: Failed to load scenario '%s'", scenarios[scenarioId - 1].name);
        return 0;
    }
}
