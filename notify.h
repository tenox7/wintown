/* notify.h - Enhanced notification system for WiNTown
 * Based on original WiNTown notification analysis
 */

#ifndef _NOTIFICATIONS_H
#define _NOTIFICATIONS_H

#include <windows.h>

/* Notification types for icon and sound selection */
typedef enum {
    NOTIF_INFO = 0,         /* Blue icon, soft chime - general information */
    NOTIF_WARNING = 1,      /* Yellow icon, alert sound - warnings */
    NOTIF_EMERGENCY = 2,    /* Red icon, siren sound - emergencies */
    NOTIF_MILESTONE = 3,    /* Green icon, celebration - achievements */
    NOTIF_FINANCIAL = 4     /* Orange icon, attention sound - budget issues */
} NotificationType;

/* Notification structure */
typedef struct {
    int id;                     /* Unique notification ID */
    NotificationType type;      /* Type determines icon/sound */
    char title[64];            /* Title bar text */
    char message[256];         /* Main message text */
    char explanation[512];     /* Detailed explanation */
    char advice[512];          /* Recommended actions */
    int hasLocation;           /* 1 if location-specific */
    int locationX, locationY;  /* Map coordinates */
    char locationName[64];     /* District/area name */
    int priority;              /* 0=low, 1=medium, 2=high, 3=critical */
    DWORD timestamp;           /* When notification was created */
} Notification;

/* Dialog resource IDs */
#define IDD_NOTIFICATION_DIALOG 9100
#define IDC_NOTIF_ICON 9101
#define IDC_NOTIF_TITLE 9102
#define IDC_NOTIF_MESSAGE 9103
#define IDC_NOTIF_EXPLANATION 9104
#define IDC_NOTIF_ADVICE 9105
#define IDC_NOTIF_LOCATION 9106
#define IDC_GOTO_LOCATION 9107

/* Notification IDs - organized by category */
/* Emergency Notifications (1000-1999) */
#define NOTIF_FIRE_REPORTED 1001
#define NOTIF_FIRE_SPREADING 1002
#define NOTIF_MULTIPLE_FIRES 1003
#define NOTIF_EARTHQUAKE 1010
#define NOTIF_NUCLEAR_MELTDOWN 1011
#define NOTIF_MONSTER_SIGHTED 1012
#define NOTIF_TORNADO 1013
#define NOTIF_FLOODING 1014
#define NOTIF_PLANE_CRASHED 1020
#define NOTIF_TRAIN_CRASHED 1021
#define NOTIF_SHIP_CRASHED 1022
#define NOTIF_HELICOPTER_CRASHED 1023

/* Infrastructure Notifications (2000-2999) */
#define NOTIF_BLACKOUTS 2001
#define NOTIF_BROWNOUTS 2002
#define NOTIF_POWER_PLANT_NEEDED 2003
#define NOTIF_TRAFFIC_JAMS 2010
#define NOTIF_ROADS_DETERIORATING 2011
#define NOTIF_MORE_ROADS_NEEDED 2012
#define NOTIF_RAIL_SYSTEM_NEEDED 2013

/* Zoning Notifications (3000-3999) */
#define NOTIF_RESIDENTIAL_NEEDED 3001
#define NOTIF_COMMERCIAL_NEEDED 3002
#define NOTIF_INDUSTRIAL_NEEDED 3003
#define NOTIF_STADIUM_NEEDED 3010
#define NOTIF_SEAPORT_NEEDED 3011
#define NOTIF_AIRPORT_NEEDED 3012

/* Public Safety Notifications (4000-4999) */
#define NOTIF_HIGH_CRIME 4001
#define NOTIF_POLICE_NEEDED 4002
#define NOTIF_POLICE_UNDERFUNDED 4003
#define NOTIF_FIRE_DEPT_NEEDED 4010
#define NOTIF_FIRE_DEPT_UNDERFUNDED 4011

/* Environmental Notifications (5000-5999) */
#define NOTIF_HIGH_POLLUTION 5001

/* Financial Notifications (6000-6999) */
#define NOTIF_CITY_BROKE 6001
#define NOTIF_BUDGET_DEFICIT 6002
#define NOTIF_TAX_TOO_HIGH 6003

/* Milestone Notifications (7000-7999) */
#define NOTIF_VILLAGE_2K 7001
#define NOTIF_TOWN_10K 7002
#define NOTIF_CITY_50K 7003
#define NOTIF_CAPITAL_100K 7004
#define NOTIF_METROPOLIS_500K 7005

/* Core notification functions */
void InitNotificationSystem(void);
void ShowNotification(int notificationId, ...);
void ShowNotificationAt(int notificationId, int x, int y, ...);
void CreateNotificationDialog(Notification* notif);

/* Dialog procedure */
BOOL CALLBACK NotificationDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Utility functions */
void CenterMapOnLocation(int x, int y);
const char* GetNotificationTitle(int notificationId);
const char* GetNotificationMessage(int notificationId);
const char* GetNotificationExplanation(int notificationId);
const char* GetNotificationAdvice(int notificationId);
NotificationType GetNotificationType(int notificationId);

/* Enhanced addGameLog that can optionally show dialog */
void addGameLogWithDialog(int showDialog, const char *format, ...);
void addGameLogAtLocation(int x, int y, const char *format, ...);

#endif /* _NOTIFICATIONS_H */
