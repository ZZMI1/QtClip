# QtClip

## Positioning

QtClip is a desktop **learning material organizer**.

Core workflow:

> New Course -> Paste/Import Screenshot -> AI Summary -> Export Markdown

QtClip focuses on this closed loop instead of long-term knowledge base features.

## Current Main Features

- Two-column main window (left navigation + right workspace)
- Course and screenshot two-level navigation
- Screenshot preview + editable AI summary workspace
- AI summarize for snippet/session (async)
- Markdown export (minimal format: course title + AI summary)
- SQLite persistence for sessions/snippets/settings

## Tech Stack

- Qt 5.15.x (Widgets)
- C++17
- SQLite
- CMake + VS2022

## Build

```powershell
cmake -S . -B out
cmake --build out --config Release
```

## Run

```powershell
out/Release/qtclipsmoke.exe
```

## Smoke Test

```powershell
out/Release/qtclipsmoke.exe --smoke
```


## Preflight Check (Maintainer)

Run before packaging or pushing release changes:

```powershell
powershell -ExecutionPolicy Bypass -File tools\qa\preflight.ps1
```

This validates:
- build success
- AI config connectivity command
- smoke workflow
