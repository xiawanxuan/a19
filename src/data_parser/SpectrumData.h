#ifndef SPECTRUMDATA_H
#define SPECTRUMDATA_H

#include <QVector>
#include <QString>
#include <QMap>
#include <QDateTime>
#include <QMetaType>

struct SpectralLine {
    double wavelength;
    double flux;
    double continuum;
    double equivalentWidth;
    double fwhm;
    double peakHeight;
    QString name;
    QString element;
    bool isEmission;
    double redshift;
    double restWavelength;
};

class SpectrumData
{
public:
    SpectrumData();

    enum Unit {
        Wavelength_Angstrom,
        Wavelength_Nanometer,
        Wavelength_Micron,
        Flux_Ergs,
        Flux_Jansky,
        Flux_Counts
    };

    QString name() const;
    void setName(const QString &name);

    QString filePath() const;
    void setFilePath(const QString &path);

    QVector<double> wavelengths() const;
    void setWavelengths(const QVector<double> &wavelengths);

    QVector<double> flux() const;
    void setFlux(const QVector<double> &flux);

    QVector<double> error() const;
    void setError(const QVector<double> &error);

    QVector<double> continuum() const;
    void setContinuum(const QVector<double> &continuum);

    QVector<SpectralLine> spectralLines() const;
    void setSpectralLines(const QVector<SpectralLine> &lines);
    void addSpectralLine(const SpectralLine &line);
    void clearSpectralLines();

    QMap<QString, QString> header() const;
    void setHeader(const QMap<QString, QString> &header);
    void setHeaderValue(const QString &key, const QString &value);

    double wavelengthUnit() const;
    void setWavelengthUnit(Unit unit);

    double fluxUnit() const;
    void setFluxUnit(Unit unit);

    double redshift() const;
    void setRedshift(double z);

    bool isCalibrated() const;
    void setCalibrated(bool calibrated);

    QDateTime observationDate() const;
    void setObservationDate(const QDateTime &date);

    QString objectName() const;
    void setObjectName(const QString &name);

    int size() const;

    double minWavelength() const;
    double maxWavelength() const;
    double minFlux() const;
    double maxFlux() const;

    bool isValid() const;

    SpectrumData copy() const;

    double fluxAtWavelength(double wavelength) const;
    int indexAtWavelength(double wavelength) const;

private:
    QString m_name;
    QString m_filePath;
    QVector<double> m_wavelengths;
    QVector<double> m_flux;
    QVector<double> m_error;
    QVector<double> m_continuum;
    QVector<SpectralLine> m_spectralLines;
    QMap<QString, QString> m_header;
    Unit m_wavelengthUnit;
    Unit m_fluxUnit;
    double m_redshift;
    bool m_isCalibrated;
    QDateTime m_observationDate;
    QString m_objectName;

    mutable double m_minWavelength;
    mutable double m_maxWavelength;
    mutable double m_minFlux;
    mutable double m_maxFlux;
    mutable bool m_statsDirty;

    void updateStats() const;
};

Q_DECLARE_METATYPE(SpectrumData)
Q_DECLARE_METATYPE(SpectralLine)

#endif // SPECTRUMDATA_H
