#include "sim.h"


short ResHisMax, Res2HisMax;
short ComHisMax, Com2HisMax;
short IndHisMax, Ind2HisMax;
short Graph10Max, Graph120Max;
short TaxFlag;


/* comefrom: Simulate */
DecTrafficMem(void)		/* tends to empty TrfDensity   */
{
  register short x, y, z;

  for (x = 0; x < HWLDX; x++)
    for (y = 0; y < HWLDY; y++)
      if (z = TrfDensity[x][y]) {
	if (z > 24) {
	  if (z > 200) TrfDensity[x][y] = z - 34;
	  else TrfDensity[x][y] = z - 24;
	}
	else TrfDensity[x][y] = 0;
      }
}


/* comefrom: Simulate */
DecROGMem(void)			/* tends to empty RateOGMem   */
{
  register short x, y, z;

  for (x = 0; x < SmX; x++)
    for (y = 0; y < SmY; y++)	{
      z = RateOGMem[x][y];
      if (z == 0) continue;
      if (z > 0)  {
	--RateOGMem[x][y];
	if (z > 200) RateOGMem[x][y] = 200;    /* prevent overflow */
	continue;
      }
      if (z < 0)  {
	++RateOGMem[x][y];
	if (z < -200) RateOGMem[x][y] = -200;
      }
    }
}


/* comefrom: MapScan */
DoRail(void)
{
  RailTotal++;
  GenerateTrain(SMapX, SMapY);
  if (RoadEffect < 30) /* Deteriorating  Rail  */
    if (!(Rand16() & 511))
      if (!(CChr & CONDBIT))
	if (RoadEffect < (Rand16() & 31)) {
	  if (CChr9 < (RAILBASE + 2))
	    Map[SMapX][SMapY] = RIVER;
	  else
	    Map[SMapX][SMapY] = RUBBLE + (Rand16() & 3) + BULLBIT;
	  return;
	}
}


/* comefrom: MapScan */
DoRadTile(void)
{
  if (!(Rand16() & 4095)) Map[SMapX][SMapY] = 0; /* Radioactive decay */
}


/* comefrom: MapScan */
DoRoad(void)
{
  register short Density, tden, z;
  static short DenTab[3] = { ROADBASE, LTRFBASE, HTRFBASE };

  RoadTotal++;
/*  GenerateBus(SMapX, SMapY); */
  if (RoadEffect < 30) /* Deteriorating Roads */
    if (!(Rand16() & 511))
      if (!(CChr & CONDBIT))
	if (RoadEffect < (Rand16() & 31)) {
	  if (((CChr9 & 15) < 2) || ((CChr9 & 15) == 15))
	    Map[SMapX][SMapY] = RIVER;
	  else
	    Map[SMapX][SMapY] = RUBBLE + (Rand16() & 3) + BULLBIT;
	  return;
	}

  if (!(CChr & BURNBIT)) { /* If Bridge */
    RoadTotal += 4;
    if (DoBridge())  return;
  }
  if (CChr9 < LTRFBASE) tden = 0;
  else if (CChr9 < HTRFBASE) tden = 1;
  else {
    RoadTotal++;
    tden = 2;
  }

  Density = (TrfDensity[SMapX >>1][SMapY >>1]) >>6;  /* Set Traf Density  */
  if (Density > 1) Density--;
  if (tden != Density) { /* tden 0..2   */
    z = ((CChr9 - ROADBASE) & 15) + DenTab[Density];
    z += CChr & (ALLBITS - ANIMBIT);
    if (Density) z += ANIMBIT;
    Map[SMapX][SMapY] = z;
  }
}


