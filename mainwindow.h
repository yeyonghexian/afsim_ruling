#pragma once

#include <QMainWindow>

#include "models.h"
#include "adjudicationengine.h"

class EnvironmentGridWidget;
class QTreeWidget;
class QPlainTextEdit;
class QComboBox;
class QLabel;
class QPushButton;
class QTimer;
class ManualAdjudicationDialog;
class TaskManagerDialog;
class RuleModelManagerDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onModeChanged(int index);
    void onRuleChanged(const QString &name);
    void onModelChanged(const QString &name);
    void startSimulation();
    void pauseSimulation();
    void advanceSimulation();
    void openTaskManager();
    void openRuleModelManager();

private:
    void setupUi();
    void setupToolBar();
    void setupStatusBar();

    void loadSampleData();
    void refreshAircraftTree();
    void refreshRuleModelSelectors();
    void refreshModeSelector();
    void refreshLogView();
    void updateTimeLabel();

    void evaluateDueTasks();
    void handleTask(Aircraft &aircraft, Task &task);
    EnvironmentFactors factorsForTask(const Task &task) const;
    void moveAircraft(Aircraft &aircraft, double secondsElapsed);

    AdjudicationRule *findRule(const QString &name);
    AdjudicationModel *findModel(const QString &name);

    void appendLog(const QString &aircraftName, const Task &task, const QString &message);

    SimulationState m_state;

    EnvironmentGridWidget *m_grid = nullptr;
    QTreeWidget *m_taskTree = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    QComboBox *m_modeCombo = nullptr;
    QComboBox *m_ruleCombo = nullptr;
    QComboBox *m_modelCombo = nullptr;
    QLabel *m_timeLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QTimer *m_timer = nullptr;

    ManualAdjudicationDialog *m_manualDialog = nullptr;
    AdjudicationEngine m_engine;
};
