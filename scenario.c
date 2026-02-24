/* scenarios.c - Scenario implementation for WiNTown
 * Based on original WiNTown code from WiNTownLegacy project
 */

#include "sim.h"
#include "sprite.h"
#include "notify.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "gdifix.h"
#include "assets.h"
#include "resource.h"

/* External log functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* External cheat flags */
extern int disastersDisabled;

/* Scenario variables */
short ScenarioID = 0;    /* Current scenario ID (0 = none) */
short DisasterEvent = 0; /* Current disaster type */
short DisasterWait = 0;  /* Countdown to next disaster */
short ScoreType = 0;     /* Score type for scenario */
short ScoreWait = 0;     /* Score wait for scenario */

/* External functions needed from other modules */
extern int SimRandom(int range);     /* From simulation.c */
extern int loadFile(char *filename); /* From main.c */

/* Disaster functions from disasters.c */
extern void doEarthquake(void);          /* Earthquake disaster */
extern void makeFlood(void);             /* Flooding disaster */
extern void makeFire(int x, int y);      /* Fire disaster at specific location */
extern void makeMonster(void);           /* Monster attack disaster */
extern void makeTornado(void);           /* Tornado disaster */
extern void makeExplosion(int x, int y); /* Explosion at specific location */
extern void makeMeltdown(void);          /* Nuclear meltdown disaster */

/* External functions from main.c */
extern void ForceFullCensus(void); /* Census calculation function */

/* External variables */
extern HWND hwndMain;
extern char cityFileName[MAX_PATH];

/* Disaster timing arrays - matches original WiNTown */
static short DisTab[9] = {0, 2, 10, 5, 20, 3, 5, 5, 2 * 48};
static short ScoreWaitTab[9] = {0,      30 * 48, 5 * 48, 5 * 48, 10 * 48,
                                5 * 48, 10 * 48, 5 * 48, 10 * 48};

/* Load scenario based on ID */
int loadScenario(int scenarioId) {
    char *name = NULL;
    int startYear = 1900;
    int startFunds = 5000;

    /* Reset city filename */
    cityFileName[0] = '\0';

    GameLevel = 0; /* Set game level to easy */

    /* Validate scenario ID range */
    if ((scenarioId < 1) || (scenarioId > 8)) {
        addGameLog("WARNING: Invalid scenario ID! Using Dullsville (1) instead.");
        scenarioId = 1;
    }

    /* Set scenario parameters */
    switch (scenarioId) {
    case 1:
        name = "Dullsville";
        ScenarioID = 1;
        startYear = 1900;
        startFunds = 5000;
        break;
    case 2:
        name = "San Francisco";
        ScenarioID = 2;
        startYear = 1906;
        startFunds = 20000;
        break;
    case 3:
        name = "Hamburg";
        ScenarioID = 3;
        startYear = 1944;
        startFunds = 20000;
        break;
    case 4:
        name = "Bern";
        ScenarioID = 4;
        startYear = 1965;
        startFunds = 20000;
        break;
    case 5:
        name = "Tokyo";
        ScenarioID = 5;
        startYear = 1957;
        startFunds = 20000;
        break;
    case 6:
        name = "Detroit";
        ScenarioID = 6;
        startYear = 1972;
        startFunds = 20000;
        break;
    case 7:
        name = "Boston";
        ScenarioID = 7;
        startYear = 2010;
        startFunds = 20000;
        break;
    case 8:
        name = "Rio de Janeiro";
        ScenarioID = 8;
        startYear = 2047;
        startFunds = 20000;
        break;
    default:
        name = "Dullsville";
        ScenarioID = 1;
        startYear = 1900;
        startFunds = 5000;
        break;
    }

    /* Set city name */
    strcpy(cityFileName, name);

    /* Set year and month */
    CityYear = startYear;
    CityMonth = 0;

    /* Load scenario map data from embedded resources */
    switch (scenarioId) {
    case 1:
        if (!loadScenarioFromResource(IDR_SCENARIO_DULLSVILLE, "Dullsville")) return 0;
        break;
    case 2:
        if (!loadScenarioFromResource(IDR_SCENARIO_SANFRANCISCO, "San Francisco")) return 0;
        break;
    case 3:
        if (!loadScenarioFromResource(IDR_SCENARIO_HAMBURG, "Hamburg")) return 0;
        break;
    case 4:
        if (!loadScenarioFromResource(IDR_SCENARIO_BERN, "Bern")) return 0;
        break;
    case 5:
        if (!loadScenarioFromResource(IDR_SCENARIO_TOKYO, "Tokyo")) return 0;
        break;
    case 6:
        if (!loadScenarioFromResource(IDR_SCENARIO_DETROIT, "Detroit")) return 0;
        break;
    case 7:
        if (!loadScenarioFromResource(IDR_SCENARIO_BOSTON, "Boston")) return 0;
        break;
    case 8:
        if (!loadScenarioFromResource(IDR_SCENARIO_RIO, "Rio de Janeiro")) return 0;
        break;
    default:
        if (!loadScenarioFromResource(IDR_SCENARIO_DULLSVILLE, "Dullsville")) return 0;
        break;
    }

    /* Set funds AFTER loading file to prevent corruption */
    TotalFunds = startFunds;

    /* Update window title with scenario name */
    {
        char winTitle[256];
        wsprintf(winTitle, "WiNTown - Scenario: %s", name);
        SetWindowText(hwndMain, winTitle);

        /* Log the scenario load */
        addGameLog("SCENARIO: %s loaded", name);
        addGameLog("Year: %d, Initial funds: $%d", startYear, startFunds);
    }

    /* Set disaster info */
    DisasterEvent = ScenarioID;
    /* Ensure bounds checking before array access */
    if (DisasterEvent >= 0 && DisasterEvent < 9) {
        DisasterWait = DisTab[DisasterEvent];
        ScoreWait = ScoreWaitTab[DisasterEvent];
    } else {
        DisasterWait = 0;
        ScoreWait = 0;
    }
    ScoreType = ScenarioID;

    /* Force scenarios to start running */
    SetGameSpeed(SPEED_MEDIUM);

    return 1;
}

