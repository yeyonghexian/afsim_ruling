#include "mainwindow.h"

#include "environmentgridwidget.h"
#include "manualadjudicationdialog.h"
#include "rulemodelmanagerdialog.h"
#include "taskmanagerdialog.h"

#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
QString requirementText(const Task &task)
{
    QStringList parts;
    if (task.requiresFire)
        parts << QStringLiteral("开火");
    if (task.requiresHit)
        parts << QStringLiteral("命中");
    if (task.requiresDetection)
        parts << QStringLiteral("探测");
    if (task.requiresJam)
        parts << QStringLiteral("电磁干扰");
    return parts.isEmpty() ? QStringLiteral("无") : parts.join(QLatin1Char(','));
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("裁决控制台"));
    resize(1600, 900);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::advanceSimulation);

    m_manualDialog = new ManualAdjudicationDialog(this);

    setupUi();
    loadSampleData();

    refreshModeSelector();
    refreshRuleModelSelectors();
    refreshAircraftTree();
    refreshLogView();
    updateTimeLabel();
    pauseSimulation();
}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    auto *centralLayout = new QHBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal, central);

    auto *leftPanel = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(6, 6, 6, 6);

    m_taskTree = new QTreeWidget(leftPanel);
    m_taskTree->setHeaderLabels({QStringLiteral("飞机/任务"), QStringLiteral("状态/时间"), QStringLiteral("详情")});
    m_taskTree->setRootIsDecorated(true);
    leftLayout->addWidget(m_taskTree, 1);

    m_logView = new QPlainTextEdit(leftPanel);
    m_logView->setReadOnly(true);
    m_logView->setPlaceholderText(QStringLiteral("裁决结果将显示在此"));
    leftLayout->addWidget(m_logView, 1);

    splitter->addWidget(leftPanel);

    m_grid = new EnvironmentGridWidget(splitter);
    splitter->addWidget(m_grid);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    centralLayout->addWidget(splitter);
    setCentralWidget(central);

    setupToolBar();
    setupStatusBar();
}

void MainWindow::setupToolBar()
{
    auto *toolbar = addToolBar(QStringLiteral("控制"));
    toolbar->setMovable(false);

    toolbar->addWidget(new QLabel(QStringLiteral("模式:"), toolbar));
    m_modeCombo = new QComboBox(toolbar);
    m_modeCombo->addItem(QStringLiteral("自动裁决"));
    m_modeCombo->addItem(QStringLiteral("人工裁决"));
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onModeChanged);
    toolbar->addWidget(m_modeCombo);

    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(QStringLiteral("裁决规则:"), toolbar));
    m_ruleCombo = new QComboBox(toolbar);
    connect(m_ruleCombo, &QComboBox::currentTextChanged, this, &MainWindow::onRuleChanged);
    toolbar->addWidget(m_ruleCombo);

    toolbar->addWidget(new QLabel(QStringLiteral("裁决模型:"), toolbar));
    m_modelCombo = new QComboBox(toolbar);
    connect(m_modelCombo, &QComboBox::currentTextChanged, this, &MainWindow::onModelChanged);
    toolbar->addWidget(m_modelCombo);

    toolbar->addSeparator();
    m_startButton = new QPushButton(QStringLiteral("开始"), toolbar);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startSimulation);
    toolbar->addWidget(m_startButton);

    m_pauseButton = new QPushButton(QStringLiteral("暂停"), toolbar);
    connect(m_pauseButton, &QPushButton::clicked, this, &MainWindow::pauseSimulation);
    toolbar->addWidget(m_pauseButton);

    toolbar->addSeparator();
    auto *taskAction = toolbar->addAction(QStringLiteral("任务管理"));
    connect(taskAction, &QAction::triggered, this, &MainWindow::openTaskManager);

    auto *ruleAction = toolbar->addAction(QStringLiteral("规则/模型管理"));
    connect(ruleAction, &QAction::triggered, this, &MainWindow::openRuleModelManager);

    toolbar->addSeparator();
    auto *clearLogBtn = new QPushButton(QStringLiteral("清除日志"), toolbar);
    connect(clearLogBtn, &QPushButton::clicked, this, &MainWindow::clearLog);
    toolbar->addWidget(clearLogBtn);

    auto *resetBtn = new QPushButton(QStringLiteral("重新开始"), toolbar);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetSimulation);
    toolbar->addWidget(resetBtn);
}

