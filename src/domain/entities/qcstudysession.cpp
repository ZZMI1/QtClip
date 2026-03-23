// File: qcstudysession.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the study session entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcstudysession.h"

QCStudySession::QCStudySession()
    : m_nId(0)
    , m_strTitle()
    , m_strCourseName()
    , m_strDescription()
    , m_sessionStatus(QCSessionStatus::ActiveSessionStatus)
    , m_dateTimeStartedAt()
    , m_dateTimeEndedAt()
    , m_dateTimeCreatedAt()
    , m_dateTimeUpdatedAt()
{
}

QCStudySession::~QCStudySession()
{
}

qint64 QCStudySession::id() const
{
    return m_nId;
}

void QCStudySession::setId(qint64 nId)
{
    m_nId = nId;
}

QString QCStudySession::title() const
{
    return m_strTitle;
}

void QCStudySession::setTitle(const QString& strTitle)
{
    m_strTitle = strTitle;
}

QString QCStudySession::courseName() const
{
    return m_strCourseName;
}

void QCStudySession::setCourseName(const QString& strCourseName)
{
    m_strCourseName = strCourseName;
}

QString QCStudySession::description() const
{
    return m_strDescription;
}

void QCStudySession::setDescription(const QString& strDescription)
{
    m_strDescription = strDescription;
}

QCSessionStatus QCStudySession::status() const
{
    return m_sessionStatus;
}

void QCStudySession::setStatus(QCSessionStatus sessionStatus)
{
    m_sessionStatus = sessionStatus;
}

QDateTime QCStudySession::startedAt() const
{
    return m_dateTimeStartedAt;
}

void QCStudySession::setStartedAt(const QDateTime& dateTimeStartedAt)
{
    m_dateTimeStartedAt = dateTimeStartedAt;
}

QDateTime QCStudySession::endedAt() const
{
    return m_dateTimeEndedAt;
}

void QCStudySession::setEndedAt(const QDateTime& dateTimeEndedAt)
{
    m_dateTimeEndedAt = dateTimeEndedAt;
}

QDateTime QCStudySession::createdAt() const
{
    return m_dateTimeCreatedAt;
}

void QCStudySession::setCreatedAt(const QDateTime& dateTimeCreatedAt)
{
    m_dateTimeCreatedAt = dateTimeCreatedAt;
}

QDateTime QCStudySession::updatedAt() const
{
    return m_dateTimeUpdatedAt;
}

void QCStudySession::setUpdatedAt(const QDateTime& dateTimeUpdatedAt)
{
    m_dateTimeUpdatedAt = dateTimeUpdatedAt;
}
