#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include "sim.h"
#include "assets.h"

extern int loadCity(char *filename);
extern int loadFile(char *filename);


static GameAssetInfo tilesetAssets[] = {
    { IDR_TILESET_CLASSIC, "classic.bmp", "Classic Tileset" },
    { IDR_TILESET_CLASSIC95, "classic95.bmp", "Classic 95 Tileset" },
    { IDR_TILESET_DEFAULT, "default.bmp", "Default Tileset" },
    { IDR_TILESET_ANCIENTASIA, "ancientasia.bmp", "Ancient Asia Tileset" },
    { IDR_TILESET_DUX, "dux.bmp", "Dux Tileset" },
    { IDR_TILESET_FUTUREEUROPE, "futureeurope.bmp", "Future Europe Tileset" },
    { IDR_TILESET_FUTUREUSA, "futureusa.bmp", "Future USA Tileset" },
    { IDR_TILESET_MEDIEVALTIMES, "medievaltimes.bmp", "Medieval Times Tileset" },
    { IDR_TILESET_MOONCOLONY, "mooncolony.bmp", "Moon Colony Tileset" },
    { IDR_TILESET_WILDWEST, "wildwest.bmp", "Wild West Tileset" },
    { 0, NULL, NULL }
};

static GameAssetInfo cityAssets[] = {
    { IDR_CITY_BADNEWS, "badnews.cty", "Bad News City" },
    { IDR_CITY_BRUCE, "bruce.cty", "Bruce City" },
    { IDR_CITY_DEADWOOD, "deadwood.cty", "Deadwood City" },
    { IDR_CITY_FINNIGAN, "finnigan.cty", "Finnigan City" },
    { IDR_CITY_FREDS, "freds.cty", "Fred's City" },
    { IDR_CITY_HAIGHT, "haight.cty", "Haight City" },
    { IDR_CITY_HAPPISLE, "happisle.cty", "Happy Isle City" },
    { IDR_CITY_JOFFBURG, "joffburg.cty", "Joffburg City" },
    { IDR_CITY_KAMAKURA, "kamakura.cty", "Kamakura City" },
    { IDR_CITY_KOBE, "kobe.cty", "Kobe City" },
    { IDR_CITY_KOWLOON, "kowloon.cty", "Kowloon City" },
    { IDR_CITY_KYOTO, "kyoto.cty", "Kyoto City" },
    { IDR_CITY_LINECITY, "linecity.cty", "Line City" },
    { IDR_CITY_MED_ISLE, "med_isle.cty", "Med Isle City" },
    { IDR_CITY_NDULLS, "ndulls.cty", "N'Dulls City" },
    { IDR_CITY_NEATMAP, "neatmap.cty", "Neat Map City" },
    { IDR_CITY_RADIAL, "radial.cty", "Radial City" },
    { IDR_CITY_SENRI, "senri.cty", "Senri City" },
    { IDR_CITY_SIMCITY4, "simcity4.cty", "SimCity 4 City" },
    { IDR_CITY_SOUTHPAC, "southpac.cty", "South Pacific City" },
    { IDR_CITY_SPLATS, "splats.cty", "Splats City" },
    { IDR_CITY_WETCITY, "wetcity.cty", "Wet City" },
    { IDR_CITY_YOKOHAMA, "yokohama.cty", "Yokohama City" },
    { 0, NULL, NULL }
};

static GameAssetInfo scenarioAssets[] = {
    { IDR_SCENARIO_BERN, "bern.scn", "Bern Scenario" },
    { IDR_SCENARIO_BOSTON, "boston.scn", "Boston Scenario" },
    { IDR_SCENARIO_DETROIT, "detroit.scn", "Detroit Scenario" },
    { IDR_SCENARIO_DULLSVILLE, "dullsville.scn", "Dullsville Scenario" },
    { IDR_SCENARIO_HAMBURG, "hamburg.scn", "Hamburg Scenario" },
    { IDR_SCENARIO_RIO, "rio.scn", "Rio Scenario" },
    { IDR_SCENARIO_SANFRANCISCO, "sanfrancisco.scn", "San Francisco Scenario" },
    { IDR_SCENARIO_TOKYO, "tokyo.scn", "Tokyo Scenario" },
    { 0, NULL, NULL }
};

