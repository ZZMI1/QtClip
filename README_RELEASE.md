# QtClip Release Guide

## For End Users

1. Download `QtClip-windows-x64.zip`.
2. Extract to any folder (for example `D:\QtClipApp`).
3. Run `qtclipsmoke.exe`.
4. If Windows SmartScreen blocks execution, choose **More info -> Run anyway**.

## Package Contents

- `qtclipsmoke.exe` : Main application.
- Qt runtime DLLs and plugin folders (`platforms`, `styles`, `imageformats`, `sqldrivers`, etc.).
- `i18n/qtclip_zh_CN.qm` : Chinese translation file.

## Build And Package (Maintainer)

Run in repository root:

```powershell
powershell -ExecutionPolicy Bypass -File tools\release\package_release.ps1
```

Optional parameters:

```powershell
powershell -ExecutionPolicy Bypass -File tools\release\package_release.ps1 -BuildDir out -Config Release -OutputDir dist -PackageName QtClip-windows-x64
```

After success, output files are generated in:

- `dist/QtClip-windows-x64/`
- `dist/QtClip-windows-x64.zip`

## Notes

- This package is a portable build (no installer).
- API keys and user settings are stored in user AppData, not inside the zip package.
- If you need an installer (`setup.exe`), wrap the generated folder with Inno Setup/NSIS later.
