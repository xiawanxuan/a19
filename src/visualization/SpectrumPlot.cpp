#include "SpectrumPlot.h"
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainterPath>
#include <QtMath>
#include <QDebug>

SpectrumPlot::SpectrumPlot(QWidget *parent)
    : QWidget(parent)
    , m_showContinuum(false)
    , m_showErrorBars(false)
    , m_showSpectralLines(true)
    , m_showGrid(true)
    , m_xLabel("Wavelength (Å)")
    , m_yLabel("Flux")
    , m_title("")
    , m_spectrumColor(100, 180, 255)
    , m_continuumColor(255, 180, 100)
    , m_backgroundColor(30, 30, 35)
    , m_gridColor(60, 60, 70)
    , m_axisColor(200, 200, 200)
    , m_textColor(220, 220, 220)
    , m_emissionLineColor(255, 100, 100)
    , m_absorptionLineColor(100, 200, 100)
    , m_xMin(0)
    , m_xMax(1)
    , m_yMin(0)
    , m_yMax(1)
    , m_dataXMin(0)
    , m_dataXMax(1)
    , m_dataYMin(0)
    , m_dataYMax(1)
    , m_leftMargin(60)
    , m_rightMargin(20)
    , m_topMargin(40)
    , m_bottomMargin(50)
    , m_isDragging(false)
    , m_cursorVisible(false)
    , m_zoomFactor(1.0)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);
    updateMargins();
}

void SpectrumPlot::setSpectrum(const SpectrumData &spectrum)
{
    m_spectrum = spectrum;
    updateDataRanges();
    resetZoom();
    update();
}

SpectrumData SpectrumPlot::spectrum() const
{
    return m_spectrum;
}

void SpectrumPlot::setShowContinuum(bool show)
{
    m_showContinuum = show;
    update();
}

bool SpectrumPlot::showContinuum() const
{
    return m_showContinuum;
}

void SpectrumPlot::setShowErrorBars(bool show)
{
    m_showErrorBars = show;
    update();
}

bool SpectrumPlot::showErrorBars() const
{
    return m_showErrorBars;
}

void SpectrumPlot::setShowSpectralLines(bool show)
{
    m_showSpectralLines = show;
    update();
}

bool SpectrumPlot::showSpectralLines() const
{
    return m_showSpectralLines;
}

void SpectrumPlot::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

bool SpectrumPlot::showGrid() const
{
    return m_showGrid;
}

void SpectrumPlot::setXLabel(const QString &label)
{
    m_xLabel = label;
    updateMargins();
    update();
}

QString SpectrumPlot::xLabel() const
{
    return m_xLabel;
}

void SpectrumPlot::setYLabel(const QString &label)
{
    m_yLabel = label;
    updateMargins();
    update();
}

QString SpectrumPlot::yLabel() const
{
    return m_yLabel;
}

void SpectrumPlot::setTitle(const QString &title)
{
    m_title = title;
    updateMargins();
    update();
}

QString SpectrumPlot::title() const
{
    return m_title;
}

void SpectrumPlot::setSpectrumColor(const QColor &color)
{
    m_spectrumColor = color;
    update();
}

QColor SpectrumPlot::spectrumColor() const
{
    return m_spectrumColor;
}

void SpectrumPlot::setBackgroundColor(const QColor &color)
{
    m_backgroundColor = color;
    update();
}

QColor SpectrumPlot::backgroundColor() const
{
    return m_backgroundColor;
}

void SpectrumPlot::setGridColor(const QColor &color)
{
    m_gridColor = color;
    update();
}

QColor SpectrumPlot::gridColor() const
{
    return m_gridColor;
}

void SpectrumPlot::zoomIn()
{
    double xCenter = (m_xMin + m_xMax) / 2.0;
    double yCenter = (m_yMin + m_yMax) / 2.0;
    double xRange = (m_xMax - m_xMin) * 0.7;
    double yRange = (m_yMax - m_yMin) * 0.7;

    m_xMin = xCenter - xRange / 2.0;
    m_xMax = xCenter + xRange / 2.0;
    m_yMin = yCenter - yRange / 2.0;
    m_yMax = yCenter + yRange / 2.0;

    m_zoomFactor *= 1.0 / 0.7;

    emit rangeChanged();
    update();
}

