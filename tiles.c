/* tiles.c - Centralized tile management for WiNTown
 * All map tile changes go through this module for debugging and validation
 */

#include "sim.h"
#include "tiles.h"
#include <stdio.h>
#include <string.h>

/* Debug and statistics globals */
long tileChangeCount = 0;
long tileErrorCount = 0;

#ifdef TILE_DEBUG
/* Tile change log file */
static FILE* tileLogFile = (FILE*)0;

/* Initialize tile logging */
static int initTileLogging() {
    if (!tileLogFile) {
        tileLogFile = fopen("tiles.log", "w");
        if (tileLogFile) {
            fprintf(tileLogFile, "=== Tile logging started ===\n");
            fflush(tileLogFile);
        } else {
            /* Could not open log file - silent fail in release */
            return 0;
        }
    }
    return (tileLogFile != (FILE*)0);
}

/* Reset tile logging - closes and reopens the log file */
void resetTileLogging() {
    if (tileLogFile) {
        fclose(tileLogFile);
        tileLogFile = (FILE*)0;
    }
    tileChangeCount = 0;
    tileErrorCount = 0;
    initTileLogging();
}

/* Convert flags to readable string */
static void flagsToString(int flags, char* buffer, int bufSize) {
    int len;
    
    buffer[0] = '\0';
    
    if (flags & POWERBIT) strcat(buffer, "POWER ");
    if (flags & CONDBIT) strcat(buffer, "COND ");
    if (flags & BURNBIT) strcat(buffer, "BURN ");
    if (flags & BULLBIT) strcat(buffer, "BULL ");
    if (flags & ANIMBIT) strcat(buffer, "ANIM ");
    if (flags & ZONEBIT) strcat(buffer, "ZONE ");
    
    if (buffer[0] == '\0') {
        strcpy(buffer, "NONE");
    } else {
        /* Remove trailing space */
        len = strlen(buffer);
        if (len > 0 && buffer[len-1] == ' ') {
            buffer[len-1] = '\0';
        }
    }
}

/* Log tile change */
static int logTileChange(int x, int y, int oldTile, int newTile, char* caller) {
    int oldBase, newBase;
    int oldFlags, newFlags;
    char oldFlagStr[64], newFlagStr[64];
    
    if (!initTileLogging()) {
        return 0;
    }
    
    oldBase = oldTile & LOMASK;
    newBase = newTile & LOMASK;
    oldFlags = oldTile & ~LOMASK;
    newFlags = newTile & ~LOMASK;
    
    flagsToString(oldFlags, oldFlagStr, sizeof(oldFlagStr));
    flagsToString(newFlags, newFlagStr, sizeof(newFlagStr));
    
    fprintf(tileLogFile, "TILE[%d,%d]: 0x%04X->0x%04X base %d->%d flags [%s]->[%s] by %s\n", 
            x, y, oldTile, newTile, oldBase, newBase, oldFlagStr, newFlagStr, caller ? caller : "unknown");
    fflush(tileLogFile);
    return 1;
}
#else
/* Debug functions empty in release builds */
static int logTileChange(int x, int y, int oldTile, int newTile, char* caller) {
    return 0;
}
#endif

/* validateTileCoords() and validateTileValue() functions removed - now using inline macros */

/* Get tile value at coordinates */
int getMapTile(int x, int y) {
    if (!BOUNDS_CHECK(x, y)) {
        return -1;
    }
    return Map[y][x];
}

/* Get only flags from tile at coordinates */
int getMapFlags(int x, int y) {
    if (!BOUNDS_CHECK(x, y)) {
        return -1;
    }
    return Map[y][x] & ~LOMASK;
}

/* Main tile setting function - all tile changes go through here */
int setMapTile(int x, int y, int tile, int flags, int operation, char* caller) {
    int oldTile, newTile;
    
    /* Validate coordinates */
    if (!BOUNDS_CHECK(x, y)) {
#ifdef TILE_DEBUG
        logTileChange(x, y, -1, -1, caller);
#endif
        tileErrorCount++;
        return 0;
    }
    
    /* Get current tile */
    oldTile = Map[y][x];
    
    /* Calculate new tile value based on operation */
    switch (operation) {
        case TILE_SET_REPLACE:
            /* Complete replacement */
            newTile = tile | flags;
            break;
            
        case TILE_SET_PRESERVE:
            /* Set tile preserving existing flags */
            newTile = (oldTile & ~LOMASK) | (tile & LOMASK) | flags;
            break;
            
        case TILE_SET_FLAGS:
            /* Set only flags, preserve tile and existing flags */
            newTile = oldTile | flags;
            break;
            
        case TILE_CLEAR_FLAGS:
            /* Clear specific flags */
            newTile = oldTile & ~flags;
            break;
            
        case TILE_TOGGLE_FLAGS:
            /* Toggle specific flags */
            newTile = oldTile ^ flags;
            break;
            
        default:
            /* Invalid operation */
            tileErrorCount++;
            return 0;
    }
    
    /* Validate new tile value */
    if ((newTile & LOMASK) >= 1024) {
#ifdef TILE_DEBUG
        fprintf(tileLogFile, "ERROR: Invalid tile base %d (full value %d) at [%d,%d] by %s\n",
                newTile & LOMASK, newTile, x, y, caller ? caller : "unknown");
        fflush(tileLogFile);
#endif
        tileErrorCount++;
        return 0;
    }
    
    /* Log the change if debugging enabled */
#ifdef TILE_DEBUG
    if (oldTile != newTile) {
        logTileChange(x, y, oldTile, newTile, caller);
    }
#endif
    
    /* Make the change */
    Map[y][x] = newTile;
    tileChangeCount++;
    
    return 1;
}

#ifdef TILE_DEBUG
/* Enable/disable debug logging */
int enableTileDebug(int enable) {
    if (!enable && tileLogFile) {
        fprintf(tileLogFile, "=== Tile logging stopped ===\n");
        fclose(tileLogFile);
        tileLogFile = (FILE*)0;
    }
    return 1;
}
#else
/* Enable/disable debug logging */
int enableTileDebug(int enable) {
    return 0;
}
#endif

/* Reset statistics */
int resetTileStats() {
    tileChangeCount = 0;
    tileErrorCount = 0;
    return 1;
}

/* Print statistics */
int printTileStats() {
#ifdef DEBUG
    addDebugLog("Tile Statistics:\n");
    addDebugLog("  Changes: %ld\n", tileChangeCount);
    addDebugLog("  Errors: %ld\n", tileErrorCount);
#ifdef TILE_DEBUG
    if (tileLogFile) {
        addDebugLog("  Debug log: tiles.log\n");
    }
#endif
#endif
    return 1;
}