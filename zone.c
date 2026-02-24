/* zone.c - Zone processing for WiNTown
 * Based on original WiNTown code from WiNTownLegacy project
 */

#include "sim.h"
#include "tiles.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* External log functions */
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* Population table values for different zone types */
/* Use global zone base constants from sim.h: RZB=265, CZB=436, IZB=625 */

/* Some definitions needed by zone.c - matches simulation.h values */
#define HOSPITALBASE 400  /* Hospital base */
#define FOOTBALLBASE 950  /* Football stadium */

/* Note: Other zone base values all come from simulation.h */

/* Zone bit flags */
#define ALLBITS 0xFFFF
#define RESBIT 0x0001
#define COMBIT 0x0002
#define INDBIT 0x0004

/* ComRate is declared in sim.h as quarter size */

/* Population calculation cache - simple optimization */
#define POP_CACHE_SIZE 512
static short resPopCache[POP_CACHE_SIZE];
static short comPopCache[POP_CACHE_SIZE];
static short indPopCache[POP_CACHE_SIZE];
static int cacheInitialized = 0;

/* Forward declarations */
static void DoResidential(int x, int y);
static void DoCommercial(int x, int y);
static void DoIndustrial(int x, int y);
static void DoHospChur(int x, int y);
static void DoSPZ(int x, int y);
/* SetSmoke function is now external from animation.c */
static int EvalLot(int x, int y);
static void BuildHouse(int x, int y, int value);
static void ResPlop(int x, int y, int den, int value);
static void ComPlop(int x, int y, int den);
static void IndPlop(int x, int y, int den);
static int ZonePlop(int x, int y, int base);
static int GetCRVal(int x, int y);
static void DoResIn(int pop, int value);
static void DoComIn(int pop, int value);
static void DoIndIn(int pop, int value);

/* Calculate population in a residential zone - optimized */
int calcResPop(int zone) {
    int index;
    short CzDen;
    int result;
    
    /* Sanity check the input */
    if (zone < RESBASE || zone > LASTZONE) {
        return 0;  /* Invalid zone tile */
    }
    
    /* Use simple cache for common cases */
    index = zone - RESBASE;
    if (index >= 0 && index < POP_CACHE_SIZE && cacheInitialized) {
        if (resPopCache[index] != 0) {
            return resPopCache[index];
        }
    }
    
    /* Additional overflow protection */
    if (zone < RZB || zone - RZB > 32767) {
        return 0;  /* Value too large for safe calculation */
    }
    
    /* Use original RZPop algorithm from s_zone.c */
    /* Note: calculation must use RZB (265) as base, not RESBASE (240) */
    CzDen = (((zone - RZB) / 9) % 4);
    result = ((CzDen << 3) + 16);  /* Optimize: CzDen * 8 = CzDen << 3 */
    
    /* Ensure result is within reasonable bounds */
    if (result < 0 || result > 1000) {
        return 0;  /* Overflow detected */
    }
    
    /* Cache the result if valid index */
    if (index >= 0 && index < POP_CACHE_SIZE) {
        resPopCache[index] = (short)result;
        cacheInitialized = 1;
    }
    
    return result;
}

/* Calculate population in a commercial zone - optimized */
int calcComPop(int zone) {
    int index;
    short CzDen;
    int result;
    
    /* Sanity check the input */
    if (zone < COMBASE || zone > LASTZONE) {
        return 0;  /* Invalid zone tile */
    }
    
    /* Use original CZPop algorithm from s_zone.c */
    if (zone == COMCLR) return (0);
    
    /* Use simple cache for common cases */
    index = zone - RESBASE;
    if (index >= 0 && index < POP_CACHE_SIZE && cacheInitialized) {
        if (comPopCache[index] != 0) {
            return comPopCache[index];
        }
    }
    
    /* Additional overflow protection */
    if (zone - COMBASE > 32767) {
        return 0;  /* Value too large for safe calculation */
    }
    
    CzDen = (((zone - CZB) / 9) % 5) + 1;
    result = CzDen;
    
    /* Ensure result is within reasonable bounds */
    if (result < 0 || result > 100) {
        return 0;  /* Overflow detected */
    }
    
    /* Cache the result if valid index */
    if (index >= 0 && index < POP_CACHE_SIZE) {
        comPopCache[index] = (short)result;
    }
    
    return result;
}