static GameAssetInfo toolIconAssets[] = {
    { IDR_ICON_RESIDENTIAL, "resident", "Residential Tool" },
    { IDR_ICON_COMMERCE, "commerce", "Commercial Tool" },
    { IDR_ICON_INDUSTRIAL, "industrl", "Industrial Tool" },
    { IDR_ICON_FIRESTATION, "firest", "Fire Station Tool" },
    { IDR_ICON_POLICESTATION, "policest", "Police Station Tool" },
    { IDR_ICON_POWERLINES, "powerln", "Power Lines Tool" },
    { IDR_ICON_ROAD, "road", "Road Tool" },
    { IDR_ICON_RAIL, "rail", "Rail Tool" },
    { IDR_ICON_PARK, "park", "Park Tool" },
    { IDR_ICON_STADIUM, "stadium", "Stadium Tool" },
    { IDR_ICON_SEAPORT, "seaport", "Seaport Tool" },
    { IDR_ICON_POWERPLANT, "coal", "Power Plant Tool" },
    { IDR_ICON_NUCLEAR, "nuclear", "Nuclear Plant Tool" },
    { IDR_ICON_AIRPORT, "airport", "Airport Tool" },
    { IDR_ICON_BULLDOZER, "bulldzr", "Bulldozer Tool" },
    { IDR_ICON_QUERY, "query", "Query Tool" },
    { IDR_ICON_BULLDOZER, "bulldzr", "No Tool" },
    { IDR_CPU_X86, "x86", "x86 CPU" },
    { IDR_CPU_X64, "x64", "x64 CPU" },
    { IDR_CPU_MIPS, "mips", "MIPS CPU" },
    { IDR_CPU_AXP, "axp", "Alpha CPU" },
    { IDR_CPU_AXP64, "axp64", "Alpha64 CPU" },
    { IDR_CPU_PPC, "ppc", "PowerPC CPU" },
    { IDR_CPU_ARM, "arm", "ARM CPU" },
    { IDR_CPU_ARM64, "arm64", "ARM64 CPU" },
    { IDR_CPU_IA64, "ia64", "IA64 CPU" },
    { 0, NULL, NULL }
};

HGLOBAL loadResourceData(int resourceId, DWORD* size) {
    HRSRC hRes;
    HGLOBAL hData;
    
    hRes = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) {
        return NULL;
    }
    
    if (size) {
        *size = SizeofResource(GetModuleHandle(NULL), hRes);
    }
    
    hData = LoadResource(GetModuleHandle(NULL), hRes);
    return hData;
}

int loadCityFromResource(int resourceId, char* cityName) {
    HGLOBAL hData;
    DWORD size;
    char* data;
    FILE* tempFile;
    char tempPath[MAX_PATH];
    char tempFileName[MAX_PATH];
    DWORD written;
    
    hData = loadResourceData(resourceId, &size);
    if (!hData) {
        return 0;
    }
    
    data = (char*)LockResource(hData);
    if (!data) {
        return 0;
    }
    
    GetTempPath(MAX_PATH, tempPath);
    GetTempFileName(tempPath, "WNT", 0, tempFileName);
    
    tempFile = fopen(tempFileName, "wb");
    if (!tempFile) {
        return 0;
    }
    
    written = (DWORD)fwrite(data, 1, size, tempFile);
    fclose(tempFile);
    
    if (written != size) {
        DeleteFile(tempFileName);
        return 0;
    }
    
    if (loadCity(tempFileName)) {
        DeleteFile(tempFileName);
        /* Set the city filename to the provided name instead of temp filename */
        if (cityName) {
            extern char cityFileName[MAX_PATH];
            extern HWND hwndMain;
            char windowTitle[256];
            strcpy(cityFileName, cityName);
            
            /* Update window title */
            wsprintf(windowTitle, "WiNTown - %s", cityName);
            SetWindowText(hwndMain, windowTitle);
        }
        return 1;
    }
    
    DeleteFile(tempFileName);
    return 0;
}

int loadScenarioFromResource(int resourceId, char* scenarioName) {
    HGLOBAL hData;
    DWORD size;
    char* data;
    FILE* tempFile;
    char tempPath[MAX_PATH];
    char tempFileName[MAX_PATH];
    DWORD written;
    
    hData = loadResourceData(resourceId, &size);
    if (!hData) {
        return 0;
    }
    
    data = (char*)LockResource(hData);
    if (!data) {
        return 0;
    }
    
    GetTempPath(MAX_PATH, tempPath);
    GetTempFileName(tempPath, "WNT", 0, tempFileName);
    
    tempFile = fopen(tempFileName, "wb");
    if (!tempFile) {
        return 0;
    }
    
    written = (DWORD)fwrite(data, 1, size, tempFile);
    fclose(tempFile);
    
    if (written != size) {
        DeleteFile(tempFileName);
        return 0;
    }
    
    if (loadFile(tempFileName)) {
        DeleteFile(tempFileName);
        /* Note: scenarioName is already set correctly by calling function */
        /* Do not overwrite it with tempFileName */
        return 1;
    }
    
    DeleteFile(tempFileName);
    return 0;
}

