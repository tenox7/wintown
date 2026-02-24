/* notify.c - Original WiNTown message system for WiNTown
 * Based on s_msg.c from original WiNTown
 */

#include "notify.h"
#include "sim.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

/* External functions */
extern HWND hwndMain;
extern void addGameLog(const char *format, ...);
extern void addDebugLog(const char *format, ...);

/* Message system variables - like original WiNTown */
int MessagePort = 0;
int MesX = 0, MesY = 0;
int MesNum = 0;
int LastPicNum = 0;
DWORD LastMesTime = 0;

/* External simulation variables */
extern int CityTime;
extern int TotalZPop, ResZPop, ComZPop, IndZPop;
extern int NuclearPop, CoalPop;
extern int RoadTotal, RailTotal;
extern int ResPop, StadiumPop, IndPop, PortPop, ComPop, APortPop;
extern int UnpwrdZCnt, PwrdZCnt;
extern int PollutionAverage, CrimeAverage, TotalPop, FireStPop, PolicePop;
extern int TaxRate, RoadEffect, FireEffect, PoliceEffect, TrafficAverage;
extern short ScenarioID, ScoreType, ScoreWait;
extern int ResCap, IndCap, ComCap;

/* Forward declarations */
int SendMes(int Mnum);
void SendMesAt(int Mnum, int x, int y);
void CheckGrowth(void);
DoScenarioScore(int scoreType);
void ClearMes(void);

/* Original SendMessages function - called every simulation cycle */
SendMessages() {
    int z;
    int PowerPop;
    float TM;

    if (ScenarioID && ScoreType && ScoreWait) {
        ScoreWait--;
        if (!ScoreWait)
            DoScenarioScore(ScoreType);
    }

    CheckGrowth();

    /* Sync message system variables with simulation variables */
    TotalZPop = ResZPop + ComZPop + IndZPop;
    PowerPop = NuclearPop + CoalPop;
    /* CityTax variable eliminated - using TaxRate directly */

    z = CityTime & 63;

    switch(z) {
        case 1:
            if ((TotalZPop >> 2) >= ResZPop)
                SendMes(1); /* need Res */
            break;
        case 5:
            if ((TotalZPop >> 3) >= ComZPop)
                SendMes(2); /* need Com */
            break;
        case 10:
            if ((TotalZPop >> 3) >= IndZPop)
                SendMes(3); /* need Ind */
            break;
        case 14:
            if ((TotalZPop > 10) && ((TotalZPop << 1) > RoadTotal))
                SendMes(4);
            break;
        case 18:
            if ((TotalZPop > 50) && (TotalZPop > RailTotal))
                SendMes(5);
            break;
        case 22:
            if ((TotalZPop > 10) && (PowerPop == 0))
                SendMes(6); /* need Power */
            break;
        case 26:
            if ((ResPop > 500) && (StadiumPop == 0)) {
                SendMes(7); /* need Stad */
                ResCap = 1;
            } else
                ResCap = 0;
            break;
        case 28:
            if ((IndPop > 70) && (PortPop == 0)) {
                SendMes(8);
                IndCap = 1;
            } else
                IndCap = 0;
            break;
        case 30:
            if ((ComPop > 100) && (APortPop == 0)) {
                SendMes(9);
                ComCap = 1;
            } else
                ComCap = 0;
            break;
        case 32:
            TM = (float)(UnpwrdZCnt + PwrdZCnt);
            if (TM)
                if ((PwrdZCnt / TM) < 0.7)
                    SendMes(15);
            break;
        case 35:
            if (PollutionAverage > 60)
                SendMes(-10);
            break;
        case 42:
            if (CrimeAverage > 100)
                SendMes(-11);
            break;
        case 45:
            if ((TotalPop > 60) && (FireStPop == 0))
                SendMes(13);
            break;
        case 48:
            if ((TotalPop > 60) && (PolicePop == 0))
                SendMes(14);
            break;
        case 51:
            if (TaxRate > 12)
                SendMes(16);
            break;
        case 54:
            if ((RoadEffect < 20) && (RoadTotal > 30))
                SendMes(17);
            break;
        case 57:
            if ((FireEffect < 700) && (TotalPop > 20))
                SendMes(18);
            break;
        case 60:
            if ((PoliceEffect < 700) && (TotalPop > 20))
                SendMes(19);
            break;
        case 63:
            if (TrafficAverage > 60)
                SendMes(-12);
            break;
    }
    return 0;
}

