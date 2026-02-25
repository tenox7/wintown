/* zone.c - Zone processing for WiNTown
 * Based on original WiNTown code from WiNTownLegacy project
 */

#include "sim.h"
#include "tiles.h"
#include "sprite.h"
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
static void RepairZone(int cx, int cy, int zCent, int zSize);
/* SetSmoke function is now external from animation.c */
static int EvalLot(int x, int y);
static void BuildHouse(int x, int y, int value);
static void MakeHosp(void);
static void ResPlop(int x, int y, int den, int value);
static void ComPlop(int x, int y, int den, int value);
static void IndPlop(int x, int y, int den, int value);
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
static void DoComOut(int pop, int value);
static void DoIndOut(int pop, int value);
static int EvalRes(int traf);
static int EvalCom(int traf);
static int EvalInd(int traf);
int DoFreePop(int x, int y);
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

static void DoHospChur(int x, int y) {
    short z;

    if (!(Map[y][x] & ZONEBIT)) return;

    SetZPower(x, y);

    z = Map[y][x] & LOMASK;

    if (z == HOSPITAL) {
        HospPop++;
        if (!(CityTime & 15))
            RepairZone(x, y, HOSPITAL, 3);
        if (NeedHosp == -1) {
            if (!Rand(20))
                ZonePlop(x, y, RESBASE);
        }
        return;
    }

    if (z == CHURCH) {
        ChurchPop++;
        if (!(CityTime & 15))
            RepairZone(x, y, CHURCH, 3);
        if (NeedChurch == -1) {
            if (!Rand(20))
                ZonePlop(x, y, RESBASE);
        }
    }
}

static void MakeHosp(void) {
    if (NeedHosp > 0) {
        ZonePlop(SMapX, SMapY, HOSPITAL - 4);
        NeedHosp = 0;
        return;
    }
    if (NeedChurch > 0) {
        ZonePlop(SMapX, SMapY, CHURCH - 4);
        NeedChurch = 0;
    }
}

