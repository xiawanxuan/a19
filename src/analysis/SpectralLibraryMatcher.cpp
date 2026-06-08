#include "SpectralLibraryMatcher.h"
#include <cmath>
#include <algorithm>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SpectralLibraryMatcher::SpectralLibraryMatcher(QObject *parent)
    : QObject(parent)
    , m_method(CorrelationCoefficient)
    , m_minZ(-0.1)
    , m_maxZ(6.0)
    , m_searchStep(0.001)
    , m_topN(5)
    , m_libraryLoaded(false)
{
    generateBuiltInTemplates();
    m_libraryLoaded = true;
}

void SpectralLibraryMatcher::setMatchMethod(MatchMethod method)
{
    m_method = method;
}

SpectralLibraryMatcher::MatchMethod SpectralLibraryMatcher::matchMethod() const
{
    return m_method;
}

void SpectralLibraryMatcher::setRedshiftRange(double minZ, double maxZ)
{
    m_minZ = minZ;
    m_maxZ = maxZ;
}

double SpectralLibraryMatcher::minRedshift() const
{
    return m_minZ;
}

double SpectralLibraryMatcher::maxRedshift() const
{
    return m_maxZ;
}

void SpectralLibraryMatcher::setSearchStep(double step)
{
    if (step > 0) {
        m_searchStep = step;
    }
}

double SpectralLibraryMatcher::searchStep() const
{
    return m_searchStep;
}

void SpectralLibraryMatcher::setTopN(int n)
{
    if (n > 0) {
        m_topN = n;
    }
}

int SpectralLibraryMatcher::topN() const
{
    return m_topN;
}

bool SpectralLibraryMatcher::loadTemplateLibrary(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return false;
    }

    QJsonArray arr = doc.array();
    m_templates.clear();

    for (const QJsonValue &val : arr) {
        if (!val.isObject()) continue;

        QJsonObject obj = val.toObject();
        TemplateSpectrum tmplt;
        tmplt.name = obj["name"].toString();
        tmplt.type = static_cast<ObjectType>(obj["type"].toInt(Type_Unknown));
        tmplt.temperature = obj["temperature"].toDouble(0);
        tmplt.metallicity = obj["metallicity"].toDouble(0);
        tmplt.surfaceGravity = obj["logg"].toDouble(0);

        if (obj.contains("wavelengths") && obj.contains("flux")) {
            QJsonArray wlArr = obj["wavelengths"].toArray();
            QJsonArray fluxArr = obj["flux"].toArray();

            for (const QJsonValue &v : wlArr) {
                tmplt.wavelengths.append(v.toDouble());
            }
            for (const QJsonValue &v : fluxArr) {
                tmplt.flux.append(v.toDouble());
            }
        }

        if (tmplt.wavelengths.size() > 10) {
            m_templates.append(tmplt);
        }
    }

    m_libraryLoaded = !m_templates.isEmpty();
    return m_libraryLoaded;
}

bool SpectralLibraryMatcher::loadBuiltInLibrary()
{
    generateBuiltInTemplates();
    m_libraryLoaded = !m_templates.isEmpty();
    return m_libraryLoaded;
}

QList<SpectralLibraryMatcher::TemplateSpectrum> SpectralLibraryMatcher::templates() const
{
    return m_templates;
}

int SpectralLibraryMatcher::templateCount() const
{
    return m_templates.size();
}

QList<SpectralLibraryMatcher::MatchResult> SpectralLibraryMatcher::matchSpectrum(
    const SpectrumData &spectrum)
{
    QList<MatchResult> results;

    if (!spectrum.isValid() || m_templates.isEmpty()) {
        return results;
    }

    int total = m_templates.size();
    int count = 0;

    for (const TemplateSpectrum &tmplt : m_templates) {
        emit matchingProgress(count + 1, total);

        double bestZ = 0.0;
        double bestScore = 0.0;

        double score = findBestRedshift(spectrum, tmplt, bestZ, bestScore);

        MatchResult result;
        result.templateName = tmplt.name;
        result.objectType = tmplt.type;
        result.matchScore = score;
        result.redshift = bestZ;
        result.correlationCoefficient = bestScore;
        result.chiSquared = 0.0;
        result.scaleFactor = 1.0;
        result.rank = 0;

        results.append(result);
        count++;
    }

    std::sort(results.begin(), results.end(),
              [](const MatchResult &a, const MatchResult &b) {
        return a.matchScore > b.matchScore;
    });

    for (int i = 0; i < results.size(); ++i) {
        results[i].rank = i + 1;
    }

    if (results.size() > m_topN) {
        results = results.mid(0, m_topN);
    }

    emit matchingComplete(results);
    return results;
}