void SpectrumPlot::zoomOut()
{
    double xCenter = (m_xMin + m_xMax) / 2.0;
    double yCenter = (m_yMin + m_yMax) / 2.0;
    double xRange = (m_xMax - m_xMin) * 1.3;
    double yRange = (m_yMax - m_yMin) * 1.3;

    m_xMin = xCenter - xRange / 2.0;
    m_xMax = xCenter + xRange / 2.0;
    m_yMin = yCenter - yRange / 2.0;
    m_yMax = yCenter + yRange / 2.0;

    m_zoomFactor *= 0.7;

    if (m_zoomFactor < 0.1) {
        resetZoom();
        return;
    }

    emit rangeChanged();
    update();
}

void SpectrumPlot::resetZoom()
{
    m_xMin = m_dataXMin;
    m_xMax = m_dataXMax;
    m_yMin = m_dataYMin;
    m_yMax = m_dataYMax;
    m_zoomFactor = 1.0;

    double yPad = (m_yMax - m_yMin) * 0.1;
    if (yPad == 0) yPad = 1.0;
    m_yMin -= yPad;
    m_yMax += yPad;

    emit rangeChanged();
    update();
}

void SpectrumPlot::pan(double dx, double dy)
{
    double xRange = m_xMax - m_xMin;
    double yRange = m_yMax - m_yMin;

    m_xMin += dx * xRange;
    m_xMax += dx * xRange;
    m_yMin += dy * yRange;
    m_yMax += dy * yRange;

    emit rangeChanged();
    update();
}

double SpectrumPlot::xMin() const
{
    return m_xMin;
}

double SpectrumPlot::xMax() const
{
    return m_xMax;
}

double SpectrumPlot::yMin() const
{
    return m_yMin;
}

double SpectrumPlot::yMax() const
{
    return m_yMax;
}

void SpectrumPlot::setXRange(double min, double max)
{
    m_xMin = min;
    m_xMax = max;
    emit rangeChanged();
    update();
}

void SpectrumPlot::setYRange(double min, double max)
{
    m_yMin = min;
    m_yMax = max;
    emit rangeChanged();
    update();
}

QPointF SpectrumPlot::mapDataToWidget(double x, double y) const
{
    int plotWidth = width() - m_leftMargin - m_rightMargin;
    int plotHeight = height() - m_topMargin - m_bottomMargin;

    double xNorm = (x - m_xMin) / (m_xMax - m_xMin);
    double yNorm = (y - m_yMin) / (m_yMax - m_yMin);

    return QPointF(m_leftMargin + xNorm * plotWidth,
                   height() - m_bottomMargin - yNorm * plotHeight);
}

QPointF SpectrumPlot::mapWidgetToData(double x, double y) const
{
    int plotWidth = width() - m_leftMargin - m_rightMargin;
    int plotHeight = height() - m_topMargin - m_bottomMargin;

    double xNorm = (x - m_leftMargin) / plotWidth;
    double yNorm = (height() - m_bottomMargin - y) / plotHeight;

    return QPointF(m_xMin + xNorm * (m_xMax - m_xMin),
                   m_yMin + yNorm * (m_yMax - m_yMin));
}

void SpectrumPlot::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);

    if (m_showGrid) {
        drawGrid(painter);
    }

    if (m_spectrum.isValid()) {
        if (m_showContinuum && !m_spectrum.continuum().isEmpty()) {
            drawContinuum(painter);
        }

        drawSpectrum(painter);

        if (m_showErrorBars && !m_spectrum.error().isEmpty()) {
            drawErrorBars(painter);
        }

        if (m_showSpectralLines && !m_spectrum.spectralLines().isEmpty()) {
            drawSpectralLines(painter);
        }
    }

    drawAxes(painter);
    drawLabels(painter);
    drawTitle(painter);

    if (m_cursorVisible) {
        drawCursor(painter);
    }
}

void SpectrumPlot::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    updateMargins();
}

void SpectrumPlot::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void SpectrumPlot::mouseMoveEvent(QMouseEvent *event)
{
    m_cursorPos = event->pos();
    m_cursorVisible = true;

    QPointF dataPoint = mapWidgetToData(event->x(), event->y());
    emit cursorPositionChanged(dataPoint.x(), dataPoint.y());

    if (m_isDragging) {
        double dx = (m_lastMousePos.x() - event->x()) /
                    static_cast<double>(width() - m_leftMargin - m_rightMargin);
        double dy = (event->y() - m_lastMousePos.y()) /
                    static_cast<double>(height() - m_topMargin - m_bottomMargin);

        pan(dx, dy);
        m_lastMousePos = event->pos();
    }

    update();
}

