#include "sim.h"


/* Disasters */


short ShakeNow;
short FloodCnt;
short FloodX, FloodY;


/* comefrom: Simulate */
DoDisasters(void)
{
  /* Chance of disasters at lev 0 1 2 */
  static short DisChance[3] = { 10*48, 5*48, 60};
  register short x;

  if (FloodCnt) FloodCnt--;
  if (DisasterEvent)
    ScenarioDisaster();

  x = GameLevel;
  if (x > 2) x = 0;

  if (NoDisasters) return;		/*post*/
  if (!Rand(DisChance[x])) {
    x = Rand(8);
    switch (x) {
    case 0:
    case 1:
      SetFire();
      break;
    case 2:
    case 3:
      MakeFlood();
      break;
    case 4:
      break;
    case 5:
      MakeTornado();
      break;
    case 6:
      MakeEarthquake();
      break;
    case 7:
    case 8:
      if (PolluteAverage > /* 80 */ 60) MakeMonster();
      break;
    }
  }
}


/* comefrom: DoDisasters */
ScenarioDisaster(void)
{
  int x, y;

  switch (DisasterEvent) {
    case 1:			/* Dullsville */
      break;
    case 2: 			/* San Francisco */
      if (DisasterWait == 1) MakeEarthquake();
      break;
    case 3:			/* Hamburg */
      DropFireBombs();
      break;
    case 4:			/* Bern */
      break;
    case 5: 			/* Tokyo */
      if (DisasterWait == 1) MakeMonster();
      break;
    case 6: 			/* Detroit */
      break;
    case 7: 			/* Boston */
      if (DisasterWait == 1) MakeMeltdown();
      break;
    case 8:			/* Rio */
      if ((DisasterWait % 24) == 0) MakeFlood();
      break;
  }
  if (DisasterWait) DisasterWait--;
  else DisasterEvent = 0;
}


/* comefrom: ScenarioDisaster */
MakeMeltdown(void)
{
  short x, y;

  for (x = 0; x < (WORLD_X - 1); x ++) {
    for (y = 0; y < (WORLD_Y - 1); y++) {
      /* TILE_IS_NUCLEAR(Map[x][y]) */
      if ((Map[x][y] & LOMASK) == NUCLEAR) {
	DoMeltdown(x, y);
	return;
      }
    }
  }
}


FireBomb()
{
  CrashX = Rand(WORLD_X - 1);
  CrashY = Rand(WORLD_Y - 1);
  MakeExplosion(CrashX, CrashY);
  ClearMes();
  SendMesAt(-30, CrashX, CrashY);
}


/* comefrom: DoDisasters ScenarioDisaster */
MakeEarthquake(void)
{
  register short x, y, z;
  short time;

  DoEarthQuake();

  SendMesAt(-23, CCx, CCy);
  time = Rand(700) + 300;
  for (z = 0; z < time; z++)  {
    x = Rand(WORLD_X - 1);
    y = Rand(WORLD_Y - 1);
    if ((x < 0) || (x > (WORLD_X - 1)) ||
	(y < 0) || (y > (WORLD_Y - 1)))
      continue;
    /* TILE_IS_VULNERABLE(Map[x][y]) */
    if (Vunerable(Map[x][y])) {
      if (z & 0x3)
	Map[x][y] = (RUBBLE + BULLBIT) + (Rand16() & 3);
      else
	Map[x][y] = (FIRE + ANIMBIT) + (Rand16() & 7);
    }
  }
}


/* comefrom: DoDisasters */
SetFire(void)
{
  register short x, y, z;

  x = Rand(WORLD_X - 1);
  y = Rand(WORLD_Y - 1);
  z = Map[x][y];
  /* TILE_IS_ARSONABLE(z) */
  if (!(z & ZONEBIT)) {
    z = z & LOMASK;
    if ((z > LHTHR) && (z < LASTZONE)) {
      Map[x][y] = FIRE + ANIMBIT + (Rand16() & 7);
      CrashX = x; CrashY = y;
      SendMesAt(-20, x, y);
    }
  }
}


/* comefrom: DoDisasters */
MakeFire(void)
{
  short t, x, y, z;
  for (t = 0; t < 40; t++)  {
    x = Rand(WORLD_X - 1);
    y = Rand(WORLD_Y - 1);
    z = Map[x][y];
    /* !(z & BURNBIT) && TILE_IS_ARSONABLE(z) */
    if ((!(z & ZONEBIT)) && (z & BURNBIT)) {
      z = z & LOMASK;
      if ((z > 21) && (z < LASTZONE)) {
	Map[x][y] = FIRE + ANIMBIT + (Rand16() & 7);
	SendMesAt(20, x, y);
	return;
      }
    }
  }
}


/* comefrom: MakeEarthquake */
Vunerable(int tem)
{
  register int tem2;

  tem2 = tem & LOMASK;
  if ((tem2 < RESBASE) ||
      (tem2 > LASTZONE) ||
      (tem & ZONEBIT))
    return(FALSE);
  return(TRUE);
}


/* comefrom: DoDisasters ScenarioDisaster */
MakeFlood(void)
{
  static short Dx[4] = { 0, 1, 0,-1};
  static short Dy[4] = {-1, 0, 1, 0};
  register short xx, yy, c;
  short z, t, x, y;

  for (z = 0; z < 300; z++) {
    x = Rand(WORLD_X - 1);
    y = Rand(WORLD_Y - 1);
    c = Map[x][y] & LOMASK; /* XXX: & LOMASK */
    /* TILE_IS_RIVER_EDGE(c) */
    if ((c > 4) && (c < 21))		/* if riveredge  */
      for (t = 0; t < 4; t++) {
	xx = x + Dx[t];
	yy = y + Dy[t];
	if (TestBounds(xx, yy)) {
	  c = Map[xx][yy];
	  /* TILE_IS_FLOODABLE(c) */
	  if ((c == 0) || ((c & BULLBIT) && (c & BURNBIT))) {
	    Map[xx][yy] = FLOOD;
	    FloodCnt = 30;
	    SendMesAt(-42, xx, yy);
	    FloodX = xx; FloodY = yy;
	    return;
	  }
	}
      }
  }
}


/* comefrom: MapScan */
DoFlood(void)
{
  static short Dx[4] = { 0, 1, 0,-1};
  static short Dy[4] = {-1, 0, 1, 0};
  register short z, c, xx, yy, t;

  if (FloodCnt)
    for (z = 0; z < 4; z++) {
      if (!(Rand16() & 7)) {
	xx = SMapX + Dx[z];
	yy = SMapY + Dy[z];
	if (TestBounds(xx, yy)) {
	  c = Map[xx][yy];
	  t = c & LOMASK;
	  /* TILE_IS_FLOODABLE2(c) */
	  if ((c & BURNBIT) ||
	      (c == 0) ||
	      ((t >= WOODS5 /* XXX */) && (t < FLOOD))) {
	    if (c & ZONEBIT)
	      FireZone(xx, yy, c);
	    Map[xx][yy] = FLOOD + Rand(2);
	  }
	}
      }
    }
  else
    if (!(Rand16() & 15))
      Map[SMapX][SMapY] = 0;
}
