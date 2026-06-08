#ifndef SPECTRUMPLOT_H
#define SPECTRUMPLOT_H

#include <QWidget>
#include <QVector>
#include <QPen>
#include <QBrush>
#include <QPointF>
#include <QRectF>
#include "data_parser/SpectrumData.h"

class SpectrumPlot : public QWidget
{
    Q_OBJECT

public:
    explicit SpectrumPlot(QWidget *parent = nullptr);

    void setSpectrum(const SpectrumData &spectrum);
    SpectrumData spectrum() const;

    void setShowContinuum(bool show);
    bool showContinuum() const;

    void setShowErrorBars(bool show);
    bool showErrorBars() const;

    void setShowSpectralLines(bool show);
    bool showSpectralLines() const;

    void setShowGrid(bool show);
    bool showGrid() const;

    void setXLabel(const QString &label);
    QString xLabel() const;

    void setYLabel(const QString &label);
    QString yLabel() const;

    void setTitle(const QString &title);
    QString title() const;

    void setSpectrumColor(const QColor &color);
    QColor spectrumColor() const;

    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;

    void setGridColor(const QColor &color);
    QColor gridColor() const;

    void zoomIn();
    void zoomOut();
    void resetZoom();

    void pan(double dx, double dy);

    double xMin() const;
    double xMax() const;
    double yMin() const;
    double yMax() const;

    void setXRange(double min, double max);
    void setYRange(double min, double max);

    QPointF mapDataToWidget(double x, double y) const;
    QPointF mapWidgetToData(double x, double y) const;

signals:
    void rangeChanged();
    void cursorPositionChanged(double wavelength, double flux);
    void spectralLineClicked(const SpectralLine &line);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void drawBackground(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawSpectrum(QPainter &painter);
    void drawContinuum(QPainter &painter);
    void drawErrorBars(QPainter &painter);
    void drawSpectralLines(QPainter &painter);
    void drawAxes(QPainter &painter);
    void drawLabels(QPainter &painter);
    void drawTitle(QPainter &painter);
    void drawCursor(QPainter &painter);

    void updateDataRanges();
    void updateMargins();

    SpectrumData m_spectrum;

    bool m_showContinuum;
    bool m_showErrorBars;
    bool m_showSpectralLines;
    bool m_showGrid;

    QString m_xLabel;
    QString m_yLabel;
    QString m_title;

    QColor m_spectrumColor;
    QColor m_continuumColor;
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_axisColor;
    QColor m_textColor;
    QColor m_emissionLineColor;
    QColor m_absorptionLineColor;

    double m_xMin, m_xMax;
    double m_yMin, m_yMax;
    double m_dataXMin, m_dataXMax;
    double m_dataYMin, m_dataYMax;

    int m_leftMargin;
    int m_rightMargin;
    int m_topMargin;
    int m_bottomMargin;

    bool m_isDragging;
    QPoint m_lastMousePos;
    QPoint m_cursorPos;
    bool m_cursorVisible;

    double m_zoomFactor;
};

#endif // SPECTRUMPLOT_H
