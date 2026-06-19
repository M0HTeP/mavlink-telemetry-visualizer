#pragma once
#include <QMainWindow>
#include <vector>
#include "common/config.h"
#include "transport/udp_transport.h"
#include "parser/mavlink_parser.h"
#include "model/telemetry_model.h"
#include "history/history_manager.h"
#include "ui/chart_widget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    UdpTransport* getTransport();

    // UI API wrappers
    std::vector<SeriesKey> getAvailableSeries() const;
    void setVisibleSeries(const std::vector<SeriesKey>& series);
    std::vector<SeriesKey> getVisibleSeries() const;
    std::vector<TelemetryPoint> querySeriesData(const SeriesKey& key, TimePoint start, TimePoint end) const;

private slots:
    void onMessageReceived(uint32_t msgid, const void* data);

private:
    UdpTransport* transport = nullptr;
    MavlinkParser* parser = nullptr;
    TelemetryModel* model = nullptr;
    HistoryManager* history = nullptr;
    ChartWidget* chartWidget = nullptr;
};