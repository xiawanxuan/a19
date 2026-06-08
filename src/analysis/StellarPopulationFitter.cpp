#include "StellarPopulationFitter.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

StellarPopulationFitter::StellarPopulationFitter(QObject *parent)
    : QObject(parent)
    , m_imf(Chabrier)
    , m_fitMethod(ChiSquared)
    , m_minAge(0.001)
    , m_maxAge(15.0)
    , m_minMetallicity(-2.5)
    , m_maxMetallicity(0.5)
    , m_nAgeBins(10)
    , m_nMetallicityBins(5)
    , m_dustLaw(true)
    , m_libraryGenerated(false)
{
    generateSSPLibrary();
}

void StellarPopulationFitter::setIMF(IMF imf)
{
    if (m_imf != imf) {
        m_imf = imf;
        m_libraryGenerated = false;
    }
}

StellarPopulationFitter::IMF StellarPopulationFitter::imf() const
{
    return m_imf;
}

void StellarPopulationFitter::setFitMethod(FitMethod method)
{
    m_fitMethod = method;
}

StellarPopulationFitter::FitMethod StellarPopulationFitter::fitMethod() const
{
    return m_fitMethod;
}

void StellarPopulationFitter::setAgeRange(double minAge, double maxAge)
{
    m_minAge = minAge;
    m_maxAge = maxAge;
    m_libraryGenerated = false;
}

double StellarPopulationFitter::minAge() const
{
    return m_minAge;
}

double StellarPopulationFitter::maxAge() const
{
    return m_maxAge;
}

void StellarPopulationFitter::setMetallicityRange(double minZ, double maxZ)
{
    m_minMetallicity = minZ;
    m_maxMetallicity = maxZ;
    m_libraryGenerated = false;
}

double StellarPopulationFitter::minMetallicity() const
{
    return m_minMetallicity;
}

double StellarPopulationFitter::maxMetallicity() const
{
    return m_maxMetallicity;
}

void StellarPopulationFitter::setNAgeBins(int n)
{
    if (n > 0) {
        m_nAgeBins = n;
        m_libraryGenerated = false;
    }
}

int StellarPopulationFitter::nAgeBins() const
{
    return m_nAgeBins;
}

void StellarPopulationFitter::setNMetallicityBins(int n)
{
    if (n > 0) {
        m_nMetallicityBins = n;
        m_libraryGenerated = false;
    }
}

int StellarPopulationFitter::nMetallicityBins() const
{
    return m_nMetallicityBins;
}

void StellarPopulationFitter::setDustLaw(bool enable)
{
    m_dustLaw = enable;
}

bool StellarPopulationFitter::dustLaw() const
{
    return m_dustLaw;
}

bool StellarPopulationFitter::generateSSPLibrary()
{
    m_sspLibrary.clear();

    QVector<double> wl;
    for (double w = 3000.0; w <= 10000.0; w += 2.0) {
        wl.append(w);
    }

    QVector<double> ages;
    for (int i = 0; i < m_nAgeBins; ++i) {
        double t = static_cast<double>(i) / (m_nAgeBins - 1);
        double age = m_minAge * std::pow(m_maxAge / m_minAge, t);
        ages.append(age);
    }

    QVector<double> metallicities;
    for (int i = 0; i < m_nMetallicityBins; ++i) {
        double z = m_minMetallicity + (m_maxMetallicity - m_minMetallicity) *
                   static_cast<double>(i) / (m_nMetallicityBins - 1);
        metallicities.append(z);
    }

    int total = ages.size() * metallicities.size();
    int count = 0;

    for (double age : ages) {
        for (double z : metallicities) {
            emit fittingProgress(count + 1, total);

            SSPModel ssp;
            ssp.name = QString("SSP_age%1_Z%2").arg(age, 0, 'f', 3).arg(z, 0, 'f', 2);
            ssp.age = age;
            ssp.metallicity = z;
            ssp.mass = 1.0;
            ssp.wavelengths = wl;
            generateSSPSpectrum(age, z, wl, ssp.flux);
            m_sspLibrary.append(ssp);
            count++;
        }
    }

    m_libraryGenerated = true;
    return true;
}

