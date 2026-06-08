#include "RedshiftCalculator.h"
#include <cmath>
#include <algorithm>
#include <QtMath>
#include <QDebug>

RedshiftCalculator::RedshiftCalculator(QObject *parent)
    : QObject(parent)
    , m_method(CrossCorrelation)
    , m_minZ(-0.1)
    , m_maxZ(10.0)
    , m_searchStep(0.001)
    , m_correlationCoefficient(0.0)
    , m_redshiftError(0.0)
{
}

void RedshiftCalculator::setMethod(Method method)
{
    m_method = method;
}

RedshiftCalculator::Method RedshiftCalculator::method() const
{
    return m_method;
}

void RedshiftCalculator::setRedshiftRange(double minZ, double maxZ)
{
    m_minZ = minZ;
    m_maxZ = maxZ;
}

double RedshiftCalculator::minRedshift() const
{
    return m_minZ;
}

double RedshiftCalculator::maxRedshift() const
{
    return m_maxZ;
}

void RedshiftCalculator::setSearchStep(double step)
{
    if (step > 0) {
        m_searchStep = step;
    }
}

double RedshiftCalculator::searchStep() const
{
    return m_searchStep;
}

double RedshiftCalculator::calculateRedshift(const SpectrumData &spectrum)
{
    if (!spectrum.isValid()) return 0.0;

    if (m_method == LineMatching) {
        return calculateRedshiftFromLines(spectrum.spectralLines(),
                                          QVector<double>());
    }

    QMap<QString, double> restLines = standardRestLines();
    QVector<double> restWavelengths;
    for (auto it = restLines.begin(); it != restLines.end(); ++it) {
        restWavelengths.append(it.value());
    }
    std::sort(restWavelengths.begin(), restWavelengths.end());

    QVector<double> tempWl;
    QVector<double> tempFlux;

    double minWl = spectrum.minWavelength() / (1 + m_maxZ);
    double maxWl = spectrum.maxWavelength() / (1 + m_minZ);
    double step = 1.0;

    for (double wl = minWl; wl <= maxWl; wl += step) {
        tempWl.append(wl);
        double flux = 1.0;
        for (double restWl : restWavelengths) {
            double sigma = 5.0;
            flux -= 0.3 * std::exp(-(wl - restWl) * (wl - restWl) / (2 * sigma * sigma));
        }
        tempFlux.append(flux);
    }

    return calculateRedshift(spectrum.wavelengths(), spectrum.flux(),
                             tempWl, tempFlux);
}

double RedshiftCalculator::calculateRedshift(const QVector<double> &wavelengths,
                                             const QVector<double> &flux,
                                             const QVector<double> &templateWavelengths,
                                             const QVector<double> &templateFlux)
{
    if (wavelengths.size() < 10 || templateWavelengths.size() < 10) {
        return 0.0;
    }

    return crossCorrelationMethod(wavelengths, flux, templateWavelengths, templateFlux);
}

double RedshiftCalculator::calculateRedshiftFromLines(const QVector<SpectralLine> &observedLines,
                                                      const QVector<double> &restWavelengths)
{
    if (observedLines.size() < 2) return 0.0;

    QMap<QString, double> restLines = standardRestLines();
    QVector<double> restWls;

    if (restWavelengths.isEmpty()) {
        for (auto it = restLines.begin(); it != restLines.end(); ++it) {
            restWls.append(it.value());
        }
    } else {
        restWls = restWavelengths;
    }

    std::sort(restWls.begin(), restWls.end());

    QVector<double> observedWls;
    for (const SpectralLine &line : observedLines) {
        observedWls.append(line.wavelength);
    }
    std::sort(observedWls.begin(), observedWls.end());

    double bestZ = 0.0;
    double bestScore = 0.0;

    int nSteps = static_cast<int>((m_maxZ - m_minZ) / m_searchStep) + 1;
    nSteps = qMin(nSteps, 10000);
    double actualStep = (m_maxZ - m_minZ) / (nSteps - 1);

    m_redshiftArray.resize(nSteps);
    m_correlationCurve.resize(nSteps);

    for (int i = 0; i < nSteps; ++i) {
        double z = m_minZ + i * actualStep;
        m_redshiftArray[i] = z;

        int matches = 0;
        double tolerance = 0.02;

        for (double obsWl : observedWls) {
            double shiftedWl = obsWl / (1 + z);

            for (double restWl : restWls) {
                if (std::abs(shiftedWl - restWl) / restWl < tolerance) {
                    matches++;
                    break;
                }
            }
        }

        double score = static_cast<double>(matches) / observedWls.size();
        m_correlationCurve[i] = score;

        if (score > bestScore) {
            bestScore = score;
            bestZ = z;
        }
    }

    m_correlationCoefficient = bestScore;
    m_redshiftError = actualStep * 3;

    return bestZ;
}

