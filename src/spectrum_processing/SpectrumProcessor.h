#ifndef SPECTRUMPROCESSOR_H
#define SPECTRUMPROCESSOR_H

#include <QObject>
#include <QVector>
#include <QString>
#include "data_parser/SpectrumData.h"
#include "WavelengthCalibrator.h"
#include "SpectralLineFitter.h"
#include "RedshiftCalculator.h"

class SpectrumProcessor : public QObject
{
    Q_OBJECT

public:
    explicit SpectrumProcessor(QObject *parent = nullptr);

    WavelengthCalibrator* wavelengthCalibrator() const;
    SpectralLineFitter* lineFitter() const;
    RedshiftCalculator* redshiftCalculator() const;

    SpectrumData smooth(const SpectrumData &spectrum, int windowSize = 5);
    SpectrumData medianFilter(const SpectrumData &spectrum, int windowSize = 5);
    SpectrumData normalize(const SpectrumData &spectrum);
    SpectrumData rebin(const SpectrumData &spectrum, double newWavelengthStep);
    SpectrumData crop(const SpectrumData &spectrum, double wlStart, double wlEnd);

    SpectrumData subtractContinuum(const SpectrumData &spectrum);
    SpectrumData divideByContinuum(const SpectrumData &spectrum);

    SpectrumData addSpectra(const SpectrumData &spec1, const SpectrumData &spec2);
    SpectrumData subtractSpectra(const SpectrumData &spec1, const SpectrumData &spec2);
    SpectrumData multiplySpectrum(const SpectrumData &spec, double factor);

    double integrate(const SpectrumData &spectrum, double wlStart, double wlEnd);
    double averageFlux(const SpectrumData &spectrum, double wlStart, double wlEnd);

    QVector<double> radialVelocityCurve(const SpectrumData &spectrum, double restWavelength, int windowSize = 50);

    bool processPipeline(SpectrumData &spectrum, bool doCalibration = true,
                         bool doContinuum = true, bool detectLines = true,
                         bool calculateRedshift = true);

    QString lastError() const;

signals:
    void processingProgress(int current, int total);
    void processingComplete(const SpectrumData &result);
    void errorOccurred(const QString &error);

private:
    WavelengthCalibrator *m_wavelengthCalibrator;
    SpectralLineFitter *m_lineFitter;
    RedshiftCalculator *m_redshiftCalculator;
    QString m_lastError;
};

#endif // SPECTRUMPROCESSOR_H
