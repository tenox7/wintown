#include "sim.h"
#include "notify.h"
#include <stdio.h>
#include <windows.h>

extern int OrigCityEvaluation();
extern int OrigEvalInit();
extern short ProblemOrder[];
extern short ProblemVotes[];
extern short EvalValid;
extern short AverageCityScore;
extern QUAD CityAssValue;

extern int addGameLog(const char *format, ...);
extern int addDebugLog(const char *format, ...);

int HospPop;
int ChurchPop;
int NeedHosp = 0;
int NeedChurch = 0;
extern int NuclearPop;
extern int CoalPop;

#define PROB_CRIME 0
#define PROB_POLLUTION 1
#define PROB_HOUSING 2
#define PROB_TAXES 3
#define PROB_TRAFFIC 4
#define PROB_UNEMPLOYMENT 5
#define PROB_FIRE 6
#define PROB_NONE 7

int WinEvalInit() {
    EvalInit();
    AverageCityScore = 500;
    return 0;
}

int WinCityEvaluation() {
    CityEvaluation();

    if (TotalPop) {
        addDebugLog("City evaluation: Score=%d, Approval=%d%%", CityScore, CityYes);

        if (CityPop > 2000 && CityClass >= 1)
            ShowNotification(NOTIF_VILLAGE_2K);
        if (CityPop > 100000 && CityClass >= 4)
            ShowNotification(NOTIF_CAPITAL_100K);

        if (CrimeAverage > 100)
            ShowNotification(NOTIF_HIGH_CRIME);
        if (PolluteAverage > 60)
            ShowNotification(NOTIF_HIGH_POLLUTION);
        if (TrafficAverage > 100)
            ShowNotification(NOTIF_TRAFFIC_JAMS);
        if (CityTax > 12)
            ShowNotification(NOTIF_TAX_TOO_HIGH);

        if (TotalPop > 60 && FireStPop == 0)
            ShowNotification(NOTIF_FIRE_DEPT_NEEDED);
        if (TotalPop > 60 && PolicePop == 0)
            ShowNotification(NOTIF_POLICE_NEEDED);
        if (ResPop > 500 && StadiumPop == 0)
            ShowNotification(NOTIF_STADIUM_NEEDED);
        if (IndPop > 70 && PortPop == 0)
            ShowNotification(NOTIF_SEAPORT_NEEDED);
        if (ComPop > 100 && APortPop == 0)
            ShowNotification(NOTIF_AIRPORT_NEEDED);
        if (TotalPop > 10 && (CoalPop + NuclearPop == 0))
            ShowNotification(NOTIF_POWER_PLANT_NEEDED);

        if (PwrdZCnt > 0 && unPwrdZCnt > 0) {
            float powerRatio = (float)PwrdZCnt / (float)(PwrdZCnt + unPwrdZCnt);
            if (powerRatio < 0.7f)
                ShowNotification(NOTIF_BLACKOUTS);
        }
    }
    return 0;
}

int CountSpecialTiles() {
    int x, y;
    short tile;
    int needThreshold;

    HospPop = 0;
    ChurchPop = 0;
    CoalPop = 0;
    NuclearPop = 0;

    for (x = 0; x < WORLD_X; x++) {
        for (y = 0; y < WORLD_Y; y++) {
            if (!(Map[x][y] & ZONEBIT)) continue;
            tile = Map[x][y] & LOMASK;
            switch (tile) {
            case HOSPITAL:   HospPop++; break;
            case CHURCH:     ChurchPop++; break;
            case POWERPLANT: CoalPop++; break;
            case NUCLEAR:    NuclearPop++; break;
            }
        }
    }

    needThreshold = ResPop >> 8;
    if (HospPop < needThreshold) NeedHosp = 1;
    else if (HospPop > needThreshold) NeedHosp = -1;
    else NeedHosp = 0;

    if (ChurchPop < needThreshold) NeedChurch = 1;
    else if (ChurchPop > needThreshold) NeedChurch = -1;
    else NeedChurch = 0;
    return 0;
}

const char *GetProblemText(int problemIndex) {
    switch (problemIndex) {
    case PROB_CRIME:        return "Crime";
    case PROB_POLLUTION:    return "Pollution";
    case PROB_HOUSING:      return "Housing";
    case PROB_TAXES:        return "Taxes";
    case PROB_TRAFFIC:      return "Traffic";
    case PROB_UNEMPLOYMENT: return "Unemployment";
    case PROB_FIRE:         return "Fire";
    default:                return "None";
    }
}

const char *GetCityClassName() {
    switch (CityClass) {
    case 0: return "Village";
    case 1: return "Town";
    case 2: return "City";
    case 3: return "Capital";
    case 4: return "Metropolis";
    case 5: return "Megalopolis";
    default: return "Village";
    }
}

int GetTopProblems(short problems[4]) {
    int i;
    for (i = 0; i < 4; i++)
        problems[i] = ProblemOrder[i];
    return 0;
}

int GetProblemVotes(int problemIndex) {
    if (problemIndex >= 0 && problemIndex < 10)
        return ProblemVotes[problemIndex];
    return 0;
}

QUAD GetCityAssessedValue() {
    return CityAssValue;
}

int IsEvaluationValid() {
    return EvalValid;
}

int GetAverageCityScore() {
    return AverageCityScore;
}