double RedshiftCalculator::redshiftToVelocity(double z)
{
    const double c = 299792.458;
    if (z < 0.1) {
        return c * z;
    }
    double beta = ((1 + z) * (1 + z) - 1) / ((1 + z) * (1 + z) + 1);
    return c * beta;
}

double RedshiftCalculator::velocityToRedshift(double v)
{
    const double c = 299792.458;
    double beta = v / c;
    return std::sqrt((1 + beta) / (1 - beta)) - 1;
}

double RedshiftCalculator::redshiftToDistance(double z, double H0, double OmegaM, double OmegaL)
{
    const double c = 299792.458;

    if (z < 0.01) {
        return c * z / H0;
    }

    int nSteps = 1000;
    double dz = z / nSteps;
    double integral = 0.0;

    for (int i = 0; i < nSteps; ++i) {
        double zPrime = (i + 0.5) * dz;
        double E = std::sqrt(OmegaM * std::pow(1 + zPrime, 3) + OmegaL);
        integral += dz / E;
    }

    double dH = c / H0;
    return dH * integral;
}

double RedshiftCalculator::distanceToRedshift(double distance, double H0, double OmegaM, double OmegaL)
{
    const double c = 299792.458;

    if (distance < c * 0.01 / H0) {
        return distance * H0 / c;
    }

    double zMin = 0.0;
    double zMax = 10.0;

    for (int iter = 0; iter < 100; ++iter) {
        double zMid = (zMin + zMax) / 2.0;
        double d = redshiftToDistance(zMid, H0, OmegaM, OmegaL);

        if (std::abs(d - distance) / distance < 1e-6) {
            return zMid;
        }

        if (d < distance) {
            zMin = zMid;
        } else {
            zMax = zMid;
        }
    }

    return (zMin + zMax) / 2.0;
}

double RedshiftCalculator::dopplerShift(double restWavelength, double z)
{
    return restWavelength * (1 + z);
}

double RedshiftCalculator::restFromObserved(double observedWavelength, double z)
{
    if (z == -1) return observedWavelength;
    return observedWavelength / (1 + z);
}

QMap<QString, double> RedshiftCalculator::standardRestLines()
{
    QMap<QString, double> lines;

    lines["Ly-alpha"] = 1215.67;
    lines["N V 1240"] = 1240.0;
    lines["Si IV 1397"] = 1397.61;
    lines["C IV 1549"] = 1549.0;
    lines["He II 1640"] = 1640.4;
    lines["C III] 1909"] = 1909.0;
    lines["Mg II 2798"] = 2798.75;
    lines["[O II] 3727"] = 3727.0;
    lines["[Ne III] 3869"] = 3868.76;
    lines["H-delta 4102"] = 4101.73;
    lines["H-gamma 4340"] = 4340.47;
    lines["H-beta 4861"] = 4861.33;
    lines["[O III] 4959"] = 4958.91;
    lines["[O III] 5007"] = 5006.84;
    lines["He I 5876"] = 5875.62;
    lines["Na I 5890"] = 5889.95;
    lines["H-alpha 6563"] = 6562.79;
    lines["[S II] 6716"] = 6716.44;
    lines["[S II] 6731"] = 6730.82;
    lines["[N II] 6584"] = 6583.45;

    return lines;
}

double RedshiftCalculator::correlationCoefficient() const
{
    return m_correlationCoefficient;
}

double RedshiftCalculator::redshiftError() const
{
    return m_redshiftError;
}

QVector<double> RedshiftCalculator::correlationCurve() const
{
    return m_correlationCurve;
}

QVector<double> RedshiftCalculator::redshiftArray() const
{
    return m_redshiftArray;
}

bool RedshiftCalculator::applyRedshift(SpectrumData &spectrum, double z)
{
    if (!spectrum.isValid()) return false;

    QVector<double> wl = spectrum.wavelengths();
    for (double &w : wl) {
        w /= (1 + z);
    }
    spectrum.setWavelengths(wl);
    spectrum.setRedshift(z);

    QVector<SpectralLine> lines = spectrum.spectralLines();
    for (SpectralLine &line : lines) {
        line.restWavelength = line.wavelength / (1 + z);
        line.redshift = z;
    }
    spectrum.setSpectralLines(lines);

    return true;
}