/* Calculate population in an industrial zone - optimized */
int calcIndPop(int zone) {
    int index;
    short CzDen;
    int result;
    
    /* Sanity check the input */
    if (zone < INDBASE || zone > LASTZONE) {
        return 0;  /* Invalid zone tile */
    }
    
    /* Use original IZPop algorithm from s_zone.c */
    if (zone == INDCLR) return (0);
    
    /* Use simple cache for common cases */
    index = zone - RESBASE;
    if (index >= 0 && index < POP_CACHE_SIZE && cacheInitialized) {
        if (indPopCache[index] != 0) {
            return indPopCache[index];
        }
    }
    
    /* Additional overflow protection */
    if (zone - INDBASE > 32767) {
        return 0;  /* Value too large for safe calculation */
    }
    
    CzDen = (((zone - IZB) / 9) % 4) + 1;
    result = CzDen;
    
    /* Ensure result is within reasonable bounds */
    if (result < 0 || result > 50) {
        return 0;  /* Overflow detected */
    }
    
    /* Cache the result if valid index */
    if (index >= 0 && index < POP_CACHE_SIZE) {
        indPopCache[index] = (short)result;
    }
    
    return result;
}
static void IncROG(int amount);
static void DoResOut(int pop, int value, int x, int y);
static void DoComOut(int pop, int x, int y);
static void DoIndOut(int pop, int x, int y);
static int EvalRes(int traf);
static int EvalCom(int traf);
static int EvalInd(int traf);
static int DoFreePop(int x, int y);
static void SetZPower(int x, int y);
/* Using global calcResPop, calcComPop, calcIndPop from simulation.h */

static int ZoneRandom(int range) {
    return rand() % range;
}

static short Rand16(void) {
    return (short)(rand() & 0xFFFF);
}

static short Rand16Signed(void) {
    return (short)(rand() & 0xFFFF);
}

static int Rand(int range) {
    return rand() % (range + 1);
}

/* Main zone processing function - based on original WiNTown code */
void DoZone(int Xloc, int Yloc, int pos) {
    /* First check if this is a zone center */
    if (!(Map[Yloc][Xloc] & ZONEBIT)) {
        return;
    }

    /* Set global position variables for this zone */
    SMapX = Xloc;
    SMapY = Yloc;

    /* Do special processing based on zone type */
    if (pos >= RESBASE) {
        if (pos < COMBASE) {
            /* Check for hospitals and churches first - they're in residential range but not residential! */
            if (pos == HOSPITAL || pos == CHURCH) {
                DoHospChur(Xloc, Yloc);
                return;
            }
            /* Residential zone */
            SetZPower(Xloc, Yloc);
            DoResidential(Xloc, Yloc);
            return;
        }

        if (pos < INDBASE) {
            /* Commercial zone */
            SetZPower(Xloc, Yloc);
            DoCommercial(Xloc, Yloc);
            return;
        }

        if (pos < PORTBASE) {
            /* Industrial zone */
            SetZPower(Xloc, Yloc);
            DoIndustrial(Xloc, Yloc);
            return;
        }
    }

    /* Handle special zones */
    if (pos < HOSPITALBASE || pos > FOOTBALLBASE) {
        switch (pos) {
        case POWERPLANT:
        case NUCLEAR:
        case PORT:
        case AIRPORT:
            SetZPower(Xloc, Yloc);
            break;
        }
    }

    /* Handle police and fire stations specifically - they're in the hospital range but need special processing */
    if (pos == POLICESTATION || pos == FIRESTATION) {
        DoSPZ(Xloc, Yloc);
        return;
    }

    if (pos >= HOSPITALBASE && pos <= FOOTBALLBASE) {
        DoHospChur(Xloc, Yloc);
        return;
    }

    /* Special zones */
    DoSPZ(Xloc, Yloc);
}

