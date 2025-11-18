#pragma once

#include <QDialog>
#include <QVector>

#include "models.h"

class QTableWidget;

class RuleModelManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RuleModelManagerDialog(QWidget *parent = nullptr);

    void setData(QVector<AdjudicationRule> *rules, QVector<AdjudicationModel> *models);

private:
    void setupRulePanel(QWidget *parent);
    void setupModelPanel(QWidget *parent);

    void refreshRuleTable();
    void refreshModelTable();

    void addRule();
    void editSelectedRule();
    void removeSelectedRule();

    void addModel();
    void editSelectedModel();
    void removeSelectedModel();

    bool editRule(AdjudicationRule &rule, bool isNew);
    bool editModel(AdjudicationModel &model, bool isNew);

    QVector<AdjudicationRule> *m_rules = nullptr;
    QVector<AdjudicationModel> *m_models = nullptr;

    QTableWidget *m_ruleTable = nullptr;
    QTableWidget *m_modelTable = nullptr;
};