/* Original CheckGrowth function */
void CheckGrowth(void) {
    static QUAD LastCityPop = 0;
    static int LastCategory = 0;
    QUAD ThisCityPop;
    int z;

    if (!(CityTime & 3)) {
        z = 0;
        ThisCityPop = CalculateCityPopulation(ResPop, ComPop, IndPop);
        if (LastCityPop) {
            if ((LastCityPop < 2000) && (ThisCityPop >= 2000)) z = 35;
            if ((LastCityPop < 10000) && (ThisCityPop >= 10000)) z = 36;
            if ((LastCityPop < 50000L) && (ThisCityPop >= 50000L)) z = 37;
            if ((LastCityPop < 100000L) && (ThisCityPop >= 100000L)) z = 38;
            if ((LastCityPop < 500000L) && (ThisCityPop >= 500000L)) z = 39;
        }
        if (z)
            if (z != LastCategory) {
                SendMes(-z);
                LastCategory = z;
            }
        LastCityPop = ThisCityPop;
    }
}

/* Original SendMes function - CRITICAL anti-spam logic */
int SendMes(int Mnum) {
    if (Mnum < 0) {
        /* Picture message (disaster) - only if different from last */
        if (Mnum != LastPicNum) {
            MessagePort = Mnum;
            MesX = 0;
            MesY = 0;
            LastPicNum = Mnum;
            addDebugLog("SendMes: Picture message %d allowed (different from last %d)", Mnum, LastPicNum);
            return 1;
        } else {
            addDebugLog("SendMes: Picture message %d BLOCKED - same as last", Mnum);
            return 0;
        }
    } else {
        /* Text message - only if no active message */
        if (!MessagePort) {
            MessagePort = Mnum;
            MesX = 0;
            MesY = 0;
            addDebugLog("SendMes: Text message %d allowed", Mnum);
            return 1;
        } else {
            addDebugLog("SendMes: Text message %d BLOCKED - active message %d", Mnum, MessagePort);
            return 0;
        }
    }
}

/* Original SendMesAt function - for disaster locations */
void SendMesAt(int Mnum, int x, int y) {
    if (SendMes(Mnum)) {
        MesX = x;
        MesY = y;
        addDebugLog("SendMesAt: Message %d at (%d,%d)", Mnum, x, y);
    }
}

/* Clear message system */
void ClearMes(void) {
    MessagePort = 0;
    MesX = 0;
    MesY = 0;
    LastPicNum = 0;
    addDebugLog("ClearMes: Message system cleared");
}