/* Process hospital/church zone */
static void DoHospChur(int x, int y) {
    short z;
    int zonePowered;

    if (!(Map[y][x] & ZONEBIT)) {
        return;
    }

    SetZPower(x, y);
    
    /* Check if zone has power */
    zonePowered = (Map[y][x] & POWERBIT) != 0;

    if (CityTime & 3) {
        return;
    }

    z = Map[y][x] & LOMASK;

    if (z == HOSPITAL) {
        /* Add hospital population to census directly */
        if (zonePowered) {
            ResPop += 30;
        } else {
            ResPop += 5; /* Even unpowered hospitals have some population */
        }

        /* Also increment trade zone count on some cycles */
        if (ZoneRandom(20) < 10) {
            IndZPop++;
        }
        return;
    }

    if (z == CHURCH) {
        if (zonePowered) {
            ResPop += 10;
        } else {
            ResPop += 2;
        }

        if (ZoneRandom(20) < 10) {
            ResZPop++;
        }
    }
}

/* Process special zone (stadiums, coal plants, etc) */
static void DoSPZ(int x, int y) {
    short z;

    if (!(Map[y][x] & ZONEBIT)) {
        return;
    }

    SetZPower(x, y);

    /* Only process every 16th time */
    if (CityTime & 15) {
        return;
    }

    z = Map[y][x] & LOMASK;

    /* Handle special case of coal power plant */
    if (z == POWERPLANT) {
        SetSmoke(x, y);
        return;
    }

    /* Handle special case of stadium */
    if (z == STADIUM) {
        int xpos;
        int ypos;

        xpos = (x - 1) + ZoneRandom(3);
        ypos = (y - 1) + ZoneRandom(3);

        /* Add stadium population to census directly */
        ComPop += 50; /* Stadiums contribute to commercial population */

        /* Additional random chance to increase commercial zone */
        if (ZoneRandom(5) == 1) {
            ComZPop += 1;
        }
        return;
    }

    /* Handle special case of nuclear power */
    if (z == NUCLEAR) {
        NuclearPop++;
        
        /* Check for nuclear meltdown based on difficulty level */
        if (DisastersEnabled && ZoneRandom(DifficultyMeltdownRisk[GameLevel]) == 0) {
            /* Trigger nuclear meltdown disaster */
            addGameLog("CRITICAL: Nuclear power plant meltdown detected!");
            makeMeltdown();
        }
        return;
    }
    
    /* Handle police station */
    if (z == POLICESTATION) {
        int effect;
        
        /* Police count managed by census - no need to duplicate here */
        
        /* Police effectiveness calculated by scanner.c using smoothing algorithm */
        /* Mark station location for scanner.c to process */
        if (Map[y][x] & POWERBIT) {
            effect = PoliceEffect;
        } else {
            effect = PoliceEffect >> 1;  /* Half effect without power */
        }
        
        /* Check for road access - police need roads to patrol */
        if (!FindPRoad()) {
            effect = effect >> 1;  /* Half effect without road access */
        }
        
        /* Set base effect at station location - scanner.c will smooth this */
        PoliceMap[y >> 2][x >> 2] = effect;
        
        /* Debug logging */
        addDebugLog("POLICE: Added %d to map at (%d,%d) -> quarter (%d,%d), value now %d", 
                   effect, x, y, x >> 2, y >> 2, PoliceMap[y >> 2][x >> 2]);
        
        /* Cap the value to prevent overflow */
        if (PoliceMap[y >> 2][x >> 2] > 250) {
            PoliceMap[y >> 2][x >> 2] = 250;
        }
        
        
        
        return;
    }
    
    /* Handle fire station */
    if (z == FIRESTATION) {
        int effect;
        
        /* Fire station count managed by census - no need to duplicate here */
        
        /* Fire station effectiveness calculated by scanner.c using smoothing algorithm */
        /* Mark station location for scanner.c to process */
        if (Map[y][x] & POWERBIT) {
            effect = FireEffect;
        } else {
            effect = FireEffect >> 1;  /* Half effect without power */
        }
        
        /* Check for road access - fire trucks need roads */
        if (!FindPRoad()) {
            effect = effect >> 1;  /* Half effect without road access */
        }
        
        /* Set base effect at station location - scanner.c will smooth this */
        FireStMap[y >> 2][x >> 2] = effect;
        
        /* Cap the value to prevent overflow */
        if (FireStMap[y >> 2][x >> 2] > 250) {
            FireStMap[y >> 2][x >> 2] = 250;
        }
        
        
        return;
    }
}

