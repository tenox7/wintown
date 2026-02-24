/* tools.c - Tool handling code for WiNTown (Windows NT version)
 * Based on original WiNTown code from WiNTownLegacy project
 */

#include "tools.h"
#include "sim.h"
#include "tiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "gdifix.h"
#include "arch.h"
#include "assets.h"

/* External reference to the toolbar width */
extern int toolbarWidth;

/* External reference to RCI values */
extern short RValve, CValve, IValve;

/* CPU detection variables */
static char cpuArchStr[64] = "Unknown";
static HBITMAP hCpuBitmap = NULL;

/* CPU detection function using arch.h */
void DetectCPUType(void) {
    SYSTEM_INFO sysInfo;
    int resourceId;
    char bitmapName[32];
    extern int findToolIconResourceByName(const char* toolName);
    extern HBITMAP loadTilesetFromResource(int resourceId);
    
    GetSystemInfo(&sysInfo);
    
    /* Use ProcessorArchitectureNames array from arch.h for CPU name */
    if (sysInfo.wProcessorArchitecture < 15) {
        strcpy(cpuArchStr, ProcessorArchitectureNames[sysInfo.wProcessorArchitecture]);
    } else {
        strcpy(cpuArchStr, PROCESSOR_ARCHITECTURE_STR_UNKNOWN);
    }
    
    /* Map to bitmap filename */
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL:
            strcpy(bitmapName, "x86");
            break;
        case PROCESSOR_ARCHITECTURE_MIPS:
            strcpy(bitmapName, "mips");
            break;
        case PROCESSOR_ARCHITECTURE_ALPHA:
            strcpy(bitmapName, "axp");
            break;
        case PROCESSOR_ARCHITECTURE_PPC:
            strcpy(bitmapName, "ppc");
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            strcpy(bitmapName, "arm");
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            strcpy(bitmapName, "ia64");
            break;
        case PROCESSOR_ARCHITECTURE_ALPHA64:
            strcpy(bitmapName, "axp64");
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            strcpy(bitmapName, "x64");
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            strcpy(bitmapName, "arm64");
            break;
        default:
            strcpy(bitmapName, "x86");
            break;
    }
    
    resourceId = findToolIconResourceByName(bitmapName);
    if (resourceId != 0) {
        hCpuBitmap = loadTilesetFromResource(resourceId);
    }
}

/* Logging functions */
extern void addGameLog(const char *format, ...);

/* Asset functions removed to avoid conflicts */

/* Constants for boolean values */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* External reference to main window handle */
extern HWND hwndMain;

/* External reference to Map array */
extern short Map[WORLD_Y][WORLD_X];

/* 
 * Tile connection tables for road, rail, and wire
 * Index is a 4-bit mask representing connections:
 * Bit 0 (value 1): North connection
 * Bit 1 (value 2): East connection
 * Bit 2 (value 4): South connection
 * Bit 3 (value 8): West connection
 */
/* Original WiNTown RoadTable - exact mapping from w_con.c */
static short RoadTable[16] = {
    66, 67, 66, 68,    /* 0000, 0001, 0010, 0011 */
    67, 67, 69, 73,    /* 0100, 0101, 0110, 0111 */
    66, 71, 66, 72,    /* 1000, 1001, 1010, 1011 */
    70, 75, 74, 76     /* 1100, 1101, 1110, 1111 */
};

/* WiNTownJS RailTable exact mapping */
static short RailTable[16] = {
    LHRAIL,       /* 0000 - No connections */
    LVRAIL,       /* 0001 - North only */
    LHRAIL,       /* 0010 - East only */
    LVRAIL2,      /* 0011 - North, East */
    LVRAIL,       /* 0100 - South only */
    LVRAIL,       /* 0101 - North, South */
    LVRAIL3,      /* 0110 - East, South */
    LVRAIL7,      /* 0111 - North, East, South */
    LHRAIL,       /* 1000 - West only */
    LVRAIL5,      /* 1001 - North, West */
    LHRAIL,       /* 1010 - East, West */
    LVRAIL6,      /* 1011 - North, East, West */
    LVRAIL4,      /* 1100 - South, West */
    LVRAIL9,      /* 1101 - North, South, West */
    LVRAIL8,      /* 1110 - East, South, West */
    LVRAIL10      /* 1111 - All connections */
};

/* WiNTownJS WireTable exact mapping */
static short WireTable[16] = {
    LHPOWER,      /* 0000 - No connections */
    LVPOWER,      /* 0001 - North only */
    LHPOWER,      /* 0010 - East only */
    LVPOWER2,     /* 0011 - North, East */
    LVPOWER,      /* 0100 - South only */
    LVPOWER,      /* 0101 - North, South */
    LVPOWER3,     /* 0110 - East, South */
    LVPOWER7,     /* 0111 - North, East, South */
    LHPOWER,      /* 1000 - West only */
    LVPOWER5,     /* 1001 - North, West */
    LHPOWER,      /* 1010 - East, West */
    LVPOWER6,     /* 1011 - North, East, West */
    LVPOWER4,     /* 1100 - South, West */
    LVPOWER9,     /* 1101 - North, South, West */
    LVPOWER8,     /* 1110 - East, South, West */
    LVPOWER10     /* 1111 - All connections */
};

/* Tool cost constants */
/* Tool cost constants are now defined in tools.h */

/* Road and bridge cost constants */
#define ROAD_COST 10
#define BRIDGE_COST 50
#define RAIL_COST 20
#define TUNNEL_COST 100
#define WIRE_COST 5
#define UNDERWATER_WIRE_COST 25

/* Tool result constants */
#define TOOLRESULT_OK 0
#define TOOLRESULT_FAILED 1
#define TOOLRESULT_NO_MONEY 2
#define TOOLRESULT_NEED_BULLDOZE 3

/* Constants needed for tools.c */
#define TILE_SIZE 16  /* Size of each tile in pixels */
#define LASTTILE 960  /* Last possible tile value */

/* Special tiles not in simulation.h */
#define HANDBALL 5    /* Bridge helper tile */
#define LHBALL 6      /* Bridge helper tile */

/* Building and zone constants with TILE_ prefix for consistent naming */
#define TILE_WOODS WOODS
#define TILE_FIRESTBASE FIRESTBASE
#define TILE_FIRESTATION FIRESTATION
#define TILE_POLICESTBASE POLICESTBASE
#define TILE_POLICESTATION POLICESTATION
#define TILE_COALBASE COALBASE
#define TILE_POWERPLANT POWERPLANT
#define TILE_NUCLEARBASE NUCLEARBASE
#define TILE_NUCLEAR NUCLEAR
#define TILE_STADIUMBASE STADIUMBASE
#define TILE_STADIUM STADIUM
#define TILE_PORTBASE PORTBASE
#define TILE_PORT PORT
#define TILE_AIRPORTBASE AIRPORTBASE
#define TILE_AIRPORT AIRPORT

/* Using constants from simulation.h for zone ranges */
#define LASTAIRPORT LASTPORT     /* Last airport tile */
#define LASTFIRESTATION POLICESTBASE - 1 /* Last fire station tile */
#define LASTPOLICESTATION STADIUMBASE - 1 /* Last police station tile */
#define LASTNUCLEAR LASTZONE     /* Last nuclear plant tile */
#define LASTSTADIUM FULLSTADIUM  /* Last stadium tile */

/* Forward declarations for tile connection functions */
int ConnectTile(int x, int y, short *tilePtr, int command);
int LayDoze(int x, int y, short *tilePtr);
int LayRoad(int x, int y, short *tilePtr);
int LayRail(int x, int y, short *tilePtr);
int LayWire(int x, int y, short *tilePtr);
void FixZone(int x, int y, short *tilePtr);
void FixSingle(int x, int y);
short NormalizeRoad(short tile);

/* Zone deletion functions */
int checkSize(short tileValue);
int checkBigZone(short tileValue, int *deltaHPtr, int *deltaVPtr);
void put3x3Rubble(int x, int y);
void put4x4Rubble(int x, int y);
void put6x6Rubble(int x, int y);

/* Main connection function used for roads, rails, and wires */
int ConnectTile(int x, int y, short *tilePtr, int command) {
    short tile;
    int result = 1;

    /* Make sure the array subscripts are in bounds */
    if (!TestBounds(x, y)) {
        return 0;
    }

    /* AutoBulldoze feature - if trying to build road, rail, or wire */
    if (command >= 2 && command <= 4) {
        /* Check if we have funds and if the tile can be bulldozed */
        if (TotalFunds > 0 && ((tile = (*tilePtr)) & BULLBIT)) {
            /* Can bulldoze small objects and rubble */
            tile &= LOMASK;
            if (tile >= RUBBLE && tile <= LASTRUBBLE) {
                Spend(1);
                *tilePtr = DIRT;
            }
        }
    }

    /* Execute the appropriate command and fix zone */
    switch (command) {
    case 0: /* Fix zone */
        FixZone(x, y, tilePtr);
        break;

    case 1: /* Bulldoze */
        result = LayDoze(x, y, tilePtr);
        FixZone(x, y, tilePtr);
        break;

    case 2: /* Lay Road */
        result = LayRoad(x, y, tilePtr);
        FixZone(x, y, tilePtr);
        break;

    case 3: /* Lay Rail */
        result = LayRail(x, y, tilePtr);
        FixZone(x, y, tilePtr);
        break;

    case 4: /* Lay Wire */
        result = LayWire(x, y, tilePtr);
        FixZone(x, y, tilePtr);
        break;
    }

    return result;
}

/* Check size of a zone centered at this tile */
int checkSize(short tileValue) {
    /* Check for the normal com, resl, ind 3x3 zones & the fireDept & PoliceDept */
    if ((tileValue >= (RESBASE - 1) && tileValue <= (PORTBASE - 1)) ||
        (tileValue >= (LASTPOWERPLANT + 1) && tileValue <= (POLICESTATION + 4))) {
        return 3;
    }

    /* 4x4 zone buildings */
    if ((tileValue >= PORTBASE && tileValue <= LASTPORT) ||
        (tileValue >= COALBASE && tileValue <= LASTPOWERPLANT) ||
        (tileValue >= STADIUMBASE && tileValue <= LASTSTADIUM)) {
        return 4;
    }

    /* 6x6 zone buildings (airport) */
    if (tileValue >= AIRPORTBASE && tileValue <= LASTAIRPORT) {
        return 6;
    }

    return 0;
}

