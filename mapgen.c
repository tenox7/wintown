#include "mapgen.h"
#include "sim.h"
#include "tiles.h"
#include <windows.h>

extern int TreeLevel, LakeLevel, CurveLevel, CreateIsland;
extern int GenerateMap();
extern short Rand16();

int generateTerrainMap(MapGenParams *params) {
    if (!params) return 0;

    switch (params->mapType) {
    case MAPTYPE_ISLAND:
        CreateIsland = 1;
        break;
    case MAPTYPE_RIVERS:
        CreateIsland = 0;
        break;
    default:
        CreateIsland = -1;
        break;
    }

    LakeLevel = (params->waterPercent > 0) ? params->waterPercent / 3 : 0;
    TreeLevel = (params->forestPercent > 0) ? params->forestPercent : 0;
    CurveLevel = -1;

    GenerateMap(Rand16());
    return 1;
}

int generateMapPreview(MapGenParams *params, HBITMAP *previewBitmap, int width, int height) {
    HDC hdc, memDC;
    HBITMAP oldBitmap;
    int x, y, mapX, mapY, tile;
    int actualWidth, actualHeight, offsetX, offsetY;
    float aspectRatio, widthRatio, heightRatio;
    COLORREF tileColor;
    HBRUSH bgBrush;

    if (!params || !previewBitmap) return 0;

    aspectRatio = (float)WORLD_X / (float)WORLD_Y;
    widthRatio = (float)width / aspectRatio;
    heightRatio = (float)height * aspectRatio;

    if (widthRatio <= height) {
        actualWidth = width;
        actualHeight = (int)widthRatio;
    } else {
        actualWidth = (int)heightRatio;
        actualHeight = height;
    }

    if (!generateTerrainMap(params)) return 0;

    hdc = GetDC(NULL);
    memDC = CreateCompatibleDC(hdc);
    *previewBitmap = CreateCompatibleBitmap(hdc, width, height);
    oldBitmap = SelectObject(memDC, *previewBitmap);

    bgBrush = CreateSolidBrush(RGB(192, 192, 192));
    SelectObject(memDC, bgBrush);
    PatBlt(memDC, 0, 0, width, height, PATCOPY);
    DeleteObject(bgBrush);

    offsetX = (width - actualWidth) / 2;
    offsetY = (height - actualHeight) / 2;

    for (y = 0; y < actualHeight; y++) {
        for (x = 0; x < actualWidth; x++) {
            mapX = (x * WORLD_X) / actualWidth;
            mapY = (y * WORLD_Y) / actualHeight;
            tile = getMapTile(mapX, mapY);

            if (tile == DIRT) {
                tileColor = RGB(139, 69, 19);
            } else if (tile >= RIVER && tile <= LASTRIVEDGE) {
                tileColor = RGB(0, 0, 200);
            } else if (tile >= TREEBASE && tile <= WOODS) {
                tileColor = RGB(0, 100, 0);
            } else {
                tileColor = RGB(139, 69, 19);
            }

            SetPixel(memDC, x + offsetX, y + offsetY, tileColor);
        }
    }

    SelectObject(memDC, oldBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdc);
    return 1;
}

int renderMapPreview(HBITMAP *previewBitmap, int width, int height) {
    HDC hdc, memDC;
    HBITMAP oldBitmap;
    int x, y, mapX, mapY, tile;
    int actualWidth, actualHeight, offsetX, offsetY;
    float aspectRatio, widthRatio, heightRatio;
    COLORREF tileColor;
    HBRUSH bgBrush;

    if (!previewBitmap) return 0;

    aspectRatio = (float)WORLD_X / (float)WORLD_Y;
    widthRatio = (float)width / aspectRatio;
    heightRatio = (float)height * aspectRatio;

    if (widthRatio <= height) {
        actualWidth = width;
        actualHeight = (int)widthRatio;
    } else {
        actualWidth = (int)heightRatio;
        actualHeight = height;
    }

    hdc = GetDC(NULL);
    memDC = CreateCompatibleDC(hdc);
    *previewBitmap = CreateCompatibleBitmap(hdc, width, height);
    oldBitmap = SelectObject(memDC, *previewBitmap);

    bgBrush = CreateSolidBrush(RGB(192, 192, 192));
    SelectObject(memDC, bgBrush);
    PatBlt(memDC, 0, 0, width, height, PATCOPY);
    DeleteObject(bgBrush);

    offsetX = (width - actualWidth) / 2;
    offsetY = (height - actualHeight) / 2;

    for (y = 0; y < actualHeight; y++) {
        for (x = 0; x < actualWidth; x++) {
            mapX = (x * WORLD_X) / actualWidth;
            mapY = (y * WORLD_Y) / actualHeight;
            tile = getMapTile(mapX, mapY) & LOMASK;

            if (tile == 0) {
                tileColor = RGB(139, 69, 19);
            } else if (tile <= 20) {
                tileColor = RGB(0, 0, 200);
            } else if (tile <= 43) {
                tileColor = RGB(0, 100, 0);
            } else if (tile <= 63) {
                tileColor = RGB(255, 100, 0);
            } else if (tile <= 207) {
                tileColor = RGB(160, 160, 160);
            } else if (tile <= 239) {
                tileColor = RGB(200, 180, 0);
            } else if (tile <= 422) {
                tileColor = RGB(100, 200, 100);
            } else if (tile <= 611) {
                tileColor = RGB(100, 150, 255);
            } else if (tile <= 843) {
                tileColor = RGB(200, 180, 100);
            } else {
                tileColor = RGB(128, 128, 128);
            }

            SetPixel(memDC, x + offsetX, y + offsetY, tileColor);
        }
    }

    SelectObject(memDC, oldBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdc);
    return 1;
}

void initMapGenParams(MapGenParams *params) {
    if (!params) return;
    params->mapType = MAPTYPE_RIVERS;
    params->waterPercent = 25;
    params->forestPercent = 30;
}
