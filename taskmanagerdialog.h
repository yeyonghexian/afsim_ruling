#pragma once

#include <QDialog>
#include <QStringList>
#include <QVector>

#include "models.h"

class QComboBox;
class QTableWidget;
class QPlainTextEdit;
class QDoubleSpinBox;

class TaskManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TaskManagerDialog(QWidget *parent = nullptr);

    void setAircrafts(QVector<Aircraft> *aircrafts);
    void setAvailableRuleNames(const QStringList &rules);

private:
    void refreshAircraftCombo();
    void loadCurrentAircraft();
    Aircraft *currentAircraft();
    Task *taskFromRow(int row);
    void populateTaskTable(const Aircraft &aircraft);
    void populateRouteEditor(const Aircraft &aircraft);
    void applyRouteChanges();
    void applySpeedChanges();

    bool editTask(Task &task, bool isNew);
    void addTask();
    void editSelectedTask();
    void removeSelectedTask();

    QVector<Aircraft> *m_aircrafts = nullptr;
    QStringList m_ruleNames;

    QComboBox *m_aircraftCombo = nullptr;
    QPlainTextEdit *m_routeEdit = nullptr;
    QDoubleSpinBox *m_speedSpin = nullptr;
    QTableWidget *m_taskTable = nullptr;
};