/* Check if this tile is part of a big zone and find its center */
int checkBigZone(short id, int *deltaHPtr, int *deltaVPtr) {
    /* Handle zone center tiles */
    switch (id) {
    case 750: /* POWERPLANT / TILE_POWERPLANT */
    case 698: /* PORT / TILE_PORT */
    case 816: /* NUCLEAR / TILE_NUCLEAR */
    case 785: /* STADIUM / TILE_STADIUM */
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;

    case 716: /* AIRPORT / TILE_AIRPORT */
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 6;
    }

    /* Coal power plant parts (745-754) */
    if (id == TILE_COALBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_COALBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_COALBASE + 3) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_COALBASE + 4) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_COALBASE + 5) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_COALBASE + 6) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_COALBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_COALBASE + 8) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 4;
    }
    /* Nuclear power plant parts (811-826) */
    else if (id == TILE_NUCLEARBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 3) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 4) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 5) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 6) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_NUCLEARBASE + 8) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 4;
    }
    /* Stadium parts (779-799) */
    else if (id == TILE_STADIUMBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_STADIUMBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 3) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 4) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_STADIUMBASE + 5) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 6) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_STADIUMBASE + 8) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 4;
    }
    /* Seaport parts (693-708) */
    else if (id == TILE_PORTBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_PORTBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_PORTBASE + 3) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_PORTBASE + 4) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_PORTBASE + 5) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 4;
    } else if (id == TILE_PORTBASE + 6) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_PORTBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 4;
    } else if (id == TILE_PORTBASE + 8) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 4;
    }
    /* Airport parts (709-744) */
    else if (id == TILE_AIRPORTBASE) {
        *deltaHPtr = -2;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 1) {
        *deltaHPtr = -1;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 2) {
        *deltaHPtr = 0;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 3) {
        *deltaHPtr = 1;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 4) {
        *deltaHPtr = 2;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 5) {
        *deltaHPtr = 3;
        *deltaVPtr = -2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 6) {
        *deltaHPtr = -2;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 7) {
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 8) {
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 9) {
        *deltaHPtr = 1;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 10) {
        *deltaHPtr = 2;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 11) {
        *deltaHPtr = 3;
        *deltaVPtr = -1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 12) {
        *deltaHPtr = -2;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 13) {
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 14) {
        *deltaHPtr = 1;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 15) {
        *deltaHPtr = 2;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 16) {
        *deltaHPtr = 3;
        *deltaVPtr = 0;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 17) {
        *deltaHPtr = -2;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 18) {
        *deltaHPtr = -1;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 19) {
        *deltaHPtr = 0;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 20) {
        *deltaHPtr = 1;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 21) {
        *deltaHPtr = 2;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 22) {
        *deltaHPtr = 3;
        *deltaVPtr = 1;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 23) {
        *deltaHPtr = -2;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 24) {
        *deltaHPtr = -1;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 25) {
        *deltaHPtr = 0;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 26) {
        *deltaHPtr = 1;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 27) {
        *deltaHPtr = 2;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 28) {
        *deltaHPtr = 3;
        *deltaVPtr = 2;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 29) {
        *deltaHPtr = -2;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 30) {
        *deltaHPtr = -1;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 31) {
        *deltaHPtr = 0;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 32) {
        *deltaHPtr = 1;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 33) {
        *deltaHPtr = 2;
        *deltaVPtr = 3;
        return 6;
    } else if (id == TILE_AIRPORTBASE + 34) {
        *deltaHPtr = 3;
        *deltaVPtr = 3;
        return 6;
    }
    /* Also handle base tiles */
    else if (id == TILE_COALBASE) {
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_NUCLEARBASE) {
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_STADIUMBASE) {
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;
    } else if (id == TILE_PORTBASE) {
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return 4;
    }

    /* Not found */
    return 0;
}

/* Create 3x3 rubble */
void put3x3Rubble(int x, int y) {
    int xx, yy;
    short zz;

    /* First pass - clear all zone bits to avoid confusion during bulldozing */
    for (yy = y - 1; yy <= y + 1; yy++) {
        for (xx = x - 1; xx <= x + 1; xx++) {
            if (TestBounds(xx, yy)) {
                /* Clear the zone bit but preserve other flags */
                SetTileZone(xx, yy, Map[yy][xx] & LOMASK, 0);
            }
        }
    }

    /* Second pass - create the rubble */
    for (yy = y - 1; yy <= y + 1; yy++) {
        for (xx = x - 1; xx <= x + 1; xx++) {
            if (TestBounds(xx, yy)) {
                zz = Map[yy][xx] & LOMASK;
                if ((zz != RADTILE) && (zz != 0)) {
                    setMapTile(xx, yy, SOMETINYEXP, ANIMBIT | BULLBIT, TILE_SET_REPLACE, "put3x3Rubble-explode");
                }
            }
        }
    }
}

/* Create 4x4 rubble */
void put4x4Rubble(int x, int y) {
    int xx, yy;
    short zz;

    /* First pass - clear all zone bits to avoid confusion during bulldozing */
    for (yy = y - 1; yy <= y + 2; yy++) {
        for (xx = x - 1; xx <= x + 2; xx++) {
            if (TestBounds(xx, yy)) {
                /* Clear the zone bit but preserve other flags */
                SetTileZone(xx, yy, Map[yy][xx] & LOMASK, 0);
            }
        }
    }

    /* Second pass - create the rubble */
    for (yy = y - 1; yy <= y + 2; yy++) {
        for (xx = x - 1; xx <= x + 2; xx++) {
            if (TestBounds(xx, yy)) {
                zz = Map[yy][xx] & LOMASK;
                if ((zz != RADTILE) && (zz != 0)) {
                    setMapTile(xx, yy, SOMETINYEXP, ANIMBIT | BULLBIT, TILE_SET_REPLACE, "put4x4Rubble-explode");
                }
            }
        }
    }
}

/* Create 6x6 rubble */
void put6x6Rubble(int x, int y) {
    int xx, yy;
    short zz;

    /* First pass - clear all zone bits to avoid confusion during bulldozing */
    for (yy = y - 2; yy <= y + 3; yy++) {
        for (xx = x - 2; xx <= x + 3; xx++) {
            if (TestBounds(xx, yy)) {
                /* Clear the zone bit but preserve other flags */
                SetTileZone(xx, yy, Map[yy][xx] & LOMASK, 0);
            }
        }
    }

    /* Second pass - create the rubble */
    for (yy = y - 2; yy <= y + 3; yy++) {
        for (xx = x - 2; xx <= x + 3; xx++) {
            if (TestBounds(xx, yy)) {
                zz = Map[yy][xx] & LOMASK;
                if ((zz != RADTILE) && (zz != 0)) {
                    setMapTile(xx, yy, SOMETINYEXP, ANIMBIT | BULLBIT, TILE_SET_REPLACE, "put6x6Rubble-explode");
                }
            }
        }
    }
}

/* Bulldoze a tile */
int LayDoze(int x, int y, short *tilePtr) {
    short tile;
    int zoneSize = 0;
    int deltaH = 0;
    int deltaV = 0;

    /* If not in bounds, we cannot bulldoze */
    if (!TestBounds(x, y)) {
        return 0;
    }

    tile = *tilePtr & LOMASK;

    /* Empty land - nothing to bulldoze, return success */
    if (tile == DIRT) {
        return 1;
    }

    /* If funds are too low (we need at least $1) */
    if (TotalFunds < 1) {
        return 0;
    }

    /* Special case for water-related structures which cost more */
    if ((tile == HANDBALL || tile == LHBALL || tile == HBRIDGE || tile == VBRIDGE || tile == BRWH ||
         tile == BRWV) &&
        TotalFunds < 5) {
        return 0;
    }

    /* If this is a zone center, handle it specially */
    if (*tilePtr & ZONEBIT) {
        /* Get the zone size based on the tile value */
        zoneSize = checkSize(tile);

        switch (zoneSize) {
        case 3:
            /* Small 3x3 zone */
            Spend(1);
            put3x3Rubble(x, y);
            return 1;

        case 4:
            /* Medium 4x4 zone */
            Spend(1);
            put4x4Rubble(x, y);
            return 1;

        case 6:
            /* Large 6x6 zone (airport) */
            Spend(1);
            put6x6Rubble(x, y);
            return 1;

        default:
            /* Unknown zone type - convert to rubble instead of just clearing */
            Spend(1);
            *tilePtr = RUBBLE | BULLBIT;
            return 1;
        }
    }
    /* If not a zone center, check if it's part of a big zone */
    else if ((zoneSize = checkBigZone(tile, &deltaH, &deltaV))) {
        /* Adjust coordinates to the zone center */
        int centerX = x + deltaH;
        int centerY = y + deltaV;

        /* Verify new coords are in bounds */
        if (TestBounds(centerX, centerY)) {
            switch (zoneSize) {
            case 3:
                Spend(1);
                put3x3Rubble(centerX, centerY);
                return 1;

            case 4:
                Spend(1);
                put4x4Rubble(centerX, centerY);
                return 1;

            case 6:
                Spend(1);
                put6x6Rubble(centerX, centerY);
                return 1;

            default:
                /* For unknown zone size types, just bulldoze normally */
                break;
            }
        }
    }

    /* Can't bulldoze lakes, rivers, radiated tiles */
    if ((tile == RIVER) || (tile == REDGE) || (tile == CHANNEL) || (tile == RADTILE)) {
        return 0;
    }

    /* Bulldoze water-related structures back to water */
    if (tile == HANDBALL || tile == LHBALL || tile == HBRIDGE || tile == VBRIDGE || tile == BRWH ||
        tile == BRWV) {
        Spend(5);
        *tilePtr = RIVER;
        return 1;
    }

    /* General bulldozing of other tiles */
    Spend(1);
    *tilePtr = DIRT;
    return 1;
}

/* Lay road */
int LayRoad(int x, int y, short *tilePtr) {
    short cost;
    short tile = *tilePtr & LOMASK;

    if (tile == RIVER || tile == REDGE || tile == CHANNEL) {
        /* Build a bridge over water */
        cost = BRIDGE_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        if (((y > 0) && (Map[y - 1][x] & LOMASK) == VRAIL) ||
            ((y < WORLD_Y - 1) && (Map[y + 1][x] & LOMASK) == VRAIL)) {
            *tilePtr = VRAILROAD | BULLBIT;
        } else {
            *tilePtr = HBRIDGE | BULLBIT;
        }
        return 1;
    }
    
    /* Handle crossing a power line */
    if (tile >= POWERBASE && (tile <= LASTPOWER)) {
        cost = ROAD_COST;
        if (TotalFunds < cost) {
            return 0;
        }
        Spend(cost);
        /* Use the built-in road/power crossing tiles */
        if (y > 0 && y < WORLD_Y - 1 && 
            (Map[y-1][x] & LOMASK) >= POWERBASE && (Map[y-1][x] & LOMASK) <= LASTPOWER &&
            (Map[y+1][x] & LOMASK) >= POWERBASE && (Map[y+1][x] & LOMASK) <= LASTPOWER) {
            /* Vertical power line needs horizontal road crossing */
            *tilePtr = HROADPOWER | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Horizontal power line needs vertical road crossing */
            *tilePtr = VROADPOWER | CONDBIT | BULLBIT | BURNBIT;
        }
        return 1;
    }

    if (tile == DIRT || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
        /* Pave road on dirt */
        cost = ROAD_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        *tilePtr = ROADS | BULLBIT | BURNBIT;
        return 1;
    }

    /* Cannot build road on radioactive tiles or other pre-existing buildings */
    return 0;
}

