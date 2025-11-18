#include "taskmanagerdialog.h"

#include <QComboBox>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QDoubleSpinBox>

namespace
{
QString taskRequirementText(const Task &task)
{
    QStringList parts;
    if (task.requiresFire)
        parts << QObject::tr("开火");
    if (task.requiresHit)
        parts << QObject::tr("命中");
    if (task.requiresDetection)
        parts << QObject::tr("探测");
    if (task.requiresJam)
        parts << QObject::tr("电磁干扰");
    return parts.join(QLatin1Char(','));
}
}

TaskManagerDialog::TaskManagerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("任务管理"));
    resize(720, 520);

    auto *mainLayout = new QVBoxLayout(this);

    auto *topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel(tr("飞机:"), this));
    m_aircraftCombo = new QComboBox(this);
    topRow->addWidget(m_aircraftCombo, 1);
    mainLayout->addLayout(topRow);

    connect(m_aircraftCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        loadCurrentAircraft();
    });

    auto *routeRow = new QHBoxLayout();
    auto *routeLabel = new QLabel(tr("航迹 (每行 x,y):"), this);
    routeRow->addWidget(routeLabel);
    m_speedSpin = new QDoubleSpinBox(this);
    m_speedSpin->setRange(0.1, 10.0);
    m_speedSpin->setSingleStep(0.1);
    m_speedSpin->setSuffix(tr(" s/格"));
    routeRow->addWidget(new QLabel(tr("速度:"), this));
    routeRow->addWidget(m_speedSpin);
    mainLayout->addLayout(routeRow);

    m_routeEdit = new QPlainTextEdit(this);
    m_routeEdit->setPlaceholderText(tr("例如: 0,0\n0,10\n10,10"));
    m_routeEdit->setMinimumHeight(120);
    mainLayout->addWidget(m_routeEdit);

    auto *applyRouteBtn = new QPushButton(tr("保存航迹与速度"), this);
    connect(applyRouteBtn, &QPushButton::clicked, this, [this]() {
        applyRouteChanges();
        applySpeedChanges();
    });
    mainLayout->addWidget(applyRouteBtn, 0, Qt::AlignRight);

    m_taskTable = new QTableWidget(this);
    m_taskTable->setColumnCount(6);
    m_taskTable->setHorizontalHeaderLabels({tr("任务"), tr("执行时间"), tr("目标"), tr("要求"), tr("规则"), tr("状态")});
    m_taskTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_taskTable, 1);

    auto *taskButtons = new QHBoxLayout();
    auto *addBtn = new QPushButton(tr("新增任务"), this);
    auto *editBtn = new QPushButton(tr("编辑任务"), this);
    auto *removeBtn = new QPushButton(tr("删除任务"), this);
    taskButtons->addWidget(addBtn);
    taskButtons->addWidget(editBtn);
    taskButtons->addWidget(removeBtn);
    taskButtons->addStretch();
    mainLayout->addLayout(taskButtons);

    connect(addBtn, &QPushButton::clicked, this, &TaskManagerDialog::addTask);
    connect(editBtn, &QPushButton::clicked, this, &TaskManagerDialog::editSelectedTask);
    connect(removeBtn, &QPushButton::clicked, this, &TaskManagerDialog::removeSelectedTask);

    auto *closeButtons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(closeButtons, &QDialogButtonBox::rejected, this, &TaskManagerDialog::reject);
    mainLayout->addWidget(closeButtons);
}

void TaskManagerDialog::setAircrafts(QVector<Aircraft> *aircrafts)
{
    m_aircrafts = aircrafts;
    refreshAircraftCombo();
}

void TaskManagerDialog::setAvailableRuleNames(const QStringList &rules)
{
    m_ruleNames = rules;
}

void TaskManagerDialog::refreshAircraftCombo()
{
    m_aircraftCombo->blockSignals(true);
    m_aircraftCombo->clear();
    if (m_aircrafts)
    {
        for (const Aircraft &ac : *m_aircrafts)
        {
            m_aircraftCombo->addItem(ac.name);
        }
    }
    m_aircraftCombo->blockSignals(false);
    loadCurrentAircraft();
}

