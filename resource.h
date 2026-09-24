#pragma once

// Tray
#define WM_TRAYICON            (WM_APP + 1)
#define ID_TRAY_APP_ICON       1001

// Tray context menu
#define IDM_TRAY_STATUS        2001
#define IDM_TRAY_START         2002
#define IDM_TRAY_STOP          2003
#define IDM_TRAY_SETTINGS      2004
#define IDM_TRAY_EXIT          2005

// Settings dialog
#define IDD_SETTINGS                    3000
#define IDC_CHK_AUTOSTART_WINDOWS       3001
#define IDC_CHK_KEEP_DISPLAY_ON         3003
#define IDC_RADIO_DURATION_HOURS        3004
#define IDC_RADIO_DURATION_UNTIL        3005
#define IDC_EDIT_HOURS                  3006
#define IDC_EDIT_UNTIL_TIME             3007
#define IDC_BTN_OK                      3008
#define IDC_BTN_CANCEL                  3009
#define IDC_STATIC_GROUP_GENERAL        3010
#define IDC_STATIC_GROUP_DURATION       3011
#define IDC_STATIC_HOURS_LABEL          3012
#define IDC_STATIC_UNTIL_LABEL          3013

// Icon resource
#define IDI_MAIN_ICON           4001

// Timer
#define TIMER_ID_TICK           5001
