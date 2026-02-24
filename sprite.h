/* sprite.h - Transportation sprite system header for WiNTown
 * Based on original WiNTown sprite system
 */

#ifndef _SPRITE_H
#define _SPRITE_H

#include <windows.h>

/* Maximum number of sprites that can exist simultaneously */
#define MAX_SPRITES     48

/* Sprite types */
#define SPRITE_UNDEFINED    0
#define SPRITE_TRAIN        1
#define SPRITE_HELICOPTER   2
#define SPRITE_AIRPLANE     3
#define SPRITE_SHIP         4
#define SPRITE_BUS          5
#define SPRITE_MONSTER      6
#define SPRITE_TORNADO      7
#define SPRITE_EXPLOSION    8
#define SPRITE_FERRY        9
#define SPRITE_POLICE       10

/* Train and bus groove offsets for lane positioning */
#define TRA_GROOVE_X    -39
#define TRA_GROOVE_Y    6
#define BUS_GROOVE_X    -39
#define BUS_GROOVE_Y    6

/* Sound effect IDs */
#define SOUND_HONKHONK_LOW      1
#define SOUND_HEAVY_TRAFFIC     2
#define SOUND_EXPLOSION_HIGH    3

/* Sprite behavior constants */
#define DEFAULT_SPRITE_SPEED            100
#define POLICE_DUTY_TIME                300     /* Police stay for 5 minutes */

/* Movement type constants for unified movement function */
#define MOVEMENT_TYPE_GROUND    0  /* Uses Dx/Dy arrays (trains, buses) */
#define MOVEMENT_TYPE_HELICOPTER 1  /* Uses BDx/BDy arrays */
#define MOVEMENT_TYPE_BOAT      2  /* Uses BPx/BPy arrays */
#define MOVEMENT_TYPE_AIRPLANE  3  /* Uses CDx/CDy arrays */

/* Random chance constants */
#define TRAIN_START_CHANCE              8       /* 1 in 8 chance to start moving */
#define TRAIN_STOP_CHANCE               4       /* 1 in 4 chance to stop at station */
#define TRAIN_STOP_DURATION_BASE        30      /* Base stop time */
#define TRAIN_STOP_DURATION_RANDOM      30      /* Additional random stop time */

#define SHIP_TURN_CHANCE                7       /* 1 in 7 chance to turn */
#define SHIP_DIRECTION_CHANGE_CHANCE    16      /* 1 in 16 chance to change direction */

#define HELICOPTER_SOUND_TIMER          100     /* Helicopter sound check interval */
#define HELICOPTER_TRAFFIC_THRESHOLD    170     /* Traffic level to trigger sound */
#define HELICOPTER_TRAFFIC_SOUND_CHANCE 7       /* 1 in 7 chance for traffic sound */
#define HELICOPTER_SOUND_DURATION       200     /* How long sound effect lasts */
#define HELICOPTER_DIRECTION_CHANCE     5       /* 1 in 5 chance to change direction */

#define POLICE_TRAFFIC_THRESHOLD        200     /* Traffic level for police report */
#define POLICE_REPORT_CHANCE            5       /* 1 in 5 chance to report */

/* Sprite generation thresholds */
#define HIGH_TRAFFIC_THRESHOLD          80      /* Traffic level for helicopter spawn */
#define TRAFFIC_HELICOPTER_CHANCE       50      /* 1 in 50 chance */
#define HIGH_CRIME_THRESHOLD            50      /* Crime level for helicopter spawn */
#define CRIME_HELICOPTER_CHANCE         80      /* 1 in 80 chance */
#define LARGE_POPULATION_THRESHOLD      10000   /* Population for airplane spawn */
#define AIRPLANE_SPAWN_CHANCE           100     /* 1 in 100 chance */
#define MIN_ROADS_FOR_BUS               10      /* Minimum roads before bus spawn */
#define BUS_SPAWN_CHANCE                25      /* 1 in 25 chance */