double RedshiftCalculator::crossCorrelationMethod(const QVector<double> &wl, const QVector<double> &flux,
                                                  const QVector<double> &tempWl, const QVector<double> &tempFlux)
{
    int nSteps = static_cast<int>((m_maxZ - m_minZ) / m_searchStep) + 1;
    nSteps = qMin(nSteps, 10000);
    double actualStep = (m_maxZ - m_minZ) / (nSteps - 1);

    m_redshiftArray.resize(nSteps);
    m_correlationCurve.resize(nSteps);

    double meanFlux = 0.0;
    for (double f : flux) meanFlux += f;
    meanFlux /= flux.size();

    double varFlux = 0.0;
    for (double f : flux) {
        double d = f - meanFlux;
        varFlux += d * d;
    }
    varFlux = std::sqrt(varFlux);

    double meanTemp = 0.0;
    for (double f : tempFlux) meanTemp += f;
    meanTemp /= tempFlux.size();

    double varTemp = 0.0;
    for (double f : tempFlux) {
        double d = f - meanTemp;
        varTemp += d * d;
    }
    varTemp = std::sqrt(varTemp);

    double bestZ = 0.0;
    double bestCorr = -1.0;

    for (int i = 0; i < nSteps; ++i) {
        double z = m_minZ + i * actualStep;
        m_redshiftArray[i] = z;

        QVector<double> shiftedWl = tempWl;
        for (double &w : shiftedWl) {
            w *= (1 + z);
        }

        QVector<double> shiftedFlux = interpolate(shiftedWl, tempFlux, wl);

        double corr = 0.0;
        int nValid = 0;

        for (int j = 0; j < flux.size(); ++j) {
            if (!std::isnan(shiftedFlux[j]) && !std::isinf(shiftedFlux[j])) {
                corr += (flux[j] - meanFlux) * (shiftedFlux[j] - meanTemp);
                nValid++;
            }
        }

        if (nValid > 0 && varFlux > 0 && varTemp > 0) {
            corr /= (varFlux * varTemp / nValid);
            m_correlationCurve[i] = corr;

            if (corr > bestCorr) {
                bestCorr = corr;
                bestZ = z;
            }
        } else {
            m_correlationCurve[i] = 0.0;
        }

        if (i % 100 == 0) {
            emit calculationProgress(i + 1, nSteps);
        }
    }

    m_correlationCoefficient = bestCorr;

    int peakIdx = 0;
    double peakVal = bestCorr;
    for (int i = 0; i < nSteps; ++i) {
        if (m_correlationCurve[i] > peakVal) {
            peakVal = m_correlationCurve[i];
            peakIdx = i;
        }
    }

    if (peakIdx > 0 && peakIdx < nSteps - 1) {
        double y0 = m_correlationCurve[peakIdx - 1];
        double y1 = m_correlationCurve[peakIdx];
        double y2 = m_correlationCurve[peakIdx + 1];

        double xShift = 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2);
        bestZ = m_redshiftArray[peakIdx] + xShift * actualStep;
    }

    double halfMax = peakVal / 2.0;
    int leftIdx = peakIdx;
    while (leftIdx > 0 && m_correlationCurve[leftIdx] > halfMax) leftIdx--;
    int rightIdx = peakIdx;
    while (rightIdx < nSteps - 1 && m_correlationCurve[rightIdx] > halfMax) rightIdx++;

    m_redshiftError = (rightIdx - leftIdx) * actualStep / 2.355;

    emit calculationComplete(bestZ, bestCorr);
    return bestZ;
}

double RedshiftCalculator::findPeak(const QVector<double> &x, const QVector<double> &y, double &peakValue)
{
    if (x.isEmpty() || y.isEmpty() || x.size() != y.size()) {
        peakValue = 0.0;
        return 0.0;
    }

    int peakIdx = 0;
    peakValue = y[0];

    for (int i = 1; i < y.size(); ++i) {
        if (y[i] > peakValue) {
            peakValue = y[i];
            peakIdx = i;
        }
    }

    if (peakIdx > 0 && peakIdx < x.size() - 1) {
        double x0 = x[peakIdx - 1];
        double x1 = x[peakIdx];
        double x2 = x[peakIdx + 1];
        double y0 = y[peakIdx - 1];
        double y1 = y[peakIdx];
        double y2 = y[peakIdx + 1];

        double denom = 2 * (y0 - 2 * y1 + y2);
        if (std::abs(denom) > 1e-10) {
            double delta = (y0 - y2) * (x2 - x1) / denom;
            return x1 + delta;
        }
    }

    return x[peakIdx];
}

QVector<double> RedshiftCalculator::interpolate(const QVector<double> &x, const QVector<double> &y,
                                                 const QVector<double> &newX)
{
    QVector<double> result(newX.size(), 0.0);

    if (x.size() < 2 || x.size() != y.size()) {
        result.fill(std::numeric_limits<double>::quiet_NaN());
        return result;
    }

    for (int i = 0; i < newX.size(); ++i) {
        double xi = newX[i];

        if (xi < x.first() || xi > x.last()) {
            result[i] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }

        int lo = 0;
        int hi = x.size() - 1;

        while (lo < hi - 1) {
            int mid = (lo + hi) / 2;
            if (x[mid] < xi) {
                lo = mid;
            } else {
                hi = mid;
            }
        }

        double t = (xi - x[lo]) / (x[hi] - x[lo]);
        result[i] = y[lo] * (1 - t) + y[hi] * t;
    }

    return result;
}
