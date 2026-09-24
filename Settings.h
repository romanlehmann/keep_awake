#pragma once
#include <windows.h>
#include <string>

// Duration-Modus fuer die Laufzeit des Keep-Awake-Vorgangs
enum class DurationMode : DWORD
{
    Hours = 0,   // laeuft N Stunden ab Start
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

    void Load();
    void Save() const;

    // Windows-Autostart ueber HKCU Run-Key spiegeln
    void ApplyAutostartRegistration(const std::wstring& exePath) const;
};
