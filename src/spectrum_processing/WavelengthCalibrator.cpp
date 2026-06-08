#include "WavelengthCalibrator.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <algorithm>
#include <QtMath>

WavelengthCalibrator::WavelengthCalibrator(QObject *parent)
    : QObject(parent)
    , m_method(Linear)
    , m_polynomialOrder(3)
    , m_calibrationError(0.0)
    , m_dirty(true)
{
}

void WavelengthCalibrator::setCalibrationPoints(const QVector<CalibrationPoint> &points)
{
    m_points = points;
    m_dirty = true;
    emit calibrationUpdated();
}

void WavelengthCalibrator::addCalibrationPoint(double pixel, double wavelength, const QString &name)
{
    CalibrationPoint point;
    point.pixel = pixel;
    point.wavelength = wavelength;
    point.lineName = name;
    m_points.append(point);
    m_dirty = true;
    emit calibrationUpdated();
}

QVector<WavelengthCalibrator::CalibrationPoint> WavelengthCalibrator::calibrationPoints() const
{
    return m_points;
}

void WavelengthCalibrator::clearCalibrationPoints()
{
    m_points.clear();
    m_coefficients.clear();
    m_dirty = true;
    emit calibrationUpdated();
}

void WavelengthCalibrator::setMethod(CalibrationMethod method)
{
    m_method = method;
    m_dirty = true;
    emit calibrationUpdated();
}

WavelengthCalibrator::CalibrationMethod WavelengthCalibrator::method() const
{
    return m_method;
}

void WavelengthCalibrator::setPolynomialOrder(int order)
{
    if (order >= 1 && order <= 10) {
        m_polynomialOrder = order;
        m_dirty = true;
        emit calibrationUpdated();
    }
}

int WavelengthCalibrator::polynomialOrder() const
{
    return m_polynomialOrder;
}

bool WavelengthCalibrator::calibrate(SpectrumData &spectrum)
{
    if (!isReady()) {
        return false;
    }

    if (m_dirty) {
        if (!computeCoefficients()) {
            return false;
        }
    }

    QVector<double> pixels = spectrum.wavelengths();
    QVector<double> wavelengths;
    wavelengths.reserve(pixels.size());

    for (double pixel : pixels) {
        wavelengths.append(pixelToWavelength(pixel));
    }

    spectrum.setWavelengths(wavelengths);
    spectrum.setCalibrated(true);

    for (SpectralLine &line : spectrum.spectralLines()) {
        double restWl = line.wavelength;
        line.wavelength = pixelToWavelength(restWl);
        if (line.restWavelength <= 0) {
            line.restWavelength = line.wavelength;
        }
    }

    return true;
}

bool WavelengthCalibrator::calibratePixelArray(QVector<double> &pixels)
{
    if (!isReady() || m_dirty && !computeCoefficients()) {
        return false;
    }

    for (int i = 0; i < pixels.size(); ++i) {
        pixels[i] = pixelToWavelength(pixels[i]);
    }
    return true;
}

double WavelengthCalibrator::pixelToWavelength(double pixel) const
{
    if (!isReady()) return pixel;

    switch (m_method) {
    case Linear:
        return linearFit(pixel);
    case Polynomial:
        return polynomialFit(pixel);
    case Spline:
        return splineFit(pixel);
    default:
        return pixel;
    }
}

