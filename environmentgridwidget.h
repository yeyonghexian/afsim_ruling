#pragma once

#include <QWidget>
#include <QHash>
#include <QVector>

#include "models.h"

class EnvironmentGridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EnvironmentGridWidget(QWidget *parent = nullptr);

    static constexpr int GridSize = 50;

    QSize sizeHint() const override;
    EnvironmentFactors factorsAt(const QPoint &cell) const;
    void setFactorsAt(const QPoint &cell, const EnvironmentFactors &factors);

    void setAircrafts(const QVector<Aircraft> *aircrafts);

signals:
    void cellFactorsChanged(const QPoint &cell, const EnvironmentFactors &factors);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QRect cellRect(const QPoint &cell) const;
    QPoint cellForPosition(const QPoint &pos) const;
    bool editFactors(QString title, EnvironmentFactors &factors) const;

    int cellIndex(const QPoint &cell) const;

    QHash<int, EnvironmentFactors> m_cellFactors;
    const QVector<Aircraft> *m_aircrafts = nullptr;
};