/* Process scenario disasters */
void scenarioDisaster(void) {
    static int disasterX, disasterY;
    static int debugCounter = 0;

    /* Add periodic debug logging */
    debugCounter++;
    if (debugCounter % 50 == 0) { /* Log every 50 calls */
        addDebugLog("scenarioDisaster called: Event=%d, Wait=%d, Disabled=%d", 
                   DisasterEvent, DisasterWait, disastersDisabled);
    }

    /* Early return if no disaster or disasters are disabled */
    if (DisasterEvent == 0 || disastersDisabled) {
        return;
    }

    /* Ensure DisasterEvent is in valid range */
    if (DisasterEvent < 1 || DisasterEvent > 8) {
        DisasterEvent = 0;
        return;
    }

    switch (DisasterEvent) {
    case 1: /* Dullsville */
        break;
    case 2: /* San Francisco */
        if (DisasterWait <= 1) {
            addGameLog("SCENARIO EVENT: San Francisco earthquake is happening now!");
            addGameLog("Significant damage reported throughout the city!");
            doEarthquake();
        } else {
            /* Debug logging for earthquake countdown */
            static int lastReported = -1;
            if (DisasterWait != lastReported) {
                addDebugLog("San Francisco earthquake countdown: %d", DisasterWait);
                lastReported = DisasterWait;
            }
        }
        break;
    case 3: /* Hamburg */
        /* Drop fire bombs */
        if (DisasterWait % 10 == 0) {
            disasterX = SimRandom(WORLD_X);
            disasterY = SimRandom(WORLD_Y);
            if (DisasterWait == 20) {
                addGameLog("SCENARIO EVENT: Hamburg firebombing attack has begun!");
                addGameLog("Multiple fires are breaking out across the city!");
            }
            addGameLog("Explosion reported at %d,%d", disasterX, disasterY);
            addDebugLog("Firebomb at coordinates %d,%d, %d bombs remaining", disasterX, disasterY,
                        DisasterWait / 10);
            makeExplosion(disasterX, disasterY);
        }
        break;
    case 4: /* Bern */
        /* No disaster in Bern scenario */
        break;
    case 5: /* Tokyo */
        if (DisasterWait <= 1) {
            addGameLog("SCENARIO EVENT: Tokyo monster attack is underway!");
            addGameLog("Giant creature is destroying buildings in its path!");
            makeMonster();
        }
        break;
    case 6: /* Detroit */
        /* Detroit gets tornado to simulate urban decay */
        if (DisasterWait <= 1) {
            addGameLog("SCENARIO EVENT: Detroit tornado has touched down!");
            addGameLog("Severe weather compounds the city's problems!");
            makeTornado();
        }
        break;
    case 7: /* Boston */
        if (DisasterWait <= 1) {
            addGameLog("SCENARIO EVENT: Boston nuclear meltdown is happening!");
            addGameLog("Nuclear power plant has suffered a catastrophic failure!");
            makeMeltdown();
        }
        break;
    case 8: /* Rio */
        if ((DisasterWait % 24) == 0) {
            if (DisasterWait == 48) {
                addGameLog("SCENARIO EVENT: Rio flood disaster is starting!");
                addGameLog("Ocean levels are rising - coastal areas at risk!");
            } else {
                addGameLog("Flood waters continue to spread!");
            }
            addDebugLog("Flood event triggered - %d hours until flood peak", DisasterWait);
            makeFlood();
        }
        break;
    default:
        /* Invalid disaster event */
        DisasterEvent = 0;
        return;
    }

    if (DisasterWait) {
        DisasterWait--;

        /* Log when disaster ends */
        if (DisasterWait == 0) {
            addGameLog("Scenario disaster has ended");
            addDebugLog("Scenario disaster ID %d complete", DisasterEvent);
            DisasterEvent = 0;
        }
    } else {
        DisasterEvent = 0;
    }
}

