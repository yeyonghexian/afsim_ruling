#pragma once

#include <QString>
#include <QVector>
#include <QPoint>
#include <QMap>
#include <QStringList>
#include <QtGlobal>

enum class TaskEvent
{
    Fire,
    Hit,
    Detect,
    Jam
};

enum class TaskStatus
{
    Pending,
    Success,
    Failed
};

enum class AdjudicationMode
{
    Automatic,
    Manual
};

struct EnvironmentFactors
{
    int oceanDepth = 50;      // 0-100
    int airDryness = 50;      // 0-100
    int emInterference = 50;  // 0-100
    int temperature = 50;     // 0-100
    int humidity = 50;        // 0-100
};

struct Task
{
    QString name;
    int executionTime = 0;               // seconds in simulation timeline
    bool requiresFire = false;
    bool requiresHit = false;
    bool requiresDetection = false;
    bool requiresJam = false;
    QPoint targetCell;
    TaskStatus status = TaskStatus::Pending;
    QString ruleName;

    QString statusText() const
    {
        switch (status)
        {
        case TaskStatus::Pending:
            return QStringLiteral("等待 %1s").arg(executionTime);
        case TaskStatus::Success:
            return QStringLiteral("执行成功");
        case TaskStatus::Failed:
            return QStringLiteral("执行失败");
        }
        return {};
    }
};

struct Aircraft
{
    QString name;
    QVector<QPoint> route; // each point is cell coordinate inside 50x50 map
    QVector<Task> tasks;
    int currentRouteIndex = 0;
    double secondsPerStep = 1.0;
    double stepAccumulator = 0.0;

    QPoint position() const
    {
        if (route.isEmpty())
        {
            return {0, 0};
        }
        int idx = qBound(0, currentRouteIndex, route.size() - 1);
        return route.at(idx);
    }
};

struct AdjudicationRule
{
    QString name;
    QMap<QString, int> behaviorWeights; // e.g. "fire" -> score
    int successThreshold = 60;
};

struct AdjudicationModel
{
    QString name;
    QStringList factorKeys; // e.g. {"oceanDepth", "airDryness"}
    double environmentWeight = 0.7; // rest is manual/other factors
};

struct ManualAdjudicationState
{
    bool fireAllowed = true;
    bool fireHit = false;
    bool detectionSuccess = true;
    bool jamSuccess = true;
};

struct TaskLogEntry
{
    QString taskName;
    QString aircraftName;
    QString message;
    QString timestamp;
};

struct SimulationState
{
    QVector<Aircraft> aircrafts;
    QVector<AdjudicationRule> rules;
    QVector<AdjudicationModel> models;
    AdjudicationMode mode = AdjudicationMode::Automatic;
    QString currentRuleName;
    QString currentModelName;
    int simulationTime = 0; // seconds
    bool paused = false;
    QVector<TaskLogEntry> logs;
};
