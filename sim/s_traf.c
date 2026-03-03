#include "sim.h"


/* Traffic Generation */


#define MAXDIS 30

short PosStackN, SMapXStack[MAXDIS+1], SMapYStack[MAXDIS+1];
short LDir;
short Zsource;
short TrafMaxX, TrafMaxY;


/* comefrom: DoIndustrial DoCommercial DoResidential */
MakeTraf(int Zt)
{
  short xtem, ytem;

  xtem = SMapX;
  ytem = SMapY;
  Zsource = Zt;
  PosStackN = 0;

#if 0
  if ((!Rand(2)) && FindPTele()) {
/* printf("Telecommute!\n"); */
    return (TRUE);
  }
#endif

  if (FindPRoad()) {		/* look for road on zone perimeter */
    if (TryDrive()) {		/* attempt to drive somewhere */
      SetTrafMem();		/* if sucessful, inc trafdensity */
      SMapX = xtem;
      SMapY = ytem;
      return (TRUE);		/* traffic passed */
    }
    SMapX = xtem;
    SMapY = ytem;
    return (FALSE);		/* traffic failed */
  }
  else return (-1);		/* no road found */
}


/* comefrom: MakeTraf */
SetTrafMem(void)
{
  register short x, z;

  for (x = PosStackN; x > 0; x--) {
    PullPos();
    if (TestBounds(SMapX, SMapY)) {
      z = Map[SMapX][SMapY] & LOMASK;
      if ((z >= ROADBASE) && (z < POWERBASE)) {
	SimSprite *sprite;

	/* check for rail */
	z = TrfDensity[SMapX >>1][SMapY >>1];
	z += 50;
	if ((z > 240) &&
	    (!Rand(5))) {
	  z = 240;
	  TrafMaxX = SMapX <<4;
	  TrafMaxY = SMapY <<4;
	  if (((sprite = GetSprite(COP)) != NULL) &&
	      (sprite->control == -1)) {
	    sprite->dest_x = TrafMaxX;
	    sprite->dest_y = TrafMaxY;
	  }
	}
	TrfDensity[SMapX >>1][SMapY >>1] = z;
      }
    }
  }
}


/* comefrom: TryGo */
PushPos(void)
{
  PosStackN++;
  SMapXStack[PosStackN] = SMapX;
  SMapYStack[PosStackN] = SMapY;
}


/* comefrom: SetTrafMem */
PullPos(void)
{
  SMapX = SMapXStack[PosStackN];
  SMapY = SMapYStack[PosStackN];
  PosStackN--;
}


/* comefrom: DoSPZone MakeTraf */
FindPRoad(void)		/* look for road on edges of zone   */
{
  static short PerimX[12] = {-1, 0, 1, 2, 2, 2, 1, 0,-1,-2,-2,-2};
  static short PerimY[12] = {-2,-2,-2,-1, 0, 1, 2, 2, 2, 1, 0,-1};
  register short tx, ty, z;

  for (z = 0; z < 12; z++) {
	  tx = SMapX + PerimX[z];
	  ty = SMapY + PerimY[z];
	  if (TestBounds(tx, ty)) {
		  if (RoadTest(Map[tx][ty])) {
			  SMapX = tx;
			  SMapY = ty;
			  return (TRUE);
		  }
	  }
  }
  return (FALSE);
}


FindPTele(void)		/* look for telecommunication on edges of zone */
{
  static short PerimX[12] = {-1, 0, 1, 2, 2, 2, 1, 0,-1,-2,-2,-2};
  static short PerimY[12] = {-2,-2,-2,-1, 0, 1, 2, 2, 2, 1, 0,-1};
  register short tx, ty, z, tile;

  for (z = 0; z < 12; z++) {
	  tx = SMapX + PerimX[z];
	  ty = SMapY + PerimY[z];
	  if (TestBounds(tx, ty)) {
	  	  tile = Map[tx][ty] & LOMASK;
		  if ((tile >= TELEBASE) && (tile <= TELELAST)) {
			  return (TRUE);
		  }
	  }
  }
  return (FALSE);
}


