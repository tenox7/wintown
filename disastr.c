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

    /* Set epicenter to center of the map */
    epicenterX = WORLD_X / 2;
    epicenterY = WORLD_Y / 2;

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

/* Check for and spread fires - called from simulation loop */
void spreadFire(void) {
    int x, y, dir, tx, ty;
    int i;
    short tileValue;

    /* Process a limited number of random locations */
    for (i = 0; i < 20; i++) {
        /* Pick a random position */
        x = SimRandom(WORLD_X);
        y = SimRandom(WORLD_Y);

        /* Skip if out of bounds */
        if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
            continue;
        }

        tileValue = Map[y][x] & LOMASK;

        /* Check if it's a fire tile */
        if (tileValue >= FIRE && tileValue < (FIRE + 8)) {
            /* It's a fire! Chance to spread to adjacent tiles */
            if (SimRandom(10) < 3) { /* 30% chance to spread */
                /* Log fire spreading only occasionally to avoid spam */
                if (SimRandom(20) == 0) {
                    addDebugLog("Fire spreading at %d,%d", x, y);
                }
                /* Pick a random direction */
                dir = SimRandom(4);
                tx = x + xDelta[dir];
                ty = y + yDelta[dir];

                /* Check if the target tile is in bounds */
                if (BOUNDS_CHECK(tx, ty)) {
                    /* Only spread to burnable tiles */
                    if (Map[ty][tx] & BURNBIT) {
                        /* Create a fire with animation */
                        setMapTile(tx, ty, FIRE + SimRandom(8), ANIMBIT, TILE_SET_REPLACE, "spreadFire-spread");
                    }
                }
            }

            /* Small chance for fire to burn out */
            if (SimRandom(10) == 0) { /* 10% chance to burn out */
                /* Convert to rubble */
                setMapTile(x, y, RUBBLE + SimRandom(4), BULLBIT, TILE_SET_REPLACE, "spreadFire-burnout");
            }
        }
    }
}

/* Create a monster (Godzilla-like) disaster */
void makeMonster(void) {
    int x, y;
    SimSprite *monster;

    /* Log the monster disaster */
    addGameLog("DISASTER: A monster has been reported in the city!");
    addGameLog("Monster is destroying everything in its path!");
    addDebugLog("Monster disaster starting - creating animated sprite");

    /* Find a good starting position (preferably at edge of map) */
    if (SimRandom(2) == 0) {
        /* Start from left or right edge */
        x = (SimRandom(2) == 0) ? 0 : (WORLD_X - 1);
        y = SimRandom(WORLD_Y);
    } else {
        /* Start from top or bottom edge */
        x = SimRandom(WORLD_X);
        y = (SimRandom(2) == 0) ? 0 : (WORLD_Y - 1);
    }

    /* Create animated monster sprite */
    monster = NewSprite(SPRITE_MONSTER, x << 4, y << 4);
    if (monster) {
        addGameLog("Animated monster sprite created at %d,%d", x, y);
        addDebugLog("Monster sprite: x=%d y=%d type=%d", monster->x, monster->y, monster->type);
    } else {
        addGameLog("Failed to create monster sprite - falling back to fire");
        /* Fallback: create fire if sprite creation failed */
        makeFire(x, y);
    }

    /* Show enhanced notification dialog */
    ShowNotification(NOTIF_MONSTER_SIGHTED);

    /* Force redraw */
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

/* Create a tornado disaster */
void makeTornado(void) {
    int x, y;
    SimSprite *tornado;

    /* Log the tornado disaster */
    addGameLog("DISASTER: Tornado warning issued!");
    addGameLog("Tornado touching down and moving across the city!");
    addDebugLog("Tornado disaster starting - creating animated sprite");

    /* Pick a random starting position */
    x = SimRandom(WORLD_X);
    y = SimRandom(WORLD_Y);

    /* Create animated tornado sprite */
    tornado = NewSprite(SPRITE_TORNADO, x << 4, y << 4);
    if (tornado) {
        addGameLog("Animated tornado sprite created at %d,%d", x, y);
        addDebugLog("Tornado sprite: x=%d y=%d type=%d", tornado->x, tornado->y, tornado->type);
    } else {
        addGameLog("Failed to create tornado sprite - falling back to fire");
        /* Fallback: create fire if sprite creation failed */
        makeFire(x, y);
    }

    /* Show enhanced notification dialog */
    ShowNotificationAt(NOTIF_TORNADO, x, y);

    /* Force redraw */
    InvalidateRect(hwndMain, NULL, FALSE);
}
