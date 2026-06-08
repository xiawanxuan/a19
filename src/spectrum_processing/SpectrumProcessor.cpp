#include "SpectrumProcessor.h"
#include <cmath>
#include <algorithm>
#include <QtMath>
#include <QDebug>

SpectrumProcessor::SpectrumProcessor(QObject *parent)
    : QObject(parent)
    , m_wavelengthCalibrator(new WavelengthCalibrator(this))
    , m_lineFitter(new SpectralLineFitter(this))
    , m_redshiftCalculator(new RedshiftCalculator(this))
{
    connect(m_lineFitter, &SpectralLineFitter::fittingProgress,
            this, &SpectrumProcessor::processingProgress);
    connect(m_redshiftCalculator, &RedshiftCalculator::calculationProgress,
            this, &SpectrumProcessor::processingProgress);
}

WavelengthCalibrator* SpectrumProcessor::wavelengthCalibrator() const
{
    return m_wavelengthCalibrator;
}

SpectralLineFitter* SpectrumProcessor::lineFitter() const
{
    return m_lineFitter;
}

RedshiftCalculator* SpectrumProcessor::redshiftCalculator() const
{
    return m_redshiftCalculator;
}

SpectrumData SpectrumProcessor::smooth(const SpectrumData &spectrum, int windowSize)
{
    if (!spectrum.isValid() || windowSize < 2) return spectrum;

    SpectrumData result = spectrum.copy();
    QVector<double> flux = spectrum.flux();
    QVector<double> smoothed(flux.size());

    int halfWindow = windowSize / 2;

    for (int i = 0; i < flux.size(); ++i) {
        double sum = 0.0;
        int count = 0;

        for (int j = -halfWindow; j <= halfWindow; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx < flux.size()) {
                sum += flux[idx];
                count++;
            }
        }

        smoothed[i] = (count > 0) ? sum / count : flux[i];
    }

    result.setFlux(smoothed);

    if (!spectrum.error().isEmpty()) {
        QVector<double> error = spectrum.error();
        QVector<double> smoothedErr(error.size());

        for (int i = 0; i < error.size(); ++i) {
            double sum = 0.0;
            int count = 0;

            for (int j = -halfWindow; j <= halfWindow; ++j) {
                int idx = i + j;
                if (idx >= 0 && idx < error.size()) {
                    sum += error[idx] * error[idx];
                    count++;
                }
            }

            smoothedErr[i] = (count > 0) ? std::sqrt(sum) / count : error[i];
        }

        result.setError(smoothedErr);
    }

    return result;
}

SpectrumData SpectrumProcessor::medianFilter(const SpectrumData &spectrum, int windowSize)
{
    if (!spectrum.isValid() || windowSize < 2) return spectrum;

    SpectrumData result = spectrum.copy();
    QVector<double> flux = spectrum.flux();
    QVector<double> filtered(flux.size());

    int halfWindow = windowSize / 2;

    for (int i = 0; i < flux.size(); ++i) {
        QVector<double> windowValues;

        for (int j = -halfWindow; j <= halfWindow; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx < flux.size()) {
                windowValues.append(flux[idx]);
            }
        }

        std::sort(windowValues.begin(), windowValues.end());
        int mid = windowValues.size() / 2;
        filtered[i] = windowValues[mid];
    }

    result.setFlux(filtered);
    return result;
}

SpectrumData SpectrumProcessor::normalize(const SpectrumData &spectrum)
{
    if (!spectrum.isValid()) return spectrum;

    SpectrumData result = spectrum.copy();
    QVector<double> flux = spectrum.flux();

    double maxVal = *std::max_element(flux.begin(), flux.end());
    if (maxVal == 0.0) return result;

    QVector<double> normalized(flux.size());
    for (int i = 0; i < flux.size(); ++i) {
        normalized[i] = flux[i] / maxVal;
    }

    result.setFlux(normalized);

    if (!spectrum.error().isEmpty()) {
        QVector<double> error = spectrum.error();
        QVector<double> normalizedErr(error.size());
        for (int i = 0; i < error.size(); ++i) {
            normalizedErr[i] = error[i] / maxVal;
        }
        result.setError(normalizedErr);
    }

    return result;
}