double WavelengthCalibrator::wavelengthToPixel(double wavelength) const
{
    if (!isReady() || m_points.size() < 2) return wavelength;

    double lo = m_points.first().pixel;
    double hi = m_points.last().pixel;

    if (m_method == Linear && m_points.size() == 2) {
        double p1 = m_points[0].pixel;
        double w1 = m_points[0].wavelength;
        double p2 = m_points[1].pixel;
        double w2 = m_points[1].wavelength;
        if (w2 != w1) {
            return p1 + (wavelength - w1) * (p2 - p1) / (w2 - w1);
        }
    }

    for (int iter = 0; iter < 100; ++iter) {
        double mid = (lo + hi) / 2;
        double midWl = pixelToWavelength(mid);
        if (std::abs(midWl - wavelength) < 1e-10) {
            return mid;
        }
        if (midWl < wavelength) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    return (lo + hi) / 2;
}

QVector<double> WavelengthCalibrator::calibrationCoefficients() const
{
    return m_coefficients;
}

double WavelengthCalibrator::calibrationError() const
{
    return m_calibrationError;
}

QMap<QString, double> WavelengthCalibrator::knownEmissionLines()
{
    QMap<QString, double> lines;

    lines["H I 4102"] = 4101.734;
    lines["H I 4340"] = 4340.472;
    lines["H I 4861"] = 4861.333;
    lines["H I 6563"] = 6562.79;
    lines["He I 4471"] = 4471.479;
    lines["He I 5876"] = 5875.624;
    lines["He II 4686"] = 4685.71;
    lines["C III 4647"] = 4647.42;
    lines["C IV 1550"] = 1550.77;
    lines["N III 4640"] = 4640.64;
    lines["N V 1240"] = 1240.0;
    lines["O III 5007"] = 5006.843;
    lines["O III 4959"] = 4958.911;
    lines["O I 6300"] = 6300.304;
    lines["Mg II 2798"] = 2798.75;
    lines["Si IV 1397"] = 1397.61;
    lines["Fe II 4584"] = 4583.83;
    lines["S II 6716"] = 6716.44;
    lines["S II 6731"] = 6730.82;
    lines["Ne III 3869"] = 3868.76;

    return lines;
}

QMap<QString, double> WavelengthCalibrator::knownAbsorptionLines()
{
    QMap<QString, double> lines;

    lines["H I 4102"] = 4101.734;
    lines["H I 4340"] = 4340.472;
    lines["H I 4861"] = 4861.333;
    lines["H I 6563"] = 6562.79;
    lines["Ca II K 3933"] = 3933.66;
    lines["Ca II H 3968"] = 3968.47;
    lines["Na I D1 5896"] = 5895.92;
    lines["Na I D2 5890"] = 5889.95;
    lines["Mg I 5176"] = 5176.7;
    lines["Fe I 4383"] = 4383.55;
    lines["Fe I 4668"] = 4668.14;
    lines["G-band 4305"] = 4305.0;
    lines["H&K 3950"] = 3950.0;
    lines["Mg II 2798"] = 2798.75;
    lines["H-beta 4861"] = 4861.333;
    lines["H-alpha 6563"] = 6562.79;
    lines["O I 7772"] = 7771.94;
    lines["O I 8446"] = 8446.76;

    return lines;
}

bool WavelengthCalibrator::loadCalibrationFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();

    m_method = static_cast<CalibrationMethod>(obj["method"].toInt(Linear));
    m_polynomialOrder = obj["polynomialOrder"].toInt(3);

    QJsonArray pointsArr = obj["points"].toArray();
    m_points.clear();

    for (const QJsonValue &val : pointsArr) {
        QJsonObject pObj = val.toObject();
        CalibrationPoint p;
        p.pixel = pObj["pixel"].toDouble();
        p.wavelength = pObj["wavelength"].toDouble();
        p.lineName = pObj["name"].toString();
        m_points.append(p);
    }

    m_dirty = true;
    emit calibrationUpdated();
    return true;
}