/* Lay rail */
int LayRail(int x, int y, short *tilePtr) {
    short cost;
    short tile = *tilePtr & LOMASK;

    if (tile == RIVER || tile == REDGE || tile == CHANNEL) {
        /* Build a rail tunnel over water */
        cost = TUNNEL_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        *tilePtr = HRAIL | BULLBIT;
        return 1;
    }
    
    /* Handle crossing a power line */
    if (tile >= POWERBASE && (tile <= LASTPOWER)) {
        cost = RAIL_COST;
        if (TotalFunds < cost) {
            return 0;
        }
        Spend(cost);
        /* Use the built-in rail/power crossing tiles */
        if (y > 0 && y < WORLD_Y - 1 && 
            (Map[y-1][x] & LOMASK) >= POWERBASE && (Map[y-1][x] & LOMASK) <= LASTPOWER &&
            (Map[y+1][x] & LOMASK) >= POWERBASE && (Map[y+1][x] & LOMASK) <= LASTPOWER) {
            /* Vertical power line needs horizontal rail crossing */
            *tilePtr = RAILHPOWERV | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Horizontal power line needs vertical rail crossing */
            *tilePtr = RAILVPOWERH | CONDBIT | BULLBIT | BURNBIT;
        }
        return 1;
    }

    if (tile == DIRT || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
        /* Lay rail on dirt */
        cost = RAIL_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        *tilePtr = RAILBASE | BULLBIT | BURNBIT;
        return 1;
    }

    /* Cannot build rail on radioactive tiles or other pre-existing buildings */
    return 0;
}

/* Lay power lines */
int LayWire(int x, int y, short *tilePtr) {
    short cost;
    short tile;
    short connectMask;
    
    tile = *tilePtr & LOMASK;
    connectMask = 0;

    if (tile == RIVER || tile == REDGE || tile == CHANNEL) {
        /* Build underwater power lines */
        cost = UNDERWATER_WIRE_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        
        /* Build the connection mask manually for underwater power lines */
        /* Check North */
        if (y > 0 && ((Map[y-1][x] & CONDBIT) || 
            ((Map[y-1][x] & LOMASK) >= POWERBASE && (Map[y-1][x] & LOMASK) <= LASTPOWER))) {
            connectMask |= 1;
        }
        
        /* Check East */
        if (x < WORLD_X - 1 && ((Map[y][x+1] & CONDBIT) || 
            ((Map[y][x+1] & LOMASK) >= POWERBASE && (Map[y][x+1] & LOMASK) <= LASTPOWER))) {
            connectMask |= 2;
        }
        
        /* Check South */
        if (y < WORLD_Y - 1 && ((Map[y+1][x] & CONDBIT) || 
            ((Map[y+1][x] & LOMASK) >= POWERBASE && (Map[y+1][x] & LOMASK) <= LASTPOWER))) {
            connectMask |= 4;
        }
        
        /* Check West */
        if (x > 0 && ((Map[y][x-1] & CONDBIT) || 
            ((Map[y][x-1] & LOMASK) >= POWERBASE && (Map[y][x-1] & LOMASK) <= LASTPOWER))) {
            connectMask |= 8;
        }

        if (connectMask != 0) {
            /* Use the proper tile from the wire table */
            *tilePtr = WireTable[connectMask & 15] | CONDBIT | BULLBIT;
        } else if ((x > 0 && x < WORLD_X - 1) && 
                  (Map[y][x-1] & CONDBIT) && (Map[y][x+1] & CONDBIT)) {
            /* Horizontal connection needed */
            *tilePtr = HPOWER | CONDBIT | BULLBIT;
        } else {
            /* Default to vertical power line for underwater */
            *tilePtr = VPOWER | CONDBIT | BULLBIT; 
        }
        return 1;
    }

    /* Handle crossing a road */
    if (tile >= ROADBASE && tile <= LASTROAD) {
        cost = WIRE_COST;
        if (TotalFunds < cost) {
            return 0;
        }
        Spend(cost);
        /* Use the built-in road/power crossing tiles */
        if (x > 0 && x < WORLD_X - 1 && 
            (Map[y][x-1] & LOMASK) >= ROADBASE && (Map[y][x-1] & LOMASK) <= LASTROAD &&
            (Map[y][x+1] & LOMASK) >= ROADBASE && (Map[y][x+1] & LOMASK) <= LASTROAD) {
            /* Horizontal road needs vertical power line crossing */
            *tilePtr = VROADPOWER | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Vertical road needs horizontal power line crossing */
            *tilePtr = HROADPOWER | CONDBIT | BULLBIT | BURNBIT;
        }
        return 1;
    }
    
    /* Handle crossing a rail */
    if (tile >= RAILBASE && tile <= LASTRAIL) {
        cost = WIRE_COST;
        if (TotalFunds < cost) {
            return 0;
        }
        Spend(cost);
        /* Use the built-in rail/power crossing tiles */
        if (x > 0 && x < WORLD_X - 1 && 
            (Map[y][x-1] & LOMASK) >= RAILBASE && (Map[y][x-1] & LOMASK) <= LASTRAIL &&
            (Map[y][x+1] & LOMASK) >= RAILBASE && (Map[y][x+1] & LOMASK) <= LASTRAIL) {
            /* Horizontal rail needs vertical power line crossing */
            *tilePtr = RAILVPOWERH | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Vertical rail needs horizontal power line crossing */
            *tilePtr = RAILHPOWERV | CONDBIT | BULLBIT | BURNBIT;
        }
        return 1;
    }

    if (tile == DIRT || (tile >= TINYEXP && tile <= LASTTINYEXP)) {
        /* Lay wire on dirt */
        short connectMask;
        
        cost = WIRE_COST;

        if (TotalFunds < cost) {
            return 0;
        }

        Spend(cost);
        
        /* Build a connection mask to determine the proper wire tile */
        connectMask = 0;
        
        /* Check North */
        if (y > 0 && ((Map[y-1][x] & CONDBIT) || 
            ((Map[y-1][x] & LOMASK) >= POWERBASE && (Map[y-1][x] & LOMASK) <= LASTPOWER))) {
            connectMask |= 1;
        }
        
        /* Check East */
        if (x < WORLD_X - 1 && ((Map[y][x+1] & CONDBIT) || 
            ((Map[y][x+1] & LOMASK) >= POWERBASE && (Map[y][x+1] & LOMASK) <= LASTPOWER))) {
            connectMask |= 2;
        }
        
        /* Check South */
        if (y < WORLD_Y - 1 && ((Map[y+1][x] & CONDBIT) || 
            ((Map[y+1][x] & LOMASK) >= POWERBASE && (Map[y+1][x] & LOMASK) <= LASTPOWER))) {
            connectMask |= 4;
        }
        
        /* Check West */
        if (x > 0 && ((Map[y][x-1] & CONDBIT) || 
            ((Map[y][x-1] & LOMASK) >= POWERBASE && (Map[y][x-1] & LOMASK) <= LASTPOWER))) {
            connectMask |= 8;
        }
        
        /* If we have connections, choose the appropriate power line tile */
        if (connectMask != 0) {
            /* Use the proper tile from the wire table */
            *tilePtr = WireTable[connectMask & 15] | CONDBIT | BULLBIT | BURNBIT;
        } else {
            /* Special case for vertical alignment - if this is a second vertical tile, 
               use VPOWER instead of LHPOWER to avoid the upside-down L issue */
            if (y > 0 && (Map[y-1][x] & LOMASK) == VPOWER) {
                *tilePtr = VPOWER | CONDBIT | BULLBIT | BURNBIT;
            } else if (y < WORLD_Y - 1 && (Map[y+1][x] & LOMASK) == VPOWER) {
                *tilePtr = VPOWER | CONDBIT | BULLBIT | BURNBIT;
            } else {
                /* Default to LHPOWER (horizontal power line) */
                *tilePtr = LHPOWER | CONDBIT | BULLBIT | BURNBIT;
            }
        }
        return 1;
    }

    /* Cannot build power lines on radioactive tiles or other pre-existing buildings */
    return 0;
}

/* Fix zone - update connections of surrounding tiles */
void FixZone(int x, int y, short *tilePtr) {
    /* Check that coordinates are valid */
    if (!TestBounds(x, y)) {
        return;
    }

    FixSingle(x, y);

    if (TestBounds(x - 1, y)) {
        FixSingle(x - 1, y);
    }

    if (TestBounds(x + 1, y)) {
        FixSingle(x + 1, y);
    }

    if (TestBounds(x, y - 1)) {
        FixSingle(x, y - 1);
    }

    if (TestBounds(x, y + 1)) {
        FixSingle(x, y + 1);
    }
}

/* NormalizeRoad function - standardizes road tile values for comparison 
 * This is equivalent to WiNTownJS's normalizeRoad function
 */
short NormalizeRoad(short tile) {
    if (tile == HROADPOWER || tile == VROADPOWER)
        return tile;
    if (tile >= ROADBASE && tile <= LASTROAD)
        return (tile & 15) + ROADBASE;
    return tile;
}

/* Fix a single tile - update its connections 
 * Closely follows WiNTownJS's fixSingle implementation
 */