/* Evaluate scenario goals and determine win/lose status */
DoScenarioScore(int scoreType) {
    int win = 0;
    int score = 0;
    char message[256];
    
    /* Only evaluate if we have an active scenario and score wait has expired */
    if (ScenarioID == 0 || ScoreWait > 0) {
        return 0;
    }
    
    /* Check victory conditions based on scenario */
    switch (ScenarioID) {
        case 1: /* Dullsville - reach City Class 4 or higher */
            if (CityClass >= 4) {
                win = 1;
                score = 500;
                strcpy(message, "Congratulations! You've transformed Dullsville into a thriving metropolis!");
            } else {
                strcpy(message, "You failed to develop Dullsville into a major city. Try building more zones!");
            }
            break;
            
        case 2: /* San Francisco - reach City Class 4 or higher after earthquake */
            if (CityClass >= 4) {
                win = 1;
                score = 500;
                strcpy(message, "Amazing! You've rebuilt San Francisco after the devastating earthquake!");
            } else {
                strcpy(message, "San Francisco remains in ruins. Focus on rebuilding residential and commercial areas!");
            }
            break;
            
        case 3: /* Hamburg - reach City Class 4 or higher after bombing */
            if (CityClass >= 4) {
                win = 1;
                score = 500;
                strcpy(message, "Excellent! Hamburg has risen from the ashes of war!");
            } else {
                strcpy(message, "Hamburg couldn't recover from the bombing. Try faster reconstruction!");
            }
            break;
            
        case 4: /* Bern - keep traffic average below 80 */
            if (TrafficAverage < 80) {
                win = 1;
                score = 500;
                strcpy(message, "Perfect! You've solved Bern's traffic problems with excellent planning!");
            } else {
                strcpy(message, "Traffic congestion remains a problem in Bern. Build more roads and rails!");
            }
            break;
            
        case 5: /* Tokyo - achieve City Score above 500 after monster attack */
            if (CityScore > 500) {
                win = 1;
                score = 500;
                strcpy(message, "Incredible! Tokyo has recovered and thrived after the monster attack!");
            } else {
                strcpy(message, "Tokyo couldn't fully recover from the monster attack. Focus on rebuilding!");
            }
            break;
            
        case 6: /* Detroit - reduce crime average below 60 */
            if (CrimeAverage < 60) {
                win = 1;
                score = 500;
                strcpy(message, "Outstanding! You've cleaned up Detroit and made it safe again!");
            } else {
                strcpy(message, "Crime remains too high in Detroit. Build more police stations!");
            }
            break;
            
        case 7: /* Boston - restore land value average above 120 after meltdown */
            if (LVAverage > 120) {
                win = 1;
                score = 500;
                strcpy(message, "Remarkable! Boston has recovered from the nuclear disaster!");
            } else {
                strcpy(message, "Land values remain low after the meltdown. Clean up radiation and rebuild!");
            }
            break;
            
        case 8: /* Rio - achieve City Score above 500 despite flooding */
            if (CityScore > 500) {
                win = 1;
                score = 500;
                strcpy(message, "Brilliant! Rio thrives despite the coastal flooding challenges!");
            } else {
                strcpy(message, "Rio couldn't overcome the flooding problems. Try building away from water!");
            }
            break;
            
        default:
            /* Unknown scenario */
            return 0;
    }
    
    /* Log the result */
    if (win) {
        addGameLog("SCENARIO SUCCESS: %s", message);
        addGameLog("Final Score: %d", score);
        CityScore = score;
    } else {
        addGameLog("SCENARIO FAILED: %s", message);
        addGameLog("Final Score: -200");
        CityScore = -200;
    }
    
    /* Show notification dialog with result */
    {
        Notification notif;
        notif.id = win ? 7001 : 7002; /* Custom scenario result IDs */
        notif.type = win ? NOTIF_MILESTONE : NOTIF_WARNING;
        strcpy(notif.title, win ? "Scenario Complete - YOU WIN!" : "Scenario Failed");
        strcpy(notif.message, message);
        strcpy(notif.explanation, win ? "Congratulations! You have successfully completed this scenario challenge." : "The scenario objectives were not met. Review your strategy and try again.");
        strcpy(notif.advice, win ? "You can try other scenarios or continue playing in sandbox mode." : "Focus on the specific requirements mentioned in the scenario description.");
        notif.hasLocation = 0;
        notif.priority = win ? 1 : 2;
        notif.timestamp = GetTickCount();
        CreateNotificationDialog(&notif);
    }
    
    /* Reset scenario */
    ScenarioID = 0;
    ScoreWait = 0;
    DisasterEvent = 0;
    DisasterWait = 0;
    return 1;
}
