#ifndef SPECTRALLINEFITTER_H
#define SPECTRALLINEFITTER_H

#include <QObject>
#include <QVector>
#include <QString>
#include "data_parser/SpectrumData.h"

class SpectralLineFitter : public QObject
{
    Q_OBJECT

public:
    explicit SpectralLineFitter(QObject *parent = nullptr);

    enum LineType {
        Gaussian,
        Lorentzian,
        Voigt
    };

    enum DetectionMethod {
        PeakDetection,
        DerivativeMethod,
        ContinuumSubtraction
    };

    struct FitResult {
        bool success;
        double center;
        double amplitude;
        double fwhm;
        double height;
        double area;
        double chiSquared;
        double redChiSquared;
        QVector<double> residuals;
        LineType lineType;
    };

    void setLineType(LineType type);
    LineType lineType() const;

    void setDetectionMethod(DetectionMethod method);
    DetectionMethod detectionMethod() const;

    void setNoiseThreshold(double sigma);
    double noiseThreshold() const;

    void setMinLineWidth(double minWidth);
    double minLineWidth() const;

    void setMaxLineWidth(double maxWidth);
    double maxLineWidth() const;

    void setMaxLines(int maxLines);
    int maxLines() const;

    void setContinuumOrder(int order);
    int continuumOrder() const;

    QVector<SpectralLine> detectLines(const SpectrumData &spectrum);
    QVector<SpectralLine> detectLinesInRange(const SpectrumData &spectrum, double wlStart, double wlEnd);

    FitResult fitLine(const SpectrumData &spectrum, double wavelengthGuess);
    FitResult fitLine(const QVector<double> &wavelengths, const QVector<double> &flux,
                      double centerGuess, double amplitudeGuess, double fwhmGuess);

    bool fitAllLines(SpectrumData &spectrum);

    QVector<double> fitContinuum(const SpectrumData &spectrum);
    QVector<double> fitContinuum(const QVector<double> &wavelengths, const QVector<double> &flux);

    double computeNoise(const SpectrumData &spectrum);
    double computeSNR(const SpectrumData &spectrum, double wavelength);

    static double gaussian(double x, double center, double amplitude, double fwhm);
    static double lorentzian(double x, double center, double amplitude, double fwhm);
    static double voigt(double x, double center, double amplitude, double fwhm, double fraction = 0.5);

    static double fwhmToSigma(double fwhm);
    static double sigmaToFwhm(double sigma);

    static double equivalentWidth(const QVector<double> &wavelengths, const QVector<double> &flux,
                                  const QVector<double> &continuum, double center, double window);

signals:
    void fittingProgress(int current, int total);
    void fittingComplete(bool success);

private:
    QVector<int> findPeaks(const QVector<double> &data, int minDistance, double threshold);
    QVector<int> findValleys(const QVector<double> &data, int minDistance, double threshold);

    bool levenbergMarquardtFit(const QVector<double> &x, const QVector<double> &y,
                               double &center, double &amplitude, double &fwhm,
                               double &chiSq);

    double computeModelValue(double x, double center, double amplitude, double fwhm);

    LineType m_lineType;
    DetectionMethod m_detectionMethod;
    double m_noiseThreshold;
    double m_minLineWidth;
    double m_maxLineWidth;
    int m_maxLines;
    int m_continuumOrder;
};

#endif // SPECTRALLINEFITTER_H
