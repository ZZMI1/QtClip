#ifndef QTCLIP_IQCSESSIONREPOSITORY_H_
#define QTCLIP_IQCSESSIONREPOSITORY_H_

// File: iqcsessionrepository.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the session repository abstraction used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QVector>

#include "../entities/qcstudysession.h"

class IQCSessionRepository
{
public:
    virtual ~IQCSessionRepository() = default;

    virtual bool createSession(const QCStudySession& session, qint64 *pnSessionId) = 0;
    virtual bool updateSession(const QCStudySession& session) = 0;
    virtual bool finishSession(qint64 nSessionId, const QDateTime& dateTimeEndedAt) = 0;
    virtual bool getSessionById(qint64 nSessionId, QCStudySession *pSession) const = 0;
    virtual bool getActiveSession(QCStudySession *pSession) const = 0;
    virtual QVector<QCStudySession> listSessions() const = 0;
    virtual bool deleteSession(qint64 nSessionId) = 0;

protected:
    IQCSessionRepository() = default;

private:
    IQCSessionRepository(const IQCSessionRepository& other);
    IQCSessionRepository& operator=(const IQCSessionRepository& other);
};

#endif // QTCLIP_IQCSESSIONREPOSITORY_H_
