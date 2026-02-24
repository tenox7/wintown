/* animation.c - Tile animation implementation for WiNTown
 * Based on original WiNTown code from WiNTownLegacy project
 */

#include "animtab.h"
#include "sim.h"
#include "tiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Animation state flags */
static int AnimationEnabled = 1; /* Animation enabled by default */

/* Forward declarations */
static void DoCoalSmoke(int x, int y);
static void DoIndustrialSmoke(int x, int y);
static void DoStadiumAnimation(int x, int y);

/* Process animations for the entire map */
void AnimateTiles(void) {
    unsigned short tilevalue, tileflags;
    int x, y;
    static int debugCount = 0;
    static int animCounter = 0;

    /* Skip animation if disabled */
    if (!AnimationEnabled) {
        return;
    }

    /* Only animate every 3rd frame to reduce CPU usage */
    if ((animCounter++ % 3) != 0) {
        return;
    }

    /* Scan the entire map for tiles with the ANIMBIT set */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            tilevalue = Map[y][x];

            /* Only process tiles with the animation bit set */
            if (tilevalue & ANIMBIT) {
                tileflags = tilevalue & MASKBITS; /* Save the flags */
                tilevalue &= LOMASK;              /* Extract base tile value */

                /* Debug animation (once every 100 frames) */
                if (debugCount == 0) {
                    /* Check for known animation types to debug them */
                    if (tilevalue >= TELEBASE && tilevalue <= TELELAST) {
                        char debugMsg[256];
                        wsprintf(debugMsg, "ANIMATION: Industrial smoke at (%d,%d) frame %d\n", 
                                 x, y, tilevalue);
                        OutputDebugString(debugMsg);
                    } else if (tilevalue == NUCLEAR_SWIRL) {
                        char debugMsg[256];
                        wsprintf(debugMsg, "ANIMATION: Nuclear reactor at (%d,%d)\n", x, y);
                        OutputDebugString(debugMsg);
                    } else if (tilevalue >= RADAR0 && tilevalue <= RADAR7) {
                        char debugMsg[256];
                        wsprintf(debugMsg, "ANIMATION: Airport radar animation at (%d,%d)\n", x, y);
                        OutputDebugString(debugMsg);
                    } else if (tilevalue == FOOTBALLGAME1 || tilevalue == FOOTBALLGAME2) {
                        char debugMsg[256];
                        wsprintf(debugMsg, "ANIMATION: Stadium game at (%d,%d)\n", x, y);
                        OutputDebugString(debugMsg);
                    }
                }

                /* Look up the next animation frame */
                tilevalue = aniTile[tilevalue];

                /* Reapply the flags */
                tilevalue |= tileflags;

                /* Update the map with the new tile */
                setMapTile(x, y, tilevalue, 0, TILE_SET_REPLACE, "AnimateTiles-frame");
            }
        }
    }
    
    /* Increment debug counter and wrap around */
    debugCount = (debugCount + 1) % 100;
}

/* Enable or disable animations */
void SetAnimationEnabled(int enabled) {
    AnimationEnabled = enabled;
}

/* Get animation enabled status */
int GetAnimationEnabled(void) {
    return AnimationEnabled;
}

