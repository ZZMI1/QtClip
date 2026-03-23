// File: qcsnippetrepositorysqlite.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the SQLite snippet repository used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsnippetrepositorysqlite.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include "../../core/database/qcdbenumcodec.h"

namespace
{
static const char *g_pszInsertSnippetSql =
    "INSERT INTO snippets("
    "session_id, type, capture_type, title, content_text, summary, note, note_kind, note_level, "
    "source, language, is_favorite, is_archived, captured_at, created_at, updated_at"
    ") VALUES("
    ":session_id, :type, :capture_type, :title, :content_text, :summary, :note, :note_kind, :note_level, "
    ":source, :language, :is_favorite, :is_archived, :captured_at, :created_at, :updated_at"
    ");";

static const char *g_pszInsertAttachmentSql =
    "INSERT INTO attachments(snippet_id, file_path, file_name, mime_type, created_at) "
    "VALUES(:snippet_id, :file_path, :file_name, :mime_type, :created_at);";

static const char *g_pszUpdateSnippetSql =
    "UPDATE snippets SET "
    "session_id = :session_id, "
    "type = :type, "
    "capture_type = :capture_type, "
    "title = :title, "
    "content_text = :content_text, "
    "summary = :summary, "
    "note = :note, "
    "note_kind = :note_kind, "
    "note_level = :note_level, "
    "source = :source, "
    "language = :language, "
    "is_favorite = :is_favorite, "
    "is_archived = :is_archived, "
    "captured_at = :captured_at, "
    "created_at = :created_at, "
    "updated_at = :updated_at "
    "WHERE id = :id;";

static const char *g_pszDeleteSnippetSql =
    "DELETE FROM snippets WHERE id = :id;";

static const char *g_pszSelectSnippetByIdSql =
    "SELECT id, session_id, type, capture_type, title, content_text, summary, note, note_kind, note_level, "
    "source, language, is_favorite, is_archived, captured_at, created_at, updated_at "
    "FROM snippets WHERE id = :id LIMIT 1;";

static const char *g_pszSelectPrimaryAttachmentBySnippetIdSql =
    "SELECT id, snippet_id, file_path, file_name, mime_type, created_at "
    "FROM attachments WHERE snippet_id = :snippet_id LIMIT 1;";

static const char *g_pszSelectSnippetsBySessionSql =
    "SELECT id, session_id, type, capture_type, title, content_text, summary, note, note_kind, note_level, "
    "source, language, is_favorite, is_archived, captured_at, created_at, updated_at "
    "FROM snippets WHERE session_id = :session_id ORDER BY captured_at DESC, id DESC;";

static const char *g_pszSelectSnippetColumnsSql =
    "SELECT DISTINCT s.id, s.session_id, s.type, s.capture_type, s.title, s.content_text, s.summary, s.note, s.note_kind, s.note_level, "
    "s.source, s.language, s.is_favorite, s.is_archived, s.captured_at, s.created_at, s.updated_at "
    "FROM snippets s ";

static const char *g_pszSearchSnippetsSql =
    "SELECT id, session_id, type, capture_type, title, content_text, summary, note, note_kind, note_level, "
    "source, language, is_favorite, is_archived, captured_at, created_at, updated_at "
    "FROM snippets "
    "WHERE title LIKE :pattern OR content_text LIKE :pattern OR summary LIKE :pattern OR note LIKE :pattern "
    "ORDER BY captured_at DESC, id DESC;";

static const char *g_pszSetFavoriteSql =
    "UPDATE snippets SET is_favorite = :is_favorite WHERE id = :id;";

static const char *g_pszSetArchivedSql =
    "UPDATE snippets SET is_archived = :is_archived WHERE id = :id;";
}

QCSnippetRepositorySqlite::QCSnippetRepositorySqlite(QCDatabaseManager *pDatabaseManager)
    : m_pDatabaseManager(pDatabaseManager)
    , m_strLastError()
    , m_strLastFailedSql()
{
}

