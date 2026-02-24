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
void Take2Census(void);

/* External cheat flags */
extern int disastersDisabled;

/* Map data */
/* RISC CPU Optimization: Align data structures on 8-byte boundaries */
#pragma pack(push, 8)
Byte PopDensity[WORLD_Y / 2][WORLD_X / 2];
Byte TrfDensity[WORLD_Y / 2][WORLD_X / 2];
Byte PollutionMem[WORLD_Y / 2][WORLD_X / 2];
Byte LandValueMem[WORLD_Y / 2][WORLD_X / 2];
Byte CrimeMem[WORLD_Y / 2][WORLD_X / 2];

/* Quarter-sized maps for effects */
Byte TerrainMem[WORLD_Y / 4][WORLD_X / 4];
Byte FireStMap[WORLD_Y / 4][WORLD_X / 4];
Byte FireRate[WORLD_Y / 4][WORLD_X / 4];
Byte PoliceMap[WORLD_Y / 4][WORLD_X / 4];
Byte PoliceMapEffect[WORLD_Y / 4][WORLD_X / 4];

/* Commercial development score */
short ComRate[WORLD_Y / 4][WORLD_X / 4];

/* Rate of growth memory */
short RateOGMem[ROGMEM_Y][ROGMEM_X];
#pragma pack(pop)

/* Runtime simulation state */
int SimSpeed = SPEED_MEDIUM;
int SimSpeedMeta = 0;
int SimPaused = 1;
int CityTime = 0;
int CityYear = 1900;
int CityMonth = 0;
QUAD TotalFunds = 5000;
int TaxRate = 7;      /* City tax rate 0-20% */
int SkipCensusReset = 0;  /* Flag to skip census reset after loading a scenario */
int DebugCensusReset = 0; /* Debug counter for tracking census resets */
int PrevResPop = 0;       /* Debug tracker for last residential population value */
int PrevCityPop = 0;      /* Debug tracker for last city population value */

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
/* CityTax variable removed - use TaxRate directly */

/* Counters */
int Scycle = 0;
int Fcycle = 0;
int Spdcycle = 0;

/* Game evaluation */
int CityYes = 0;
int CityNo = 0;
QUAD CityPop = 0;
int CityScore = 500;
int deltaCityScore = 0;
int CityClass = 0;
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
int UnpwrdZCnt = 0;
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
int PoliceEffect = 1000;  /* Back to original values for debugging */
int FireEffect = 1000;    /* Back to original values for debugging */
int TrafficAverage = 0;
int PollutionAverage = 0;
int CrimeAverage = 0;
int LVAverage = 0;

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
int FloodCnt = 0;         /* Flood countdown timer - floods spread while > 0, recede when 0 */
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
static short CChr;
static short CChr9;

/* Random number generator - Windows compatible */
void RandomlySeedRand(void) {
    srand((unsigned int)GetTickCount());
}

