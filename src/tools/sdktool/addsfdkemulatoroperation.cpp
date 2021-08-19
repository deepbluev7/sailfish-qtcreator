/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013-2014,2017,2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "addsfdkemulatoroperation.h"
#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "rmkeysoperation.h"
#include "getoperation.h"
#include "sfdkutils.h"

#include "../../libs/sfdk/sfdkconstants.h"

#include <QDateTime>
#include <QDir>

#include <iostream>

// FIXME most of these should be possible to detect at first run
const char MER_PARAM_PRODUCT_NAME[] = "--product-name";
const char MER_PARAM_PRODUCT_RELEASE[] = "--product-release";
const char MER_PARAM_VM_URI[] = "--vm-uri";
const char MER_PARAM_VM_FACTORY_SNAPSHOT[] = "--vm-factory-snapshot";
const char MER_PARAM_AUTODETECTED[] = "--autodetected";
const char MER_PARAM_SHARED_SSH[] = "--shared-ssh";
const char MER_PARAM_SHARED_CONFIG[] = "--shared-config";
const char MER_PARAM_HOST[] = "--host";
const char MER_PARAM_USERNAME[] = "--username";
const char MER_PARAM_PRIVATE_KEY_FILE[] = "--private-key-file";
const char MER_PARAM_SSH_PORT[] = "--ssh-port";
const char MER_PARAM_QML_LIVE_PORTS[] = "--qml-live-ports";
const char MER_PARAM_FREE_PORTS[] = "--free-ports";
const char MER_PARAM_MAC[] = "--mac";
const char MER_PARAM_SUBNET[] = "--subnet";
const char MER_PARAM_DEVICE_MODEL[] = "--device-model";
const char MER_PARAM_VIEW_SCALED[] = "--view-scaled";
const char MER_PARAM_INSTALLDIR[] = "--installdir";

using namespace Utils;
namespace C = Sfdk::Constants;

AddSfdkEmulatorOperation::AddSfdkEmulatorOperation()
{
}

QString AddSfdkEmulatorOperation::name() const
{
    return QLatin1String("addSfdkEmulator");
}

QString AddSfdkEmulatorOperation::helpText() const
{
    return QLatin1String("add an Sfdk emulator");
}