/* comefrom: MapScan */
DoBridge(void)
{
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
  register z, x, y, MPtem;

  if (CChr9 == BRWV) { /*  Vertical bridge close */
    if ((!(Rand16() & 3)) &&
	(GetBoatDis() > 340))
      for (z = 0; z < 7; z++) { /* Close  */
	x = SMapX + VDx[z];
	y = SMapY + VDy[z];
	if (TestBounds(x, y))
	  if ((Map[x][y] & LOMASK) == (VBRTAB[z] & LOMASK))
	    Map[x][y] = VBRTAB2[z];
      }
    return (TRUE);
  }
  if (CChr9 == BRWH) { /*  Horizontal bridge close  */
    if ((!(Rand16() & 3)) &&
	(GetBoatDis() > 340))
      for (z = 0; z < 7; z++) { /* Close  */
	x = SMapX + HDx[z];
	y = SMapY + HDy[z];
	if (TestBounds(x, y))
	  if ((Map[x][y] & LOMASK) == (HBRTAB[z] & LOMASK))
	    Map[x][y] = HBRTAB2[z];
      }
    return (TRUE);
  }

  if ((GetBoatDis() < 300) || (!(Rand16() & 7))) {
    if (CChr9 & 1) {
      if (SMapX < (WORLD_X - 1))
	if (Map[SMapX + 1][SMapY] == CHANNEL) { /* Vertical open */
	  for (z = 0; z < 7; z++) {
	    x = SMapX + VDx[z];
	    y = SMapY + VDy[z];
	    if (TestBounds(x, y))  {
	      MPtem = Map[x][y];
	      if ((MPtem == CHANNEL) ||
		  ((MPtem & 15) == (VBRTAB2[z] & 15)))
		Map[x][y] = VBRTAB[z];
	    }
	  }
	  return (TRUE);
	}
      return (FALSE);
    } else {
      if (SMapY > 0)
	if (Map[SMapX][SMapY - 1] == CHANNEL) { /* Horizontal open  */
	  for (z = 0; z < 7; z++) {
	    x = SMapX + HDx[z];
	    y = SMapY + HDy[z];
	    if (TestBounds(x, y)) {
	      MPtem = Map[x][y];
	      if (((MPtem & 15) == (HBRTAB2[z] & 15)) ||
		  (MPtem == CHANNEL))
		Map[x][y] = HBRTAB[z];
	    }
	  }
	  return (TRUE);
	}
      return (FALSE);
    }
  }
  return (FALSE);
}


/* comefrom: MapScan */
DoFire(void)
{
  static short DX[4] = { -1,  0,  1,  0 };
  static short DY[4] = {  0, -1,  0,  1 };
  register short z, Xtem, Ytem, Rate, c;

  for (z = 0; z < 4; z++) {
    if (!(Rand16() & 7)) {
      Xtem = SMapX + DX[z];
      Ytem = SMapY + DY[z];
      if (TestBounds(Xtem, Ytem)) {
	c = Map[Xtem][Ytem];
	if (c & BURNBIT) {
	  if (c & ZONEBIT) {
	    FireZone(Xtem, Ytem, c);
	    if ((c & LOMASK) > IZB)  { /*  Explode  */
	      MakeExplosionAt((Xtem <<4) + 8, (Ytem <<4) + 8);
	    }
	  }
	  Map[Xtem][Ytem] = FIRE + (Rand16() & 3) + ANIMBIT;
	}
      }
    }
  }
  z = FireRate[SMapX >>3][SMapY >>3];
  Rate = 10;
  if (z) {
    Rate = 3;
    if (z > 20) Rate = 2;
    if (z > 100) Rate = 1;
  }
  if (!Rand(Rate))
    Map[SMapX][SMapY] = RUBBLE + (Rand16() & 3) + BULLBIT;
}


