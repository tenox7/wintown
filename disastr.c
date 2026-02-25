/* disasters.c - Disaster implementation for WiNTown
 * Based on original WiNTown code from WiNTownLegacy project
 */

#include "sim.h"
#include "tiles.h"
#include "notify.h"
#include "sprite.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

/* External log functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* External functions */
extern int SimRandom(int range);

/* External variables */
extern HWND hwndMain;
extern short Map[WORLD_Y][WORLD_X];

/* Forward declarations */
static void doMeltdown(int sx, int sy);

/* Common movement direction arrays */
static const short xDelta[4] = {0, 1, 0, -1};
static const short yDelta[4] = {-1, 0, 1, 0};

/* Local definitions for disaster purposes */
/* Use the constant from simulation.h */
#define LOCAL_LASTZONE LASTZONE /* Last zone tile for disaster purposes */


/* Trigger an earthquake disaster */
void doEarthquake(void) {
    int x, y, z;
    int time;
    short tile, tileValue;
    int epicenterX, epicenterY;

    epicenterX = CCx;
    epicenterY = CCy;

    /* Random earthquake damage - with reasonable limits */
    time = SimRandom(700) + 300;
    if (time > 1000) {
        time = 1000; /* Cap to prevent excessive processing */
    }

    /* Start earthquake screen shake effect */
    startEarthquake();

    /* Log the earthquake */
    addGameLog("DISASTER: EARTHQUAKE!!!");
    addGameLog("Epicenter at coordinates %d,%d", epicenterX, epicenterY);
    addDebugLog("Earthquake: Magnitude %d, Duration %d", (time / 100), time);

    for (z = 0; z < time; z++) {
        /* Get random coordinates but ensure they are within bounds */
        x = SimRandom(WORLD_X);
        y = SimRandom(WORLD_Y);

        if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
            continue;
        }

        /* Check if tile is vulnerable */
        tile = Map[y][x];
        tileValue = tile & LOMASK;

        if ((tileValue >= RESBASE) && (tileValue <= LOCAL_LASTZONE) && !(tile & ZONEBIT)) {
            if (z & 0x3) {
                /* Create rubble (every 4th iteration) */
                setMapTile(x, y, RUBBLE + SimRandom(4), BULLBIT, TILE_SET_REPLACE, "doEarthquake-rubble");
            } else {
                /* Create fire */
                setMapTile(x, y, FIRE + SimRandom(8), ANIMBIT, TILE_SET_REPLACE, "doEarthquake-fire");
            }
        }
    }

    /* Show enhanced notification dialog AFTER earthquake damage */
    addDebugLog("Showing earthquake notification at (%d,%d)", epicenterX, epicenterY);
    ShowNotificationAt(NOTIF_EARTHQUAKE, epicenterX, epicenterY);
    addDebugLog("Earthquake notification call completed");

    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}

/* Create an explosion */
void makeExplosion(int x, int y) {
    int dir, tx, ty;

    /* Validate coordinates first */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Create fire at explosion center */
    setMapTile(x, y, FIRE + SimRandom(8), ANIMBIT, TILE_SET_REPLACE, "makeExplosion-center");

    /* Create fire in surrounding tiles (N, E, S, W) */
    for (dir = 0; dir < 4; dir++) {
        tx = x + xDelta[dir];
        ty = y + yDelta[dir];

        /* Check bounds for each surrounding tile */
        if (BOUNDS_CHECK(tx, ty)) {
            /* Only set fire if not a zone center */
            if (!(Map[ty][tx] & ZONEBIT)) {
                setMapTile(tx, ty, FIRE + SimRandom(8), ANIMBIT, TILE_SET_REPLACE, "makeExplosion-spread");
            }
        }
    }

    /* Show enhanced notification dialog */
    ShowNotificationAt(NOTIF_FIRE_REPORTED, x, y);

    /* Log explosion */
    addGameLog("DISASTER: Explosion at %d,%d!", x, y);
    addDebugLog("Explosion created at coordinates %d,%d", x, y);

    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}

void makeFire(int x, int y) {
    short t, z;
    int i;

    for (i = 0; i < 40; i++) {
        x = SimRandom(WORLD_X);
        y = SimRandom(WORLD_Y);
        z = Map[y][x];
        if ((!(z & ZONEBIT)) && (z & BURNBIT)) {
            t = z & LOMASK;
            if (t > 21 && t < LASTZONE) {
                Map[y][x] = FIRE + ANIMBIT + (rand() & 7);
                ShowNotificationAt(NOTIF_FIRE_REPORTED, x, y);
                return;
            }
        }
    }
}

