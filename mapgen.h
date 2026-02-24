/* mapgen.h - Map Generation System Header for WiNTown */

#ifndef MAPGEN_H
#define MAPGEN_H

#include <windows.h>

/* Map types */
#define MAPTYPE_RIVERS 0
#define MAPTYPE_ISLAND 1

/* Map generation parameters */
typedef struct {
    int mapType;        /* MAPTYPE_RIVERS or MAPTYPE_ISLAND */
    int waterPercent;   /* Percentage of water coverage (0-100) */
    int forestPercent;  /* Percentage of forest coverage (0-100) */
} MapGenParams;

/* Function prototypes */
int generateTerrainMap(MapGenParams *params);
int generateMapPreview(MapGenParams *params, HBITMAP *previewBitmap, int width, int height);
void initMapGenParams(MapGenParams *params);

#endif /* MAPGEN_H */