HBITMAP loadBitmapFromResource(int resourceId) {
    HGLOBAL hData;
    DWORD size;
    char* data;
    HBITMAP hBitmap;
    
    hData = loadResourceData(resourceId, &size);
    if (!hData) {
        return NULL;
    }
    
    data = (char*)LockResource(hData);
    if (!data) {
        return NULL;
    }
    
    hBitmap = CreateBitmap(16, 16, 1, 8, data);
    
    return hBitmap;
}

int getTilesetCount() {
    int count = 0;
    extern void addDebugLog(const char *format, ...);
    
    while (tilesetAssets[count].resourceId != 0) {
        count++;
    }
    
    addDebugLog("getTilesetCount: Found %d tilesets", count);
    return count;
}

GameAssetInfo* getTilesetInfo(int index) {
    if (index < 0 || index >= getTilesetCount()) {
        return NULL;
    }
    return &tilesetAssets[index];
}

int getCityCount() {
    int count = 0;
    while (cityAssets[count].resourceId != 0) {
        count++;
    }
    return count;
}

GameAssetInfo* getCityInfo(int index) {
    if (index < 0 || index >= getCityCount()) {
        return NULL;
    }
    return &cityAssets[index];
}

int getAssetScenarioCount() {
    int count = 0;
    while (scenarioAssets[count].resourceId != 0) {
        count++;
    }
    return count;
}

GameAssetInfo* getAssetScenarioInfo(int index) {
    if (index < 0 || index >= getAssetScenarioCount()) {
        return NULL;
    }
    return &scenarioAssets[index];
}

int findCityResourceByName(const char* filename) {
    int i;
    for (i = 0; cityAssets[i].resourceId != 0; i++) {
        if (strcmp(cityAssets[i].filename, filename) == 0) {
            return cityAssets[i].resourceId;
        }
    }
    return 0;
}

int findScenarioResourceByName(const char* filename) {
    int i;
    for (i = 0; scenarioAssets[i].resourceId != 0; i++) {
        if (strcmp(scenarioAssets[i].filename, filename) == 0) {
            return scenarioAssets[i].resourceId;
        }
    }
    return 0;
}

int findTilesetResourceByName(const char* filename) {
    int i;
    extern void addDebugLog(const char *format, ...);
    
    addDebugLog("findTilesetResourceByName: Looking for tileset '%s'", filename);
    
    for (i = 0; tilesetAssets[i].resourceId != 0; i++) {
        addDebugLog("findTilesetResourceByName: Checking '%s' against '%s'", filename, tilesetAssets[i].filename);
        if (strcmp(tilesetAssets[i].filename, filename) == 0) {
            addDebugLog("findTilesetResourceByName: Found match! Resource ID = %d", tilesetAssets[i].resourceId);
            return tilesetAssets[i].resourceId;
        }
    }
    
    addDebugLog("findTilesetResourceByName: No match found for '%s'", filename);
    return 0;
}

