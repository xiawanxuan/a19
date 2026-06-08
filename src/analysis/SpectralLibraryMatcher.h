#ifndef SPECTRALLIBRARYMATCHER_H
#define SPECTRALLIBRARYMATCHER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include <QList>
#include "data_parser/SpectrumData.h"

class SpectralLibraryMatcher : public QObject
{
    Q_OBJECT

public:
    enum MatchMethod {
        CrossCorrelation,
        ChiSquared,
        CorrelationCoefficient,
        EuclideanDistance
    };

    enum ObjectType {
        Type_O,
        Type_B,
        Type_A,
        Type_F,
        Type_G,
        Type_K,
        Type_M,
        Type_WD,
        Type_QSO,
        Type_Galaxy,
        Type_HII,
        Type_Seyfert1,
        Type_Seyfert2,
        Type_Liner,
        Type_Unknown
    };

    struct MatchResult {
        QString templateName;
        ObjectType objectType;
        double matchScore;
        double correlationCoefficient;
        double chiSquared;
        double redshift;
        double scaleFactor;
        int rank;
    };

    struct TemplateSpectrum {
        QString name;
        ObjectType type;
        double temperature;
        double metallicity;
        double surfaceGravity;
        QVector<double> wavelengths;
        QVector<double> flux;
    };

    explicit SpectralLibraryMatcher(QObject *parent = nullptr);

    void setMatchMethod(MatchMethod method);
    MatchMethod matchMethod() const;

    void setRedshiftRange(double minZ, double maxZ);
    double minRedshift() const;
    double maxRedshift() const;

    void setSearchStep(double step);
    double searchStep() const;

    void setTopN(int n);
    int topN() const;

    bool loadTemplateLibrary(const QString &path);
    bool loadBuiltInLibrary();

    QList<TemplateSpectrum> templates() const;
    int templateCount() const;

    QList<MatchResult> matchSpectrum(const SpectrumData &spectrum);
    MatchResult bestMatch(const SpectrumData &spectrum);

    static QString objectTypeName(ObjectType type);
    static QString objectTypeDescription(ObjectType type);
    static QList<ObjectType> allObjectTypes();

    double crossCorrelation(const QVector<double> &x, const QVector<double> &y1,
                            const QVector<double> &y2);
    double correlationCoefficient(const QVector<double> &y1,
                                   const QVector<double> &y2);
    double chiSquared(const QVector<double> &y1, const QVector<double> &y2,
                       double sigma = 1.0);

signals:
    void matchingProgress(int current, int total);
    void matchingComplete(const QList<MatchResult> &results);

private:
    void generateBuiltInTemplates();
    void resampleSpectrum(const QVector<double> &srcWl, const QVector<double> &srcFlux,
                           const QVector<double> &dstWl, QVector<double> &dstFlux);
    void normalizeFlux(QVector<double> &flux);
    double findBestRedshift(const SpectrumData &spectrum,
                             const TemplateSpectrum &tmplt,
                             double &bestZ, double &bestScore);

    MatchMethod m_method;
    double m_minZ;
    double m_maxZ;
    double m_searchStep;
    int m_topN;

    QList<TemplateSpectrum> m_templates;
    bool m_libraryLoaded;
};

#endif // SPECTRALLIBRARYMATCHER_H