QCSnippetRepositorySqlite::~QCSnippetRepositorySqlite()
{
}

bool QCSnippetRepositorySqlite::createSnippet(const QCSnippet& snippet, qint64 *pnSnippetId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (snippet.type() == QCSnippetType::ImageSnippetType)
    {
        setLastError(QString::fromUtf8("Image snippets must be created with a primary attachment."));
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszInsertSnippetSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);

    if (!bindSnippetValues(&query, snippet))
        return false;

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (nullptr != pnSnippetId)
        *pnSnippetId = query.lastInsertId().toLongLong();

    setLastFailedSql(QString());
    return true;
}

bool QCSnippetRepositorySqlite::createSnippetWithPrimaryAttachment(const QCSnippet& snippet,
                                                                   const QCAttachment& primaryAttachment,
                                                                   qint64 *pnSnippetId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (snippet.type() != QCSnippetType::ImageSnippetType)
    {
        setLastError(QString::fromUtf8("Primary attachment creation requires ImageSnippetType."));
        return false;
    }

    if (primaryAttachment.filePath().isEmpty())
    {
        setLastError(QString::fromUtf8("Primary attachment file path is empty."));
        return false;
    }

    if (!primaryAttachment.createdAt().isValid())
    {
        setLastError(QString::fromUtf8("Primary attachment createdAt is invalid."));
        return false;
    }

    if (!m_pDatabaseManager->beginTransaction())
    {
        setLastError(m_pDatabaseManager->lastError());
        setLastFailedSql(m_pDatabaseManager->lastFailedSql());
        return false;
    }

    qint64 nSnippetId = 0;
    QSqlQuery snippetQuery(m_pDatabaseManager->database());
    const QString strInsertSnippetSql = QString::fromUtf8(g_pszInsertSnippetSql);
    setLastFailedSql(strInsertSnippetSql);
    snippetQuery.prepare(strInsertSnippetSql);

    if (!bindSnippetValues(&snippetQuery, snippet))
        return rollbackTransactionPreserveError(m_strLastError, m_strLastFailedSql);

    if (!snippetQuery.exec())
    {
        setLastError(snippetQuery.lastError().text());
        return rollbackTransactionPreserveError(m_strLastError, m_strLastFailedSql);
    }

    nSnippetId = snippetQuery.lastInsertId().toLongLong();

    QSqlQuery attachmentQuery(m_pDatabaseManager->database());
    const QString strInsertAttachmentSql = QString::fromUtf8(g_pszInsertAttachmentSql);
    setLastFailedSql(strInsertAttachmentSql);
    attachmentQuery.prepare(strInsertAttachmentSql);
    attachmentQuery.bindValue(QString::fromUtf8(":snippet_id"), nSnippetId);
    attachmentQuery.bindValue(QString::fromUtf8(":file_path"), primaryAttachment.filePath());
    attachmentQuery.bindValue(QString::fromUtf8(":file_name"), primaryAttachment.fileName());
    attachmentQuery.bindValue(QString::fromUtf8(":mime_type"), primaryAttachment.mimeType());
    attachmentQuery.bindValue(QString::fromUtf8(":created_at"), primaryAttachment.createdAt().toString(Qt::ISODate));

    if (!attachmentQuery.exec())
    {
        setLastError(attachmentQuery.lastError().text());
        return rollbackTransactionPreserveError(m_strLastError, m_strLastFailedSql);
    }

    if (!m_pDatabaseManager->commitTransaction())
    {
        setLastError(m_pDatabaseManager->lastError());
        setLastFailedSql(m_pDatabaseManager->lastFailedSql());
        return rollbackTransactionPreserveError(m_strLastError, m_strLastFailedSql);
    }

    if (nullptr != pnSnippetId)
        *pnSnippetId = nSnippetId;

    setLastFailedSql(QString());
    return true;
}

