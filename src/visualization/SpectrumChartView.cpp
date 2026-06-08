#include "SpectrumChartView.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QtMath>
#include <QDebug>

SpectrumChartView::SpectrumChartView(QWidget *parent)
    : QChartView(parent)
    , m_chart(new QChart())
    , m_spectrumSeries(nullptr)
    , m_continuumSeries(nullptr)
    , m_fillSeries(nullptr)
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_axisYLog(new QLogValueAxis())
    , m_showContinuum(false)
    , m_showErrorBars(false)
    , m_showSpectralLines(true)
    , m_logY(false)
    , m_xLabel("Wavelength (Å)")
    , m_yLabel("Flux")
    , m_chartTitle("")
    , m_dataXMin(0)
    , m_dataXMax(1)
    , m_dataYMin(0)
    , m_dataYMax(1)
{
    setChart(m_chart);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QChartView::RubberBandDrag);
    setRubberBand(QChartView::RectangleRubberBand);

    m_chart->setTheme(QChart::ChartThemeDark);
    m_chart->setBackgroundBrush(QBrush(QColor(30, 30, 35)));
    m_chart->setTitleBrush(QBrush(Qt::white));

    m_axisX->setTitleText(m_xLabel);
    m_axisX->setLabelsColor(Qt::white);
    m_axisX->setTitleBrush(QBrush(Qt::white));
    m_axisX->setGridLineColor(QColor(60, 60, 70));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    m_axisY->setTitleText(m_yLabel);
    m_axisY->setLabelsColor(Qt::white);
    m_axisY->setTitleBrush(QBrush(Qt::white));
    m_axisY->setGridLineColor(QColor(60, 60, 70));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_axisYLog->setBase(10);
    m_axisYLog->setTitleText(m_yLabel);
    m_axisYLog->setLabelsColor(Qt::white);
    m_axisYLog->setTitleBrush(QBrush(Qt::white));
    m_axisYLog->setGridLineColor(QColor(60, 60, 70));

    createSeries();
}

void SpectrumChartView::setSpectrum(const SpectrumData &spectrum)
{
    m_spectrum = spectrum;

    if (m_spectrum.isValid()) {
        m_dataXMin = m_spectrum.minWavelength();
        m_dataXMax = m_spectrum.maxWavelength();
        m_dataYMin = m_spectrum.minFlux();
        m_dataYMax = m_spectrum.maxFlux();

        if (m_dataYMin <= 0 && m_logY) {
            m_dataYMin = m_dataYMax * 0.001;
        }
    }

    updateChart();
    resetZoom();
}

SpectrumData SpectrumChartView::spectrum() const
{
    return m_spectrum;
}

void SpectrumChartView::addSpectrumOverlay(const SpectrumData &spectrum, const QColor &color)
{
    QLineSeries *series = new QLineSeries();
    series->setName(spectrum.name());
    series->setColor(color);
    series->setPen(QPen(color, 1.5));

    QVector<double> wl = spectrum.wavelengths();
    QVector<double> flux = spectrum.flux();
    for (int i = 0; i < wl.size(); ++i) {
        series->append(wl[i], flux[i]);
    }

    m_chart->addSeries(series);
    series->attachAxis(m_axisX);
    series->attachAxis(m_logY ? m_axisY : m_axisY);

    m_overlaySeries.append(series);
    m_overlaySpectra.append(spectrum);
}

void SpectrumChartView::clearOverlays()
{
    for (QLineSeries *series : m_overlaySeries) {
        m_chart->removeSeries(series);
        delete series;
    }
    m_overlaySeries.clear();
    m_overlaySpectra.clear();
}

void SpectrumChartView::setShowContinuum(bool show)
{
    m_showContinuum = show;
    if (m_continuumSeries) {
        m_continuumSeries->setVisible(show);
    }
}

bool SpectrumChartView::showContinuum() const
{
    return m_showContinuum;
}