void FixSingle(int x, int y) {
    short tile;
    short adjTile = 0; /* Adjacency bitmask */
    short mapValue;
    /* Raw and normalized neighbor values for road/wire logic */
    short rawNorth, normNorth;
    short rawEast, normEast;
    short rawSouth, normSouth;
    short rawWest, normWest;

    /* Verify the coordinates */
    if (!TestBounds(x, y)) {
        return;
    }

    tile = Map[y][x] & LOMASK;

    /* Skip some types of tiles */
    if (tile < 1 || tile >= LASTTILE) {
        return;
    }

    /* Normalize the current tile for comparison */
    tile = NormalizeRoad(tile);

    /* Check for road connections */
    if (tile >= ROADS && tile <= INTERSECTION) {
        /* North: allow N-S roads (VROADPOWER, HRAILROAD), exclude HBRIDGE pattern */
        if (y > 0) {
            rawNorth = Map[y - 1][x] & LOMASK;
            normNorth = NormalizeRoad(rawNorth);
            if ((rawNorth == HRAILROAD) || (rawNorth == VROADPOWER) ||
                ((normNorth >= ROADBASE) && (normNorth <= ROADBASE + 14) &&
                 (normNorth != ROADBASE))) {
                adjTile |= 1;
            }
        }

        /* East: allow E-W roads (HROADPOWER, VRAILROAD), exclude VBRIDGE pattern */
        if (x < WORLD_X - 1) {
            rawEast = Map[y][x + 1] & LOMASK;
            normEast = NormalizeRoad(rawEast);
            if ((rawEast == VRAILROAD) || (rawEast == HROADPOWER) ||
                ((normEast >= ROADBASE) && (normEast <= ROADBASE + 14) &&
                 (normEast != ROADBASE + 1))) {
                adjTile |= 2;
            }
        }

        /* South: same as north */
        if (y < WORLD_Y - 1) {
            rawSouth = Map[y + 1][x] & LOMASK;
            normSouth = NormalizeRoad(rawSouth);
            if ((rawSouth == HRAILROAD) || (rawSouth == VROADPOWER) ||
                ((normSouth >= ROADBASE) && (normSouth <= ROADBASE + 14) &&
                 (normSouth != ROADBASE))) {
                adjTile |= 4;
            }
        }

        /* West: same as east */
        if (x > 0) {
            rawWest = Map[y][x - 1] & LOMASK;
            normWest = NormalizeRoad(rawWest);
            if ((rawWest == VRAILROAD) || (rawWest == HROADPOWER) ||
                ((normWest >= ROADBASE) && (normWest <= ROADBASE + 14) &&
                 (normWest != ROADBASE + 1))) {
                adjTile |= 8;
            }
        }

        tile = RoadTable[adjTile];
        setMapTile(x, y, tile, BULLBIT | BURNBIT, TILE_SET_PRESERVE, "FixSingle-road");
        return;
    }

    /* Check for rail connections */
    if (tile >= LHRAIL && tile <= LVRAIL10) {
        /* Check the north side */
        if (y > 0) {
            mapValue = Map[y - 1][x] & LOMASK;
            mapValue = NormalizeRoad(mapValue);
            if (mapValue >= RAILHPOWERV && mapValue <= VRAILROAD &&
                mapValue != RAILHPOWERV && mapValue != HRAILROAD &&
                mapValue != HRAIL) {
                adjTile |= 1;  /* North connection */
            }
        }

        /* Check the east side */
        if (x < WORLD_X - 1) {
            mapValue = Map[y][x + 1] & LOMASK;
            mapValue = NormalizeRoad(mapValue);
            if (mapValue >= RAILHPOWERV && mapValue <= VRAILROAD &&
                mapValue != RAILVPOWERH && mapValue != VRAILROAD &&
                mapValue != VRAIL) {
                adjTile |= 2;  /* East connection */
            }
        }

        /* Check the south side */
        if (y < WORLD_Y - 1) {
            mapValue = Map[y + 1][x] & LOMASK;
            mapValue = NormalizeRoad(mapValue);
            if (mapValue >= RAILHPOWERV && mapValue <= VRAILROAD &&
                mapValue != RAILHPOWERV && mapValue != HRAILROAD &&
                mapValue != HRAIL) {
                adjTile |= 4;  /* South connection */
            }
        }

        /* Check the west side */
        if (x > 0) {
            mapValue = Map[y][x - 1] & LOMASK;
            mapValue = NormalizeRoad(mapValue);
            if (mapValue >= RAILHPOWERV && mapValue <= VRAILROAD &&
                mapValue != RAILVPOWERH && mapValue != VRAILROAD &&
                mapValue != VRAIL) {
                adjTile |= 8;  /* West connection */
            }
        }

        /* Update the rail tile with proper connections */
        tile = RailTable[adjTile];
        setMapTile(x, y, tile, BULLBIT | BURNBIT, TILE_SET_PRESERVE, "FixSingle-rail");
        return;
    }

    /* Check for wire connections */
    if (tile >= LHPOWER && tile <= LVPOWER10) {
        /* Check the north side */
        if (y > 0) {
            if ((Map[y - 1][x] & CONDBIT) != 0) {
                rawNorth = (Map[y - 1][x] & LOMASK);
                /* Treat any conductive neighbor except specific vertical crossings/power as a connection */
                if (rawNorth != VPOWER && rawNorth != VROADPOWER && rawNorth != RAILVPOWERH) {
                    adjTile |= 1;  /* North connection */
                }
            }
        }

        /* Check the east side */
        if (x < WORLD_X - 1) {
            if ((Map[y][x + 1] & CONDBIT) != 0) {
                rawEast = (Map[y][x + 1] & LOMASK);
                if (rawEast != HPOWER && rawEast != HROADPOWER && rawEast != RAILHPOWERV) {
                    adjTile |= 2;  /* East connection */
                }
            }
        }

        /* Check the south side */
        if (y < WORLD_Y - 1) {
            if ((Map[y + 1][x] & CONDBIT) != 0) {
                rawSouth = (Map[y + 1][x] & LOMASK);
                if (rawSouth != VPOWER && rawSouth != VROADPOWER && rawSouth != RAILVPOWERH) {
                    adjTile |= 4;  /* South connection */
                }
            }
        }

        /* Check the west side */
        if (x > 0) {
            if ((Map[y][x - 1] & CONDBIT) != 0) {
                rawWest = (Map[y][x - 1] & LOMASK);
                if (rawWest != HPOWER && rawWest != HROADPOWER && rawWest != RAILHPOWERV) {
                    adjTile |= 8;  /* West connection */
                }
            }
        }

        /* Update the wire tile with proper connections */
        tile = WireTable[adjTile];
        setMapTile(x, y, tile, BULLBIT | BURNBIT | CONDBIT, TILE_SET_PRESERVE, "FixSingle-wire");
        return;
    }
    
    /* Crossing tiles don't need self-fixing, they are set by placement code */
}

/* Toolbar state variables */
static int currentTool = bulldozerState;
static int toolResult = TOOLRESULT_OK;
static int toolCost = 0;

/* Last mouse position for hover effect */
static int lastMouseMapX = -1;
static int lastMouseMapY = -1;

/* Mapping following original Micropolis tool layout (16 tools: 0-15) - removed chalk/eraser */
static const int toolbarToStateMapping[16] = {
    residentialState, /* 0 - Residential Zone */
    commercialState,  /* 1 - Commercial Zone */
    industrialState,  /* 2 - Industrial Zone */
    fireState,        /* 3 - Fire Station */
    queryState,       /* 4 - Query Tool */
    policeState,      /* 5 - Police Station */
    wireState,        /* 6 - Power Lines */
    bulldozerState,   /* 7 - Bulldozer */
    railState,        /* 8 - Railway */
    roadState,        /* 9 - Roads */
    stadiumState,     /* 10 - Stadium */
    parkState,        /* 11 - Park */
    seaportState,     /* 12 - Seaport */
    powerState,       /* 13 - Coal Power Plant */
    nuclearState,     /* 14 - Nuclear Power Plant */
    airportState      /* 15 - Airport */
};

/* Reverse mapping from tool state to toolbar position for fast lookups */
static const int stateToToolbarMapping[19] = {
    0,  /* residentialState (0) -> position 0 */
    1,  /* commercialState (1) -> position 1 */
    2,  /* industrialState (2) -> position 2 */
    3,  /* fireState (3) -> position 3 */
    5,  /* policeState (4) -> position 5 */
    6,  /* wireState (5) -> position 6 */
    9,  /* roadState (6) -> position 9 */
    8,  /* railState (7) -> position 8 */
    11, /* parkState (8) -> position 11 */
    10, /* stadiumState (9) -> position 10 */
    12, /* seaportState (10) -> position 12 */
    13, /* powerState (11) -> position 13 */
    14, /* nuclearState (12) -> position 14 */
    15, /* airportState (13) -> position 15 */
    0,  /* networkState (14) - not used in toolbar */
    7,  /* bulldozerState (15) -> position 7 */
    4,  /* queryState (16) -> position 4 */
    0,  /* windowState (17) - not used in toolbar */
    0   /* noToolState (18) - not used in toolbar */
};

/* Tool active flag - needs to be exportable to main.c */
int isToolActive = 0; /* 0 = FALSE, 1 = TRUE */

/* Helper function to check 3x3 area for zone placement */
int Check3x3Area(int x, int y, int *cost) {
    int dx, dy;
    short tile;
    int clearCost = 0;

    /* Check bounds for a 3x3 zone */
    if (x < 1 || x >= WORLD_X - 1 || y < 1 || y >= WORLD_Y - 1) {
        return 0;
    }

    /* Check all tiles in the 3x3 area */
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            tile = Map[y + dy][x + dx] & LOMASK;

            /* Check if tile is clear or can be bulldozed */
            if (tile != DIRT) {
                /* Can be bulldozed at additional cost */
                if ((AutoBulldoze && tile >= TREEBASE && tile <= LASTRUBBLE) ||
                    (tile >= RUBBLE && tile <= LASTRUBBLE) ||
                    (tile >= TINYEXP && tile <= LASTTINYEXP)) {
                    clearCost += 1;
                } else {
                    /* Not buildable */
                    return 0;
                }
            }
        }
    }

    *cost = clearCost;
    return 1;
}

/* Helper function to check 4x4 area for zone placement */
int Check4x4Area(int x, int y, int *cost) {
    int dx, dy;
    short tile;
    int clearCost = 0;

    /* Check bounds for a 4x4 zone */
    if (x < 1 || x >= WORLD_X - 2 || y < 1 || y >= WORLD_Y - 2) {
        return 0;
    }

    /* Check all tiles in the 4x4 area */
    for (dy = -1; dy <= 2; dy++) {
        for (dx = -1; dx <= 2; dx++) {
            tile = Map[y + dy][x + dx] & LOMASK;

            /* Check if tile is clear or can be bulldozed */
            if (tile != DIRT) {
                /* Can be bulldozed at additional cost */
                if ((AutoBulldoze && tile >= TREEBASE && tile <= LASTRUBBLE) ||
                    (tile >= RUBBLE && tile <= LASTRUBBLE) ||
                    (tile >= TINYEXP && tile <= LASTTINYEXP)) {
                    clearCost += 1;
                } else {
                    /* Not buildable */
                    return 0;
                }
            }
        }
    }

    *cost = clearCost;
    return 1;
}

/* Helper function to check 6x6 area for zone placement */
int Check6x6Area(int x, int y, int *cost) {
    int dx, dy;
    short tile;
    int clearCost = 0;

    /* Check bounds for a 6x6 zone */
    if (x < 2 || x >= WORLD_X - 3 || y < 2 || y >= WORLD_Y - 3) {
        return 0;
    }

    /* Check all tiles in the 6x6 area */
    for (dy = -2; dy <= 3; dy++) {
        for (dx = -2; dx <= 3; dx++) {
            tile = Map[y + dy][x + dx] & LOMASK;

            /* Check if tile is clear or can be bulldozed */
            if (tile != DIRT) {
                /* Can be bulldozed at additional cost */
                if ((AutoBulldoze && tile >= TREEBASE && tile <= LASTRUBBLE) ||
                    (tile >= RUBBLE && tile <= LASTRUBBLE) ||
                    (tile >= TINYEXP && tile <= LASTTINYEXP)) {
                    clearCost += 1;
                } else {
                    /* Not buildable */
                    return 0;
                }
            }
        }
    }

    *cost = clearCost;
    return 1;
}

