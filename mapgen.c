/* mapgen.c - Map Generation System for WiNTown
 * Based on original Micropolis terrain generation algorithms
 * Adapted for Windows NT 4.0 with Visual Studio 4.0
 */

#include "mapgen.h"
#include "sim.h"
#include "tiles.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>

/* External functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* Map generation constants */
#define WORLD_X 120
#define WORLD_Y 100
#define RIVER_TILE 2
#define RIVER_EDGE_TILE 3
#define CHANNEL_TILE 4
#define WOODS_TILE 37
#define DIRT_TILE 0

/* Random number generator state */
static int randArray[5] = {1018, 4521, 202, 419, 3};

/* Map generation parameters */
static MapGenParams currentParams;

/* Initialize random number generator */
static void initRandom(void) {
    DWORD tick = GetTickCount();
    randArray[0] = tick & 0xFFFF;
    randArray[1] = (tick >> 8) & 0xFFFF;
    randArray[2] = (tick >> 16) & 0xFFFF;
    randArray[3] = (tick >> 24) & 0xFFFF;
    randArray[4] = tick & 0x7FFF;
}

/* Generate random number in range [0, range] */
static int genRandom(int range) {
    register int x, newv, divisor;
    
    if (range <= 0) return 0;
    
    divisor = 32767 / (range + 1);
    newv = 0;
    
    for (x = 4; x > 0; x--) {
        newv += (randArray[x] = randArray[x-1]);
    }
    
    randArray[0] = newv;
    x = (newv & 32767) / divisor;
    
    if (x > range) return range;
    return x;
}

/* Enhanced random with skewed distribution */
static int genSkewedRandom(int limit) {
    int x, z;
    
    z = genRandom(limit);
    x = genRandom(limit);
    
    if (z < x) return z;
    return x;
}

/* Clear map to dirt */
static void clearMapToDirt(void) {
    int x, y;
    
    addGameLog("Clearing map to dirt for terrain generation");
    
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            setMapTile(x, y, DIRT_TILE, 0, TILE_SET_REPLACE, "clearMapToDirt");
        }
    }
}

/* Place river pattern at current position */
static void placeRiverPattern(int x, int y, int isLarge) {
    static int largePattern[9][9] = {
        {0,0,0,3,3,3,0,0,0},
        {0,0,3,2,2,2,3,0,0},
        {0,3,2,2,2,2,2,3,0},
        {3,2,2,2,2,2,2,2,3},
        {3,2,2,2,4,2,2,2,3},
        {3,2,2,2,2,2,2,2,3},
        {0,3,2,2,2,2,2,3,0},
        {0,0,3,2,2,2,3,0,0},
        {0,0,0,3,3,3,0,0,0}
    };
    
    static int smallPattern[6][6] = {
        {0,0,3,3,0,0},
        {0,3,2,2,3,0},
        {3,2,2,2,2,3},
        {3,2,2,2,2,3},
        {0,3,2,2,3,0},
        {0,0,3,3,0,0}
    };
    
    int px, py, tile, worldX, worldY;
    int size = isLarge ? 9 : 6;
    
    for (py = 0; py < size; py++) {
        for (px = 0; px < size; px++) {
            worldX = x + px;
            worldY = y + py;
            
            if (worldX >= 0 && worldX < WORLD_X && worldY >= 0 && worldY < WORLD_Y) {
                tile = isLarge ? largePattern[py][px] : smallPattern[py][px];
                if (tile != 0) {
                    int existing = getMapTile(worldX, worldY);
                    
                    /* Don't overwrite existing water with weaker water */
                    if (existing == RIVER_TILE && tile != CHANNEL_TILE) {
                        continue;
                    }
                    if (existing == CHANNEL_TILE) {
                        continue;
                    }
                    
                    setMapTile(worldX, worldY, tile, 0, TILE_SET_REPLACE, "placeRiverPattern");
                }
            }
        }
    }
}

