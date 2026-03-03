/* simulation.c - Core simulation implementation for WiNTown
 * Based on original WiNTown code from WiNTownLegacy project
 */

#include "sim.h"
#include "tiles.h"
#include "sprite.h"
#include "charts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "gdifix.h"

/* External log functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* Forward declarations */

/* External cheat flags */
extern int disastersDisabled;

/* Map data */
/* RISC CPU Optimization: Align data structures on 8-byte boundaries */
#pragma pack(push, 8)
Byte PopDensity[WORLD_X / 2][WORLD_Y / 2];
Byte TrfDensity[WORLD_X / 2][WORLD_Y / 2];
Byte PollutionMem[WORLD_X / 2][WORLD_Y / 2];
Byte LandValueMem[WORLD_X / 2][WORLD_Y / 2];
Byte CrimeMem[WORLD_X / 2][WORLD_Y / 2];

/* Quarter-sized map */
Byte TerrainMem[WORLD_X / 4][WORLD_Y / 4];

/* Eighth-sized maps (SmX x SmY) - matches original Micropolis resolution */
short FireStMap[SmX][SmY];
short FireRate[SmX][SmY];
short PoliceMap[SmX][SmY];
short PoliceMapEffect[SmX][SmY];
short ComRate[SmX][SmY];

/* Rate of growth memory */
short RateOGMem[ROGMEM_X][ROGMEM_Y];

/* Power distribution bitmap */
#define POWERMAPROW ((WORLD_X + 15) / 16)
#define PWRMAPSIZE (POWERMAPROW * WORLD_Y)
short PowerMap[PWRMAPSIZE];
#pragma pack(pop)

/* Runtime simulation state */
int SimSpeed = SPEED_MEDIUM;
int SimSpeedMeta = 0;
int SimPaused = 1;
int CityTime = 0;
int CityYear = 1900;
int CityMonth = 0;
QUAD TotalFunds = 5000;
int CityTax = 7;      /* City tax rate 0-20% */
int DoInitialEval = 0;

/* External declarations for scenario variables */
extern short ScenarioID;
extern short DisasterEvent;
extern short DisasterWait;
extern short ScoreType;
extern short ScoreWait;
extern void scenarioDisaster(void);
extern DoScenarioScore(int scoreType);

/* Zone population totals for SendMessages */
int TotalZPop = 0;
int ResZPop = 0;
int ComZPop = 0;
int IndZPop = 0;
int CoalPop = 0;
int FireStPop = 0;
short AvCityTax = 0;

/* Counters */
int Scycle = 0;
int Fcycle = 0;
int Spdcycle = 0;

/* Game evaluation - CityYes/CityNo/CityPop/CityScore/deltaCityScore/CityClass in s_eval.c */
int CityLevel = 0;
int CityLevelPop = 0;
int GameLevel = 0;
int ResCap = 0;
int ComCap = 0;
int IndCap = 0;

/* City statistics */
int ResPop = 0;
int ComPop = 0;
int IndPop = 0;
int TotalPop = 0;
int LastTotalPop = 0;
float Delta = 1.0f;

/* Temporary census accumulation variables removed - Issue #17 fixed
 * Modern systems are fast enough that direct updates to main variables
 * during map scanning don't cause noticeable display flicker
 */

/* Infrastructure counts */
int PwrdZCnt = 0;
int unPwrdZCnt = 0;
int RoadTotal = 0;
int RailTotal = 0;
int FirePop = 0;
int PolicePop = 0;
int StadiumPop = 0;
int PortPop = 0;
int APortPop = 0;
int NuclearPop = 0;

/* External effects */
int RoadEffect = 0;
int PoliceEffect = 1000;
int FireEffect = 1000;
int PolluteAverage = 0;
int CrimeAverage = 0;
int LVAverage = 0;
short NewPower = 0;

/* Growth rates */
short RValve = 0;
short CValve = 0;
short IValve = 0;
int ValveFlag = 0;

/* Economic model variables (from original Micropolis) */
float EMarket = 4.0f;
short CrimeRamp = 0;
short PolluteRamp = 0;
short CashFlow = 0;

