// Keep Awake - Tray-Anwendung
// Haelt den Rechner wach (SetThreadExecutionState + simulierter Scroll-Lock-Tastendruck),
// ohne dass ein sichtbares Fenster oder eine Konsole stoert.

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "resource.h"
#include "Settings.h"

namespace
{
    const wchar_t* kWindowClassName = L"PioneerRD_KeepAwake_MainWnd";
    const wchar_t* kAppTitle = L"Keep Awake";
    const UINT kTimerIntervalMs = 60 * 1000; // 1x pro Minute Aktivitaet simulieren, wie im Ursprungsskript

    HINSTANCE g_hInstance = nullptr;
    HWND g_hMainWnd = nullptr;
    NOTIFYICONDATAW g_nid = {};
    Settings g_settings;

    bool g_isActive = false;
    SYSTEMTIME g_endTimeLocal = {};

    // ---- Aktivitaet simulieren: identisches Verhalten wie das bisherige PowerShell-Skript ----
    void PressActivityKey()
    {
        // Scroll Lock zweimal druecken (an+aus), damit der LED-Status unveraendert bleibt
        keybd_event(VK_SCROLL, 0, 0, 0);
        keybd_event(VK_SCROLL, 0, KEYEVENTF_KEYUP, 0);
        keybd_event(VK_SCROLL, 0, 0, 0);
        keybd_event(VK_SCROLL, 0, KEYEVENTF_KEYUP, 0);
    }

    void ApplyExecutionState(bool active)
    {
        if (active)
        {
            DWORD flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED;
            if (g_settings.keepDisplayOn)
            {
                flags |= ES_DISPLAY_REQUIRED;
            }
            SetThreadExecutionState(flags);
        }
        else
        {
            SetThreadExecutionState(ES_CONTINUOUS);
        }
    }

    std::wstring GetExePath()
    {
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        return std::wstring(buffer);
    }

    std::wstring FormatTime(const SYSTEMTIME& st)
    {
        wchar_t buf[16];
        swprintf_s(buf, L"%02d:%02d", st.wHour, st.wMinute);
        return std::wstring(buf);
    }

    // Berechnet den Endzeitpunkt basierend auf den aktuellen Einstellungen, ausgehend von "jetzt"
    SYSTEMTIME ComputeEndTime()
    {
        SYSTEMTIME now;
        GetLocalTime(&now);

        if (g_settings.durationMode == DurationMode::Hours)
        {
            FILETIME ft;
            SystemTimeToFileTime(&now, &ft);
            ULARGE_INTEGER uli;
            uli.LowPart = ft.dwLowDateTime;
            uli.HighPart = ft.dwHighDateTime;

            ULONGLONG addTicks = static_cast<ULONGLONG>(g_settings.durationHours) * 3600ULL * 10000000ULL;
            uli.QuadPart += addTicks;

            ft.dwLowDateTime = uli.LowPart;
            ft.dwHighDateTime = uli.HighPart;

            SYSTEMTIME end;
            FileTimeToSystemTime(&ft, &end);
            return end;
        }
        else
        {
            int hh = 18, mm = 0;
            swscanf_s(g_settings.untilTime.c_str(), L"%d:%d", &hh, &mm);

            SYSTEMTIME end = now;
            end.wHour = static_cast<WORD>(hh);
            end.wMinute = static_cast<WORD>(mm);
            end.wSecond = 0;
            end.wMilliseconds = 0;

            // Falls die Zielzeit heute schon vorbei ist, gilt sie fuer morgen
            FILETIME ftNow, ftEnd;
            SystemTimeToFileTime(&now, &ftNow);
            SystemTimeToFileTime(&end, &ftEnd);
            ULARGE_INTEGER uNow, uEnd;
            uNow.LowPart = ftNow.dwLowDateTime; uNow.HighPart = ftNow.dwHighDateTime;
            uEnd.LowPart = ftEnd.dwLowDateTime; uEnd.HighPart = ftEnd.dwHighDateTime;

            if (uEnd.QuadPart <= uNow.QuadPart)
            {
                uEnd.QuadPart += 24ULL * 3600ULL * 10000000ULL;
                ftEnd.dwLowDateTime = uEnd.LowPart;
                ftEnd.dwHighDateTime = uEnd.HighPart;
                FileTimeToSystemTime(&ftEnd, &end);
            }
            return end;
        }
    }

