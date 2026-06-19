#include "ui/chart_widget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QChart>
#include <QLineSeries>
#include <QLegend>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>
#include <algorithm>
#include <limits>
#include <chrono>
#include "mavlink_c_library_v2/common/mavlink.h"

ChartViewWithPan::ChartViewWithPan(QChart* chart, QWidget* parent)
    : QChartView(chart, parent)
{
}

void ChartViewWithPan::setOwner(ChartWidget* ownerWidget) {
    owner = ownerWidget;
}

void ChartViewWithPan::wheelEvent(QWheelEvent* event) {
    if (owner) {
        owner->handleWheelZoom(event);
        event->accept();
    } else {
        QChartView::wheelEvent(event);
    }
}

void ChartViewWithPan::mousePressEvent(QMouseEvent* event) {
    QChartView::mousePressEvent(event);
    if (event->isAccepted()) {
        return;
    }
    if (owner && event->button() == Qt::LeftButton) {
        owner->handlePanStarted(event->pos());
        event->accept();
        return;
    }
}

void ChartViewWithPan::mouseMoveEvent(QMouseEvent* event) {
    if (owner && owner->panning) {
        owner->handlePanMoved(event->pos());
        event->accept();
        return;
    }
    QChartView::mouseMoveEvent(event);
}

void ChartViewWithPan::mouseReleaseEvent(QMouseEvent* event) {
    if (owner && event->button() == Qt::LeftButton) {
        owner->handlePanStopped();
        event->accept();
        return;
    }
    QChartView::mouseReleaseEvent(event);
}

ChartWidget::ChartWidget(HistoryManager* history, QWidget* parent) 
    : QWidget(parent), historyManager(history)
{
    const auto& cfg = getAppConfig();
    timeWindowSeconds = cfg.chart.window_seconds;
    refreshMs = cfg.chart.update_ms;
    liveRestoreMs = cfg.chart.live_restore_ms;
    auto* layout = new QVBoxLayout(this);

    chart = new QChart();
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chartView = new ChartViewWithPan(chart, this);
    chartView->setOwner(this);
    chartView->setRenderHint(QPainter::Antialiasing, false);
    chartView->setRubberBand(QChartView::NoRubberBand);
    chartView->setMouseTracking(true);
    if (chartView->viewport()) {
        chartView->viewport()->setMouseTracking(true);
    }

    axisX = new QValueAxis();
    axisY = new QValueAxis();
    axisX->setTitleText("Секунды назад");
    axisY->setTitleText("Значение");
    axisX->setRange(0, timeWindowSeconds);
    axisX->setLabelFormat("%.0f");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    // Создаём серии
    seriesAltitude = new QLineSeries();
    seriesAltitude->setName("Высота (м)");
    seriesAltitude->setUseOpenGL(true);
    seriesSpeed = new QLineSeries();
    seriesSpeed->setName("Скорость (м/с)");
    seriesSpeed->setUseOpenGL(true);
    seriesVoltage = new QLineSeries();
    seriesVoltage->setName("Напряжение (В)");
    seriesVoltage->setUseOpenGL(true);
    seriesHeading = new QLineSeries();
    seriesHeading->setName("Курс (°)");
    seriesHeading->setUseOpenGL(true);

    chart->addSeries(seriesAltitude);
    chart->addSeries(seriesSpeed);
    chart->addSeries(seriesVoltage);
    chart->addSeries(seriesHeading);

    seriesMap[{MAVLINK_MSG_ID_GLOBAL_POSITION_INT, "relative_alt"}] = seriesAltitude;
    seriesMap[{MAVLINK_MSG_ID_VFR_HUD, "groundspeed"}] = seriesSpeed;
    seriesMap[{MAVLINK_MSG_ID_SYS_STATUS, "voltage_battery"}] = seriesVoltage;
    seriesMap[{MAVLINK_MSG_ID_VFR_HUD, "heading"}] = seriesHeading;

    visibleSeriesKeys = {
        {MAVLINK_MSG_ID_GLOBAL_POSITION_INT, "relative_alt"},
        {MAVLINK_MSG_ID_VFR_HUD, "groundspeed"},
        {MAVLINK_MSG_ID_SYS_STATUS, "voltage_battery"},
        {MAVLINK_MSG_ID_VFR_HUD, "heading"}
    };
    setVisibleSeries(visibleSeriesKeys);

    // Привязка осей
    seriesAltitude->attachAxis(axisX);
    seriesAltitude->attachAxis(axisY);
    seriesSpeed->attachAxis(axisX);
    seriesSpeed->attachAxis(axisY);
    seriesVoltage->attachAxis(axisX);
    seriesVoltage->attachAxis(axisY);
    seriesHeading->attachAxis(axisX);
    seriesHeading->attachAxis(axisY);

    for (auto marker : chart->legend()->markers()) {
        QObject::connect(marker, &QLegendMarker::clicked, this, [this, marker]() {
            handleLegendMarkerClicked(marker);
        });
    }

    layout->addWidget(chartView);

    // Таймер обновления
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &ChartWidget::updateCharts);
    updateTimer->start(refreshMs);

    liveRestoreTimer = new QTimer(this);
    liveRestoreTimer->setSingleShot(true);
    connect(liveRestoreTimer, &QTimer::timeout, this, [this]() {
        userInteracted = false;
        updateCharts();
    });
}

