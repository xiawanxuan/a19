#ifndef REDSHIFTCALCULATOR_H
#define REDSHIFTCALCULATOR_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include "data_parser/SpectrumData.h"

class RedshiftCalculator : public QObject
{
    Q_OBJECT

public:
    explicit RedshiftCalculator(QObject *parent = nullptr);

    enum Method {
        CrossCorrelation,
        LineMatching,
        TemplateFitting
    };

    void setMethod(Method method);
    Method method() const;

    void setRedshiftRange(double minZ, double maxZ);
    double minRedshift() const;
    double maxRedshift() const;

    void setSearchStep(double step);
    double searchStep() const;

    double calculateRedshift(const SpectrumData &spectrum);
    double calculateRedshift(const QVector<double> &wavelengths,
                             const QVector<double> &flux,
                             const QVector<double> &templateWavelengths,
                             const QVector<double> &templateFlux);

    double calculateRedshiftFromLines(const QVector<SpectralLine> &observedLines,
                                      const QVector<double> &restWavelengths);

    static double redshiftToVelocity(double z);
    static double velocityToRedshift(double v);

    static double redshiftToDistance(double z, double H0 = 70.0, double OmegaM = 0.3, double OmegaL = 0.7);
    static double distanceToRedshift(double distance, double H0 = 70.0, double OmegaM = 0.3, double OmegaL = 0.7);

    static double dopplerShift(double restWavelength, double z);
    static double restFromObserved(double observedWavelength, double z);

    static QMap<QString, double> standardRestLines();

    double correlationCoefficient() const;
    double redshiftError() const;

    QVector<double> correlationCurve() const;
    QVector<double> redshiftArray() const;

    bool applyRedshift(SpectrumData &spectrum, double z);

signals:
    void calculationProgress(int current, int total);
    void calculationComplete(double redshift, double confidence);

private:
    double crossCorrelationMethod(const QVector<double> &wl, const QVector<double> &flux,
                                  const QVector<double> &tempWl, const QVector<double> &tempFlux);

    double lineMatchingMethod(const QVector<SpectralLine> &lines);

    double findPeak(const QVector<double> &x, const QVector<double> &y, double &peakValue);

    QVector<double> interpolate(const QVector<double> &x, const QVector<double> &y,
                                const QVector<double> &newX);

    Method m_method;
    double m_minZ;
    double m_maxZ;
    double m_searchStep;
    double m_correlationCoefficient;
    double m_redshiftError;
    QVector<double> m_correlationCurve;
    QVector<double> m_redshiftArray;
};

#endif // REDSHIFTCALCULATOR_H