void MainWindow::setupStatusBar()
{
    m_timeLabel = new QLabel(QStringLiteral("仿真时间: 0 s"), this);
    statusBar()->addPermanentWidget(m_timeLabel);
}

void MainWindow::onModeChanged(int index)
{
    m_state.mode = index == 0 ? AdjudicationMode::Automatic : AdjudicationMode::Manual;
    const bool automatic = m_state.mode == AdjudicationMode::Automatic;
    if (m_ruleCombo)
        m_ruleCombo->setEnabled(automatic);
    if (m_modelCombo)
        m_modelCombo->setEnabled(automatic);
}

void MainWindow::onRuleChanged(const QString &name)
{
    if (!name.isEmpty())
    {
        m_state.currentRuleName = name;
    }
}

void MainWindow::onModelChanged(const QString &name)
{
    if (!name.isEmpty())
    {
        m_state.currentModelName = name;
    }
}

void MainWindow::startSimulation()
{
    if (!m_timer || !m_state.paused)
        return;

    m_state.paused = false;
    m_timer->start(1000);
    if (m_startButton)
        m_startButton->setEnabled(false);
    if (m_pauseButton)
        m_pauseButton->setEnabled(true);
}

void MainWindow::pauseSimulation()
{
    if (!m_timer)
        return;
    m_state.paused = true;
    m_timer->stop();
    if (m_startButton)
        m_startButton->setEnabled(true);
    if (m_pauseButton)
        m_pauseButton->setEnabled(false);
}

void MainWindow::advanceSimulation()
{
    if (m_state.paused)
        return;

    ++m_state.simulationTime;
    updateTimeLabel();

    for (Aircraft &ac : m_state.aircrafts)
    {
        moveAircraft(ac, 1.0);
    }
    if (m_grid)
    {
        m_grid->update();
    }

    evaluateDueTasks();
}

void MainWindow::openTaskManager()
{
    TaskManagerDialog dialog(this);
    dialog.setAircrafts(&m_state.aircrafts);
    QStringList rules;
    for (const AdjudicationRule &rule : m_state.rules)
    {
        rules << rule.name;
    }
    dialog.setAvailableRuleNames(rules);

    dialog.exec();
    refreshAircraftTree();
    if (m_grid)
        m_grid->update();
}

void MainWindow::openRuleModelManager()
{
    RuleModelManagerDialog dialog(this);
    dialog.setData(&m_state.rules, &m_state.models);
    dialog.exec();

    refreshRuleModelSelectors();
}

