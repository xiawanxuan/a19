#include "SpectrumData.h"
#include <cmath>
#include <algorithm>

SpectrumData::SpectrumData()
    : m_wavelengthUnit(Wavelength_Angstrom)
    , m_fluxUnit(Flux_Counts)
    , m_redshift(0.0)
    , m_isCalibrated(false)
    , m_minWavelength(0.0)
    , m_maxWavelength(0.0)
    , m_minFlux(0.0)
    , m_maxFlux(0.0)
    , m_statsDirty(true)
{
}

QString SpectrumData::name() const
{
    return m_name;
}

void SpectrumData::setName(const QString &name)
{
    m_name = name;
}

QString SpectrumData::filePath() const
{
    return m_filePath;
}

void SpectrumData::setFilePath(const QString &path)
{
    m_filePath = path;
}

QVector<double> SpectrumData::wavelengths() const
{
    return m_wavelengths;
}

void SpectrumData::setWavelengths(const QVector<double> &wavelengths)
{
    m_wavelengths = wavelengths;
    m_statsDirty = true;
}

QVector<double> SpectrumData::flux() const
{
    return m_flux;
}

void SpectrumData::setFlux(const QVector<double> &flux)
{
    m_flux = flux;
    m_statsDirty = true;
}

QVector<double> SpectrumData::error() const
{
    return m_error;
}

void SpectrumData::setError(const QVector<double> &error)
{
    m_error = error;
}

QVector<double> SpectrumData::continuum() const
{
    return m_continuum;
}

void SpectrumData::setContinuum(const QVector<double> &continuum)
{
    m_continuum = continuum;
}

QVector<SpectralLine> SpectrumData::spectralLines() const
{
    return m_spectralLines;
}

void SpectrumData::setSpectralLines(const QVector<SpectralLine> &lines)
{
    m_spectralLines = lines;
}

void SpectrumData::addSpectralLine(const SpectralLine &line)
{
    m_spectralLines.append(line);
}

void SpectrumData::clearSpectralLines()
{
    m_spectralLines.clear();
}

QMap<QString, QString> SpectrumData::header() const
{
    return m_header;
}

void SpectrumData::setHeader(const QMap<QString, QString> &header)
{
    m_header = header;
}

void SpectrumData::setHeaderValue(const QString &key, const QString &value)
{
    m_header.insert(key, value);
}

double SpectrumData::wavelengthUnit() const
{
    return m_wavelengthUnit;
}

void SpectrumData::setWavelengthUnit(Unit unit)
{
    m_wavelengthUnit = unit;
}

double SpectrumData::fluxUnit() const
{
    return m_fluxUnit;
}

void SpectrumData::setFluxUnit(Unit unit)
{
    m_fluxUnit = unit;
}

double SpectrumData::redshift() const
{
    return m_redshift;
}

void SpectrumData::setRedshift(double z)
{
    m_redshift = z;
}

bool SpectrumData::isCalibrated() const
{
    return m_isCalibrated;
}

void SpectrumData::setCalibrated(bool calibrated)
{
    m_isCalibrated = calibrated;
}

QDateTime SpectrumData::observationDate() const
{
    return m_observationDate;
}

void SpectrumData::setObservationDate(const QDateTime &date)
{
    m_observationDate = date;
}

QString SpectrumData::objectName() const
{
    return m_objectName;
}

void SpectrumData::setObjectName(const QString &name)
{
    m_objectName = name;
}

int SpectrumData::size() const
{
    return qMin(m_wavelengths.size(), m_flux.size());
}

double SpectrumData::minWavelength() const
{
    if (m_statsDirty) updateStats();
    return m_minWavelength;
}

double SpectrumData::maxWavelength() const
{
    if (m_statsDirty) updateStats();
    return m_maxWavelength;
}

double SpectrumData::minFlux() const
{
    if (m_statsDirty) updateStats();
    return m_minFlux;
}

double SpectrumData::maxFlux() const
{
    if (m_statsDirty) updateStats();
    return m_maxFlux;
}

bool SpectrumData::isValid() const
{
    return !m_wavelengths.isEmpty() && !m_flux.isEmpty()
           && m_wavelengths.size() == m_flux.size();
}

SpectrumData SpectrumData::copy() const
{
    SpectrumData copy;
    copy.m_name = m_name;
    copy.m_filePath = m_filePath;
    copy.m_wavelengths = m_wavelengths;
    copy.m_flux = m_flux;
    copy.m_error = m_error;
    copy.m_continuum = m_continuum;
    copy.m_spectralLines = m_spectralLines;
    copy.m_header = m_header;
    copy.m_wavelengthUnit = m_wavelengthUnit;
    copy.m_fluxUnit = m_fluxUnit;
    copy.m_redshift = m_redshift;
    copy.m_isCalibrated = m_isCalibrated;
    copy.m_observationDate = m_observationDate;
    copy.m_objectName = m_objectName;
    copy.m_statsDirty = true;
    return copy;
}

double SpectrumData::fluxAtWavelength(double wavelength) const
{
    int idx = indexAtWavelength(wavelength);
    if (idx < 0 || idx >= m_flux.size()) return 0.0;
    return m_flux[idx];
}

int SpectrumData::indexAtWavelength(double wavelength) const
{
    if (m_wavelengths.isEmpty()) return -1;

    int lo = 0;
    int hi = m_wavelengths.size() - 1;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (m_wavelengths[mid] == wavelength) {
            return mid;
        } else if (m_wavelengths[mid] < wavelength) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    if (hi < 0) return 0;
    if (lo >= m_wavelengths.size()) return m_wavelengths.size() - 1;

    double dHi = std::abs(m_wavelengths[hi] - wavelength);
    double dLo = std::abs(m_wavelengths[lo] - wavelength);
    return (dHi < dLo) ? hi : lo;
}

void SpectrumData::updateStats() const
{
    if (m_wavelengths.isEmpty()) {
        m_minWavelength = 0.0;
        m_maxWavelength = 0.0;
        m_minFlux = 0.0;
        m_maxFlux = 0.0;
        m_statsDirty = false;
        return;
    }

    m_minWavelength = *std::min_element(m_wavelengths.begin(), m_wavelengths.end());
    m_maxWavelength = *std::max_element(m_wavelengths.begin(), m_wavelengths.end());

    if (!m_flux.isEmpty()) {
        m_minFlux = *std::min_element(m_flux.begin(), m_flux.end());
        m_maxFlux = *std::max_element(m_flux.begin(), m_flux.end());
    }

    m_statsDirty = false;
}