/* Generate river system */
static void generateRivers(void) {
    int startX, startY, dir, lastDir;
    int x, y;
    static int dirTable[2][8] = {
        { 0, 1, 1, 1, 0, -1, -1, -1},
        {-1,-1, 0, 1, 1,  1,  0, -1}
    };
    
    addGameLog("Generating river system");
    
    /* Start position */
    startX = 40 + genRandom(40);
    startY = 33 + genRandom(33);
    
    /* Generate main river branch */
    x = startX;
    y = startY;
    lastDir = genRandom(3);
    dir = lastDir;
    
    while (x >= 0 && x < WORLD_X-9 && y >= 0 && y < WORLD_Y-9) {
        placeRiverPattern(x, y, 1); /* Large river pattern */
        
        /* Change direction with some randomness */
        if (genRandom(10) > 4) dir++;
        if (genRandom(10) > 4) dir--;
        if (genRandom(10) == 0) dir = lastDir;
        
        dir = dir & 7;
        x += dirTable[0][dir];
        y += dirTable[1][dir];
    }
    
    /* Generate secondary branch */
    x = startX;
    y = startY;
    lastDir = lastDir ^ 4;
    dir = lastDir;
    
    while (x >= 0 && x < WORLD_X-9 && y >= 0 && y < WORLD_Y-9) {
        placeRiverPattern(x, y, 1); /* Large river pattern */
        
        /* Change direction with some randomness */
        if (genRandom(10) > 4) dir++;
        if (genRandom(10) > 4) dir--;
        if (genRandom(10) == 0) dir = lastDir;
        
        dir = dir & 7;
        x += dirTable[0][dir];
        y += dirTable[1][dir];
    }
    
    /* Generate small tributary */
    x = startX;
    y = startY;
    lastDir = genRandom(3);
    dir = lastDir;
    
    while (x >= 0 && x < WORLD_X-6 && y >= 0 && y < WORLD_Y-6) {
        placeRiverPattern(x, y, 0); /* Small river pattern */
        
        /* Change direction with some randomness */
        if (genRandom(10) > 5) dir++;
        if (genRandom(10) > 5) dir--;
        if (genRandom(12) == 0) dir = lastDir;
        
        dir = dir & 7;
        x += dirTable[0][dir];
        y += dirTable[1][dir];
    }
}

/* Generate island map */
static void generateIsland(void) {
    int x, y;
    
    addGameLog("Generating island map");
    
    /* Fill edges with water */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            setMapTile(x, y, RIVER_TILE, 0, TILE_SET_REPLACE, "generateIsland");
        }
    }
    
    /* Clear center area to land */
    for (y = 5; y < WORLD_Y-5; y++) {
        for (x = 5; x < WORLD_X-5; x++) {
            setMapTile(x, y, DIRT_TILE, 0, TILE_SET_REPLACE, "generateIsland");
        }
    }
    
    /* Add irregular coastline */
    for (x = 0; x < WORLD_X-5; x += 2) {
        int y1 = genSkewedRandom(18);
        int y2 = 90 - genSkewedRandom(18);
        
        placeRiverPattern(x, y1, 1);
        placeRiverPattern(x, y2, 1);
        placeRiverPattern(x, 0, 0);
        placeRiverPattern(x, 94, 0);
    }
    
    for (y = 0; y < WORLD_Y-5; y += 2) {
        int x1 = genSkewedRandom(18);
        int x2 = 110 - genSkewedRandom(18);
        
        placeRiverPattern(x1, y, 1);
        placeRiverPattern(x2, y, 1);
        placeRiverPattern(0, y, 0);
        placeRiverPattern(114, y, 0);
    }
}

/* Generate lakes */
static void generateLakes(int waterPercent) {
    int lakeCount, i, j;
    int x, y, lakeSize;
    
    addGameLog("Generating lakes with %d%% water coverage", waterPercent);
    
    lakeCount = (waterPercent * 15) / 100; /* Scale lake count based on water percentage */
    
    for (i = 0; i < lakeCount; i++) {
        x = genRandom(99) + 10;
        y = genRandom(80) + 10;
        lakeSize = genRandom(12) + 2;
        
        for (j = 0; j < lakeSize; j++) {
            int lakeX = x - 6 + genRandom(12);
            int lakeY = y - 6 + genRandom(12);
            
            if (lakeX >= 0 && lakeX < WORLD_X && lakeY >= 0 && lakeY < WORLD_Y) {
                if (genRandom(4)) {
                    placeRiverPattern(lakeX, lakeY, 0);
                } else {
                    placeRiverPattern(lakeX, lakeY, 1);
                }
            }
        }
    }
}

