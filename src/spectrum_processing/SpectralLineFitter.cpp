#include "SpectralLineFitter.h"
#include <cmath>
#include <algorithm>
#include <QtMath>
#include <QDebug>

SpectralLineFitter::SpectralLineFitter(QObject *parent)
    : QObject(parent)
    , m_lineType(Gaussian)
    , m_detectionMethod(ContinuumSubtraction)
    , m_noiseThreshold(3.0)
    , m_minLineWidth(1.0)
    , m_maxLineWidth(100.0)
    , m_maxLines(100)
    , m_continuumOrder(5)
{
}

void SpectralLineFitter::setLineType(LineType type)
{
    m_lineType = type;
}

SpectralLineFitter::LineType SpectralLineFitter::lineType() const
{
    return m_lineType;
}

void SpectralLineFitter::setDetectionMethod(DetectionMethod method)
{
    m_detectionMethod = method;
}

SpectralLineFitter::DetectionMethod SpectralLineFitter::detectionMethod() const
{
    return m_detectionMethod;
}

void SpectralLineFitter::setNoiseThreshold(double sigma)
{
    m_noiseThreshold = sigma;
}

double SpectralLineFitter::noiseThreshold() const
{
    return m_noiseThreshold;
}

void SpectralLineFitter::setMinLineWidth(double minWidth)
{
    m_minLineWidth = minWidth;
}

double SpectralLineFitter::minLineWidth() const
{
    return m_minLineWidth;
}

void SpectralLineFitter::setMaxLineWidth(double maxWidth)
{
    m_maxLineWidth = maxWidth;
}

double SpectralLineFitter::maxLineWidth() const
{
    return m_maxLineWidth;
}

void SpectralLineFitter::setMaxLines(int maxLines)
{
    m_maxLines = maxLines;
}

int SpectralLineFitter::maxLines() const
{
    return m_maxLines;
}

void SpectralLineFitter::setContinuumOrder(int order)
{
    m_continuumOrder = order;
}

int SpectralLineFitter::continuumOrder() const
{
    return m_continuumOrder;
}

QVector<SpectralLine> SpectralLineFitter::detectLines(const SpectrumData &spectrum)
{
    if (!spectrum.isValid()) return QVector<SpectralLine>();

    return detectLinesInRange(spectrum, spectrum.minWavelength(), spectrum.maxWavelength());
}