/* comefrom: DoFire MakeFlood */
FireZone(int Xloc, int Yloc, int ch)
{
  register short Xtem, Ytem;
  short x, y, XYmax;

  RateOGMem[Xloc >>3][Yloc >>3] -= 20;

  ch = ch & LOMASK;
  if (ch < PORTBASE)
    XYmax = 2;
  else
    if (ch == AIRPORT)
      XYmax = 5;
    else
      XYmax = 4;

  for (x = -1; x < XYmax; x++)
    for (y = -1; y < XYmax; y++) {
      Xtem = Xloc + x;
      Ytem = Yloc + y;
      if ((Xtem < 0) || (Xtem > (WORLD_X - 1)) ||
	  (Ytem < 0) || (Ytem > (WORLD_Y - 1)))
	continue;
      if ((short)(Map[Xtem][Ytem] & LOMASK) >= ROADBASE) /* post release */
	Map[Xtem][Ytem] |= BULLBIT;
    }
}


/* comefrom: DoSPZone DoHospChur */
RepairZone(short ZCent, short zsize)
{
  short cnt;
  register short x, y, ThCh;

  zsize--;
  cnt = 0;
  for (y = -1; y < zsize; y++)
    for (x = -1; x < zsize; x++) {
      int xx = SMapX + x;
      int yy = SMapY + y;
      cnt++;
      if (TestBounds(xx, yy)) {
	ThCh = Map[xx][yy];
	if (ThCh & ZONEBIT) continue;
	if (ThCh & ANIMBIT) continue;
	ThCh = ThCh & LOMASK;
	if ((ThCh < RUBBLE) || (ThCh >= ROADBASE)) {
	  Map[xx][yy] = ZCent - 3 - zsize + cnt + CONDBIT + BURNBIT;
	}
      }
    }
}


/* comefrom: Simulate DoSimInit */
SetValves(void)
{
  static short TaxTable[21] = {
    200, 150, 120, 100, 80, 50, 30, 0, -10, -40, -100,
    -150, -200, -250, -300, -350, -400, -450, -500, -550, -600 };
  float Employment, Migration, Births, LaborBase, IntMarket;
  float Rratio, Cratio, Iratio, temp;
  float NormResPop, PjResPop, PjComPop, PjIndPop;
  register short z;

  MiscHis[1] = (short)EMarket;
  MiscHis[2] = ResPop;
  MiscHis[3] = ComPop;
  MiscHis[4] = IndPop;
  MiscHis[5] = RValve;
  MiscHis[6] = CValve;
  MiscHis[7] = IValve;
  MiscHis[10] = CrimeRamp;
  MiscHis[11] = PolluteRamp;
  MiscHis[12] = LVAverage;
  MiscHis[13] = CrimeAverage;
  MiscHis[14] = PolluteAverage;
  MiscHis[15] = GameLevel;
  MiscHis[16] = CityClass;
  MiscHis[17] = CityScore;

  NormResPop = ResPop / 8;
  LastTotalPop = TotalPop;
  TotalPop = NormResPop + ComPop + IndPop;

  if (NormResPop) Employment = ((ComHis[1] + IndHis[1]) / NormResPop);
  else Employment = 1;

  Migration = NormResPop * (Employment - 1);
  Births = NormResPop * (.02); 			/* Birth Rate  */
  PjResPop = NormResPop + Migration + Births;	/* Projected Res.Pop  */

  if (temp = (ComHis[1] + IndHis[1])) LaborBase = (ResHis[1] / temp);
  else LaborBase = 1;
  if (LaborBase > 1.3) LaborBase = 1.3;
  if (LaborBase < 0) LaborBase = 0;  /* LB > 1 - .1  */

  for (z = 0; z < 2; z++)
    temp = ResHis[z] + ComHis[z] + IndHis[z];
  IntMarket = (NormResPop + ComPop + IndPop) / 3.7;

  PjComPop = IntMarket * LaborBase;

  z = GameLevel;			/* New ExtMarket */
  temp = 1;
  switch (z)  {
  case 0:
    temp = 1.2;
    break;
  case 1:
    temp = 1.1;
    break;
  case 2:
    temp = .98;
    break;
  }

  PjIndPop = IndPop * LaborBase * temp;
  if (PjIndPop < 5) PjIndPop = 5;

  if (NormResPop) Rratio = (PjResPop / NormResPop); /* projected -vs- actual */
  else Rratio = 1.3;
  if (ComPop) Cratio = (PjComPop / ComPop);
  else Cratio = PjComPop;
  if (IndPop) Iratio = (PjIndPop / IndPop);
  else Iratio = PjIndPop;

  if (Rratio > 2) Rratio = 2;
  if (Cratio > 2) Cratio = 2;
  if (Iratio > 2) Iratio = 2;

  z = CityTax + GameLevel;
  if (z > 20) z = 20;
  Rratio = ((Rratio -1) * 600) + TaxTable[z]; /* global tax/Glevel effects */
  Cratio = ((Cratio -1) * 600) + TaxTable[z];
  Iratio = ((Iratio -1) * 600) + TaxTable[z];

  if (Rratio > 0)		/* ratios are velocity changes to valves  */
    if (RValve <  2000) RValve += Rratio;
  if (Rratio < 0)
    if (RValve > -2000) RValve += Rratio;
  if (Cratio > 0)
    if (CValve <  1500) CValve += Cratio;
  if (Cratio < 0)
    if (CValve > -1500) CValve += Cratio;
  if (Iratio > 0)
    if (IValve <  1500) IValve += Iratio;
  if (Iratio < 0)
    if (IValve > -1500) IValve += Iratio;

  if (RValve >  2000) RValve =  2000;
  if (RValve < -2000) RValve = -2000;
  if (CValve >  1500) CValve =  1500;
  if (CValve < -1500) CValve = -1500;
  if (IValve >  1500) IValve =  1500;
  if (IValve < -1500) IValve = -1500;

  if ((ResCap) && (RValve > 0)) RValve = 0;	/* Stad, Prt, Airprt  */
  if ((ComCap) && (CValve > 0)) CValve = 0;
  if ((IndCap) && (IValve > 0)) IValve = 0;
  ValveFlag = 1;
}