SpectrumData SpectrumProcessor::rebin(const SpectrumData &spectrum, double newWavelengthStep)
{
    if (!spectrum.isValid() || newWavelengthStep <= 0) return spectrum;

    double minWl = spectrum.minWavelength();
    double maxWl = spectrum.maxWavelength();
    int newSize = static_cast<int>((maxWl - minWl) / newWavelengthStep) + 1;

    if (newSize < 2) return spectrum;

    SpectrumData result = spectrum.copy();

    QVector<double> newWavelengths(newSize);
    QVector<double> newFlux(newSize, 0.0);
    QVector<double> newError(newSize, 0.0);

    QVector<double> oldWl = spectrum.wavelengths();
    QVector<double> oldFlux = spectrum.flux();
    QVector<double> oldErr = spectrum.error();

    for (int i = 0; i < newSize; ++i) {
        newWavelengths[i] = minWl + i * newWavelengthStep;
    }

    for (int i = 0; i < newSize; ++i) {
        double wl = newWavelengths[i];

        int lo = 0;
        int hi = oldWl.size() - 1;

        while (lo < hi - 1) {
            int mid = (lo + hi) / 2;
            if (oldWl[mid] < wl) {
                lo = mid;
            } else {
                hi = mid;
            }
        }

        if (lo >= 0 && hi < oldWl.size()) {
            double t = (wl - oldWl[lo]) / (oldWl[hi] - oldWl[lo]);
            newFlux[i] = oldFlux[lo] * (1 - t) + oldFlux[hi] * t;

            if (!oldErr.isEmpty() && lo < oldErr.size() && hi < oldErr.size()) {
                double e0 = oldErr[lo];
                double e1 = oldErr[hi];
                newError[i] = std::sqrt(e0 * e0 * (1 - t) * (1 - t) + e1 * e1 * t * t);
            }
        }
    }

    result.setWavelengths(newWavelengths);
    result.setFlux(newFlux);

    if (!oldErr.isEmpty()) {
        result.setError(newError);
    }

    return result;
}

SpectrumData SpectrumProcessor::crop(const SpectrumData &spectrum, double wlStart, double wlEnd)
{
    if (!spectrum.isValid() || wlStart >= wlEnd) return spectrum;

    int startIdx = spectrum.indexAtWavelength(wlStart);
    int endIdx = spectrum.indexAtWavelength(wlEnd);

    if (startIdx >= endIdx) return spectrum;

    SpectrumData result = spectrum.copy();

    result.setWavelengths(spectrum.wavelengths().mid(startIdx, endIdx - startIdx + 1));
    result.setFlux(spectrum.flux().mid(startIdx, endIdx - startIdx + 1));

    if (!spectrum.error().isEmpty()) {
        result.setError(spectrum.error().mid(startIdx, endIdx - startIdx + 1));
    }

    if (!spectrum.continuum().isEmpty()) {
        result.setContinuum(spectrum.continuum().mid(startIdx, endIdx - startIdx + 1));
    }

    return result;
}

SpectrumData SpectrumProcessor::subtractContinuum(const SpectrumData &spectrum)
{
    if (!spectrum.isValid()) return spectrum;

    SpectrumData result = spectrum.copy();

    QVector<double> continuum;
    if (!spectrum.continuum().isEmpty() && spectrum.continuum().size() == spectrum.flux().size()) {
        continuum = spectrum.continuum();
    } else {
        continuum = m_lineFitter->fitContinuum(spectrum);
    }

    QVector<double> flux = spectrum.flux();
    QVector<double> subtracted(flux.size());

    for (int i = 0; i < flux.size(); ++i) {
        subtracted[i] = flux[i] - continuum[i];
    }

    result.setFlux(subtracted);
    result.setContinuum(continuum);

    return result;
}

SpectrumData SpectrumProcessor::divideByContinuum(const SpectrumData &spectrum)
{
    if (!spectrum.isValid()) return spectrum;

    SpectrumData result = spectrum.copy();

    QVector<double> continuum;
    if (!spectrum.continuum().isEmpty() && spectrum.continuum().size() == spectrum.flux().size()) {
        continuum = spectrum.continuum();
    } else {
        continuum = m_lineFitter->fitContinuum(spectrum);
    }

    QVector<double> flux = spectrum.flux();
    QVector<double> divided(flux.size());

    for (int i = 0; i < flux.size(); ++i) {
        if (continuum[i] != 0) {
            divided[i] = flux[i] / continuum[i];
        } else {
            divided[i] = 1.0;
        }
    }

    result.setFlux(divided);
    result.setContinuum(QVector<double>(continuum.size(), 1.0));

    return result;
}

SpectrumData SpectrumProcessor::addSpectra(const SpectrumData &spec1, const SpectrumData &spec2)
{
    if (!spec1.isValid() || !spec2.isValid()) return spec1;

    SpectrumData result = spec1.copy();
    QVector<double> flux1 = spec1.flux();
    QVector<double> flux2 = spec2.flux();

    int minSize = qMin(flux1.size(), flux2.size());
    QVector<double> summed(minSize);

    for (int i = 0; i < minSize; ++i) {
        summed[i] = flux1[i] + flux2[i];
    }

    result.setFlux(summed);
    result.setName(spec1.name() + " + " + spec2.name());

    return result;
}

SpectrumData SpectrumProcessor::subtractSpectra(const SpectrumData &spec1, const SpectrumData &spec2)
{
    if (!spec1.isValid() || !spec2.isValid()) return spec1;

    SpectrumData result = spec1.copy();
    QVector<double> flux1 = spec1.flux();
    QVector<double> flux2 = spec2.flux();

    int minSize = qMin(flux1.size(), flux2.size());
    QVector<double> difference(minSize);

    for (int i = 0; i < minSize; ++i) {
        difference[i] = flux1[i] - flux2[i];
    }

    result.setFlux(difference);
    result.setName(spec1.name() + " - " + spec2.name());

    return result;
}

