#include "Settings.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <fstream>
#include <sstream>
#include <map>

namespace
{
    std::wstring GetAppDataFolder()
    {
        PWSTR path = nullptr;
        std::wstring result;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path)))
        {
            result = path;
            CoTaskMemFree(path);
        }
        return result;
    }

    std::wstring GetStartupFolder()
    {
        PWSTR path = nullptr;
        std::wstring result;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &path)))
        {
            result = path;
            CoTaskMemFree(path);
        }
        return result;
    }

    void EnsureDirectoryExists(const std::wstring& dir)
    {
        SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
    }

    std::wstring Trim(const std::wstring& s)
    {
        size_t start = s.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos) return L"";
        size_t end = s.find_last_not_of(L" \t\r\n");
        return s.substr(start, end - start + 1);
    }
}

std::wstring Settings::GetSettingsFilePath()
{
    std::wstring dir = GetAppDataFolder() + L"\\KeepAwake";
    EnsureDirectoryExists(dir);
    return dir + L"\\settings.ini";
}

std::wstring Settings::GetStartupShortcutPath()
{
    return GetStartupFolder() + L"\\KeepAwake.lnk";
}

void Settings::Load()
{
    std::wifstream file(GetSettingsFilePath().c_str());
    if (!file.is_open())
    {
        return; // Noch keine Datei vorhanden, Defaults verwenden
    }

    std::map<std::wstring, std::wstring> values;
    std::wstring line;
    while (std::getline(file, line))
    {
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;
        std::wstring key = Trim(line.substr(0, eq));
        std::wstring value = Trim(line.substr(eq + 1));
        values[key] = value;
    }

    auto getBool = [&](const wchar_t* key, bool def) {
        auto it = values.find(key);
        return it != values.end() ? (it->second == L"1") : def;
    };
    auto getInt = [&](const wchar_t* key, int def) {
        auto it = values.find(key);
        if (it == values.end()) return def;
        try { return std::stoi(it->second); } catch (...) { return def; }
    };
    auto getStr = [&](const wchar_t* key, const std::wstring& def) {
        auto it = values.find(key);
        return it != values.end() ? it->second : def;
    };

    autostartWithWindows = getBool(L"AutostartWithWindows", false);
    keepDisplayOn = getBool(L"KeepDisplayOn", false);
    durationMode = static_cast<DurationMode>(getInt(L"DurationMode", 0));
    durationHours = getInt(L"DurationHours", 10);
    untilTime = getStr(L"UntilTime", L"18:00");
}

void Settings::Save() const
{
    std::wofstream file(GetSettingsFilePath().c_str(), std::ios::trunc);
    if (!file.is_open())
    {
        return;
    }

    file << L"AutostartWithWindows=" << (autostartWithWindows ? 1 : 0) << L"\n";
    file << L"KeepDisplayOn=" << (keepDisplayOn ? 1 : 0) << L"\n";
    file << L"DurationMode=" << static_cast<int>(durationMode) << L"\n";
    file << L"DurationHours=" << durationHours << L"\n";
    file << L"UntilTime=" << untilTime << L"\n";
}

void Settings::ApplyAutostartRegistration(const std::wstring& exePath) const
{
    std::wstring shortcutPath = GetStartupShortcutPath();

    if (!autostartWithWindows)
    {
        DeleteFileW(shortcutPath.c_str());
        return;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool needUninit = SUCCEEDED(hr);

    IShellLinkW* shellLink = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&shellLink));
    if (SUCCEEDED(hr) && shellLink)
    {
        shellLink->SetPath(exePath.c_str());
        shellLink->SetDescription(L"Keep Awake - haelt den Rechner wach");

        IPersistFile* persistFile = nullptr;
        hr = shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persistFile));
        if (SUCCEEDED(hr) && persistFile)
        {
            persistFile->Save(shortcutPath.c_str(), TRUE);
            persistFile->Release();
        }
        shellLink->Release();
    }

    if (needUninit)
    {
        CoUninitialize();
    }
}
