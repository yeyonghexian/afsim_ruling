#include "manualadjudicationdialog.h"

#include <QVBoxLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

ManualAdjudicationDialog::ManualAdjudicationDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("人工裁决"));
    auto *layout = new QVBoxLayout(this);

    m_hintLabel = new QLabel(this);
    m_hintLabel->setWordWrap(true);
    layout->addWidget(m_hintLabel);

    m_fireAllowedCheck = new QCheckBox("允许开火", this);
    m_fireHitCheck = new QCheckBox("命中目标", this);
    m_detectionCheck = new QCheckBox("探测成功", this);
    m_jamCheck = new QCheckBox("电磁干扰成功", this);

    layout->addWidget(m_fireAllowedCheck);
    layout->addWidget(m_fireHitCheck);
    layout->addWidget(m_detectionCheck);
    layout->addWidget(m_jamCheck);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ManualAdjudicationDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ManualAdjudicationDialog::reject);
    layout->addWidget(buttons);
}

void ManualAdjudicationDialog::setContext(const QString &title, const Task &task, const ManualAdjudicationState &state)
{
    setWindowTitle(title);

    QStringList summary;
    summary << tr("任务: %1").arg(task.name);
    summary << tr("时间: %1 s").arg(task.executionTime);
    summary << tr("目标: (%1, %2)").arg(task.targetCell.x()).arg(task.targetCell.y());

    QStringList requirements;
    if (task.requiresFire)
        requirements << tr("开火");
    if (task.requiresHit)
        requirements << tr("命中");
    if (task.requiresDetection)
        requirements << tr("探测");
    if (task.requiresJam)
        requirements << tr("电磁干扰");

    if (!requirements.isEmpty())
    {
        summary << tr("裁决环节: %1").arg(requirements.join(QLatin1Char(',')));
    }

    m_hintLabel->setText(summary.join(QStringLiteral("\n")));

    m_fireAllowedCheck->setChecked(state.fireAllowed);
    m_fireHitCheck->setChecked(state.fireHit);
    m_detectionCheck->setChecked(state.detectionSuccess);
    m_jamCheck->setChecked(state.jamSuccess);

    m_fireAllowedCheck->setVisible(task.requiresFire);
    m_fireHitCheck->setVisible(task.requiresHit);
    m_detectionCheck->setVisible(task.requiresDetection);
    m_jamCheck->setVisible(task.requiresJam);
}

ManualAdjudicationState ManualAdjudicationDialog::state() const
{
    ManualAdjudicationState s;
    s.fireAllowed = m_fireAllowedCheck->isChecked();
    s.fireHit = m_fireHitCheck->isChecked();
    s.detectionSuccess = m_detectionCheck->isChecked();
    s.jamSuccess = m_jamCheck->isChecked();
    return s;
}