QList<StellarPopulationFitter::SSPModel> StellarPopulationFitter::sspLibrary() const
{
    return m_sspLibrary;
}

int StellarPopulationFitter::sspCount() const
{
    return m_sspLibrary.size();
}

StellarPopulationFitter::PopulationResult StellarPopulationFitter::fitPopulation(
    const SpectrumData &spectrum)
{
    PopulationResult result;
    result.totalMass = 0.0;
    result.meanAge = 0.0;
    result.meanMetallicity = 0.0;
    result.massWeightedAge = 0.0;
    result.massWeightedMetallicity = 0.0;
    result.chiSquared = 1e30;
    result.reducedChiSquared = 1e30;
    result.starFormationRate = 0.0;
    result.ageSpread = 0.0;

    if (!spectrum.isValid()) {
        return result;
    }

    if (!m_libraryGenerated) {
        generateSSPLibrary();
    }

    QVector<double> specWl = spectrum.wavelengths();
    QVector<double> specFlux = spectrum.flux();
    int nSpec = specWl.size();

    int nSSP = m_sspLibrary.size();
    if (nSSP < 2 || nSpec < 10) {
        return result;
    }

    QVector<QVector<double>> A(nSpec, QVector<double>(nSSP, 0.0));
    for (int j = 0; j < nSSP; ++j) {
        const SSPModel &ssp = m_sspLibrary[j];
        QVector<double> resampledFlux;

        if (ssp.wavelengths.size() < 2) continue;

        for (int i = 0; i < nSpec; ++i) {
            double w = specWl[i];
            if (w <= ssp.wavelengths.first()) {
                resampledFlux.append(ssp.flux.first());
            } else if (w >= ssp.wavelengths.last()) {
                resampledFlux.append(ssp.flux.last());
            } else {
                int lo = 0, hi = ssp.wavelengths.size() - 1;
                while (lo < hi - 1) {
                    int mid = (lo + hi) / 2;
                    if (ssp.wavelengths[mid] <= w) lo = mid;
                    else hi = mid;
                }
                double t = (w - ssp.wavelengths[lo]) / (ssp.wavelengths[hi] - ssp.wavelengths[lo]);
                double f = ssp.flux[lo] * (1.0 - t) + ssp.flux[hi] * t;
                resampledFlux.append(f);
            }
        }

        for (int i = 0; i < nSpec; ++i) {
            A[i][j] = resampledFlux[i];
        }
    }

    QVector<double> x(nSSP, 1.0 / nSSP);
    double chiSq = 0.0;

    if (m_fitMethod == NonNegativeLeastSquares) {
        nnlsFit(A, specFlux, x, chiSq);
    } else {
        double bestChiSq = 1e30;
        QVector<double> bestX = x;

        int nIter = 100;
        for (int iter = 0; iter < nIter; ++iter) {
            emit fittingProgress(iter + 1, nIter);

            double lambda = 0.01;
            for (int iterInner = 0; iterInner < 50; ++iterInner) {
                QVector<double> gradient(nSSP, 0.0);
                QVector<double> hessian(nSSP, 0.0);

                for (int i = 0; i < nSpec; ++i) {
                    double model = 0.0;
                    for (int j = 0; j < nSSP; ++j) {
                        model += A[i][j] * x[j];
                    }
                    double residual = specFlux[i] - model;

                    for (int j = 0; j < nSSP; ++j) {
                        gradient[j] += A[i][j] * residual;
                        hessian[j] += A[i][j] * A[i][j];
                    }
                }

                bool updated = false;
                for (int j = 0; j < nSSP; ++j) {
                    if (std::abs(hessian[j]) > 1e-20) {
                        double delta = gradient[j] / (hessian[j] * (1.0 + lambda));
                        double newX = x[j] + delta;
                        if (newX < 0) newX = 0;

                        if (std::abs(delta) > 1e-10) {
                            updated = true;
                        }
                    }
                }

                if (!updated) break;
                lambda *= 0.5;
            }

            double totalMass = 0.0;
            for (double v : x) totalMass += v;
            if (totalMass > 1e-20) {
                for (double &v : x) v /= totalMass;
            }

            chiSq = 0.0;
            for (int i = 0; i < nSpec; ++i) {
                double model = 0.0;
                for (int j = 0; j < nSSP; ++j) {
                    model += A[i][j] * x[j];
                }
                double d = specFlux[i] - model;
                chiSq += d * d;
            }

            if (chiSq < bestChiSq) {
                bestChiSq = chiSq;
                bestX = x;
            }

            for (int j = 0; j < nSSP; ++j) {
                x[j] = bestX[j] + 0.1 * (static_cast<double>(rand()) / RAND_MAX - 0.5);
                if (x[j] < 0) x[j] = 0;
            }
        }

        x = bestX;
        chiSq = bestChiSq;
    }

    double totalMass = 0.0;
    for (double v : x) totalMass += v;
    if (totalMass > 1e-20) {
        for (double &v : x) v /= totalMass;
    }

    result.massFractions = x;

    double sumMass = 0.0;
    double sumAge = 0.0;
    double sumZ = 0.0;
    double sumAge2 = 0.0;

    QVector<double> ageBins;
    QMap<double, double> ageMassMap;

    for (int j = 0; j < nSSP; ++j) {
        const SSPModel &ssp = m_sspLibrary[j];
        double mass = x[j] * ssp.mass;

        sumMass += mass;
        sumAge += mass * ssp.age;
        sumZ += mass * ssp.metallicity;
        sumAge2 += mass * ssp.age * ssp.age;

        ageMassMap[ssp.age] += mass;

        QString compName = QString("Age%1_Z%2")
            .arg(ssp.age, 0, 'f', 2)
            .arg(ssp.metallicity, 0, 'f', 2);
        result.componentMasses[compName] = mass;
    }

    for (auto it = ageMassMap.begin(); it != ageMassMap.end(); ++it) {
        ageBins.append(it.key());
    }
    result.ageBins = ageBins;

    if (sumMass > 1e-20) {
        result.totalMass = sumMass;
        result.meanAge = sumAge / sumMass;
        result.meanMetallicity = sumZ / sumMass;
        result.massWeightedAge = sumAge / sumMass;
        result.massWeightedMetallicity = sumZ / sumMass;

        double varAge = sumAge2 / sumMass - result.meanAge * result.meanAge;
        result.ageSpread = (varAge > 0) ? std::sqrt(varAge) : 0.0;

        double sfr = sumMass / (m_maxAge * 1e9);
        result.starFormationRate = sfr;
    }

    result.chiSquared = chiSq;
    if (nSpec > nSSP) {
        result.reducedChiSquared = chiSq / (nSpec - nSSP);
    }

    emit fittingComplete(result);
    return result;
}

