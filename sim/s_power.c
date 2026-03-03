#include "sim.h"


/* Power Scan */


int PowerStackNum;
char PowerStackX[PWRSTKSIZE], PowerStackY[PWRSTKSIZE];
QUAD MaxPower, NumPower;


/* comefrom: TestForCond DoPowerScan TryGo */
MoveMapSim (short MDir)
{
  switch (MDir) {
  case 0:
    if (SMapY > 0) {
      SMapY--;
      return (TRUE);
    }
    if (SMapY < 0)
      SMapY = 0;
    return (FALSE);
  case 1:
    if (SMapX < (WORLD_X - 1)) {
      SMapX++;
      return (TRUE);
    }
    if (SMapX > (WORLD_X - 1))
      SMapX = WORLD_X - 1;
    return (FALSE);
  case 2:
    if (SMapY < (WORLD_Y - 1)) {
      SMapY++;
      return (TRUE);
    }
    if (SMapY > (WORLD_Y - 1))
      SMapY = WORLD_Y - 1;
    return (FALSE);
  case 3:
    if (SMapX > 0) {
      SMapX--;
      return (TRUE);
    }
    if (SMapX < 0)
      SMapX = 0;
    return (FALSE);
  case 4:
    return (TRUE);
  }
  return (FALSE);
}


/* inlined -Don */
#if 0
/* comefrom: DoPowerScan */
SetPowerBit(void)
{
  register int PowerWrd;

  /* XXX: assumes 120x100 */
  PowerWrd = (SMapX >>4) + (SMapY <<3);
  PowerMap[PowerWrd] |= 1 << (SMapX & 15);
}
#endif


/* inlined */
#if 0
/* comefrom: TestForCond */
int TestPowerBit(void)
{
  register int PowerWrd;

  if ((CChr9 == NUCLEAR) || (CChr9 == POWERPLANT))
    return(1);
  /* XXX: assumes 120x100 */
  PowerWrd = (SMapX >>4) + (SMapY <<3);
  if (PowerWrd >= PWRMAPSIZE)
    return(0);
  return ((PowerMap[PowerWrd] & (1 << (SMapX & 15))) ? 1 : 0);
}
#endif


/* comefrom: DoPowerScan */
short TestForCond(short TFDir)
{
  register int xsave, ysave, PowerWrd;

  xsave = SMapX;
  ysave = SMapY;
  if (MoveMapSim(TFDir)) {
    /* TestPowerBit was taking a lot of time so I swapped the & order -Don */
#if 0
    if ((Map[SMapX][SMapY] & CONDBIT) &&
	(!TestPowerBit()))
#else
    if ((Map[SMapX][SMapY] & CONDBIT) &&
	(CChr9 != NUCLEAR) &&
	(CChr9 != POWERPLANT) &&
	(
#if 0
	 (PowerWrd = (SMapX >>4) + (SMapY <<3)),
#else
	 (PowerWrd = POWERWORD(SMapX, SMapY)),
#endif
	 ((PowerWrd > PWRMAPSIZE) ||
	  ((PowerMap[PowerWrd] & (1 << (SMapX & 15))) == 0))))
#endif
    {
      SMapX = xsave;
      SMapY = ysave;
      return (TRUE);
    }
  }
  SMapX = xsave;
  SMapY = ysave;
  return (FALSE);
}


/* comefrom: Simulate SpecialInit InitSimMemory */
DoPowerScan()
{
  short ADir;
  register int ConNum, Dir, x;

  for (x = 0; x < PWRMAPSIZE; x++)
    PowerMap[x] = 0;	/* ClearPowerMem */

  MaxPower = (CoalPop * 700L) + (NuclearPop * 2000L); /* post release */
  NumPower = 0;

  while (PowerStackNum)	{
    PullPowerStack();
    ADir = 4;
    do {
      if (++NumPower > MaxPower) {
	SendMes(40);
	return;
      }
      MoveMapSim(ADir);
/* inlined -Don */
#if 0
      SetPowerBit();
#else
#if 1
      SETPOWERBIT(SMapX, SMapY);
#else
      PowerMap[(SMapX >>4) + (SMapY <<3)] |= 1 << (SMapX & 15);
#endif
#endif
      ConNum = 0;
      Dir = 0;
      while ((Dir < 4) && (ConNum < 2))	{
	if (TestForCond(Dir)) {
	  ConNum++;
	  ADir = Dir;
	}
	Dir++;
      }
      if (ConNum > 1)
	PushPowerStack();
    } while (ConNum);
  }
}


/* comefrom: DoPowerScan DoSPZone */
PushPowerStack(void)
{
  if (PowerStackNum < (PWRSTKSIZE - 2)) {
    PowerStackNum++;
    PowerStackX[PowerStackNum] = SMapX;
    PowerStackY[PowerStackNum] = SMapY;
   }
}


/* comefrom: DoPowerScan */
PullPowerStack(void)
{
  if (PowerStackNum > 0)  {
    SMapX = PowerStackX[PowerStackNum];
    SMapY = PowerStackY[PowerStackNum];
    PowerStackNum--;
  }
}


