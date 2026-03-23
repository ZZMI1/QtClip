#ifndef QTCLIP_QCSTUDYSESSION_H_
#define QTCLIP_QCSTUDYSESSION_H_

// File: qcstudysession.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the study session entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QString>

// Session lifecycle states used by the first QtClip release.
enum class QCSessionStatus
{
    ActiveSessionStatus,
    FinishedSessionStatus
};

class QCStudySession
{
public:
    QCStudySession();
    ~QCStudySession();

    qint64 id() const;
    void setId(qint64 nId);

    QString title() const;
    void setTitle(const QString& strTitle);

    QString courseName() const;
    void setCourseName(const QString& strCourseName);

    QString description() const;
    void setDescription(const QString& strDescription);

    QCSessionStatus status() const;
    void setStatus(QCSessionStatus sessionStatus);

    QDateTime startedAt() const;
    void setStartedAt(const QDateTime& dateTimeStartedAt);

    QDateTime endedAt() const;
    void setEndedAt(const QDateTime& dateTimeEndedAt);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& dateTimeCreatedAt);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& dateTimeUpdatedAt);

private:
    qint64 m_nId;
    QString m_strTitle;
    QString m_strCourseName;
    QString m_strDescription;
    QCSessionStatus m_sessionStatus;
    QDateTime m_dateTimeStartedAt;
    QDateTime m_dateTimeEndedAt;
    QDateTime m_dateTimeCreatedAt;
    QDateTime m_dateTimeUpdatedAt;
};

#endif // QTCLIP_QCSTUDYSESSION_H_