void SpectrumChartView::setShowErrorBars(bool show)
{
    m_showErrorBars = show;
}

bool SpectrumChartView::showErrorBars() const
{
    return m_showErrorBars;
}

void SpectrumChartView::setShowSpectralLines(bool show)
{
    m_showSpectralLines = show;
}

bool SpectrumChartView::showSpectralLines() const
{
    return m_showSpectralLines;
}

void SpectrumChartView::setLogY(bool logY)
{
    if (m_logY == logY) return;
    m_logY = logY;
    updateAxes();
}

bool SpectrumChartView::logY() const
{
    return m_logY;
}

void SpectrumChartView::setXLabel(const QString &label)
{
    m_xLabel = label;
    m_axisX->setTitleText(label);
}

QString SpectrumChartView::xLabel() const
{
    return m_xLabel;
}

void SpectrumChartView::setYLabel(const QString &label)
{
    m_yLabel = label;
    m_axisY->setTitleText(label);
    m_axisYLog->setTitleText(label);
}

QString SpectrumChartView::yLabel() const
{
    return m_yLabel;
}

void SpectrumChartView::setTitle(const QString &title)
{
    m_chartTitle = title;
    m_chart->setTitle(title);
}

QString SpectrumChartView::title() const
{
    return m_chartTitle;
}

void SpectrumChartView::zoomIn()
{
    m_chart->zoomIn();
    emit rangeChanged();
}

void SpectrumChartView::zoomOut()
{
    m_chart->zoomOut();
    emit rangeChanged();
}

void SpectrumChartView::resetZoom()
{
    m_chart->zoomReset();
    emit rangeChanged();
}

void SpectrumChartView::setXRange(double min, double max)
{
    m_axisX->setRange(min, max);
    emit rangeChanged();
}

void SpectrumChartView::setYRange(double min, double max)
{
    if (m_logY) {
        if (min <= 0) min = 0.001;
        m_axisYLog->setRange(min, max);
    } else {
        m_axisY->setRange(min, max);
    }
    emit rangeChanged();
}

double SpectrumChartView::xMin() const
{
    return m_axisX->min();
}

double SpectrumChartView::xMax() const
{
    return m_axisX->max();
}

double SpectrumChartView::yMin() const
{
    if (m_logY) {
        return m_axisYLog->min();
    }
    return m_axisY->min();
}

double SpectrumChartView::yMax() const
{
    if (m_logY) {
        return m_axisYLog->max();
    }
    return m_axisY->max();
}

void SpectrumChartView::saveChart(const QString &filePath, int width, int height)
{
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    m_chart->paint(&painter, pixmap.rect());
    painter.end();

    pixmap.save(filePath);
}

void SpectrumChartView::mouseMoveEvent(QMouseEvent *event)
{
    QChartView::mouseMoveEvent(event);

    QPointF chartPos = m_chart->mapToValue(event->pos());
    emit cursorPositionChanged(chartPos.x(), chartPos.y());
}

void SpectrumChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        resetZoom();
        return;
    }
    QChartView::mousePressEvent(event);
}

void SpectrumChartView::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        m_chart->zoomIn();
    } else {
        m_chart->zoomOut();
    }
    emit rangeChanged();
}

void SpectrumChartView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        m_chart->zoomIn();
        emit rangeChanged();
        break;
    case Qt::Key_Minus:
        m_chart->zoomOut();
        emit rangeChanged();
        break;
    case Qt::Key_R:
        resetZoom();
        break;
    default:
        QChartView::keyPressEvent(event);
    }
}

void SpectrumChartView::resizeEvent(QResizeEvent *event)
{
    QChartView::resizeEvent(event);
}

