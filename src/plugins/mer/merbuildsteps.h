/****************************************************************************
**
** Copyright (C) 2014-2015,2018-2019 Jolla Ltd.
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

#ifndef MERBUILDSTEPS_H
#define MERBUILDSTEPS_H

#include "merabstractvmstartstep.h"

#include <projectexplorer/abstractprocessstep.h>

namespace Mer {
namespace Internal {

class MerSdkStartStep : public MerAbstractVmStartStep
{
    Q_OBJECT

public:
    explicit MerSdkStartStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    bool init() override;

    static Utils::Id stepId();
    static QString displayName();
};

class MerClearBuildEnvironmentStep: public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    MerClearBuildEnvironmentStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    bool init() override;

    static Utils::Id stepId();
    static QString displayName();

private:
    Utils::StringAspect *m_arguments = nullptr;
};

template<class Step>
class MerBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    MerBuildStepFactory()
    {
        registerStep<Step>(Step::stepId());
        setDisplayName(Step::displayName());
    }
};

} // namespace Internal
} // namespace Mer

#endif // MERBUILDSTEPS_H