/* Add smoke animation to industrial zones and coal power plants */
void SetSmoke(int x, int y) {
    /* Tables for industrial smoke positions and animations */
    static short AniThis[8] = { 1, 0, 1, 1, 0, 0, 1, 1 };
    static short DX1[8] = { -1, 0, 1, 0, 0, 0, 0, 1 };
    static short DY1[8] = { -1, 0, -1, -1, 0, 0, -1, -1 };
    static short AniTabA[8] = { 0, 0, 32, 40, 0, 0, 48, 56 };
    static short AniTabC[8] = { IND1, 0, IND2, IND4, 0, 0, IND6, IND8 };
    short tile, z;
    int xx, yy;
    int i;
    int isPowerPlant = 0;

    /* Skip if the position is invalid */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    tile = Map[y][x] & LOMASK;
    
    /* Check if this is a power plant - handle separately */
    if (tile == POWERPLANT) {
        isPowerPlant = 1;
        /* Power plant smoke handling */
        for (i = 0; i < 4; i++) {
            xx = x + smokeOffsetX[i];
            yy = y + smokeOffsetY[i];

            /* Make sure the smoke position is valid */
            if (BOUNDS_CHECK(xx, yy)) {
                /* Cache tile value to avoid multiple lookups - Issue #18 optimization */
                short currentTile = Map[yy][xx];
                short baseTile = currentTile & LOMASK;
                
                /* Only set the tile if it doesn't already have the animation bit set
                   or if it's not already a smoke tile. This avoids resetting the
                   animation sequence and makes it flow better. */
                if (!(currentTile & ANIMBIT) || baseTile < COALSMOKE1 ||
                    baseTile > COALSMOKE4 + 4) {

                    /* Set the appropriate smoke tile with animation */
                    switch (i) {
                    case 0:
                        setMapTile(xx, yy, COALSMOKE1, ANIMBIT | CONDBIT | POWERBIT | BURNBIT, TILE_SET_REPLACE, "SetSmoke-coal1");
                        break;
                    case 1:
                        setMapTile(xx, yy, COALSMOKE2, ANIMBIT | CONDBIT | POWERBIT | BURNBIT, TILE_SET_REPLACE, "SetSmoke-coal2");
                        break;
                    case 2:
                        setMapTile(xx, yy, COALSMOKE3, ANIMBIT | CONDBIT | POWERBIT | BURNBIT, TILE_SET_REPLACE, "SetSmoke-coal3");
                        break;
                    case 3:
                        setMapTile(xx, yy, COALSMOKE4, ANIMBIT | CONDBIT | POWERBIT | BURNBIT, TILE_SET_REPLACE, "SetSmoke-coal4");
                        break;
                    }
                }
            }
        }
        return;
    }
    
    /* Handle industrial zones */
    if (tile < INDBASE || tile > LASTIND) {
        return;  /* Not an industrial zone */
    }
    
    /* Only animate powered zones */
    if (!(Map[y][x] & POWERBIT)) {
        return;
    }
    
    /* Calculate which industrial building type (0-7) */
    z = (tile - INDBASE) >> 3;
    z = z & 7;
    
    /* Check if this industrial type should have smoke */
    if (AniThis[z]) {
        xx = x + DX1[z];
        yy = y + DY1[z];
        
        if (BOUNDS_CHECK(xx, yy)) {
            /* Only animate if it's the right base tile */
            if ((Map[yy][xx] & LOMASK) == AniTabC[z]) {
                /* Set the animated smoke tile */
                setMapTile(xx, yy, SMOKEBASE + AniTabA[z], ANIMBIT | CONDBIT | POWERBIT | BURNBIT, TILE_SET_REPLACE, "SetSmoke-industrial");
            }
        }
    }
}

/* Add smoke animation to industrial buildings */
static void DoIndustrialSmoke(int x, int y) {
    int i;
    int xx, yy;
    short baseTile;
    short smokeTile;

    /* Skip if the position is invalid */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Check if this industrial building has power */
    if (!(Map[y][x] & POWERBIT)) {
        return;
    }

    /* Get the base tile value */
    baseTile = Map[y][x] & LOMASK;

    /* Look for industrial buildings that need smoke */
    for (i = 0; i < 8; i++) {
        /* Skip if this isn't a valid smoke position */
        if (indSmokeTable[i] == 0) {
            continue;
        }

        /* Check if this industrial building type gets smoke */
        if (baseTile == indSmokeTable[i]) {
            /* Calculate the smoke position */
            xx = x + indOffsetX[i];
            yy = y + indOffsetY[i];

            /* Make sure the smoke position is valid */
            if (BOUNDS_CHECK(xx, yy)) {
                /* Cache tile value to avoid multiple lookups - Issue #18 optimization */
                short currentTile = Map[yy][xx];
                short baseTile = currentTile & LOMASK;
                
                /* Choose the appropriate smoke stack tile */
                switch (i) {
                case 0: smokeTile = INDSMOKE1; break;
                case 2: smokeTile = INDSMOKE3; break;
                case 3: smokeTile = INDSMOKE4; break;
                case 6: smokeTile = INDSMOKE6; break;
                case 7: smokeTile = INDSMOKE7; break;
                default: smokeTile = INDSMOKE1; break;
                }

                /* Skip if already animated with the right tile - use TELEBASE range */
                if ((currentTile & ANIMBIT) && baseTile >= TELEBASE &&
                    baseTile <= TELELAST) {
                    continue;
                }

                /* Set the smoke animation */
                setMapTile(xx, yy, smokeTile, ANIMBIT | CONDBIT | POWERBIT | BURNBIT, TILE_SET_REPLACE, "DoIndustrialSmoke");
            }
        }
    }
}