void SpectrumChartView::updateChart()
{
    clearSeries();
    createSeries();

    if (!m_spectrum.isValid()) return;

    QVector<double> wl = m_spectrum.wavelengths();
    QVector<double> flux = m_spectrum.flux();
    QVector<double> continuum = m_spectrum.continuum();

    for (int i = 0; i < wl.size(); ++i) {
        m_spectrumSeries->append(wl[i], flux[i]);

        if (m_showContinuum && i < continuum.size()) {
            m_continuumSeries->append(wl[i], continuum[i]);
        }
    }

    if (!flux.isEmpty()) {
        double minFlux = *std::min_element(flux.begin(), flux.end());
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        double yPad = (maxFlux - minFlux) * 0.1;

        if (m_logY) {
            if (minFlux <= 0) minFlux = maxFlux * 0.001;
            m_axisYLog->setRange(minFlux * 0.5, maxFlux * 1.5);
        } else {
            m_axisY->setRange(minFlux - yPad, maxFlux + yPad);
        }

        m_axisX->setRange(m_dataXMin, m_dataXMax);
    }
}

void SpectrumChartView::updateAxes()
{
    if (m_logY) {
        if (m_chart->axes(Qt::Vertical).contains(m_axisY)) {
            m_chart->removeAxis(m_axisY);
        }
        if (!m_chart->axes(Qt::Vertical).contains(m_axisYLog)) {
            m_chart->addAxis(m_axisYLog, Qt::AlignLeft);
        }
    } else {
        if (m_chart->axes(Qt::Vertical).contains(m_axisYLog)) {
            m_chart->removeAxis(m_axisYLog);
        }
        if (!m_chart->axes(Qt::Vertical).contains(m_axisY)) {
            m_chart->addAxis(m_axisY, Qt::AlignLeft);
        }
    }

    QList<QAbstractSeries *> seriesList = m_chart->series();
    for (QAbstractSeries *s : seriesList) {
        QXYSeries *xySeries = qobject_cast<QXYSeries *>(s);
        if (xySeries) {
            if (m_logY) {
                xySeries->attachAxis(m_axisYLog);
            } else {
                xySeries->attachAxis(m_axisY);
            }
            xySeries->attachAxis(m_axisX);
        }
    }
}

void SpectrumChartView::createSeries()
{
    m_spectrumSeries = new QLineSeries();
    m_spectrumSeries->setName("Spectrum");
    m_spectrumSeries->setColor(QColor(100, 180, 255));
    m_spectrumSeries->setPen(QPen(QColor(100, 180, 255), 1.5));
    m_chart->addSeries(m_spectrumSeries);
    m_spectrumSeries->attachAxis(m_axisX);
    m_spectrumSeries->attachAxis(m_axisY);

    m_continuumSeries = new QLineSeries();
    m_continuumSeries->setName("Continuum");
    m_continuumSeries->setColor(QColor(255, 180, 100));
    m_continuumSeries->setPen(QPen(QColor(255, 180, 100), 1.2, Qt::DashLine));
    m_continuumSeries->setVisible(m_showContinuum);
    m_chart->addSeries(m_continuumSeries);
    m_continuumSeries->attachAxis(m_axisX);
    m_continuumSeries->attachAxis(m_axisY);

    m_chart->legend()->setVisible(true);
    m_chart->legend()->setLabelColor(Qt::white);
    m_chart->legend()->setBackgroundVisible(true);
    m_chart->legend()->setBrush(QBrush(QColor(50, 50, 55)));
    m_chart->legend()->setBorderColor(QColor(70, 70, 75));
}

void SpectrumChartView::clearSeries()
{
    clearOverlays();

    if (m_spectrumSeries) {
        m_chart->removeSeries(m_spectrumSeries);
        delete m_spectrumSeries;
        m_spectrumSeries = nullptr;
    }

    if (m_continuumSeries) {
        m_chart->removeSeries(m_continuumSeries);
        delete m_continuumSeries;
        m_continuumSeries = nullptr;
    }

    if (m_fillSeries) {
        m_chart->removeSeries(m_fillSeries);
        delete m_fillSeries;
        m_fillSeries = nullptr;
    }
}
