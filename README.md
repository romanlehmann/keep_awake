# Keep Awake

Native Windows tray application that keeps the machine from going to sleep,
without a console window or taskbar entry.

Reimplements the behavior of the previous `keep-awake.ps1` script
(`SetThreadExecutionState` + a harmless double Scroll Lock keypress every
minute) as a proper background tray app with persistent settings.

## Features

- Runs entirely in the system tray, no visible window, no taskbar icon.
- Tray context menu: status, start/stop, settings, exit.
- Settings dialog (persisted in `HKCU\Software\PioneerRD\KeepAwake`):
  - Start automatically with Windows (registers itself in the `Run` key).
  - Start minimized to tray.
  - Also keep the display on (`ES_DISPLAY_REQUIRED`).
  - Duration: either "N hours after start" or "until HH:MM".
- Starts keeping the machine awake immediately on launch, using the last
  saved duration setting.

## Build

Requires a MinGW-w64 toolchain (tested with WinLibs GCC 16.1.0,
installed via `winget install --id BrechtSanders.WinLibs.POSIX.UCRT`).

```powershell
mingw32-make
```

Produces `KeepAwake.exe` (statically linked, no extra DLLs required).

## Autostart

Enable "Automatisch mit Windows starten" in the settings dialog; this writes
`HKCU\Software\Microsoft\Windows\CurrentVersion\Run\KeepAwake` pointing at
the current executable path.