/* Process active message */
void doMessage(void) {
    char messageStr[256];
    int firstTime;

    messageStr[0] = 0;

    if (MessagePort) {
        MesNum = MessagePort;
        MessagePort = 0;
        LastMesTime = GetTickCount();
        firstTime = 1;
        addDebugLog("doMessage: Processing message %d", MesNum);
    } else {
        firstTime = 0;
        if (MesNum == 0) return;
        if (MesNum < 0) {
            MesNum = -MesNum;
            LastMesTime = GetTickCount();
        } else if ((GetTickCount() - LastMesTime) > (60 * 1000)) { /* 60 seconds timeout */
            MesNum = 0;
            addDebugLog("doMessage: Message timed out");
            return;
        }
    }

    if (MesNum >= 0) {
        if (MesNum == 0) return;
        if (MesNum > 60) {
            MesNum = 0;
            return;
        }

        /* Get message text */
        switch(MesNum) {
            case 1: strcpy(messageStr, "More residential zones needed."); break;
            case 2: strcpy(messageStr, "More commercial zones needed."); break;
            case 3: strcpy(messageStr, "More industrial zones needed."); break;
            case 4: strcpy(messageStr, "More roads required."); break;
            case 5: strcpy(messageStr, "More rail system needed."); break;
            case 6: strcpy(messageStr, "More power needed."); break;
            case 7: strcpy(messageStr, "Residents demand a stadium."); break;
            case 8: strcpy(messageStr, "Industry requires a seaport."); break;
            case 9: strcpy(messageStr, "Commerce requires an airport."); break;
            case 10: strcpy(messageStr, "Pollution very high."); break;
            case 11: strcpy(messageStr, "Crime very high."); break;
            case 12: strcpy(messageStr, "Traffic very heavy."); break;
            case 13: strcpy(messageStr, "Fire protection needed."); break;
            case 14: strcpy(messageStr, "Police protection needed."); break;
            case 15: strcpy(messageStr, "Blackouts reported. More power needed."); break;
            case 16: strcpy(messageStr, "Tax rate too high."); break;
            case 17: strcpy(messageStr, "Roads deteriorating rapidly."); break;
            case 18: strcpy(messageStr, "Fire departments need funding."); break;
            case 19: strcpy(messageStr, "Police departments need funding."); break;
            case 20: strcpy(messageStr, "Fire reported!"); break;
            case 21: strcpy(messageStr, "Monster attack!"); break;
            case 22: strcpy(messageStr, "Earthquake!"); break;
            case 23: strcpy(messageStr, "Tornado!"); break;
            case 24: strcpy(messageStr, "Flooding!"); break;
            case 25: strcpy(messageStr, "Nuclear meltdown!"); break;
            case 26: strcpy(messageStr, "Airplane crashed!"); break;
            case 27: strcpy(messageStr, "Train crashed!"); break;
            case 28: strcpy(messageStr, "Ship crashed!"); break;
            case 29: strcpy(messageStr, "Helicopter crashed!"); break;
            case 30: strcpy(messageStr, "Major fire spreading!"); break;
            case 35: strcpy(messageStr, "Population reached 2,000! Your village has become a town."); break;
            case 36: strcpy(messageStr, "Population reached 10,000! Your town has become a city."); break;
            case 37: strcpy(messageStr, "Population reached 50,000! Your city has become a capital."); break;
            case 38: strcpy(messageStr, "Population reached 100,000! Your capital has become a metropolis."); break;
            case 39: strcpy(messageStr, "Population reached 500,000! Your metropolis has become a megalopolis."); break;
            default: sprintf(messageStr, "City Message %d", MesNum); break;
        }

        addGameLog("CITY: %s", messageStr);

    } else {
        /* Picture message (disaster) */
        int pictId = -(MesNum);
        
        switch(pictId) {
            case 10: strcpy(messageStr, "Pollution very high!"); break;
            case 11: strcpy(messageStr, "Crime wave spreading!"); break;
            case 12: strcpy(messageStr, "Traffic jams everywhere!"); break;
            case 20: strcpy(messageStr, "FIRE REPORTED!"); break;
            case 21: strcpy(messageStr, "MONSTER ATTACK!"); break;
            case 22: strcpy(messageStr, "EARTHQUAKE!"); break;
            case 23: strcpy(messageStr, "TORNADO!"); break;
            case 24: strcpy(messageStr, "FLOODING!"); break;
            case 25: strcpy(messageStr, "NUCLEAR MELTDOWN!"); break;
            case 35: strcpy(messageStr, "Population reached 2,000!"); break;
            case 36: strcpy(messageStr, "Population reached 10,000!"); break;
            case 37: strcpy(messageStr, "Population reached 50,000!"); break;
            case 38: strcpy(messageStr, "Population reached 100,000!"); break;
            case 39: strcpy(messageStr, "Population reached 500,000!"); break;
            default: sprintf(messageStr, "Disaster %d", pictId); break;
        }

        if (MesX > 0 && MesY > 0) {
            char locStr[64];
            sprintf(locStr, " at (%d,%d)", MesX, MesY);
            strcat(messageStr, locStr);
        }

        addGameLog("ALERT: %s", messageStr);
        
        /* Show dialog for disasters only */
        if (pictId >= 20 && pictId <= 25) {
            Notification notif;
            notif.id = pictId;
            notif.locationX = MesX;
            notif.locationY = MesY;
            notif.hasLocation = (MesX > 0 && MesY > 0) ? 1 : 0;
            strcpy(notif.message, messageStr);
            notif.timestamp = GetTickCount();
            notif.priority = 3;
            CreateNotificationDialog(&notif);
        }

        MessagePort = pictId; /* resend text message */
    }
}

/* Replace old notification functions with new SendMes equivalents */
void ShowNotification(int notificationId, ...) {
    /* Map new IDs to original message numbers */
    switch(notificationId) {
        case NOTIF_EARTHQUAKE:
            SendMes(-22);
            break;
        case NOTIF_FIRE_REPORTED:
            SendMes(-20);
            break;
        case NOTIF_MONSTER_SIGHTED:
            SendMes(-21);
            break;
        default:
            addDebugLog("ShowNotification: Unknown ID %d", notificationId);
            break;
    }
}