QVector<SpectralLine> SpectralLineFitter::detectLinesInRange(const SpectrumData &spectrum, double wlStart, double wlEnd)
{
    if (!spectrum.isValid()) return QVector<SpectralLine>();

    QVector<double> wavelengths = spectrum.wavelengths();
    QVector<double> flux = spectrum.flux();

    int startIdx = spectrum.indexAtWavelength(wlStart);
    int endIdx = spectrum.indexAtWavelength(wlEnd);
    if (startIdx >= endIdx) return QVector<SpectralLine>();

    QVector<double> subWl = wavelengths.mid(startIdx, endIdx - startIdx + 1);
    QVector<double> subFlux = flux.mid(startIdx, endIdx - startIdx + 1);

    QVector<double> continuum = fitContinuum(subWl, subFlux);

    QVector<double> residuals(subFlux.size());
    for (int i = 0; i < subFlux.size(); ++i) {
        residuals[i] = subFlux[i] - continuum[i];
    }

    double noise = 0.0;
    for (double r : residuals) {
        noise += r * r;
    }
    noise = std::sqrt(noise / residuals.size());

    double threshold = m_noiseThreshold * noise;

    int minDistance = qMax(3, static_cast<int>(m_minLineWidth / (subWl.last() - subWl.first()) * subWl.size()));

    QVector<int> peakIndices = findPeaks(residuals, minDistance, threshold);
    QVector<int> valleyIndices = findValleys(residuals, minDistance, -threshold);

    QVector<SpectralLine> lines;

    for (int idx : peakIndices) {
        if (lines.size() >= m_maxLines) break;

        double wl = subWl[idx];
        double peakFlux = subFlux[idx];
        double cont = continuum[idx];
        double height = residuals[idx];

        double fwhm = 0.0;
        double halfMax = cont + height / 2.0;

        int leftIdx = idx;
        while (leftIdx > 0 && subFlux[leftIdx] > halfMax) leftIdx--;
        int rightIdx = idx;
        while (rightIdx < subFlux.size() - 1 && subFlux[rightIdx] > halfMax) rightIdx++;

        if (rightIdx > leftIdx && leftIdx >= 0 && rightIdx < subWl.size()) {
            fwhm = subWl[rightIdx] - subWl[leftIdx];
        }

        if (fwhm >= m_minLineWidth && fwhm <= m_maxLineWidth) {
            SpectralLine line;
            line.wavelength = wl;
            line.flux = peakFlux;
            line.continuum = cont;
            line.peakHeight = height;
            line.fwhm = fwhm;
            line.isEmission = true;
            line.name = QString("Emission %1").arg(wl, 0, 'f', 2);
            line.restWavelength = wl;

            double ew = equivalentWidth(subWl, subFlux, continuum, wl, fwhm * 3);
            line.equivalentWidth = ew;

            lines.append(line);
        }
    }

    for (int idx : valleyIndices) {
        if (lines.size() >= m_maxLines * 2) break;

        double wl = subWl[idx];
        double valleyFlux = subFlux[idx];
        double cont = continuum[idx];
        double depth = std::abs(residuals[idx]);

        double fwhm = 0.0;
        double halfMin = cont - depth / 2.0;

        int leftIdx = idx;
        while (leftIdx > 0 && subFlux[leftIdx] < halfMin) leftIdx--;
        int rightIdx = idx;
        while (rightIdx < subFlux.size() - 1 && subFlux[rightIdx] < halfMin) rightIdx++;

        if (rightIdx > leftIdx && leftIdx >= 0 && rightIdx < subWl.size()) {
            fwhm = subWl[rightIdx] - subWl[leftIdx];
        }

        if (fwhm >= m_minLineWidth && fwhm <= m_maxLineWidth) {
            SpectralLine line;
            line.wavelength = wl;
            line.flux = valleyFlux;
            line.continuum = cont;
            line.peakHeight = -depth;
            line.fwhm = fwhm;
            line.isEmission = false;
            line.name = QString("Absorption %1").arg(wl, 0, 'f', 2);
            line.restWavelength = wl;

            double ew = equivalentWidth(subWl, subFlux, continuum, wl, fwhm * 3);
            line.equivalentWidth = ew;

            lines.append(line);
        }
    }

    std::sort(lines.begin(), lines.end(), [](const SpectralLine &a, const SpectralLine &b) {
        return a.wavelength < b.wavelength;
    });

    return lines;
}

SpectralLineFitter::FitResult SpectralLineFitter::fitLine(const SpectrumData &spectrum, double wavelengthGuess)
{
    int idx = spectrum.indexAtWavelength(wavelengthGuess);
    if (idx < 0) {
        FitResult result;
        result.success = false;
        return result;
    }

    double fluxVal = spectrum.flux()[idx];
    double contEst = fluxVal * 0.9;

    int windowHalf = 50;
    int start = qMax(0, idx - windowHalf);
    int end = qMin(spectrum.size() - 1, idx + windowHalf);

    QVector<double> wl = spectrum.wavelengths().mid(start, end - start + 1);
    QVector<double> flux = spectrum.flux().mid(start, end - start + 1);

    double contLeft = spectrum.flux()[start];
    double contRight = spectrum.flux()[end];
    double amplitude = fluxVal - (contLeft + contRight) / 2.0;

    double fwhmGuess = (spectrum.wavelengths().at(qMin(idx + 5, spectrum.size() - 1)) -
                      spectrum.wavelengths().at(qMax(idx - 5, 0)));

    return fitLine(wl, flux, wavelengthGuess, amplitude, fwhmGuess);
}

