#ifndef QTCLIP_QCSESSIONSERVICE_H_
#define QTCLIP_QCSESSIONSERVICE_H_

// File: qcsessionservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the thin study session service used to drive the first QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QVector>

#include "../domain/entities/qcstudysession.h"
#include "../domain/interfaces/iqcsessionrepository.h"

class QCSessionService
{
public:
    explicit QCSessionService(IQCSessionRepository *pSessionRepository);
    ~QCSessionService();

    bool createSession(QCStudySession *pSession);
    bool updateSession(QCStudySession *pSession);
    bool finishSession(qint64 nSessionId, const QDateTime& dateTimeEndedAt);
    bool getSessionById(qint64 nSessionId, QCStudySession *pSession) const;
    bool getActiveSession(QCStudySession *pSession) const;
    QVector<QCStudySession> listSessions() const;
    bool deleteSession(qint64 nSessionId);

    QString lastError() const;
    void clearError() const;

private:
    QCSessionService(const QCSessionService& other);
    QCSessionService& operator=(const QCSessionService& other);

    void setLastError(const QString& strError) const;

private:
    IQCSessionRepository *m_pSessionRepository;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCSESSIONSERVICE_H_
