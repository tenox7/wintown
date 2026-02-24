/* tiles.h - Centralized tile management for WiNTown
 * Provides unified interface for all map tile changes
 */

#ifndef _TILES_H
#define _TILES_H

/* Tile change operations */
#define TILE_SET_REPLACE    0  /* Replace tile completely */
#define TILE_SET_PRESERVE   1  /* Set tile preserving existing flags */
#define TILE_SET_FLAGS      2  /* Set only flags, preserve tile */
#define TILE_CLEAR_FLAGS    3  /* Clear specific flags */
#define TILE_TOGGLE_FLAGS   4  /* Toggle specific flags */

/* Function prototypes */
int setMapTile(int x, int y, int tile, int flags, int operation, char* caller);
int getMapTile(int x, int y);
int getMapFlags(int x, int y);

/* Debug logging control */
int enableTileDebug(int enable);
#ifdef TILE_DEBUG
void resetTileLogging();
#endif

/* Tile change statistics */
extern long tileChangeCount;
extern long tileErrorCount;
int resetTileStats();
int printTileStats();

#endif /* _TILES_H */