/* comefrom: Simulate DoSimInit */
ClearCensus(void)
{
  register short x, y, z;

  z = 0;
  PwrdZCnt = z;
  unPwrdZCnt = z;
  FirePop = z;
  RoadTotal = z;
  RailTotal = z;
  ResPop = z;
  ComPop = z;
  IndPop = z;
  ResZPop = z;
  ComZPop = z;
  IndZPop = z;
  HospPop = z;
  ChurchPop = z;
  PolicePop = z;
  FireStPop = z;
  StadiumPop = z;
  CoalPop = z;
  NuclearPop = z;
  PortPop = z;
  APortPop = z;
  PowerStackNum = z;		/* Reset before Mapscan */
  for (x = 0; x < SmX; x++)
    for (y = 0; y < SmY; y++) {
      FireStMap[x][y] = z;
      PoliceMap[x][y] = z;
    }
}


/* comefrom: Simulate */
TakeCensus(void)
{
  short x;

  /* put census#s in Historical Graphs and scroll data  */
  ResHisMax = 0;
  ComHisMax = 0;
  IndHisMax = 0;
  for (x = 118; x >= 0; x--)	{
    if ((ResHis[x + 1] = ResHis[x]) > ResHisMax) ResHisMax = ResHis[x];
    if ((ComHis[x + 1] = ComHis[x]) > ComHisMax) ComHisMax = ComHis[x];
    if ((IndHis[x + 1] = IndHis[x]) > IndHisMax) IndHisMax = IndHis[x];
    CrimeHis[x + 1] = CrimeHis[x];
    PollutionHis[x + 1] = PollutionHis[x];
    MoneyHis[x + 1] = MoneyHis[x];
  }

  Graph10Max = ResHisMax;
  if (ComHisMax > Graph10Max) Graph10Max = ComHisMax;
  if (IndHisMax > Graph10Max) Graph10Max = IndHisMax;

  ResHis[0] = ResPop / 8;
  ComHis[0] = ComPop;
  IndHis[0] = IndPop;

  CrimeRamp += (CrimeAverage - CrimeRamp) / 4;
  CrimeHis[0] = CrimeRamp;

  PolluteRamp += (PolluteAverage - PolluteRamp) / 4;
  PollutionHis[0] = PolluteRamp;

  x = (CashFlow / 20) + 128;	/* scale to 0..255  */
  if (x < 0) x = 0;
  if (x > 255) x = 255;

  MoneyHis[0] = x;
  if (CrimeHis[0] > 255) CrimeHis[0] = 255;
  if (PollutionHis[0] > 255) PollutionHis[0] = 255;

  ChangeCensus(); /* XXX: if 10 year graph view */

  if (HospPop < (ResPop >>8)) NeedHosp = TRUE;
  if (HospPop > (ResPop >>8)) NeedHosp = -1;
  if (HospPop == (ResPop >>8)) NeedHosp = FALSE;

  if (ChurchPop < (ResPop >>8)) NeedChurch = TRUE;
  if (ChurchPop > (ResPop >>8)) NeedChurch = -1;
  if (ChurchPop == (ResPop >>8)) NeedChurch = FALSE;
}