QVector<double> StellarPopulationFitter::generateSpectrum(
    const QVector<double> &wavelengths, const QVector<double> &weights)
{
    QVector<double> result(wavelengths.size(), 0.0);

    if (weights.size() != m_sspLibrary.size()) {
        return result;
    }

    for (int j = 0; j < m_sspLibrary.size(); ++j) {
        if (std::abs(weights[j]) < 1e-20) continue;

        const SSPModel &ssp = m_sspLibrary[j];

        QVector<double> resampled;
        if (ssp.wavelengths.size() < 2) continue;

        for (double w : wavelengths) {
            if (w <= ssp.wavelengths.first()) {
                resampled.append(ssp.flux.first());
            } else if (w >= ssp.wavelengths.last()) {
                resampled.append(ssp.flux.last());
            } else {
                int lo = 0, hi = ssp.wavelengths.size() - 1;
                while (lo < hi - 1) {
                    int mid = (lo + hi) / 2;
                    if (ssp.wavelengths[mid] <= w) lo = mid;
                    else hi = mid;
                }
                double t = (w - ssp.wavelengths[lo]) / (ssp.wavelengths[hi] - ssp.wavelengths[lo]);
                double f = ssp.flux[lo] * (1.0 - t) + ssp.flux[hi] * t;
                resampled.append(f);
            }
        }

        for (int i = 0; i < wavelengths.size(); ++i) {
            result[i] += weights[j] * resampled[i];
        }
    }

    return result;
}

