#ifndef _SIM_BRIDGE_H
#define _SIM_BRIDGE_H

#include "../sim.h"
#include "../sprite.h"
#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#undef ALLBITS
#define ALLBITS (~LOMASK)

#define BLBNBIT (BULLBIT|BURNBIT)
#define PROBNUM 10
#define SHI 4

#define POWERMAPROW ((WORLD_X + 15) / 16)
#define PWRMAPSIZE (POWERMAPROW * WORLD_Y)
#define POWERWORD(x) ((x) >>4)
#define SETPOWERBIT(x) (PowerMap[POWERWORD(x)] |= (1 << ((x) & 0xf)))
#define PWRSTKSIZE ((WORLD_X * WORLD_Y) / 4)

#define NMAPS 15
#define DYMAP 14
#define FIMAP 12
#define PDMAP 6
#define RGMAP 7
#define PLMAP 9
#define CRMAP 10
#define LVMAP 11
#define POMAP 13

extern short PowerMap[];
extern short CrashX, CrashY;
extern short CChr, CChr9;
extern short Rand16();
extern short Rand16Signed();
extern int Rand(int range);
extern int GetBoatDis();
extern int MoveMapSim(short MDir);
extern int BridgeSendMes(int msg, int x, int y);
extern DropFireBombs();
extern FireBomb();

static Byte tem[HWLDX][HWLDY];
static Byte tem2[HWLDX][HWLDY];
static short STem[SmX][SmY];
static Byte Qtem[QWX][QWY];

#define NoDisasters (!DisastersEnabled)
#define MakeExplosionAt(px, py) MakeExplosion((px) >> 4, (py) >> 4)
#define SendMesAt(msg, x, y) BridgeSendMes(msg, x, y)
#define ClearMes()
#define ChangeEval()
#define ChangeCensus()
#define drawCurrPercents()

#define SeedRand(s) srand((unsigned int)(s))
#define GenerateNewCity _orig_GenerateNewCity
#define GenerateSomeCity _orig_GenerateSomeCity
static char *CityFileName;
static struct { long tv_usec; long tv_sec; } start_time;
#define ckfree(p)
#define gettimeofday(a,b) 0
#define InitWillStuff()
#define ResetMapState()
#define ResetEditorState()
#define InvalidateEditors()
#define InvalidateMaps()
#define UpdateFunds()
#define Eval(x)
#define Kick()

#endif