/* comefrom: Simulate */
Take2Census(void)    /* Long Term Graphs */
{
  short x;

  Res2HisMax = 0;
  Com2HisMax = 0;
  Ind2HisMax = 0;
  for (x = 238; x >= 120; x--)	{
    if ((ResHis[x + 1] = ResHis[x]) > Res2HisMax) Res2HisMax = ResHis[x];
    if ((ComHis[x + 1] = ComHis[x]) > Com2HisMax) Com2HisMax = ComHis[x];
    if ((IndHis[x + 1] = IndHis[x]) > Ind2HisMax) Ind2HisMax = IndHis[x];
    CrimeHis[x + 1] = CrimeHis[x];
    PollutionHis[x + 1] = PollutionHis[x];
    MoneyHis[x + 1] = MoneyHis[x];
  }
  Graph120Max = Res2HisMax;
  if (Com2HisMax > Graph120Max) Graph120Max = Com2HisMax;
  if (Ind2HisMax > Graph120Max) Graph120Max = Ind2HisMax;

  ResHis[120] = ResPop / 8;
  ComHis[120] = ComPop;
  IndHis[120] = IndPop;
  CrimeHis[120] = CrimeHis[0] ;
  PollutionHis[120] = PollutionHis[0];
  MoneyHis[120] = MoneyHis[0];
  ChangeCensus(); /* XXX: if 120 year graph view */
}


/* comefrom: Simulate */
CollectTax(void)
{
  static float RLevels[3] = { 0.7, 0.9, 1.2 };
  static float FLevels[3] = { 1.4, 1.2, 0.8 };
  short z;

  CashFlow = 0;
  if (!TaxFlag) { /* if the Tax Port is clear  */
    /* XXX: do something with z */
    z = AvCityTax / 48;  /* post */
    AvCityTax = 0;
    PoliceFund = PolicePop * 100;
    FireFund   = FireStPop * 100;
    RoadFund   = (RoadTotal + (RailTotal * 2)) * RLevels[GameLevel];
    TaxFund = (((QUAD)TotalPop * LVAverage) / 120) *
      	      CityTax * FLevels[GameLevel];
    if (TotalPop) {	/* if there are people to tax  */
      CashFlow = TaxFund - (PoliceFund + FireFund + RoadFund);

      DoBudget();
    } else {
      RoadEffect = 32;
      PoliceEffect = 1000;
      FireEffect = 1000;
    }
  }
}


UpdateFundEffects(void)
{
  if (RoadFund)
    RoadEffect = (short)(((float)RoadSpend /
			  (float)RoadFund) * 32.0);
  else
    RoadEffect = 32;

  if (PoliceFund)
    PoliceEffect = (short)(((float)PoliceSpend /
			    (float)PoliceFund) * 1000.0);
  else
    PoliceEffect = 1000;

  if (FireFund)
    FireEffect = (short)(((float)FireSpend /
			  (float)FireFund) * 1000.0);
  else
    FireEffect = 1000;

  drawCurrPercents();
}