void ChartWidget::handleWheelZoom(QWheelEvent* we) {
    const QPointF pf = we->position();
    const QPoint p = pf.toPoint();
    const double range = axisX->max() - axisX->min();
    if (range > 0 && chartView->width() > 0) {
        const double delta = we->angleDelta().y();
        const double factor = std::pow(0.9, delta / 120.0);
        const double rel = static_cast<double>(p.x()) / chartView->width();
        const double center = axisX->min() + rel * range;
        const double newRange = std::max(0.001, range * factor);
        double newMin = center - rel * newRange;
        double newMax = newMin + newRange;
        axisX->setRange(newMin, newMax);
        userInteracted = true;
        if (liveRestoreTimer && liveRestoreMs > 0) liveRestoreTimer->start(liveRestoreMs);
    }
}

void ChartWidget::handlePanStarted(const QPoint& startPos) {
    panning = true;
    panStartGlobalPos = startPos;
    panStartMin = axisX->min();
    panStartMax = axisX->max();
    userInteracted = true;
    if (liveRestoreTimer && liveRestoreMs > 0) liveRestoreTimer->start(liveRestoreMs);
}

void ChartWidget::handlePanMoved(const QPoint& pos) {
    if (!panning) {
        return;
    }
    const double range = panStartMax - panStartMin;
    const int viewWidth = chartView->viewport() ? chartView->viewport()->width() : chartView->width();
    if (viewWidth > 0 && range > 0) {
        const double dx = static_cast<double>(pos.x() - panStartGlobalPos.x());
        const double delta = dx * range / static_cast<double>(viewWidth);
        axisX->setRange(panStartMin - delta, panStartMax - delta);
        userInteracted = true;
        if (liveRestoreTimer && liveRestoreMs > 0) liveRestoreTimer->start(liveRestoreMs);
    }
}

void ChartWidget::handlePanStopped() {
    panning = false;
}

void ChartWidget::handleLegendMarkerClicked(QLegendMarker* marker) {
    if (!marker || !marker->series()) {
        return;
    }

    const bool visible = marker->series()->isVisible();
    marker->series()->setVisible(!visible);
    marker->setVisible(true);
    visibleSeriesKeys.clear();
    for (const auto& entry : seriesMap) {
        if (entry.second && entry.second->isVisible()) {
            visibleSeriesKeys.push_back(entry.first);
        }
    }
}

std::vector<SeriesKey> ChartWidget::getAvailableSeries() const {
    if (!historyManager) {
        return {};
    }
    return historyManager->getAvailableSeries();
}

void ChartWidget::setVisibleSeries(const std::vector<SeriesKey>& series) {
    visibleSeriesKeys = series;
    for (const auto& entry : seriesMap) {
        const bool shouldShow = std::find(series.begin(), series.end(), entry.first) != series.end();
        if (entry.second) {
            entry.second->setVisible(shouldShow);
        }
    }
}

std::vector<SeriesKey> ChartWidget::getVisibleSeries() const {
    std::vector<SeriesKey> visible;
    for (const auto& entry : seriesMap) {
        if (entry.second && entry.second->isVisible()) {
            visible.push_back(entry.first);
        }
    }
    return visible;
}

std::vector<TelemetryPoint> ChartWidget::getSeriesData(const SeriesKey& key, TimePoint start, TimePoint end) const {
    if (!historyManager) {
        return {};
    }
    return historyManager->getData(key, start, end);
}

// Override wheelEvent via eventFilter: zoom the X axis
// Note: we still handle wheel via eventFilter because chartView installed it
// and we're intercepting events there.
// Zoom centered on cursor


void ChartWidget::updateCharts() {
    if (!historyManager) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto start = now - std::chrono::seconds(timeWindowSeconds);

    struct SeriesConfig {
        QLineSeries* series;
        uint32_t msgid;
        std::string param;
    } configs[] = {
        {seriesAltitude, MAVLINK_MSG_ID_GLOBAL_POSITION_INT, "relative_alt"},
        {seriesSpeed, MAVLINK_MSG_ID_VFR_HUD, "groundspeed"},
        {seriesVoltage, MAVLINK_MSG_ID_SYS_STATUS, "voltage_battery"},
        {seriesHeading, MAVLINK_MSG_ID_VFR_HUD, "heading"}
    };

    double minValue = std::numeric_limits<double>::infinity();
    double maxValue = -std::numeric_limits<double>::infinity();
    bool hasData = false;

    for (const auto& cfg : configs) {
        cfg.series->clear();
        const SeriesKey key{cfg.msgid, cfg.param};
        const auto points = historyManager->getData(key, start, now);
        for (const auto& point : points) {
            const double elapsed = std::chrono::duration<double>(point.timestamp - start).count();
            // Invert X so that time=0 is on the right and max time is on the left
            const double x = static_cast<double>(timeWindowSeconds) - elapsed;
            cfg.series->append(x, point.value);
            minValue = std::min(minValue, point.value);
            maxValue = std::max(maxValue, point.value);
            hasData = true;
        }
    }

    if (hasData) {
        const double padding = std::max(0.1, (maxValue - minValue) * 0.1);
        double lower = minValue - padding;
        if (lower < 0.0) lower = 0.0; // ensure Y starts at 0 and not negative
        axisY->setRange(lower, maxValue + padding);
    } else {
        axisY->setRange(0, 1);
    }

    if (!userInteracted) { axisX->setRange(0, timeWindowSeconds); }
    qDebug() << "Chart update:" << (hasData ? "data updated" : "no data");
}
