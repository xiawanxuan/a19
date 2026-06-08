#include "ReportGeneratorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

ReportGeneratorDialog::ReportGeneratorDialog(QWidget *parent)
    : QDialog(parent)
    , m_generator(new ReportGenerator(this))
{
    setWindowTitle(tr("Analysis Report Generator"));
    setMinimumSize(650, 600);
    setupUi();

    connect(m_generator, &ReportGenerator::reportProgress,
            this, &ReportGeneratorDialog::onReportProgress);
    connect(m_generator, &ReportGenerator::reportComplete,
            this, &ReportGeneratorDialog::onReportComplete);
}

void ReportGeneratorDialog::setSpectrum(const SpectrumData &spectrum)
{
    m_spectrum = spectrum;
}

void ReportGeneratorDialog::setMatchResults(
    const QList<SpectralLibraryMatcher::MatchResult> &matches)
{
    m_matchResults = matches;
    if (!matches.isEmpty()) {
        m_libraryMatchCheck->setChecked(true);
        m_libraryMatchCheck->setEnabled(true);
    } else {
        m_libraryMatchCheck->setChecked(false);
        m_libraryMatchCheck->setEnabled(false);
    }
}

void ReportGeneratorDialog::setPopulationResult(
    const StellarPopulationFitter::PopulationResult &popResult)
{
    m_popResult = popResult;
    if (popResult.totalMass > 0) {
        m_populationFitCheck->setChecked(true);
        m_populationFitCheck->setEnabled(true);
    } else {
        m_populationFitCheck->setChecked(false);
        m_populationFitCheck->setEnabled(false);
    }
}

void ReportGeneratorDialog::generateReport()
{
    if (!m_spectrum.isValid()) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("No valid spectrum loaded."));
        return;
    }

    QString outputPath = m_outputPathEdit->text().trimmed();
    if (outputPath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("Please specify an output file path."));
        return;
    }

    QFileInfo fi(outputPath);
    if (!fi.dir().exists()) {
        if (!QDir().mkpath(fi.dir().absolutePath())) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Cannot create output directory."));
            return;
        }
    }

    m_generateBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("Generating report..."));

    ReportGenerator::ReportConfig config;
    config.title = m_titleEdit->text();
    config.author = m_authorEdit->text();
    config.includeSpectrumPlot = m_spectrumPlotCheck->isChecked();
    config.includeLineTable = m_lineTableCheck->isChecked();
    config.includeLibraryMatch = m_libraryMatchCheck->isChecked();
    config.includePopulationFit = m_populationFitCheck->isChecked();
    config.includeRedshift = m_redshiftCheck->isChecked();
    config.includeHeader = m_headerCheck->isChecked();

    m_generator->setConfig(config);

    ReportGenerator::ReportFormat format =
        static_cast<ReportGenerator::ReportFormat>(
            m_formatCombo->currentData().toInt());

    bool success = false;

    if (m_populationFitCheck->isChecked() && m_libraryMatchCheck->isChecked()) {
        success = m_generator->generateFullReport(
            m_spectrum, m_matchResults, m_popResult, outputPath, format);
    } else if (m_libraryMatchCheck->isChecked()) {
        success = m_generator->generateReport(
            m_spectrum, m_matchResults, outputPath, format);
    } else if (m_populationFitCheck->isChecked()) {
        success = m_generator->generateReport(
            m_spectrum, m_popResult, outputPath, format);
    } else {
        success = m_generator->generateReport(
            m_spectrum, outputPath, format);
    }

    m_generateBtn->setEnabled(true);
    m_progressBar->setVisible(false);

    if (success) {
        m_statusLabel->setText(tr("Report generated successfully!"));
        if (format == ReportGenerator::HTML) {
            QString html = m_generator->generateHtmlReport(
                m_spectrum, m_matchResults, m_popResult,
                m_populationFitCheck->isChecked());
            m_previewEdit->setHtml(html);
        } else if (format == ReportGenerator::Text) {
            QString text = m_generator->generateTextReport(
                m_spectrum, m_matchResults, m_popResult,
                m_populationFitCheck->isChecked());
            m_previewEdit->setPlainText(text);
        } else {
            m_previewEdit->setPlainText(
                tr("PDF report saved to:\n%1").arg(outputPath));
        }
    } else {
        m_statusLabel->setText(tr("Failed to generate report."));
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to generate report."));
    }
}

void ReportGeneratorDialog::browseOutputPath()
{
    ReportGenerator::ReportFormat format =
        static_cast<ReportGenerator::ReportFormat>(
            m_formatCombo->currentData().toInt());

    QString filter;
    QString defaultExt;

    switch (format) {
    case ReportGenerator::PDF:
        filter = tr("PDF Files (*.pdf)");
        defaultExt = ".pdf";
        break;
    case ReportGenerator::HTML:
        filter = tr("HTML Files (*.html *.htm)");
        defaultExt = ".html";
        break;
    case ReportGenerator::Text:
        filter = tr("Text Files (*.txt)");
        defaultExt = ".txt";
        break;
    default:
        filter = tr("All Files (*)");
        defaultExt = ".pdf";
    }

    QString defaultPath = QDir::homePath() + "/spectrum_report" + defaultExt;
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save Report"), defaultPath, filter);

    if (!fileName.isEmpty()) {
        m_outputPathEdit->setText(fileName);
    }
}

