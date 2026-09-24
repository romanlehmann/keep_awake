#include "Settings.h"

static const wchar_t* kRegSettingsPath = L"Software\\PioneerRD\\KeepAwake";
static const wchar_t* kRegRunPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* kRegRunValueName = L"KeepAwake";

static DWORD ReadDword(HKEY key, const wchar_t* name, DWORD defaultValue)
{
    DWORD value = defaultValue;
    DWORD size = sizeof(value);
    DWORD type = REG_DWORD;
    if (RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size) != ERROR_SUCCESS)
    {
        return defaultValue;
    }
    return value;
}

static std::wstring ReadString(HKEY key, const wchar_t* name, const std::wstring& defaultValue)
{
    wchar_t buffer[64] = {};
    DWORD size = sizeof(buffer);
    DWORD type = REG_SZ;
    if (RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) != ERROR_SUCCESS)
    {
        return defaultValue;
    }
    return std::wstring(buffer);
}

void Settings::Load()
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegSettingsPath, 0, KEY_READ, &key) != ERROR_SUCCESS)
    {
        return; // Noch keine Einstellungen gespeichert, Defaults verwenden
    }

    autostartWithWindows = ReadDword(key, L"AutostartWithWindows", 0) != 0;
    startInTray = ReadDword(key, L"StartInTray", 1) != 0;
    keepDisplayOn = ReadDword(key, L"KeepDisplayOn", 0) != 0;
    durationMode = static_cast<DurationMode>(ReadDword(key, L"DurationMode", 0));
    durationHours = static_cast<int>(ReadDword(key, L"DurationHours", 10));
    untilTime = ReadString(key, L"UntilTime", L"18:00");

    RegCloseKey(key);
}

void Settings::Save() const
{
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegSettingsPath, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
    {
        return;
    }

    DWORD v;
    v = autostartWithWindows ? 1 : 0;
    RegSetValueExW(key, L"AutostartWithWindows", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
    v = startInTray ? 1 : 0;
    RegSetValueExW(key, L"StartInTray", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
    v = keepDisplayOn ? 1 : 0;
    RegSetValueExW(key, L"KeepDisplayOn", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
    v = static_cast<DWORD>(durationMode);
    RegSetValueExW(key, L"DurationMode", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
    v = static_cast<DWORD>(durationHours);
    RegSetValueExW(key, L"DurationHours", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
    RegSetValueExW(key, L"UntilTime", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(untilTime.c_str()),
        static_cast<DWORD>((untilTime.size() + 1) * sizeof(wchar_t)));

    RegCloseKey(key);
}

void Settings::ApplyAutostartRegistration(const std::wstring& exePath) const
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegRunPath, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
    {
        return;
    }

    if (autostartWithWindows)
    {
        std::wstring command = L"\"" + exePath + L"\"";
        RegSetValueExW(key, kRegRunValueName, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    }
    else
    {
        RegDeleteValueW(key, kRegRunValueName);
    }

    RegCloseKey(key);
}
