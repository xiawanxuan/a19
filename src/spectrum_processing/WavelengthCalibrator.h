#ifndef WAVELENGTHCALIBRATOR_H
#define WAVELENGTHCALIBRATOR_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QPair>
#include "data_parser/SpectrumData.h"

class WavelengthCalibrator : public QObject
{
    Q_OBJECT

public:
    explicit WavelengthCalibrator(QObject *parent = nullptr);

    struct CalibrationPoint {
        double pixel;
        double wavelength;
        QString lineName;
    };

    enum CalibrationMethod {
        Linear,
        Polynomial,
        Spline
    };

    void setCalibrationPoints(const QVector<CalibrationPoint> &points);
    void addCalibrationPoint(double pixel, double wavelength, const QString &name = QString());
    QVector<CalibrationPoint> calibrationPoints() const;
    void clearCalibrationPoints();

    void setMethod(CalibrationMethod method);
    CalibrationMethod method() const;

    void setPolynomialOrder(int order);
    int polynomialOrder() const;

    bool calibrate(SpectrumData &spectrum);
    bool calibratePixelArray(QVector<double> &pixels);

    double pixelToWavelength(double pixel) const;
    double wavelengthToPixel(double wavelength) const;

    QVector<double> calibrationCoefficients() const;

    double calibrationError() const;

    static QMap<QString, double> knownEmissionLines();
    static QMap<QString, double> knownAbsorptionLines();

    bool loadCalibrationFromFile(const QString &filePath);
    bool saveCalibrationToFile(const QString &filePath);

    bool isReady() const;

signals:
    void calibrationUpdated();
    void calibrationErrorChanged(double error);

private:
    bool computeCoefficients();
    double linearFit(double pixel) const;
    double polynomialFit(double pixel) const;
    double splineFit(double pixel) const;

    struct SplineCoeffs {
        double x0;
        double a;
        double b;
        double c;
        double d;
    };

    QVector<CalibrationPoint> m_points;
    CalibrationMethod m_method;
    int m_polynomialOrder;
    QVector<double> m_coefficients;
    QVector<SplineCoeffs> m_splineCoefficients;
    double m_calibrationError;
    bool m_dirty;
    double m_xMean;
    double m_xScale;
};

#endif // WAVELENGTHCALIBRATOR_H
