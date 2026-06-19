#include "ui/main_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    transport = new UdpTransport(this);
    parser = new MavlinkParser(this);
    model = new TelemetryModel(this);
    history = new HistoryManager(this);
    chartWidget = new ChartWidget(history, this);

    // UI
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    
    auto* title = new QLabel("Mavlink Telemetry Visualizer", this);
    title->setAlignment(Qt::AlignCenter);

    layout->addWidget(title);
    layout->addWidget(chartWidget);

    setCentralWidget(central);
    resize(1400, 900);

    // Соединения
    connect(transport, &UdpTransport::dataReceived, 
            parser, &MavlinkParser::processData);

    connect(parser, &MavlinkParser::messageParsed, 
            this, &MainWindow::onMessageReceived);

    connect(model, &TelemetryModel::valueUpdated, 
            history, &HistoryManager::onValueUpdated);

    qDebug() << "✅ MainWindow: Все соединения установлены";
}

UdpTransport* MainWindow::getTransport() {
    return transport;
}

std::vector<SeriesKey> MainWindow::getAvailableSeries() const {
    if (!chartWidget) {
        return {};
    }
    return chartWidget->getAvailableSeries();
}

void MainWindow::setVisibleSeries(const std::vector<SeriesKey>& series) {
    if (!chartWidget) {
        return;
    }
    chartWidget->setVisibleSeries(series);
}

std::vector<SeriesKey> MainWindow::getVisibleSeries() const {
    if (!chartWidget) {
        return {};
    }
    return chartWidget->getVisibleSeries();
}

std::vector<TelemetryPoint> MainWindow::querySeriesData(const SeriesKey& key, TimePoint start, TimePoint end) const {
    if (!chartWidget) {
        return {};
    }
    return chartWidget->getSeriesData(key, start, end);
}

void MainWindow::onMessageReceived(uint32_t msgid, const void* data) {
    qDebug() << "📨 Received message:" << msgid;

    if (msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
        auto* pos = static_cast<const mavlink_global_position_int_t*>(data);
        model->updateValue(msgid, "relative_alt", pos->relative_alt / 1000.0);
    }
    else if (msgid == MAVLINK_MSG_ID_VFR_HUD) {
        auto* hud = static_cast<const mavlink_vfr_hud_t*>(data);
        model->updateValue(msgid, "groundspeed", hud->groundspeed);
        model->updateValue(msgid, "heading", hud->heading);
    }
    else if (msgid == MAVLINK_MSG_ID_SYS_STATUS) {
        auto* sys = static_cast<const mavlink_sys_status_t*>(data);
        model->updateValue(msgid, "voltage_battery", sys->voltage_battery / 1000.0);
    }
}