void makeMonster(void) {
    int x, y, z, done;
    SimSprite *monster;
    extern short PolMaxX, PolMaxY;

    monster = GetSpriteByType(SPRITE_MONSTER);
    if (monster) {
        monster->count = 1000;
        monster->dest_x = PolMaxX << 4;
        monster->dest_y = PolMaxY << 4;
        return;
    }

    done = 0;
    for (z = 0; z < 300; z++) {
        x = SimRandom(WORLD_X - 20) + 10;
        y = SimRandom(WORLD_Y - 10) + 5;
        if ((Map[y][x] & LOMASK) == RIVER) {
            done = 1;
            break;
        }
    }
    if (!done) {
        x = WORLD_X / 2;
        y = WORLD_Y / 2;
    }

    monster = NewSprite(SPRITE_MONSTER, x << 4, y << 4);
    if (monster) {
        monster->dest_x = PolMaxX << 4;
        monster->dest_y = PolMaxY << 4;
    }

    ShowNotification(NOTIF_MONSTER_SIGHTED);
    InvalidateRect(hwndMain, NULL, FALSE);
}

/* Create a flood disaster */
void makeFlood(void) {
    extern int FloodCnt;
    int x, y, xx, yy;
    int waterFound = 0;
    int attempts = 0;
    int t;
    short tileValue;

    FloodCnt = 30;

    /* Log the flood disaster */
    addGameLog("DISASTER: Flooding has been reported!");
    addGameLog("Water levels are rising in low-lying areas!");
    addDebugLog("Flood disaster starting from water edge");

    /* Try to find water edge to start flood, with a reasonable attempt limit */
    while (!waterFound && attempts < 300) {
        /* Generate random coordinates within world bounds */
        x = SimRandom(WORLD_X);
        y = SimRandom(WORLD_Y);

        /* Validate coordinates */
        if (BOUNDS_CHECK(x, y)) {
            tileValue = Map[y][x] & LOMASK;

            /* Look for river/water tiles */
            if (tileValue > 4 && tileValue < 21) {
                /* Check all four adjacent tiles */
                for (t = 0; t < 4; t++) {
                    xx = x + xDelta[t];
                    yy = y + yDelta[t];

                    /* Check if adjacent tile is in bounds and floodable */
                    if (BOUNDS_CHECK(xx, yy)) {
                        if (Map[yy][xx] == DIRT ||
                            ((Map[yy][xx] & BULLBIT) && (Map[yy][xx] & BURNBIT))) {

                            setMapTile(xx, yy, FLOOD + SimRandom(3), 0, TILE_SET_REPLACE, "makeFlood-initial");
                            waterFound = 1;

                            ShowNotificationAt(NOTIF_FLOODING, xx, yy);
                            break;
                        }
                    }
                }
            }
        }
        attempts++;
    }

    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}

void makeMeltdown(void) {
    int x, y;

    for (x = 0; x < WORLD_X - 1; x++) {
        for (y = 0; y < WORLD_Y - 1; y++) {
            if ((Map[y][x] & LOMASK) == NUCLEAR) {
                doMeltdown(x, y);
                return;
            }
        }
    }
}

static void doMeltdown(int sx, int sy) {
    int x, y, z, t;

    makeExplosion(sx - 1, sy - 1);
    makeExplosion(sx - 1, sy + 2);
    makeExplosion(sx + 2, sy - 1);
    makeExplosion(sx + 2, sy + 2);

    for (x = sx - 1; x < sx + 3; x++) {
        for (y = sy - 1; y < sy + 3; y++) {
            if (BOUNDS_CHECK(x, y))
                Map[y][x] = FIRE + (rand() & 3) + ANIMBIT;
        }
    }

    for (z = 0; z < 200; z++) {
        x = sx - 20 + SimRandom(41);
        y = sy - 15 + SimRandom(31);
        if (!BOUNDS_CHECK(x, y)) continue;
        t = Map[y][x];
        if (t & ZONEBIT) continue;
        if ((t & BURNBIT) || (t == 0))
            Map[y][x] = RADTILE;
    }

    ShowNotificationAt(NOTIF_NUCLEAR_MELTDOWN, sx, sy);
    InvalidateRect(hwndMain, NULL, FALSE);
}

void makeTornado(void) {
    int x, y;
    SimSprite *tornado;

    if (GetSpriteByType(SPRITE_TORNADO))
        return;

    x = SimRandom(WORLD_X - 20) + 10;
    y = SimRandom(WORLD_Y - 10) + 5;

    tornado = NewSprite(SPRITE_TORNADO, (x << 4) + 8, (y << 4) + 8);

    ShowNotificationAt(NOTIF_TORNADO, x, y);
    InvalidateRect(hwndMain, NULL, FALSE);
}