/* Disasters */
extern short DisasterEvent; /* Defined in scenarios.c */
extern short DisasterWait;  /* Defined in scenarios.c */
int DisasterLevel = 0;
int DisastersEnabled = 1;  /* Enable/disable disasters (0=disabled, 1=enabled) */
/* FloodCnt/ShakeNow/FloodX/FloodY in s_disast.c */
int AutoBulldoze = 1;      /* Auto-bulldoze enabled flag */
int SimTimerDelay = 200;   /* Timer delay in milliseconds based on speed */

/* Difficulty level multiplier tables - based on original WiNTown */
float DifficultyTaxEfficiency[3] = { 1.4f, 1.2f, 0.8f };      /* Easy, Medium, Hard */
float DifficultyMaintenanceCost[3] = { 0.7f, 0.9f, 1.2f };    /* Easy, Medium, Hard */
float DifficultyIndustrialGrowth[3] = { 1.2f, 1.1f, 0.98f };  /* Easy, Medium, Hard */
short DifficultyDisasterChance[3] = { 480, 240, 60 };         /* Easy, Medium, Hard */
short DifficultyMeltdownRisk[3] = { 30000, 20000, 10000 };    /* Easy, Medium, Hard */

/* Internal work variables - also used by power.c */
int SMapX, SMapY; /* Current map position (no longer static, needed by power.c) */
static int TMapX, TMapY;
short CChr;
short CChr9;

/* Random number generator - Windows compatible */
void RandomlySeedRand(void) {
    srand((unsigned int)GetTickCount());
}

/* Public random number function - available to other modules */
int SimRandom(int range) {
    return (rand() % range);
}

int Rand(int range) {
    return rand() % (range + 1);
}

short Rand16() {
    return (short)(rand() & 0xFFFF);
}

short Rand16Signed() {
    return (short)(rand() & 0xFFFF);
}

void DoSimInit(void) {
    int x, y;
    int oldResPop, oldComPop, oldIndPop, oldTotalPop, oldCityClass;
    QUAD oldCityPop;
    int centerX, centerY, distance, value;

    /* Save previous population values */
    oldResPop = ResPop;
    oldComPop = ComPop;
    oldIndPop = IndPop;
    oldTotalPop = TotalPop;
    oldCityPop = CityPop;
    oldCityClass = CityClass;
    
    /* Reset tile logging to overwrite the log file */
#ifdef TILE_DEBUG
    resetTileLogging();
#endif

    /* Clear all the density maps */
    memset(PopDensity, 0, sizeof(PopDensity));
    memset(TrfDensity, 0, sizeof(TrfDensity));
    memset(PollutionMem, 0, sizeof(PollutionMem));
    memset(LandValueMem, 0, sizeof(LandValueMem));
    memset(CrimeMem, 0, sizeof(CrimeMem));

    /* Clear all the effect maps */
    memset(TerrainMem, 0, sizeof(TerrainMem));
    memset(FireStMap, 0, sizeof(FireStMap));
    memset(FireRate, 0, sizeof(FireRate));
    memset(PoliceMap, 0, sizeof(PoliceMap));
    memset(PoliceMapEffect, 0, sizeof(PoliceMapEffect));
    memset(ComRate, 0, sizeof(ComRate));
    memset(RateOGMem, 0, sizeof(RateOGMem));

    /* Initialize land values to make simulation more visually interesting */
    centerX = WORLD_X / 4;
    centerY = WORLD_Y / 4;

    for (y = 0; y < WORLD_Y / 2; y++) {
        for (x = 0; x < WORLD_X / 2; x++) {
            /* Create a gradient of land values */
            distance = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
            value = 250 - (distance / 10);
            if (value < 1) {
                value = 1;
            }
            if (value > 250) {
                value = 250;
            }

            LandValueMem[x][y] = (Byte)value;
        }
    }

    /* Clear power status from all tiles */
    for (y = 0; y < WORLD_Y; y++) {
        for (x = 0; x < WORLD_X; x++) {
            /* Clear the power bit in the map */
            setMapTile(x, y, 0, POWERBIT, TILE_CLEAR_FLAGS, "DoSimInit-clear");
        }
    }

    /* Set up random number generator */
    RandomlySeedRand();

    /* Initialize simulation counters */
    Scycle = 0;
    Fcycle = 0;
    Spdcycle = 0;

    /* Default settings */
    SimSpeed = SPEED_MEDIUM;
    SimPaused = 0; /* Set to running by default */
    CityTime = 0;
    CityYear = 1900;
    CityMonth = 0;

    /* Population counters */
    /* Initialize population values - don't force minimums */
    ResPop = oldResPop;
    ComPop = oldComPop;
    IndPop = oldIndPop;
    TotalPop = oldTotalPop;
    CityPop = oldCityPop;
    CityClass = oldCityClass;
    LastTotalPop = oldTotalPop;

    RValve = 0;
    CValve = 0;
    IValve = 0;
    EMarket = 6.0f;
    CrimeRamp = 0;
    PolluteRamp = 0;
    ResCap = 0;
    ComCap = 0;
    IndCap = 0;
    ValveFlag = 1;

    /* Set initial funds and tax rate */
    TotalFunds = 50000; /* Give more starting money */
    CityTax = 7;

    /* Initial game state */
    CityScore = 500;
    DisasterEvent = 0;
    DisasterWait = 0;

    /* Initialize evaluation system */
    WinEvalInit();

    /* Initialize budget system */
    InitBudget();

    /* Initialize sprite system */
    InitSprites();

    /* Generate a random disaster wait period */
    DisasterWait = SimRandom(51) + 49;

    DoInitialEval = 1;

    TakeCensus();
}

