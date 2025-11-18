#include "rulemodelmanagerdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>

RuleModelManagerDialog::RuleModelManagerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("规则与模型管理"));
    resize(800, 500);

    auto *layout = new QHBoxLayout(this);
    auto *rulePanel = new QWidget(this);
    auto *modelPanel = new QWidget(this);
    layout->addWidget(rulePanel, 1);
    layout->addWidget(modelPanel, 1);

    setupRulePanel(rulePanel);
    setupModelPanel(modelPanel);
}

void RuleModelManagerDialog::setData(QVector<AdjudicationRule> *rules, QVector<AdjudicationModel> *models)
{
    m_rules = rules;
    m_models = models;
    refreshRuleTable();
    refreshModelTable();
}

void RuleModelManagerDialog::setupRulePanel(QWidget *parent)
{
    auto *layout = new QVBoxLayout(parent);
    layout->addWidget(new QLabel(QStringLiteral("裁决规则"), parent));

    m_ruleTable = new QTableWidget(parent);
    m_ruleTable->setColumnCount(3);
    m_ruleTable->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("阈值"), QStringLiteral("权重")});
    m_ruleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ruleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ruleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_ruleTable, 1);

    auto *buttons = new QHBoxLayout();
    auto *addBtn = new QPushButton(QStringLiteral("新增"), parent);
    auto *editBtn = new QPushButton(QStringLiteral("编辑"), parent);
    auto *removeBtn = new QPushButton(QStringLiteral("删除"), parent);
    buttons->addWidget(addBtn);
    buttons->addWidget(editBtn);
    buttons->addWidget(removeBtn);
    buttons->addStretch();
    layout->addLayout(buttons);

    connect(addBtn, &QPushButton::clicked, this, &RuleModelManagerDialog::addRule);
    connect(editBtn, &QPushButton::clicked, this, &RuleModelManagerDialog::editSelectedRule);
    connect(removeBtn, &QPushButton::clicked, this, &RuleModelManagerDialog::removeSelectedRule);
}

void RuleModelManagerDialog::setupModelPanel(QWidget *parent)
{
    auto *layout = new QVBoxLayout(parent);
    layout->addWidget(new QLabel(QStringLiteral("裁决模型"), parent));

    m_modelTable = new QTableWidget(parent);
    m_modelTable->setColumnCount(3);
    m_modelTable->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("环境因子"), QStringLiteral("环境权重")});
    m_modelTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_modelTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_modelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_modelTable, 1);

    auto *buttons = new QHBoxLayout();
    auto *addBtn = new QPushButton(QStringLiteral("新增"), parent);
    auto *editBtn = new QPushButton(QStringLiteral("编辑"), parent);
    auto *removeBtn = new QPushButton(QStringLiteral("删除"), parent);
    buttons->addWidget(addBtn);
    buttons->addWidget(editBtn);
    buttons->addWidget(removeBtn);
    buttons->addStretch();
    layout->addLayout(buttons);

    connect(addBtn, &QPushButton::clicked, this, &RuleModelManagerDialog::addModel);
    connect(editBtn, &QPushButton::clicked, this, &RuleModelManagerDialog::editSelectedModel);
    connect(removeBtn, &QPushButton::clicked, this, &RuleModelManagerDialog::removeSelectedModel);
}

void RuleModelManagerDialog::refreshRuleTable()
{
    m_ruleTable->setRowCount(m_rules ? m_rules->size() : 0);
    if (!m_rules)
        return;

    for (int row = 0; row < m_rules->size(); ++row)
    {
        const AdjudicationRule &rule = m_rules->at(row);
        m_ruleTable->setItem(row, 0, new QTableWidgetItem(rule.name));
        m_ruleTable->setItem(row, 1, new QTableWidgetItem(QString::number(rule.successThreshold)));
        QStringList weights;
        for (auto it = rule.behaviorWeights.cbegin(); it != rule.behaviorWeights.cend(); ++it)
        {
            weights << QStringLiteral("%1:%2").arg(it.key()).arg(it.value());
        }
        m_ruleTable->setItem(row, 2, new QTableWidgetItem(weights.join(QLatin1Char(','))));
    }
}

void RuleModelManagerDialog::refreshModelTable()
{
    m_modelTable->setRowCount(m_models ? m_models->size() : 0);
    if (!m_models)
        return;

    for (int row = 0; row < m_models->size(); ++row)
    {
        const AdjudicationModel &model = m_models->at(row);
        m_modelTable->setItem(row, 0, new QTableWidgetItem(model.name));
        m_modelTable->setItem(row, 1, new QTableWidgetItem(model.factorKeys.join(QLatin1Char(','))));
        m_modelTable->setItem(row, 2, new QTableWidgetItem(QString::number(model.environmentWeight, 'f', 2)));
    }
}