void SpectrumPlot::wheelEvent(QWheelEvent *event)
{
    QPointF dataPoint = mapWidgetToData(event->position().x(), event->position().y());

    if (event->angleDelta().y() > 0) {
        double xRange = (m_xMax - m_xMin) * 0.85;
        double yRange = (m_yMax - m_yMin) * 0.85;

        double xRatio = (dataPoint.x() - m_xMin) / (m_xMax - m_xMin);
        double yRatio = (dataPoint.y() - m_yMin) / (m_yMax - m_yMin);

        m_xMin = dataPoint.x() - xRatio * xRange;
        m_xMax = dataPoint.x() + (1 - xRatio) * xRange;
        m_yMin = dataPoint.y() - yRatio * yRange;
        m_yMax = dataPoint.y() + (1 - yRatio) * yRange;

        m_zoomFactor /= 0.85;
    } else {
        double xRange = (m_xMax - m_xMin) / 0.85;
        double yRange = (m_yMax - m_yMin) / 0.85;

        double xRatio = (dataPoint.x() - m_xMin) / (m_xMax - m_xMin);
        double yRatio = (dataPoint.y() - m_yMin) / (m_yMax - m_yMin);

        m_xMin = dataPoint.x() - xRatio * xRange;
        m_xMax = dataPoint.x() + (1 - xRatio) * xRange;
        m_yMin = dataPoint.y() - yRatio * yRange;
        m_yMax = dataPoint.y() + (1 - yRatio) * yRange;

        m_zoomFactor *= 0.85;
    }

    emit rangeChanged();
    update();
}

void SpectrumPlot::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_cursorVisible = false;
    m_isDragging = false;
    unsetCursor();
    update();
}

void SpectrumPlot::drawBackground(QPainter &painter)
{
    painter.fillRect(rect(), m_backgroundColor);

    QRect plotRect(m_leftMargin, m_topMargin,
                   width() - m_leftMargin - m_rightMargin,
                   height() - m_topMargin - m_bottomMargin);

    QColor plotBg = m_backgroundColor.lighter(120);
    painter.fillRect(plotRect, plotBg);
}

void SpectrumPlot::drawGrid(QPainter &painter)
{
    int plotWidth = width() - m_leftMargin - m_rightMargin;
    int plotHeight = height() - m_topMargin - m_bottomMargin;

    painter.setPen(QPen(m_gridColor, 1, Qt::DashLine));

    int numXDivisions = 8;
    for (int i = 0; i <= numXDivisions; ++i) {
        double x = m_leftMargin + static_cast<double>(i) / numXDivisions * plotWidth;
        painter.drawLine(QPointF(x, m_topMargin),
                         QPointF(x, height() - m_bottomMargin));
    }

    int numYDivisions = 6;
    for (int i = 0; i <= numYDivisions; ++i) {
        double y = m_topMargin + static_cast<double>(i) / numYDivisions * plotHeight;
        painter.drawLine(QPointF(m_leftMargin, y),
                         QPointF(width() - m_rightMargin, y));
    }
}

void SpectrumPlot::drawSpectrum(QPainter &painter)
{
    if (!m_spectrum.isValid()) return;

    QVector<double> wavelengths = m_spectrum.wavelengths();
    QVector<double> flux = m_spectrum.flux();

    int n = qMin(wavelengths.size(), flux.size());
    if (n < 2) return;

    QPainterPath path;
    bool firstPoint = true;

    for (int i = 0; i < n; ++i) {
        double wl = wavelengths[i];
        double fl = flux[i];

        if (wl < m_xMin || wl > m_xMax) {
            firstPoint = true;
            continue;
        }

        QPointF pt = mapDataToWidget(wl, fl);

        if (firstPoint) {
            path.moveTo(pt);
            firstPoint = false;
        } else {
            path.lineTo(pt);
        }
    }

    painter.setPen(QPen(m_spectrumColor, 1.5));
    painter.drawPath(path);

    QPainterPath fillPath = path;
    QPointF bottomLeft = mapDataToWidget(m_xMin, m_yMin);
    QPointF bottomRight = mapDataToWidget(m_xMax, m_yMin);

    if (fillPath.elementCount() > 0) {
        fillPath.lineTo(bottomRight);
        fillPath.lineTo(bottomLeft);
        fillPath.closeSubpath();

        QLinearGradient gradient(0, m_topMargin, 0, height() - m_bottomMargin);
        gradient.setColorAt(0.0, QColor(m_spectrumColor.red(), m_spectrumColor.green(),
                                         m_spectrumColor.blue(), 80));
        gradient.setColorAt(1.0, QColor(m_spectrumColor.red(), m_spectrumColor.green(),
                                         m_spectrumColor.blue(), 10));

        painter.fillPath(fillPath, gradient);
    }
}

