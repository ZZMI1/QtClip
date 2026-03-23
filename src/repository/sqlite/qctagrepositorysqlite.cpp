// File: qctagrepositorysqlite.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the SQLite tag repository used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qctagrepositorysqlite.h"

#include <QDateTime>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

namespace
{
static const char *g_pszInsertTagSql =
    "INSERT INTO tags(name, color, created_at) VALUES(:name, :color, :created_at);";

static const char *g_pszUpdateTagSql =
    "UPDATE tags SET name = :name, color = :color WHERE id = :id;";

static const char *g_pszDeleteTagSql =
    "DELETE FROM tags WHERE id = :id;";

static const char *g_pszSelectTagByIdSql =
    "SELECT id, name, color, created_at FROM tags WHERE id = :id LIMIT 1;";

static const char *g_pszSelectTagByNameSql =
    "SELECT id, name, color, created_at FROM tags WHERE name = :name LIMIT 1;";

static const char *g_pszSelectTagsSql =
    "SELECT id, name, color, created_at FROM tags ORDER BY name COLLATE NOCASE ASC, id ASC;";

static const char *g_pszSelectTagsBySnippetSql =
    "SELECT t.id, t.name, t.color, t.created_at "
    "FROM tags t "
    "INNER JOIN snippet_tags st ON st.tag_id = t.id "
    "WHERE st.snippet_id = :snippet_id "
    "ORDER BY t.name COLLATE NOCASE ASC, t.id ASC;";

static const char *g_pszDeleteSnippetTagsSql =
    "DELETE FROM snippet_tags WHERE snippet_id = :snippet_id;";

static const char *g_pszInsertSnippetTagSql =
    "INSERT INTO snippet_tags(snippet_id, tag_id) VALUES(:snippet_id, :tag_id);";
}

QCTagRepositorySqlite::QCTagRepositorySqlite(QCDatabaseManager *pDatabaseManager)
    : m_pDatabaseManager(pDatabaseManager)
    , m_strLastError()
    , m_strLastFailedSql()
{
}

QCTagRepositorySqlite::~QCTagRepositorySqlite()
{
}

bool QCTagRepositorySqlite::createTag(const QCTag& tag, qint64 *pnTagId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszInsertTagSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":name"), tag.name());
    query.bindValue(QString::fromUtf8(":color"), tag.color());
    query.bindValue(QString::fromUtf8(":created_at"), tag.createdAt().toString(Qt::ISODate));

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (nullptr != pnTagId)
        *pnTagId = query.lastInsertId().toLongLong();

    setLastFailedSql(QString());
    return true;
}

bool QCTagRepositorySqlite::updateTag(const QCTag& tag)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszUpdateTagSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":name"), tag.name());
    query.bindValue(QString::fromUtf8(":color"), tag.color());
    query.bindValue(QString::fromUtf8(":id"), tag.id());

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No tag was updated."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

bool QCTagRepositorySqlite::deleteTag(qint64 nTagId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszDeleteTagSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":id"), nTagId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No tag was deleted."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

bool QCTagRepositorySqlite::getTagById(qint64 nTagId, QCTag *pTag) const
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (nullptr == pTag)
    {
        setLastError(QString::fromUtf8("QCTag output pointer is null."));
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectTagByIdSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":id"), nTagId);

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

    if (!mapTag(&query, pTag))
        return false;

    setLastFailedSql(QString());
    return true;
}

bool QCTagRepositorySqlite::getTagByName(const QString& strName, QCTag *pTag) const
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (nullptr == pTag)
    {
        setLastError(QString::fromUtf8("QCTag output pointer is null."));
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectTagByNameSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":name"), strName);

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

    if (!mapTag(&query, pTag))
        return false;

    setLastFailedSql(QString());
    return true;
}

QVector<QCTag> QCTagRepositorySqlite::listTags() const
{
    clearErrors();

    QVector<QCTag> vecTags;
    if (!ensureDatabaseReady())
        return vecTags;

    const QString strSql = QString::fromUtf8(g_pszSelectTagsSql);
    setLastFailedSql(strSql);
    QSqlQuery query(m_pDatabaseManager->database());
    if (!query.exec(strSql))
    {
        setLastError(query.lastError().text());
        return vecTags;
    }

    while (query.next())
    {
        QCTag tag;
        if (!mapTag(&query, &tag))
        {
            vecTags.clear();
            return vecTags;
        }

        vecTags.append(tag);
    }

    setLastFailedSql(QString());
    return vecTags;
}

