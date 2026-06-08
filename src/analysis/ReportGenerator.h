#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include "data_parser/SpectrumData.h"
#include "analysis/SpectralLibraryMatcher.h"
#include "analysis/StellarPopulationFitter.h"

class QPixmap;
class QTextDocument;

class ReportGenerator : public QObject
{
    Q_OBJECT

public:
    enum ReportFormat {
        PDF,
        HTML,
        Text
    };

    struct ReportConfig {
        QString title;
        QString author;
        QString date;
        bool includeSpectrumPlot;
        bool includeLineTable;
        bool includeLibraryMatch;
        bool includePopulationFit;
        bool includeRedshift;
        bool includeHeader;

        ReportConfig()
            : title("Astronomical Spectrum Analysis Report")
            , author("Spectrum Analyzer")
            , date("")
            , includeSpectrumPlot(true)
            , includeLineTable(true)
            , includeLibraryMatch(true)
            , includePopulationFit(false)
            , includeRedshift(true)
            , includeHeader(true)
        {}
    };

    explicit ReportGenerator(QObject *parent = nullptr);

    void setConfig(const ReportConfig &config);
    ReportConfig config() const;

    bool generateReport(const SpectrumData &spectrum,
                        const QString &outputPath,
                        ReportFormat format = PDF);

    bool generateReport(const SpectrumData &spectrum,
                        const QList<SpectralLibraryMatcher::MatchResult> &matches,
                        const QString &outputPath,
                        ReportFormat format = PDF);

    bool generateReport(const SpectrumData &spectrum,
                        const StellarPopulationFitter::PopulationResult &popResult,
                        const QString &outputPath,
                        ReportFormat format = PDF);

    bool generateFullReport(const SpectrumData &spectrum,
                             const QList<SpectralLibraryMatcher::MatchResult> &matches,
                             const StellarPopulationFitter::PopulationResult &popResult,
                             const QString &outputPath,
                             ReportFormat format = PDF);

    QString generateHtmlReport(const SpectrumData &spectrum,
                                const QList<SpectralLibraryMatcher::MatchResult> &matches,
                                const StellarPopulationFitter::PopulationResult &popResult,
                                bool includePop);

    QString generateTextReport(const SpectrumData &spectrum,
                                const QList<SpectralLibraryMatcher::MatchResult> &matches,
                                const StellarPopulationFitter::PopulationResult &popResult,
                                bool includePop);

signals:
    void reportProgress(int current, int total);
    void reportComplete(bool success, const QString &path);

private:
    QString generateHeaderHtml();
    QString generateSpectrumInfoHtml(const SpectrumData &spectrum);
    QString generateSpectralLinesHtml(const SpectrumData &spectrum);
    QString generateRedshiftHtml(const SpectrumData &spectrum);
    QString generateLibraryMatchHtml(
        const QList<SpectralLibraryMatcher::MatchResult> &matches);
    QString generatePopulationFitHtml(
        const StellarPopulationFitter::PopulationResult &result);
    QString generateFooterHtml();

    QPixmap generateSpectrumPlot(const SpectrumData &spectrum,
                                   int width, int height);

    bool savePdf(const QString &html, const QString &outputPath);
    bool saveHtml(const QString &html, const QString &outputPath);
    bool saveText(const QString &text, const QString &outputPath);

    ReportConfig m_config;
};

#endif // REPORTGENERATOR_H
