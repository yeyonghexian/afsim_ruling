#include "environmentgridwidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QLabel>

namespace
{
constexpr int kCellPadding = 1;
}

EnvironmentGridWidget::EnvironmentGridWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(400, 400);
}

QSize EnvironmentGridWidget::sizeHint() const
{
    return {640, 640};
}

EnvironmentFactors EnvironmentGridWidget::factorsAt(const QPoint &cell) const
{
    return m_cellFactors.value(cellIndex(cell), EnvironmentFactors{});
}

void EnvironmentGridWidget::setFactorsAt(const QPoint &cell, const EnvironmentFactors &factors)
{
    m_cellFactors.insert(cellIndex(cell), factors);
    update();
    emit cellFactorsChanged(cell, factors);
}

void EnvironmentGridWidget::setAircrafts(const QVector<Aircraft> *aircrafts)
{
    m_aircrafts = aircrafts;
    update();
}

void EnvironmentGridWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(18, 27, 39));

    const int cellSize = qMin(width(), height()) / GridSize;
    const int boardSize = cellSize * GridSize;
    const QPoint origin((width() - boardSize) / 2, (height() - boardSize) / 2);
    const QRect boardRect(origin, QSize(boardSize, boardSize));

    painter.setPen(QPen(QColor(70, 90, 110)));
    painter.drawRect(boardRect);

    painter.save();
    painter.setClipRect(boardRect);

    for (int y = 0; y < GridSize; ++y)
    {
        for (int x = 0; x < GridSize; ++x)
        {
            const QRect cellR = cellRect({x, y});
            const EnvironmentFactors factors = factorsAt({x, y});
            const double avg = (factors.oceanDepth + factors.airDryness + factors.emInterference + factors.temperature + factors.humidity) / 500.0;
            QColor fill = QColor::fromHsvF(0.55 - avg * 0.25, 0.35 + avg * 0.25, 0.4 + avg * 0.5, 0.6);
            painter.fillRect(cellR.adjusted(kCellPadding, kCellPadding, -kCellPadding, -kCellPadding), fill);
        }
    }

    painter.setPen(QColor(45, 60, 80));
    for (int i = 1; i < GridSize; ++i)
    {
        const int x = origin.x() + i * cellSize;
        const int y = origin.y() + i * cellSize;
        painter.drawLine(x, origin.y(), x, origin.y() + boardSize);
        painter.drawLine(origin.x(), y, origin.x() + boardSize, y);
    }

    if (m_aircrafts)
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        int hueStep = 360 / qMax(1, m_aircrafts->size());
        for (int idx = 0; idx < m_aircrafts->size(); ++idx)
        {
            const Aircraft &ac = m_aircrafts->at(idx);
            QColor color = QColor::fromHsv((idx * hueStep) % 360, 200, 255);
            painter.setPen(QPen(color, 2));

            for (int p = 0; p + 1 < ac.route.size(); ++p)
            {
                const QPointF start = cellRect(ac.route.at(p)).center();
                const QPointF end = cellRect(ac.route.at(p + 1)).center();
                painter.drawLine(start, end);
            }

            const QPointF pos = cellRect(ac.position()).center();
            painter.setBrush(color);
            painter.drawEllipse(pos, cellSize * 0.3, cellSize * 0.3);
            painter.drawText(pos + QPointF(6, -6), ac.name);
        }
    }

    painter.restore();
}

void EnvironmentGridWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    const QPoint cell = cellForPosition(event->pos());
    if (cell.x() < 0 || cell.y() < 0)
    {
        return;
    }

    EnvironmentFactors factors = factorsAt(cell);
    if (editFactors(QStringLiteral("设置环境因子 (%1, %2)").arg(cell.x()).arg(cell.y()), factors))
    {
        setFactorsAt(cell, factors);
    }
}

QRect EnvironmentGridWidget::cellRect(const QPoint &cell) const
{
    const int cellSize = qMin(width(), height()) / GridSize;
    const int boardSize = cellSize * GridSize;
    const QPoint origin((width() - boardSize) / 2, (height() - boardSize) / 2);

    return QRect(origin.x() + cell.x() * cellSize,
                 origin.y() + cell.y() * cellSize,
                 cellSize,
                 cellSize);
}

QPoint EnvironmentGridWidget::cellForPosition(const QPoint &pos) const
{
    const int cellSize = qMin(width(), height()) / GridSize;
    if (cellSize == 0)
    {
        return {-1, -1};
    }

    const int boardSize = cellSize * GridSize;
    const QPoint origin((width() - boardSize) / 2, (height() - boardSize) / 2);
    if (!QRect(origin, QSize(boardSize, boardSize)).contains(pos))
    {
        return {-1, -1};
    }

    return {(pos.x() - origin.x()) / cellSize, (pos.y() - origin.y()) / cellSize};
}

bool EnvironmentGridWidget::editFactors(QString title, EnvironmentFactors &factors) const
{
    QDialog dialog(const_cast<EnvironmentGridWidget *>(this));
    dialog.setWindowTitle(std::move(title));

    auto *layout = new QFormLayout(&dialog);

    auto spin = [&](const QString &label, int value) {
        auto *box = new QSpinBox(&dialog);
        box->setRange(0, 100);
        box->setValue(value);
        layout->addRow(new QLabel(label, &dialog), box);
        return box;
    };

    QSpinBox *ocean = spin(QStringLiteral("海洋深度"), factors.oceanDepth);
    QSpinBox *air = spin(QStringLiteral("空气干燥度"), factors.airDryness);
    QSpinBox *em = spin(QStringLiteral("电磁干扰系数"), factors.emInterference);
    QSpinBox *temp = spin(QStringLiteral("气温"), factors.temperature);
    QSpinBox *hum = spin(QStringLiteral("湿度"), factors.humidity);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        factors.oceanDepth = ocean->value();
        factors.airDryness = air->value();
        factors.emInterference = em->value();
        factors.temperature = temp->value();
        factors.humidity = hum->value();
        return true;
    }
    return false;
}

int EnvironmentGridWidget::cellIndex(const QPoint &cell) const
{
    return cell.y() * GridSize + cell.x();
}