SpectrumData SpectrumProcessor::multiplySpectrum(const SpectrumData &spec, double factor)
{
    if (!spec.isValid()) return spec;

    SpectrumData result = spec.copy();
    QVector<double> flux = spec.flux();
    QVector<double> multiplied(flux.size());

    for (int i = 0; i < flux.size(); ++i) {
        multiplied[i] = flux[i] * factor;
    }

    result.setFlux(multiplied);

    if (!spec.error().isEmpty()) {
        QVector<double> error = spec.error();
        QVector<double> multipliedErr(error.size());
        for (int i = 0; i < error.size(); ++i) {
            multipliedErr[i] = error[i] * std::abs(factor);
        }
        result.setError(multipliedErr);
    }

    return result;
}

double SpectrumProcessor::integrate(const SpectrumData &spectrum, double wlStart, double wlEnd)
{
    if (!spectrum.isValid() || wlStart >= wlEnd) return 0.0;

    int startIdx = spectrum.indexAtWavelength(wlStart);
    int endIdx = spectrum.indexAtWavelength(wlEnd);

    if (startIdx >= endIdx) return 0.0;

    QVector<double> wl = spectrum.wavelengths();
    QVector<double> flux = spectrum.flux();

    double integral = 0.0;

    for (int i = startIdx + 1; i <= endIdx; ++i) {
        double dwl = wl[i] - wl[i - 1];
        double avgFlux = (flux[i] + flux[i - 1]) / 2.0;
        integral += avgFlux * dwl;
    }

    return integral;
}

double SpectrumProcessor::averageFlux(const SpectrumData &spectrum, double wlStart, double wlEnd)
{
    if (!spectrum.isValid() || wlStart >= wlEnd) return 0.0;

    double integral = integrate(spectrum, wlStart, wlEnd);
    return integral / (wlEnd - wlStart);
}

QVector<double> SpectrumProcessor::radialVelocityCurve(const SpectrumData &spectrum, double restWavelength, int windowSize)
{
    if (!spectrum.isValid()) return QVector<double>();

    QVector<double> velocities;
    const double c = 299792.458;

    int n = spectrum.size();
    QVector<double> wl = spectrum.wavelengths();
    QVector<double> flux = spectrum.flux();

    for (int i = 0; i < n; ++i) {
        int start = qMax(0, i - windowSize);
        int end = qMin(n - 1, i + windowSize);

        double peakFlux = 0.0;
        double peakWl = 0.0;

        for (int j = start; j <= end; ++j) {
            if (flux[j] > peakFlux) {
                peakFlux = flux[j];
                peakWl = wl[j];
            }
        }

        if (restWavelength > 0) {
            double z = (peakWl - restWavelength) / restWavelength;
            velocities.push_back(z * c);
        } else {
            velocities.push_back(0.0);
        }
    }

    return velocities;
}

bool SpectrumProcessor::processPipeline(SpectrumData &spectrum, bool doCalibration,
                                        bool doContinuum, bool detectLines,
                                        bool calculateRedshift)
{
    if (!spectrum.isValid()) {
        m_lastError = "Invalid spectrum data";
        emit errorOccurred(m_lastError);
        return false;
    }

    int totalSteps = (doCalibration ? 1 : 0) + (doContinuum ? 1 : 0) +
                     (detectLines ? 1 : 0) + (calculateRedshift ? 1 : 0);
    int currentStep = 0;

    if (doCalibration) {
        if (m_wavelengthCalibrator->isReady()) {
            m_wavelengthCalibrator->calibrate(spectrum);
        }
        currentStep++;
        emit processingProgress(currentStep, totalSteps);
    }

    if (doContinuum) {
        QVector<double> cont = m_lineFitter->fitContinuum(spectrum);
        spectrum.setContinuum(cont);
        currentStep++;
        emit processingProgress(currentStep, totalSteps);
    }

    if (detectLines) {
        m_lineFitter->fitAllLines(spectrum);
        currentStep++;
        emit processingProgress(currentStep, totalSteps);
    }

    if (calculateRedshift) {
        double z = m_redshiftCalculator->calculateRedshift(spectrum);
        spectrum.setRedshift(z);

        QVector<SpectralLine> lines = spectrum.spectralLines();
        for (SpectralLine &line : lines) {
            line.redshift = z;
            line.restWavelength = line.wavelength / (1 + z);
        }
        spectrum.setSpectralLines(lines);

        currentStep++;
        emit processingProgress(currentStep, totalSteps);
    }

    emit processingComplete(spectrum);
    return true;
}

QString SpectrumProcessor::lastError() const
{
    return m_lastError;
}