bool QCSnippetRepositorySqlite::updateSnippet(const QCSnippet& snippet)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszUpdateSnippetSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);

    if (!bindSnippetValues(&query, snippet))
        return false;

    query.bindValue(QString::fromUtf8(":id"), snippet.id());

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No snippet was updated."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

bool QCSnippetRepositorySqlite::deleteSnippet(qint64 nSnippetId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszDeleteSnippetSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":id"), nSnippetId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No snippet was deleted."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

bool QCSnippetRepositorySqlite::getSnippetById(qint64 nSnippetId, QCSnippet *pSnippet) const
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (nullptr == pSnippet)
    {
        setLastError(QString::fromUtf8("QCSnippet output pointer is null."));
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectSnippetByIdSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":id"), nSnippetId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (!query.next())
    {
        setLastFailedSql(QString());
        return false;
    }

    if (!mapSnippet(&query, pSnippet))
        return false;

    setLastFailedSql(QString());
    return true;
}

bool QCSnippetRepositorySqlite::getPrimaryAttachmentBySnippetId(qint64 nSnippetId,
                                                                QCAttachment *pPrimaryAttachment) const
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (nullptr == pPrimaryAttachment)
    {
        setLastError(QString::fromUtf8("QCAttachment output pointer is null."));
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectPrimaryAttachmentBySnippetIdSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":snippet_id"), nSnippetId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (!query.next())
    {
        setLastFailedSql(QString());
        return false;
    }

    if (!mapAttachment(&query, pPrimaryAttachment))
        return false;

    setLastFailedSql(QString());
    return true;
}

QVector<QCSnippet> QCSnippetRepositorySqlite::listSnippetsBySession(qint64 nSessionId) const
{
    clearErrors();

    QVector<QCSnippet> vecSnippets;
    if (!ensureDatabaseReady())
        return vecSnippets;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectSnippetsBySessionSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":session_id"), nSessionId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return vecSnippets;
    }

    while (query.next())
    {
        QCSnippet snippet;
        if (!mapSnippet(&query, &snippet))
        {
            vecSnippets.clear();
            return vecSnippets;
        }

        vecSnippets.append(snippet);
    }

    setLastFailedSql(QString());
    return vecSnippets;
}

QVector<QCSnippet> QCSnippetRepositorySqlite::querySnippets(qint64 nSessionId,
                                                            const QString& strKeyword,
                                                            bool bFavoriteOnly,
                                                            bool bReviewOnly,
                                                            qint64 nTagId) const
{
    clearErrors();

    QVector<QCSnippet> vecSnippets;
    if (!ensureDatabaseReady())
        return vecSnippets;

    QString strSql = QString::fromUtf8(g_pszSelectSnippetColumnsSql);
    if (nTagId > 0)
        strSql += QString::fromUtf8("INNER JOIN snippet_tags st ON st.snippet_id = s.id ");

    strSql += QString::fromUtf8("WHERE s.session_id = :session_id ");

    const QString strTrimmedKeyword = strKeyword.trimmed();
    if (!strTrimmedKeyword.isEmpty())
        strSql += QString::fromUtf8("AND (s.title LIKE :pattern OR s.content_text LIKE :pattern OR s.summary LIKE :pattern OR s.note LIKE :pattern) ");

    if (bFavoriteOnly)
        strSql += QString::fromUtf8("AND s.is_favorite = 1 ");

    if (bReviewOnly)
        strSql += QString::fromUtf8("AND s.note_level = :review_note_level ");

    if (nTagId > 0)
        strSql += QString::fromUtf8("AND st.tag_id = :tag_id ");

    strSql += QString::fromUtf8("ORDER BY s.captured_at DESC, s.id DESC;");

    QSqlQuery query(m_pDatabaseManager->database());
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":session_id"), nSessionId);

    if (!strTrimmedKeyword.isEmpty())
        query.bindValue(QString::fromUtf8(":pattern"), QString::fromUtf8("%%1%").arg(strTrimmedKeyword));

    if (bReviewOnly)
    {
        QString strReviewNoteLevelValue;
        QString strReviewNoteLevelError;
        if (!QCDbEnumCodec::toDbValue(QCNoteLevel::ReviewNoteLevel, &strReviewNoteLevelValue, &strReviewNoteLevelError))
        {
            setLastError(strReviewNoteLevelError);
            return vecSnippets;
        }

        query.bindValue(QString::fromUtf8(":review_note_level"), strReviewNoteLevelValue);
    }

    if (nTagId > 0)
        query.bindValue(QString::fromUtf8(":tag_id"), nTagId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return vecSnippets;
    }

    while (query.next())
    {
        QCSnippet snippet;
        if (!mapSnippet(&query, &snippet))
        {
            vecSnippets.clear();
            return vecSnippets;
        }

        vecSnippets.append(snippet);
    }

    setLastFailedSql(QString());
    return vecSnippets;
}