HBITMAP loadTilesetFromResource(int resourceId) {
    HGLOBAL hData;
    DWORD size;
    char* data;
    HBITMAP hBitmap;
    FILE* tempFile;
    char tempPath[MAX_PATH];
    char tempFileName[MAX_PATH];
    DWORD written;
    extern void addDebugLog(const char *format, ...);
    extern HANDLE LoadImageFromFile(LPCSTR filename, UINT fuLoad);
    
    addDebugLog("loadTilesetFromResource: Loading resource ID %d", resourceId);
    
    hData = loadResourceData(resourceId, &size);
    if (!hData) {
        addDebugLog("loadTilesetFromResource: Failed to load resource data for ID %d", resourceId);
        return NULL;
    }
    
    addDebugLog("loadTilesetFromResource: Resource data loaded, size = %d bytes", size);
    
    data = (char*)LockResource(hData);
    if (!data) {
        return NULL;
    }
    
    /* Create temporary file to write bitmap data */
    GetTempPath(MAX_PATH, tempPath);
    GetTempFileName(tempPath, "WNT", 0, tempFileName);
    
    tempFile = fopen(tempFileName, "wb");
    if (!tempFile) {
        addDebugLog("loadTilesetFromResource: Failed to create temp file");
        return NULL;
    }
    
    written = (DWORD)fwrite(data, 1, size, tempFile);
    fclose(tempFile);
    
    if (written != size) {
        DeleteFile(tempFileName);
        addDebugLog("loadTilesetFromResource: Failed to write all data to temp file");
        return NULL;
    }
    
    /* Use LoadImageFromFile with same flags as original code */
    hBitmap = (HBITMAP)LoadImageFromFile(tempFileName, LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTCOLOR);
    
    /* Clean up temp file */
    DeleteFile(tempFileName);
    
    if (hBitmap) {
        /* Convert to 8-bit using system palette for proper color rendering */
        extern HPALETTE hPalette;
        extern HBITMAP convertTo8Bit(HBITMAP hSrcBitmap, HDC hdc, HPALETTE hSystemPalette);
        extern HWND hwndMain;
        
        HDC hdcTemp = GetDC(hwndMain);
        HBITMAP hConvertedBitmap = convertTo8Bit(hBitmap, hdcTemp, hPalette);
        ReleaseDC(hwndMain, hdcTemp);
        
        if (hConvertedBitmap) {
            DeleteObject(hBitmap);
            hBitmap = hConvertedBitmap;
            addDebugLog("loadTilesetFromResource: Successfully converted to 8-bit palette");
        } else {
            addDebugLog("loadTilesetFromResource: Failed to convert to 8-bit, using original");
        }
        
        addDebugLog("loadTilesetFromResource: Successfully loaded tileset bitmap");
    } else {
        addDebugLog("loadTilesetFromResource: Failed to load bitmap from temp file");
    }
    
    return hBitmap;
}

int findToolIconResourceByName(const char* toolName) {
    int i;
    extern void addDebugLog(const char *format, ...);
    
    addDebugLog("findToolIconResourceByName: Looking for tool icon '%s'", toolName);
    
    for (i = 0; toolIconAssets[i].resourceId != 0; i++) {
        if (strcmp(toolIconAssets[i].filename, toolName) == 0) {
            addDebugLog("findToolIconResourceByName: Found match! Resource ID = %d", toolIconAssets[i].resourceId);
            return toolIconAssets[i].resourceId;
        }
    }
    
    addDebugLog("findToolIconResourceByName: No match found for '%s'", toolName);
    return 0;
}

int findSpriteResourceByName(const char* spriteType, int frameNumber) {
    extern void addDebugLog(const char *format, ...);
    
    addDebugLog("findSpriteResourceByName: Looking for sprite '%s' frame %d", spriteType, frameNumber);
    
    /* Calculate resource ID based on sprite type and frame */
    if (strcmp(spriteType, "airplane") == 0) {
        if (frameNumber >= 0 && frameNumber <= 10) {
            return IDR_SPRITE_AIRPLANE_0 + frameNumber;
        }
    } else if (strcmp(spriteType, "bus") == 0) {
        if (frameNumber >= 0 && frameNumber <= 3) {
            return IDR_SPRITE_BUS_0 + frameNumber;
        }
    } else if (strcmp(spriteType, "helicopter") == 0) {
        if (frameNumber >= 0 && frameNumber <= 7) {
            return IDR_SPRITE_HELICOPTER_0 + frameNumber;
        }
    } else if (strcmp(spriteType, "monster") == 0) {
        if (frameNumber >= 0 && frameNumber <= 15) {
            return IDR_SPRITE_MONSTER_0 + frameNumber;
        }
    } else if (strcmp(spriteType, "ship") == 0) {
        if (frameNumber >= 0 && frameNumber <= 7) {
            return IDR_SPRITE_SHIP_0 + frameNumber;
        }
    } else if (strcmp(spriteType, "train") == 0) {
        if (frameNumber >= 0 && frameNumber <= 4) {
            return IDR_SPRITE_TRAIN_0 + frameNumber;
        }
    } else if (strcmp(spriteType, "explosion") == 0) {
        if (frameNumber >= 0 && frameNumber <= 5) {
            return IDR_SPRITE_EXPLOSION_0 + frameNumber;
        }
    } else if (strcmp(spriteType, "tornado") == 0) {
        if (frameNumber >= 0 && frameNumber <= 2) {
            return IDR_SPRITE_TORNADO_0 + frameNumber;
        }
    }
    
    addDebugLog("findSpriteResourceByName: No match found for '%s' frame %d", spriteType, frameNumber);
    return 0;
}