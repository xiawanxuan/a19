#ifndef REPORTGENERATORDIALOG_H
#define REPORTGENERATORDIALOG_H

#include <QDialog>
#include "data_parser/SpectrumData.h"
#include "analysis/ReportGenerator.h"
#include "analysis/SpectralLibraryMatcher.h"
#include "analysis/StellarPopulationFitter.h"

class QLineEdit;
class QPushButton;
class QGroupBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QProgressBar;
class QTextEdit;

class ReportGeneratorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReportGeneratorDialog(QWidget *parent = nullptr);

    void setSpectrum(const SpectrumData &spectrum);
    void setMatchResults(const QList<SpectralLibraryMatcher::MatchResult> &matches);
    void setPopulationResult(const StellarPopulationFitter::PopulationResult &popResult);

private slots:
    void generateReport();
    void browseOutputPath();
    void onReportProgress(int current, int total);
    void onReportComplete(bool success, const QString &path);

private:
    void setupUi();

    QGroupBox *m_contentGroup;
    QCheckBox *m_spectrumPlotCheck;
    QCheckBox *m_lineTableCheck;
    QCheckBox *m_libraryMatchCheck;
    QCheckBox *m_populationFitCheck;
    QCheckBox *m_redshiftCheck;
    QCheckBox *m_headerCheck;

    QGroupBox *m_formatGroup;
    QComboBox *m_formatCombo;

    QGroupBox *m_metaGroup;
    QLineEdit *m_titleEdit;
    QLineEdit *m_authorEdit;

    QLineEdit *m_outputPathEdit;
    QPushButton *m_browseBtn;

    QPushButton *m_generateBtn;
    QPushButton *m_closeBtn;

    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QTextEdit *m_previewEdit;

    SpectrumData m_spectrum;
    QList<SpectralLibraryMatcher::MatchResult> m_matchResults;
    StellarPopulationFitter::PopulationResult m_popResult;
    ReportGenerator *m_generator;
};

#endif // REPORTGENERATORDIALOG_H