/* Industrial pollution handler */
/* Forward declaration of the SetSmoke function from animation.c */
extern void SetSmoke(int x, int y);
/* Forward declaration of FindPRoad from traffic.c */
extern int FindPRoad(void);

/* Process industrial zone */
static void DoIndustrial(int x, int y) {
    short zone;
    int tpop, zscore, TrfGood;
    int zonePowered;

    zone = Map[y][x];
    if (!(zone & ZONEBIT))
        return;

    SetZPower(x, y);
    zonePowered = (Map[y][x] & POWERBIT) != 0;
    SetSmoke(x, y);

    IndZPop++;
    tpop = calcIndPop(zone & LOMASK);
    IndPop += tpop;

    SMapX = x;
    SMapY = y;

    if (tpop > Rand(5))
        TrfGood = MakeTraffic(2);
    else
        TrfGood = 1;

    if (TrfGood == -1) {
        DoIndOut(tpop, x, y);
        return;
    }

    if (!(Rand16() & 7)) {
        zscore = IValve + EvalInd(TrfGood);
        if (!zonePowered) zscore = -500;

        if ((zscore > -350) &&
            (((short)(zscore - 26380)) > Rand16Signed())) {
            DoIndIn(tpop, Rand16() & 1);
            return;
        }
        if ((zscore < 350) &&
            (((short)(zscore + 26380)) < Rand16Signed()))
            DoIndOut(tpop, x, y);
    }
}

static void DoCommercial(int x, int y) {
    short zone;
    int tpop, zscore, locvalve, value, TrfGood;
    int zonePowered;

    zone = Map[y][x];
    if (!(zone & ZONEBIT))
        return;

    SetZPower(x, y);
    zonePowered = (Map[y][x] & POWERBIT) != 0;

    ComZPop++;
    tpop = calcComPop(zone & LOMASK);
    ComPop += tpop;

    SMapX = x;
    SMapY = y;

    if (tpop > Rand(5))
        TrfGood = MakeTraffic(1);
    else
        TrfGood = 1;

    if (TrfGood == -1) {
        value = GetCRVal(x, y);
        DoComOut(tpop, x, y);
        return;
    }

    if (!(Rand16() & 7)) {
        locvalve = EvalCom(TrfGood);
        zscore = CValve + locvalve;
        if (!zonePowered) zscore = -500;

        if (TrfGood &&
            (zscore > -350) &&
            (((short)(zscore - 26380)) > Rand16Signed())) {
            value = GetCRVal(x, y);
            DoComIn(tpop, value);
            return;
        }
        if ((zscore < 350) &&
            (((short)(zscore + 26380)) < Rand16Signed())) {
            value = GetCRVal(x, y);
            DoComOut(tpop, x, y);
        }
    }
}