QVector<QCSnippet> QCSnippetRepositorySqlite::searchSnippets(const QString& strKeyword) const
{
    clearErrors();

    QVector<QCSnippet> vecSnippets;
    if (!ensureDatabaseReady())
        return vecSnippets;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSearchSnippetsSql);
    const QString strPattern = QString::fromUtf8("%%1%").arg(strKeyword);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":pattern"), strPattern);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return vecSnippets;
    }

    while (query.next())
    {
        QCSnippet snippet;
        if (!mapSnippet(&query, &snippet))
        {
            vecSnippets.clear();
            return vecSnippets;
        }

        vecSnippets.append(snippet);
    }

    setLastFailedSql(QString());
    return vecSnippets;
}

bool QCSnippetRepositorySqlite::setFavorite(qint64 nSnippetId, bool bIsFavorite)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSetFavoriteSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":is_favorite"), bIsFavorite ? 1 : 0);
    query.bindValue(QString::fromUtf8(":id"), nSnippetId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No snippet favorite state was updated."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

bool QCSnippetRepositorySqlite::setArchived(qint64 nSnippetId, bool bIsArchived)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSetArchivedSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":is_archived"), bIsArchived ? 1 : 0);
    query.bindValue(QString::fromUtf8(":id"), nSnippetId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No snippet archived state was updated."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

QString QCSnippetRepositorySqlite::lastError() const
{
    return m_strLastError;
}

QString QCSnippetRepositorySqlite::lastFailedSql() const
{
    return m_strLastFailedSql;
}

bool QCSnippetRepositorySqlite::ensureDatabaseReady() const
{
    if (nullptr == m_pDatabaseManager)
    {
        setLastError(QString::fromUtf8("Database manager is null."));
        return false;
    }

    if (!m_pDatabaseManager->isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    return true;
}

bool QCSnippetRepositorySqlite::bindSnippetValues(QSqlQuery *pQuery, const QCSnippet& snippet) const
{
    if (nullptr == pQuery)
    {
        setLastError(QString::fromUtf8("Snippet query pointer is null."));
        return false;
    }

    QString strTypeValue;
    QString strTypeError;
    if (!QCDbEnumCodec::toDbValue(snippet.type(), &strTypeValue, &strTypeError))
    {
        setLastError(strTypeError);
        return false;
    }

    QString strCaptureTypeValue;
    QString strCaptureTypeError;
    if (!QCDbEnumCodec::toDbValue(snippet.captureType(), &strCaptureTypeValue, &strCaptureTypeError))
    {
        setLastError(strCaptureTypeError);
        return false;
    }

    QString strNoteKindValue;
    QString strNoteKindError;
    if (!QCDbEnumCodec::toDbValue(snippet.noteKind(), &strNoteKindValue, &strNoteKindError))
    {
        setLastError(strNoteKindError);
        return false;
    }

    QString strNoteLevelValue;
    QString strNoteLevelError;
    if (!QCDbEnumCodec::toDbValue(snippet.noteLevel(), &strNoteLevelValue, &strNoteLevelError))
    {
        setLastError(strNoteLevelError);
        return false;
    }

    if (!snippet.capturedAt().isValid() || !snippet.createdAt().isValid() || !snippet.updatedAt().isValid())
    {
        setLastError(QString::fromUtf8("Snippet required date time values are invalid."));
        return false;
    }

    pQuery->bindValue(QString::fromUtf8(":session_id"), snippet.sessionId());
    pQuery->bindValue(QString::fromUtf8(":type"), strTypeValue);
    pQuery->bindValue(QString::fromUtf8(":capture_type"), strCaptureTypeValue);
    pQuery->bindValue(QString::fromUtf8(":title"), snippet.title());
    pQuery->bindValue(QString::fromUtf8(":content_text"), snippet.contentText());
    pQuery->bindValue(QString::fromUtf8(":summary"), snippet.summary());
    pQuery->bindValue(QString::fromUtf8(":note"), snippet.note());
    pQuery->bindValue(QString::fromUtf8(":note_kind"), strNoteKindValue);
    pQuery->bindValue(QString::fromUtf8(":note_level"), strNoteLevelValue);
    pQuery->bindValue(QString::fromUtf8(":source"), snippet.source());
    pQuery->bindValue(QString::fromUtf8(":language"), snippet.language());
    pQuery->bindValue(QString::fromUtf8(":is_favorite"), snippet.isFavorite() ? 1 : 0);
    pQuery->bindValue(QString::fromUtf8(":is_archived"), snippet.isArchived() ? 1 : 0);
    pQuery->bindValue(QString::fromUtf8(":captured_at"), snippet.capturedAt().toString(Qt::ISODate));
    pQuery->bindValue(QString::fromUtf8(":created_at"), snippet.createdAt().toString(Qt::ISODate));
    pQuery->bindValue(QString::fromUtf8(":updated_at"), snippet.updatedAt().toString(Qt::ISODate));

    return true;
}

bool QCSnippetRepositorySqlite::mapSnippet(QSqlQuery *pQuery, QCSnippet *pSnippet) const
{
    if (nullptr == pQuery || nullptr == pSnippet)
    {
        setLastError(QString::fromUtf8("Snippet mapping input is invalid."));
        return false;
    }

    const QSqlRecord record = pQuery->record();
    const int nIdIndex = record.indexOf(QString::fromUtf8("id"));
    const int nSessionIdIndex = record.indexOf(QString::fromUtf8("session_id"));
    const int nTypeIndex = record.indexOf(QString::fromUtf8("type"));
    const int nCaptureTypeIndex = record.indexOf(QString::fromUtf8("capture_type"));
    const int nTitleIndex = record.indexOf(QString::fromUtf8("title"));
    const int nContentTextIndex = record.indexOf(QString::fromUtf8("content_text"));
    const int nSummaryIndex = record.indexOf(QString::fromUtf8("summary"));
    const int nNoteIndex = record.indexOf(QString::fromUtf8("note"));
    const int nNoteKindIndex = record.indexOf(QString::fromUtf8("note_kind"));
    const int nNoteLevelIndex = record.indexOf(QString::fromUtf8("note_level"));
    const int nSourceIndex = record.indexOf(QString::fromUtf8("source"));
    const int nLanguageIndex = record.indexOf(QString::fromUtf8("language"));
    const int nIsFavoriteIndex = record.indexOf(QString::fromUtf8("is_favorite"));
    const int nIsArchivedIndex = record.indexOf(QString::fromUtf8("is_archived"));
    const int nCapturedAtIndex = record.indexOf(QString::fromUtf8("captured_at"));
    const int nCreatedAtIndex = record.indexOf(QString::fromUtf8("created_at"));
    const int nUpdatedAtIndex = record.indexOf(QString::fromUtf8("updated_at"));

    if (nIdIndex < 0 || nSessionIdIndex < 0 || nTypeIndex < 0 || nCaptureTypeIndex < 0
        || nTitleIndex < 0 || nContentTextIndex < 0 || nSummaryIndex < 0 || nNoteIndex < 0
        || nNoteKindIndex < 0 || nNoteLevelIndex < 0 || nSourceIndex < 0 || nLanguageIndex < 0
        || nIsFavoriteIndex < 0 || nIsArchivedIndex < 0 || nCapturedAtIndex < 0
        || nCreatedAtIndex < 0 || nUpdatedAtIndex < 0)
    {
        setLastError(QString::fromUtf8("Snippet query result is missing required fields."));
        return false;
    }

    QCSnippetType snippetType = QCSnippetType::ImageSnippetType;
    QString strSnippetTypeError;
    if (!QCDbEnumCodec::fromDbValue(pQuery->value(nTypeIndex).toString(), &snippetType, &strSnippetTypeError))
    {
        setLastError(strSnippetTypeError);
        return false;
    }

    QCCaptureType captureType = QCCaptureType::ScreenCaptureType;
    QString strCaptureTypeError;
    if (!QCDbEnumCodec::fromDbValue(pQuery->value(nCaptureTypeIndex).toString(), &captureType, &strCaptureTypeError))
    {
        setLastError(strCaptureTypeError);
        return false;
    }

    QCNoteKind noteKind = QCNoteKind::ConceptNoteKind;
    QString strNoteKindError;
    if (!QCDbEnumCodec::fromDbValue(pQuery->value(nNoteKindIndex).toString(), &noteKind, &strNoteKindError))
    {
        setLastError(strNoteKindError);
        return false;
    }

    QCNoteLevel noteLevel = QCNoteLevel::NormalNoteLevel;
    QString strNoteLevelError;
    if (!QCDbEnumCodec::fromDbValue(pQuery->value(nNoteLevelIndex).toString(), &noteLevel, &strNoteLevelError))
    {
        setLastError(strNoteLevelError);
        return false;
    }

    QDateTime dateTimeCapturedAt;
    if (!parseRequiredDateTime(QString::fromUtf8("captured_at"), pQuery->value(nCapturedAtIndex).toString(), &dateTimeCapturedAt))
        return false;

    QDateTime dateTimeCreatedAt;
    if (!parseRequiredDateTime(QString::fromUtf8("created_at"), pQuery->value(nCreatedAtIndex).toString(), &dateTimeCreatedAt))
        return false;

    QDateTime dateTimeUpdatedAt;
    if (!parseRequiredDateTime(QString::fromUtf8("updated_at"), pQuery->value(nUpdatedAtIndex).toString(), &dateTimeUpdatedAt))
        return false;

    // Map the current query row into a pure snippet domain object.
    pSnippet->setId(pQuery->value(nIdIndex).toLongLong());
    pSnippet->setSessionId(pQuery->value(nSessionIdIndex).toLongLong());
    pSnippet->setType(snippetType);
    pSnippet->setCaptureType(captureType);
    pSnippet->setTitle(pQuery->value(nTitleIndex).toString());
    pSnippet->setContentText(pQuery->value(nContentTextIndex).toString());
    pSnippet->setSummary(pQuery->value(nSummaryIndex).toString());
    pSnippet->setNote(pQuery->value(nNoteIndex).toString());
    pSnippet->setNoteKind(noteKind);
    pSnippet->setNoteLevel(noteLevel);
    pSnippet->setSource(pQuery->value(nSourceIndex).toString());
    pSnippet->setLanguage(pQuery->value(nLanguageIndex).toString());
    pSnippet->setIsFavorite(0 != pQuery->value(nIsFavoriteIndex).toInt());
    pSnippet->setIsArchived(0 != pQuery->value(nIsArchivedIndex).toInt());
    pSnippet->setCapturedAt(dateTimeCapturedAt);
    pSnippet->setCreatedAt(dateTimeCreatedAt);
    pSnippet->setUpdatedAt(dateTimeUpdatedAt);

    return true;
}

bool QCSnippetRepositorySqlite::mapAttachment(QSqlQuery *pQuery, QCAttachment *pAttachment) const
{
    if (nullptr == pQuery || nullptr == pAttachment)
    {
        setLastError(QString::fromUtf8("Attachment mapping input is invalid."));
        return false;
    }

    const QSqlRecord record = pQuery->record();
    const int nIdIndex = record.indexOf(QString::fromUtf8("id"));
    const int nSnippetIdIndex = record.indexOf(QString::fromUtf8("snippet_id"));
    const int nFilePathIndex = record.indexOf(QString::fromUtf8("file_path"));
    const int nFileNameIndex = record.indexOf(QString::fromUtf8("file_name"));
    const int nMimeTypeIndex = record.indexOf(QString::fromUtf8("mime_type"));
    const int nCreatedAtIndex = record.indexOf(QString::fromUtf8("created_at"));

    if (nIdIndex < 0 || nSnippetIdIndex < 0 || nFilePathIndex < 0 || nFileNameIndex < 0
        || nMimeTypeIndex < 0 || nCreatedAtIndex < 0)
    {
        setLastError(QString::fromUtf8("Attachment query result is missing required fields."));
        return false;
    }

    QDateTime dateTimeCreatedAt;
    if (!parseRequiredDateTime(QString::fromUtf8("created_at"), pQuery->value(nCreatedAtIndex).toString(), &dateTimeCreatedAt))
        return false;

    pAttachment->setId(pQuery->value(nIdIndex).toLongLong());
    pAttachment->setSnippetId(pQuery->value(nSnippetIdIndex).toLongLong());
    pAttachment->setFilePath(pQuery->value(nFilePathIndex).toString());
    pAttachment->setFileName(pQuery->value(nFileNameIndex).toString());
    pAttachment->setMimeType(pQuery->value(nMimeTypeIndex).toString());
    pAttachment->setCreatedAt(dateTimeCreatedAt);

    return true;
}

bool QCSnippetRepositorySqlite::parseRequiredDateTime(const QString& strFieldName,
                                                      const QString& strValue,
                                                      QDateTime *pDateTime) const
{
    if (nullptr == pDateTime)
    {
        setLastError(QString::fromUtf8("Required date time output pointer is null."));
        return false;
    }

    const QDateTime dateTime = QDateTime::fromString(strValue, Qt::ISODate);
    if (!dateTime.isValid())
    {
        setLastError(QString::fromUtf8("Invalid ISO date time value in field %1: %2")
            .arg(strFieldName, strValue));
        return false;
    }

    *pDateTime = dateTime;
    return true;
}

bool QCSnippetRepositorySqlite::parseOptionalDateTime(const QString& strFieldName,
                                                      const QString& strValue,
                                                      QDateTime *pDateTime) const
{
    if (nullptr == pDateTime)
    {
        setLastError(QString::fromUtf8("Optional date time output pointer is null."));
        return false;
    }

    if (strValue.isEmpty())
    {
        *pDateTime = QDateTime();
        return true;
    }

    return parseRequiredDateTime(strFieldName, strValue, pDateTime);
}

bool QCSnippetRepositorySqlite::rollbackTransactionPreserveError(const QString& strOriginalError,
                                                                 const QString& strOriginalSql)
{
    if (!m_pDatabaseManager->rollbackTransaction())
    {
        setLastError(m_pDatabaseManager->lastError());
        setLastFailedSql(m_pDatabaseManager->lastFailedSql());
        return false;
    }

    setLastError(strOriginalError);
    setLastFailedSql(strOriginalSql);
    return false;
}

void QCSnippetRepositorySqlite::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}

void QCSnippetRepositorySqlite::setLastFailedSql(const QString& strSql) const
{
    m_strLastFailedSql = strSql;
}

void QCSnippetRepositorySqlite::clearErrors() const
{
    m_strLastError.clear();
    m_strLastFailedSql.clear();
}