/* comefrom: MakeTraf */
TryDrive(void)
{
  short z;

  LDir = 5;
  for (z = 0; z < MAXDIS; z++) {	/* Maximum distance to try */
    if (TryGo(z)) {			/* if it got a road */
      if (DriveDone())			/* if destination is reached */
	return (TRUE);			/* pass */
    } else {
      if (PosStackN) {			/* deadend , backup */
	PosStackN--;
	z += 3;
      }
      else return (FALSE);		/* give up at start  */
    }
  }
  return (FALSE);			/* gone maxdis */
}


/* comefrom: TryDrive */
TryGo(int z)
{
  short x, rdir, realdir;

#if 0
  rdir = Rand(3); /* XXX: Heaviest user of Rand */
#else
  rdir = Rand16() & 3;
#endif
  for (x = rdir; x < (rdir + 4); x++) {	/* for the 4 directions */
    realdir = x & 3;
    if (realdir == LDir) continue;	/* skip last direction */
    if (RoadTest(GetFromMap(realdir))) {
      MoveMapSim(realdir);
      LDir = (realdir + 2) & 3;
      if (z & 1)			/* save pos every other move */
	PushPos();
      return (TRUE);
    }
  }
  return (FALSE);
}


/* comefrom: TryGo DriveDone */
GetFromMap(int x)
{
  switch (x) {
  case 0:
    if (SMapY > 0)
      return (Map[SMapX][SMapY - 1] & LOMASK);
    return (FALSE);
  case 1:
    if (SMapX < (WORLD_X - 1))
      return (Map[SMapX + 1][SMapY] & LOMASK);
    return (FALSE);
  case 2:
    if (SMapY < (WORLD_Y - 1))
      return (Map[SMapX][SMapY + 1] & LOMASK);
    return (FALSE);
  case 3:
    if (SMapX > 0)
      return (Map[SMapX - 1][SMapY] & LOMASK);
    return (FALSE);
  default:
    return (FALSE);
  }
}


/* comefrom: TryDrive */
DriveDone(void)
{
  static short TARGL[3] = {COMBASE, LHTHR, LHTHR};
  static short TARGH[3] = {NUCLEAR, PORT, COMBASE};	/* for destinations */
  register short x, z, l, h;

/* unwound -Don */
#if 0
  for (x = 0; x < 4; x++) {	/* R>C C>I I>R  */
    z = GetFromMap(x);
    if ((z >= TARGL[Zsource]) && (z <= TARGH[Zsource]))
      return (TRUE);
  }
#else
  l = TARGL[Zsource];
  h = TARGH[Zsource];

  if (SMapY > 0) {
    z = Map[SMapX][SMapY - 1] & LOMASK;
    if ((z >= l) && (z <= h))
      return (TRUE);
  }
  if (SMapX < (WORLD_X - 1)) {
    z = Map[SMapX + 1][SMapY] & LOMASK;
    if ((z >= l) && (z <= h))
      return (TRUE);
  }
  if (SMapY < (WORLD_Y - 1)) {
    z = Map[SMapX][SMapY + 1] & LOMASK;
    if ((z >= l) && (z <= h))
      return (TRUE);
  }
  if (SMapX > 0) {
    z = Map[SMapX - 1][SMapY] & LOMASK;
    if ((z >= l) && (z <= h))
      return (TRUE);
  }
#endif

  return (FALSE);
}


/* comefrom: TryGo FindPRoad */
RoadTest(int x)
{
  x = x & LOMASK;
  if (x < ROADBASE)
    return (FALSE);
  if (x > LASTRAIL)
    return (FALSE);
  if ((x >= POWERBASE) && (x < RAILHPOWERV))
    return (FALSE);
  return (TRUE);
}

