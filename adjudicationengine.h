#pragma once

#include "models.h"

class AdjudicationEngine
{
public:
    AdjudicationEngine() = default;

    double computeEnvironmentScore(const EnvironmentFactors &factors, const QStringList &keys) const;
    bool eventSuccess(TaskEvent event,
                      const EnvironmentFactors &factors,
                      const AdjudicationModel &model,
                      AdjudicationMode mode,
                      const ManualAdjudicationState &manualState) const;

    TaskStatus adjudicate(Task &task,
                          const EnvironmentFactors &factors,
                          const AdjudicationRule &rule,
                          const AdjudicationModel &model,
                          AdjudicationMode mode,
                          const ManualAdjudicationState &manualState,
                          QStringList *log = nullptr) const;

private:
    double weightFor(const AdjudicationRule &rule, const QString &behaviorKey) const;
};
