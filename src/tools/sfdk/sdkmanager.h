/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include <utils/optional.h>

#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QString>

#include <memory>

namespace Sfdk {

class BuildEngine;
class BuildTargetData;
class Device;
class Emulator;
class RemoteProcess;
class Sdk;
class VirtualMachine;

class ToolsInfo
{
    Q_GADGET

public:
    enum Flag {
        NoFlag = 0x0,
        Tooling = 0x1,
        Target = 0x2,
        Available = 0x4,
        Installed = 0x8,
        UserDefined = 0x10,
        Snapshot = 0x20,
        Outdated = 0x40,
        Latest = 0x80,
        EarlyAccess = 0x100
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)

    QString name;
    QString parentName;
    Flags flags = NoFlag;
};

class EmulatorInfo
{
    Q_GADGET

public:
    enum Flag {
        NoFlag = 0x0,
        Available = 0x1,
        Installed = 0x2,
        UserDefined = 0x4,
        Default = 0x8,
        Latest = 0x10,
        EarlyAccess = 0x20
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)

    QString name;
    Flags flags = NoFlag;
};

class SdkManager
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(Sfdk::SdkManager)

public:
    enum ToolsTypeHint {
        NoToolsHint,
        ToolingHint,
        TargetHint
    };

    enum ListToolsOption {
        NoListToolsOption = 0x0,
        InstalledTools = 0x1,
        AvailableTools = 0x2,
        UserDefinedTools = 0x4,
        SnapshotTools = 0x8,
        CheckSnapshots = 0x10
    };
    Q_DECLARE_FLAGS(ListToolsOptions, ListToolsOption)
    Q_FLAG(ListToolsOptions)

    enum ListEmulatorsOption {
        NoListEmulatorOption = 0x0,
        InstalledEmulators = 0x1,
        AvailableEmulators = 0x2,
        UserDefinedEmulators = 0x4
    };
    Q_DECLARE_FLAGS(ListEmulatorsOptions, ListEmulatorsOption)
    Q_FLAG(ListEmulatorsOptions)

    explicit SdkManager(bool useSystemSettingsOnly);
    ~SdkManager();

    static bool isValid();

    static QString installationPath();
    static QString sdkMaintenanceToolPath();

    static bool hasEngine();
    static QString noEngineFoundMessage();
    static BuildEngine *engine();
    static bool startEngine();
    static bool stopEngine();
    static bool isEngineRunning();
    static int runOnEngine(const QString &program, const QStringList &arguments,
            const QProcessEnvironment &extraEnvironment = {},
            Utils::optional<bool> runInTerminal = {},
            QIODevice *out = nullptr, QIODevice *err = nullptr);

    static int runHook(const QString &program, const QStringList &arguments,
            const QString &workingDirectory, const QProcessEnvironment &extraEnvironment);
    static int runHookNative(const QString &program, const QStringList &arguments,
            const QString &workingDirectory, const QProcessEnvironment &extraEnvironment);
    static int runHookNativeByName(const QString &hook, const QStringList &arguments,
            const QString &workingDirectory, const QProcessEnvironment &extraEnvironment);

    static void setEnableReversePathMapping(bool enable);

    static bool listTools(ListToolsOptions options, QList<ToolsInfo> *info);
    static bool updateTools(const QString &name, ToolsTypeHint typeHint);
    static bool registerTools(const QString &maybeName, ToolsTypeHint typeHint,
            const QString &maybeUserName, const QString &maybePassword);
    static bool installTools(const QString &name, ToolsTypeHint typeHint);
    static bool installCustomTools(const QString &name, const QString &imageFileOrUrl,
            ToolsTypeHint typeHint, const QString &maybeTooling, bool noSnapshot);
    static bool cloneTools(const QString &name, const QString &cloneName, ToolsTypeHint typeHint);
    static bool removeTools(const QString &name, ToolsTypeHint typeHint, bool snapshotsOf);

    static BuildTargetData configuredTarget(QString *errorMessage);

    static Device *configuredDevice(QString *errorMessage);
    static Device *deviceByName(const QString &deviceName, QString *errorMessage);
    static bool prepareForRunOnDevice(const Device &device, RemoteProcess *process);
    static int runOnDevice(const Device &device, const QString &program,
            const QStringList &arguments, Utils::optional<bool> runInTerminal = {});

    static Emulator *emulatorByName(const QString &emulatorName, QString *errorMessage);
    static bool startEmulator(const Emulator &emulator);
    static bool stopEmulator(const Emulator &emulator);
    static bool isEmulatorRunning(const Emulator &emulator);
    static int runOnEmulator(const Emulator &emulator, const QString &program,
            const QStringList &arguments, Utils::optional<bool> runInTerminal = {});
    static bool listEmulators(ListEmulatorsOptions options, QList<EmulatorInfo> *info);
    static bool installEmulator(const QString &name);
    static bool removeEmulator(const QString &name);

    static bool startReliably(VirtualMachine *virtualMachine);
    static bool stopReliably(VirtualMachine *virtualMachine);
    static bool isRunningReliably(VirtualMachine *virtualMachine);

    static void saveSettings();

    static QString stateAvailableMessage() { return tr("available"); }
    static QString stateInstalledMessage() { return tr("installed"); }
    static QString stateSdkProvidedMessage() { return tr("sdk-provided"); }
    static QString stateUserDefinedMessage() { return tr("user-defined"); }
    static QString stateAutodetectedMessage() { return tr("autodetected"); }
    static QString stateLatestMessage() { return tr("latest"); }
    static QString stateEarlyAccessMessage() { return tr("early-access"); }
    static QString stateDefaultMessage() { return tr("default"); }

private:
    QString cleanSharedSrc() const;
    QString cleanSharedTarget(QString *errorMessage) const;
    bool mapEnginePaths(QString *program, QStringList *arguments, QString *workingDirectory,
            QProcessEnvironment *environment) const;
    bool reverseMapEnginePaths(QString *program, QStringList *arguments,
            QString *workingDirectory, QProcessEnvironment *environment) const;
    QByteArray maybeReverseMapEnginePaths(const QByteArray &commandOutput,
            const QString &cleanSharedSrc, const QString &cleanSharedTarget) const;
    QProcessEnvironment environmentToForwardToEngine() const;

private:
    static SdkManager *s_instance;
    bool m_enableReversePathMapping = true;
    std::unique_ptr<Sdk> m_sdk;
    BuildEngine *m_buildEngine = nullptr;
};

} // namespace Sfdk