static void DoResidential(int x, int y) {
    short zone;
    short tileId;
    int tpop, zscore, locvalve, value, TrfGood;
    int zonePowered;

    zone = Map[y][x];
    if (!(zone & ZONEBIT))
        return;

    SetZPower(x, y);
    zonePowered = (Map[y][x] & POWERBIT) != 0;

    ResZPop++;
    tileId = zone & LOMASK;

    if (tileId == FREEZ)
        tpop = DoFreePop(x, y);
    else
        tpop = calcResPop(tileId);

    ResPop += tpop;

    SMapX = x;
    SMapY = y;

    if (tpop > Rand(35))
        TrfGood = MakeTraffic(0);
    else
        TrfGood = 1;

    if (TrfGood == -1) {
        value = GetCRVal(x, y);
        DoResOut(tpop, value, x, y);
        return;
    }

    if ((tileId == FREEZ) || (!(Rand16() & 7))) {
        locvalve = EvalRes(TrfGood);
        zscore = RValve + locvalve;
        if (!zonePowered) zscore = -500;

        if ((zscore > -350) &&
            (((short)(zscore - 26380)) > Rand16Signed())) {
            value = GetCRVal(x, y);
            DoResIn(tpop, value);
            return;
        }
        if ((zscore < 350) &&
            (((short)(zscore + 26380)) < Rand16Signed())) {
            value = GetCRVal(x, y);
            DoResOut(tpop, value, x, y);
        }
    }
}

/* Calculate land value for a location - matches original WiNTown */
static int GetCRVal(int x, int y) {
    register short LVal;
    
    LVal = LandValueMem[SMapY >>1][SMapX >>1];
    LVal -= PollutionMem[SMapY >>1][SMapX >>1];
    if (LVal < 30) return (0);
    if (LVal < 80) return (1);
    if (LVal < 150) return (2);
    return (3);
}

/* Handle residential zone growth - matches original WiNTown */
static void DoResIn(int pop, int value) {
    short z;
    short currentTile;
    
    z = PollutionMem[SMapY >>1][SMapX >>1];
    if (z > 128) return;
    
    /* Check current tile - don't modify hospitals, churches, etc */
    currentTile = Map[SMapY][SMapX] & LOMASK;
    if (currentTile == HOSPITAL || currentTile == CHURCH) {
        addDebugLog("DoResIn: Skipping non-residential tile %d at %d,%d", currentTile, SMapX, SMapY);
        return;
    }
    
    if ((Map[SMapY][SMapX] & LOMASK) == FREEZ) {
        if (pop < 8) {
            BuildHouse(SMapX, SMapY, value);
            IncROG(1);
            return;
        }
        /* FREEZ tiles with high population should upgrade */
        if (PopDensity[SMapY >>1][SMapX >>1] > 64) {
            /* High density - upgrade from FREEZ to proper residential */
            ResPlop(SMapX, SMapY, 0, value);
            IncROG(8);
            return;
        }
        return;
    }
    if (pop < 40) {
        int density = (pop / 8) - 1;
        
        /* Allow negative densities like original - this is normal for low population */
        ResPlop(SMapX, SMapY, density, value);
        IncROG(8);
    }
}

/* Handle commercial zone growth - matches original WiNTown */
static void DoComIn(int pop, int value) {
    register short z;
    
    z = LandValueMem[SMapY >>1][SMapX >>1];
    z = z >>5;
    if (pop > z) return;
    
    if (pop < 6) {
        ComPlop(SMapX, SMapY, pop);
        IncROG(8);
    }
}

/* Handle industrial zone growth - matches original WiNTown */
static void DoIndIn(int pop, int value) {
    if (pop < 4) {
        IndPlop(SMapX, SMapY, pop);
        IncROG(8);
    }
}

/* Increment Rate of Growth map - matches original WiNTown */
static void IncROG(int amount) {
    RateOGMem[SMapY >> 3][SMapX >> 3] += (short)(amount << 2);
}