SpectralLineFitter::FitResult SpectralLineFitter::fitLine(const QVector<double> &wavelengths,
                                                      const QVector<double> &flux,
                                                      double centerGuess,
                                                      double amplitudeGuess,
                                                      double fwhmGuess)
{
    FitResult result;
    result.success = false;
    result.center = centerGuess;
    result.amplitude = amplitudeGuess;
    result.fwhm = fwhmGuess;
    result.height = amplitudeGuess;
    result.lineType = m_lineType;

    if (wavelengths.size() < 5 || wavelengths.size() != flux.size()) {
        return result;
    }

    double center = centerGuess;
    double amplitude = amplitudeGuess;
    double fwhm = fwhmGuess;
    double chiSq = 0.0;

    if (fwhm <= 0) fwhm = (wavelengths.last() - wavelengths.first()) / 10.0;

    bool success = levenbergMarquardtFit(wavelengths, flux, center, amplitude, fwhm, chiSq);

    result.success = success;
    result.center = center;
    result.amplitude = amplitude;
    result.fwhm = fwhm;
    result.height = amplitude;
    result.chiSquared = chiSq;

    if (wavelengths.size() > 3) {
        result.redChiSquared = chiSq / (wavelengths.size() - 3);
    }

    if (m_lineType == Gaussian) {
        result.area = amplitude * fwhm * std::sqrt(M_PI / std::log(2.0)) / 2.0;
    } else if (m_lineType == Lorentzian) {
        result.area = M_PI * amplitude * fwhm / 2.0;
    } else if (m_lineType == Voigt) {
        double voigtFraction = 0.5;
        double areaGauss = amplitude * (1.0 - voigtFraction) * fwhm * std::sqrt(M_PI / std::log(2.0)) / 2.0;
        double areaLorentz = amplitude * voigtFraction * M_PI * fwhm / 2.0;
        result.area = areaGauss + areaLorentz;
    }

    result.residuals.resize(wavelengths.size());
    for (int i = 0; i < wavelengths.size(); ++i) {
        result.residuals[i] = flux[i] - computeModelValue(wavelengths[i], center, amplitude, fwhm);
    }

    return result;
}

bool SpectralLineFitter::fitAllLines(SpectrumData &spectrum)
{
    QVector<SpectralLine> lines = detectLines(spectrum);
    if (lines.isEmpty()) return false;

    QVector<SpectralLine> fittedLines;

    for (int i = 0; i < lines.size(); ++i) {
        emit fittingProgress(i + 1, lines.size());

        const SpectralLine &line = lines[i];

        FitResult fit = fitLine(spectrum, line.wavelength);

        SpectralLine fitted = line;
        if (fit.success) {
            fitted.wavelength = fit.center;
            fitted.fwhm = fit.fwhm;
            fitted.peakHeight = fit.height;
        }

        fittedLines.append(fitted);
    }

    spectrum.setSpectralLines(fittedLines);
    emit fittingComplete(true);
    return true;
}

QVector<double> SpectralLineFitter::fitContinuum(const SpectrumData &spectrum)
{
    return fitContinuum(spectrum.wavelengths(), spectrum.flux());
}

QVector<double> SpectralLineFitter::fitContinuum(const QVector<double> &wavelengths, const QVector<double> &flux)
{
    int n = wavelengths.size();
    QVector<double> continuum(n, 0.0);

    if (n < 10) {
        double mean = 0.0;
        for (double f : flux) mean += f;
        mean /= n;
        continuum.fill(mean);
        return continuum;
    }

    int order = qMin(m_continuumOrder, n / 10);
    order = qMax(order, 1);

    QVector<int> sampleIndices;
    int step = n / (order * 3);
    step = qMax(step, 1);

    for (int i = 0; i < n; i += step) {
        sampleIndices.append(i);
    }
    if (sampleIndices.last() != n - 1) {
        sampleIndices.append(n - 1);
    }

    QVector<double> sampleX(sampleIndices.size());
    QVector<double> sampleY(sampleIndices.size());

    for (int i = 0; i < sampleIndices.size(); ++i) {
        int idx = sampleIndices[i];
        sampleX[i] = wavelengths[idx];
        sampleY[i] = flux[idx];
    }

    for (int iter = 0; iter < 3; ++iter) {
        QVector<double> newSampleX;
        QVector<double> newSampleY;

        QVector<double> tempCont(n);
        for (int i = 0; i < n; ++i) {
            if (i == 0) {
                tempCont[i] = sampleY.first();
            } else if (i == n - 1) {
                tempCont[i] = sampleY.last();
            } else {
                double frac = static_cast<double>(i) / (n - 1);
                int seg = static_cast<int>(frac * (sampleX.size() - 1));
                seg = qBound(0, seg, sampleX.size() - 2);
                double t = (wavelengths[i] - sampleX[seg]) / (sampleX[seg + 1] - sampleX[seg]);
                tempCont[i] = sampleY[seg] * (1 - t) + sampleY[seg + 1] * t;
            }
        }

        double meanDev = 0.0;
        for (int i = 0; i < n; ++i) {
            meanDev += std::abs(flux[i] - tempCont[i]);
        }
        meanDev /= n;

        for (int idx : sampleIndices) {
            if (std::abs(flux[idx] - tempCont[idx]) < 2.0 * meanDev) {
                newSampleX.append(wavelengths[idx]);
                newSampleY.append(flux[idx]);
            }
        }

        if (newSampleX.size() < 3) break;

        sampleX = newSampleX;
        sampleY = newSampleY;
    }

    for (int i = 0; i < n; ++i) {
        if (sampleX.size() < 2) {
            continuum[i] = sampleY.first();
            continue;
        }

        double x = wavelengths[i];

        if (x <= sampleX.first()) {
            double slope = (sampleY[1] - sampleY[0]) / (sampleX[1] - sampleX[0]);
            continuum[i] = sampleY.first() + slope * (x - sampleX.first());
        } else if (x >= sampleX.last()) {
            int m = sampleX.size();
            double slope = (sampleY[m-1] - sampleY[m-2]) / (sampleX[m-1] - sampleX[m-2]);
            continuum[i] = sampleY.last() + slope * (x - sampleX.last());
        } else {
            int seg = 0;
            for (int j = 0; j < sampleX.size() - 1; ++j) {
                if (x >= sampleX[j] && x <= sampleX[j + 1]) {
                    seg = j;
                    break;
                }
            }

            double t = (x - sampleX[seg]) / (sampleX[seg + 1] - sampleX[seg]);
            double y0 = sampleY[seg];
            double y1 = sampleY[seg + 1];

            double y_1 = (seg > 0) ? sampleY[seg - 1] : y0;
            double y2 = (seg < sampleX.size() - 2) ? sampleY[seg + 2] : y1;

            double a0 = y0;
            double a1 = 0.5 * (y1 - y_1);
            double a2 = y_1 - 2.5 * y0 + 2 * y1 - 0.5 * y2;
            double a3 = 0.5 * (y2 - y_1) + 1.5 * (y0 - y1);

            continuum[i] = a0 + a1 * t + a2 * t * t + a3 * t * t * t;
        }
    }

    return continuum;
}