QVector<QCTag> QCTagRepositorySqlite::listTagsBySnippet(qint64 nSnippetId) const
{
    clearErrors();

    QVector<QCTag> vecTags;
    if (!ensureDatabaseReady())
        return vecTags;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectTagsBySnippetSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":snippet_id"), nSnippetId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return vecTags;
    }

    while (query.next())
    {
        QCTag tag;
        if (!mapTag(&query, &tag))
        {
            vecTags.clear();
            return vecTags;
        }

        vecTags.append(tag);
    }

    setLastFailedSql(QString());
    return vecTags;
}

bool QCTagRepositorySqlite::replaceSnippetTags(qint64 nSnippetId, const QVector<qint64>& vecTagIds)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSet<qint64> setUniqueIds;
    for (int i = 0; i < vecTagIds.size(); ++i)
    {
        if (vecTagIds.at(i) > 0)
            setUniqueIds.insert(vecTagIds.at(i));
    }

    bool bSuccess = m_pDatabaseManager->executeInTransaction([this, nSnippetId, setUniqueIds]() -> bool {
        QSqlQuery deleteQuery(m_pDatabaseManager->database());
        const QString strDeleteSql = QString::fromUtf8(g_pszDeleteSnippetTagsSql);
        setLastFailedSql(strDeleteSql);
        deleteQuery.prepare(strDeleteSql);
        deleteQuery.bindValue(QString::fromUtf8(":snippet_id"), nSnippetId);
        if (!deleteQuery.exec())
        {
            setLastError(deleteQuery.lastError().text());
            return false;
        }

        const QList<qint64> vecUniqueIds = setUniqueIds.values();
        for (int i = 0; i < vecUniqueIds.size(); ++i)
        {
            QSqlQuery insertQuery(m_pDatabaseManager->database());
            const QString strInsertSql = QString::fromUtf8(g_pszInsertSnippetTagSql);
            setLastFailedSql(strInsertSql);
            insertQuery.prepare(strInsertSql);
            insertQuery.bindValue(QString::fromUtf8(":snippet_id"), nSnippetId);
            insertQuery.bindValue(QString::fromUtf8(":tag_id"), vecUniqueIds.at(i));
            if (!insertQuery.exec())
            {
                setLastError(insertQuery.lastError().text());
                return false;
            }
        }

        return true;
    });

    if (!bSuccess)
    {
        if (m_strLastError.isEmpty())
            setLastError(m_pDatabaseManager->lastError());
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

QString QCTagRepositorySqlite::lastError() const
{
    return m_strLastError;
}

QString QCTagRepositorySqlite::lastFailedSql() const
{
    return m_strLastFailedSql;
}

bool QCTagRepositorySqlite::ensureDatabaseReady() const
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

bool QCTagRepositorySqlite::mapTag(QSqlQuery *pQuery, QCTag *pTag) const
{
    if (nullptr == pQuery || nullptr == pTag)
    {
        setLastError(QString::fromUtf8("Tag mapping input is invalid."));
        return false;
    }

    const QSqlRecord record = pQuery->record();
    const int nIdIndex = record.indexOf(QString::fromUtf8("id"));
    const int nNameIndex = record.indexOf(QString::fromUtf8("name"));
    const int nColorIndex = record.indexOf(QString::fromUtf8("color"));
    const int nCreatedAtIndex = record.indexOf(QString::fromUtf8("created_at"));
    if (nIdIndex < 0 || nNameIndex < 0 || nColorIndex < 0 || nCreatedAtIndex < 0)
    {
        setLastError(QString::fromUtf8("Tag query result is missing required fields."));
        return false;
    }

    QDateTime dateTimeCreatedAt;
    if (!parseRequiredDateTime(pQuery->value(nCreatedAtIndex).toString(), &dateTimeCreatedAt))
        return false;

    pTag->setId(pQuery->value(nIdIndex).toLongLong());
    pTag->setName(pQuery->value(nNameIndex).toString());
    pTag->setColor(pQuery->value(nColorIndex).toString());
    pTag->setCreatedAt(dateTimeCreatedAt);
    return true;
}

bool QCTagRepositorySqlite::parseRequiredDateTime(const QString& strValue, QDateTime *pDateTime) const
{
    if (nullptr == pDateTime)
    {
        setLastError(QString::fromUtf8("Required date time output pointer is null."));
        return false;
    }

    const QDateTime dateTime = QDateTime::fromString(strValue, Qt::ISODate);
    if (!dateTime.isValid())
    {
        setLastError(QString::fromUtf8("Invalid ISO date time value for tag: %1").arg(strValue));
        return false;
    }

    *pDateTime = dateTime;
    return true;
}

void QCTagRepositorySqlite::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}

void QCTagRepositorySqlite::setLastFailedSql(const QString& strSql) const
{
    m_strLastFailedSql = strSql;
}

void QCTagRepositorySqlite::clearErrors() const
{
    m_strLastError.clear();
    m_strLastFailedSql.clear();
}