SpectralLibraryMatcher::MatchResult SpectralLibraryMatcher::bestMatch(
    const SpectrumData &spectrum)
{
    QList<MatchResult> results = matchSpectrum(spectrum);
    if (!results.isEmpty()) {
        return results.first();
    }
    return MatchResult();
}

QString SpectralLibraryMatcher::objectTypeName(ObjectType type)
{
    switch (type) {
    case Type_O: return "O-Type Star";
    case Type_B: return "B-Type Star";
    case Type_A: return "A-Type Star";
    case Type_F: return "F-Type Star";
    case Type_G: return "G-Type Star";
    case Type_K: return "K-Type Star";
    case Type_M: return "M-Type Star";
    case Type_WD: return "White Dwarf";
    case Type_QSO: return "Quasar (QSO)";
    case Type_Galaxy: return "Galaxy";
    case Type_HII: return "H II Region";
    case Type_Seyfert1: return "Seyfert 1";
    case Type_Seyfert2: return "Seyfert 2";
    case Type_Liner: return "LINER";
    case Type_Unknown:
    default: return "Unknown";
    }
}

QString SpectralLibraryMatcher::objectTypeDescription(ObjectType type)
{
    switch (type) {
    case Type_O: return "Hot, massive stars with strong He II lines";
    case Type_B: return "Hot stars with He I lines, no He II";
    case Type_A: return "Stars with strong Balmer hydrogen lines";
    case Type_F: return "Stars with Ca H&K lines, weaker Balmer lines";
    case Type_G: return "Sun-like stars with Ca H&K and metal lines";
    case Type_K: return "Cooler stars with strong molecular bands";
    case Type_M: return "Cool red stars with TiO absorption bands";
    case Type_WD: return "Hot compact degenerate stellar remnant";
    case Type_QSO: return "Active galactic nucleus with broad emission lines";
    case Type_Galaxy: return "Composite stellar population spectrum";
    case Type_HII: return "Ionized hydrogen region with emission lines";
    case Type_Seyfert1: return "Type 1 Seyfert galaxy with broad lines";
    case Type_Seyfert2: return "Type 2 Seyfert galaxy with narrow lines";
    case Type_Liner: return "Low-ionization nuclear emission-line region";
    case Type_Unknown:
    default: return "Unknown object type";
    }
}

QList<SpectralLibraryMatcher::ObjectType> SpectralLibraryMatcher::allObjectTypes()
{
    QList<ObjectType> types;
    types << Type_O << Type_B << Type_A << Type_F << Type_G
          << Type_K << Type_M << Type_WD << Type_QSO << Type_Galaxy
          << Type_HII << Type_Seyfert1 << Type_Seyfert2 << Type_Liner;
    return types;
}

double SpectralLibraryMatcher::crossCorrelation(const QVector<double> &,
                                                  const QVector<double> &y1,
                                                  const QVector<double> &y2)
{
    int n = qMin(y1.size(), y2.size());
    if (n < 10) return 0.0;

    double mean1 = 0.0, mean2 = 0.0;
    for (int i = 0; i < n; ++i) {
        mean1 += y1[i];
        mean2 += y2[i];
    }
    mean1 /= n;
    mean2 /= n;

    double num = 0.0, den1 = 0.0, den2 = 0.0;
    for (int i = 0; i < n; ++i) {
        double d1 = y1[i] - mean1;
        double d2 = y2[i] - mean2;
        num += d1 * d2;
        den1 += d1 * d1;
        den2 += d2 * d2;
    }

    double den = std::sqrt(den1 * den2);
    if (std::abs(den) < 1e-20) return 0.0;

    return num / den;
}

double SpectralLibraryMatcher::correlationCoefficient(const QVector<double> &y1,
                                                       const QVector<double> &y2)
{
    return crossCorrelation(QVector<double>(), y1, y2);
}

double SpectralLibraryMatcher::chiSquared(const QVector<double> &y1,
                                           const QVector<double> &y2, double sigma)
{
    int n = qMin(y1.size(), y2.size());
    if (n < 10) return 1e30;

    double chi2 = 0.0;
    double sigma2 = sigma * sigma;
    if (sigma2 < 1e-20) sigma2 = 1.0;

    for (int i = 0; i < n; ++i) {
        double diff = y1[i] - y2[i];
        chi2 += diff * diff / sigma2;
    }

    return chi2 / n;
}