void SimFrame(void) {
    /* Main simulation frame entry point */
    static int debugCounter = 0;
    
    debugCounter++;
    if (debugCounter % 300 == 0) { /* Log every 300 frames (about every 5 seconds) */
        addDebugLog("SimFrame called: SimPaused=%d, SimSpeed=%d, Spdcycle=%d", SimPaused, SimSpeed, Spdcycle);
    }

    if (SimPaused) {
        if (debugCounter % 300 == 0) {
            addDebugLog("SimFrame: Simulation is paused, returning early");
        }
        return;
    }

    /* Update the speed cycle counter */
    Spdcycle = (Spdcycle + 1) & (SPEEDCYCLE - 1);

    /* Execute simulation steps based on speed */
    switch (SimSpeed) {
    case SPEED_PAUSED:
        /* Do nothing - simulation is paused */
        break;

    case SPEED_SLOW:
        /* Slow speed - process every 5th frame */
        if ((Spdcycle % 5) == 0) {
            Fcycle = (Fcycle + 1) & 1023;
            Simulate(Fcycle & 15);
        }
        break;

    case SPEED_MEDIUM:
        /* Medium speed - process every 3rd frame */
        if ((Spdcycle % 3) == 0) {
            Fcycle = (Fcycle + 1) & 1023;
            Simulate(Fcycle & 15);
        }
        break;

    case SPEED_FAST:
        /* Fast speed - process every frame */
        Fcycle = (Fcycle + 1) & 1023;
        Simulate(Fcycle & 15);
        break;
    }

    MoveSprites();
}