/* Toolbar button constants */
#define TB_BULLDOZER 100
#define TB_ROAD 101
#define TB_RAIL 102
#define TB_WIRE 103
#define TB_PARK 104
#define TB_RESIDENTIAL 105
#define TB_COMMERCIAL 106
#define TB_INDUSTRIAL 107
#define TB_FIRESTATION 108
#define TB_POLICESTATION 109
#define TB_STADIUM 110
#define TB_SEAPORT 111
#define TB_POWERPLANT 112
#define TB_NUCLEAR 113
#define TB_AIRPORT 114
#define TB_QUERY 115

static HWND hwndToolbar = NULL; /* Toolbar window handle */
static int toolbarWidth = 132;  /* Original Micropolis toolbar width */

/* Tool bitmap handles */
static HBITMAP hToolBitmaps[16]; /* Tool bitmaps */

/* Button position and size data for original Micropolis layout */
typedef struct {
    int x, y;        /* Button position */
    int width, height; /* Button size */
    int iconWidth, iconHeight; /* Icon size within button */
} ToolButtonLayout;

static const ToolButtonLayout toolLayout[16] = {
    {9, 58, 34, 50, 34, 50},    /* 0 - Residential Zone (tall) */
    {47, 58, 34, 50, 34, 50},   /* 1 - Commercial Zone (tall) */
    {85, 58, 34, 50, 34, 50},   /* 2 - Industrial Zone (tall) */
    {9, 112, 34, 34, 34, 34},   /* 3 - Fire Station (square) */
    {47, 112, 34, 34, 34, 34},  /* 4 - Query Tool (square) */
    {85, 112, 34, 34, 34, 34},  /* 5 - Police Station (square) */
    {9, 150, 34, 34, 34, 34},   /* 6 - Power Lines (square) */
    {47, 150, 34, 34, 34, 34},  /* 7 - Bulldozer (square) */
    {6, 188, 56, 24, 56, 24},   /* 8 - Railway (wide) */
    {66, 188, 56, 24, 56, 24},  /* 9 - Roads (wide) */
    {15, 220, 42, 42, 42, 42},  /* 10 - Stadium (shifted up, centered) */
    {85, 150, 34, 34, 34, 34},  /* 11 - Park (moved to row 3) */
    {75, 220, 42, 42, 42, 42},  /* 12 - Seaport (shifted up, centered) */
    {15, 270, 42, 42, 42, 42},  /* 13 - Coal Power Plant (shifted up, centered) */
    {75, 270, 42, 42, 42, 42},  /* 14 - Nuclear Power Plant (shifted up, centered) */
    {37, 320, 58, 58, 58, 58}   /* 15 - Airport (shifted up, recentered for 58x58) */
};

/* File names for tool bitmaps - removed chalk/eraser tools */
static const char *toolBitmapFiles[16] = {
    "resident",   /* 0 - Residential Zone */
    "commerce",    /* 1 - Commercial Zone */
    "industrl",    /* 2 - Industrial Zone */
    "firest",      /* 3 - Fire Station */
    "query",       /* 4 - Query Tool */
    "policest",    /* 5 - Police Station */
    "powerln",     /* 6 - Power Lines */
    "bulldzr",     /* 7 - Bulldozer */
    "rail",        /* 8 - Railway */
    "road",        /* 9 - Roads */
    "stadium",     /* 10 - Stadium */
    "park",        /* 11 - Park */
    "seaport",     /* 12 - Seaport */
    "coal",        /* 13 - Coal Power Plant */
    "nuclear",     /* 14 - Nuclear Power Plant */
    "airport"      /* 15 - Airport */
};

/* Function to process toolbar button clicks */
LRESULT CALLBACK ToolbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    int buttonY;
    int i;
    int buttonX;
    int isSelected;
    int mouseX, mouseY;
    int toolIndex;

    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_PAINT: {
        int iconCenterX, iconCenterY;
        
        hdc = BeginPaint(hwnd, &ps);

        /* Fill the background */
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

        /* Draw buttons using original Micropolis positioning */
        for (i = 0; i < 16; i++) {
            /* Use exact positioning from original layout */
            buttonX = toolLayout[i].x;
            buttonY = toolLayout[i].y;

            /* Set up button rect using original button size */
            rect.left = buttonX;
            rect.top = buttonY;
            rect.right = buttonX + toolLayout[i].width;
            rect.bottom = buttonY + toolLayout[i].height;

            /* Determine if this button is selected */
            isSelected = (GetCurrentTool() == toolbarToStateMapping[i]);

            /* Create a flat gray background for all buttons */
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

            /* Draw a thin dark gray border */
            FrameRect(hdc, &rect, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

            /* Center the icon within the button based on actual button and icon sizes */
            iconCenterX = buttonX + (toolLayout[i].width - toolLayout[i].iconWidth) / 2;
            iconCenterY = buttonY + (toolLayout[i].height - toolLayout[i].iconHeight) / 2;
            DrawToolIcon(hdc, toolbarToStateMapping[i], iconCenterX, iconCenterY, toolLayout[i].iconWidth, toolLayout[i].iconHeight, isSelected);
        }

        /* Draw CPU indicator at bottom of toolbar */
        {
            RECT cpuRect;
            int cpuX, cpuY;
            HDC hdcMem;
            HBITMAP hOldBitmap;
            FILE* debugFile;
            
            /* Get toolbar height and position CPU display at bottom */
            GetClientRect(hwnd, &cpuRect);
            cpuX = 34; /* Center 64px icon in 132px toolbar */
            cpuY = cpuRect.bottom - 72; /* At bottom of toolbar with space for 64px icon */
            
            /* Debug logging to file */
            debugFile = fopen("debug.log", "a");
            if (debugFile) {
                fprintf(debugFile, "CPU Display: rect.bottom=%d, cpuY=%d, bitmap=%p\n", 
                        cpuRect.bottom, cpuY, hCpuBitmap);
                fclose(debugFile);
            }
            
            /* Draw CPU bitmap if available */
            if (hCpuBitmap) {
                hdcMem = CreateCompatibleDC(hdc);
                if (hdcMem) {
                    hOldBitmap = SelectObject(hdcMem, hCpuBitmap);
                    
                    /* Draw CPU bitmap (64x64) */
                    BitBlt(hdc, cpuX, cpuY, 64, 64, hdcMem, 0, 0, SRCCOPY);
                    
                    SelectObject(hdcMem, hOldBitmap);
                    DeleteDC(hdcMem);
                }
            } else {
                /* Draw red placeholder rectangle if bitmap failed to load */
                HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
                RECT bitmapRect;
                bitmapRect.left = cpuX;
                bitmapRect.top = cpuY;
                bitmapRect.right = cpuX + 64;
                bitmapRect.bottom = cpuY + 64;
                FillRect(hdc, &bitmapRect, redBrush);
                DeleteObject(redBrush);
            }
            
            /* CPU text label removed - icon only */
        }

        /* Show current tool information if a tool is active */
        if (isToolActive) {
            const char *toolName;
            int toolCost;
            char buffer[256];
            int textY = 5; /* Anchor to top of toolbar */

            /* Get the tool name */
            switch (GetCurrentTool()) {
            case bulldozerState:
                toolName = "Bulldozer";
                break;
            case roadState:
                toolName = "Road";
                break;
            case railState:
                toolName = "Rail";
                break;
            case wireState:
                toolName = "Wire";
                break;
            case parkState:
                toolName = "Park";
                break;
            case residentialState:
                toolName = "Residential Zone";
                break;
            case commercialState:
                toolName = "Commercial Zone";
                break;
            case industrialState:
                toolName = "Industrial Zone";
                break;
            case fireState:
                toolName = "Fire Station";
                break;
            case policeState:
                toolName = "Police Station";
                break;
            case stadiumState:
                toolName = "Stadium";
                break;
            case seaportState:
                toolName = "Seaport";
                break;
            case powerState:
                toolName = "Coal Power Plant";
                break;
            case nuclearState:
                toolName = "Nuclear Power Plant";
                break;
            case airportState:
                toolName = "Airport";
                break;
            case queryState:
                toolName = "Query";
                break;
            default:
                toolName = "Unknown Tool";
                break;
            }

            toolCost = GetToolCost();

            /* Show the tool name and cost in the toolbar */
            SetTextColor(hdc, RGB(0, 0, 0)); /* Black text */
            SetBkMode(hdc, TRANSPARENT);
            wsprintf(buffer, "Tool: %s", toolName);
            TextOut(hdc, 10, textY, buffer, lstrlen(buffer));

            if (toolCost > 0) {
                wsprintf(buffer, "Cost: $%d", toolCost);
                TextOut(hdc, 10, textY + 15, buffer, lstrlen(buffer));
            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        /* Get mouse coordinates */
        mouseX = LOWORD(lParam);
        mouseY = HIWORD(lParam);

        /* Check each button's area to find which was clicked */
        for (toolIndex = 0; toolIndex < 16; toolIndex++) {
            if (mouseX >= toolLayout[toolIndex].x && 
                mouseX < toolLayout[toolIndex].x + toolLayout[toolIndex].width &&
                mouseY >= toolLayout[toolIndex].y && 
                mouseY < toolLayout[toolIndex].y + toolLayout[toolIndex].height) {
                
                /* Found the clicked button */
                SelectTool(toolbarToStateMapping[toolIndex]);

                /* Redraw the toolbar */
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }
        }
        return 0;
    }

    case WM_DESTROY:
        hwndToolbar = NULL;
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* Load all toolbar bitmap resources */
void LoadToolbarBitmaps(void) {
    int i;
    int resourceId;

    /* Loading tool icons from embedded resources */

    /* Load the bitmaps from embedded resources */
    for (i = 0; i < 16; i++) {
        /* Find the resource ID for this tool */
        resourceId = findToolIconResourceByName(toolBitmapFiles[i]);
        
        if (resourceId != 0) {
            /* Load the bitmap from embedded resource */
            hToolBitmaps[i] = loadTilesetFromResource(resourceId);
            
            if (hToolBitmaps[i] != NULL) {
                /* Successfully loaded tool bitmap */
            } else {
                /* Failed to load tool bitmap */
            }
        } else {
            /* Resource not found for tool */
            hToolBitmaps[i] = NULL;
        }
    }
}

/* Clean up toolbar bitmap resources */
void CleanupToolbarBitmaps(void) {
    int i;

    /* Delete all bitmap handles */
    for (i = 0; i < 16; i++) {
        if (hToolBitmaps[i]) {
            DeleteObject(hToolBitmaps[i]);
            hToolBitmaps[i] = NULL;
        }
    }
}

/* Draw a tool icon using bitmap resources */
void DrawToolIcon(HDC hdc, int toolType, int x, int y, int desiredWidth, int desiredHeight, int isSelected) {
    HDC hdcMem;
    HBITMAP hbmOld;
    int toolIndex;
    BITMAP bm;
    int width, height;
    int centerX, centerY;
    RECT toolRect;
    HPEN hYellowPen;
    HPEN hOldPen;
    char debugMsg[100];
    char indexStr[8];
    RECT rect;

    /* Get the toolbar index using the reverse mapping */
    if (toolType >= 0 && toolType < 17) {
        toolIndex = stateToToolbarMapping[toolType];
    } else {
        toolIndex = 0;
    }

    /* Make sure index is in range */
    if (toolIndex < 0 || toolIndex >= 16) {
        return;
    }

    /* Create a memory DC */
    hdcMem = CreateCompatibleDC(hdc);

    /* Select the bitmap (we only have one version now) */
    if (hToolBitmaps[toolIndex]) {
        hbmOld = SelectObject(hdcMem, hToolBitmaps[toolIndex]);

        /* Get the bitmap dimensions */
        if (GetObject(hToolBitmaps[toolIndex], sizeof(BITMAP), &bm) == 0) {
            /* If GetObject fails, use default dimensions */
            bm.bmWidth = 24;
            bm.bmHeight = 24;

            /* Log the error */
            wsprintf(debugMsg, "GetObject failed for bitmap %d", toolIndex);
            OutputDebugString(debugMsg);
        }
    } else {
        /* Failed to load bitmap - fall back to drawing a rectangle with tool name */
        wsprintf(debugMsg, "No bitmap for tool %d", toolIndex);
        OutputDebugString(debugMsg);

        rect.left = x + 4;
        rect.top = y + 4;
        rect.right = x + 28;
        rect.bottom = y + 28;

        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        /* Show tool index as a visual indicator */
        wsprintf(indexStr, "%d", toolIndex);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, x + 12, y + 12, indexStr, lstrlen(indexStr));

        DeleteDC(hdcMem);
        return;
    }

    /* Use the desired dimensions instead of bitmap dimensions */
    width = desiredWidth;
    height = desiredHeight;

    /* Use the passed coordinates directly - they are already calculated to center the icon */
    centerX = x;
    centerY = y;

    /* Draw the bitmap scaled to desired size */
    StretchBlt(hdc, centerX, centerY, width, height, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight,
               SRCCOPY);

    /* If this tool is selected, draw a thick yellow box around the icon */
    if (isSelected) {
        /* Draw the highlight box around the icon */
        toolRect.left = x - 2;
        toolRect.top = y - 2;
        toolRect.right = x + width + 2;
        toolRect.bottom = y + height + 2;

        /* Create a yellow pen for the outline */
        hYellowPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 0));
        hOldPen = SelectObject(hdc, hYellowPen);

        /* Draw the rectangle */
        SelectObject(hdc, GetStockObject(NULL_BRUSH)); /* Transparent interior */
        Rectangle(hdc, toolRect.left, toolRect.top, toolRect.right, toolRect.bottom);

        /* Clean up the pen */
        SelectObject(hdc, hOldPen);
        DeleteObject(hYellowPen);
    }

    /* Clean up */
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
}