void RuleModelManagerDialog::addRule()
{
    if (!m_rules)
        return;

    AdjudicationRule rule;
    rule.name = QStringLiteral("规则%1").arg(m_rules->size() + 1);
    if (editRule(rule, true))
    {
        m_rules->append(rule);
        refreshRuleTable();
    }
}

void RuleModelManagerDialog::editSelectedRule()
{
    if (!m_rules)
        return;
    const int row = m_ruleTable->currentRow();
    if (row < 0 || row >= m_rules->size())
        return;

    AdjudicationRule copy = m_rules->at(row);
    if (editRule(copy, false))
    {
        (*m_rules)[row] = copy;
        refreshRuleTable();
    }
}

void RuleModelManagerDialog::removeSelectedRule()
{
    if (!m_rules)
        return;
    const int row = m_ruleTable->currentRow();
    if (row < 0 || row >= m_rules->size())
        return;
    m_rules->removeAt(row);
    refreshRuleTable();
}

void RuleModelManagerDialog::addModel()
{
    if (!m_models)
        return;
    AdjudicationModel model;
    model.name = QStringLiteral("模型%1").arg(m_models->size() + 1);
    if (editModel(model, true))
    {
        m_models->append(model);
        refreshModelTable();
    }
}

void RuleModelManagerDialog::editSelectedModel()
{
    if (!m_models)
        return;
    const int row = m_modelTable->currentRow();
    if (row < 0 || row >= m_models->size())
        return;

    AdjudicationModel copy = m_models->at(row);
    if (editModel(copy, false))
    {
        (*m_models)[row] = copy;
        refreshModelTable();
    }
}

void RuleModelManagerDialog::removeSelectedModel()
{
    if (!m_models)
        return;
    const int row = m_modelTable->currentRow();
    if (row < 0 || row >= m_models->size())
        return;
    m_models->removeAt(row);
    refreshModelTable();
}

bool RuleModelManagerDialog::editRule(AdjudicationRule &rule, bool isNew)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isNew ? QStringLiteral("新增规则") : QStringLiteral("编辑规则"));
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(rule.name, &dialog);
    layout->addRow(QStringLiteral("名称"), nameEdit);

    auto *thresholdSpin = new QSpinBox(&dialog);
    thresholdSpin->setRange(0, 1000);
    thresholdSpin->setValue(rule.successThreshold);
    layout->addRow(QStringLiteral("成功阈值"), thresholdSpin);

    auto *weightEdit = new QLineEdit(&dialog);
    QStringList weightParts;
    for (auto it = rule.behaviorWeights.cbegin(); it != rule.behaviorWeights.cend(); ++it)
    {
        weightParts << QStringLiteral("%1:%2").arg(it.key()).arg(it.value());
    }
    weightEdit->setPlaceholderText(QStringLiteral("格式: fire:20,hit:30"));
    weightEdit->setText(weightParts.join(QLatin1Char(',')));
    layout->addRow(QStringLiteral("行为权重"), weightEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        rule.name = nameEdit->text();
        rule.successThreshold = thresholdSpin->value();
        rule.behaviorWeights.clear();
        const QStringList entries = weightEdit->text().split(',', Qt::SkipEmptyParts);
        for (const QString &entry : entries)
        {
            const QStringList kv = entry.split(':');
            if (kv.size() == 2)
            {
                rule.behaviorWeights.insert(kv.at(0).trimmed(), kv.at(1).trimmed().toInt());
            }
        }
        return true;
    }
    return false;
}

bool RuleModelManagerDialog::editModel(AdjudicationModel &model, bool isNew)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isNew ? QStringLiteral("新增模型") : QStringLiteral("编辑模型"));
    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit(model.name, &dialog);
    layout->addRow(QStringLiteral("名称"), nameEdit);

    auto *factorsEdit = new QLineEdit(model.factorKeys.join(QLatin1Char(',')), &dialog);
    factorsEdit->setPlaceholderText(QStringLiteral("例如: oceanDepth,airDryness"));
    layout->addRow(QStringLiteral("环境因子"), factorsEdit);

    auto *weightSpin = new QDoubleSpinBox(&dialog);
    weightSpin->setRange(0.0, 1.0);
    weightSpin->setSingleStep(0.05);
    weightSpin->setValue(model.environmentWeight);
    layout->addRow(QStringLiteral("环境权重"), weightSpin);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        model.name = nameEdit->text();
        model.factorKeys = factorsEdit->text().split(',', Qt::SkipEmptyParts);
        for (QString &key : model.factorKeys)
        {
            key = key.trimmed();
        }
        model.environmentWeight = weightSpin->value();
        return true;
    }
    return false;
}