void MainWindow::loadSampleData()
{
    m_state.logs.clear();
    m_state.simulationTime = 0;
    m_state.aircrafts.clear();
    m_state.rules.clear();
    m_state.models.clear();
    m_state.mode = AdjudicationMode::Automatic;

    AdjudicationRule baseRule;
    baseRule.name = QStringLiteral("标准规则");
    baseRule.successThreshold = 60;
    baseRule.behaviorWeights.insert(QStringLiteral("fire"), 25);
    baseRule.behaviorWeights.insert(QStringLiteral("hit"), 25);
    baseRule.behaviorWeights.insert(QStringLiteral("detect"), 20);
    baseRule.behaviorWeights.insert(QStringLiteral("jam"), 15);
    m_state.rules.append(baseRule);

    AdjudicationRule aggressiveRule;
    aggressiveRule.name = QStringLiteral("进攻优先");
    aggressiveRule.successThreshold = 55;
    aggressiveRule.behaviorWeights.insert(QStringLiteral("fire"), 30);
    aggressiveRule.behaviorWeights.insert(QStringLiteral("hit"), 30);
    aggressiveRule.behaviorWeights.insert(QStringLiteral("detect"), 10);
    aggressiveRule.behaviorWeights.insert(QStringLiteral("jam"), 15);
    m_state.rules.append(aggressiveRule);

    AdjudicationModel envModel;
    envModel.name = QStringLiteral("环境优先模型");
    envModel.factorKeys = QStringList{QStringLiteral("oceanDepth"), QStringLiteral("airDryness"), QStringLiteral("emInterference")};
    envModel.environmentWeight = 0.8;
    m_state.models.append(envModel);

    AdjudicationModel balancedModel;
    balancedModel.name = QStringLiteral("均衡模型");
    balancedModel.factorKeys = QStringList{QStringLiteral("temperature"), QStringLiteral("humidity")};
    balancedModel.environmentWeight = 0.6;
    m_state.models.append(balancedModel);

    m_state.currentRuleName = baseRule.name;
    m_state.currentModelName = envModel.name;

    Aircraft red;
    red.name = QStringLiteral("红方-1");
    red.route = {QPoint(2, 2), QPoint(10, 5), QPoint(20, 15), QPoint(30, 25), QPoint(40, 35)};
    red.secondsPerStep = 1.5;

    Task patrol;
    patrol.name = QStringLiteral("空域巡逻");
    patrol.executionTime = 3;
    patrol.requiresDetection = true;
    patrol.targetCell = QPoint(10, 5);
    patrol.ruleName = baseRule.name;
    red.tasks.append(patrol);

    Task strike;
    strike.name = QStringLiteral("远程打击");
    strike.executionTime = 8;
    strike.requiresFire = true;
    strike.requiresHit = true;
    strike.requiresJam = true;
    strike.targetCell = QPoint(30, 25);
    strike.ruleName = aggressiveRule.name;
    red.tasks.append(strike);

    Aircraft blue;
    blue.name = QStringLiteral("蓝方-1");
    blue.route = {QPoint(48, 10), QPoint(40, 12), QPoint(32, 20), QPoint(20, 30), QPoint(5, 40)};
    blue.secondsPerStep = 1.2;

    Task recon;
    recon.name = QStringLiteral("光电侦察");
    recon.executionTime = 5;
    recon.requiresDetection = true;
    recon.targetCell = QPoint(32, 20);
    recon.ruleName = baseRule.name;
    blue.tasks.append(recon);

    Task support;
    support.name = QStringLiteral("干扰支援");
    support.executionTime = 9;
    support.requiresJam = true;
    support.targetCell = QPoint(20, 30);
    support.ruleName = balancedModel.name;
    blue.tasks.append(support);

    m_state.aircrafts.append(red);
    m_state.aircrafts.append(blue);

    if (m_grid)
    {
        m_grid->setAircrafts(&m_state.aircrafts);
    }
}

void MainWindow::refreshAircraftTree()
{
    if (!m_taskTree)
        return;

    m_taskTree->clear();
    for (const Aircraft &ac : m_state.aircrafts)
    {
        auto *aircraftItem = new QTreeWidgetItem(m_taskTree);
        aircraftItem->setText(0, ac.name);
        aircraftItem->setText(1, QStringLiteral("速度 %1s/格").arg(ac.secondsPerStep, 0, 'f', 1));
        aircraftItem->setText(2, QStringLiteral("航迹点 %1").arg(ac.route.size()));

        for (const Task &task : ac.tasks)
        {
            auto *taskItem = new QTreeWidgetItem(aircraftItem);
            taskItem->setText(0, QStringLiteral("- %1").arg(task.name));
            taskItem->setText(1, task.statusText());
            taskItem->setText(2, QStringLiteral("目标(%1,%2) | %3").arg(task.targetCell.x()).arg(task.targetCell.y()).arg(requirementText(task)));
            if (task.status == TaskStatus::Success)
            {
                taskItem->setForeground(1, QBrush(QColor(0, 128, 0)));
            }
            else if (task.status == TaskStatus::Failed)
            {
                taskItem->setForeground(1, QBrush(Qt::red));
            }
        }
        aircraftItem->setExpanded(true);
    }
}

