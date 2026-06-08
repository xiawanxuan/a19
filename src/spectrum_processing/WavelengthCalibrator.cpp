#include "WavelengthCalibrator.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <algorithm>
#include <QDebug>

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
    std::sort(m_points.begin(), m_points.end(),
              [](const CalibrationPoint &a, const CalibrationPoint &b) {
        return a.pixel < b.pixel;
    });
    m_dirty = true;
    emit calibrationUpdated();
}

void WavelengthCalibrator::addCalibrationPoint(double pixel, double wavelength,
                                                const QString &name)
{
    CalibrationPoint p;
    p.pixel = pixel;
    p.wavelength = wavelength;
    p.lineName = name;
    m_points.append(p);

    std::sort(m_points.begin(), m_points.end(),
              [](const CalibrationPoint &a, const CalibrationPoint &b) {
        return a.pixel < b.pixel;
    });

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
    m_splineCoefficients.clear();
    m_calibrationError = 0.0;
    m_dirty = true;
    emit calibrationUpdated();
}

void WavelengthCalibrator::setMethod(CalibrationMethod method)
{
    if (m_method != method) {
        m_method = method;
        m_dirty = true;
        emit calibrationUpdated();
    }
}

WavelengthCalibrator::CalibrationMethod WavelengthCalibrator::method() const
{
    return m_method;
}