/* Create toolbar window */
void CreateToolbar(HWND hwndParent, int x, int y, int width, int height) {
    WNDCLASS wc;
    RECT clientRect;

    /* Load the tool bitmaps */
    LoadToolbarBitmaps();
    
    /* Detect CPU type and load appropriate bitmap */
    DetectCPUType();

    /* Register the toolbar window class if not already done */
    if (!GetClassInfo(NULL, "WiNTownToolbar", &wc)) {
        /*wc.cbSize = sizeof(WNDCLASS); */
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = ToolbarProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "WiNTownToolbar";
        /*wc.hIconSm = NULL; */

        RegisterClass(&wc);
    }

    /* Create the toolbar window */
    GetClientRect(hwndParent, &clientRect);

    hwndToolbar = CreateWindowEx(
        WS_EX_DLGMODALFRAME, /* Using dialog frame style for a raised appearance */
        "WiNTownToolbar", NULL,
        WS_CHILD | WS_VISIBLE, /* Removed WS_BORDER as DLGMODALFRAME provides its own border */
        0, 0,                  /* x, y - will be adjusted below */
        toolbarWidth, clientRect.bottom, hwndParent, NULL, GetModuleHandle(NULL), NULL);

    /* Set the tool to bulldozer by default */
    SelectTool(bulldozerState);

    /* Force a redraw of the toolbar */
    if (hwndToolbar) {
        InvalidateRect(hwndToolbar, NULL, TRUE);
    }
}

/* Select a tool - set the current tool and activate it */
void SelectTool(int toolType) {
    /* Update the current tool */
    currentTool = toolType;

    /* Set tool active state based on tool type */
    isToolActive = (toolType != noToolState);
    
    /* Reset tool dragging state when changing tools */
    isToolDragging = FALSE;

    /* Only redraw the toolbar - the main window doesn't need to be redrawn completely */
    if (hwndToolbar) {
        InvalidateRect(hwndToolbar, NULL, TRUE);
    }

    /* Update tool cost */
    switch (currentTool) {
    case bulldozerState:
        toolCost = TOOL_BULLDOZER_COST;
        break;

    case roadState:
        toolCost = TOOL_ROAD_COST;
        break;

    case railState:
        toolCost = TOOL_RAIL_COST;
        break;

    case wireState:
        toolCost = TOOL_WIRE_COST;
        break;

    case parkState:
        toolCost = TOOL_PARK_COST;
        break;

    case residentialState:
        toolCost = TOOL_RESIDENTIAL_COST;
        break;

    case commercialState:
        toolCost = TOOL_COMMERCIAL_COST;
        break;

    case industrialState:
        toolCost = TOOL_INDUSTRIAL_COST;
        break;

    case fireState:
        toolCost = TOOL_FIRESTATION_COST;
        break;

    case policeState:
        toolCost = TOOL_POLICESTATION_COST;
        break;

    case stadiumState:
        toolCost = TOOL_STADIUM_COST;
        break;

    case seaportState:
        toolCost = TOOL_SEAPORT_COST;
        break;

    case powerState:
        toolCost = TOOL_POWERPLANT_COST;
        break;

    case nuclearState:
        toolCost = TOOL_NUCLEAR_COST;
        break;

    case airportState:
        toolCost = TOOL_AIRPORT_COST;
        break;

    case queryState:
        toolCost = 0; /* Query tool is free */
        break;

    default:
        toolCost = 0;
        break;
    }
}

/* Check if we have enough funds for the current tool */
int CheckFunds(int cost) {
    /* Don't charge in certain situations */
    if (cost <= 0) {
        return 1;
    }

    if (TotalFunds >= cost) {
        return 1;
    }

    return 0;
}

/* Apply the bulldozer tool */
int DoBulldozer(int mapX, int mapY) {
    short tile;
    int zoneSize;
    int deltaH = 0;
    int deltaV = 0;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    /* Get current tile */
    tile = Map[mapY][mapX] & LOMASK;

    /* If this is empty land, just return success - nothing to do */
    if (tile == DIRT) {
        return TOOLRESULT_OK;
    }

    /* If funds are too low (we need at least $1) */
    if (TotalFunds < 1) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Special case for water-related structures which cost more */
    if ((tile == RIVER || tile == REDGE || tile == CHANNEL || tile == HANDBALL || tile == LHBALL ||
         tile == HBRIDGE || tile == VBRIDGE || tile == BRWH || tile == BRWV) &&
        TotalFunds < 5) {
        return TOOLRESULT_NO_MONEY;
    }

    /* First check if it's part of a zone or big building */
    if (Map[mapY][mapX] & ZONEBIT) {
        /* Direct center-tile bulldozing */
        zoneSize = checkSize(tile);
        switch (zoneSize) {
        case 3:
            Spend(1);
            put3x3Rubble(mapX, mapY);
            return TOOLRESULT_OK;

        case 4:
            Spend(1);
            put4x4Rubble(mapX, mapY);
            return TOOLRESULT_OK;

        case 6:
            Spend(1);
            put6x6Rubble(mapX, mapY);
            return TOOLRESULT_OK;

        default:
            /* Fall through to normal bulldozing for unknown zone types */
            break;
        }
    }
    /* Check if it's part of a large zone but not the center */
    else if ((zoneSize = checkBigZone(tile, &deltaH, &deltaV))) {
        /* Part of a large zone, adjust to the center */
        int centerX = mapX + deltaH;
        int centerY = mapY + deltaV;

        /* Make sure the adjusted coordinates are within bounds */
        if (TestBounds(centerX, centerY)) {
            switch (zoneSize) {
            case 3:
                Spend(1);
                put3x3Rubble(centerX, centerY);
                return TOOLRESULT_OK;

            case 4:
                Spend(1);
                put4x4Rubble(centerX, centerY);
                return TOOLRESULT_OK;

            case 6:
                Spend(1);
                put6x6Rubble(centerX, centerY);
                return TOOLRESULT_OK;
            }
        }
    }

    /* Regular tile bulldozing - direct approach without ConnectTile */
    if (tile == RIVER || tile == REDGE || tile == CHANNEL) {
        /* Can't bulldoze natural water features */
        return TOOLRESULT_FAILED;
    } else if (tile == HANDBALL || tile == LHBALL || tile == HBRIDGE || tile == VBRIDGE ||
               tile == BRWH || tile == BRWV) {
        /* Water-related structures cost $5 */
        Spend(5);
        setMapTile(mapX, mapY, RIVER, 0, TILE_SET_REPLACE, "DoBulldozer-water");
    } else if (tile == RADTILE) {
        /* Can't bulldoze radiation */
        return TOOLRESULT_FAILED;
    } else {
        /* All other tiles cost $1 */
        Spend(1);
        setMapTile(mapX, mapY, DIRT, 0, TILE_SET_REPLACE, "DoBulldozer-dirt");
    }

    /* Fix neighboring tiles after bulldozing */
    FixZone(mapX, mapY, &Map[mapY][mapX]);

    return TOOLRESULT_OK;
}