QString AddSfdkEmulatorOperation::argumentsHelpText() const
{
    const QString indent = QLatin1String("    ");
    return indent + QLatin1String(MER_PARAM_INSTALLDIR) + QLatin1String(" <DIR>            SDK installation directory (required).\n")
         + indent + QLatin1String(MER_PARAM_PRODUCT_NAME) + QLatin1String(" <NAME>         product name (required).\n")
         + indent + QLatin1String(MER_PARAM_PRODUCT_RELEASE) + QLatin1String(" <NAME>      product release name (required).\n")
         + indent + QLatin1String(MER_PARAM_VM_URI) + QLatin1String(" <URI>                virtual machine URI (required).\n")
         + indent + QLatin1String(MER_PARAM_VM_FACTORY_SNAPSHOT) + QLatin1String(" <NAME>  virtual machine factory snapshot (required).\n")
         + indent + QLatin1String(MER_PARAM_AUTODETECTED) + QLatin1String(" <BOOL>         is emulator autodetected.\n")
         + indent + QLatin1String(MER_PARAM_SHARED_SSH) + QLatin1String(" <PATH>           shared \"ssh\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_CONFIG) + QLatin1String(" <PATH>        shared \"config\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_HOST) + QLatin1String(" <NAME>                 ssh hostname (required).\n")
         + indent + QLatin1String(MER_PARAM_USERNAME) + QLatin1String(" <NAME>             ssh username (required).\n")
         + indent + QLatin1String(MER_PARAM_PRIVATE_KEY_FILE) + QLatin1String(" <FILE>     ssh private key file (required).\n")
         + indent + QLatin1String(MER_PARAM_SSH_PORT) + QLatin1String(" <NUMBER>           ssh port (required).\n")
         + indent + QLatin1String(MER_PARAM_QML_LIVE_PORTS) + QLatin1String(" <RANGE>      QmlLive ports (required).\n")
         + indent + QLatin1String(MER_PARAM_FREE_PORTS) + QLatin1String(" <RANGE>          free ports (required).\n")
         + indent + QLatin1String(MER_PARAM_MAC) + QLatin1String(" <ADDRESS>               MAC address (required).\n")
         + indent + QLatin1String(MER_PARAM_SUBNET) + QLatin1String(" <PREFIX>             network address prefix (required).\n")
         + indent + QLatin1String(MER_PARAM_DEVICE_MODEL) + QLatin1String(" <NAME>         device model name (required).\n")
         + indent + QLatin1String(MER_PARAM_VIEW_SCALED) + QLatin1String(" <BOOL>          view scaled (required).\n");
}

bool AddSfdkEmulatorOperation::setArguments(const QStringList &args)
{

    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String(MER_PARAM_INSTALLDIR)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_installDir = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_PRODUCT_NAME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_productName = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_PRODUCT_RELEASE)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_productRelease = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VM_URI)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_vmUri = QUrl(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VM_FACTORY_SNAPSHOT)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_vmFactorySnapshot = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_AUTODETECTED)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_autodetected = next == QLatin1String("true");
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_SSH)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedSshPath = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_CONFIG)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedConfigPath = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_HOST)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_host = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_USERNAME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_userName = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_PRIVATE_KEY_FILE)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_privateKeyFile = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SSH_PORT)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sshPort = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_QML_LIVE_PORTS)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_qmlLivePorts = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_FREE_PORTS)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_freePorts = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_MAC)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_mac = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SUBNET)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_subnet = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_DEVICE_MODEL)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_deviceModelName = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VIEW_SCALED)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_viewScaled = next == QLatin1String("true");
            continue;
        }
    }

    const char MISSING[] = " parameter missing.";
    bool error = false;
    if (m_installDir.isEmpty()) {
        std::cerr << MER_PARAM_INSTALLDIR << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_productName.isEmpty()) {
        std::cerr << MER_PARAM_PRODUCT_NAME << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_productRelease.isEmpty()) {
        std::cerr << MER_PARAM_PRODUCT_RELEASE << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_vmUri.isEmpty()) {
        std::cerr << MER_PARAM_VM_URI << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_vmFactorySnapshot.isEmpty()) {
        std::cerr << MER_PARAM_VM_FACTORY_SNAPSHOT << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedSshPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_SSH << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedConfigPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_CONFIG << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_host.isEmpty()) {
        std::cerr << MER_PARAM_HOST << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_userName.isEmpty()) {
        std::cerr << MER_PARAM_USERNAME << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_privateKeyFile.isEmpty()) {
        std::cerr << MER_PARAM_PRIVATE_KEY_FILE << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sshPort == 0) {
        std::cerr << MER_PARAM_SSH_PORT << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_qmlLivePorts.count() == 0) {
        std::cerr << MER_PARAM_QML_LIVE_PORTS << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_freePorts.count() == 0) {
        std::cerr << MER_PARAM_FREE_PORTS << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_mac.isEmpty()) {
        std::cerr << MER_PARAM_MAC << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_subnet.isEmpty()) {
        std::cerr << MER_PARAM_SUBNET << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_deviceModelName.isEmpty()) {
        std::cerr << MER_PARAM_DEVICE_MODEL << MISSING << std::endl << std::endl;
        error = true;
    }

    return !error;
}

int AddSfdkEmulatorOperation::execute() const
{
    useSfdkSettingsPath();

    QVariantMap map = load(QLatin1String("SfdkEmulators"));
    if (map.isEmpty())
        map = initializeEmulators(m_installDir);

    QVariantMap deviceModelMap;
    const int deviceModelsCount = map.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt();
    for (int i = 0; i < deviceModelsCount; ++i) {
        const QString deviceModelKey = QLatin1String(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(i);
        const QVariantMap tmpDeviceModelMap = map.value(deviceModelKey).toMap();
        const QString deviceModelName = tmpDeviceModelMap.value(C::DEVICE_MODEL_NAME).toString();
        if (deviceModelName == m_deviceModelName) {
            deviceModelMap = tmpDeviceModelMap;
            break;
        }
    }
    if (deviceModelMap.isEmpty()) {
        std::cerr << "Device model not found" << std::endl;
        return 2;
    };

    const QVariantMap result = addEmulator(map, m_productName, m_productRelease, m_vmUri, QDateTime::currentDateTime(),
            m_vmFactorySnapshot, m_autodetected, m_sharedSshPath, m_sharedConfigPath, m_host,
            m_userName, m_privateKeyFile, m_sshPort, m_qmlLivePorts, m_freePorts, m_mac, m_subnet,
            deviceModelMap, m_viewScaled);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, QLatin1String("SfdkEmulators")) ? 0 : 3;
}