Aircraft *TaskManagerDialog::currentAircraft()
{
    if (!m_aircrafts)
    {
        return nullptr;
    }
    const int idx = m_aircraftCombo->currentIndex();
    if (idx < 0 || idx >= m_aircrafts->size())
    {
        return nullptr;
    }
    return &(*m_aircrafts)[idx];
}

void TaskManagerDialog::loadCurrentAircraft()
{
    Aircraft *ac = currentAircraft();
    if (!ac)
    {
        m_routeEdit->clear();
        m_speedSpin->setValue(1.0);
        m_taskTable->setRowCount(0);
        return;
    }

    QStringList lines;
    for (const QPoint &pt : ac->route)
    {
        lines << QStringLiteral("%1,%2").arg(pt.x()).arg(pt.y());
    }
    m_routeEdit->setPlainText(lines.join(QLatin1Char('\n')));
    m_speedSpin->setValue(ac->secondsPerStep);
    populateTaskTable(*ac);
}

void TaskManagerDialog::populateTaskTable(const Aircraft &aircraft)
{
    m_taskTable->setRowCount(aircraft.tasks.size());
    for (int row = 0; row < aircraft.tasks.size(); ++row)
    {
        const Task &task = aircraft.tasks.at(row);
        auto setItem = [&](int column, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setData(Qt::UserRole, row);
            m_taskTable->setItem(row, column, item);
        };
        setItem(0, task.name);
        setItem(1, QString::number(task.executionTime));
        setItem(2, QStringLiteral("(%1,%2)").arg(task.targetCell.x()).arg(task.targetCell.y()));
        setItem(3, taskRequirementText(task));
        setItem(4, task.ruleName);
        setItem(5, task.statusText());
    }
}

void TaskManagerDialog::applyRouteChanges()
{
    Aircraft *ac = currentAircraft();
    if (!ac)
    {
        return;
    }

    const QStringList lines = m_routeEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
    QVector<QPoint> newRoute;
    newRoute.reserve(lines.size());
    for (const QString &line : lines)
    {
        const QStringList parts = line.split(',', Qt::SkipEmptyParts);
        if (parts.size() != 2)
        {
            QMessageBox::warning(this, tr("格式错误"), tr("航迹行必须为 x,y"));
            return;
        }
        bool okX = false;
        bool okY = false;
        int x = parts.at(0).trimmed().toInt(&okX);
        int y = parts.at(1).trimmed().toInt(&okY);
        if (!okX || !okY || x < 0 || x >= EnvironmentGridWidget::GridSize || y < 0 || y >= EnvironmentGridWidget::GridSize)
        {
            QMessageBox::warning(this, tr("范围错误"), tr("坐标需在 0-%1 之间").arg(EnvironmentGridWidget::GridSize - 1));
            return;
        }
        newRoute.append(QPoint(x, y));
    }

    if (!newRoute.isEmpty())
    {
        ac->route = newRoute;
        ac->currentRouteIndex = 0;
        ac->stepAccumulator = 0.0;
    }
}

void TaskManagerDialog::applySpeedChanges()
{
    Aircraft *ac = currentAircraft();
    if (!ac)
    {
        return;
    }
    ac->secondsPerStep = m_speedSpin->value();
}

Task *TaskManagerDialog::taskFromRow(int row)
{
    Aircraft *ac = currentAircraft();
    if (!ac || row < 0 || row >= ac->tasks.size())
    {
        return nullptr;
    }
    return &ac->tasks[row];
}

void TaskManagerDialog::addTask()
{
    Aircraft *ac = currentAircraft();
    if (!ac)
    {
        return;
    }

    Task task;
    task.name = tr("新任务%1").arg(ac->tasks.size() + 1);
    task.ruleName = !m_ruleNames.isEmpty() ? m_ruleNames.first() : QString();
    if (editTask(task, true))
    {
        ac->tasks.append(task);
        populateTaskTable(*ac);
    }
}