void ReportGeneratorDialog::onReportProgress(int current, int total)
{
    if (total > 0) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(current);
    }
}

void ReportGeneratorDialog::onReportComplete(bool success, const QString &path)
{
    Q_UNUSED(success)
    Q_UNUSED(path)
    m_progressBar->setVisible(false);
}

void ReportGeneratorDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_metaGroup = new QGroupBox(tr("Report Metadata"), this);
    QFormLayout *metaLayout = new QFormLayout(m_metaGroup);

    m_titleEdit = new QLineEdit(m_metaGroup);
    m_titleEdit->setText(tr("Astronomical Spectrum Analysis Report"));
    metaLayout->addRow(tr("Title:"), m_titleEdit);

    m_authorEdit = new QLineEdit(m_metaGroup);
    m_authorEdit->setText(tr("Spectrum Analyzer"));
    metaLayout->addRow(tr("Author:"), m_authorEdit);

    mainLayout->addWidget(m_metaGroup);

    m_contentGroup = new QGroupBox(tr("Report Content"), this);
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentGroup);

    m_spectrumPlotCheck = new QCheckBox(tr("Include spectrum plot"), m_contentGroup);
    m_spectrumPlotCheck->setChecked(true);
    contentLayout->addWidget(m_spectrumPlotCheck);

    m_lineTableCheck = new QCheckBox(tr("Include spectral line table"), m_contentGroup);
    m_lineTableCheck->setChecked(true);
    contentLayout->addWidget(m_lineTableCheck);

    m_redshiftCheck = new QCheckBox(tr("Include redshift analysis"), m_contentGroup);
    m_redshiftCheck->setChecked(true);
    contentLayout->addWidget(m_redshiftCheck);

    m_libraryMatchCheck = new QCheckBox(tr("Include spectral library matching"), m_contentGroup);
    m_libraryMatchCheck->setChecked(false);
    contentLayout->addWidget(m_libraryMatchCheck);

    m_populationFitCheck = new QCheckBox(tr("Include stellar population analysis"), m_contentGroup);
    m_populationFitCheck->setChecked(false);
    contentLayout->addWidget(m_populationFitCheck);

    m_headerCheck = new QCheckBox(tr("Include header and footer"), m_contentGroup);
    m_headerCheck->setChecked(true);
    contentLayout->addWidget(m_headerCheck);

    mainLayout->addWidget(m_contentGroup);

    m_formatGroup = new QGroupBox(tr("Output Format"), this);
    QFormLayout *formatLayout = new QFormLayout(m_formatGroup);

    m_formatCombo = new QComboBox(m_formatGroup);
    m_formatCombo->addItem(tr("PDF (Portable Document Format)"), ReportGenerator::PDF);
    m_formatCombo->addItem(tr("HTML (Web Page)"), ReportGenerator::HTML);
    m_formatCombo->addItem(tr("Plain Text"), ReportGenerator::Text);
    formatLayout->addRow(tr("Format:"), m_formatCombo);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    m_outputPathEdit = new QLineEdit(m_formatGroup);
    m_outputPathEdit->setText(QDir::homePath() + "/spectrum_report.pdf");
    pathLayout->addWidget(m_outputPathEdit);

    m_browseBtn = new QPushButton(tr("Browse..."), m_formatGroup);
    connect(m_browseBtn, &QPushButton::clicked,
            this, &ReportGeneratorDialog::browseOutputPath);
    pathLayout->addWidget(m_browseBtn);

    formatLayout->addRow(tr("Output File:"), pathLayout);

    mainLayout->addWidget(m_formatGroup);

    QGroupBox *previewGroup = new QGroupBox(tr("Preview"), this);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

    m_previewEdit = new QTextEdit(previewGroup);
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setPlainText(tr("Report preview will appear here..."));
    previewLayout->addWidget(m_previewEdit);

    mainLayout->addWidget(previewGroup, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel(tr("Ready"), this);
    m_statusLabel->setStyleSheet("color: #666;");
    mainLayout->addWidget(m_statusLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_generateBtn = new QPushButton(tr("Generate Report"), this);
    m_generateBtn->setDefault(true);
    connect(m_generateBtn, &QPushButton::clicked,
            this, &ReportGeneratorDialog::generateReport);
    buttonLayout->addWidget(m_generateBtn);

    m_closeBtn = new QPushButton(tr("Close"), this);
    connect(m_closeBtn, &QPushButton::clicked,
            this, &QDialog::accept);
    buttonLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(buttonLayout);
}