/* Sprite generation frequency */
#define TRAIN_SPAWN_FREQUENCY           25      /* 1 in 25 simulation cycles */
#define SHIP_SPAWN_FREQUENCY            100     /* 1 in 100 simulation cycles */
#define AIRCRAFT_SPAWN_FREQUENCY        50      /* 1 in 50 simulation cycles */

/* Disaster sprite constants */
#define FIRE_START_CHANCE               1000    /* 1 in 1000 chance for fire */
#define MELTDOWN_CHANCE                 500     /* 1 in 500 chance for meltdown */
#define MONSTER_SPAWN_CHANCE            300     /* 1 in 300 chance for monster */
#define MONSTER_LIFESPAN                1000    /* Monster lives 1000 cycles */
#define TORNADO_LIFESPAN                200     /* Tornado lives 200 cycles */
#define TORNADO_EARLY_END_CHANCE        500     /* 1 in 500 chance early end */

/* Collision and bounds constants */
#define SPRITE_BOUNDS_MARGIN            100     /* Margin before sprite removal */
#define HELICOPTER_SPAWN_MARGIN         20      /* Margin from map edge */
#define HELICOPTER_WANDER_RANGE         60      /* Random movement range */
#define HELICOPTER_WANDER_OFFSET        30      /* Center offset for wander */

/* Sprite structure */
typedef struct SimSprite {
    int type;           /* Sprite type (train, ship, etc.) */
    int frame;          /* Current animation frame */
    int x, y;          /* Position in world coordinates */
    int width, height; /* Sprite dimensions */
    int x_offset, y_offset; /* Drawing offset */
    int x_hot, y_hot;  /* Hot spot for collision detection */
    int orig_x, orig_y; /* Original spawn position */
    int dest_x, dest_y; /* Destination coordinates */
    int count;         /* General purpose counter */
    int sound_count;   /* Sound effect timer */
    int dir;           /* Current direction (0-7 or 0-3) */
    int new_dir;       /* Target direction for turning */
    int step;          /* Animation step counter */
    int flag;          /* General purpose flag */
    int control;       /* Control state (-1=auto, 0+=player) */
    int turn;          /* Turn state */
    int accel;         /* Acceleration value */
    int speed;         /* Movement speed */
} SimSprite;

/* Function prototypes */

/* Core sprite system */
void InitSprites(void);
SimSprite* NewSprite(int type, int x, int y);
void DestroySprite(SimSprite *sprite);
void MoveSprites(void);

/* Individual sprite behaviors */
void DoTrainSprite(SimSprite *sprite);
void DoShipSprite(SimSprite *sprite);
void DoAirplaneSprite(SimSprite *sprite);
void DoCopterSprite(SimSprite *sprite);
void DoBusSprite(SimSprite *sprite);
void DoPoliceSprite(SimSprite *sprite);
void DoMonsterSprite(SimSprite *sprite);
void DoTornadoSprite(SimSprite *sprite);
void DoExplosion(SimSprite *sprite);

/* Sprite generation functions */
void GenerateTrains(void);
void GenerateShips(void);
void GenerateAircraft(void);
void GenerateHelicopters(void);

/* Utility functions */
int GetSpriteCount(void);
SimSprite* GetSprite(int index);
void MoveSprite(SimSprite *sprite, int movementType);

/* External variables that need to be defined elsewhere */
extern short Map[100][120];          /* Main game map */
extern unsigned char TrfDensity[50][60]; /* Traffic density map */
extern int TotalPop;                 /* Total city population */
extern int TrafficAverage;           /* Average traffic level */
extern int CrimeAverage;             /* Average crime level */
extern int RoadTotal;                /* Total road count */
extern int SimRandom(int range);     /* Random number generator */
extern void makeFire(int x, int y);  /* Fire creation function */

#endif /* _SPRITE_H */