double SpectralLineFitter::computeNoise(const SpectrumData &spectrum)
{
    if (spectrum.size() < 10) return 0.0;

    QVector<double> diff(spectrum.size() - 1);
    QVector<double> flux = spectrum.flux();

    for (int i = 0; i < flux.size() - 1; ++i) {
        diff[i] = flux[i + 1] - flux[i];
    }

    double mean = 0.0;
    for (double d : diff) mean += d;
    mean /= diff.size();

    double var = 0.0;
    for (double d : diff) {
        double dev = d - mean;
        var += dev * dev;
    }
    var /= diff.size();

    return std::sqrt(var) / std::sqrt(2.0);
}

double SpectralLineFitter::computeSNR(const SpectrumData &spectrum, double wavelength)
{
    double noise = computeNoise(spectrum);
    if (noise == 0) return 0.0;

    double flux = spectrum.fluxAtWavelength(wavelength);
    return flux / noise;
}

double SpectralLineFitter::gaussian(double x, double center, double amplitude, double fwhm)
{
    double sigma = fwhmToSigma(fwhm);
    double dx = x - center;
    return amplitude * std::exp(-dx * dx / (2.0 * sigma * sigma));
}

double SpectralLineFitter::lorentzian(double x, double center, double amplitude, double fwhm)
{
    double gamma = fwhm / 2.0;
    double dx = x - center;
    return amplitude / (1.0 + (dx * dx) / (gamma * gamma));
}

double SpectralLineFitter::voigt(double x, double center, double amplitude, double fwhm, double fraction)
{
    double g = gaussian(x, center, amplitude * (1 - fraction), fwhm);
    double l = lorentzian(x, center, amplitude * fraction, fwhm);
    return g + l;
}

double SpectralLineFitter::fwhmToSigma(double fwhm)
{
    return fwhm / (2.0 * std::sqrt(2.0 * std::log(2.0)));
}

double SpectralLineFitter::sigmaToFwhm(double sigma)
{
    return 2.0 * sigma * std::sqrt(2.0 * std::log(2.0));
}