QString StellarPopulationFitter::imfName(IMF imf)
{
    switch (imf) {
    case Salpeter: return "Salpeter (1955)";
    case Kroupa: return "Kroupa (2001)";
    case Chabrier: return "Chabrier (2003)";
    default: return "Unknown";
    }
}

QString StellarPopulationFitter::fitMethodName(FitMethod method)
{
    switch (method) {
    case ChiSquared: return "Chi-Squared";
    case MaximumLikelihood: return "Maximum Likelihood";
    case NonNegativeLeastSquares: return "NNLS";
    default: return "Unknown";
    }
}

QString StellarPopulationFitter::ageToString(double ageGyr)
{
    if (ageGyr >= 1.0) {
        return QString("%1 Gyr").arg(ageGyr, 0, 'f', 2);
    } else {
        return QString("%1 Myr").arg(ageGyr * 1000.0, 0, 'f', 1);
    }
}

void StellarPopulationFitter::generateSSPSpectrum(double age, double metallicity,
                                                    const QVector<double> &wavelengths,
                                                    QVector<double> &flux)
{
    int n = wavelengths.size();
    flux.resize(n);

    double logAge = std::log10(age * 1e9);

    double massFractionOB = 0.0;
    double massFractionA = 0.0;
    double massFractionFG = 0.0;
    double massFractionKM = 0.0;
    double massFractionWD = 0.0;

    if (age < 0.01) {
        massFractionOB = 0.3;
        massFractionA = 0.25;
        massFractionFG = 0.25;
        massFractionKM = 0.2;
        massFractionWD = 0.0;
    } else if (age < 0.1) {
        massFractionOB = 0.1;
        massFractionA = 0.3;
        massFractionFG = 0.35;
        massFractionKM = 0.25;
        massFractionWD = 0.0;
    } else if (age < 1.0) {
        massFractionOB = 0.01;
        massFractionA = 0.15;
        massFractionFG = 0.4;
        massFractionKM = 0.4;
        massFractionWD = 0.04;
    } else if (age < 5.0) {
        massFractionOB = 0.0;
        massFractionA = 0.05;
        massFractionFG = 0.35;
        massFractionKM = 0.5;
        massFractionWD = 0.1;
    } else {
        massFractionOB = 0.0;
        massFractionA = 0.01;
        massFractionFG = 0.25;
        massFractionKM = 0.55;
        massFractionWD = 0.19;
    }

    double zFactor = std::pow(10.0, metallicity);

    for (int i = 0; i < n; ++i) {
        double w = wavelengths[i];

        double fOB = blackbodyFlux(w, 30000.0) * massFractionOB;
        double fA = blackbodyFlux(w, 9500.0) * massFractionA;
        double fFG = blackbodyFlux(w, 6500.0) * massFractionFG;
        double fKM = blackbodyFlux(w, 4000.0) * massFractionKM;
        double fWD = blackbodyFlux(w, 10000.0) * massFractionWD;

        double total = fOB + fA + fFG + fKM + fWD;

        double metalCorr = metallicityCorrection(w, metallicity);
        total *= metalCorr;

        flux[i] = total;
    }

    double maxFlux = 0.0;
    for (double f : flux) {
        if (f > maxFlux) maxFlux = f;
    }
    if (maxFlux > 1e-20) {
        for (int i = 0; i < n; ++i) {
            flux[i] /= maxFlux;
        }
    }

    double balmerJump = 0.0;
    if (age < 1.0) {
        balmerJump = 0.15 * (1.0 - age);
    }
    if (balmerJump > 0) {
        for (int i = 0; i < n; ++i) {
            double w = wavelengths[i];
            if (w < 4000.0) {
                double d = (w - 3800.0) / 100.0;
                flux[i] *= 1.0 - balmerJump * std::exp(-d * d);
            }
        }
    }

    double breakStrength = 0.0;
    if (age > 1.0) {
        breakStrength = std::min(0.4, 0.1 + 0.05 * logAge);
    }
    if (breakStrength > 0) {
        for (int i = 0; i < n; ++i) {
            double w = wavelengths[i];
            if (w > 3700.0 && w < 4200.0) {
                double d = (w - 4000.0) / 150.0;
                double factor = 1.0 - breakStrength * (1.0 + std::tanh(-d)) / 2.0;
                flux[i] *= factor;
            }
        }
    }

    Q_UNUSED(zFactor)
}

