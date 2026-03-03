#include "sim.h"


/* comefrom: DoZone */
DoSPZone(short PwrOn)
{
  static short MltdwnTab[3] = { 30000, 20000, 10000 };  /* simadj */
  register z;

  switch (CChr9) {

  case POWERPLANT:
    CoalPop++;
    if (!(CityTime & 7))
      RepairZone(POWERPLANT, 4); /* post */
    PushPowerStack();
    CoalSmoke(SMapX, SMapY);
    return;

  case NUCLEAR:
    if (!NoDisasters && !Rand(MltdwnTab[GameLevel])) {
      DoMeltdown(SMapX, SMapY);
      return;
    }
    NuclearPop++;
    if (!(CityTime & 7))
      RepairZone(NUCLEAR, 4); /* post */
    PushPowerStack();
    return;

  case FIRESTATION:
    FireStPop++;
    if (!(CityTime & 7))
      RepairZone(FIRESTATION, 3); /* post */

    if (PwrOn)
      z = FireEffect;			/* if powered get effect  */
    else
      z = FireEffect >>1;		/* from the funding ratio  */

    if (!FindPRoad())
      z = z >>1;			/* post FD's need roads  */

    FireStMap[SMapX >>3][SMapY >>3] += z;
    return;

  case POLICESTATION:
    PolicePop++;
    if (!(CityTime & 7))
      RepairZone(POLICESTATION, 3); /* post */

    if (PwrOn)
      z = PoliceEffect;
    else
      z = PoliceEffect >>1;

    if (!FindPRoad())
      z = z >>1; /* post PD's need roads */

    PoliceMap[SMapX >>3][SMapY >>3] += z;
    return;

  case STADIUM:
    StadiumPop++;
    if (!(CityTime & 15))
      RepairZone(STADIUM, 4);
    if (PwrOn)
      if (!((CityTime + SMapX + SMapY) & 31)) {	/* post release */
	DrawStadium(FULLSTADIUM);
	Map[SMapX + 1][SMapY] = FOOTBALLGAME1 + ANIMBIT;
	Map[SMapX + 1][SMapY + 1] = FOOTBALLGAME2 + ANIMBIT;
      }
    return;

 case FULLSTADIUM:
    StadiumPop++;
    if (!((CityTime + SMapX + SMapY) & 7))	/* post release */
      DrawStadium(STADIUM);
    return;

 case AIRPORT:
    APortPop++;
    if (!(CityTime & 7))
      RepairZone(AIRPORT, 6);

    if (PwrOn) { /* post */
      if ((Map[SMapX + 1][SMapY - 1] & LOMASK) == RADAR)
	Map[SMapX + 1][SMapY - 1] = RADAR + ANIMBIT + CONDBIT + BURNBIT;
    } else
      Map[SMapX + 1][SMapY - 1] = RADAR + CONDBIT + BURNBIT;

    if (PwrOn)
      DoAirport();
    return;

 case PORT:
    PortPop++;
    if ((CityTime & 15) == 0) {
      RepairZone(PORT, 4);
    }
    if (PwrOn &&
	(GetSprite(SHI) == NULL)) {
      GenerateShip();
    }
    return;
  }
}


/* comefrom: DoSPZone */
DrawStadium(int z)
{
  register int x, y;

  z = z - 5;
  for (y = (SMapY - 1); y < (SMapY + 3); y++)
    for (x = (SMapX - 1); x < (SMapX + 3); x++)
      Map[x][y] = (z++) | BNCNBIT;
  Map[SMapX][SMapY] |= ZONEBIT | PWRBIT;
}


/* comefrom: DoSPZone */
DoAirport(void)
{
  if (!(Rand(5))) {
    GeneratePlane(SMapX, SMapY);
    return;
  }
  if (!(Rand(12)))
    GenerateCopter(SMapX, SMapY);
}


/* comefrom: DoSPZone */
CoalSmoke(int mx, int my)
{
  static short SmTb[4] = { COALSMOKE1, COALSMOKE2, COALSMOKE3, COALSMOKE4 };
  static short dx[4] = {  1,  2,  1,  2 };
  static short dy[4] = { -1, -1,  0,  0 };
  register short x;

  for (x = 0; x < 4; x++)
    Map[mx + dx[x]][my + dy[x]] =
      SmTb[x] | ANIMBIT | CONDBIT | PWRBIT | BURNBIT;
}