void Simulate(int mod16) {
    static short SpdPwr[4] = { 1,  2,  4,  5 };
    static short SpdPtl[4] = { 1,  2,  7, 17 };
    static short SpdCri[4] = { 1,  1,  8, 18 };
    static short SpdPop[4] = { 1,  1,  9, 19 };
    static short SpdFir[4] = { 1,  1, 10, 20 };
    short spd;

    spd = (short)SimSpeed;
    if (spd > 3) spd = 3;

    switch (mod16) {
    case 0:
        if (++Scycle > 1023)
            Scycle = 0;
        if (DoInitialEval) {
            DoInitialEval = 0;
            WinCityEvaluation();
        }
        DoTimeStuff();
        AvCityTax += CityTax;
        if (!(Scycle & 1))
            SetValves();
        ClearCensus();

        if (ValveFlag) {
            ValveFlag = 0;
            UpdateToolbar();
        }

        AnimateTiles();
        doMessage();
        break;

    case 1:
    case 2:
#ifdef DEBUG
        /* Debug: Map scan segment tracking */
        {
            static int logOnce = 0;
            if (!logOnce) {
                addDebugLog("MAP SCAN: Starting optimized segment processing");
                logOnce = 1;
            }
        }
#endif
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        /* Scan map in 8 different segments (1/8th each time) */
        {
            int xs = (mod16 - 1) * (WORLD_X / 8);
            int xe = xs + (WORLD_X / 8);
            MapScan(xs, xe, 0, WORLD_Y);
        }
        break;

    case 9:
        /* Update CityPop only when population counters change */
        /* Population counters are now updated directly during map scanning */
        CityPop = CalculateCityPopulation(ResPop, ComPop, IndPop);

        /* Update police coverage display map immediately after stations are scanned */
        /* This ensures the minimap shows current coverage without waiting for CrimeScan */
        {
            int x, y;
            for (x = 0; x < SmX; x++)
                for (y = 0; y < SmY; y++)
                    PoliceMapEffect[x][y] = PoliceMap[x][y];
        }

        /* Update charts every case 9 (every 16 cycles) */
        if (g_chartData) {
            addDebugLog("Case 9: Updating chart data - Scycle=%d", Scycle);
            UpdateChartData();
        } else {
            addDebugLog("Case 9: Skipping chart update - g_chartData is NULL");
        }
        
        /* Every 4 cycles, take census for graphs */
        if ((Scycle % CENSUSRATE) == 0) {
            addDebugLog("Taking census, Scycle=%d", Scycle);
            TakeCensus();
        }

        /* Every 48 cycles (CENSUSRATE*12), take long-term census for 120-year graphs */
        if ((Scycle % (CENSUSRATE * 12)) == 0) {
            Take2Census();
        }

        /* Every 48 time units, do tax collection and evaluation (as in original) */
        addDebugLog("CHECKING TAX: CityTime=%d, TAXFREQ=%d, mod=%d", CityTime, TAXFREQ, (CityTime % TAXFREQ));
        if ((CityTime % TAXFREQ) == 0) {
            addDebugLog("Tax collection triggered: CityTime=%d, TAXFREQ=%d", CityTime, TAXFREQ);
            CollectTax();        /* Collect taxes based on population */
            CountSpecialTiles(); /* Count special buildings */
            WinCityEvaluation();    /* Evaluate city conditions */
        }
        break;

    case 10:
        if (!(Scycle % 5)) DecROGMem();
        DecTrafficMem();
        SendMessages();
        break;

    case 11:
        if (!(Scycle % SpdPwr[spd])) {
            DoPowerScan();
            NewPower = 1;
        }

        /* Check if population has gone to zero (but not initially) */
        if (TotalPop > 0 || LastTotalPop == 0) {
            LastTotalPop = TotalPop;
        } else if (TotalPop == 0 && LastTotalPop != 0) {
            /* ToDo: DoShowPicture(POPULATIONLOST_BIT); */
            LastTotalPop = 0;

            /* Log catastrophic population decline */
            addGameLog("CRISIS: All citizens have left the city!");
        }

        /* Update city class based on population */
        CityClass = 0; /* Village */
        if (CityPop > 2000) {
            CityClass++; /* Town */
        }
        if (CityPop > 10000) {
            CityClass++; /* City */
        }
        if (CityPop > 50000) {
            CityClass++; /* Capital */
        }
        if (CityPop > 100000) {
            CityClass++; /* Metropolis */
        }
        if (CityPop > 500000) {
            CityClass++; /* Megalopolis */
        }

        /* Log city class - only done when population changes */
        addDebugLog("City class: %s (Pop: %d)", GetCityClassName(), (int)CityPop);
        break;

    case 12:
        if (!(Scycle % SpdPtl[spd])) {
            PTLScan();
            addDebugLog("Pollution average: %d", PolluteAverage);
            addDebugLog("Land value average: %d", LVAverage);
        }

        /* Update special animations (power plants, etc.) - Issue #19 timing fix
         * Align with CityTime-based timing to avoid unnecessary calls */
        if ((Scycle % 8) == 0) {
            UpdateSpecialAnimations();
        }

        /* Process tile animations more frequently for smoother motion */
        AnimateTiles();
        break;

    case 13:
        if (!(Scycle % SpdCri[spd])) {
            CrimeScan();
            if (CrimeAverage > 100)
                addGameLog("WARNING: Crime level is very high (%d)", CrimeAverage);
        }
        break;

    case 14:
        if (!(Scycle % SpdPop[spd]))
            PopDenScan();
        break;

    case 15:
        if (!(Scycle % SpdFir[spd]))
            FireAnalysis();
        DoDisasters();

        break;
    }
}