QVariantMap AddSfdkEmulatorOperation::initializeEmulators(const QString &installDir, int version)
{
    const int CURRENT_VERSION = 1;

    QVariantMap map;
    map.insert(QLatin1String(C::EMULATORS_VERSION_KEY), version != -1 ? version : CURRENT_VERSION);
    map.insert(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY), installDir);
    map.insert(QLatin1String(C::EMULATORS_COUNT_KEY), 0);
    map.insert(QLatin1String(C::DEVICE_MODELS_COUNT_KEY), 0);
    return map;
}

QVariantMap AddSfdkEmulatorOperation::addEmulator(const QVariantMap &map,
                                          const QString &productName,
                                          const QString &productRelease,
                                          const QUrl &vmUri,
                                          const QDateTime &creationTime,
                                          const QString &vmFactorySnapshot,
                                          bool autodetected,
                                          const QString &sharedSshPath,
                                          const QString &sharedConfigPath,
                                          const QString &host,
                                          const QString &userName,
                                          const QString &privateKeyFile,
                                          quint16 sshPort,
                                          const QString &qmlLivePorts,
                                          const QString &freePorts,
                                          const QString &mac,
                                          const QString &subnet,
                                          const QVariantMap &deviceModelMap,
                                          bool viewScaled)
{
    QStringList valueKeys = FindValueOperation::findValue(map, QVariant(vmUri));
    bool hasTarget = false;
    foreach (const QString &t, valueKeys) {
        if (t.endsWith(QLatin1Char('/') + QLatin1String(C::EMULATOR_VM_URI))) {
            hasTarget = true;
            break;
        }
    }
    if (hasTarget) {
        std::cerr << "Error: VM " << qPrintable(vmUri.toString()) << " already defined as an emulator." << std::endl;
        return QVariantMap();
    }

    bool ok;
    const int count = GetOperation::get(map, QLatin1String(C::EMULATORS_COUNT_KEY)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Invalid emulators count, file seems corrupted." << std::endl;
        return QVariantMap();
    }

    const QString emulator = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(count);

    // remove old count
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, QStringList()
            << QLatin1String(C::EMULATORS_COUNT_KEY));

    KeyValuePairList data;
    auto addPrefix = [&](const QString &key, const QVariant &value) {
        return KeyValuePair(QStringList{emulator, key}, value);
    };
    data << addPrefix(QLatin1String(C::EMULATOR_PRODUCT_NAME), QVariant(productName));
    data << addPrefix(QLatin1String(C::EMULATOR_PRODUCT_RELEASE), QVariant(productRelease));
    data << addPrefix(QLatin1String(C::EMULATOR_VM_URI), QVariant(vmUri));
    data << addPrefix(QLatin1String(C::EMULATOR_CREATION_TIME), QVariant(creationTime));
    data << addPrefix(QLatin1String(C::EMULATOR_FACTORY_SNAPSHOT), QVariant(vmFactorySnapshot));
    data << addPrefix(QLatin1String(C::EMULATOR_AUTODETECTED), QVariant(autodetected));
    data << addPrefix(QLatin1String(C::EMULATOR_SHARED_SSH), QVariant(sharedSshPath));
    data << addPrefix(QLatin1String(C::EMULATOR_SHARED_CONFIG), QVariant(sharedConfigPath));
    data << addPrefix(QLatin1String(C::EMULATOR_HOST), QVariant(host));
    data << addPrefix(QLatin1String(C::EMULATOR_USER_NAME), QVariant(userName));
    data << addPrefix(QLatin1String(C::EMULATOR_PRIVATE_KEY_FILE), QVariant(privateKeyFile));
    data << addPrefix(QLatin1String(C::EMULATOR_SSH_PORT), QVariant(sshPort));
    data << addPrefix(QLatin1String(C::EMULATOR_QML_LIVE_PORTS), QVariant(qmlLivePorts));
    data << addPrefix(QLatin1String(C::EMULATOR_FREE_PORTS), QVariant(freePorts));
    data << addPrefix(QLatin1String(C::EMULATOR_MAC), QVariant(mac));
    data << addPrefix(QLatin1String(C::EMULATOR_SUBNET), QVariant(subnet));
    data << addPrefix(QLatin1String(C::EMULATOR_DEVICE_MODEL), QVariant(deviceModelMap));
    data << addPrefix(QLatin1String(C::EMULATOR_VIEW_SCALED), QVariant(viewScaled));
    data << KeyValuePair(QLatin1String(C::EMULATORS_COUNT_KEY), QVariant(count + 1));

    return AddKeysOperation::addKeys(cleaned, data);
}

