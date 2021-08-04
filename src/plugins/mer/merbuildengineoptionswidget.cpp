/****************************************************************************
**
** Copyright (C) 2012-2015,2017-2019 Jolla Ltd.
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

#include "merbuildengineoptionswidget.h"

#include "merbuildenginedetailswidget.h"
#include "merconnectionmanager.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mertargetmanagementdialog.h"
#include "mervmconnectionui.h"
#include "mervmselectiondialog.h"
#include "ui_merbuildengineoptionswidget.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <utils/fileutils.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QUrl>

using namespace Core;
using namespace QSsh;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerBuildEngineOptionsWidget::MerBuildEngineOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerBuildEngineOptionsWidget)
    , m_status(tr("Not connected."))
{
    m_ui->setupUi(this);
    connect(Sdk::instance(), &Sdk::buildEngineAdded,
            this, &MerBuildEngineOptionsWidget::onBuildEngineAdded);
    connect(Sdk::instance(), &Sdk::aboutToRemoveBuildEngine,
            this, &MerBuildEngineOptionsWidget::onAboutToRemoveBuildEngine);
    connect(m_ui->buildEngineComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &MerBuildEngineOptionsWidget::onBuildEngineChanged);
    connect(m_ui->addButton, &QPushButton::clicked,
            this, &MerBuildEngineOptionsWidget::onAddButtonClicked);
    connect(m_ui->removeButton, &QPushButton::clicked,
            this, &MerBuildEngineOptionsWidget::onRemoveButtonClicked);
    connect(m_ui->startVirtualMachineButton, &QPushButton::clicked,
            this, &MerBuildEngineOptionsWidget::onStartVirtualMachineButtonClicked);
    connect(m_ui->stopVirtualMachineButton, &QPushButton::clicked,
            this, &MerBuildEngineOptionsWidget::onStopVirtualMachineButtonClicked);
    connect(m_ui->manageTargetsButton, &QPushButton::clicked,
            this, &MerBuildEngineOptionsWidget::onManageTargetsButtonClicked);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::testConnectionButtonClicked,
            this, &MerBuildEngineOptionsWidget::onTestConnectionButtonClicked);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::sshTimeoutChanged,
            this, &MerBuildEngineOptionsWidget::onSshTimeoutChanged);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::sshPortChanged,
            this, &MerBuildEngineOptionsWidget::onSshPortChanged);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::headlessCheckBoxToggled,
            this, &MerBuildEngineOptionsWidget::onHeadlessCheckBoxToggled);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::srcFolderApplyButtonClicked,
            this, &MerBuildEngineOptionsWidget::onSrcFolderApplyButtonClicked);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::dBusPortChanged,
            this, &MerBuildEngineOptionsWidget::onDBusPortChanged);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::wwwProxyChanged,
            this, &MerBuildEngineOptionsWidget::onWwwProxyChanged);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::memorySizeMbChanged,
            this, &MerBuildEngineOptionsWidget::onMemorySizeMbChanged);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::swapSizeMbChanged,
            this, &MerBuildEngineOptionsWidget::onSwapSizeMbChanged);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::cpuCountChanged,
            this, &MerBuildEngineOptionsWidget::onCpuCountChanged);
    connect(m_ui->buildEngineDetailsWidget, &MerBuildEngineDetailsWidget::storageSizeMbChnaged,
            this, &MerBuildEngineOptionsWidget::onStorageSizeMbChnaged);
    for (int i = 0; i < Sdk::buildEngines().count(); ++i)
        onBuildEngineAdded(i);
    update();
}

MerBuildEngineOptionsWidget::~MerBuildEngineOptionsWidget()
{
    delete m_ui;
}

void MerBuildEngineOptionsWidget::setBuildEngine(const QUrl &uri)
{
    QTC_ASSERT(uri.isValid(), return);
    QTC_ASSERT(m_buildEngines.contains(uri), return);

    if (m_virtualMachine == uri)
        return;

    m_virtualMachine = uri;
    m_status = tr("Not tested.");
    update();
}

QString MerBuildEngineOptionsWidget::searchKeyWordMatchString() const
{
    const QChar blank = QLatin1Char(' ');
    QString keys;
    int count = m_ui->buildEngineComboBox->count();
    for (int i = 0; i < count; ++i)
        keys += m_ui->buildEngineComboBox->itemText(i) + blank;
    if (m_ui->buildEngineDetailsWidget->isVisible())
        keys += m_ui->buildEngineDetailsWidget->searchKeyWordMatchString();

    return keys.trimmed();
}

void MerBuildEngineOptionsWidget::store()
{
    m_storing = true;

    QProgressDialog progress(this);
    progress.setWindowModality(Qt::WindowModal);
    QPushButton *cancelButton = new QPushButton(tr("Abort"), &progress);
    cancelButton->setDisabled(true);
    progress.setCancelButton(cancelButton);
    progress.setMinimumDuration(2000);
    progress.setMinimum(0);
    progress.setMaximum(0);

    bool ok = true;

    QList<BuildEngine *> lockedDownBuildEngines;
    ok &= lockDownConnectionsOrCancelChangesThatNeedIt(&lockedDownBuildEngines);

    for (BuildEngine *const buildEngine : qAsConst(m_buildEngines)) {
        progress.setLabelText(tr("Applying build engine settings: '%1'").arg(buildEngine->name()));

        if (m_sshTimeout.contains(buildEngine)) {
            SshConnectionParameters sshParameters = buildEngine->virtualMachine()->sshParameters();
            sshParameters.timeout = m_sshTimeout[buildEngine];
            buildEngine->virtualMachine()->setSshParameters(sshParameters);
        }
        if (m_sshPort.contains(buildEngine)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&BuildEngine::setSshPort), buildEngine,
                    m_sshPort[buildEngine]);
            if (!stepOk) {
                m_ui->buildEngineDetailsWidget->setSshPort(buildEngine->sshPort());
                m_sshPort.remove(buildEngine);
                ok = false;
            }
        }
        if (m_headless.contains(buildEngine))
            buildEngine->virtualMachine()->setHeadless(m_headless[buildEngine]);
        if (m_dBusPort.contains(buildEngine)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&BuildEngine::setDBusPort), buildEngine,
                    m_dBusPort[buildEngine]);
            if (!stepOk) {
                m_ui->buildEngineDetailsWidget->setDBusPort(buildEngine->dBusPort());
                m_dBusPort.remove(buildEngine);
                ok = false;
            }
        }
        if (m_wwwProxy.contains(buildEngine)) {
            buildEngine->setWwwProxy(m_wwwProxy[buildEngine], m_wwwProxyServers[buildEngine],
                    m_wwwProxyExcludes[buildEngine]);
        }

        if (m_memorySizeMb.contains(buildEngine)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setMemorySizeMb),
                    buildEngine->virtualMachine(), m_memorySizeMb[buildEngine]);
            if (!stepOk) {
                m_ui->buildEngineDetailsWidget->setMemorySizeMb(
                        buildEngine->virtualMachine()->memorySizeMb());
                m_memorySizeMb.remove(buildEngine);
                ok = false;
            }
        }
        if (m_swapSizeMb.contains(buildEngine)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setSwapSizeMb),
                    buildEngine->virtualMachine(), m_swapSizeMb[buildEngine]);
            if (!stepOk) {
                m_ui->buildEngineDetailsWidget->setSwapSizeMb(
                        buildEngine->virtualMachine()->swapSizeMb());
                m_swapSizeMb.remove(buildEngine);
                ok = false;
            }
        }
        if (m_cpuCount.contains(buildEngine)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setCpuCount),
                    buildEngine->virtualMachine(), m_cpuCount[buildEngine]);
            if (!stepOk) {
                m_ui->buildEngineDetailsWidget->setCpuCount(
                        buildEngine->virtualMachine()->cpuCount());
                m_cpuCount.remove(buildEngine);
                ok = false;
            }
        }
        if (m_storageSizeMb.contains(buildEngine)) {
            const int newStorageSizeMb = m_storageSizeMb[buildEngine];
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setStorageSizeMb),
                    buildEngine->virtualMachine(), newStorageSizeMb);
            if (!stepOk) {
                m_storageSizeMb.remove(buildEngine);
                ok = false;
            }
            m_ui->buildEngineDetailsWidget->setStorageSizeMb(
                    buildEngine->virtualMachine()->storageSizeMb());
        }
    }

    for (BuildEngine *const buildEngine : lockedDownBuildEngines)
        buildEngine->virtualMachine()->lockDown(false, this, IgnoreAsynchronousReturn<bool>);

    if (!ok) {
        progress.cancel();
        QMessageBox::warning(this, tr("Some changes could not be saved!"),
                             tr("Failed to apply some of the changes to build engines"));
    }

    for (BuildEngine *const engine : Sdk::buildEngines()) {
        if (!m_buildEngines.contains(engine->uri()))
            Sdk::removeBuildEngine(engine->uri());
    }
    for (std::unique_ptr<BuildEngine> &newBuildEngine : m_newBuildEngines)
        Sdk::addBuildEngine(std::move(newBuildEngine));
    m_newBuildEngines.clear();

    m_sshTimeout.clear();
    m_sshPort.clear();
    m_headless.clear();
    m_dBusPort.clear();
    m_memorySizeMb.clear();
    m_swapSizeMb.clear();
    m_cpuCount.clear();
    m_storageSizeMb.clear();

    m_storing = false;
    update();
}

bool MerBuildEngineOptionsWidget::lockDownConnectionsOrCancelChangesThatNeedIt(QList<BuildEngine *>
        *lockedDownBuildEngines)
{
    QTC_ASSERT(lockedDownBuildEngines, return false);

    QList<BuildEngine *> failed;

    for (BuildEngine *const buildEngine : qAsConst(m_buildEngines)) {
        if (m_sshPort.value(buildEngine) == buildEngine->sshPort())
            m_sshPort.remove(buildEngine);
        if (m_dBusPort.value(buildEngine) == buildEngine->dBusPort())
            m_dBusPort.remove(buildEngine);
        if (m_memorySizeMb.value(buildEngine) == buildEngine->virtualMachine()->memorySizeMb())
            m_memorySizeMb.remove(buildEngine);
        if (m_swapSizeMb.value(buildEngine) == buildEngine->virtualMachine()->swapSizeMb())
            m_swapSizeMb.remove(buildEngine);
        if (m_cpuCount.value(buildEngine) == buildEngine->virtualMachine()->cpuCount())
            m_cpuCount.remove(buildEngine);
        if (m_storageSizeMb.value(buildEngine) == buildEngine->virtualMachine()->storageSizeMb())
            m_storageSizeMb.remove(buildEngine);

        if (!m_sshPort.contains(buildEngine)
                && !m_dBusPort.contains(buildEngine)
                && !m_memorySizeMb.contains(buildEngine)
                && !m_swapSizeMb.contains(buildEngine)
                && !m_cpuCount.contains(buildEngine)
                && !m_storageSizeMb.contains(buildEngine)) {
            continue;
        }

        if (!buildEngine->virtualMachine()->isOff()) {
            QPointer<QMessageBox> questionBox = new QMessageBox(QMessageBox::Question,
                    tr("Close Virtual Machine"),
                    tr("Close the \"%1\" virtual machine?")
                    .arg(buildEngine->virtualMachine()->name()),
                    QMessageBox::Yes | QMessageBox::No,
                    this);
            questionBox->setInformativeText(
                    tr("Some of the changes require stopping the virtual machine "
                        "before they can be applied."));
            if (questionBox->exec() != QMessageBox::Yes) {
                failed.append(buildEngine);
                continue;
            }
        }

        bool ok;
        execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::lockDown),
                buildEngine->virtualMachine(), true);
        if (!ok) {
            failed.append(buildEngine);
            continue;
        }

        lockedDownBuildEngines->append(buildEngine);
    }

    for (BuildEngine *const buildEngine : qAsConst(failed)) {
        m_ui->buildEngineDetailsWidget->setSshPort(buildEngine->sshPort());
        m_sshPort.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setDBusPort(buildEngine->dBusPort());
        m_dBusPort.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setMemorySizeMb(
                buildEngine->virtualMachine()->memorySizeMb());
        m_memorySizeMb.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setSwapSizeMb(
                buildEngine->virtualMachine()->swapSizeMb());
        m_swapSizeMb.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setCpuCount(buildEngine->virtualMachine()->cpuCount());
        m_cpuCount.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setStorageSizeMb(
                buildEngine->virtualMachine()->storageSizeMb());
        m_storageSizeMb.remove(buildEngine);
    }

    return failed.isEmpty();
}

void MerBuildEngineOptionsWidget::onBuildEngineChanged(int index)
{
    setBuildEngine(m_ui->buildEngineComboBox->itemData(index).toUrl());
}

void MerBuildEngineOptionsWidget::onAddButtonClicked()
{
    MerVmSelectionDialog dialog(this);
    dialog.setWindowTitle(tr("Add a %1 Build Engine").arg(Sdk::osVariant()));
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (m_buildEngines.contains(dialog.selectedVmUri()))
        return;

    std::unique_ptr<BuildEngine> buildEngine;
    // FIXME overload execAsynchronous to support this
    QEventLoop loop;
    auto whenDone = [&loop, &buildEngine](std::unique_ptr<BuildEngine> &&newBuildEngine) {
        loop.quit();
        buildEngine = std::move(newBuildEngine);
    };
    Sdk::createBuildEngine(dialog.selectedVmUri(), &loop, whenDone);
    loop.exec();
    QTC_ASSERT(buildEngine, return);

    m_buildEngines[buildEngine->virtualMachine()->uri()] = buildEngine.get();
    m_virtualMachine = buildEngine->virtualMachine()->uri();
    m_newBuildEngines.emplace_back(std::move(buildEngine));
    update();
}

void MerBuildEngineOptionsWidget::onRemoveButtonClicked()
{
    const QUrl vmUri = m_ui->buildEngineComboBox->itemData(
            m_ui->buildEngineComboBox->currentIndex(), Qt::UserRole).toUrl();
    QTC_ASSERT(vmUri.isValid(), return);

    if (m_buildEngines.contains(vmUri)) {
         BuildEngine *const removed = m_buildEngines.take(vmUri);
         Utils::erase(m_newBuildEngines, removed);
         if (!m_buildEngines.isEmpty())
             m_virtualMachine = m_buildEngines.keys().last();
         else
             m_virtualMachine.clear();

         m_sshTimeout.remove(removed);
         m_sshPort.remove(removed);
         m_headless.remove(removed);
         m_dBusPort.remove(removed);
         m_wwwProxy.remove(removed);
         m_wwwProxyServers.remove(removed);
         m_wwwProxyExcludes.remove(removed);
         m_storageSizeMb.remove(removed);
         m_memorySizeMb.remove(removed);
         m_swapSizeMb.remove(removed);
         m_cpuCount.remove(removed);
    }
    update();
}

void MerBuildEngineOptionsWidget::onTestConnectionButtonClicked()
{
    BuildEngine *const buildEngine = m_buildEngines[m_virtualMachine];
    if (!buildEngine->virtualMachine()->isOff()) {
        SshConnectionParameters params = buildEngine->virtualMachine()->sshParameters();
        if (m_sshPort.contains(buildEngine))
            params.setPort(m_sshPort[buildEngine]);
        m_ui->buildEngineDetailsWidget->setStatus(tr("Connecting…"));
        m_ui->buildEngineDetailsWidget->setTestButtonEnabled(false);
        m_status = MerConnectionManager::testConnection(params);
        m_ui->buildEngineDetailsWidget->setTestButtonEnabled(true);
        update();
    } else {
        m_ui->buildEngineDetailsWidget->setStatus(tr("Virtual machine is not running."));
    }
}

void MerBuildEngineOptionsWidget::onStartVirtualMachineButtonClicked()
{
    BuildEngine *const buildEngine = m_buildEngines[m_virtualMachine];
    if (buildEngine->virtualMachine()->isStateChangePending()) {
        MerVmConnectionUi::informStateChangePending();
        return;
    }
    buildEngine->virtualMachine()->connectTo(VirtualMachine::NoConnectOption, this,
            IgnoreAsynchronousReturn<bool>);
}

void MerBuildEngineOptionsWidget::onStopVirtualMachineButtonClicked()
{
    BuildEngine *const buildEngine = m_buildEngines[m_virtualMachine];
    if (buildEngine->virtualMachine()->isStateChangePending()) {
        MerVmConnectionUi::informStateChangePending();
        return;
    }
    buildEngine->virtualMachine()->disconnectFrom(this,
            IgnoreAsynchronousReturn<bool>);
}

void MerBuildEngineOptionsWidget::onManageTargetsButtonClicked()
{
    BuildEngine *const buildEngine = m_buildEngines[m_virtualMachine];
    MerTargetManagementDialog dialog(buildEngine, ICore::dialogParent());
    dialog.exec();
}

void MerBuildEngineOptionsWidget::onBuildEngineAdded(int index)
{
    BuildEngine *const buildEngine = Sdk::buildEngines().at(index);

    m_buildEngines[buildEngine->uri()] = buildEngine;
    m_virtualMachine = buildEngine->uri();

    auto cleaner = [=](auto*... caches) {
        return [=]() {
#if __cplusplus < 201703L
            using helper = int[];
            helper{ (caches->remove(buildEngine), 0)... };
#else
            (caches->remove(buildEngine), ...);
#endif
            update();
        };
    };

    connect(buildEngine->virtualMachine(), &VirtualMachine::sshParametersChanged,
            this, cleaner(&m_sshPort, &m_sshTimeout));
    connect(buildEngine->virtualMachine(), &VirtualMachine::headlessChanged,
            this, cleaner(&m_headless));
    connect(buildEngine, &BuildEngine::dBusPortChanged,
            this, cleaner(&m_dBusPort));
    connect(buildEngine, &BuildEngine::wwwProxyChanged,
            this, cleaner(&m_wwwProxy, &m_wwwProxyServers, &m_wwwProxyExcludes));
    connect(buildEngine->virtualMachine(), &VirtualMachine::storageSizeMbChanged,
            this, cleaner(&m_storageSizeMb));
    connect(buildEngine->virtualMachine(), &VirtualMachine::memorySizeMbChanged,
            this, cleaner(&m_memorySizeMb));
    connect(buildEngine->virtualMachine(), &VirtualMachine::swapSizeMbChanged,
            this, cleaner(&m_swapSizeMb));
    connect(buildEngine->virtualMachine(), &VirtualMachine::cpuCountChanged,
            this, cleaner(&m_cpuCount));

    update();
}

void MerBuildEngineOptionsWidget::onAboutToRemoveBuildEngine(int index)
{
    BuildEngine *const buildEngine = Sdk::buildEngines().at(index);

    m_buildEngines.remove(buildEngine->uri());
    if (m_virtualMachine == buildEngine->uri()) {
        m_virtualMachine.clear();
        if (!m_buildEngines.isEmpty())
            m_virtualMachine = m_buildEngines.first()->uri();
    }

    m_sshTimeout.remove(buildEngine);
    m_sshPort.remove(buildEngine);
    m_headless.remove(buildEngine);
    m_dBusPort.remove(buildEngine);
    m_wwwProxy.remove(buildEngine);
    m_wwwProxyServers.remove(buildEngine);
    m_wwwProxyExcludes.remove(buildEngine);
    m_storageSizeMb.remove(buildEngine);
    m_memorySizeMb.remove(buildEngine);
    m_swapSizeMb.remove(buildEngine);
    m_cpuCount.remove(buildEngine);

    update();
}

void MerBuildEngineOptionsWidget::onSrcFolderApplyButtonClicked(const QString &newFolder)
{
    BuildEngine *const buildEngine = m_buildEngines[m_virtualMachine];

    if ((newFolder == buildEngine->sharedSrcPath().toString())
        || (newFolder + QLatin1Char('/') == buildEngine->sharedSrcPath().toString())) {
        QMessageBox::information(this, tr("Choose a new folder"),
                                 tr("The given folder (%1) is the current workspace folder. "
                                    "Please choose another folder if you want to change it.")
                                 .arg(QDir::toNativeSeparators(buildEngine->sharedSrcPath().toString())));
        return;
    }

    if (!buildEngine->virtualMachine()->isOff()) {
        QPointer<QMessageBox> questionBox = new QMessageBox(QMessageBox::Question,
                tr("Close Virtual Machine"),
                tr("Close the \"%1\" virtual machine?").arg(buildEngine->name()),
                QMessageBox::Yes | QMessageBox::No,
                this);
        questionBox->setInformativeText(
                tr("Virtual machine must be closed before the source folder can be changed."));
        if (questionBox->exec() != QMessageBox::Yes) {
            // reset the path in the chooser
            m_ui->buildEngineDetailsWidget->setSrcFolderChooserPath(
                    buildEngine->sharedSrcPath().toString());
            return;
        }
    }

    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::lockDown),
            buildEngine->virtualMachine(), true);
    if (!ok) {
        QMessageBox::warning(this, tr("Failed"),
                tr("Workspace folder not changed"));
        // reset the path in the chooser
        m_ui->buildEngineDetailsWidget->setSrcFolderChooserPath(
                buildEngine->sharedSrcPath().toString());
        return;
    }

    execAsynchronous(std::tie(ok), std::mem_fn(&BuildEngine::setSharedSrcPath), buildEngine,
            FilePath::fromUserInput(newFolder));

    buildEngine->virtualMachine()->lockDown(false, this, IgnoreAsynchronousReturn<bool>);

    if (ok) {
        if (!Core::DocumentManager::projectsDirectory().isChildOf(buildEngine->sharedSrcPath()))
            Core::DocumentManager::setProjectsDirectory(buildEngine->sharedSrcPath());
        const QMessageBox::StandardButton response =
            QMessageBox::question(this, tr("Success!"),
                                  tr("Workspace folder for %1 changed to %2.\n\n"
                                     "Do you want to start %1 now?")
                                  .arg(buildEngine->name()).arg(QDir::toNativeSeparators(newFolder)),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
        if (response == QMessageBox::Yes) {
            buildEngine->virtualMachine()->connectTo(VirtualMachine::NoConnectOption, this,
                    IgnoreAsynchronousReturn<bool>);
        }
    }
    else {
        QMessageBox::warning(this, tr("Changing the workspace folder failed!"),
                             tr("Unable to change the workspace folder to %1")
                             .arg(QDir::toNativeSeparators(newFolder)));
        // reset the path in the chooser
        m_ui->buildEngineDetailsWidget->setSrcFolderChooserPath(
                buildEngine->sharedSrcPath().toString());
    }
}

void MerBuildEngineOptionsWidget::update()
{
    if (m_storing)
        return;

    m_ui->buildEngineComboBox->clear();
    for (BuildEngine *const buildEngine : m_buildEngines)
        m_ui->buildEngineComboBox->addItem(buildEngine->name(), buildEngine->uri());

    bool show = !m_virtualMachine.isEmpty();
    BuildEngine *const buildEngine = m_buildEngines.value(m_virtualMachine);
    QTC_ASSERT(!show || buildEngine, show = false);

    disconnect(m_vmOffConnection);

    if (show) {
        const auto sshParameters = buildEngine->virtualMachine()->sshParameters();
        m_ui->buildEngineDetailsWidget->setBuildEngine(buildEngine);
        if (m_sshTimeout.contains(buildEngine))
            m_ui->buildEngineDetailsWidget->setSshTimeout(m_sshTimeout[buildEngine]);
        else
            m_ui->buildEngineDetailsWidget->setSshTimeout(sshParameters.timeout);

        if (m_sshPort.contains(buildEngine))
            m_ui->buildEngineDetailsWidget->setSshPort(m_sshPort[buildEngine]);
        else
            m_ui->buildEngineDetailsWidget->setSshPort(buildEngine->sshPort());

        if (m_headless.contains(buildEngine))
            m_ui->buildEngineDetailsWidget->setHeadless(m_headless[buildEngine]);
        else
            m_ui->buildEngineDetailsWidget->setHeadless(buildEngine->virtualMachine()->isHeadless());

        if (m_dBusPort.contains(buildEngine))
            m_ui->buildEngineDetailsWidget->setDBusPort(m_dBusPort[buildEngine]);
        else
            m_ui->buildEngineDetailsWidget->setDBusPort(buildEngine->dBusPort());

        if (m_wwwProxy.contains(buildEngine)) {
            m_ui->buildEngineDetailsWidget->setWwwProxy(m_wwwProxy[buildEngine],
                    m_wwwProxyServers[buildEngine], m_wwwProxyExcludes[buildEngine]);
        } else {
            m_ui->buildEngineDetailsWidget->setWwwProxy(buildEngine->wwwProxyType(),
                    buildEngine->wwwProxyServers(), buildEngine->wwwProxyExcludes());
        }

        if (m_memorySizeMb.contains(buildEngine)) {
            m_ui->buildEngineDetailsWidget->setMemorySizeMb(m_memorySizeMb[buildEngine]);
        } else {
            m_ui->buildEngineDetailsWidget->setMemorySizeMb(
                    buildEngine->virtualMachine()->memorySizeMb());
        }

        if (m_swapSizeMb.contains(buildEngine)) {
            m_ui->buildEngineDetailsWidget->setSwapSizeMb(m_swapSizeMb[buildEngine]);
        } else {
            m_ui->buildEngineDetailsWidget->setSwapSizeMb(
                    buildEngine->virtualMachine()->swapSizeMb());
        }

        if (m_cpuCount.contains(buildEngine))
            m_ui->buildEngineDetailsWidget->setCpuCount(m_cpuCount[buildEngine]);
        else
            m_ui->buildEngineDetailsWidget->setCpuCount(buildEngine->virtualMachine()->cpuCount());

        if (m_storageSizeMb.contains(buildEngine)) {
            m_ui->buildEngineDetailsWidget->setStorageSizeMb(m_storageSizeMb[buildEngine]);
        } else {
            m_ui->buildEngineDetailsWidget->setStorageSizeMb(
                    buildEngine->virtualMachine()->storageSizeMb());
        }

        onVmOffChanged(buildEngine->virtualMachine()->isOff());
        m_vmOffConnection = connect(buildEngine->virtualMachine(),
                &VirtualMachine::virtualMachineOffChanged, this,
                &MerBuildEngineOptionsWidget::onVmOffChanged);

        int index = m_ui->buildEngineComboBox->findData(m_virtualMachine);
        m_ui->buildEngineComboBox->setCurrentIndex(index);
        m_ui->buildEngineDetailsWidget->setStatus(m_status);
    }

    m_ui->buildEngineDetailsWidget->setVisible(show);
    m_ui->removeButton->setEnabled(show && !buildEngine->isAutodetected());
    m_ui->startVirtualMachineButton->setEnabled(show);
    m_ui->stopVirtualMachineButton->setEnabled(show);
}

void MerBuildEngineOptionsWidget::onSshTimeoutChanged(int timeout)
{
    //store keys to be saved on save click
    m_sshTimeout[m_buildEngines[m_virtualMachine]] = timeout;
}

void MerBuildEngineOptionsWidget::onSshPortChanged(quint16 port)
{
    BuildEngine *const buildEngine = m_buildEngines[m_virtualMachine];

    //store keys to be saved on save click
    m_sshPort[buildEngine] = port;

    m_ui->buildEngineDetailsWidget->setSshPortOccupied(
            (buildEngine->virtualMachine()->isOff() || port != buildEngine->sshPort())
            && Sfdk::isPortOccupied(port));
}

void MerBuildEngineOptionsWidget::onHeadlessCheckBoxToggled(bool checked)
{
    //store keys to be saved on save click
    m_headless[m_buildEngines[m_virtualMachine]] = checked;
}

void MerBuildEngineOptionsWidget::onDBusPortChanged(quint16 port)
{
    //store keys to be saved on save click
    m_dBusPort[m_buildEngines[m_virtualMachine]] = port;
}

void MerBuildEngineOptionsWidget::onWwwProxyChanged(const QString &type, const QString &servers,
        const QString &excludes)
{
    //store keys to be saved on save click
    m_wwwProxy[m_buildEngines[m_virtualMachine]] = type;
    m_wwwProxyServers[m_buildEngines[m_virtualMachine]] = servers;
    m_wwwProxyExcludes[m_buildEngines[m_virtualMachine]] = excludes;
}

void MerBuildEngineOptionsWidget::onMemorySizeMbChanged(int sizeMb)
{
    m_memorySizeMb[m_buildEngines[m_virtualMachine]] = sizeMb;
}

void MerBuildEngineOptionsWidget::onSwapSizeMbChanged(int sizeMb)
{
    m_swapSizeMb[m_buildEngines[m_virtualMachine]] = sizeMb;
}

void MerBuildEngineOptionsWidget::onCpuCountChanged(int count)
{
    m_cpuCount[m_buildEngines[m_virtualMachine]] = count;
}

void MerBuildEngineOptionsWidget::onStorageSizeMbChnaged(int sizeMb)
{
    m_storageSizeMb[m_buildEngines[m_virtualMachine]] = sizeMb;
}

void MerBuildEngineOptionsWidget::onVmOffChanged(bool vmOff)
{
    BuildEngine *const buildEngine = m_buildEngines[m_virtualMachine];

    // If the VM is started, cancel any unsaved changes to values that cannot be changed online to
    // prevent inconsistencies
    if (!vmOff) {
        m_ui->buildEngineDetailsWidget->setSshPort(buildEngine->sshPort());
        m_sshPort.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setDBusPort(buildEngine->dBusPort());
        m_dBusPort.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setMemorySizeMb(
                buildEngine->virtualMachine()->memorySizeMb());
        m_memorySizeMb.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setSwapSizeMb(
                buildEngine->virtualMachine()->swapSizeMb());
        m_swapSizeMb.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setCpuCount(buildEngine->virtualMachine()->cpuCount());
        m_cpuCount.remove(buildEngine);
        m_ui->buildEngineDetailsWidget->setStorageSizeMb(
                buildEngine->virtualMachine()->storageSizeMb());
        m_storageSizeMb.remove(buildEngine);
    }

    m_ui->buildEngineDetailsWidget->setVmOffStatus(vmOff);
    m_ui->buildEngineDetailsWidget->setSshPortOccupied(
            vmOff && Sfdk::isPortOccupied(buildEngine->sshPort()));
}

} // Internal
} // Mer