void DoTimeStuff(void) {
    static int lastMilestone = 0;
    static int lastCityClass = 0;
    int currentMilestone;

    /* Process time advancement */
    CityTime++;

    CityMonth++;
    
#ifdef DEBUG
    /* Log RCI values monthly for debugging */
    addDebugLog("Monthly RCI: Residential=%d Commercial=%d Industrial=%d (Month %d)", 
                ResPop, ComPop, IndPop, CityMonth);
#endif
    
    if (CityMonth > 11) {
        CityMonth = 0;
        CityYear++;

        /* Log the new year */
        addGameLog("New year: %d", CityYear);

        /* Log population milestones */
        currentMilestone = ((int)CityPop / 10000) * 10000;

        if (CityPop > 0 && currentMilestone > lastMilestone) {
            if (currentMilestone == 10000) {
                addGameLog("Population milestone: 10,000 citizens!");
            } else if (currentMilestone == 50000) {
                addGameLog("Population milestone: 50,000 citizens!");
            } else if (currentMilestone == 100000) {
                addGameLog("Population milestone: 100,000 citizens!");
            } else if (currentMilestone == 500000) {
                addGameLog("Population milestone: 500,000 citizens!");
            } else if (currentMilestone >= 1000000 && (currentMilestone % 1000000) == 0) {
                addGameLog("Population milestone: %d Million citizens!",
                           currentMilestone / 1000000);
            } else if (currentMilestone > 0) {
                addGameLog("Population milestone: %d citizens", currentMilestone);
            }

            lastMilestone = currentMilestone;
        }

        /* Check for city class changes */
        if (CityClass > lastCityClass) {
            addGameLog("City upgraded to %s!", GetCityClassName());
            lastCityClass = CityClass;
        }

        /* Removed: artificial fund injection not in original game */
    }

}

/* DoDisasters/SetFire now in s_disast.c */



/* GetBoatDis kept here for WinTown sprite system compatibility */
int GetBoatDis(void) {
    int dist, mx, my, dx, dy, i;
    SimSprite *sprite;

    dist = 99999;
    mx = (SMapX << 4) + 8;
    my = (SMapY << 4) + 8;

    for (i = 0; i < MAX_SPRITES; i++) {
        sprite = GetSpriteByIndex(i);
        if (!sprite) continue;
        if (sprite->type != SPRITE_SHIP || sprite->frame == 0) continue;
        dx = sprite->x + sprite->x_hot - mx;
        if (dx < 0) dx = -dx;
        dy = sprite->y + sprite->y_hot - my;
        if (dy < 0) dy = -dy;
        dx += dy;
        if (dx < dist) dist = dx;
    }
    return dist;
}