static void DrawStadium(int cx, int cy, int base) {
    int ix, iy;
    int z;
    z = base - 5;
    for (iy = cy - 1; iy < cy + 3; iy++)
        for (ix = cx - 1; ix < cx + 3; ix++)
            Map[iy][ix] = (z++) | BURNBIT | CONDBIT;
    Map[cy][cx] |= ZONEBIT | POWERBIT;
}

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

    switch (z) {
    case POWERPLANT:
        CoalPop++;
        if (!(CityTime & 7))
            RepairZone(x, y, POWERPLANT, 4);
        SetSmoke(x, y);
        return;

    case NUCLEAR:
        NuclearPop++;
        if (!(CityTime & 7))
            RepairZone(x, y, NUCLEAR, 4);
        if (DisastersEnabled && !Rand(DifficultyMeltdownRisk[GameLevel])) {
            makeMeltdown();
        }
        return;

    case FIRESTATION:
        FireStPop++;
        if (!(CityTime & 7))
            RepairZone(x, y, FIRESTATION, 3);
        {
            int effect;
            effect = (Map[y][x] & POWERBIT) ? FireEffect : (FireEffect >> 1);
            if (!FindPRoad())
                effect >>= 1;
            FireStMap[y >> 3][x >> 3] += effect;
        }
        return;

    case POLICESTATION:
        PolicePop++;
        if (!(CityTime & 7))
            RepairZone(x, y, POLICESTATION, 3);
        {
            int effect;
            effect = (Map[y][x] & POWERBIT) ? PoliceEffect : (PoliceEffect >> 1);
            if (!FindPRoad())
                effect >>= 1;
            PoliceMap[y >> 3][x >> 3] += effect;
        }
        return;

    case STADIUM:
        StadiumPop++;
        if (!(CityTime & 15))
            RepairZone(x, y, STADIUM, 4);
        if (Map[y][x] & POWERBIT) {
            if (!((CityTime + x + y) & 31)) {
                DrawStadium(x, y, FULLSTADIUM);
                Map[y][x + 1] = FOOTBALLGAME1 + ANIMBIT;
                Map[y + 1][x + 1] = FOOTBALLGAME2 + ANIMBIT;
            }
        }
        return;

    case FULLSTADIUM:
        StadiumPop++;
        if (!((CityTime + x + y) & 7))
            DrawStadium(x, y, STADIUM);
        return;

    case AIRPORT:
        APortPop++;
        if (!(CityTime & 7))
            RepairZone(x, y, AIRPORT, 6);
        if (Map[y][x] & POWERBIT) {
            if ((Map[y - 1][x + 1] & LOMASK) == RADAR)
                Map[y - 1][x + 1] = RADAR + ANIMBIT + CONDBIT + BURNBIT;
        } else {
            Map[y - 1][x + 1] = RADAR + CONDBIT + BURNBIT;
        }
        if (Map[y][x] & POWERBIT) {
            if (!SimRandom(5)) {
                GenerateAircraft();
            }
        }
        return;

    case PORT:
        PortPop++;
        if (!(CityTime & 15))
            RepairZone(x, y, PORT, 4);
        if ((Map[y][x] & POWERBIT) && GetSpriteByType(SPRITE_SHIP) == NULL) {
            GenerateShips();
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
        DoIndOut(tpop, Rand16() & 1);
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
            DoIndOut(tpop, Rand16() & 1);
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
        DoComOut(tpop, value);
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
            DoComOut(tpop, value);
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
            if ((!tpop) && (!(Rand16() & 3))) {
                MakeHosp();
                return;
            }
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

    z = PollutionMem[SMapY >>1][SMapX >>1];
    if (z > 128) return;

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

static void DoComIn(int pop, int value) {
    register short z;

    z = LandValueMem[SMapY >>1][SMapX >>1];
    z = z >>5;
    if (pop > z) return;

    if (pop < 5) {
        ComPlop(SMapX, SMapY, pop, value);
        IncROG(8);
    }
}

static void DoIndIn(int pop, int value) {
    if (pop < 4) {
        IndPlop(SMapX, SMapY, pop, value);
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

static void DoComOut(int pop, int value) {
    if (pop > 1) {
        ComPlop(SMapX, SMapY, pop - 2, value);
        IncROG(-8);
        return;
    }
    if (pop == 1) {
        ZonePlop(SMapX, SMapY, COMBASE);
        IncROG(-8);
    }
}

static void DoIndOut(int pop, int value) {
    if (pop > 1) {
        IndPlop(SMapX, SMapY, pop - 2, value);
        IncROG(-8);
        return;
    }
    if (pop == 1) {
        ZonePlop(SMapX, SMapY, INDCLR - 4);
        IncROG(-8);
    }
}

/* Using the global calcResPop, calcComPop, calcIndPop functions defined earlier */

static void BuildHouse(int x, int y, int value) {
    static short ZeX[9] = {0, -1, 0, 1, -1, 1, -1, 0, 1};
    static short ZeY[9] = {0, -1, -1, -1, 0, 0, 1, 1, 1};
    short z, score, hscore, BestLoc;
    int xx, yy;

    BestLoc = 0;
    hscore = 0;
    for (z = 1; z < 9; z++) {
        xx = x + ZeX[z];
        yy = y + ZeY[z];
        if (!BOUNDS_CHECK(xx, yy)) continue;
        score = EvalLot(xx, yy);
        if (score == 0) continue;
        if (score > hscore) {
            hscore = score;
            BestLoc = z;
        }
        if ((score == hscore) && !(Rand16() & 7))
            BestLoc = z;
    }
    if (!BestLoc) return;
    xx = x + ZeX[BestLoc];
    yy = y + ZeY[BestLoc];
    if (!BOUNDS_CHECK(xx, yy)) return;
    Map[yy][xx] = HOUSE + BNCNBIT + BULLBIT + Rand(2) + (value * 3);
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

static void ComPlop(int x, int y, int den, int value) {
    int base;

    base = (((value * 5) + den) * 9) + CZB - 4;
    ZonePlop(x, y, base);
}

static void IndPlop(int x, int y, int den, int value) {
    int base;

    base = (((value * 4) + den) * 9) + IZB - 4;
    ZonePlop(x, y, base);
}

static int EvalLot(int x, int y) {
    static short DX[4] = {0, 1, 0, -1};
    static short DY[4] = {-1, 0, 1, 0};
    short z, score;
    int xx, yy, i;

    if (!BOUNDS_CHECK(x, y)) return -1;

    z = Map[y][x] & LOMASK;
    if (z && ((z < RESBASE) || (z > RESBASE + 8)))
        return -1;

    score = 1;
    for (i = 0; i < 4; i++) {
        xx = x + DX[i];
        yy = y + DY[i];
        if (BOUNDS_CHECK(xx, yy) &&
            Map[yy][xx] &&
            ((Map[yy][xx] & LOMASK) <= LASTROAD))
            score++;
    }
    return score;
}

static int ZonePlop(int xpos, int ypos, int base) {
    int dx, dy, x, y, index;
    short tile;

    if (!BOUNDS_CHECK(xpos, ypos)) return 0;
    if (base < 0 || base > LASTZONE) return 0;

    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            x = xpos + dx;
            y = ypos + dy;
            if (!BOUNDS_CHECK(x, y)) continue;
            tile = Map[y][x] & LOMASK;
            if ((tile >= FLOOD) && (tile < ROADBASE))
                return 0;
        }
    }

    index = 0;
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            x = xpos + dx;
            y = ypos + dy;
            if (BOUNDS_CHECK(x, y))
                Map[y][x] = (base + index) | BNCNBIT;
            index++;
        }
    }
    Map[ypos][xpos] |= ZONEBIT | BULLBIT;

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
    Value = ComRate[SMapY >> 3][SMapX >> 3];
    return (Value);
}

/* Evaluate industrial zone desirability - matches original WiNTown */
static int EvalInd(int traf) {
    if (traf < 0) return (-1000);
    return (0);
}

/* Count population of free houses */
int DoFreePop(int x, int y) {
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

static void SetZPower(int x, int y) {
}

static void RepairZone(int cx, int cy, int zCent, int zSize) {
    int cnt, x, y, xx, yy;
    short ThCh;

    zSize--;
    cnt = 0;
    for (y = -1; y < zSize; y++) {
        for (x = -1; x < zSize; x++) {
            xx = cx + x;
            yy = cy + y;
            cnt++;
            if (!BOUNDS_CHECK(xx, yy)) continue;
            ThCh = Map[yy][xx];
            if (ThCh & ZONEBIT) continue;
            if (ThCh & ANIMBIT) continue;
            ThCh = ThCh & LOMASK;
            if ((ThCh < RUBBLE) || (ThCh >= ROADBASE))
                Map[yy][xx] = (short)(zCent - 3 - zSize + cnt + CONDBIT + BURNBIT);
        }
    }
}

