#ifndef QTCLIP_IQCEXPORTREPOSITORY_H_
#define QTCLIP_IQCEXPORTREPOSITORY_H_

// File: iqcexportrepository.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the export repository abstraction used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QVector>

#include "../entities/qcexportrecord.h"

class IQCExportRepository
{
public:
    virtual ~IQCExportRepository() = default;

    virtual bool createExportRecord(const QCExportRecord& exportRecord, qint64 *pnExportRecordId) = 0;
    virtual bool getExportRecordById(qint64 nExportRecordId, QCExportRecord *pExportRecord) const = 0;
    virtual QVector<QCExportRecord> listExportRecordsBySession(qint64 nSessionId) const = 0;
    virtual bool deleteExportRecord(qint64 nExportRecordId) = 0;

protected:
    IQCExportRepository() = default;

private:
    IQCExportRepository(const IQCExportRepository& other);
    IQCExportRepository& operator=(const IQCExportRepository& other);
};

#endif // QTCLIP_IQCEXPORTREPOSITORY_H_