/* Public random number function - available to other modules */
int SimRandom(int range) {
    return (rand() % range);
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

            LandValueMem[y][x] = (Byte)value;
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
    TaxRate = 7;

    /* Initial game state */
    CityScore = 500;
    DisasterEvent = 0;
    DisasterWait = 0;

    /* Initialize evaluation system */
    EvalInit();

    /* Initialize budget system */
    InitBudget();

    /* Initialize sprite system */
    InitSprites();

    /* Generate a random disaster wait period */
    DisasterWait = SimRandom(51) + 49;

    /* Force an initial census to populate values */
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
    /* Main simulation logic */

    /* Perform different actions based on the cycle position (mod 16) */
    switch (mod16) {
    case 0:
        /* Increment simulation cycle counter as in original */
        if (++Scycle > 1023) {
            Scycle = 0;
        }
        
        /* Increment time, check for disasters, process valve changes */
        DoTimeStuff();

        if (!(Scycle & 1))
            SetValves();

        if (ValveFlag) {
            ValveFlag = 0;
            UpdateToolbar();
        }

        /* Power scan moved to case 11 to avoid duplicate calls */

        /* Process tile animations */
        AnimateTiles();

        /* Original WiNTown message system */
        SendMessages();
        doMessage();
        break;

    case 1:
        /* Clear census before starting a new scan cycle */
        ClearCensus();
        /* FALLTHROUGH to start map scanning */

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
        if (FloodCnt > 0) FloodCnt--;

        /* Update CityPop only when population counters change */
        /* Population counters are now updated directly during map scanning */
        CityPop = CalculateCityPopulation(ResPop, ComPop, IndPop);

        /* Update police coverage display map immediately after stations are scanned */
        /* This ensures the minimap shows current coverage without waiting for CrimeScan */
        {
            int x, y, totalCoverage = 0, maxCoverage = 0;
            for (x = 0; x < WORLD_X / 4; x++) {
                for (y = 0; y < WORLD_Y / 4; y++) {
                    PoliceMapEffect[y][x] = PoliceMap[y][x];
                    if (PoliceMap[y][x] > 0) {
                        totalCoverage += PoliceMap[y][x];
                        if (PoliceMap[y][x] > maxCoverage) {
                            maxCoverage = PoliceMap[y][x];
                        }
                    }
                }
            }
            if (totalCoverage > 0) {
                addDebugLog("POLICE MAP: Total=%d Max=%d Stations=%d", totalCoverage, maxCoverage, PolicePop);
            } else if (PolicePop > 0) {
                addDebugLog("POLICE MAP: No coverage despite %d stations - PoliceEffect=%d", PolicePop, PoliceEffect);
            }
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
            CityEvaluation();    /* Evaluate city conditions */
        }
        break;

    case 10:
        DecTrafficMap();
        DecROGMem();

        /* Calculate traffic average periodically */
        if ((Scycle % 4) == 0) {
            CalcTrafficAverage();

            /* Log traffic */
            if (TrafficAverage > 100) {
                addDebugLog("Traffic level: %d (Heavy)", TrafficAverage);
            } else if (TrafficAverage > 50) {
                addDebugLog("Traffic level: %d (Moderate)", TrafficAverage);
            }
        }

        /* CityPop is updated in case 9 when population counters change - no need to recalculate */

        /* Run animations for smoother motion */
        AnimateTiles();
        break;

    case 11:
        /* Process power grid updates */
        DoPowerScan();

        /* Generate transportation sprites */
        GenerateTrains();
        GenerateShips();
        GenerateAircraft();
        GenerateHelicopters();

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
        /* Process pollution spread (at a reduced rate) */
        if ((Scycle % 16) == 12) {
            PTLScan(); /* Do pollution, terrain, and land value */

            /* Log pollution and land value */
            addDebugLog("Pollution average: %d", PollutionAverage);
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
        /* Process crime spread (at a reduced rate) */
        if ((Scycle % 4) == 1) {
            CrimeScan(); /* Do crime map analysis */

            /* Log crime level */
            if (CrimeAverage > 100) {
                addGameLog("WARNING: Crime level is very high (%d)", CrimeAverage);
            } else if (CrimeAverage > 50) {
                addDebugLog("Crime average: %d (Moderate)", CrimeAverage);
            }
        }
        break;

    case 14:
        /* Process population density (at a reduced rate) */
        if ((Scycle % 16) == 14) {
            PopDenScan();   /* Do population density scan */
            FireAnalysis(); /* Update fire protection effect */
        }
        break;

    case 15:
        /* Process fire analysis and disasters (at a reduced rate) */
        if ((Scycle % 4) == 3) {
            /* Process fire spreading - skip if disasters are disabled */
            if (!disastersDisabled) {
                spreadFire();
            }

            /* Log fire information */
            if (FirePop > 0) {
                addDebugLog("Active fires: %d", FirePop);
            }
        }

        /* Process disasters */
        addDebugLog("DISASTER CHECK: Event=%d, Disabled=%d, Case=15", DisasterEvent, disastersDisabled);
        if (DisasterEvent && !disastersDisabled) {
            /* Process scenario-based disasters */
            addDebugLog("CALLING scenarioDisaster()");
            scenarioDisaster();
        } else {
            /* Debug why scenarioDisaster is not being called */
            static int debugCount = 0;
            debugCount++;
            if (debugCount % 100 == 0) { /* Log every 100 cycles */
                addDebugLog("scenarioDisaster NOT called: Event=%d, Disabled=%d (cycle %d)", 
                           DisasterEvent, disastersDisabled, debugCount);
            }
        }
        
        /* Process scenario evaluation */
        if (ScenarioID > 0 && ScoreWait > 0) {
            ScoreWait--;
            if (ScoreWait == 0) {
                /* Trigger scenario evaluation */
                DoScenarioScore(ScoreType);
            }
        }

        /* Process tile animations again at the end of the cycle */
        AnimateTiles();
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

    /* Manage disasters */
    if (disastersDisabled || !DisastersEnabled) {
        /* Clear any active disasters if cheat is enabled or disasters disabled */
        DisasterEvent = 0;
        DisasterWait = 0;
    } else if (DisasterEvent) {
        DisasterWait = 0;
    } else {
        if (DisasterWait > 0) {
            DisasterWait--;
        } else {
            /* Check for random disasters based on difficulty level */
            if (SimRandom(DifficultyDisasterChance[GameLevel]) == 0) {
                /* Choose a disaster type */
                switch (SimRandom(8)) {
                case 0:
                case 1:
                    makeFlood();
                    break;
                case 2:
                case 3:
                    makeFire(SimRandom(WORLD_X), SimRandom(WORLD_Y));
                    break;
                case 4:
                case 5:
                    makeFire(SimRandom(WORLD_X), SimRandom(WORLD_Y));
                    break;
                case 6:
                    makeTornado();
                    break;
                case 7:
                    doEarthquake();
                    break;
                }
            }
            /* Reset disaster wait period using difficulty-based timing */
            DisasterWait = SimRandom(DifficultyDisasterChance[GameLevel] / 10) + (DifficultyDisasterChance[GameLevel] / 20);
        }
    }
}

void SetValves(void) {
    static short TaxTable[21] = {
        200, 150, 120, 100, 80, 50, 30, 0, -10, -40, -100,
        -150, -200, -250, -300, -350, -400, -450, -500, -550, -600
    };
    float Employment, Migration, Births, LaborBase, IntMarket;
    float Rratio, Cratio, Iratio, temp;
    float NormResPop, PjResPop, PjComPop, PjIndPop;
    float hCom, hInd, hRes;
    short z;
    int prevIdx;

    prevIdx = HISTLEN / 2 - 2;
    hCom = (float)ComHis[prevIdx];
    hInd = (float)IndHis[prevIdx];
    hRes = (float)ResHis[prevIdx];

    NormResPop = (float)(ResPop / 8);

    if (NormResPop != 0)
        Employment = (hCom + hInd) / NormResPop;
    else
        Employment = 1.0f;

    Migration = NormResPop * (Employment - 1.0f);
    Births = NormResPop * 0.02f;
    PjResPop = NormResPop + Migration + Births;

    temp = hCom + hInd;
    if (temp != 0)
        LaborBase = hRes / temp;
    else
        LaborBase = 1.0f;
    if (LaborBase > 1.3f) LaborBase = 1.3f;
    if (LaborBase < 0) LaborBase = 0.0f;

    IntMarket = (NormResPop + (float)ComPop + (float)IndPop) / 3.7f;
    PjComPop = IntMarket * LaborBase;

    temp = 1.0f;
    switch (GameLevel) {
    case 0: temp = 1.2f; break;
    case 1: temp = 1.1f; break;
    case 2: temp = 0.98f; break;
    }

    PjIndPop = (float)IndPop * LaborBase * temp;
    if (PjIndPop < 5.0f) PjIndPop = 5.0f;

    if (NormResPop != 0) Rratio = PjResPop / NormResPop;
    else Rratio = 1.3f;
    if (ComPop != 0) Cratio = PjComPop / (float)ComPop;
    else Cratio = PjComPop;
    if (IndPop != 0) Iratio = PjIndPop / (float)IndPop;
    else Iratio = PjIndPop;

    if (Rratio > 2.0f) Rratio = 2.0f;
    if (Cratio > 2.0f) Cratio = 2.0f;
    if (Iratio > 2.0f) Iratio = 2.0f;

    z = (short)TaxRate + (short)GameLevel;
    if (z > 20) z = 20;
    Rratio = ((Rratio - 1) * 600) + TaxTable[z];
    Cratio = ((Cratio - 1) * 600) + TaxTable[z];
    Iratio = ((Iratio - 1) * 600) + TaxTable[z];

    if (Rratio > 0 && RValve < 2000) RValve += (short)Rratio;
    if (Rratio < 0 && RValve > -2000) RValve += (short)Rratio;
    if (Cratio > 0 && CValve < 1500) CValve += (short)Cratio;
    if (Cratio < 0 && CValve > -1500) CValve += (short)Cratio;
    if (Iratio > 0 && IValve < 1500) IValve += (short)Iratio;
    if (Iratio < 0 && IValve > -1500) IValve += (short)Iratio;

    if (RValve > 2000) RValve = 2000;
    if (RValve < -2000) RValve = -2000;
    if (CValve > 1500) CValve = 1500;
    if (CValve < -1500) CValve = -1500;
    if (IValve > 1500) IValve = 1500;
    if (IValve < -1500) IValve = -1500;

    if (ResCap && RValve > 0) RValve = 0;
    if (ComCap && CValve > 0) CValve = 0;
    if (IndCap && IValve > 0) IValve = 0;

    ValveFlag = 1;
}

void ClearCensus(void) {
    /* CRITICAL: Save previous population values BEFORE resetting */
    QUAD oldCityPop;
    int oldResPop;
    int oldComPop;
    int oldIndPop;
    int oldTotalPop;

    oldCityPop = CityPop;
    oldResPop = ResPop;
    oldComPop = ComPop;
    oldIndPop = IndPop;
    oldTotalPop = TotalPop;

    /* DEBUG: Track previous population values */
    PrevResPop = ResPop;
    PrevCityPop = (int)CityPop;

    /* Log infrastructure counts before resetting */
    addDebugLog("Infrastructure: Roads=%d Rail=%d Fire=%d Police=%d", RoadTotal, RailTotal, FirePop,
                PolicePop);
    addDebugLog("Special zones: Stadium=%d Port=%d Airport=%d Nuclear=%d", StadiumPop, PortPop,
                APortPop, NuclearPop);
    addDebugLog("Power: Powered=%d Unpowered=%d", PwrdZCnt, UnpwrdZCnt);

    /* Infrastructure counts always need resetting */
    RoadTotal = 0;
    RailTotal = 0;
    FirePop = 0;
    PolicePop = 0;
    StadiumPop = 0;
    PortPop = 0;
    APortPop = 0;
    NuclearPop = 0;
    /* Power zone counts are managed exclusively by DoPowerScan() - do not reset here */

    /* Fire and police maps are now cleared in case 1 before map scanning */

    /* Reset temporary census variables using unified function */
    /* This prevents display flicker during census calculation */
    ResetCensusCounters();

    /* DEBUG: Increment counter to track census resets */
    DebugCensusReset++;
}

void TakeCensus(void) {
    /* Store city statistics in the history arrays */
    int i;
    QUAD newCityPop;
    char debugMsg[256];
    int growth;
    int decline;

    /* CRITICAL: Make sure we have valid population counts even if they're small */
    if (ResPop <= 0 && (Map[4][4] & LOMASK) == RESBASE) {
        ResPop = 50;  /* Set a minimal initial population */
    }

    /* Calculate total population - use unified function */
    TotalPop = CalculateTotalPopulation(ResPop, ComPop, IndPop);

    /* Sanity check population values before calculation */
    if (ResPop < 0 || ComPop < 0 || IndPop < 0) {
        wsprintf(debugMsg, "WARNING: Negative zone population detected! R=%d C=%d I=%d\n", 
                 ResPop, ComPop, IndPop);
        OutputDebugString(debugMsg);
        
        /* Clamp to zero */
        if (ResPop < 0) ResPop = 0;
        if (ComPop < 0) ComPop = 0;
        if (IndPop < 0) IndPop = 0;
    }

    /* Calculate new city population using unified function */
    newCityPop = CalculateCityPopulation(ResPop, ComPop, IndPop);

    /* Sanity check the result */
    if (newCityPop < 0) {
        wsprintf(debugMsg, "ERROR: Negative CityPop calculated! R=%d C=%d I=%d -> %ld\n", 
                 ResPop, ComPop, IndPop, newCityPop);
        OutputDebugString(debugMsg);
        addDebugLog("ERROR: Negative population! R=%d C=%d I=%d", ResPop, ComPop, IndPop);
        
        /* Try to recover by using a simpler formula */
        newCityPop = (QUAD)(ResPop + ComPop + IndPop) * 100L;
    }

    /* Update CityPop based on current zone populations */
    if (ResPop > 0 || ComPop > 0 || IndPop > 0) {
        CityPop = newCityPop;
    } else {
        /* No zones means no population */
        CityPop = 0;
    }

    /* CRITICAL: Make sure we have some population value if there are zones */
    if (CityPop < 100 && (PwrdZCnt > 0 || UnpwrdZCnt > 0)) {
        /* Set a minimum population if there are any zones */
        CityPop = 100;
    }

    /* DEBUG: Output current population state */
    {
        wsprintf(debugMsg,
                 "DEBUG Population: Res=%d Com=%d Ind=%d Total=%d CityPop=%d (Prev=%d) Resets=%d\n",
                 ResPop, ComPop, IndPop, TotalPop, (int)CityPop, PrevCityPop, DebugCensusReset);
        OutputDebugString(debugMsg);

        /* Add to log window */
        addDebugLog("Census: R=%d C=%d I=%d Total=%d CityPop=%d", ResPop, ComPop, IndPop, TotalPop,
                    (int)CityPop);
    }

    /* Determine city class based on population */
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

    /* Only set minimum population if we actually have zones */
    if (CityPop == 0 && (ResPop > 0 || ComPop > 0 || IndPop > 0)) {
        /* Calculate from zone counts using unified function */
        CityPop = CalculateCityPopulation(ResPop, ComPop, IndPop);

        /* If still zero despite having zones, use minimum value */
        if (CityPop == 0) {
            CityPop = 100; /* Minimum village size */
            CityClass = 0; /* Village */
        }
    }
    /* If no zones, population should remain 0 */

    /* Track population changes for growth rate calculations */
    if (CityPop > PrevCityPop) {
        /* Population is growing */

        growth = (int)(CityPop - PrevCityPop);
        wsprintf(debugMsg, "GROWTH: Population increased from %d to %d (+%d)\n", PrevCityPop,
                 (int)CityPop, growth);
        OutputDebugString(debugMsg);

        /* Add to log window - use regular log for population growth */
        if (growth > 100) {
            addGameLog("Population growing: +%d citizens", growth);
        }
    } else if (CityPop < PrevCityPop) {
        /* Population is declining */

        decline = (int)(PrevCityPop - CityPop);
        wsprintf(debugMsg, "DECLINE: Population decreased from %d to %d (-%d)\n", PrevCityPop,
                 (int)CityPop, decline);
        OutputDebugString(debugMsg);

        /* Add to log window - use regular log for population decline */
        if (decline > 100) {
            addGameLog("Population declining: -%d citizens", decline);
        }
    }

    /* Save current value for next comparison */
    PrevCityPop = (int)CityPop;

    /* Update graph history */
    for (i = 0; i < HISTLEN / 2 - 1; i++) {
        ResHis[i] = ResHis[i + 1];
        ComHis[i] = ComHis[i + 1];
        IndHis[i] = IndHis[i + 1];
        CrimeHis[i] = CrimeHis[i + 1];
        PollutionHis[i] = PollutionHis[i + 1];
        MoneyHis[i] = MoneyHis[i + 1];
    }

    /* Update miscellaneous history */
    for (i = 0; i < MISCHISTLEN / 2 - 1; i++) {
        MiscHis[i] = MiscHis[i + 1];
    }

    /* Record current values in history */
    ResHis[HISTLEN / 2 - 1] = ResPop / 8;
    ComHis[HISTLEN / 2 - 1] = ComPop;
    IndHis[HISTLEN / 2 - 1] = IndPop;
    CrimeHis[HISTLEN / 2 - 1] = CrimeAverage;
    PollutionHis[HISTLEN / 2 - 1] = PollutionAverage;
    MoneyHis[HISTLEN / 2 - 1] = (short)(TotalFunds / 100);

    /* Note: MiscHis will be updated in the specific subsystem implementations */
}

void Take2Census(void) {
    int x;
    short Res2HisMax, Com2HisMax, Ind2HisMax;

    Res2HisMax = 0;
    Com2HisMax = 0;
    Ind2HisMax = 0;

    for (x = HISTLEN / 2 - 2; x >= HISTLEN / 4; x--) {
        ResHis[x + 1] = ResHis[x];
        if (ResHis[x] > Res2HisMax) Res2HisMax = ResHis[x];
        ComHis[x + 1] = ComHis[x];
        if (ComHis[x] > Com2HisMax) Com2HisMax = ComHis[x];
        IndHis[x + 1] = IndHis[x];
        if (IndHis[x] > Ind2HisMax) Ind2HisMax = IndHis[x];
        CrimeHis[x + 1] = CrimeHis[x];
        PollutionHis[x + 1] = PollutionHis[x];
        MoneyHis[x + 1] = MoneyHis[x];
    }

    ResHis[HISTLEN / 4] = ResPop / 8;
    ComHis[HISTLEN / 4] = ComPop;
    IndHis[HISTLEN / 4] = IndPop;
    CrimeHis[HISTLEN / 4] = CrimeHis[0];
    PollutionHis[HISTLEN / 4] = PollutionHis[0];
    MoneyHis[HISTLEN / 4] = MoneyHis[0];
}

void DecROGMem(void) {
    int x, y;
    short z;

    for (y = 0; y < ROGMEM_Y; y++) {
        for (x = 0; x < ROGMEM_X; x++) {
            z = RateOGMem[y][x];
            if (z == 0) continue;
            if (z > 0) z--;
            else z++;
            RateOGMem[y][x] = z;
        }
    }
}

static void FireZone(int x, int y, int ch) {
    int XYmax, dx, dy, tx, ty;

    ch = ch & LOMASK;
    if (ch < PORTBASE)
        XYmax = 2;
    else if (ch == AIRPORT)
        XYmax = 5;
    else
        XYmax = 4;

    for (dx = -1; dx < XYmax; dx++) {
        for (dy = -1; dy < XYmax; dy++) {
            tx = x + dx;
            ty = y + dy;
            if (!BOUNDS_CHECK(tx, ty)) continue;
            if ((Map[ty][tx] & LOMASK) >= ROADBASE)
                Map[ty][tx] |= BULLBIT;
        }
    }
}

static void DoFireScan(void) {
    static short DX[4] = {-1, 0, 1, 0};
    static short DY[4] = {0, -1, 0, 1};
    int z, Rate;
    short Xtem, Ytem, c;

    for (z = 0; z < 4; z++) {
        if (!(rand() & 7)) {
            Xtem = SMapX + DX[z];
            Ytem = SMapY + DY[z];
            if (BOUNDS_CHECK(Xtem, Ytem)) {
                c = Map[Ytem][Xtem];
                if (c & BURNBIT) {
                    if (c & ZONEBIT)
                        FireZone(Xtem, Ytem, c);
                    Map[Ytem][Xtem] = FIRE + (rand() & 3) + ANIMBIT;
                }
            }
        }
    }

    z = FireRate[SMapY >> 2][SMapX >> 2];
    Rate = 10;
    if (z) {
        Rate = 3;
        if (z > 20) Rate = 2;
        if (z > 100) Rate = 1;
    }
    if (SimRandom(Rate + 1) == 0)
        Map[SMapY][SMapX] = RUBBLE + (rand() & 3) + BULLBIT;
}

static void DoFloodScan(void) {
    static short DX[4] = {0, 1, 0, -1};
    static short DY[4] = {-1, 0, 1, 0};
    int z;
    short xx, yy, c, t;

    if (FloodCnt) {
        for (z = 0; z < 4; z++) {
            if (!(rand() & 7)) {
                xx = SMapX + DX[z];
                yy = SMapY + DY[z];
                if (BOUNDS_CHECK(xx, yy)) {
                    c = Map[yy][xx];
                    t = c & LOMASK;
                    if ((c & BURNBIT) || (c == 0) ||
                        ((t >= WOODS5) && (t < FLOOD))) {
                        if (c & ZONEBIT)
                            FireZone(xx, yy, c);
                        Map[yy][xx] = FLOOD + SimRandom(3);
                    }
                }
            }
        }
    } else {
        if (!(rand() & 15))
            Map[SMapY][SMapX] = DIRT;
    }
}

static void DoRadTile(void) {
    if (!(rand() & 4095))
        Map[SMapY][SMapX] = DIRT;
}

static short Rand16(void) {
    return (short)(rand() & 0xFFFF);
}

static int GetBoatDis(void) {
    int dist, mx, my, dx, dy, i;
    SimSprite *sprite;

    dist = 99999;
    mx = (SMapX << 4) + 8;
    my = (SMapY << 4) + 8;

    for (i = 0; i < MAX_SPRITES; i++) {
        sprite = GetSprite(i);
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

static int DoBridge(void) {
    static short HDx[7] = { -2,  2, -2, -1,  0,  1,  2 };
    static short HDy[7] = { -1, -1,  0,  0,  0,  0,  0 };
    static short HBRTAB[7] = {
        HBRDG1 | BULLBIT, HBRDG3 | BULLBIT, HBRDG0 | BULLBIT,
        RIVER, BRWH | BULLBIT, RIVER, HBRDG2 | BULLBIT };
    static short HBRTAB2[7] = {
        RIVER, RIVER, HBRIDGE | BULLBIT, HBRIDGE | BULLBIT, HBRIDGE | BULLBIT,
        HBRIDGE | BULLBIT, HBRIDGE | BULLBIT };
    static short VDx[7] = {  0,  1,  0,  0,  0,  0,  1 };
    static short VDy[7] = { -2, -2, -1,  0,  1,  2,  2 };
    static short VBRTAB[7] = {
        VBRDG0 | BULLBIT, VBRDG1 | BULLBIT, RIVER, BRWV | BULLBIT,
        RIVER, VBRDG2 | BULLBIT, VBRDG3 | BULLBIT };
    static short VBRTAB2[7] = {
        VBRIDGE | BULLBIT, RIVER, VBRIDGE | BULLBIT, VBRIDGE | BULLBIT,
        VBRIDGE | BULLBIT, VBRIDGE | BULLBIT, RIVER };
    int z, x, y, MPtem;

    if (CChr9 == BRWV) {
        if (!(Rand16() & 3) && (GetBoatDis() > 340)) {
            for (z = 0; z < 7; z++) {
                x = SMapX + VDx[z];
                y = SMapY + VDy[z];
                if (BOUNDS_CHECK(x, y))
                    if ((Map[y][x] & LOMASK) == (VBRTAB[z] & LOMASK))
                        Map[y][x] = VBRTAB2[z];
            }
        }
        return 1;
    }
    if (CChr9 == BRWH) {
        if (!(Rand16() & 3) && (GetBoatDis() > 340)) {
            for (z = 0; z < 7; z++) {
                x = SMapX + HDx[z];
                y = SMapY + HDy[z];
                if (BOUNDS_CHECK(x, y))
                    if ((Map[y][x] & LOMASK) == (HBRTAB[z] & LOMASK))
                        Map[y][x] = HBRTAB2[z];
            }
        }
        return 1;
    }

    if ((GetBoatDis() < 300) || !(Rand16() & 7)) {
        if (CChr9 & 1) {
            if (SMapX < (WORLD_X - 1))
                if (Map[SMapY][SMapX + 1] == CHANNEL) {
                    for (z = 0; z < 7; z++) {
                        x = SMapX + VDx[z];
                        y = SMapY + VDy[z];
                        if (BOUNDS_CHECK(x, y)) {
                            MPtem = Map[y][x];
                            if ((MPtem == CHANNEL) ||
                                ((MPtem & 15) == (VBRTAB2[z] & 15)))
                                Map[y][x] = VBRTAB[z];
                        }
                    }
                    return 1;
                }
            return 0;
        } else {
            if (SMapY > 0)
                if (Map[SMapY - 1][SMapX] == CHANNEL) {
                    for (z = 0; z < 7; z++) {
                        x = SMapX + HDx[z];
                        y = SMapY + HDy[z];
                        if (BOUNDS_CHECK(x, y)) {
                            MPtem = Map[y][x];
                            if (((MPtem & 15) == (HBRTAB2[z] & 15)) ||
                                (MPtem == CHANNEL))
                                Map[y][x] = HBRTAB[z];
                        }
                    }
                    return 1;
                }
            return 0;
        }
    }
    return 0;
}

static void DoRoadScan(void) {
    short Density, tden, z;
    static short DenTab[3] = {ROADBASE, LTRFBASE, HTRFBASE};

    RoadTotal++;

    if (RoadEffect < 30) {
        if (!(rand() & 511)) {
            if (!(CChr & CONDBIT)) {
                if (RoadEffect < (rand() & 31)) {
                    if (((CChr9 & 15) < 2) || ((CChr9 & 15) == 15))
                        Map[SMapY][SMapX] = RIVER;
                    else
                        Map[SMapY][SMapX] = RUBBLE + (rand() & 3) + BULLBIT;
                    return;
                }
            }
        }
    }

    if (!(CChr & BURNBIT)) {
        RoadTotal += 4;
        if (DoBridge()) return;
        return;
    }

    if (CChr9 == HROADPOWER || CChr9 == VROADPOWER)
        return;

    if (CChr9 < LTRFBASE) tden = 0;
    else if (CChr9 < HTRFBASE) tden = 1;
    else {
        RoadTotal++;
        tden = 2;
    }

    Density = TrfDensity[SMapY >> 1][SMapX >> 1] >> 6;
    if (Density > 1) Density--;
    if (tden != Density) {
        z = ((CChr9 - ROADBASE) & 15) + DenTab[Density];
        z |= (CChr & (MASKBITS & ~ANIMBIT));
        if (Density) z |= ANIMBIT;
        Map[SMapY][SMapX] = z;
    }
}

static void DoRailScan(void) {
    RailTotal++;

    if (RoadEffect < 30) {
        if (!(rand() & 511)) {
            if (!(CChr & CONDBIT)) {
                if (RoadEffect < (rand() & 31)) {
                    if (CChr9 < (RAILBASE + 2))
                        Map[SMapY][SMapX] = RIVER;
                    else
                        Map[SMapY][SMapX] = RUBBLE + (rand() & 3) + BULLBIT;
                    return;
                }
            }
        }
    }
}

void MapScan(int x1, int x2, int y1, int y2) {
    int x, y;

    if (x1 < 0 || x2 > WORLD_X || y1 < 0 || y2 > WORLD_Y)
        return;

    for (y = y1; y < y2; y++) {
        for (x = x1; x < x2; x++) {
            CChr = Map[y][x];
            if (!CChr) continue;

            CChr9 = CChr & LOMASK;
            if (CChr9 < FLOOD) continue;

            SMapX = x;
            SMapY = y;

            if (CChr9 < ROADBASE) {
                if (CChr9 >= FIREBASE) {
                    FirePop++;
                    if (!(rand() & 3))
                        DoFireScan();
                    continue;
                }
                if (CChr9 < RADTILE)
                    DoFloodScan();
                else
                    DoRadTile();
                continue;
            }

            if (CChr9 < POWERBASE) {
                DoRoadScan();
                continue;
            }

            if (CChr & ZONEBIT) {
                DoZone(x, y, CChr9);
                continue;
            }

            if (CChr9 >= RAILBASE && CChr9 < RESBASE) {
                DoRailScan();
                continue;
            }

            if (CChr9 >= SOMETINYEXP && CChr9 <= LASTTINYEXP)
                Map[y][x] = RUBBLE + (rand() & 3) + BULLBIT;
        }
    }
}

/* Unified population calculation functions */

/* Calculate city population using standard WiNTown formula */
QUAD CalculateCityPopulation(int resPop, int comPop, int indPop) {
    QUAD result;
    
    /* Sanity check inputs */
    if (resPop < 0) resPop = 0;
    if (comPop < 0) comPop = 0;
    if (indPop < 0) indPop = 0;
    
    result = (QUAD)resPop + ((QUAD)(comPop + indPop) * 8L);
    
    /* Ensure non-negative result */
    if (result < 0) {
        result = 0;
    }
    
    return result;
}

/* Calculate total population for game mechanics */
int CalculateTotalPopulation(int resPop, int comPop, int indPop) {
    long result;
    
    /* Sanity check inputs */
    if (resPop < 0) resPop = 0;
    if (comPop < 0) comPop = 0;
    if (indPop < 0) indPop = 0;
    
    /* Use standard formula: TotalPop = (Res + Com + Ind) * 8 */
    result = (long)(resPop + comPop + indPop) * 8L;
    
    /* Clamp to int range */
    if (result > 32767) result = 32767;
    if (result < 0) result = 0;
    
    return (int)result;
}

/* Add population to the appropriate temporary counter */
void AddToZonePopulation(int zoneType, int amount) {
    switch (zoneType) {
        case ZONE_TYPE_RESIDENTIAL:
            ResPop += amount;
            break;
        case ZONE_TYPE_COMMERCIAL:
            ComPop += amount;
            break;
        case ZONE_TYPE_INDUSTRIAL:
            IndPop += amount;
            break;
        default:
            /* Invalid zone type - do nothing */
            break;
    }
}

/* Reset temporary census counters */
void ResetCensusCounters(void) {
    ResPop = 0;
    ComPop = 0;
    IndPop = 0;
}

/* Set tile with power status in one operation */
void SetTileWithPower(int x, int y, int tile, int powered) {
    if (!BOUNDS_CHECK(x, y)) {
        return;
    }
    
    if (powered) {
        setMapTile(x, y, tile, POWERBIT, TILE_SET_REPLACE | TILE_SET_FLAGS, "SetTileWithPower-powered");
    } else {
        setMapTile(x, y, tile, 0, TILE_SET_REPLACE, "SetTileWithPower-unpowered");
    }
}

/* Set tile zone status */
void SetTileZone(int x, int y, int tile, int isZone) {
    if (!BOUNDS_CHECK(x, y)) {
        return;
    }
    
    if (isZone) {
        setMapTile(x, y, tile, ZONEBIT, TILE_SET_REPLACE | TILE_SET_FLAGS, "SetTileZone-zone");
    } else {
        setMapTile(x, y, tile, ZONEBIT, TILE_SET_REPLACE | TILE_CLEAR_FLAGS, "SetTileZone-clear");
    }
}

/* Upgrade tile preserving existing flags */
void UpgradeTile(int x, int y, int newTile) {
    if (!BOUNDS_CHECK(x, y)) {
        return;
    }
    
    setMapTile(x, y, newTile, 0, TILE_SET_PRESERVE, "UpgradeTile");
}

/* Set rubble tile */
void SetRubbleTile(int x, int y) {
    int rubbleType;
    
    if (!BOUNDS_CHECK(x, y)) {
        return;
    }
    
    rubbleType = (SimRandom(4) + RUBBLE) | BULLBIT;
    setMapTile(x, y, rubbleType, 0, TILE_SET_REPLACE, "SetRubbleTile");
}

/* Unified power status management function */
/* Set power status without updating zone counts - for power scan algorithm */
void SetPowerStatusOnly(int x, int y, int powered) {
    /* Bounds check */
    if (!BOUNDS_CHECK(x, y)) {
        return;
    }
    
    /* Update the tile power bit directly */
    if (powered) {
        setMapTile(x, y, 0, POWERBIT, TILE_SET_FLAGS, "SetPowerStatusOnly-powered");
    } else {
        setMapTile(x, y, 0, POWERBIT, TILE_CLEAR_FLAGS, "SetPowerStatusOnly-unpowered");
    }
}

void UpdatePowerStatus(int x, int y, int powered) {
    int wasPowered;
    
    /* Bounds check */
    if (!BOUNDS_CHECK(x, y)) {
        return;
    }
    
    /* Check current power status before changing */
    wasPowered = (Map[y][x] & POWERBIT) != 0;
    
    /* Update the tile power bit directly */
    if (powered) {
        setMapTile(x, y, 0, POWERBIT, TILE_SET_FLAGS, "UpdatePowerStatus-powered");
    } else {
        setMapTile(x, y, 0, POWERBIT, TILE_CLEAR_FLAGS, "UpdatePowerStatus-unpowered");
    }
    
    /* Power zone counts are managed exclusively by DoPowerScan() in power.c */
    /* Individual updates should not modify zone counts to avoid conflicts */
}

int GetPValue(int x, int y) {
    /* Get power status at a given position */
    if (BOUNDS_CHECK(x, y)) {
        return (Map[y][x] & POWERBIT) != 0;
    }
    return 0;
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