void SpectrumPlot::drawContinuum(QPainter &painter)
{
    if (!m_spectrum.isValid() || m_spectrum.continuum().isEmpty()) return;

    QVector<double> wavelengths = m_spectrum.wavelengths();
    QVector<double> continuum = m_spectrum.continuum();

    int n = qMin(wavelengths.size(), continuum.size());
    if (n < 2) return;

    QPainterPath path;
    bool firstPoint = true;

    for (int i = 0; i < n; ++i) {
        double wl = wavelengths[i];
        double cont = continuum[i];

        if (wl < m_xMin || wl > m_xMax) {
            firstPoint = true;
            continue;
        }

        QPointF pt = mapDataToWidget(wl, cont);

        if (firstPoint) {
            path.moveTo(pt);
            firstPoint = false;
        } else {
            path.lineTo(pt);
        }
    }

    painter.setPen(QPen(m_continuumColor, 1.2, Qt::DashLine));
    painter.drawPath(path);
}

void SpectrumPlot::drawErrorBars(QPainter &painter)
{
    if (!m_spectrum.isValid() || m_spectrum.error().isEmpty()) return;

    QVector<double> wavelengths = m_spectrum.wavelengths();
    QVector<double> flux = m_spectrum.flux();
    QVector<double> error = m_spectrum.error();

    int n = qMin(wavelengths.size(), qMin(flux.size(), error.size()));
    if (n < 2) return;

    int step = qMax(1, n / 200);

    painter.setPen(QPen(QColor(255, 255, 255, 100), 1));

    for (int i = 0; i < n; i += step) {
        double wl = wavelengths[i];
        if (wl < m_xMin || wl > m_xMax) continue;

        double fl = flux[i];
        double err = error[i];

        QPointF ptTop = mapDataToWidget(wl, fl + err);
        QPointF ptBottom = mapDataToWidget(wl, fl - err);
        QPointF ptMid = mapDataToWidget(wl, fl);

        painter.drawLine(ptTop, ptBottom);

        double barWidth = 3;
        painter.drawLine(ptTop.x() - barWidth, ptTop.y(),
                         ptTop.x() + barWidth, ptTop.y());
        painter.drawLine(ptBottom.x() - barWidth, ptBottom.y(),
                         ptBottom.x() + barWidth, ptBottom.y());
    }
}

void SpectrumPlot::drawSpectralLines(QPainter &painter)
{
    if (!m_spectrum.isValid()) return;

    QVector<SpectralLine> lines = m_spectrum.spectralLines();

    for (const SpectralLine &line : lines) {
        double wl = line.wavelength;
        if (wl < m_xMin || wl > m_xMax) continue;

        QColor color = line.isEmission ? m_emissionLineColor : m_absorptionLineColor;

        QPointF ptTop(wl, m_yMax);
        QPointF ptBottom(wl, m_yMin);

        QPointF topWidget = mapDataToWidget(ptTop.x(), ptTop.y());
        QPointF bottomWidget = mapDataToWidget(ptBottom.x(), ptBottom.y());

        painter.setPen(QPen(color, 1, Qt::SolidLine));
        painter.drawLine(topWidget, bottomWidget);

        QPointF labelPos = mapDataToWidget(wl, m_yMax);
        labelPos.setY(m_topMargin + 15);

        painter.setPen(color);
        QFont font = painter.font();
        font.setPointSize(8);
        painter.setFont(font);

        painter.save();
        painter.translate(labelPos);
        painter.rotate(-45);
        painter.drawText(QPoint(2, 0), line.name);
        painter.restore();
    }
}

