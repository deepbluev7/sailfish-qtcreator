/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#pragma once

#include "sfdkglobal.h"

#include "asynchronous_p.h"

#include "virtualmachine.h"

#include <ssh/sshconnection.h>

#include <QBasicTimer>
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QtGlobal>

#include <functional>

namespace Sfdk {

class VmConnectionWaitForSystemRunningProcess;
class VmConnectionRemoteShutdownProcess;
class VmConnection : public QObject
{
    Q_OBJECT

    enum VmState {
        VmOff,
        VmAskBeforeStarting,
        VmStarting,
        VmStartingError,
        VmRunning,
        VmSoftClosing,
        VmHardClosing,
        VmCommitting,
        VmZombie,
    };

    enum SshState {
        SshNotConnected,
        SshConnecting,
        SshConnectingError,
        SshConnected,
        SshDisconnecting,
        SshDisconnected,
        SshConnectionLost,
    };

public:
    explicit VmConnection(VirtualMachine *parent);
    ~VmConnection() override;

    VirtualMachine *virtualMachine() const;

    VirtualMachine::State state() const;
    bool isStateChangePending() const;
    QString errorString() const;

    bool isVirtualMachineOff(bool *runningHeadless = 0, bool *startedOutside = 0) const;
    void lockDown(const QObject *context, const Functor<bool> &functor);
    void release(const QObject *context, const Functor<bool> &functor);
    // FIXME unsafe. Maybe add a version of lockDown(false) that does not fail when unlocked
    bool isLockedDown() const;

    void refresh(const QObject *context, const Functor<bool> &functor);
    void connectTo(VirtualMachine::ConnectOptions options, const QObject *context,
        const Functor<bool> &functor);
    void disconnectFrom(const QObject *context, const Functor<bool> &functor);

signals:
    void stateChanged();
    void stateChangePendingChanged(bool pending);
    void virtualMachineChanged();
    void virtualMachineOffChanged(bool vmOff);
    void lockDownFailed();
    void initGuest();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void scheduleReset();
    void reset();

    // state machine
    void updateState();
    void vmStmTransition(VmState toState, const char *event);
    bool vmStmExiting();
    bool vmStmEntering();
    void vmStmExec();
    bool vmStmStep();
    void sshStmTransition(SshState toState, const char *event);
    bool sshStmExiting();
    bool sshStmEntering();
    void sshStmExec();
    bool sshStmStep();

    void createConnection();
    void vmWantFastPollState(bool want);
    void vmPollState(const QObject *context = nullptr, const Functor<bool> &functor = {});
    void sshTryConnect();

    BatchComposer batchComposer() const;
    void addPendingStateChange(BatchRunner *batch);

    static const char *str(VirtualMachine::State state);
    static const char *str(VmState vmState);
    static const char *str(SshState sshState);

    using Ui = VirtualMachine::ConnectionUi;
    VirtualMachine::ConnectionUi *ui() const;
    void ask(Ui::Question which, void (VmConnection::*onStatusChanged)(),
            std::function<void()> ifYes, std::function<void()> ifNo);

    bool shouldCheckSshPortIsOccupied() const;

private slots:
    void vmStmScheduleExec();
    void sshStmScheduleExec();
    void onSshConnected();
    void onSshDisconnected();
    void onSshErrorOccured();
    void onGuestInitFinished();
    void onRemoteShutdownProcessFinished();
    void onCommitFinished(bool ok);
    void onAboutToShutDown();

private:
    const QPointer<VirtualMachine> m_vm;
    QPointer<QSsh::SshConnection> m_connection;
    QSsh::SshConnectionParameters m_lastConnectionParameters;
    QPointer<BatchRunner> m_batch;

    // state
    VirtualMachine::State m_state;
    int m_pendingStateChangesCount;
    QString m_errorString;
    VmState m_vmState;
    QElapsedTimer m_vmStateEntryTimer;
    bool m_vmStartedOutside;
    SshState m_sshState;

    // on-transition flags
    bool m_vmStmTransition;
    bool m_sshStmTransition;

    // state machine inputs (notice the difference in handling
    // m_lockDownRequested compared to m_{dis,}connectRequested!)
    bool m_lockDownRequested;
    bool m_connectRequested;
    bool m_disconnectRequested;
    bool m_connectLaterRequested;
    VirtualMachine::ConnectOptions m_connectOptions;
    bool m_cachedVmExists;
    bool m_cachedVmRunning;
    bool m_cachedVmRunningHeadless;
    bool m_cachedSshConnected;
    bool m_cachedSshErrorOccured;
    QString m_cachedSshErrorString;
    QPointer<QSsh::SshConnection> m_cachedSshErrorOrigin;
    bool m_vmGuestInitializing;
    bool m_vmCommitting;

    // timeout timers
    QBasicTimer m_vmStartingTimeoutTimer;
    QBasicTimer m_vmSoftClosingTimeoutTimer;
    QBasicTimer m_vmHardClosingTimeoutTimer;

    // background task timers
    int m_vmWantFastPollState;
    QBasicTimer m_vmStatePollTimer;
    bool m_pollingVmState;
    QBasicTimer m_sshTryConnectTimer;

    // state machine idle execution
    QBasicTimer m_vmStmExecTimer;
    QBasicTimer m_sshStmExecTimer;

    // auto invoke reset after properties are changed
    QBasicTimer m_resetTimer;

    QPointer<VmConnectionRemoteShutdownProcess> m_remoteShutdownProcess;
};

} // namespace Sfdk