/* Generate forests */
static void generateForests(int forestPercent) {
    int forestCount, i, j;
    int x, y, forestSize;
    static int dirTable[2][8] = {
        { 0, 1, 1, 1, 0, -1, -1, -1},
        {-1,-1, 0, 1, 1,  1,  0, -1}
    };
    
    addGameLog("Generating forests with %d%% forest coverage", forestPercent);
    
    forestCount = (forestPercent * 150) / 100; /* Scale forest count based on percentage */
    
    for (i = 0; i < forestCount; i++) {
        x = genRandom(119);
        y = genRandom(99);
        forestSize = genRandom(100 + (forestPercent * 2)) + 50;
        
        for (j = 0; j < forestSize; j++) {
            int dir = genRandom(7);
            
            x += dirTable[0][dir];
            y += dirTable[1][dir];
            
            if (x >= 0 && x < WORLD_X && y >= 0 && y < WORLD_Y) {
                int existing = getMapTile(x, y);
                if (existing == DIRT_TILE) {
                    setMapTile(x, y, WOODS_TILE, 0, TILE_SET_REPLACE, "generateForests");
                }
            }
        }
    }
}

/* Smooth river edges */
static void smoothRiverEdges(void) {
    static int dx[4] = {-1, 0, 1, 0};
    static int dy[4] = {0, 1, 0, -1};
    static int edgeTable[16] = {13, 13, 17, 15, 5, 2, 19, 17, 9, 11, 2, 13, 7, 9, 5, 2};
    int x, y, z, bitIndex, temp, nx, ny;
    
    addGameLog("Smoothing river edges");
    
    for (x = 0; x < WORLD_X; x++) {
        for (y = 0; y < WORLD_Y; y++) {
            if (getMapTile(x, y) == RIVER_EDGE_TILE) {
                bitIndex = 0;
                
                for (z = 0; z < 4; z++) {
                    bitIndex = bitIndex << 1;
                    nx = x + dx[z];
                    ny = y + dy[z];
                    
                    if (nx >= 0 && nx < WORLD_X && ny >= 0 && ny < WORLD_Y) {
                        if (getMapTile(nx, ny) != DIRT_TILE) {
                            bitIndex++;
                        }
                    }
                }
                
                temp = edgeTable[bitIndex & 15];
                if (temp != 2 && genRandom(1)) {
                    temp++;
                }
                
                setMapTile(x, y, temp, 0, TILE_SET_REPLACE, "smoothRiverEdges");
            }
        }
    }
}

/* Smooth forest edges */
static void smoothForestEdges(void) {
    static int dx[4] = {-1, 0, 1, 0};
    static int dy[4] = {0, 1, 0, -1};
    static int treeEdgeTable[16] = {0, 0, 0, 34, 0, 0, 36, 35, 0, 32, 0, 33, 30, 31, 29, 37};
    int x, y, z, bitIndex, temp, tile, nx, ny, neighborTile;
    
    addGameLog("Smoothing forest edges");
    
    for (x = 0; x < WORLD_X; x++) {
        for (y = 0; y < WORLD_Y; y++) {
            tile = getMapTile(x, y);
            
            if (tile == WOODS_TILE) {
                bitIndex = 0;
                
                for (z = 0; z < 4; z++) {
                    bitIndex = bitIndex << 1;
                    nx = x + dx[z];
                    ny = y + dy[z];
                    
                    if (nx >= 0 && nx < WORLD_X && ny >= 0 && ny < WORLD_Y) {
                        neighborTile = getMapTile(nx, ny);
                        if (neighborTile == WOODS_TILE) {
                            bitIndex++;
                        }
                    }
                }
                
                temp = treeEdgeTable[bitIndex & 15];
                if (temp != 0) {
                    if (temp != 37) {
                        if ((x + y) & 1) {
                            temp = temp - 8;
                        }
                    }
                    setMapTile(x, y, temp, 0, TILE_SET_REPLACE, "smoothForestEdges");
                }
            }
        }
    }
}

/* Generate terrain map based on parameters */
int generateTerrainMap(MapGenParams *params) {
    if (!params) {
        addGameLog("ERROR: generateTerrainMap called with NULL params");
        return 0;
    }
    
    currentParams = *params;
    
    addGameLog("Generating terrain map: Type=%s, Water=%d%%, Forest=%d%%",
              (params->mapType == MAPTYPE_RIVERS) ? "Rivers" : "Island",
              params->waterPercent, params->forestPercent);
    
    /* Initialize random number generator */
    initRandom();
    
    /* Clear map to dirt */
    clearMapToDirt();
    
    /* Generate base terrain based on type */
    switch (params->mapType) {
    case MAPTYPE_RIVERS:
        generateRivers();
        break;
    case MAPTYPE_ISLAND:
        generateIsland();
        break;
    default:
        /* Random choice - 10% chance for island */
        if (genRandom(10) == 0) {
            generateIsland();
        } else {
            generateRivers();
        }
        break;
    }
    
    /* Add lakes based on water percentage */
    if (params->waterPercent > 0) {
        generateLakes(params->waterPercent);
    }
    
    /* Add forests based on forest percentage */
    if (params->forestPercent > 0) {
        generateForests(params->forestPercent);
    }
    
    /* Smooth edges for natural appearance */
    smoothRiverEdges();
    smoothForestEdges();
    smoothForestEdges(); /* Run twice for better smoothing */
    
    addGameLog("Terrain generation completed successfully");
    return 1;
}

