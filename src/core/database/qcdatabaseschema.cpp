// File: qcdatabaseschema.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Defines database schema statements for QtClip SQLite initialization.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcdatabaseschema.h"

#include <QString>

namespace
{
static const char *g_pszSchemaVersionKey = "schema_version";
static const int g_nCurrentSchemaVersion = 1;

static const char *g_pszCreateStudySessionsTable =
    "CREATE TABLE IF NOT EXISTS study_sessions ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "title TEXT NOT NULL, "
    "course_name TEXT, "
    "description TEXT, "
    "status TEXT NOT NULL CHECK (status IN ('ActiveSessionStatus', 'FinishedSessionStatus')), "
    "started_at TEXT NOT NULL, "
    "ended_at TEXT, "
    "created_at TEXT NOT NULL, "
    "updated_at TEXT NOT NULL"
    ");";

static const char *g_pszCreateSnippetsTable =
    "CREATE TABLE IF NOT EXISTS snippets ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "session_id INTEGER NOT NULL, "
    "type TEXT NOT NULL CHECK (type IN ('ImageSnippetType', 'TextSnippetType', 'CodeSnippetType')), "
    "capture_type TEXT NOT NULL CHECK (capture_type IN ('ScreenCaptureType', 'ManualCaptureType', 'ImportCaptureType')), "
    "title TEXT, "
    "content_text TEXT, "
    "summary TEXT, "
    "note TEXT, "
    "note_kind TEXT NOT NULL CHECK (note_kind IN ('ConceptNoteKind', 'CodeNoteKind', 'HomeworkNoteKind', 'DefinitionNoteKind', 'QuestionNoteKind', 'ReviewNoteKind')), "
    "note_level TEXT NOT NULL CHECK (note_level IN ('NormalNoteLevel', 'ImportantNoteLevel', 'ReviewNoteLevel')), "
    "source TEXT, "
    "language TEXT, "
    "is_favorite INTEGER NOT NULL DEFAULT 0 CHECK (is_favorite IN (0, 1)), "
    "is_archived INTEGER NOT NULL DEFAULT 0 CHECK (is_archived IN (0, 1)), "
    "captured_at TEXT NOT NULL, "
    "created_at TEXT NOT NULL, "
    "updated_at TEXT NOT NULL, "
    "FOREIGN KEY(session_id) REFERENCES study_sessions(id) ON DELETE CASCADE"
    ");";

static const char *g_pszCreateAttachmentsTable =
    "CREATE TABLE IF NOT EXISTS attachments ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "snippet_id INTEGER NOT NULL, "
    "file_path TEXT NOT NULL, "
    "file_name TEXT, "
    "mime_type TEXT, "
    "created_at TEXT NOT NULL, "
    "FOREIGN KEY(snippet_id) REFERENCES snippets(id) ON DELETE CASCADE"
    ");";

static const char *g_pszCreateTagsTable =
    "CREATE TABLE IF NOT EXISTS tags ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "name TEXT NOT NULL UNIQUE, "
    "color TEXT, "
    "created_at TEXT NOT NULL"
    ");";

static const char *g_pszCreateSnippetTagsTable =
    "CREATE TABLE IF NOT EXISTS snippet_tags ("
    "snippet_id INTEGER NOT NULL, "
    "tag_id INTEGER NOT NULL, "
    "PRIMARY KEY(snippet_id, tag_id), "
    "FOREIGN KEY(snippet_id) REFERENCES snippets(id) ON DELETE CASCADE, "
    "FOREIGN KEY(tag_id) REFERENCES tags(id) ON DELETE CASCADE"
    ");";

static const char *g_pszCreateAiRecordsTable =
    "CREATE TABLE IF NOT EXISTS ai_records ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "session_id INTEGER, "
    "snippet_id INTEGER, "
    "task_type TEXT NOT NULL CHECK (task_type IN ('SnippetTitleTask', 'SnippetSummaryTask', 'SnippetTagsTask', 'SessionSummaryTask')), "
    "provider_name TEXT NOT NULL, "
    "model_name TEXT, "
    "prompt_text TEXT, "
    "response_text TEXT, "
    "status TEXT NOT NULL, "
    "error_message TEXT, "
    "created_at TEXT NOT NULL, "
    "CHECK (session_id IS NOT NULL OR snippet_id IS NOT NULL), "
    "FOREIGN KEY(session_id) REFERENCES study_sessions(id) ON DELETE SET NULL, "
    "FOREIGN KEY(snippet_id) REFERENCES snippets(id) ON DELETE SET NULL"
    ");";

static const char *g_pszCreateAppSettingsTable =
    "CREATE TABLE IF NOT EXISTS app_settings ("
    "key TEXT PRIMARY KEY, "
    "value TEXT, "
    "updated_at TEXT NOT NULL"
    ");";

static const char *g_pszCreateExportsTable =
    "CREATE TABLE IF NOT EXISTS exports ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "session_id INTEGER NOT NULL, "
    "export_type TEXT NOT NULL, "
    "file_path TEXT NOT NULL, "
    "created_at TEXT NOT NULL, "
    "FOREIGN KEY(session_id) REFERENCES study_sessions(id) ON DELETE CASCADE"
    ");";

static const char *g_pszCreateSnippetsSessionCapturedAtIndex =
    "CREATE INDEX IF NOT EXISTS idx_snippets_session_id_captured_at "
    "ON snippets(session_id, captured_at);";

static const char *g_pszCreateSnippetsSessionArchivedIndex =
    "CREATE INDEX IF NOT EXISTS idx_snippets_session_id_is_archived "
    "ON snippets(session_id, is_archived);";

static const char *g_pszCreateAttachmentsSnippetIdIndex =
    "CREATE INDEX IF NOT EXISTS idx_attachments_snippet_id "
    "ON attachments(snippet_id);";

static const char *g_pszCreateAttachmentsSnippetIdUniqueIndex =
    "CREATE UNIQUE INDEX IF NOT EXISTS idx_attachments_snippet_id_unique "
    "ON attachments(snippet_id);";

static const char *g_pszCreateAiRecordsSessionIdIndex =
    "CREATE INDEX IF NOT EXISTS idx_ai_records_session_id "
    "ON ai_records(session_id);";

static const char *g_pszCreateAiRecordsSnippetIdIndex =
    "CREATE INDEX IF NOT EXISTS idx_ai_records_snippet_id "
    "ON ai_records(snippet_id);";

static const char *g_pszCreateAiRecordsTaskTypeIndex =
    "CREATE INDEX IF NOT EXISTS idx_ai_records_task_type "
    "ON ai_records(task_type);";

static const char *g_pszCreateExportsSessionIdIndex =
    "CREATE INDEX IF NOT EXISTS idx_exports_session_id "
    "ON exports(session_id);";
}

