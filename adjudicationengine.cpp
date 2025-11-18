#include "adjudicationengine.h"

#include <QtMath>

namespace
{
int factorValue(const EnvironmentFactors &factors, const QString &key)
{
    if (key == QLatin1String("oceanDepth"))
        return factors.oceanDepth;
    if (key == QLatin1String("airDryness"))
        return factors.airDryness;
    if (key == QLatin1String("emInterference"))
        return factors.emInterference;
    if (key == QLatin1String("temperature"))
        return factors.temperature;
    if (key == QLatin1String("humidity"))
        return factors.humidity;
    return 50;
}
}

double AdjudicationEngine::computeEnvironmentScore(const EnvironmentFactors &factors, const QStringList &keys) const
{
    if (keys.isEmpty())
    {
        return 0.5;
    }

    double sum = 0;
    for (const QString &key : keys)
    {
        sum += factorValue(factors, key) / 100.0;
    }
    return sum / keys.size();
}

bool AdjudicationEngine::eventSuccess(TaskEvent event,
                                      const EnvironmentFactors &factors,
                                      const AdjudicationModel &model,
                                      AdjudicationMode mode,
                                      const ManualAdjudicationState &manualState) const
{
    if (mode == AdjudicationMode::Manual)
    {
        switch (event)
        {
        case TaskEvent::Fire:
            return manualState.fireAllowed;
        case TaskEvent::Hit:
            return manualState.fireHit;
        case TaskEvent::Detect:
            return manualState.detectionSuccess;
        case TaskEvent::Jam:
            return manualState.jamSuccess;
        }
    }

    const double envScore = computeEnvironmentScore(factors, model.factorKeys);
    const double manualWeight = 1.0 - model.environmentWeight;

    double base = model.environmentWeight * envScore + manualWeight * 0.9; // assume manual factors succeed

    switch (event)
    {
    case TaskEvent::Fire:
        base += 0.05;
        break;
    case TaskEvent::Hit:
        base -= 0.05;
        break;
    case TaskEvent::Detect:
        base += 0.02;
        break;
    case TaskEvent::Jam:
        base -= 0.02;
        break;
    }

    return base >= 0.5;
}

double AdjudicationEngine::weightFor(const AdjudicationRule &rule, const QString &behaviorKey) const
{
    return rule.behaviorWeights.value(behaviorKey, 0);
}

TaskStatus AdjudicationEngine::adjudicate(Task &task,
                                          const EnvironmentFactors &factors,
                                          const AdjudicationRule &rule,
                                          const AdjudicationModel &model,
                                          AdjudicationMode mode,
                                          const ManualAdjudicationState &manualState,
                                          QStringList *log) const
{
    double score = 0;

    auto logEvent = [&](const QString &text) {
        if (log)
        {
            log->append(text);
        }
    };

    if (task.requiresFire)
    {
        const bool ok = eventSuccess(TaskEvent::Fire, factors, model, mode, manualState);
        score += ok ? weightFor(rule, QStringLiteral("fire")) : 0;
        logEvent(ok ? QObject::tr("开火许可通过") : QObject::tr("开火许可被拒"));

        if (task.requiresHit)
        {
            const bool hit = ok && eventSuccess(TaskEvent::Hit, factors, model, mode, manualState);
            score += hit ? weightFor(rule, QStringLiteral("hit")) : 0;
            logEvent(hit ? QObject::tr("命中目标") : QObject::tr("未命中目标"));
        }
    }

    if (task.requiresDetection)
    {
        const bool detect = eventSuccess(TaskEvent::Detect, factors, model, mode, manualState);
        score += detect ? weightFor(rule, QStringLiteral("detect")) : 0;
        logEvent(detect ? QObject::tr("探测成功") : QObject::tr("探测失败"));
    }

    if (task.requiresJam)
    {
        const bool jam = eventSuccess(TaskEvent::Jam, factors, model, mode, manualState);
        score += jam ? weightFor(rule, QStringLiteral("jam")) : 0;
        logEvent(jam ? QObject::tr("电磁干扰成功") : QObject::tr("电磁干扰失败"));
    }

    TaskStatus result = score >= rule.successThreshold ? TaskStatus::Success : TaskStatus::Failed;
    logEvent(QObject::tr("任务得分 %1 / %2").arg(score).arg(rule.successThreshold));
    task.status = result;
    return result;
}
