#ifndef SPECTRUMCHARTVIEW_H
#define SPECTRUMCHARTVIEW_H

#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QScatterSeries>
#include <QAreaSeries>
#include <QValueAxis>
#include <QLogValueAxis>
#include <QList>
#include "data_parser/SpectrumData.h"

class SpectrumChartView : public QChartView
{
    Q_OBJECT

public:
    explicit SpectrumChartView(QWidget *parent = nullptr);

    void setSpectrum(const SpectrumData &spectrum);
    SpectrumData spectrum() const;

    void addSpectrumOverlay(const SpectrumData &spectrum, const QColor &color);
    void clearOverlays();

    void setShowContinuum(bool show);
    bool showContinuum() const;

    void setShowErrorBars(bool show);
    bool showErrorBars() const;

    void setShowSpectralLines(bool show);
    bool showSpectralLines() const;

    void setLogY(bool logY);
    bool logY() const;

    void setXLabel(const QString &label);
    QString xLabel() const;

    void setYLabel(const QString &label);
    QString yLabel() const;

    void setTitle(const QString &title);
    QString title() const;

    void zoomIn();
    void zoomOut();
    void resetZoom();

    void setXRange(double min, double max);
    void setYRange(double min, double max);

    double xMin() const;
    double xMax() const;
    double yMin() const;
    double yMax() const;

    void saveChart(const QString &filePath, int width = 1200, int height = 800);

signals:
    void cursorPositionChanged(double wavelength, double flux);
    void spectralLineSelected(const SpectralLine &line);
    void rangeChanged();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateChart();
    void updateAxes();
    void createSeries();
    void clearSeries();

    QChart *m_chart;
    SpectrumData m_spectrum;

    QLineSeries *m_spectrumSeries;
    QLineSeries *m_continuumSeries;
    QAreaSeries *m_fillSeries;
    QList<QLineSeries *> m_overlaySeries;
    QList<SpectrumData> m_overlaySpectra;

    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QLogValueAxis *m_axisYLog;

    bool m_showContinuum;
    bool m_showErrorBars;
    bool m_showSpectralLines;
    bool m_logY;

    QString m_xLabel;
    QString m_yLabel;
    QString m_chartTitle;

    double m_dataXMin, m_dataXMax;
    double m_dataYMin, m_dataYMax;
};

#endif // SPECTRUMCHARTVIEW_H