    bool IsNowPastEndTime()
    {
        SYSTEMTIME now;
        GetLocalTime(&now);
        FILETIME ftNow, ftEnd;
        SystemTimeToFileTime(&now, &ftNow);
        SystemTimeToFileTime(&g_endTimeLocal, &ftEnd);
        ULARGE_INTEGER uNow, uEnd;
        uNow.LowPart = ftNow.dwLowDateTime; uNow.HighPart = ftNow.dwHighDateTime;
        uEnd.LowPart = ftEnd.dwLowDateTime; uEnd.HighPart = ftEnd.dwHighDateTime;
        return uNow.QuadPart >= uEnd.QuadPart;
    }

    void UpdateTrayTooltip()
    {
        std::wstring text = kAppTitle;
        if (g_isActive)
        {
            text += L" - aktiv bis " + FormatTime(g_endTimeLocal) + L" Uhr";
        }
        else
        {
            text += L" - inaktiv";
        }
        wcsncpy_s(g_nid.szTip, text.c_str(), _TRUNCATE);
        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    }

    void StopKeepAwake()
    {
        if (!g_isActive) return;
        g_isActive = false;
        KillTimer(g_hMainWnd, TIMER_ID_TICK);
        ApplyExecutionState(false);
        UpdateTrayTooltip();
    }

    void StartKeepAwake()
    {
        g_isActive = true;
        g_endTimeLocal = ComputeEndTime();
        ApplyExecutionState(true);
        PressActivityKey();
        SetTimer(g_hMainWnd, TIMER_ID_TICK, kTimerIntervalMs, nullptr);
        UpdateTrayTooltip();
    }

    void ShowBalloon(const std::wstring& text)
    {
        g_nid.uFlags |= NIF_INFO;
        wcsncpy_s(g_nid.szInfoTitle, kAppTitle, _TRUNCATE);
        wcsncpy_s(g_nid.szInfo, text.c_str(), _TRUNCATE);
        g_nid.dwInfoFlags = NIIF_INFO;
        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
        g_nid.uFlags &= ~NIF_INFO;
    }

    // ---- Einstellungsdialog ----
    void LoadSettingsIntoDialog(HWND hDlg)
    {
        CheckDlgButton(hDlg, IDC_CHK_AUTOSTART_WINDOWS, g_settings.autostartWithWindows ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHK_START_IN_TRAY, g_settings.startInTray ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHK_KEEP_DISPLAY_ON, g_settings.keepDisplayOn ? BST_CHECKED : BST_UNCHECKED);

        CheckRadioButton(hDlg, IDC_RADIO_DURATION_HOURS, IDC_RADIO_DURATION_UNTIL,
            g_settings.durationMode == DurationMode::Hours ? IDC_RADIO_DURATION_HOURS : IDC_RADIO_DURATION_UNTIL);

        SetDlgItemInt(hDlg, IDC_EDIT_HOURS, g_settings.durationHours, FALSE);
        SetDlgItemTextW(hDlg, IDC_EDIT_UNTIL_TIME, g_settings.untilTime.c_str());
    }

    void SaveSettingsFromDialog(HWND hDlg)
    {
        g_settings.autostartWithWindows = IsDlgButtonChecked(hDlg, IDC_CHK_AUTOSTART_WINDOWS) == BST_CHECKED;
        g_settings.startInTray = IsDlgButtonChecked(hDlg, IDC_CHK_START_IN_TRAY) == BST_CHECKED;
        g_settings.keepDisplayOn = IsDlgButtonChecked(hDlg, IDC_CHK_KEEP_DISPLAY_ON) == BST_CHECKED;

        g_settings.durationMode = (IsDlgButtonChecked(hDlg, IDC_RADIO_DURATION_HOURS) == BST_CHECKED)
            ? DurationMode::Hours : DurationMode::UntilTime;

        BOOL ok = FALSE;
        int hours = static_cast<int>(GetDlgItemInt(hDlg, IDC_EDIT_HOURS, &ok, FALSE));
        if (ok && hours > 0 && hours <= 999)
        {
            g_settings.durationHours = hours;
        }

        wchar_t buf[16] = {};
        GetDlgItemTextW(hDlg, IDC_EDIT_UNTIL_TIME, buf, 16);
        int hh = -1, mm = -1;
        if (swscanf_s(buf, L"%d:%d", &hh, &mm) == 2 && hh >= 0 && hh < 24 && mm >= 0 && mm < 60)
        {
            g_settings.untilTime = buf;
        }

        g_settings.Save();
        g_settings.ApplyAutostartRegistration(GetExePath());
    }

    INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            LoadSettingsIntoDialog(hDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDC_BTN_OK:
                SaveSettingsFromDialog(hDlg);
                // Laeuft der Vorgang bereits, Endzeit sofort an neue Einstellungen anpassen
                if (g_isActive)
                {
                    g_endTimeLocal = ComputeEndTime();
                    UpdateTrayTooltip();
                }
                EndDialog(hDlg, IDOK);
                return TRUE;
            case IDC_BTN_CANCEL:
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        return FALSE;
    }

    void OpenSettingsDialog()
    {
        DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_SETTINGS), g_hMainWnd, SettingsDlgProc, 0);
    }

    // ---- Tray-Kontextmenu ----
    void ShowTrayContextMenu(HWND hWnd)
    {
        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();

        std::wstring statusText = g_isActive
            ? (L"Status: aktiv bis " + FormatTime(g_endTimeLocal) + L" Uhr")
            : L"Status: inaktiv";
        AppendMenuW(hMenu, MF_STRING | MF_GRAYED, IDM_TRAY_STATUS, statusText.c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

        if (g_isActive)
        {
            AppendMenuW(hMenu, MF_STRING, IDM_TRAY_STOP, L"Stoppen");
        }
        else
        {
            AppendMenuW(hMenu, MF_STRING, IDM_TRAY_START, L"Starten");
        }

        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_SETTINGS, L"Einstellungen...");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"Beenden");

        SetForegroundWindow(hWnd); // noetig, damit sich das Menu bei Klick daneben korrekt schliesst
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
        PostMessage(hWnd, WM_NULL, 0, 0);

        DestroyMenu(hMenu);
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_TRAYICON:
            switch (lParam)
            {
            case WM_LBUTTONDBLCLK:
                OpenSettingsDialog();
                break;
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                ShowTrayContextMenu(hWnd);
                break;
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDM_TRAY_START:
                StartKeepAwake();
                return 0;
            case IDM_TRAY_STOP:
                StopKeepAwake();
                return 0;
            case IDM_TRAY_SETTINGS:
                OpenSettingsDialog();
                return 0;
            case IDM_TRAY_EXIT:
                DestroyWindow(hWnd);
                return 0;
            }
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_ID_TICK)
            {
                if (IsNowPastEndTime())
                {
                    StopKeepAwake();
                    ShowBalloon(L"Laufzeit abgelaufen, Standby wieder erlaubt.");
                }
                else
                {
                    ApplyExecutionState(true);
                    PressActivityKey();
                    UpdateTrayTooltip();
                }
            }
            return 0;

        case WM_DESTROY:
            StopKeepAwake();
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    g_hInstance = hInstance;

    g_settings.Load();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kWindowClassName;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    RegisterClassExW(&wc);

    // Unsichtbares Hauptfenster: dient nur als Nachrichtenziel fuer Tray/Timer, taucht nicht in der Taskleiste auf
    g_hMainWnd = CreateWindowExW(0, kWindowClassName, kAppTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, nullptr, nullptr, hInstance, nullptr);

    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hMainWnd;
    g_nid.uID = ID_TRAY_APP_ICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    wcsncpy_s(g_nid.szTip, kAppTitle, _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Autostart-Registrierung mit dem aktuellen Pfad synchron halten (falls die Exe verschoben wurde)
    g_settings.ApplyAutostartRegistration(GetExePath());

    // Verhalten wie das bisherige Batch-/PowerShell-Skript: beim Start sofort aktiv werden
    StartKeepAwake();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
