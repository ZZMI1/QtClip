#ifndef QTCLIP_QCAISERVICE_H_
#define QTCLIP_QCAISERVICE_H_

// File: qcaiservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the thin AI record service used to drive the first QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QVector>

#include "../domain/entities/qcairecord.h"
#include "../domain/interfaces/iqcairecordrepository.h"

class QCAiService
{
public:
    explicit QCAiService(IQCAiRecordRepository *pAiRecordRepository);
    ~QCAiService();

    bool createAiRecord(QCAiRecord *pAiRecord);
    bool updateAiRecord(const QCAiRecord& aiRecord);
    bool getAiRecordById(qint64 nAiRecordId, QCAiRecord *pAiRecord) const;
    QVector<QCAiRecord> listAiRecordsBySession(qint64 nSessionId) const;
    QVector<QCAiRecord> listAiRecordsBySnippet(qint64 nSnippetId) const;

    QString lastError() const;
    void clearError() const;

private:
    QCAiService(const QCAiService& other);
    QCAiService& operator=(const QCAiService& other);

    bool validateAiRecord(const QCAiRecord& aiRecord) const;
    void setLastError(const QString& strError) const;

private:
    IQCAiRecordRepository *m_pAiRecordRepository;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCAISERVICE_H_
