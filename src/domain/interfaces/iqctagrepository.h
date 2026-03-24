#ifndef QTCLIP_IQCTAGREPOSITORY_H_
#define QTCLIP_IQCTAGREPOSITORY_H_

// File: iqctagrepository.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the tag repository abstraction used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QVector>

#include "../entities/qctag.h"

class IQCTagRepository
{
public:
    virtual ~IQCTagRepository() = default;

    virtual bool createTag(const QCTag& tag, qint64 *pnTagId) = 0;
    virtual bool updateTag(const QCTag& tag) = 0;
    virtual bool deleteTag(qint64 nTagId) = 0;
    virtual bool getTagById(qint64 nTagId, QCTag *pTag) const = 0;
    virtual bool getTagByName(const QString& strName, QCTag *pTag) const = 0;
    virtual QVector<QCTag> listTags() const = 0;
    virtual QVector<QCTag> listTagsBySnippet(qint64 nSnippetId) const = 0;
    virtual int countSnippetsByTag(qint64 nTagId) const = 0;
    virtual bool replaceSnippetTags(qint64 nSnippetId, const QVector<qint64>& vecTagIds) = 0;

protected:
    IQCTagRepository() = default;

private:
    IQCTagRepository(const IQCTagRepository& other);
    IQCTagRepository& operator=(const IQCTagRepository& other);
};

#endif // QTCLIP_IQCTAGREPOSITORY_H_