double SpectralLineFitter::equivalentWidth(const QVector<double> &wavelengths,
                                            const QVector<double> &flux,
                                            const QVector<double> &continuum,
                                            double center, double window)
{
    if (wavelengths.size() < 2) return 0.0;

    double ew = 0.0;
    double wlMin = center - window / 2.0;
    double wlMax = center + window / 2.0;

    for (int i = 1; i < wavelengths.size(); ++i) {
        double wl0 = wavelengths[i - 1];
        double wl1 = wavelengths[i];

        if (wl1 < wlMin || wl0 > wlMax) continue;

        double dwl = wl1 - wl0;
        double f0 = flux[i - 1];
        double f1 = flux[i];
        double c0 = continuum[i - 1];
        double c1 = continuum[i];

        if (c0 > 0 && c1 > 0) {
            double frac0 = 1.0 - f0 / c0;
            double frac1 = 1.0 - f1 / c1;
            ew += 0.5 * (frac0 + frac1) * dwl;
        }
    }

    return ew;
}

QVector<int> SpectralLineFitter::findPeaks(const QVector<double> &data, int minDistance, double threshold)
{
    QVector<int> peaks;
    int n = data.size();

    for (int i = 1; i < n - 1; ++i) {
        if (data[i] > data[i - 1] && data[i] >= data[i + 1] && data[i] > threshold) {
            if (!peaks.isEmpty() && i - peaks.last() < minDistance) {
                if (data[i] > data[peaks.last()]) {
                    peaks.last() = i;
                }
            } else {
                peaks.append(i);
            }
        }
    }

    return peaks;
}

QVector<int> SpectralLineFitter::findValleys(const QVector<double> &data, int minDistance, double threshold)
{
    QVector<int> valleys;
    int n = data.size();

    for (int i = 1; i < n - 1; ++i) {
        if (data[i] < data[i - 1] && data[i] <= data[i + 1] && data[i] < threshold) {
            if (!valleys.isEmpty() && i - valleys.last() < minDistance) {
                if (data[i] < data[valleys.last()]) {
                    valleys.last() = i;
                }
            } else {
                valleys.append(i);
            }
        }
    }

    return valleys;
}