double StellarPopulationFitter::blackbodyFlux(double wavelength, double temperature)
{
    double hc_kT = 1.438777e7 / (temperature * wavelength);
    double denom = std::exp(hc_kT) - 1.0;
    if (std::abs(denom) < 1e-20) return 1e30;
    return 1.0 / (std::pow(wavelength, 5.0) * denom);
}

double StellarPopulationFitter::metallicityCorrection(double wavelength, double metallicity)
{
    double z = metallicity;
    double factor = 1.0;

    if (wavelength < 4500.0) {
        double uvDepression = 0.1 * z * (1.0 - wavelength / 4500.0);
        factor *= (1.0 + uvDepression);
    }

    if (std::abs(wavelength - 3933.0) < 50.0) {
        double d = (wavelength - 3933.0) / 10.0;
        factor *= 1.0 - 0.1 * z * std::exp(-d * d);
    }
    if (std::abs(wavelength - 3968.0) < 50.0) {
        double d = (wavelength - 3968.0) / 10.0;
        factor *= 1.0 - 0.08 * z * std::exp(-d * d);
    }

    QVector<double> metalLines = {4226.7, 4383.5, 4668.1, 5167.0, 5172.7, 5183.6};
    for (double lineWl : metalLines) {
        if (std::abs(wavelength - lineWl) < 10.0) {
            double d = (wavelength - lineWl) / 2.0;
            factor *= 1.0 - 0.05 * (1.0 + z) * std::exp(-d * d);
        }
    }

    return factor;
}

double StellarPopulationFitter::dustExtinction(double wavelength, double av, double rv)
{
    double x = 1.0 / (wavelength * 1e-4);
    double klam = 0.0;

    if (x < 1.1) {
        klam = rv * (0.356 + 0.539 * x + 0.039 * x * x);
    } else if (x < 3.3) {
        double y = x - 1.82;
        klam = rv * (1.0 + 0.17699 * y - 0.50447 * y * y - 0.02427 * y * y * y
                       + 0.72085 * y * y * y * y + 0.01979 * y * y * y * y * y
                       - 0.77530 * y * y * y * y * y * y
                       + 0.32999 * y * y * y * y * y * y * y);
    } else {
        klam = rv * (1.569 + 1.048 * x + 0.008 * x * x);
    }

    double extinction = std::pow(10.0, -0.4 * av * klam);
    return extinction;
}

bool StellarPopulationFitter::nnlsFit(const QVector<QVector<double>> &,
                                        const QVector<double> &,
                                        QVector<double> &x, double &chiSq)
{
    int n = x.size();
    chiSq = 0.0;

    for (int i = 0; i < n; ++i) {
        if (x[i] < 0) x[i] = 0;
    }

    double total = 0.0;
    for (double v : x) total += v;
    if (total > 1e-20) {
        for (double &v : x) v /= total;
    }

    return true;
}
