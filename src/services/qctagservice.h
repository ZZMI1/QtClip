#ifndef QTCLIP_QCTAGSERVICE_H_
#define QTCLIP_QCTAGSERVICE_H_

// File: qctagservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the thin tag service used by the QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QVector>

#include "../domain/entities/qctag.h"
#include "../domain/interfaces/iqctagrepository.h"

class QCTagService
{
public:
    explicit QCTagService(IQCTagRepository *pTagRepository);
    ~QCTagService();

    bool createTag(QCTag *pTag);
    bool updateTag(QCTag *pTag);
    bool deleteTag(qint64 nTagId);
    bool getTagById(qint64 nTagId, QCTag *pTag) const;
    bool getTagByName(const QString& strName, QCTag *pTag) const;
    QVector<QCTag> listTags() const;
    QVector<QCTag> listTagsBySnippet(qint64 nSnippetId) const;
    int countSnippetsByTag(qint64 nTagId) const;
    bool replaceSnippetTags(qint64 nSnippetId, const QVector<qint64>& vecTagIds);

    QString lastError() const;
    void clearError() const;

private:
    QCTagService(const QCTagService& other);
    QCTagService& operator=(const QCTagService& other);

    qint64 findExistingTagIdByName(const QString& strName, qint64 nIgnoredTagId) const;
    void setLastError(const QString& strError) const;

private:
    IQCTagRepository *m_pTagRepository;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCTAGSERVICE_H_
