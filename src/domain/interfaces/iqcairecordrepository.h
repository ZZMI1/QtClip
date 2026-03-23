#ifndef QTCLIP_IQCAIRECORDREPOSITORY_H_
#define QTCLIP_IQCAIRECORDREPOSITORY_H_

// File: iqcairecordrepository.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the AI record repository abstraction used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QVector>

#include "../entities/qcairecord.h"

class IQCAiRecordRepository
{
public:
    virtual ~IQCAiRecordRepository() = default;

    virtual bool createAiRecord(const QCAiRecord& aiRecord, qint64 *pnAiRecordId) = 0;
    virtual bool updateAiRecord(const QCAiRecord& aiRecord) = 0;
    virtual bool getAiRecordById(qint64 nAiRecordId, QCAiRecord *pAiRecord) const = 0;
    virtual QVector<QCAiRecord> listAiRecordsBySession(qint64 nSessionId) const = 0;
    virtual QVector<QCAiRecord> listAiRecordsBySnippet(qint64 nSnippetId) const = 0;
    virtual bool deleteAiRecord(qint64 nAiRecordId) = 0;

protected:
    IQCAiRecordRepository() = default;

private:
    IQCAiRecordRepository(const IQCAiRecordRepository& other);
    IQCAiRecordRepository& operator=(const IQCAiRecordRepository& other);
};

#endif // QTCLIP_IQCAIRECORDREPOSITORY_H_