/* Update stadium animations for games */
static void DoStadiumAnimation(int x, int y) {
    int centerX, centerY;

    /* Skip if the position is invalid */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Check if this is a stadium with power */
    if ((Map[y][x] & LOMASK) != STADIUM || !(Map[y][x] & POWERBIT)) {
        return;
    }

    /* Only update periodically - games happen based on city time and position */
    if (!((CityTime + x + y) & 31)) {
        /* Stadium center coordinates to place the game field */
        centerX = x + 1;
        centerY = y;

        /* Make sure the position is valid */
        if (centerX >= 0 && centerX < WORLD_X && centerY >= 0 && centerY < WORLD_Y) {
            /* Set up the football game animation */
            setMapTile(centerX, centerY, FOOTBALLGAME1, ANIMBIT | CONDBIT | BURNBIT, TILE_SET_REPLACE, "DoStadiumAnimation-game1");
            
            /* Set up the second part of the football game animation */
            if (centerY+1 >= 0 && centerY+1 < WORLD_Y) {
                setMapTile(centerX, centerY+1, FOOTBALLGAME2, ANIMBIT | CONDBIT | BURNBIT, TILE_SET_REPLACE, "DoStadiumAnimation-game2");
            }
        }
    }

    /* Check if we need to end the game */
    if (((CityTime + x + y) & 7) == 0) {
        /* Scan for football game tiles and remove them */
        centerX = x + 1;
        centerY = y;

        if (centerX >= 0 && centerX < WORLD_X && centerY >= 0 && centerY < WORLD_Y) {
            short tileValue = Map[centerY][centerX] & LOMASK;
            
            /* If we find football game tiles, revert them */
            if (tileValue >= FOOTBALLGAME1 && tileValue <= FOOTBALLGAME1 + 16) {
                setMapTile(centerX, centerY, 0, ANIMBIT, TILE_CLEAR_FLAGS, "DoStadiumAnimation-revert1");
            }
            
            if (centerY+1 >= 0 && centerY+1 < WORLD_Y) {
                tileValue = Map[centerY+1][centerX] & LOMASK;
                
                if (tileValue >= FOOTBALLGAME2 && tileValue <= FOOTBALLGAME2 + 16) {
                    setMapTile(centerX, centerY+1, 0, ANIMBIT, TILE_CLEAR_FLAGS, "DoStadiumAnimation-revert2");
                }
            }
        }
    }
}

/* Update fire animations */
void UpdateFire(int x, int y) {
    /* Check bounds */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Set fire tiles to animate */
    if ((Map[y][x] & LOMASK) >= FIREBASE && (Map[y][x] & LOMASK) <= (FIREBASE + 7)) {
        setMapTile(x, y, 0, ANIMBIT, TILE_SET_FLAGS, "UpdateFire-animate");
    }
}

/* Update nuclear power plant animations */
void UpdateNuclearPower(int x, int y) {
    /* Check bounds */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Set the nuclear core to animate */
    if ((Map[y][x] & LOMASK) == NUCLEAR) {
        int xx, yy;

        /* Set animation bits on the nuclear plant's core */
        xx = x + 2;
        yy = y - 1;

        if (BOUNDS_CHECK(xx, yy)) {
            /* Set the nuclear swirl animation bit with appropriate flags */
            setMapTile(xx, yy, NUCLEAR_SWIRL, ANIMBIT | CONDBIT | POWERBIT | BURNBIT, TILE_SET_REPLACE, "UpdateNuclearPower-swirl");
        }
    }
}

/* Update airport radar animation */
void UpdateAirportRadar(int x, int y) {
    /* Check bounds */
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return;
    }

    /* Check if this is an airport */
    if ((Map[y][x] & LOMASK) == AIRPORT) {
        int xx, yy;

        /* Set animation bit on the radar tower tile */
        xx = x + 1;
        yy = y - 1;

        if (BOUNDS_CHECK(xx, yy)) {
            /* Set the radar animation bit with appropriate flags */
            setMapTile(xx, yy, RADAR0, ANIMBIT | CONDBIT | BURNBIT, TILE_SET_REPLACE, "UpdateAirportRadar-rotate");
        }
    }
}

/* Update all special animations - called periodically from simulation */
void UpdateSpecialAnimations(void) {
    int x, y;
    short tileValue;

    /* Skip if animation is disabled */
    if (!AnimationEnabled) {
        return;
    }

    /* Scan entire map for special animations */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            /* Cache full tile value to avoid multiple lookups - Issue #18 optimization */
            short fullTileValue = Map[y][x];
            tileValue = fullTileValue & LOMASK;
            
            /* Only process zone centers to avoid unnecessary work */
            if (!(fullTileValue & ZONEBIT)) {
                continue;
            }

            /* Add smoke to power plants */
            if (tileValue == POWERPLANT) {
                SetSmoke(x, y);
            }

            /* Add smoke to industrial buildings */
            if (tileValue >= INDBASE && tileValue <= LASTIND) {
                DoIndustrialSmoke(x, y);
            }

            /* Animate nuclear plants */
            if (tileValue == NUCLEAR) {
                UpdateNuclearPower(x, y);
            }

            /* Animate airport radar */
            if (tileValue == AIRPORT) {
                UpdateAirportRadar(x, y);
            }
            
            /* Animate stadium */
            if (tileValue == STADIUM) {
                DoStadiumAnimation(x, y);
            }
        }
    }
}