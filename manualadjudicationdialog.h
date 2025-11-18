#pragma once

#include <QDialog>

#include "models.h"

class QCheckBox;
class QLabel;

class ManualAdjudicationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ManualAdjudicationDialog(QWidget *parent = nullptr);

    void setContext(const QString &title, const Task &task, const ManualAdjudicationState &state);
    ManualAdjudicationState state() const;

private:
    QLabel *m_hintLabel = nullptr;
    QCheckBox *m_fireAllowedCheck = nullptr;
    QCheckBox *m_fireHitCheck = nullptr;
    QCheckBox *m_detectionCheck = nullptr;
    QCheckBox *m_jamCheck = nullptr;
};
