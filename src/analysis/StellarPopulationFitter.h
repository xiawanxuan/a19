#ifndef STELLARPOPULATIONFITTER_H
#define STELLARPOPULATIONFITTER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include <QList>
#include "data_parser/SpectrumData.h"

class StellarPopulationFitter : public QObject
{
    Q_OBJECT

public:
    enum IMF {
        Salpeter,
        Kroupa,
        Chabrier
    };

    enum FitMethod {
        ChiSquared,
        MaximumLikelihood,
        NonNegativeLeastSquares
    };

    struct SSPModel {
        QString name;
        double age;
        double metallicity;
        double mass;
        QVector<double> wavelengths;
        QVector<double> flux;
    };

    struct PopulationResult {
        double totalMass;
        double meanAge;
        double meanMetallicity;
        double massWeightedAge;
        double massWeightedMetallicity;
        double starFormationRate;
        double chiSquared;
        double reducedChiSquared;
        double ageSpread;
        QVector<double> massFractions;
        QVector<double> ageBins;
        QVector<double> metallicityBins;
        QMap<QString, double> componentMasses;
    };

    explicit StellarPopulationFitter(QObject *parent = nullptr);

    void setIMF(IMF imf);
    IMF imf() const;

    void setFitMethod(FitMethod method);
    FitMethod fitMethod() const;

    void setAgeRange(double minAge, double maxAge);
    double minAge() const;
    double maxAge() const;

    void setMetallicityRange(double minZ, double maxZ);
    double minMetallicity() const;
    double maxMetallicity() const;

    void setNAgeBins(int n);
    int nAgeBins() const;

    void setNMetallicityBins(int n);
    int nMetallicityBins() const;

    void setDustLaw(bool enable);
    bool dustLaw() const;

    bool generateSSPLibrary();
    QList<SSPModel> sspLibrary() const;
    int sspCount() const;

    PopulationResult fitPopulation(const SpectrumData &spectrum);

    QVector<double> generateSpectrum(const QVector<double> &wavelengths,
                                      const QVector<double> &weights);

    static QString imfName(IMF imf);
    static QString fitMethodName(FitMethod method);
    static QString ageToString(double ageGyr);

signals:
    void fittingProgress(int current, int total);
    void fittingComplete(const PopulationResult &result);

private:
    void generateSSPSpectrum(double age, double metallicity,
                              const QVector<double> &wavelengths,
                              QVector<double> &flux);

    double blackbodyFlux(double wavelength, double temperature);
    double metallicityCorrection(double wavelength, double metallicity);
    double dustExtinction(double wavelength, double av, double rv = 3.1);

    bool nnlsFit(const QVector<QVector<double>> &A, const QVector<double> &b,
                 QVector<double> &x, double &chiSq);

    IMF m_imf;
    FitMethod m_fitMethod;
    double m_minAge;
    double m_maxAge;
    double m_minMetallicity;
    double m_maxMetallicity;
    int m_nAgeBins;
    int m_nMetallicityBins;
    bool m_dustLaw;

    QList<SSPModel> m_sspLibrary;
    bool m_libraryGenerated;
};

#endif // STELLARPOPULATIONFITTER_H