bool WavelengthCalibrator::saveCalibrationToFile(const QString &filePath)
{
    QJsonObject root;
    root["method"] = m_method;
    root["polynomialOrder"] = m_polynomialOrder;

    QJsonArray pointsArr;
    for (const CalibrationPoint &p : m_points) {
        QJsonObject pObj;
        pObj["pixel"] = p.pixel;
        pObj["wavelength"] = p.wavelength;
        pObj["name"] = p.lineName;
        pointsArr.append(pObj);
    }
    root["points"] = pointsArr;

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool WavelengthCalibrator::isReady() const
{
    if (m_points.size() < 2) return false;
    if (m_method == Polynomial && m_points.size() < m_polynomialOrder + 1) return false;
    return true;
}

bool WavelengthCalibrator::computeCoefficients()
{
    if (!isReady()) return false;

    int n = m_points.size();

    QVector<double> x(n), y(n);
    for (int i = 0; i < n; ++i) {
        x[i] = m_points[i].pixel;
        y[i] = m_points[i].wavelength;
    }

    double totalError = 0.0;

    if (m_method == Linear || m_points.size() == 2) {
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        for (int i = 0; i < n; ++i) {
            sumX += x[i];
            sumY += y[i];
            sumXY += x[i] * y[i];
            sumX2 += x[i] * x[i];
        }
        double denom = n * sumX2 - sumX * sumX;
        if (denom == 0) return false;

        double slope = (n * sumXY - sumX * sumY) / denom;
        double intercept = (sumY - slope * sumX) / n;

        m_coefficients.clear();
        m_coefficients.append(intercept);
        m_coefficients.append(slope);

        for (int i = 0; i < n; ++i) {
            double fitted = intercept + slope * x[i];
            totalError += (y[i] - fitted) * (y[i] - fitted);
        }
    } else if (m_method == Polynomial) {
        int order = qMin(m_polynomialOrder, n - 1);
        m_coefficients = QVector<double>(order + 1, 0.0);

        QVector<QVector<double>> A(order + 1, QVector<double>(order + 1, 0.0));
        QVector<double> b(order + 1, 0.0);

        QVector<double> xPowers(2 * order + 1, 0.0);
        for (int i = 0; i < n; ++i) {
            double px = 1.0;
            for (int j = 0; j <= 2 * order; ++j) {
                xPowers[j] += px;
                px *= x[i];
            }
        }

        for (int i = 0; i <= order; ++i) {
            for (int j = 0; j <= order; ++j) {
                A[i][j] = xPowers[i + j];
            }
        }

        for (int i = 0; i <= order; ++i) {
            for (int j = 0; j < n; ++j) {
                b[i] += y[j] * qPow(x[j], i);
            }
        }

        for (int k = 0; k < order; ++k) {
            int pivot = k;
            double maxVal = std::abs(A[k][k]);
            for (int i = k + 1; i <= order; ++i) {
                if (std::abs(A[i][k]) > maxVal) {
                    maxVal = std::abs(A[i][k]);
                    pivot = i;
                }
            }
            if (pivot != k) {
                std::swap(A[k], A[pivot]);
                std::swap(b[k], b[pivot]);
            }

            for (int i = k + 1; i <= order; ++i) {
                double factor = A[i][k] / A[k][k];
                for (int j = k; j <= order; ++j) {
                    A[i][j] -= factor * A[k][j];
                }
                b[i] -= factor * b[k];
            }
        }

        for (int i = order; i >= 0; --i) {
            double sum = b[i];
            for (int j = i + 1; j <= order; ++j) {
                sum -= A[i][j] * m_coefficients[j];
            }
            m_coefficients[i] = sum / A[i][i];
        }

        for (int i = 0; i < n; ++i) {
            double fitted = 0.0;
            for (int j = 0; j <= order; ++j) {
                fitted += m_coefficients[j] * qPow(x[i], j);
            }
            totalError += (y[i] - fitted) * (y[i] - fitted);
        }
    }

    if (n > 0) {
        m_calibrationError = std::sqrt(totalError / n);
    } else {
        m_calibrationError = 0.0;
    }

    emit calibrationErrorChanged(m_calibrationError);
    m_dirty = false;
    return true;
}

double WavelengthCalibrator::linearFit(double pixel) const
{
    if (m_coefficients.size() < 2) return pixel;
    return m_coefficients[0] + m_coefficients[1] * pixel;
}

double WavelengthCalibrator::polynomialFit(double pixel) const
{
    if (m_coefficients.isEmpty()) return pixel;

    double result = 0.0;
    double px = 1.0;
    for (double coef : m_coefficients) {
        result += coef * px;
        px *= pixel;
    }
    return result;
}

double WavelengthCalibrator::splineFit(double pixel) const
{
    if (m_points.size() < 4) return linearFit(pixel);

    int n = m_points.size();
    if (pixel <= m_points.first().pixel) {
        double dx = m_points[1].pixel - m_points[0].pixel;
        double dy = m_points[1].wavelength - m_points[0].wavelength;
        return m_points[0].wavelength + (pixel - m_points[0].pixel) * dy / dx;
    }
    if (pixel >= m_points.last().pixel) {
        double dx = m_points[n-1].pixel - m_points[n-2].pixel;
        double dy = m_points[n-1].wavelength - m_points[n-2].wavelength;
        return m_points[n-1].wavelength + (pixel - m_points[n-1].pixel) * dy / dx;
    }

    for (int i = 0; i < n - 1; ++i) {
        if (pixel >= m_points[i].pixel && pixel <= m_points[i+1].pixel) {
            double x0 = m_points[i].pixel;
            double x1 = m_points[i+1].pixel;
            double y0 = m_points[i].wavelength;
            double y1 = m_points[i+1].wavelength;

            double t = (pixel - x0) / (x1 - x0);
            double y = y0 * (1 - t) + y1 * t;

            if (i > 0 && i < n - 2) {
                double x_1 = m_points[i-1].pixel;
                double x2 = m_points[i+2].pixel;
                double y_1 = m_points[i-1].wavelength;
                double y2 = m_points[i+2].wavelength;

                double d0 = (y1 - y_1) / (x1 - x_1) * (x1 - x0) / 6.0;
                double d1 = (y2 - y0) / (x2 - x0) * (x1 - x0) / 6.0;

                y += (3*t*t - 2*t*t*t - 1) * d0 + (t*t*t - t*t) * d1;
            }

            return y;
        }
    }

    return pixel;
}
