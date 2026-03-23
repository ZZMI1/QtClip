# QtClip Agents Guide

## Project
- QtClip is a Qt5 + VS2022 desktop app for course screenshot organization and AI note assistance.
- Current focus is the first-release pipeline: class session -> screen capture or image import -> quick note -> AI summary -> Markdown export.

## Naming Rules
- File names are lowercase.
- Class names use the `QC` prefix.
- Member variables use the `m_` prefix.
- Prefer separated `.h` and `.cpp` files.

## Database And Repository Rules
- SQLite access stays inside `repository/sqlite` and `core/database`.
- UI and service layers must not execute SQL directly.
- Enum TEXT fields must use `QCDbEnumCodec` for database encoding and decoding.
- `ImageSnippetType` records must use `createSnippetWithPrimaryAttachment(...)`.
- Repository delete methods currently remove database rows only; file deletion is not handled there.
- Markdown export currently writes files directly and does not persist export records yet.

## Completed Modules
- `core/database`: `qcdatabasemanager`, `qcdatabaseschema`, `qcdbenumcodec`
- `domain/entities`: session, snippet, attachment, tag, AI record, app setting, export record
- `domain/interfaces`: session, snippet, tag, AI record, settings, export repositories
- `repository/sqlite`: session, snippet, AI record repositories
- `services`: session, snippet, AI thin services, export data service, Markdown renderer, Markdown export service, screen capture service
- `ui/widgets`: main window plus create-session, create-text-snippet, create-image-snippet, quick-capture dialogs

## Build Command
- Configure and build: `cmake -S . -B out; if ($LASTEXITCODE -eq 0) { cmake --build out --config Release }`
- GUI executable: `out/Release/qtclipsmoke.exe`
- Smoke mode: `out/Release/qtclipsmoke.exe --smoke`
- Current third-party Qt layout should be treated as `Release` for builds.

## Next Priority
- Keep the project compiling cleanly.
- Then decide whether `IQCExportRepository` should be implemented before `QCTagRepositorySqlite`.
- After that, add true area selection capture on top of the current full-screen capture flow.