/* Generate small preview bitmap for map */
int generateMapPreview(MapGenParams *params, HBITMAP *previewBitmap, int width, int height) {
    HDC hdc, memDC;
    HBITMAP oldBitmap;
    int x, y, mapX, mapY, tile;
    int actualWidth, actualHeight, offsetX, offsetY;
    float aspectRatio, widthRatio, heightRatio;
    COLORREF tileColor;
    HBRUSH bgBrush;
    
    if (!params || !previewBitmap) {
        return 0;
    }
    
    /* Calculate proper aspect ratio (WORLD_X:WORLD_Y = 120:100 = 1.2:1) */
    aspectRatio = (float)WORLD_X / (float)WORLD_Y;
    widthRatio = (float)width / aspectRatio;
    heightRatio = (float)height * aspectRatio;
    
    if (widthRatio <= height) {
        /* Width constrains, center vertically */
        actualWidth = width;
        actualHeight = (int)widthRatio;
    } else {
        /* Height constrains, center horizontally */
        actualWidth = (int)heightRatio;
        actualHeight = height;
    }
    
    addGameLog("Generating map preview %dx%d (actual: %dx%d)", width, height, actualWidth, actualHeight);
    
    /* Create temporary terrain map */
    if (!generateTerrainMap(params)) {
        return 0;
    }
    
    /* Create preview bitmap */
    hdc = GetDC(NULL);
    memDC = CreateCompatibleDC(hdc);
    *previewBitmap = CreateCompatibleBitmap(hdc, width, height);
    oldBitmap = SelectObject(memDC, *previewBitmap);
    
    /* Fill background with gray */
    bgBrush = CreateSolidBrush(RGB(192, 192, 192));
    SelectObject(memDC, bgBrush);
    PatBlt(memDC, 0, 0, width, height, PATCOPY);
    DeleteObject(bgBrush);
    
    /* Calculate centering offsets */
    offsetX = (width - actualWidth) / 2;
    offsetY = (height - actualHeight) / 2;
    
    /* Draw preview centered with proper aspect ratio */
    for (y = 0; y < actualHeight; y++) {
        for (x = 0; x < actualWidth; x++) {
            /* Map preview coordinates to world coordinates */
            mapX = (x * WORLD_X) / actualWidth;
            mapY = (y * WORLD_Y) / actualHeight;
            
            tile = getMapTile(mapX, mapY);
            
            /* Convert tile to color */
            switch (tile) {
            case DIRT_TILE:
                tileColor = RGB(139, 69, 19); /* Brown */
                break;
            case RIVER_TILE:
            case CHANNEL_TILE:
                tileColor = RGB(0, 0, 255); /* Blue */
                break;
            case RIVER_EDGE_TILE:
                tileColor = RGB(0, 100, 200); /* Light blue */
                break;
            case WOODS_TILE:
                tileColor = RGB(0, 128, 0); /* Green */
                break;
            default:
                if (tile >= 29 && tile <= 37) {
                    tileColor = RGB(0, 100, 0); /* Dark green for tree variants */
                } else if (tile >= 2 && tile <= 20) {
                    tileColor = RGB(0, 0, 200); /* Blue for water variants */
                } else {
                    tileColor = RGB(139, 69, 19); /* Brown for everything else */
                }
                break;
            }
            
            SetPixel(memDC, x + offsetX, y + offsetY, tileColor);
        }
    }
    
    /* Clean up */
    SelectObject(memDC, oldBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdc);
    
    addGameLog("Map preview generated successfully");
    return 1;
}

/* Initialize map generation parameters with defaults */
void initMapGenParams(MapGenParams *params) {
    if (!params) return;
    
    params->mapType = MAPTYPE_RIVERS;
    params->waterPercent = 25;
    params->forestPercent = 30;
}