void SpectralLibraryMatcher::generateBuiltInTemplates()
{
    m_templates.clear();

    QVector<double> wl;
    for (double w = 3000.0; w <= 10000.0; w += 2.0) {
        wl.append(w);
    }
    int n = wl.size();

    {
        TemplateSpectrum tmplt;
        tmplt.name = "O5V";
        tmplt.type = Type_O;
        tmplt.temperature = 42000;
        tmplt.metallicity = 0.0;
        tmplt.surfaceGravity = 4.0;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            double T = tmplt.temperature;
            double hc_kT = 1.438777e7 / (T * w);
            flux[i] = 1.0 / (std::pow(w, 5.0) * (std::exp(hc_kT) - 1.0));
            if (i > 0 && std::isinf(flux[i])) flux[i] = flux[i-1];
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            if (std::abs(w - 4686.0) < 5.0) {
                double d = (w - 4686.0) / 2.0;
                flux[i] *= 1.0 + 0.3 * std::exp(-d * d);
            }
            if (std::abs(w - 4542.0) < 4.0) {
                double d = (w - 4542.0) / 1.5;
                flux[i] *= 1.0 - 0.15 * std::exp(-d * d);
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "B0V";
        tmplt.type = Type_B;
        tmplt.temperature = 30000;
        tmplt.metallicity = 0.0;
        tmplt.surfaceGravity = 4.0;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            double T = tmplt.temperature;
            double hc_kT = 1.438777e7 / (T * w);
            flux[i] = 1.0 / (std::pow(w, 5.0) * (std::exp(hc_kT) - 1.0));
            if (i > 0 && std::isinf(flux[i])) flux[i] = flux[i-1];
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            if (std::abs(w - 4340.0) < 8.0) {
                double d = (w - 4340.0) / 3.0;
                flux[i] *= 1.0 - 0.1 * std::exp(-d * d);
            }
            if (std::abs(w - 4861.0) < 8.0) {
                double d = (w - 4861.0) / 3.0;
                flux[i] *= 1.0 - 0.08 * std::exp(-d * d);
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "A0V";
        tmplt.type = Type_A;
        tmplt.temperature = 9600;
        tmplt.metallicity = 0.0;
        tmplt.surfaceGravity = 4.0;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            double T = tmplt.temperature;
            double hc_kT = 1.438777e7 / (T * w);
            flux[i] = 1.0 / (std::pow(w, 5.0) * (std::exp(hc_kT) - 1.0));
            if (i > 0 && std::isinf(flux[i])) flux[i] = flux[i-1];
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        QVector<double> balmerWaves = {3970.07, 4101.73, 4340.47, 4861.33, 6562.79, 8446.76};
        for (double wlBalmer : balmerWaves) {
            for (int i = 0; i < n; ++i) {
                double w = wl[i];
                if (std::abs(w - wlBalmer) < 15.0) {
                    double d = (w - wlBalmer) / 3.0;
                    double depth = (wlBalmer < 5000) ? 0.25 : 0.15;
                    flux[i] *= 1.0 - depth * std::exp(-d * d);
                }
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "F5V";
        tmplt.type = Type_F;
        tmplt.temperature = 6500;
        tmplt.metallicity = 0.0;
        tmplt.surfaceGravity = 4.3;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            double T = tmplt.temperature;
            double hc_kT = 1.438777e7 / (T * w);
            flux[i] = 1.0 / (std::pow(w, 5.0) * (std::exp(hc_kT) - 1.0));
            if (i > 0 && std::isinf(flux[i])) flux[i] = flux[i-1];
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            if (std::abs(w - 3933.66) < 5.0) {
                double d = (w - 3933.66) / 1.5;
                flux[i] *= 1.0 - 0.3 * std::exp(-d * d);
            }
            if (std::abs(w - 3968.47) < 5.0) {
                double d = (w - 3968.47) / 1.5;
                flux[i] *= 1.0 - 0.25 * std::exp(-d * d);
            }
            if (std::abs(w - 4861.33) < 8.0) {
                double d = (w - 4861.33) / 2.5;
                flux[i] *= 1.0 - 0.12 * std::exp(-d * d);
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "G2V (Sun-like)";
        tmplt.type = Type_G;
        tmplt.temperature = 5778;
        tmplt.metallicity = 0.0;
        tmplt.surfaceGravity = 4.44;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            double T = tmplt.temperature;
            double hc_kT = 1.438777e7 / (T * w);
            flux[i] = 1.0 / (std::pow(w, 5.0) * (std::exp(hc_kT) - 1.0));
            if (i > 0 && std::isinf(flux[i])) flux[i] = flux[i-1];
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            if (std::abs(w - 3933.66) < 6.0) {
                double d = (w - 3933.66) / 1.8;
                flux[i] *= 1.0 - 0.35 * std::exp(-d * d);
            }
            if (std::abs(w - 3968.47) < 6.0) {
                double d = (w - 3968.47) / 1.8;
                flux[i] *= 1.0 - 0.3 * std::exp(-d * d);
            }
            if (std::abs(w - 4226.73) < 4.0) {
                double d = (w - 4226.73) / 1.2;
                flux[i] *= 1.0 - 0.25 * std::exp(-d * d);
            }
            if (std::abs(w - 4861.33) < 8.0) {
                double d = (w - 4861.33) / 2.5;
                flux[i] *= 1.0 - 0.18 * std::exp(-d * d);
            }
            if (std::abs(w - 6562.79) < 8.0) {
                double d = (w - 6562.79) / 2.5;
                flux[i] *= 1.0 - 0.1 * std::exp(-d * d);
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "K5V";
        tmplt.type = Type_K;
        tmplt.temperature = 4400;
        tmplt.metallicity = 0.0;
        tmplt.surfaceGravity = 4.5;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            double T = tmplt.temperature;
            double hc_kT = 1.438777e7 / (T * w);
            flux[i] = 1.0 / (std::pow(w, 5.0) * (std::exp(hc_kT) - 1.0));
            if (i > 0 && std::isinf(flux[i])) flux[i] = flux[i-1];
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            if (std::abs(w - 4226.73) < 6.0) {
                double d = (w - 4226.73) / 2.0;
                flux[i] *= 1.0 - 0.35 * std::exp(-d * d);
            }
            if (std::abs(w - 5167.0) < 5.0) {
                double d = (w - 5167.0) / 2.0;
                flux[i] *= 1.0 - 0.2 * std::exp(-d * d);
            }
            if (std::abs(w - 5889.95) < 4.0) {
                double d = (w - 5889.95) / 1.5;
                flux[i] *= 1.0 - 0.4 * std::exp(-d * d);
            }
            if (std::abs(w - 5895.92) < 4.0) {
                double d = (w - 5895.92) / 1.5;
                flux[i] *= 1.0 - 0.35 * std::exp(-d * d);
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "M2V";
        tmplt.type = Type_M;
        tmplt.temperature = 3500;
        tmplt.metallicity = 0.0;
        tmplt.surfaceGravity = 5.0;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            double T = tmplt.temperature;
            double hc_kT = 1.438777e7 / (T * w);
            flux[i] = 1.0 / (std::pow(w, 5.0) * (std::exp(hc_kT) - 1.0));
            if (i > 0 && std::isinf(flux[i])) flux[i] = flux[i-1];
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        for (int bandStart = 6000; bandStart < 9500; bandStart += 250) {
            for (int i = 0; i < n; ++i) {
                double w = wl[i];
                if (w > bandStart && w < bandStart + 100) {
                    double d = (w - (bandStart + 50)) / 30.0;
                    flux[i] *= 1.0 - 0.15 * std::exp(-d * d);
                }
            }
        }

        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            if (std::abs(w - 6562.79) < 5.0) {
                double d = (w - 6562.79) / 2.0;
                flux[i] *= 1.0 - 0.12 * std::exp(-d * d);
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "QSO (Type 1)";
        tmplt.type = Type_QSO;
        tmplt.temperature = 100000;
        tmplt.metallicity = 3.0;
        tmplt.surfaceGravity = 0.0;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        double alpha = -0.5;
        for (int i = 0; i < n; ++i) {
            double w = wl[i] / 1450.0;
            flux[i] = std::pow(w, alpha);
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        QVector<double> emissionLines = {1033.8, 1215.7, 1240.0, 1304.4, 1396.8,
                                          1549.0, 1640.4, 1909.0, 2796.3,
                                          4101.7, 4340.5, 4861.3, 6562.8};
        for (double wlLine : emissionLines) {
            for (int i = 0; i < n; ++i) {
                double w = wl[i];
                if (std::abs(w - wlLine) < 30.0) {
                    double d = (w - wlLine) / 10.0;
                    double strength = (wlLine > 3000) ? 0.5 : 0.8;
                    flux[i] *= 1.0 + strength * std::exp(-d * d);
                }
            }
        }

        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            if (w < 3727.0) {
                double blueSlope = std::pow(w / 3000.0, -0.5);
                flux[i] *= blueSlope;
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "Elliptical Galaxy";
        tmplt.type = Type_Galaxy;
        tmplt.temperature = 5500;
        tmplt.metallicity = -0.5;
        tmplt.surfaceGravity = 0.0;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            double T = 5000.0;
            double hc_kT = 1.438777e7 / (T * w);
            flux[i] = 1.0 / (std::pow(w, 4.5) * (std::exp(hc_kT) - 1.0));
            if (i > 0 && std::isinf(flux[i])) flux[i] = flux[i-1];
        }
        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        for (int i = 0; i < n; ++i) {
            double w = wl[i];
            if (std::abs(w - 4000.0) < 200.0) {
                double d = (w - 4000.0) / 80.0;
                flux[i] *= 1.0 - 0.15 * (1.0 + std::tanh(-d)) / 2.0;
            }
            if (std::abs(w - 5175.0) < 30.0) {
                double d = (w - 5175.0) / 10.0;
                flux[i] *= 1.0 - 0.1 * std::exp(-d * d);
            }
            if (std::abs(w - 5890.0) < 15.0) {
                double d = (w - 5890.0) / 5.0;
                flux[i] *= 1.0 - 0.08 * std::exp(-d * d);
            }
            if (std::abs(w - 6563.0) < 10.0) {
                double d = (w - 6563.0) / 3.0;
                flux[i] *= 1.0 + 0.1 * std::exp(-d * d);
            }
        }

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }

    {
        TemplateSpectrum tmplt;
        tmplt.name = "H II Region";
        tmplt.type = Type_HII;
        tmplt.temperature = 10000;
        tmplt.metallicity = -0.2;
        tmplt.surfaceGravity = 0.0;
        tmplt.wavelengths = wl;

        QVector<double> flux(n);
        for (int i = 0; i < n; ++i) {
            flux[i] = 0.2;
        }

        QMap<double, double> emissionLines;
        emissionLines[3727.0] = 1.0;
        emissionLines[3869.0] = 0.5;
        emissionLines[3968.5] = 0.7;
        emissionLines[4101.7] = 0.3;
        emissionLines[4340.5] = 0.5;
        emissionLines[4685.7] = 0.2;
        emissionLines[4861.3] = 1.0;
        emissionLines[4958.9] = 0.35;
        emissionLines[5006.8] = 1.0;
        emissionLines[5875.6] = 0.15;
        emissionLines[6300.3] = 0.25;
        emissionLines[6562.8] = 2.8;
        emissionLines[6716.4] = 0.3;
        emissionLines[6730.8] = 0.3;

        for (auto it = emissionLines.begin(); it != emissionLines.end(); ++it) {
            double wlLine = it.key();
            double strength = it.value();
            for (int i = 0; i < n; ++i) {
                double w = wl[i];
                if (std::abs(w - wlLine) < 50.0) {
                    double d = (w - wlLine) / 2.0;
                    flux[i] += strength * std::exp(-d * d);
                }
            }
        }

        double maxFlux = *std::max_element(flux.begin(), flux.end());
        for (int i = 0; i < n; ++i) flux[i] /= maxFlux;

        tmplt.flux = flux;
        m_templates.append(tmplt);
    }
}

void SpectralLibraryMatcher::resampleSpectrum(const QVector<double> &srcWl,
                                                const QVector<double> &srcFlux,
                                                const QVector<double> &dstWl,
                                                QVector<double> &dstFlux)
{
    dstFlux.clear();
    dstFlux.reserve(dstWl.size());

    if (srcWl.size() < 2 || srcFlux.size() != srcWl.size()) {
        return;
    }

    for (double wl : dstWl) {
        if (wl <= srcWl.first()) {
            dstFlux.append(srcFlux.first());
        } else if (wl >= srcWl.last()) {
            dstFlux.append(srcFlux.last());
        } else {
            int lo = 0, hi = srcWl.size() - 1;
            while (lo < hi - 1) {
                int mid = (lo + hi) / 2;
                if (srcWl[mid] <= wl) {
                    lo = mid;
                } else {
                    hi = mid;
                }
            }
            double t = (wl - srcWl[lo]) / (srcWl[hi] - srcWl[lo]);
            double f = srcFlux[lo] * (1.0 - t) + srcFlux[hi] * t;
            dstFlux.append(f);
        }
    }
}

void SpectralLibraryMatcher::normalizeFlux(QVector<double> &flux)
{
    if (flux.isEmpty()) return;

    double mean = 0.0;
    for (double f : flux) mean += f;
    mean /= flux.size();

    double stddev = 0.0;
    for (double f : flux) {
        double d = f - mean;
        stddev += d * d;
    }
    stddev = std::sqrt(stddev / flux.size());

    if (stddev < 1e-20) return;

    for (int i = 0; i < flux.size(); ++i) {
        flux[i] = (flux[i] - mean) / stddev;
    }
}

double SpectralLibraryMatcher::findBestRedshift(const SpectrumData &spectrum,
                                                 const TemplateSpectrum &tmplt,
                                                 double &bestZ, double &bestScore)
{
    if (!spectrum.isValid() || tmplt.wavelengths.isEmpty()) {
        bestZ = 0.0;
        bestScore = 0.0;
        return 0.0;
    }

    QVector<double> specWl = spectrum.wavelengths();
    QVector<double> specFlux = spectrum.flux();

    QVector<double> tmpltWl = tmplt.wavelengths;
    QVector<double> tmpltFlux = tmplt.flux;

    double minZ = m_minZ;
    double maxZ = m_maxZ;
    double step = m_searchStep;

    bestZ = 0.0;
    bestScore = -1e30;

    int nSteps = qMax(1, static_cast<int>((maxZ - minZ) / step));
    step = (maxZ - minZ) / nSteps;

    QVector<double> specNorm = specFlux;
    normalizeFlux(specNorm);

    for (int i = 0; i <= nSteps; ++i) {
        double z = minZ + i * step;

        QVector<double> shiftedWl;
        for (double w : tmpltWl) {
            shiftedWl.append(w * (1 + z));
        }

        QVector<double> resampledFlux;
        resampleSpectrum(shiftedWl, tmpltFlux, specWl, resampledFlux);

        if (resampledFlux.size() != specFlux.size()) continue;

        QVector<double> tmpltNorm = resampledFlux;
        normalizeFlux(tmpltNorm);

        double score = 0.0;
        switch (m_method) {
        case CrossCorrelation:
        case CorrelationCoefficient:
            score = correlationCoefficient(specNorm, tmpltNorm);
            break;
        case ChiSquared:
            score = -chiSquared(specFlux, resampledFlux);
            break;
        case EuclideanDistance: {
            double dist = 0.0;
            for (int j = 0; j < specNorm.size(); ++j) {
                double d = specNorm[j] - tmpltNorm[j];
                dist += d * d;
            }
            score = -std::sqrt(dist);
            break;
        }
        }

        if (score > bestScore) {
            bestScore = score;
            bestZ = z;
        }
    }

    if (bestZ > minZ && bestZ < maxZ) {
        double z1 = bestZ - step;
        double z2 = bestZ + step;

        QVector<double> shiftedWl1, shiftedWl2;
        for (double w : tmpltWl) {
            shiftedWl1.append(w * (1 + z1));
            shiftedWl2.append(w * (1 + z2));
        }

        QVector<double> resampled1, resampled2;
        resampleSpectrum(shiftedWl1, tmpltFlux, specWl, resampled1);
        resampleSpectrum(shiftedWl2, tmpltFlux, specWl, resampled2);

        if (!resampled1.isEmpty() && !resampled2.isEmpty()) {
            QVector<double> norm1 = resampled1, norm2 = resampled2;
            normalizeFlux(norm1);
            normalizeFlux(norm2);

            double score1 = correlationCoefficient(specNorm, norm1);
            double score2 = correlationCoefficient(specNorm, norm2);

            double a = (score2 + score1 - 2 * bestScore) / (2 * step * step);
            double b = (score2 - score1) / (2 * step);

            if (std::abs(a) > 1e-20) {
                double dZ = -b / (2 * a);
                if (std::abs(dZ) < step) {
                    bestZ += dZ;
                    bestScore = bestScore + b * dZ + a * dZ * dZ;
                }
            }
        }
    }

    return bestScore;
}
