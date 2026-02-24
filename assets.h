#ifndef ASSETS_H
#define ASSETS_H

#include <windows.h>

typedef struct {
    int resourceId;
    char* filename;
    char* description;
} GameAssetInfo;

HGLOBAL loadResourceData(int resourceId, DWORD* size);
int loadCityFromResource(int resourceId, char* cityName);
int loadScenarioFromResource(int resourceId, char* scenarioName);
HBITMAP loadBitmapFromResource(int resourceId);

int getTilesetCount();
GameAssetInfo* getTilesetInfo(int index);

int getCityCount();
GameAssetInfo* getCityInfo(int index);

int getAssetScenarioCount();
GameAssetInfo* getAssetScenarioInfo(int index);

int findCityResourceByName(const char* filename);
int findScenarioResourceByName(const char* filename);
int findTilesetResourceByName(const char* filename);
HBITMAP loadTilesetFromResource(int resourceId);
int findToolIconResourceByName(const char* toolName);
int findSpriteResourceByName(const char* spriteType, int frameNumber);

#endif