void MapScan(int x1, int x2, int y1, int y2) {
    int x, y;

    for (x = x1; x < x2; x++) {
        for (y = 0; y < WORLD_Y; y++) {
            if (CChr = Map[x][y]) {
                CChr9 = CChr & LOMASK;
                if (CChr9 >= FLOOD) {
                    SMapX = x;
                    SMapY = y;
                    if (CChr9 < ROADBASE) {
                        if (CChr9 >= FIREBASE) {
                            FirePop++;
                            if (!(Rand16() & 3)) DoFire();
                            continue;
                        }
                        if (CChr9 < RADTILE) DoFlood();
                        else DoRadTile();
                        continue;
                    }

                    if (NewPower && (CChr & CONDBIT))
                        SetZPower();

                    if ((CChr9 >= ROADBASE) &&
                        (CChr9 < POWERBASE)) {
                        DoRoad();
                        continue;
                    }

                    if (CChr & ZONEBIT) {
                        DoZone();
                        continue;
                    }

                    if ((CChr9 >= RAILBASE) &&
                        (CChr9 < RESBASE)) {
                        DoRail();
                        continue;
                    }
                    if ((CChr9 >= SOMETINYEXP) &&
                        (CChr9 <= LASTTINYEXP))
                        Map[x][y] = RUBBLE + (Rand16() & 3) + BULLBIT;
                }
            }
        }
    }
}

/* Unified population calculation functions */

/* Calculate city population using standard WiNTown formula */
QUAD CalculateCityPopulation(int resPop, int comPop, int indPop) {
    QUAD result;

    if (resPop < 0) resPop = 0;
    if (comPop < 0) comPop = 0;
    if (indPop < 0) indPop = 0;

    result = ((QUAD)resPop + ((QUAD)(comPop + indPop) * 8L)) * 20L;

    if (result < 0) result = 0;

    return result;
}

/* Calculate total population for game mechanics */
int CalculateTotalPopulation(int resPop, int comPop, int indPop) {
    long result;

    if (resPop < 0) resPop = 0;
    if (comPop < 0) comPop = 0;
    if (indPop < 0) indPop = 0;

    result = (long)(resPop / 8) + comPop + indPop;

    if (result > 32767) result = 32767;
    if (result < 0) result = 0;

    return (int)result;
}

void SetTileZone(int x, int y, int tile, int isZone) {
    if (!BOUNDS_CHECK(x, y)) return;
    if (isZone)
        setMapTile(x, y, tile, ZONEBIT, TILE_SET_REPLACE | TILE_SET_FLAGS, "SetTileZone");
    else
        setMapTile(x, y, tile, ZONEBIT, TILE_SET_REPLACE | TILE_CLEAR_FLAGS, "SetTileZone");
}


/* Check if coordinates are within map bounds */
int TestBounds(int x, int y) {
    return BOUNDS_CHECK(x, y);
}

/* Timer ID for simulation */
#define SIM_TIMER_ID 1

/* Timer interval in milliseconds - faster to match original game timing */
#define SIM_TIMER_INTERVAL 100

/* External function declaration for the UI update */
extern void UpdateSimulationMenu(HWND hwnd, int speed);

/* Global timer ID to track between function calls */
static UINT SimTimerID = 0;

void SetSimulationSpeed(HWND hwnd, int speed) {
    /* Update the simulation speed */
    SimSpeed = speed;

    /* Update UI (menu checkmarks) */
    UpdateSimulationMenu(hwnd, speed);

    /* If paused, stop the timer */
    if (speed == SPEED_PAUSED) {
        SimPaused = 1;
        if (SimTimerID) {
            KillTimer(hwnd, SIM_TIMER_ID);
            SimTimerID = 0;
        }
    } else {
        /* Otherwise, make sure the timer is running */
        SimPaused = 0;
        if (!SimTimerID) {
            SimTimerID = (UINT)SetTimer(hwnd, SIM_TIMER_ID, SIM_TIMER_INTERVAL, NULL);

            /* Check for timer creation failure */
            if (!SimTimerID) {
                MessageBox(hwnd, "Failed to create simulation timer", "Error",
                           MB_ICONERROR | MB_OK);
            }
        }
    }
}

/* Cleanup simulation timer when program exits */
void CleanupSimTimer(HWND hwnd) {
    if (SimTimerID) {
        KillTimer(hwnd, SIM_TIMER_ID);
        SimTimerID = 0;
    }
}