void TaskManagerDialog::editSelectedTask()
{
    QTableWidgetItem *item = m_taskTable->currentItem();
    if (!item)
    {
        return;
    }
    const int row = item->row();
    Task *task = taskFromRow(row);
    if (!task)
    {
        return;
    }

    Task copy = *task;
    if (editTask(copy, false))
    {
        *task = copy;
        task->status = TaskStatus::Pending;
        populateTaskTable(*currentAircraft());
    }
}

void TaskManagerDialog::removeSelectedTask()
{
    Aircraft *ac = currentAircraft();
    if (!ac)
    {
        return;
    }
    QTableWidgetItem *item = m_taskTable->currentItem();
    if (!item)
    {
        return;
    }
    const int row = item->row();
    if (row < 0 || row >= ac->tasks.size())
    {
        return;
    }
    ac->tasks.removeAt(row);
    populateTaskTable(*ac);
}

bool TaskManagerDialog::editTask(Task &task, bool isNew)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isNew ? tr("新增任务") : tr("编辑任务"));
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(task.name, &dialog);
    layout->addRow(tr("名称"), nameEdit);

    auto *timeSpin = new QSpinBox(&dialog);
    timeSpin->setRange(0, 3600);
    timeSpin->setValue(task.executionTime);
    layout->addRow(tr("执行时间(s)"), timeSpin);

    auto *targetRow = new QWidget(&dialog);
    auto *targetLayout = new QHBoxLayout(targetRow);
    targetLayout->setContentsMargins(0, 0, 0, 0);
    auto *xSpin = new QSpinBox(&dialog);
    xSpin->setRange(0, EnvironmentGridWidget::GridSize - 1);
    xSpin->setValue(task.targetCell.x());
    auto *ySpin = new QSpinBox(&dialog);
    ySpin->setRange(0, EnvironmentGridWidget::GridSize - 1);
    ySpin->setValue(task.targetCell.y());
    targetLayout->addWidget(new QLabel("X", &dialog));
    targetLayout->addWidget(xSpin);
    targetLayout->addWidget(new QLabel("Y", &dialog));
    targetLayout->addWidget(ySpin);
    layout->addRow(tr("目标格"), targetRow);

    auto *fireCheck = new QCheckBox(tr("需要开火"), &dialog);
    fireCheck->setChecked(task.requiresFire);
    auto *hitCheck = new QCheckBox(tr("需要命中"), &dialog);
    hitCheck->setChecked(task.requiresHit);
    auto *detectCheck = new QCheckBox(tr("需要探测"), &dialog);
    detectCheck->setChecked(task.requiresDetection);
    auto *jamCheck = new QCheckBox(tr("需要电磁干扰"), &dialog);
    jamCheck->setChecked(task.requiresJam);

    layout->addRow(fireCheck);
    layout->addRow(hitCheck);
    layout->addRow(detectCheck);
    layout->addRow(jamCheck);

    auto *ruleCombo = new QComboBox(&dialog);
    ruleCombo->addItems(m_ruleNames);
    int ruleIndex = m_ruleNames.indexOf(task.ruleName);
    if (ruleIndex >= 0)
    {
        ruleCombo->setCurrentIndex(ruleIndex);
    }
    layout->addRow(tr("裁决规则"), ruleCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        task.name = nameEdit->text();
        task.executionTime = timeSpin->value();
        task.targetCell = {xSpin->value(), ySpin->value()};
        task.requiresFire = fireCheck->isChecked();
        task.requiresHit = hitCheck->isChecked();
        task.requiresDetection = detectCheck->isChecked();
        task.requiresJam = jamCheck->isChecked();
        task.ruleName = ruleCombo->currentText();
        task.status = TaskStatus::Pending;
        return true;
    }
    return false;
}