void ShowNotificationAt(int notificationId, int x, int y, ...) {
    /* Map new IDs to original message numbers */
    switch(notificationId) {
        case NOTIF_EARTHQUAKE:
            SendMesAt(-22, x, y);
            break;
        case NOTIF_FIRE_REPORTED:
            SendMesAt(-20, x, y);
            break;
        case NOTIF_MONSTER_SIGHTED:
            SendMesAt(-21, x, y);
            break;
        case NOTIF_TORNADO:
            SendMesAt(-23, x, y);
            break;
        case NOTIF_FLOODING:
            SendMesAt(-24, x, y);
            break;
        case NOTIF_NUCLEAR_MELTDOWN:
            SendMesAt(-25, x, y);
            break;
        default:
            addDebugLog("ShowNotificationAt: Unknown ID %d", notificationId);
            break;
    }
}

/* DoScenarioScore is defined in scenario.c */

/* Notification dialog - keep existing implementation */
static Notification* currentNotification = NULL;

void CreateNotificationDialog(Notification* notif) {
    if (!notif) return;
    
    currentNotification = notif;
    addDebugLog("Creating notification dialog for ID %d", notif->id);
    
    DialogBoxParam(GetModuleHandle(NULL), 
                   MAKEINTRESOURCE(IDD_NOTIFICATION_DIALOG),
                   hwndMain,
                   (DLGPROC)NotificationDialogProc,
                   (LPARAM)notif);
}

BOOL CALLBACK NotificationDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Notification* notif = currentNotification;
    char titleText[256];
    char explanationText[512];
    char adviceText[512];
    char locationText[128];
    
    switch (msg) {
        case WM_INITDIALOG:
            if (!notif) return FALSE;
            
            switch (notif->id) {
                case 22: /* EARTHQUAKE */
                    strcpy(titleText, "EARTHQUAKE DISASTER");
                    strcpy(explanationText, "A major earthquake has struck your city! Buildings have been damaged and infrastructure may be compromised.");
                    strcpy(adviceText, "Rebuild damaged areas quickly. Consider earthquake-resistant construction for the future.");
                    break;
                case 20: /* FIRE */
                    strcpy(titleText, "FIRE EMERGENCY");
                    strcpy(explanationText, "A serious fire has broken out in your city! Fire departments are responding to the emergency.");
                    strcpy(adviceText, "Ensure adequate fire station coverage. Consider fireproof building materials in high-risk areas.");
                    break;
                case 21: /* MONSTER */
                    strcpy(titleText, "MONSTER ATTACK");
                    strcpy(explanationText, "A giant monster has appeared in your city! It is causing massive destruction as it moves through the area.");
                    strcpy(adviceText, "The monster will eventually leave on its own. Focus on rebuilding damaged areas afterward.");
                    break;
                default:
                    strcpy(titleText, "CITY ALERT");
                    strcpy(explanationText, "An important event has occurred in your city that requires your attention.");
                    strcpy(adviceText, "Review the situation and take appropriate action as needed.");
                    break;
            }
            
            SetDlgItemText(hwnd, IDC_NOTIF_TITLE, titleText);
            SetDlgItemText(hwnd, IDC_NOTIF_MESSAGE, notif->message);
            SetDlgItemText(hwnd, IDC_NOTIF_EXPLANATION, explanationText);
            SetDlgItemText(hwnd, IDC_NOTIF_ADVICE, adviceText);
            
            if (notif->hasLocation && notif->locationX >= 0 && notif->locationY >= 0) {
                sprintf(locationText, "Location: Sector %d, %d", notif->locationX, notif->locationY);
                SetDlgItemText(hwnd, IDC_NOTIF_LOCATION, locationText);
                EnableWindow(GetDlgItem(hwnd, IDC_GOTO_LOCATION), TRUE);
            } else {
                SetDlgItemText(hwnd, IDC_NOTIF_LOCATION, "Location: Unknown");
                EnableWindow(GetDlgItem(hwnd, IDC_GOTO_LOCATION), FALSE);
            }
            
            return TRUE;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return TRUE;
                    
                case IDC_GOTO_LOCATION:
                    if (notif && notif->hasLocation && notif->locationX >= 0 && notif->locationY >= 0) {
                        CenterMapOnLocation(notif->locationX, notif->locationY);
                        addGameLog("Centered map on disaster location (%d, %d)", notif->locationX, notif->locationY);
                    }
                    return TRUE;
            }
            break;
    }
    
    return FALSE;
}

void CenterMapOnLocation(int x, int y) {
    addGameLog("Centering map on (%d, %d)", x, y);
}

/* Initialize notification system */
void InitNotificationSystem(void) {
    ClearMes();
    addDebugLog("Notification system initialized with original WiNTown logic");
}