void WavelengthCalibrator::setPolynomialOrder(int order)
{
    if (order < 1) order = 1;
    if (order > 10) order = 10;
    if (m_polynomialOrder != order) {
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

    QVector<SpectralLine> lines = spectrum.spectralLines();
    for (SpectralLine &line : lines) {
        double restWl = line.wavelength;
        line.wavelength = pixelToWavelength(restWl);
        if (line.restWavelength <= 0) {
            line.restWavelength = line.wavelength;
        }
    }
    spectrum.setSpectralLines(lines);

    return true;
}

bool WavelengthCalibrator::calibratePixelArray(QVector<double> &pixels)
{
    if (!isReady() || (m_dirty && !computeCoefficients())) {
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

    double wlLo = pixelToWavelength(lo);
    double wlHi = pixelToWavelength(hi);

    if (wlLo > wlHi) {
        std::swap(lo, hi);
        std::swap(wlLo, wlHi);
    }

    if (wavelength <= wlLo) {
        double dx = m_points[1].pixel - m_points[0].pixel;
        double dy = m_points[1].wavelength - m_points[0].wavelength;
        if (std::abs(dy) > 1e-20) {
            return m_points[0].pixel + (wavelength - m_points[0].wavelength) * dx / dy;
        }
        return lo;
    }
    if (wavelength >= wlHi) {
        int n = m_points.size();
        double dx = m_points[n-1].pixel - m_points[n-2].pixel;
        double dy = m_points[n-1].wavelength - m_points[n-2].wavelength;
        if (std::abs(dy) > 1e-20) {
            return m_points[n-1].pixel + (wavelength - m_points[n-1].wavelength) * dx / dy;
        }
        return hi;
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

    std::sort(m_points.begin(), m_points.end(),
              [](const CalibrationPoint &a, const CalibrationPoint &b) {
        return a.pixel < b.pixel;
    });

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
    if (m_method == Spline && m_points.size() < 4) return false;
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
        if (std::abs(denom) < 1e-20) return false;

        double slope = (n * sumXY - sumX * sumY) / denom;
        double intercept = (sumY - slope * sumX) / n;

        m_coefficients.clear();
        m_coefficients.append(intercept);
        m_coefficients.append(slope);

        m_xMean = 0.0;
        m_xScale = 1.0;

        for (int i = 0; i < n; ++i) {
            double fitted = intercept + slope * x[i];
            totalError += (y[i] - fitted) * (y[i] - fitted);
        }
    } else if (m_method == Polynomial) {
        int order = qMin(m_polynomialOrder, n - 1);

        double xMean = 0.0, xMin = x[0], xMax = x[0];
        for (int i = 0; i < n; ++i) {
            xMean += x[i];
            if (x[i] < xMin) xMin = x[i];
            if (x[i] > xMax) xMax = x[i];
        }
        xMean /= n;
        double xScale = (xMax - xMin) * 0.5;
        if (xScale < 1e-20) xScale = 1.0;

        m_xMean = xMean;
        m_xScale = xScale;

        QVector<double> xn(n);
        for (int i = 0; i < n; ++i) {
            xn[i] = (x[i] - xMean) / xScale;
        }

        QVector<QVector<double>> V(n, QVector<double>(order + 1, 0.0));
        QVector<double> c(order + 1, 0.0);
        QVector<double> dp(order + 1, 0.0);

        for (int i = 0; i < n; ++i) {
            V[i][0] = 1.0;
            if (order >= 1) {
                V[i][1] = xn[i];
            }
            for (int j = 2; j <= order; ++j) {
                V[i][j] = xn[i] * V[i][j-1];
            }
        }

        QVector<QVector<double>> A(order + 1, QVector<double>(order + 1, 0.0));
        QVector<double> b(order + 1, 0.0);

        for (int i = 0; i <= order; ++i) {
            for (int j = 0; j <= order; ++j) {
                double sum = 0.0;
                for (int k = 0; k < n; ++k) {
                    sum += V[k][i] * V[k][j];
                }
                A[i][j] = sum;
            }
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += V[k][i] * y[k];
            }
            b[i] = sum;
        }

        QVector<int> pivot(order + 1);
        for (int i = 0; i <= order; ++i) pivot[i] = i;

        for (int k = 0; k <= order; ++k) {
            int pivotRow = k;
            double maxVal = std::abs(A[k][k]);
            for (int i = k + 1; i <= order; ++i) {
                if (std::abs(A[i][k]) > maxVal) {
                    maxVal = std::abs(A[i][k]);
                    pivotRow = i;
                }
            }
            if (pivotRow != k) {
                std::swap(A[k], A[pivotRow]);
                std::swap(b[k], b[pivotRow]);
                std::swap(pivot[k], pivot[pivotRow]);
            }

            if (std::abs(A[k][k]) < 1e-15) {
                return false;
            }

            for (int i = k + 1; i <= order; ++i) {
                double factor = A[i][k] / A[k][k];
                for (int j = k; j <= order; ++j) {
                    A[i][j] -= factor * A[k][j];
                }
                b[i] -= factor * b[k];
            }
        }

        QVector<double> cNorm(order + 1, 0.0);
        for (int i = order; i >= 0; --i) {
            double sum = b[i];
            for (int j = i + 1; j <= order; ++j) {
                sum -= A[i][j] * cNorm[j];
            }
            cNorm[i] = sum / A[i][i];
        }

        m_coefficients = QVector<double>(order + 1, 0.0);
        QVector<double> powXScale(order + 1, 1.0);
        for (int i = 1; i <= order; ++i) {
            powXScale[i] = powXScale[i-1] * xScale;
        }

        QVector<QVector<double>> binom(order + 1, QVector<double>(order + 1, 0.0));
        for (int i = 0; i <= order; ++i) {
            binom[i][0] = 1.0;
            binom[i][i] = 1.0;
            for (int j = 1; j < i; ++j) {
                binom[i][j] = binom[i-1][j-1] + binom[i-1][j];
            }
        }

        for (int i = 0; i <= order; ++i) {
            double term = 0.0;
            for (int k = i; k <= order; ++k) {
                double sign = ((k - i) % 2 == 0) ? 1.0 : -1.0;
                term += cNorm[k] * binom[k][i] * sign *
                        std::pow(xMean, k - i) / powXScale[k];
            }
            m_coefficients[i] = term;
        }

        for (int i = 0; i < n; ++i) {
            double fitted = 0.0;
            double px = 1.0;
            for (int j = 0; j <= order; ++j) {
                fitted += m_coefficients[j] * px;
                px *= x[i];
            }
            totalError += (y[i] - fitted) * (y[i] - fitted);
        }
    } else if (m_method == Spline) {
        m_splineCoefficients.clear();

        int nPts = m_points.size();
        if (nPts < 4) {
            m_method = Linear;
            return computeCoefficients();
        }

        QVector<double> h(nPts - 1, 0.0);
        for (int i = 0; i < nPts - 1; ++i) {
            h[i] = x[i+1] - x[i];
            if (std::abs(h[i]) < 1e-20) return false;
        }

        QVector<double> alpha(nPts - 1, 0.0);
        for (int i = 1; i < nPts - 1; ++i) {
            alpha[i] = 3.0 * (y[i+1] - y[i]) / h[i] -
                        3.0 * (y[i] - y[i-1]) / h[i-1];
        }

        QVector<double> l(nPts, 0.0);
        QVector<double> mu(nPts, 0.0);
        QVector<double> z(nPts, 0.0);
        QVector<double> c(nPts, 0.0);

        l[0] = 1.0;
        mu[0] = 0.0;
        z[0] = 0.0;

        for (int i = 1; i < nPts - 1; ++i) {
            l[i] = 2.0 * (x[i+1] - x[i-1]) - h[i-1] * mu[i-1];
            if (std::abs(l[i]) < 1e-20) return false;
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i-1] * z[i-1]) / l[i];
        }

        l[nPts-1] = 1.0;
        z[nPts-1] = 0.0;
        c[nPts-1] = 0.0;

        for (int j = nPts - 2; j >= 0; --j) {
            c[j] = z[j] - mu[j] * c[j+1];
        }

        QVector<double> b(nPts - 1, 0.0);
        QVector<double> d(nPts - 1, 0.0);
        for (int i = 0; i < nPts - 1; ++i) {
            b[i] = (y[i+1] - y[i]) / h[i] - h[i] * (c[i+1] + 2.0 * c[i]) / 3.0;
            d[i] = (c[i+1] - c[i]) / (3.0 * h[i]);
        }

        for (int i = 0; i < nPts - 1; ++i) {
            SplineCoeffs coeffs;
            coeffs.x0 = x[i];
            coeffs.a = y[i];
            coeffs.b = b[i];
            coeffs.c = c[i];
            coeffs.d = d[i];
            m_splineCoefficients.append(coeffs);
        }

        for (int i = 0; i < n; ++i) {
            double fitted = splineFit(x[i]);
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
    int n = m_points.size();
    if (n < 4) return linearFit(pixel);

    if (pixel <= m_points.first().pixel) {
        double dx = m_points[1].pixel - m_points[0].pixel;
        double dy = m_points[1].wavelength - m_points[0].wavelength;
        if (std::abs(dx) > 1e-20) {
            return m_points[0].wavelength + (pixel - m_points[0].pixel) * dy / dx;
        }
        return m_points[0].wavelength;
    }
    if (pixel >= m_points.last().pixel) {
        double dx = m_points[n-1].pixel - m_points[n-2].pixel;
        double dy = m_points[n-1].wavelength - m_points[n-2].wavelength;
        if (std::abs(dx) > 1e-20) {
            return m_points[n-1].wavelength + (pixel - m_points[n-1].pixel) * dy / dx;
        }
        return m_points[n-1].wavelength;
    }

    int lo = 0, hi = n - 1;
    while (lo < hi - 1) {
        int mid = (lo + hi) / 2;
        if (pixel >= m_points[mid].pixel) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    int idx = lo;

    if (idx >= m_splineCoefficients.size()) {
        idx = m_splineCoefficients.size() - 1;
    }

    const SplineCoeffs &coeffs = m_splineCoefficients[idx];
    double dx = pixel - coeffs.x0;
    return coeffs.a + coeffs.b * dx + coeffs.c * dx * dx + coeffs.d * dx * dx * dx;
}