void MainWindow::refreshRuleModelSelectors()
{
    if (m_ruleCombo)
    {
        m_ruleCombo->blockSignals(true);
        m_ruleCombo->clear();
        for (const AdjudicationRule &rule : m_state.rules)
        {
            m_ruleCombo->addItem(rule.name);
        }
        int ruleIndex = m_ruleCombo->findText(m_state.currentRuleName);
        if (ruleIndex < 0 && m_ruleCombo->count() > 0)
        {
            ruleIndex = 0;
            m_state.currentRuleName = m_ruleCombo->itemText(0);
        }
        if (ruleIndex >= 0)
        {
            m_ruleCombo->setCurrentIndex(ruleIndex);
        }
        m_ruleCombo->setEnabled(m_state.mode == AdjudicationMode::Automatic && m_ruleCombo->count() > 0);
        m_ruleCombo->blockSignals(false);
    }

    if (m_modelCombo)
    {
        m_modelCombo->blockSignals(true);
        m_modelCombo->clear();
        for (const AdjudicationModel &model : m_state.models)
        {
            m_modelCombo->addItem(model.name);
        }
        int modelIndex = m_modelCombo->findText(m_state.currentModelName);
        if (modelIndex < 0 && m_modelCombo->count() > 0)
        {
            modelIndex = 0;
            m_state.currentModelName = m_modelCombo->itemText(0);
        }
        if (modelIndex >= 0)
        {
            m_modelCombo->setCurrentIndex(modelIndex);
        }
        m_modelCombo->setEnabled(m_state.mode == AdjudicationMode::Automatic && m_modelCombo->count() > 0);
        m_modelCombo->blockSignals(false);
    }
}

void MainWindow::refreshModeSelector()
{
    if (!m_modeCombo)
        return;
    m_modeCombo->blockSignals(true);
    m_modeCombo->setCurrentIndex(m_state.mode == AdjudicationMode::Automatic ? 0 : 1);
    m_modeCombo->blockSignals(false);
    onModeChanged(m_modeCombo->currentIndex());
}

void MainWindow::refreshLogView()
{
    if (!m_logView)
        return;

    QStringList lines;
    QString lastTaskKey;
    for (const TaskLogEntry &entry : m_state.logs)
    {
        QString currentTaskKey = entry.aircraftName + QLatin1String("::") + entry.taskName;
        
        // 如果任务改变，添加空行分隔
        if (!lastTaskKey.isEmpty() && lastTaskKey != currentTaskKey)
        {
            lines << QString();
        }
        
        lines << QStringLiteral("[%1][%2][%3] %4").arg(entry.timestamp, entry.aircraftName, entry.taskName, entry.message);
        lastTaskKey = currentTaskKey;
    }
    m_logView->setPlainText(lines.join(QLatin1Char('\n')));
    if (auto *bar = m_logView->verticalScrollBar())
    {
        bar->setValue(bar->maximum());
    }
}

void MainWindow::updateTimeLabel()
{
    if (m_timeLabel)
    {
        m_timeLabel->setText(QStringLiteral("仿真时间: %1 s").arg(m_state.simulationTime));
    }
}

void MainWindow::evaluateDueTasks()
{
    for (Aircraft &ac : m_state.aircrafts)
    {
        for (Task &task : ac.tasks)
        {
            if (task.status == TaskStatus::Pending && task.executionTime <= m_state.simulationTime)
            {
                handleTask(ac, task);
            }
        }
    }
    refreshAircraftTree();
}

