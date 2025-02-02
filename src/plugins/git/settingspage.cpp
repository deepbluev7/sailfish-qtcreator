/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "settingspage.h"
#include "gitsettings.h"
#include "gitplugin.h"
#include "gitclient.h"

#include "ui_settingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <vcsbase/vcsbaseconstants.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>

using namespace VcsBase;

namespace Git {
namespace Internal {

class GitSettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Git::Internal::SettingsPageWidget)

public:
    GitSettingsPageWidget(GitSettings *settings, const std::function<void()> &onChange);

    void apply() final;

private:
    void updateNoteField();

    std::function<void()> m_onChange;
    GitSettings *m_settings;
    Ui::SettingsPage m_ui;
};

GitSettingsPageWidget::GitSettingsPageWidget(GitSettings *settings, const std::function<void()> &onChange)
    : m_onChange(onChange), m_settings(settings)
{
    m_ui.setupUi(this);
    if (Utils::HostOsInfo::isWindowsHost()) {
        const QByteArray currentHome = qgetenv("HOME");
        const QString toolTip
                = tr("Set the environment variable HOME to \"%1\"\n(%2).\n"
                     "This causes Git to look for the SSH-keys in that location\n"
                     "instead of its installation directory when run outside git bash.").
                arg(QDir::homePath(),
                    currentHome.isEmpty() ? tr("not currently set") :
                            tr("currently set to \"%1\"").arg(QString::fromLocal8Bit(currentHome)));
        m_ui.winHomeCheckBox->setToolTip(toolTip);
    } else {
        m_ui.winHomeCheckBox->setVisible(false);
    }
    updateNoteField();

    m_ui.repBrowserCommandPathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.repBrowserCommandPathChooser->setHistoryCompleter("Git.RepoCommand.History");
    m_ui.repBrowserCommandPathChooser->setPromptDialogTitle(tr("Git Repository Browser Command"));

    connect(m_ui.pathLineEdit, &QLineEdit::textChanged, this, &GitSettingsPageWidget::updateNoteField);

    GitSettings &s = *m_settings;
    m_ui.pathLineEdit->setText(s.stringValue(GitSettings::pathKey));
    m_ui.logCountSpinBox->setValue(s.intValue(GitSettings::logCountKey));
    m_ui.timeoutSpinBox->setValue(s.intValue(GitSettings::timeoutKey));
    m_ui.pullRebaseCheckBox->setChecked(s.boolValue(GitSettings::pullRebaseKey));
    m_ui.addImmediatelyCheckBox->setChecked(s.boolValue(GitSettings::addImmediatelyKey));
    m_ui.winHomeCheckBox->setChecked(s.boolValue(GitSettings::winSetHomeEnvironmentKey));
    m_ui.gitkOptionsLineEdit->setText(s.stringValue(GitSettings::gitkOptionsKey));
    m_ui.repBrowserCommandPathChooser->setPath(s.stringValue(GitSettings::repositoryBrowserCmd));
}

void GitSettingsPageWidget::apply()
{
    GitSettings rc = *m_settings;
    rc.setValue(GitSettings::pathKey, m_ui.pathLineEdit->text());
    rc.setValue(GitSettings::logCountKey, m_ui.logCountSpinBox->value());
    rc.setValue(GitSettings::timeoutKey, m_ui.timeoutSpinBox->value());
    rc.setValue(GitSettings::pullRebaseKey, m_ui.pullRebaseCheckBox->isChecked());
    rc.setValue(GitSettings::addImmediatelyKey, m_ui.addImmediatelyCheckBox->isChecked());
    rc.setValue(GitSettings::winSetHomeEnvironmentKey, m_ui.winHomeCheckBox->isChecked());
    rc.setValue(GitSettings::gitkOptionsKey, m_ui.gitkOptionsLineEdit->text().trimmed());
    rc.setValue(GitSettings::repositoryBrowserCmd, m_ui.repBrowserCommandPathChooser->path().trimmed());

    if (rc != *m_settings) {
        *m_settings = rc;
        m_onChange();
    }
}

void GitSettingsPageWidget::updateNoteField()
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.prependOrSetPath(m_ui.pathLineEdit->text());

    bool showNote = env.searchInPath("perl").isEmpty();

    m_ui.noteFieldlabel->setVisible(showNote);
    m_ui.noteLabel->setVisible(showNote);
}

// -------- SettingsPage

GitSettingsPage::GitSettingsPage(GitSettings *settings, const std::function<void()> &onChange)
{
    setId(VcsBase::Constants::VCS_ID_GIT);
    setDisplayName(GitSettingsPageWidget::tr("Git"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([settings, onChange] { return new GitSettingsPageWidget(settings, onChange); });
}

} // namespace Internal
} // namespace Git
