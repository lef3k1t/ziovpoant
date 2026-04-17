# ZIOVPOANT

Windows GUI application in C++ with CMake build.

## Implemented

- tray icon is created on startup;
- left click on the tray icon shows the main window;
- right click on the tray icon opens a context menu;
- tray context menu contains `Open` and `Exit`;
- tray icon is restored after Explorer/taskbar recreation via `TaskbarCreated`;
- hidden startup mode is supported with `--hidden` or `--tray-only`;
- closing the main window hides the app to tray instead of terminating it;
- main window menu contains `File -> Exit`;
- second instance is blocked with a named mutex;
- GitHub Actions builds the project on Windows;
- build artifact contains `ZIOVPOANT.exe`.

## Local build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Executable path:

```text
build/Release/ZIOVPOANT.exe
```

## Hidden launch

```powershell
.\build\Release\ZIOVPOANT.exe --hidden
```