bool SpectralLineFitter::levenbergMarquardtFit(const QVector<double> &x, const QVector<double> &y,
                                               double &center, double &amplitude, double &fwhm,
                                               double &chiSq)
{
    const int maxIterations = 200;
    const double tolerance = 1e-10;
    double lambda = 0.01;
    const double lambdaUp = 10.0;
    const double lambdaDown = 0.1;
    const double lambdaMin = 1e-7;
    const double lambdaMax = 1e7;

    int n = x.size();
    chiSq = 0.0;

    for (int i = 0; i < n; ++i) {
        double model = computeModelValue(x[i], center, amplitude, fwhm);
        double diff = y[i] - model;
        chiSq += diff * diff;
    }

    if (chiSq == 0.0) return true;

    double voigtFraction = 0.5;

    for (int iter = 0; iter < maxIterations; ++iter) {
        double J00 = 0.0, J01 = 0.0, J02 = 0.0;
        double J11 = 0.0, J12 = 0.0, J22 = 0.0;
        double b0 = 0.0, b1 = 0.0, b2 = 0.0;

        double gamma = fwhm / 2.0;
        double sigma = fwhmToSigma(fwhm);
        double sigma2 = sigma * sigma;
        double gamma2 = gamma * gamma;

        double dfdFwhmScale = sigma / fwhm * std::sqrt(2.0 * std::log(2.0));

        for (int i = 0; i < n; ++i) {
            double dx = x[i] - center;
            double dx2 = dx * dx;

            double dfdA = 0.0;
            double dfddx = 0.0;
            double dfdfwhm = 0.0;
            double model = 0.0;

            if (m_lineType == Gaussian) {
                double expArg = -dx2 / (2.0 * sigma2);
                double g = amplitude * std::exp(expArg);
                model = g;

                dfdA = g / amplitude;
                dfddx = g * dx / sigma2;
                dfdfwhm = g * dx2 / (sigma2 * sigma) * dfdFwhmScale;

            } else if (m_lineType == Lorentzian) {
                double denom = 1.0 + dx2 / gamma2;
                double l = amplitude / denom;
                model = l;

                dfdA = 1.0 / denom;
                dfddx = 2.0 * amplitude * dx / (gamma2 * denom * denom);
                dfdfwhm = 2.0 * amplitude * dx2 / (gamma * gamma2 * denom * denom) * 0.5;

            } else if (m_lineType == Voigt) {
                double expArg = -dx2 / (2.0 * sigma2);
                double g = amplitude * (1.0 - voigtFraction) * std::exp(expArg);
                double denom = 1.0 + dx2 / gamma2;
                double l = amplitude * voigtFraction / denom;
                model = g + l;

                double dfdA_g = (1.0 - voigtFraction) * std::exp(expArg);
                double dfdA_l = voigtFraction / denom;
                dfdA = dfdA_g + dfdA_l;

                double dfddx_g = g * dx / sigma2;
                double dfddx_l = 2.0 * amplitude * voigtFraction * dx / (gamma2 * denom * denom);
                dfddx = dfddx_g + dfddx_l;

                double dfdfwhm_g = g * dx2 / (sigma2 * sigma) * dfdFwhmScale;
                double dfdfwhm_l = 2.0 * amplitude * voigtFraction * dx2 /
                                   (gamma * gamma2 * denom * denom) * 0.5;
                dfdfwhm = dfdfwhm_g + dfdfwhm_l;
            }

            double residual = y[i] - model;

            J00 += dfdA * dfdA;
            J01 += dfdA * dfddx;
            J02 += dfdA * dfdfwhm;
            J11 += dfddx * dfddx;
            J12 += dfddx * dfdfwhm;
            J22 += dfdfwhm * dfdfwhm;

            b0 += dfdA * residual;
            b1 += dfddx * residual;
            b2 += dfdfwhm * residual;
        }

        double diag0 = J00 * (1.0 + lambda);
        double diag1 = J11 * (1.0 + lambda);
        double diag2 = J22 * (1.0 + lambda);

        double a00 = diag0, a01 = J01, a02 = J02;
        double a11 = diag1, a12 = J12;
        double a22 = diag2;
        double r0 = b0, r1 = b1, r2 = b2;

        double det = a00 * (a11 * a22 - a12 * a12) -
                     a01 * (a01 * a22 - a02 * a12) +
                     a02 * (a01 * a12 - a02 * a11);

        if (std::abs(det) < 1e-30) {
            lambda *= lambdaUp;
            if (lambda > lambdaMax) return iter > 0;
            continue;
        }

        double invDet = 1.0 / det;

        double dA = (r0 * (a11 * a22 - a12 * a12) -
                    r1 * (a01 * a22 - a02 * a12) +
                    r2 * (a01 * a12 - a02 * a11)) * invDet;

        double dCenter = (-r0 * (a01 * a22 - a02 * a12) +
                         r1 * (a00 * a22 - a02 * a02) -
                         r2 * (a00 * a12 - a01 * a02)) * invDet;

        double dFwhm = (r0 * (a01 * a12 - a02 * a11) -
                       r1 * (a00 * a12 - a01 * a02) +
                       r2 * (a00 * a11 - a01 * a01)) * invDet;

        double newCenter = center + dCenter;
        double newAmplitude = amplitude + dA;
        double newFwhm = fwhm + dFwhm;

        if (newFwhm < m_minLineWidth) newFwhm = m_minLineWidth;
        if (newFwhm > m_maxLineWidth) newFwhm = m_maxLineWidth;

        if (std::abs(newAmplitude) > 1e10) {
            lambda *= lambdaUp;
            continue;
        }

        double newChiSq = 0.0;
        for (int i = 0; i < n; ++i) {
            double model = computeModelValue(x[i], newCenter, newAmplitude, newFwhm);
            double diff = y[i] - model;
            newChiSq += diff * diff;
        }

        if (newChiSq < chiSq) {
            center = newCenter;
            amplitude = newAmplitude;
            fwhm = newFwhm;

            double delta = chiSq - newChiSq;
            chiSq = newChiSq;

            lambda = std::max(lambda * lambdaDown, lambdaMin);

            if (delta < tolerance * chiSq) {
                return true;
            }
        } else {
            lambda = std::min(lambda * lambdaUp, lambdaMax);
        }

        if (lambda > lambdaMax) {
            return chiSq > 0;
        }
    }

    return true;
}

double SpectralLineFitter::computeModelValue(double x, double center, double amplitude, double fwhm)
{
    switch (m_lineType) {
    case Gaussian:
        return gaussian(x, center, amplitude, fwhm);
    case Lorentzian:
        return lorentzian(x, center, amplitude, fwhm);
    case Voigt:
        return voigt(x, center, amplitude, fwhm);
    default:
        return gaussian(x, center, amplitude, fwhm);
    }
}