/* Handle residential zone decline */
static void DoResOut(int pop, int value, int x, int y) {
    int locX;
    int locY;
    int loc;
    int z;
    static short brdr[9] = {0,3,6,1,4,7,2,5,8};

    /* Follow original WiNTown behavior to keep 3x3 tiles consistent */
    if (!pop) {
        return;
    }

    if (pop > 16) {
        /* Reduce density/value by re-plopping a lower tier */
        ResPlop(x, y, ((pop - 24) / 8), value);
        IncROG(-8);
        return;
    }

    if (pop == 16) {
        IncROG(-8);
        /* Center becomes FREEZ with zone + bulldoze bits */
        setMapTile(x, y, FREEZ, ZONEBIT | BULLBIT, TILE_SET_REPLACE, "DoResOut-freez-center");

        /* Ring becomes low houses varying by land value */
        for (locX = x - 1; locX <= x + 1; locX++) {
            for (locY = y - 1; locY <= y + 1; locY++) {
                if (locX >= 0 && locX < WORLD_X && locY >= 0 && locY < WORLD_Y) {
                    if ((Map[locY][locX] & LOMASK) != FREEZ) {
                        setMapTile(locX, locY, (LHTHR + value + ZoneRandom(2)), BULLBIT, TILE_SET_REPLACE, "DoResOut-freez-ring");
                    }
                }
            }
        }
        return;
    }

    /* pop < 16: shrink one border house back to FREEZ pattern */
    IncROG(-1);
    z = 0;
    for (locX = x - 1; locX <= x + 1; locX++) {
        for (locY = y - 1; locY <= y + 1; locY++) {
            if (locX >= 0 && locX < WORLD_X && locY >= 0 && locY < WORLD_Y) {
                loc = Map[locY][locX] & LOMASK;
                if ((loc >= LHTHR) && (loc <= HHTHR)) {
                    setMapTile(locX, locY, (FREEZ - 4 + brdr[z]), BULLBIT, TILE_SET_REPLACE, "DoResOut-shrink");
                    return;
                }
            }
            z++;
        }
    }
}

/* Handle commercial zone decline */
static void DoComOut(int pop, int x, int y) {
    short base;

    base = (Map[y][x] & LOMASK) - COMBASE;

    if (base == 0) {
        return;
    }

    if ((base > 0) && (ZoneRandom(8) == 0)) {
        /* Gradually decay */
        UpgradeTile(x, y, COMBASE + base - 1);
    }
}

/* Handle industrial zone decline */
static void DoIndOut(int pop, int x, int y) {
    short base;

    base = (Map[y][x] & LOMASK) - INDBASE;

    if (base == 0) {
        return;
    }

    if ((base > 0) && (ZoneRandom(8) == 0)) {
        /* Gradually decay */
        setMapTile(x, y, INDBASE + base - 1, 0, TILE_SET_PRESERVE, "DoIndOut-decline");
    }
}

/* Using the global calcResPop, calcComPop, calcIndPop functions defined earlier */

/* Build a house at a location */
static void BuildHouse(int x, int y, int value) {
    short z;
    short score;
    short xx;
    short yy;

    z = 0;

    /* Find best place to build a house */
    for (yy = -1; yy <= 1; yy++) {
        for (xx = -1; xx <= 1; xx++) {
            if (xx || yy) {
                score = EvalLot(x + xx, y + yy);
                if (score > z) {
                    z = score;
                }
            }
        }
    }

    /* If we found any valid location, build house there */
    if (z > 0) {
        /* Would build the house at the specified location */
    }
}

/* Place a residential zone */
static void ResPlop(int x, int y, int den, int value) {
    int base;
    
    addDebugLog("ResPlop: x=%d y=%d den=%d value=%d", x, y, den, value);
    
    if (value < 0) value = 0;
    if (value > 3) value = 3;
    
    /* Original formula: base is top-left of 3x3 block */
    base = (((value * 4) + den) * 9) + RZB - 4;
    
    ZonePlop(x, y, base);
}

/* Place a commercial zone */
static void ComPlop(int x, int y, int den) {
    int targetTile;
    
    /* Debug logging to track parameters */
    addDebugLog("ComPlop: x=%d y=%d den=%d", x, y, den);
    
    /* Use original WiNTown formula */
    /* base = (((Value * 5) + Den) * 9) + CZB - 4 */
    targetTile = (((0 * 5) + den) * 9) + CZB - 4;
    
    /* Debug logging for calculation */
    addDebugLog("ComPlop calc: (((0 * 5) + den=%d) * 9) + CZB=%d - 4 = %d", 
               den, CZB, targetTile);
    
    ZonePlop(x, y, targetTile);
}