QString QCDatabaseSchema::schemaVersionKey()
{
    return QString::fromUtf8(g_pszSchemaVersionKey);
}

int QCDatabaseSchema::currentSchemaVersion()
{
    return g_nCurrentSchemaVersion;
}

QStringList QCDatabaseSchema::allStatements()
{
    QStringList listStatements;

    listStatements << QString::fromUtf8(g_pszCreateStudySessionsTable)
                   << QString::fromUtf8(g_pszCreateSnippetsTable)
                   << QString::fromUtf8(g_pszCreateAttachmentsTable)
                   << QString::fromUtf8(g_pszCreateTagsTable)
                   << QString::fromUtf8(g_pszCreateSnippetTagsTable)
                   << QString::fromUtf8(g_pszCreateAiRecordsTable)
                   << QString::fromUtf8(g_pszCreateAppSettingsTable)
                   << QString::fromUtf8(g_pszCreateExportsTable)
                   << QString::fromUtf8(g_pszCreateSnippetsSessionCapturedAtIndex)
                   << QString::fromUtf8(g_pszCreateSnippetsSessionArchivedIndex)
                   << QString::fromUtf8(g_pszCreateAttachmentsSnippetIdIndex)
                   << QString::fromUtf8(g_pszCreateAttachmentsSnippetIdUniqueIndex)
                   << QString::fromUtf8(g_pszCreateAiRecordsSessionIdIndex)
                   << QString::fromUtf8(g_pszCreateAiRecordsSnippetIdIndex)
                   << QString::fromUtf8(g_pszCreateAiRecordsTaskTypeIndex)
                   << QString::fromUtf8(g_pszCreateExportsSessionIdIndex);

    return listStatements;
}