#ifdef WITH_TESTS
bool AddSfdkEmulatorOperation::test() const
{
    QVariantMap map = initializeEmulators(QLatin1String("/dir"), 42);

    if (map.count() != 4
            || !map.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
            || map.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != 42
            || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dir")
            || !map.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
            || map.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 0
            || !map.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 0)
        return false;

    const auto now = QDateTime::currentDateTime();

    map = addEmulator(map,
                 QLatin1String("Sailfish OS"),
                 QLatin1String("1.2.3.4"),
                 QUrl("sfdkvm:VirtualBox#testEmulator"),
                 now,
                 QLatin1String("testSnapshot"),
                 true,
                 QLatin1String("/test/sharedSshPath"),
                 QLatin1String("/test/sharedConfigPath"),
                 QLatin1String("host"),
                 QLatin1String("user"),
                 QLatin1String("/test/privateKey"),
                 22,
                 QLatin1String("10234"),
                 QLatin1String("10000-10010"),
                 QLatin1String("de:ad:be:ef:fe:ed"),
                 QLatin1String("10.220.220"),
                 QVariantMap(),
                 false);

    const QString emulator = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(0);

    if (map.count() != 5
            || !map.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
            || map.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != 42
            || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dir")
            || !map.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
            || map.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 1
            || !map.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 0
            || !map.contains(emulator))
        return false;

    const QVariantMap emulatorMap = map.value(emulator).toMap();
    if (emulatorMap.count() != 18
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_PRODUCT_NAME))
            || emulatorMap.value(QLatin1String(C::EMULATOR_PRODUCT_NAME)).toString() != QLatin1String("Sailfish OS")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_PRODUCT_RELEASE))
            || emulatorMap.value(QLatin1String(C::EMULATOR_PRODUCT_RELEASE)).toString() != QLatin1String("1.2.3.4")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_VM_URI))
            || emulatorMap.value(QLatin1String(C::EMULATOR_VM_URI)).toUrl() != QUrl("sfdkvm:VirtualBox#testEmulator")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_CREATION_TIME))
            || emulatorMap.value(QLatin1String(C::EMULATOR_CREATION_TIME)).toDateTime() != now
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_FACTORY_SNAPSHOT))
            || emulatorMap.value(QLatin1String(C::EMULATOR_FACTORY_SNAPSHOT)).toString() != QLatin1String("testSnapshot")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_AUTODETECTED))
            || emulatorMap.value(QLatin1String(C::EMULATOR_AUTODETECTED)).toBool() != true
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_SHARED_SSH))
            || emulatorMap.value(QLatin1String(C::EMULATOR_SHARED_SSH)).toString() != QLatin1String("/test/sharedSshPath")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_SHARED_CONFIG))
            || emulatorMap.value(QLatin1String(C::EMULATOR_SHARED_CONFIG)).toString() != QLatin1String("/test/sharedConfigPath")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_HOST))
            || emulatorMap.value(QLatin1String(C::EMULATOR_HOST)).toString() != QLatin1String("host")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_USER_NAME))
            || emulatorMap.value(QLatin1String(C::EMULATOR_USER_NAME)).toString() != QLatin1String("user")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_PRIVATE_KEY_FILE))
            || emulatorMap.value(QLatin1String(C::EMULATOR_PRIVATE_KEY_FILE)).toString() != QLatin1String("/test/privateKey")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_SSH_PORT))
            || emulatorMap.value(QLatin1String(C::EMULATOR_SSH_PORT)).toInt() != 22
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_QML_LIVE_PORTS))
            || emulatorMap.value(QLatin1String(C::EMULATOR_QML_LIVE_PORTS)).toString() != QLatin1String("10234")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_FREE_PORTS))
            || emulatorMap.value(QLatin1String(C::EMULATOR_FREE_PORTS)).toString() != QLatin1String("10000-10010")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_MAC))
            || emulatorMap.value(QLatin1String(C::EMULATOR_MAC)).toString() != QLatin1String("de:ad:be:ef:fe:ed")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_SUBNET))
            || emulatorMap.value(QLatin1String(C::EMULATOR_SUBNET)).toString() != QLatin1String("10.220.220")
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_DEVICE_MODEL))
            || emulatorMap.value(QLatin1String(C::EMULATOR_DEVICE_MODEL)).toMap() != QVariantMap()
            || !emulatorMap.contains(QLatin1String(C::EMULATOR_VIEW_SCALED))
            || emulatorMap.value(QLatin1String(C::EMULATOR_VIEW_SCALED)).toBool() != false)
        return false;

    return true;
}
#endif