/* Place an industrial zone */
static void IndPlop(int x, int y, int den) {
    int targetTile;
    
    /* Debug logging to track parameters */
    addDebugLog("IndPlop: x=%d y=%d den=%d", x, y, den);
    
    /* Use original WiNTown formula */
    /* base = (((Value * 4) + Den) * 9) + IZB - 4 */
    targetTile = (((0 * 4) + den) * 9) + IZB - 4;
    
    /* Debug logging for calculation */
    addDebugLog("IndPlop calc: (((0 * 4) + den=%d) * 9) + IZB=%d - 4 = %d", 
               den, IZB, targetTile);
    
    ZonePlop(x, y, targetTile);
}

/* Evaluate a lot for building a house */
static int EvalLot(int x, int y) {
    int score;
    short z;

    score = 1;

    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) {
        return -1;
    }

    z = Map[y][x] & LOMASK;

    if ((z >= RESBASE) && (z <= RESBASE + 8)) {
        score = 0;
    }

    if (score && (z != DIRT)) {
        score = 0;
    }

    if (!score) {
        return score;
    }

    /* Suitable place found! */
    return score;
}

/* Place a 3x3 zone on the map */
static int ZonePlop(int xpos, int ypos, int base) {
    int dx;
    int dy;
    int x;
    int y;
    int index;

    /* Bounds check */
    if (xpos < 0 || xpos >= WORLD_X || ypos < 0 || ypos >= WORLD_Y) {
        return 0;
    }

    /* Validate base range */
    if (base < 0 || base > LASTZONE) {
        addDebugLog("ERROR: Invalid zone base %d at %d,%d", base, xpos, ypos);
        return 0;
    }

    /* Lay out tiles in reading order; center is index 4 */
    index = 0;
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            x = xpos + dx;
            y = ypos + dy;
            if (!BOUNDS_CHECK(x, y)) {
                index++;
                continue;
            }
            if (dx == 0 && dy == 0) {
                setMapTile(x, y, base + index, ZONEBIT | BULLBIT | CONDBIT, TILE_SET_REPLACE, "ZonePlop-center");
            } else {
                setMapTile(x, y, base + index, BULLBIT | CONDBIT, TILE_SET_REPLACE, "ZonePlop-tile");
            }
            index++;
        }
    }

    return 1;
}

/* Evaluate residential zone desirability - matches original WiNTown */
static int EvalRes(int traf) {
    register short Value;
    
    if (traf < 0) return (-3000);
    
    Value = LandValueMem[SMapY >>1][SMapX >>1];
    Value -= PollutionMem[SMapY >>1][SMapX >>1];
    
    if (Value < 0) Value = 0;        /* Cap at 0 */
    else Value = Value <<5;
    
    if (Value > 6000) Value = 6000;  /* Cap at 6000 */
    
    Value = Value - 3000;
    return (Value);
}

/* Evaluate commercial zone desirability - matches original WiNTown */
static int EvalCom(int traf) {
    short Value;
    
    if (traf < 0) return (-3000);
    Value = ComRate[SMapY >>2][SMapX >>2];
    return (Value);
}

/* Evaluate industrial zone desirability - matches original WiNTown */
static int EvalInd(int traf) {
    if (traf < 0) return (-1000);
    return (0);
}

/* Count population of free houses */
static int DoFreePop(int x, int y) {
    int count;
    int xx;
    int yy;
    int xxx;
    int yyy;
    short z;

    count = 0;

    for (yy = -1; yy <= 1; yy++) {
        for (xx = -1; xx <= 1; xx++) {
            xxx = x + xx;
            yyy = y + yy;

            if (BOUNDS_CHECK(xxx, yyy)) {
                z = Map[yyy][xxx] & LOMASK;

                if (z >= LHTHR && z <= HHTHR) {
                    count++;
                }
            }
        }
    }

    return count;
}

/* Set zone power status - wrapper for unified power system */
static void SetZPower(int x, int y) {
    /* Power status is already set in the tile POWERBIT by DoPowerScan */
    /* No need to duplicate - the power system now works directly with POWERBIT */
}