void MainWindow::handleTask(Aircraft &aircraft, Task &task)
{
    AdjudicationRule *rule = findRule(task.ruleName.isEmpty() ? m_state.currentRuleName : task.ruleName);
    if (!rule && !m_state.rules.isEmpty())
    {
        rule = &m_state.rules.first();
    }
    if (!rule)
    {
        appendLog(aircraft.name, task, QStringLiteral("未找到可用的裁决规则"));
        task.status = TaskStatus::Failed;
        return;
    }

    AdjudicationModel *model = findModel(m_state.currentModelName);
    if (!model && !m_state.models.isEmpty())
    {
        model = &m_state.models.first();
    }
    if (!model)
    {
        appendLog(aircraft.name, task, QStringLiteral("未找到可用的裁决模型"));
        task.status = TaskStatus::Failed;
        return;
    }

    ManualAdjudicationState manualState;
    if (m_state.mode == AdjudicationMode::Manual)
    {
        const bool resumeAfter = !m_state.paused;
        if (resumeAfter)
        {
            pauseSimulation();
        }

        m_manualDialog->setContext(QStringLiteral("人工裁决 - %1").arg(task.name), task, manualState);
        if (m_manualDialog->exec() == QDialog::Accepted)
        {
            manualState = m_manualDialog->state();
        }
        else
        {
            appendLog(aircraft.name, task, QStringLiteral("人工裁决被取消，任务失败"));
            task.status = TaskStatus::Failed;
            if (resumeAfter)
            {
                startSimulation();
            }
            return;
        }

        if (resumeAfter)
        {
            startSimulation();
        }
    }

    QStringList logEntries;
    EnvironmentFactors factors = factorsForTask(task);
    TaskStatus status = m_engine.adjudicate(task, factors, *rule, *model, m_state.mode, manualState, &logEntries);
    for (const QString &line : logEntries)
    {
        appendLog(aircraft.name, task, line);
    }
    appendLog(aircraft.name, task, status == TaskStatus::Success ? QStringLiteral("任务裁决成功") : QStringLiteral("任务裁决失败"));
    refreshLogView();
}

EnvironmentFactors MainWindow::factorsForTask(const Task &task) const
{
    if (m_grid)
    {
        return m_grid->factorsAt(task.targetCell);
    }
    return {};
}

void MainWindow::moveAircraft(Aircraft &aircraft, double secondsElapsed)
{
    if (aircraft.route.size() < 2)
        return;

    aircraft.stepAccumulator += secondsElapsed;
    while (aircraft.stepAccumulator >= aircraft.secondsPerStep && aircraft.currentRouteIndex + 1 < aircraft.route.size())
    {
        aircraft.stepAccumulator -= aircraft.secondsPerStep;
        ++aircraft.currentRouteIndex;
    }
}

AdjudicationRule *MainWindow::findRule(const QString &name)
{
    for (AdjudicationRule &rule : m_state.rules)
    {
        if (rule.name == name)
        {
            return &rule;
        }
    }
    return nullptr;
}

AdjudicationModel *MainWindow::findModel(const QString &name)
{
    for (AdjudicationModel &model : m_state.models)
    {
        if (model.name == name)
        {
            return &model;
        }
    }
    return nullptr;
}

void MainWindow::appendLog(const QString &aircraftName, const Task &task, const QString &message)
{
    TaskLogEntry entry;
    entry.aircraftName = aircraftName;
    entry.taskName = task.name;
    entry.message = message;
    entry.timestamp = QStringLiteral("T+%1s").arg(m_state.simulationTime);
    m_state.logs.append(entry);
    refreshLogView();
}

void MainWindow::clearLog()
{
    m_state.logs.clear();
    refreshLogView();
}

void MainWindow::resetSimulation()
{
    // 暂停仿真
    pauseSimulation();

    // 重置仿真时间
    m_state.simulationTime = 0;

    // 重置所有飞机状态
    for (Aircraft &aircraft : m_state.aircrafts)
    {
        aircraft.currentRouteIndex = 0;
        aircraft.stepAccumulator = 0.0;

        // 重置所有任务状态为待执行
        for (Task &task : aircraft.tasks)
        {
            task.status = TaskStatus::Pending;
        }
    }

    // 清除日志
    m_state.logs.clear();

    // 刷新所有显示
    refreshAircraftTree();
    refreshLogView();
    updateTimeLabel();

    if (m_grid)
    {
        m_grid->update();
    }
}