/* Apply the road tool */
int DoRoad(int mapX, int mapY) {
    short result;
    short baseTile;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    baseTile = Map[mapY][mapX] & LOMASK;

    /* Check if we need to bulldoze first - now allows roads over power lines */
    if (baseTile != DIRT && baseTile != RIVER && baseTile != REDGE && baseTile != CHANNEL &&
        !(baseTile >= TINYEXP && baseTile <= LASTTINYEXP) &&
        !(baseTile >= ROADBASE && baseTile <= LASTROAD) &&
        !(baseTile >= RAILBASE && baseTile <= LASTRAIL) &&
        !(baseTile >= POWERBASE && baseTile <= LASTPOWER)) {
        return TOOLRESULT_NEED_BULLDOZE;
    }

    /* Check if we have enough money - maximum cost case */
    if ((baseTile == RIVER || baseTile == REDGE || baseTile == CHANNEL) &&
        !CheckFunds(BRIDGE_COST)) {
        return TOOLRESULT_NO_MONEY;
    } else if (!CheckFunds(ROAD_COST)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Use Connect tile to build and connect the road (command 2) */
    result = ConnectTile(mapX, mapY, &Map[mapY][mapX], 2);

    if (result == 0) {
        return TOOLRESULT_FAILED;
    }

    return TOOLRESULT_OK;
}

/* Apply the rail tool */
int DoRail(int mapX, int mapY) {
    short result;
    short baseTile;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    baseTile = Map[mapY][mapX] & LOMASK;

    /* Check if we need to bulldoze first - now allows rails over power lines */
    if (baseTile != DIRT && baseTile != RIVER && baseTile != REDGE && baseTile != CHANNEL &&
        !(baseTile >= TINYEXP && baseTile <= LASTTINYEXP) &&
        !(baseTile >= ROADBASE && baseTile <= LASTROAD) &&
        !(baseTile >= RAILBASE && baseTile <= LASTRAIL) &&
        !(baseTile >= POWERBASE && baseTile <= LASTPOWER)) {
        return TOOLRESULT_NEED_BULLDOZE;
    }

    /* Check if we have enough money - maximum cost case */
    if ((baseTile == RIVER || baseTile == REDGE || baseTile == CHANNEL) &&
        !CheckFunds(TUNNEL_COST)) {
        return TOOLRESULT_NO_MONEY;
    } else if (!CheckFunds(RAIL_COST)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Use Connect tile to build and connect the rail (command 3) */
    result = ConnectTile(mapX, mapY, &Map[mapY][mapX], 3);

    if (result == 0) {
        return TOOLRESULT_FAILED;
    }

    return TOOLRESULT_OK;
}

/* Apply the wire tool */
int DoWire(int mapX, int mapY) {
    short result;
    short baseTile;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    baseTile = Map[mapY][mapX] & LOMASK;

    /* Check if we need to bulldoze first - now allows power over roads and rails */
    if (baseTile != DIRT && baseTile != RIVER && baseTile != REDGE && baseTile != CHANNEL &&
        !(baseTile >= TINYEXP && baseTile <= LASTTINYEXP) &&
        !(baseTile >= ROADBASE && baseTile <= LASTROAD) &&
        !(baseTile >= RAILBASE && baseTile <= LASTRAIL)) {
        return TOOLRESULT_NEED_BULLDOZE;
    }

    /* Check if we have enough money - maximum cost case */
    if ((baseTile == RIVER || baseTile == REDGE || baseTile == CHANNEL) &&
        !CheckFunds(UNDERWATER_WIRE_COST)) {
        return TOOLRESULT_NO_MONEY;
    } else if (!CheckFunds(WIRE_COST)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Use Connect tile to build and connect the wire (command 4) */
    result = ConnectTile(mapX, mapY, &Map[mapY][mapX], 4);

    if (result == 0) {
        return TOOLRESULT_FAILED;
    }

    return TOOLRESULT_OK;
}

/* Apply the park tool */
int DoPark(int mapX, int mapY) {
    short tile;
    int randval;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    tile = Map[mapY][mapX] & LOMASK;

    /* Parks can only be built on clear land */
    if (tile != TILE_DIRT) {
        return TOOLRESULT_NEED_BULLDOZE;
    }

    /* Check if we have enough money */
    if (!CheckFunds(TOOL_PARK_COST)) {
        return TOOLRESULT_NO_MONEY;
    }

    Spend(TOOL_PARK_COST); /* Deduct cost */

    /* Random park type */
    randval = SimRandom(4);

    /* Set the tile to a random park tile */
    setMapTile(mapX, mapY, randval + TILE_WOODS, BURNBIT | BULLBIT, TILE_SET_REPLACE, "DoPark-forest");

    return TOOLRESULT_OK;
}

/* Helper function for placing a 3x3 zone */
int PlaceZone(int mapX, int mapY, int baseValue, int totalCost) {
    int dx, dy;
    int index = 0;
    int bulldozeCost = 0;

    /* Check if we can build here */
    if (!Check3x3Area(mapX, mapY, &bulldozeCost)) {
        return TOOLRESULT_FAILED;
    }

    /* Total cost includes zone cost plus any clearing costs */
    totalCost += bulldozeCost;

    /* Check if we have enough money */
    if (!CheckFunds(totalCost)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Clear area if needed and charge for it */
    if (bulldozeCost > 0) {
        for (dy = -1; dy <= 1; dy++) {
            for (dx = -1; dx <= 1; dx++) {
                short tile = Map[mapY + dy][mapX + dx] & LOMASK;
                if (tile != DIRT && (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP))) {
                    setMapTile(mapX + dx, mapY + dy, DIRT, 0, TILE_SET_REPLACE, "clearArea-dirt");
                }
            }
        }
        Spend(bulldozeCost);
    }

    /* Charge for the zone */
    Spend(totalCost - bulldozeCost);

    /* Place the 3x3 zone with proper power conductivity for ALL tiles */
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) {
                /* Center tile gets ZONEBIT, CONDBIT, and BULLBIT */
                setMapTile(mapX, mapY, baseValue + 4, ZONEBIT | BULLBIT | CONDBIT, TILE_SET_REPLACE, "PlaceZone-center");
            } else {
                /* All other tiles in the zone get CONDBIT too for proper power distribution */
                setMapTile(mapX + dx, mapY + dy, baseValue + index, BULLBIT | CONDBIT, TILE_SET_REPLACE, "PlaceZone-tile");
            }
            index++;
        }
    }

    /* Fix the zone edges to connect with neighbors */
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            FixZone(mapX + dx, mapY + dy, &Map[mapY + dy][mapX + dx]);
        }
    }

    return TOOLRESULT_OK;
}

/* Apply the residential zone tool */
int DoResidential(int mapX, int mapY) {
    /* Use the zone helper for residential zones */
    return PlaceZone(mapX, mapY, RESBASE, TOOL_RESIDENTIAL_COST);
}

/* Apply the commercial zone tool */
int DoCommercial(int mapX, int mapY) {
    /* Use the zone helper for commercial zones */
    return PlaceZone(mapX, mapY, COMBASE, TOOL_COMMERCIAL_COST);
}

/* Apply the industrial zone tool */
int DoIndustrial(int mapX, int mapY) {
    /* Use the zone helper for industrial zones */
    return PlaceZone(mapX, mapY, INDBASE, TOOL_INDUSTRIAL_COST);
}

/* Apply the fire station tool */
int DoFireStation(int mapX, int mapY) {
    /* Use the zone helper for fire stations */
    return PlaceZone(mapX, mapY, TILE_FIRESTBASE, TOOL_FIRESTATION_COST);
}

/* Apply the police station tool */
int DoPoliceStation(int mapX, int mapY) {
    /* Use the zone helper for police stations */
    return PlaceZone(mapX, mapY, TILE_POLICESTBASE, TOOL_POLICESTATION_COST);
}

/* Helper function for placing a 4x4 building */
int Place4x4Building(int mapX, int mapY, int baseValue, int centerTile, int totalCost) {
    int dx, dy;
    int index = 0;
    int bulldozeCost = 0;

    /* Check if we can build here */
    if (!Check4x4Area(mapX, mapY, &bulldozeCost)) {
        return TOOLRESULT_FAILED;
    }

    /* Total cost includes building cost plus any clearing costs */
    totalCost += bulldozeCost;

    /* Check if we have enough money */
    if (!CheckFunds(totalCost)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Clear area if needed and charge for it */
    if (bulldozeCost > 0) {
        for (dy = -1; dy <= 2; dy++) {
            for (dx = -1; dx <= 2; dx++) {
                short tile = Map[mapY + dy][mapX + dx] & LOMASK;
                if (tile != DIRT && (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP))) {
                    setMapTile(mapX + dx, mapY + dy, DIRT, 0, TILE_SET_REPLACE, "clearArea-dirt");
                }
            }
        }
        Spend(bulldozeCost);
    }

    /* Charge for the building */
    Spend(totalCost - bulldozeCost);

    /* Place the 4x4 building */
    index = 0;
    for (dy = -1; dy <= 2; dy++) {
        for (dx = -1; dx <= 2; dx++) {
            if (dx == 0 && dy == 0) {
                /* Center tile gets ZONEBIT */
                setMapTile(mapX, mapY, centerTile, ZONEBIT | BULLBIT, TILE_SET_REPLACE, "PlaceBuilding-center");
            } else {
                /* Skip the center index (5) */
                if (index == 5) {
                    index++;
                }
                setMapTile(mapX + dx, mapY + dy, baseValue + index, BULLBIT, TILE_SET_REPLACE, "PlaceBuilding-tile");
            }
            index++;
        }
    }

    /* Fix the building edges to connect with neighbors */
    for (dy = -1; dy <= 2; dy++) {
        for (dx = -1; dx <= 2; dx++) {
            FixZone(mapX + dx, mapY + dy, &Map[mapY + dy][mapX + dx]);
        }
    }

    return TOOLRESULT_OK;
}

/* Apply the coal power plant tool */
int DoPowerPlant(int mapX, int mapY) {
    /* Use the building helper for power plants */
    return Place4x4Building(mapX, mapY, TILE_COALBASE, TILE_POWERPLANT, TOOL_POWERPLANT_COST);
}

/* Apply the nuclear power plant tool */
int DoNuclearPlant(int mapX, int mapY) {
    /* Use the building helper for nuclear plants */
    return Place4x4Building(mapX, mapY, TILE_NUCLEARBASE, TILE_NUCLEAR, TOOL_NUCLEAR_COST);
}

/* Apply the stadium tool */
int DoStadium(int mapX, int mapY) {
    /* Use the building helper for stadiums */
    return Place4x4Building(mapX, mapY, TILE_STADIUMBASE, TILE_STADIUM, TOOL_STADIUM_COST);
}

/* Apply the seaport tool */
int DoSeaport(int mapX, int mapY) {
    /* Use the building helper for seaports */
    return Place4x4Building(mapX, mapY, TILE_PORTBASE, TILE_PORT, TOOL_SEAPORT_COST);
}