void SpectrumPlot::drawAxes(QPainter &painter)
{
    painter.setPen(QPen(m_axisColor, 2));

    painter.drawLine(m_leftMargin, m_topMargin,
                     m_leftMargin, height() - m_bottomMargin);

    painter.drawLine(m_leftMargin, height() - m_bottomMargin,
                     width() - m_rightMargin, height() - m_bottomMargin);

    painter.setPen(QPen(m_textColor, 1));
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    int numXDivisions = 8;
    int plotWidth = width() - m_leftMargin - m_rightMargin;
    int plotHeight = height() - m_topMargin - m_bottomMargin;

    for (int i = 0; i <= numXDivisions; ++i) {
        double x = m_leftMargin + static_cast<double>(i) / numXDivisions * plotWidth;
        double value = m_xMin + static_cast<double>(i) / numXDivisions * (m_xMax - m_xMin);

        painter.drawLine(QPointF(x, height() - m_bottomMargin),
                         QPointF(x, height() - m_bottomMargin + 5));

        QString label = QString::number(value, 'f', 0);
        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(label);
        painter.drawText(QPointF(x - textWidth / 2.0, height() - m_bottomMargin + 18), label);
    }

    int numYDivisions = 6;
    for (int i = 0; i <= numYDivisions; ++i) {
        double y = m_topMargin + static_cast<double>(i) / numYDivisions * plotHeight;
        double value = m_yMax - static_cast<double>(i) / numYDivisions * (m_yMax - m_yMin);

        painter.drawLine(QPointF(m_leftMargin - 5, y),
                         QPointF(m_leftMargin, y));

        QString label = QString::number(value, 'g', 4);
        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(label);
        painter.drawText(QPointF(m_leftMargin - textWidth - 8, y + 4), label);
    }
}

void SpectrumPlot::drawLabels(QPainter &painter)
{
    painter.setPen(m_textColor);
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(m_xLabel);

    painter.drawText(QPointF((width() - textWidth) / 2.0, height() - 8), m_xLabel);

    painter.save();
    painter.translate(18, height() / 2.0);
    painter.rotate(-90);
    int yTextWidth = fm.horizontalAdvance(m_yLabel);
    painter.drawText(QPointF(-yTextWidth / 2.0, 0), m_yLabel);
    painter.restore();
}

void SpectrumPlot::drawTitle(QPainter &painter)
{
    if (m_title.isEmpty()) return;

    painter.setPen(m_textColor);
    QFont font = painter.font();
    font.setPointSize(12);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(m_title);

    painter.drawText(QPointF((width() - textWidth) / 2.0, 25), m_title);
}

void SpectrumPlot::drawCursor(QPainter &painter)
{
    if (m_cursorPos.x() < m_leftMargin || m_cursorPos.x() > width() - m_rightMargin)
        return;
    if (m_cursorPos.y() < m_topMargin || m_cursorPos.y() > height() - m_bottomMargin)
        return;

    painter.setPen(QPen(QColor(255, 255, 255, 150), 1, Qt::DotLine));

    painter.drawLine(QPointF(m_cursorPos.x(), m_topMargin),
                     QPointF(m_cursorPos.x(), height() - m_bottomMargin));

    painter.drawLine(QPointF(m_leftMargin, m_cursorPos.y()),
                     QPointF(width() - m_rightMargin, m_cursorPos.y()));

    QPointF dataPoint = mapWidgetToData(m_cursorPos.x(), m_cursorPos.y());
    QString info = QString("λ: %1 Å\nFlux: %2")
                       .arg(dataPoint.x(), 0, 'f', 2)
                       .arg(dataPoint.y(), 0, 'g', 4);

    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    QRect textRect = painter.fontMetrics().boundingRect(info);
    int tooltipX = m_cursorPos.x() + 15;
    int tooltipY = m_cursorPos.y() - 10;

    if (tooltipX + textRect.width() + 20 > width() - m_rightMargin) {
        tooltipX = m_cursorPos.x() - textRect.width() - 20;
    }
    if (tooltipY - textRect.height() < m_topMargin) {
        tooltipY = m_cursorPos.y() + 20;
    }

    QColor bgColor(0, 0, 0, 200);
    painter.fillRect(tooltipX - 5, tooltipY - textRect.height() - 5,
                     textRect.width() + 10, textRect.height() + 10, bgColor);

    painter.setPen(Qt::white);
    painter.drawText(QPoint(tooltipX, tooltipY - textRect.height() / 4), info);
}

void SpectrumPlot::updateDataRanges()
{
    if (!m_spectrum.isValid()) {
        m_dataXMin = 0;
        m_dataXMax = 1;
        m_dataYMin = 0;
        m_dataYMax = 1;
        return;
    }

    m_dataXMin = m_spectrum.minWavelength();
    m_dataXMax = m_spectrum.maxWavelength();
    m_dataYMin = m_spectrum.minFlux();
    m_dataYMax = m_spectrum.maxFlux();

    if (m_dataXMin == m_dataXMax) {
        m_dataXMax = m_dataXMin + 1;
    }
    if (m_dataYMin == m_dataYMax) {
        m_dataYMax = m_dataYMin + 1;
    }
}

void SpectrumPlot::updateMargins()
{
    m_leftMargin = 65;
    m_rightMargin = 25;
    m_topMargin = m_title.isEmpty() ? 20 : 45;
    m_bottomMargin = 60;
}
