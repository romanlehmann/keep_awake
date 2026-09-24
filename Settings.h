#pragma once
#include <windows.h>
#include <string>

// Duration-Modus fuer die Laufzeit des Keep-Awake-Vorgangs
enum class DurationMode : int
{
    Hours = 0,    // laeuft N Stunden ab Start
    UntilTime = 1 // laeuft bis zu einer festen Uhrzeit (HH:MM, ggf. am naechsten Tag)
};

struct Settings
{
    bool autostartWithWindows = false;
    bool startInTray = true;
    bool keepDisplayOn = false;
    DurationMode durationMode = DurationMode::Hours;
    int durationHours = 10;          // Standard ~ 580 Minuten wie im bisherigen Skript
    std::wstring untilTime = L"18:00";

    // Liest/schreibt eine einfache INI-Datei unter %APPDATA%\KeepAwake\settings.ini
    // (keine Registry, damit keine besonderen Rechte noetig sind)
    void Load();
    void Save() const;

    // Legt/entfernt eine .lnk-Verknuepfung im Windows-Autostart-Ordner
    // (%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup) statt eines Registry-Run-Keys.
    void ApplyAutostartRegistration(const std::wstring& exePath) const;

    static std::wstring GetSettingsFilePath();
    static std::wstring GetStartupShortcutPath();
};