/* comefrom: InitSimMemory SimLoadInit */
SetCommonInits(void)
{
  EvalInit();
  RoadEffect = 32;
  PoliceEffect = 1000;
  FireEffect = 1000;
  TaxFlag = 0;
  TaxFund = 0;
}


short InitSimLoad;


/* comefrom: SimLoadInit */
DoNilPower(void)
{
  register short x, y, z;

  for (x = 0; x < WORLD_X; x++)
    for (y = 0; y < WORLD_Y; y++) {
      z = Map[x][y];
      if (z & ZONEBIT) {
	SMapX = x;
	SMapY = y;
	CChr = z;
	SetZPower();
      }
    }
}


/* comefrom: DoSimInit */
InitSimMemory(void)
{
  register short x, z;

  z = 0;
  SetCommonInits();
  for (x = 0; x < 240; x++)  {
    ResHis[x] = z;
    ComHis[x] = z;
    IndHis[x] = z;
    MoneyHis[x] = 128;
    CrimeHis[x] = z;
    PollutionHis[x] = z;
  }
  CrimeRamp = z;
  PolluteRamp = z;
  TotalPop = z;
  RValve = z;
  CValve = z;
  IValve = z;
  ResCap = z;
  ComCap = z;
  IndCap = z;

  EMarket = 6.0;
  DisasterEvent = 0;
  ScoreType = 0;

  /* This clears powermem */
  PowerStackNum = z;
  DoPowerScan();
  NewPower = 1;		/* post rel */

  InitSimLoad = 0;
}


/* comefrom: DoSimInit */
SimLoadInit(void)
{
  static short DisTab[9] = { 0, 2, 10, 5, 20, 3, 5, 5, 2 * 48};
  static short ScoreWaitTab[9] = { 0, 30 * 48, 5 * 48, 5 * 48, 10 * 48,
				   5 * 48, 10 * 48, 5 * 48, 10 * 48 };
  register int z;

  z = 0;
  EMarket = (float)MiscHis[1];
  ResPop = MiscHis[2];
  ComPop = MiscHis[3];
  IndPop = MiscHis[4];
  RValve = MiscHis[5];
  CValve = MiscHis[6];
  IValve = MiscHis[7];
  CrimeRamp = MiscHis[10];
  PolluteRamp = MiscHis[11];
  LVAverage = MiscHis[12];
  CrimeAverage = MiscHis[13];
  PolluteAverage = MiscHis[14];
  GameLevel = MiscHis[15];

  if (CityTime < 0) CityTime = 0;
  if (!EMarket) EMarket = 4.0;
  if ((GameLevel > 2) || (GameLevel < 0)) GameLevel = 0;
  SetGameLevel(GameLevel);

  SetCommonInits();

  CityClass = MiscHis[16];
  CityScore = MiscHis[17];

  if ((CityClass > 5) || (CityClass < 0)) CityClass = 0;
  if ((CityScore > 999) || (CityScore < 1)) CityScore = 500;

  ResCap = 0;
  ComCap = 0;
  IndCap = 0;

  AvCityTax = (CityTime % 48) * 7;  /* post */

  for (z = 0; z < PWRMAPSIZE; z++)
    PowerMap[z] = ~0; /* set power Map */
  DoNilPower();

  if (ScenarioID > 8) ScenarioID = 0;

  if (ScenarioID) {
    DisasterEvent = ScenarioID;
    DisasterWait = DisTab[DisasterEvent];
    ScoreType = DisasterEvent;
    ScoreWait = ScoreWaitTab[DisasterEvent];
  } else {
    DisasterEvent = 0;
    ScoreType = 0;
  }

  RoadEffect = 32;
  PoliceEffect = 1000; /*post*/
  FireEffect = 1000;
  InitSimLoad = 0;
}
