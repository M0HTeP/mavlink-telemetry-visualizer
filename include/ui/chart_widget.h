#pragma once
#include <QWidget>
#include <QChartView>
#include <QChart>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLineSeries>
#include <QTimer>
#include <QValueAxis>
#include <QLegendMarker>
#include <vector>
#include <unordered_map>
#include "history/history_manager.h"
#include "common/config.h"

class ChartViewWithPan : public QChartView {
public:
    explicit ChartViewWithPan(QChart* chart, QWidget* parent = nullptr);
    void setOwner(class ChartWidget* ownerWidget);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    class ChartWidget* owner = nullptr;
};

class ChartWidget : public QWidget {
    Q_OBJECT
    friend class ChartViewWithPan;
public:
    explicit ChartWidget(HistoryManager* history, QWidget* parent = nullptr);

    // API for UI
    std::vector<SeriesKey> getAvailableSeries() const;
    void setVisibleSeries(const std::vector<SeriesKey>& series);
    std::vector<SeriesKey> getVisibleSeries() const;
    std::vector<TelemetryPoint> getSeriesData(const SeriesKey& key, TimePoint start, TimePoint end) const;

private:
    void updateCharts();
    void handleLegendMarkerClicked(QLegendMarker* marker);
    void handleWheelZoom(QWheelEvent* event);
    void handlePanStarted(const QPoint& startPos);
    void handlePanMoved(const QPoint& pos);
    void handlePanStopped();

private:
    HistoryManager* historyManager = nullptr;
    ChartViewWithPan* chartView = nullptr;
    QChart* chart = nullptr;
    QValueAxis* axisX = nullptr;
    QValueAxis* axisY = nullptr;

    QTimer* updateTimer = nullptr;
    bool userInteracted = false;
    int timeWindowSeconds;
    int refreshMs;

    bool panning = false;
    QPoint panStartGlobalPos;
    double panStartMin = 0.0;
    double panStartMax = 0.0;
    int liveRestoreMs;
    QTimer* liveRestoreTimer = nullptr;

    std::vector<SeriesKey> visibleSeriesKeys;
    std::unordered_map<SeriesKey, QLineSeries*> seriesMap;

    // Серии для обязательных параметров
    QLineSeries* seriesAltitude = nullptr;
    QLineSeries* seriesSpeed = nullptr;
    QLineSeries* seriesVoltage = nullptr;
    QLineSeries* seriesHeading = nullptr;
};