/* Helper function for placing a 6x6 building like an airport */
int Place6x6Building(int mapX, int mapY, int baseValue, int centerTile, int totalCost) {
    int dx, dy;
    int index = 0;
    int bulldozeCost = 0;

    /* Check if we can build here */
    if (!Check6x6Area(mapX, mapY, &bulldozeCost)) {
        return TOOLRESULT_FAILED;
    }

    /* Total cost includes building cost plus any clearing costs */
    totalCost += bulldozeCost;

    /* Check if we have enough money */
    if (!CheckFunds(totalCost)) {
        return TOOLRESULT_NO_MONEY;
    }

    /* Clear area if needed and charge for it */
    if (bulldozeCost > 0) {
        for (dy = -2; dy <= 3; dy++) {
            for (dx = -2; dx <= 3; dx++) {
                short tile = Map[mapY + dy][mapX + dx] & LOMASK;
                if (tile != DIRT && (tile == RUBBLE || (tile >= TINYEXP && tile <= LASTTINYEXP))) {
                    setMapTile(mapX + dx, mapY + dy, DIRT, 0, TILE_SET_REPLACE, "clearArea-dirt");
                }
            }
        }
        Spend(bulldozeCost);
    }

    /* Charge for the building */
    Spend(totalCost - bulldozeCost);

    /* Place the 6x6 building */
    index = 0;
    for (dy = -2; dy <= 3; dy++) {
        for (dx = -2; dx <= 3; dx++) {
            if (dx == 0 && dy == 0) {
                /* Center tile gets ZONEBIT */
                setMapTile(mapX, mapY, centerTile, ZONEBIT | BULLBIT, TILE_SET_REPLACE, "PlaceBuilding-center");
            } else {
                /* Skip the center index (14) */
                if (index == 14) {
                    index++;
                }
                setMapTile(mapX + dx, mapY + dy, baseValue + index, BULLBIT, TILE_SET_REPLACE, "PlaceBuilding-tile");
            }
            index++;
        }
    }

    /* Fix the building edges to connect with neighbors */
    for (dy = -2; dy <= 3; dy++) {
        for (dx = -2; dx <= 3; dx++) {
            FixZone(mapX + dx, mapY + dy, &Map[mapY + dy][mapX + dx]);
        }
    }

    return TOOLRESULT_OK;
}

/* Apply the airport tool */
int DoAirport(int mapX, int mapY) {
    /* Use the building helper for airports */
    return Place6x6Building(mapX, mapY, TILE_AIRPORTBASE, TILE_AIRPORT, TOOL_AIRPORT_COST);
}

/* Helper function to get zone name from tile */
const char *GetZoneName(short tile) {
    short baseTile = tile & LOMASK;

    if (baseTile >= RESBASE && baseTile <= LASTRES) {
        return "Residential Zone";
    } else if (baseTile >= COMBASE && baseTile <= LASTCOM) {
        return "Commercial Zone";
    } else if (baseTile >= INDBASE && baseTile <= LASTIND) {
        return "Industrial Zone";
    } else if (baseTile >= PORTBASE && baseTile <= LASTPORT) {
        return "Seaport";
    } else if (baseTile >= AIRPORTBASE && baseTile <= LASTAIRPORT) {
        return "Airport";
    } else if (baseTile >= COALBASE && baseTile <= LASTPOWERPLANT) {
        return "Coal Power Plant";
    } else if (baseTile >= NUCLEARBASE && baseTile <= LASTNUCLEAR) {
        return "Nuclear Power Plant";
    } else if (baseTile >= FIRESTBASE && baseTile <= LASTFIRESTATION) {
        return "Fire Station";
    } else if (baseTile >= POLICESTBASE && baseTile <= LASTPOLICESTATION) {
        return "Police Station";
    } else if (baseTile >= STADIUMBASE && baseTile <= LASTSTADIUM) {
        return "Stadium";
    } else if (baseTile >= ROADBASE && baseTile <= LASTROAD) {
        return "Road";
    } else if (baseTile >= RAILBASE && baseTile <= LASTRAIL) {
        return "Rail";
    } else if (baseTile >= POWERBASE && baseTile <= LASTPOWER) {
        return "Power Line";
    } else if (baseTile == RIVER || baseTile == REDGE || baseTile == CHANNEL) {
        return "Water";
    } else if (baseTile >= RUBBLE && baseTile <= LASTRUBBLE) {
        return "Rubble";
    } else if (baseTile >= TREEBASE && baseTile <= LASTTREE) {
        return "Trees";
    } else if (baseTile == RADTILE) {
        return "Radiation";
    } else if (baseTile >= FIREBASE && baseTile <= LASTFIRE) {
        return "Fire";
    } else if (baseTile >= FLOOD && baseTile <= LASTFLOOD) {
        return "Flood";
    }

    return "Clear Land";
}

/* Apply the query tool - shows information about the tile */
int DoQuery(int mapX, int mapY) {
    short tile;
    char message[256];
    const char *zoneName;

    /* Check bounds */
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return TOOLRESULT_FAILED;
    }

    tile = Map[mapY][mapX];

    /* Get zone name from tile */
    zoneName = GetZoneName(tile);

    /* Prepare message */
    wsprintf(message, "Location: %d, %d\nTile Type: %s\nHas Power: %s", mapX, mapY, zoneName,
             (tile & POWERBIT) ? "Yes" : "No");

    /* Display zone information in game log */
    addGameLog("ZONE INFO: %s", message);

    return TOOLRESULT_OK;
}

/* Apply the current tool at the given coordinates */
int ApplyTool(int mapX, int mapY) {
    int result = TOOLRESULT_FAILED;

    switch (currentTool) {
    case bulldozerState:
        result = DoBulldozer(mapX, mapY);
        break;

    case roadState:
        result = DoRoad(mapX, mapY);
        break;

    case railState:
        result = DoRail(mapX, mapY);
        break;

    case wireState:
        result = DoWire(mapX, mapY);
        break;

    case parkState:
        result = DoPark(mapX, mapY);
        break;

    case residentialState:
        result = DoResidential(mapX, mapY);
        break;

    case commercialState:
        result = DoCommercial(mapX, mapY);
        break;

    case industrialState:
        result = DoIndustrial(mapX, mapY);
        break;

    case fireState:
        result = DoFireStation(mapX, mapY);
        break;

    case policeState:
        result = DoPoliceStation(mapX, mapY);
        break;

    case stadiumState:
        result = DoStadium(mapX, mapY);
        break;

    case seaportState:
        result = DoSeaport(mapX, mapY);
        break;

    case powerState:
        result = DoPowerPlant(mapX, mapY);
        break;

    case nuclearState:
        result = DoNuclearPlant(mapX, mapY);
        break;

    case airportState:
        result = DoAirport(mapX, mapY);
        break;

    case queryState:
        result = DoQuery(mapX, mapY);
        break;

    default:
        result = TOOLRESULT_FAILED;
        break;
    }

    /* Store the result for later display */
    toolResult = result;

    /* Force redraw of map */
    InvalidateRect(hwndMain, NULL, FALSE);

    return result;
}

/* Get the current tool */
int GetCurrentTool(void) {
    return currentTool;
}

/* Function to update the toolbar display (including RCI bars) */
void UpdateToolbar(void) {
    if (hwndToolbar) {
        InvalidateRect(hwndToolbar, NULL, TRUE);
    }
}

/* Get the last tool result */
int GetToolResult(void) {
    return toolResult;
}

/* Get the current tool cost */
int GetToolCost(void) {
    return toolCost;
}

/* Get the size of a tool (1x1, 3x3, 4x4, or 6x6) */
int GetToolSize(int toolType) {
    switch (toolType) {
    case residentialState:
    case commercialState:
    case industrialState:
    case fireState:
    case policeState:
        return TOOL_SIZE_3X3; /* 3x3 zones */

    case stadiumState:
    case seaportState:
    case powerState:
    case nuclearState:
        return TOOL_SIZE_4X4; /* 4x4 buildings */

    case airportState:
        return TOOL_SIZE_6X6; /* 6x6 buildings */

    case roadState:
    case railState:
    case wireState:
    case parkState:
    case bulldozerState:
    case queryState:
    default:
        return TOOL_SIZE_1X1; /* Single tile tools */
    }
}

/* Draw the tool hover highlight at the given map position */
void DrawToolHover(HDC hdc, int mapX, int mapY, int toolType, int xOffset, int yOffset) {
    int size = GetToolSize(toolType);
    int screenX, screenY;
    int startX, startY;
    int width, height;
    HPEN hPen;
    HPEN hOldPen;
    HBRUSH hOldBrush;

    /* Calculate the starting position based on tool size */
    switch (size) {
    case TOOL_SIZE_3X3:
        /* Center 3x3 highlight at cursor */
        startX = mapX - 1;
        startY = mapY - 1;
        width = 3;
        height = 3;
        break;

    case TOOL_SIZE_4X4:
        /* Center 4x4 highlight at cursor */
        startX = mapX - 1;
        startY = mapY - 1;
        width = 4;
        height = 4;
        break;

    case TOOL_SIZE_6X6:
        /* Center 6x6 highlight at cursor */
        startX = mapX - 2;
        startY = mapY - 2;
        width = 6;
        height = 6;
        break;

    case TOOL_SIZE_1X1:
    default:
        /* Single tile highlight */
        startX = mapX;
        startY = mapY;
        width = 1;
        height = 1;
        break;
    }

    /* Convert map coordinates to screen coordinates */
    screenX = (startX * TILE_SIZE) - xOffset;
    screenY = (startY * TILE_SIZE) - yOffset;

    /* Create a white pen for the outline */
    hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));

    if (!hPen) {
        return;
    }

    /* Select pen and null brush (for hollow rectangle) */
    hOldPen = SelectObject(hdc, hPen);
    hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

    /* Draw the rectangle */
    Rectangle(hdc, screenX, screenY, screenX + (width * TILE_SIZE), screenY + (height * TILE_SIZE));

    /* Clean up */
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);

    /* Save the last position for efficiency */
    lastMouseMapX = mapX;
    lastMouseMapY = mapY;
}

/* Convert screen coordinates to map coordinates */
void ScreenToMap(int screenX, int screenY, int *mapX, int *mapY, int xOffset, int yOffset) {
    /*
     * Convert mouse position to map coordinates.
     * Add xOffset to account for the map scrolling,
     * no need to adjust for toolbar as screenX is already relative to the left of client area
     */
    *mapX = (screenX + xOffset - toolbarWidth) / TILE_SIZE;
    *mapY = (screenY + yOffset) / TILE_SIZE;
}

/* Mouse handler for tools - converts mouse coordinates to map coordinates and applies the current
 * tool */
int HandleToolMouse(int mouseX, int mouseY, int xOffset, int yOffset) {
    int mapX, mapY;

    /* Convert screen coordinates to map coordinates */
    ScreenToMap(mouseX, mouseY, &mapX, &mapY, xOffset, yOffset);

    /* Apply the current tool */
    return ApplyTool(mapX, mapY);
}
