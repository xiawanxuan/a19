#include "ExportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Export Spectrum");
    setMinimumSize(500, 400);

    setupUi();
}

void ExportDialog::setSpectrum(const SpectrumData &spectrum)
{
    m_currentSpectrum = spectrum;
    m_exportAllCheck->setEnabled(false);
    m_exportAllCheck->setChecked(false);
    updatePreview();
}

void ExportDialog::setSpectra(const QVector<SpectrumData> &spectra)
{
    m_spectra = spectra;
    if (!spectra.isEmpty()) {
        m_currentSpectrum = spectra.first();
    }
    m_exportAllCheck->setEnabled(spectra.size() > 1);
    m_exportAllCheck->setChecked(spectra.size() > 1);
    updatePreview();
}

QString ExportDialog::outputFile() const
{
    return m_outputEdit->text();
}

DataParser::FileFormat ExportDialog::format() const
{
    return static_cast<DataParser::FileFormat>(
        m_formatCombo->currentData().toInt());
}

bool ExportDialog::exportAll() const
{
    return m_exportAllCheck->isChecked();
}

void ExportDialog::browseOutput()
{
    QString formatName = m_formatCombo->currentText().toLower();
    QString filter;

    switch (format()) {
    case DataParser::Format_CSV:
        filter = "CSV Files (*.csv)";
        break;
    case DataParser::Format_TXT:
        filter = "Text Files (*.txt *.dat)";
        break;
    case DataParser::Format_JSON:
        filter = "JSON Files (*.json)";
        break;
    case DataParser::Format_FITS:
        filter = "FITS Files (*.fits *.fit)";
        break;
    default:
        filter = "All Files (*.*)";
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Export Spectrum",
        QString(),
        filter
    );

    if (!fileName.isEmpty()) {
        m_outputEdit->setText(fileName);
        updatePreview();
    }
}

void ExportDialog::exportData()
{
    if (!validateInput()) {
        return;
    }

    DataParser parser;

    if (m_exportAllCheck->isChecked() && !m_spectra.isEmpty()) {
        QFileInfo info(m_outputEdit->text());
        QString dir = info.absolutePath();
        QString baseName = info.baseName();
        QString ext = info.suffix();

        int successCount = 0;
        for (int i = 0; i < m_spectra.size(); ++i) {
            QString fileName;
            if (m_spectra.size() == 1) {
                fileName = m_outputEdit->text();
            } else {
                fileName = QString("%1/%2_%3.%4")
                    .arg(dir, baseName, m_spectra[i].name(), ext);
            }

            if (parser.saveSpectrum(m_spectra[i], fileName, format())) {
                successCount++;
            }
        }

        if (successCount > 0) {
            QMessageBox::information(this, "Export Complete",
                                     QString("Successfully exported %1 of %2 spectra.")
                                         .arg(successCount).arg(m_spectra.size()));
            accept();
        } else {
            QMessageBox::critical(this, "Export Failed",
                                  "Failed to export spectra.");
        }
    } else {
        if (parser.saveSpectrum(m_currentSpectrum, m_outputEdit->text(), format())) {
            QMessageBox::information(this, "Export Complete",
                                     "Spectrum exported successfully.");
            accept();
        } else {
            QMessageBox::critical(this, "Export Failed",
                                  QString("Failed to export: %1").arg(parser.lastError()));
        }
    }
}

void ExportDialog::updatePreview()
{
    QString info;

    if (m_exportAllCheck->isChecked() && !m_spectra.isEmpty()) {
        info = QString("Export %1 spectra\n")
                   .arg(m_spectra.size());

        qint64 totalSize = 0;
        for (const SpectrumData &spec : m_spectra) {
            totalSize += spec.size() * 16;
        }
        info += QString("Estimated size: %1 KB")
                    .arg(totalSize / 1024);
    } else if (m_currentSpectrum.isValid()) {
        info = QString("Spectrum: %1\n%2 data points\nWavelength: %3 - %4 Å")
                   .arg(m_currentSpectrum.name())
                   .arg(m_currentSpectrum.size())
                   .arg(m_currentSpectrum.minWavelength(), 0, 'f', 2)
                   .arg(m_currentSpectrum.maxWavelength(), 0, 'f', 2);

        qint64 estSize = m_currentSpectrum.size() * 16;
        info += QString("\nEstimated size: %1 KB").arg(estSize / 1024);
    } else {
        info = "No spectrum selected";
    }

    m_previewLabel->setText(info);
}

void ExportDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *outputGroup = new QGroupBox("Output");
    QGridLayout *outputLayout = new QGridLayout(outputGroup);

    outputLayout->addWidget(new QLabel("Format:"), 0, 0);
    m_formatCombo = new QComboBox();
    m_formatCombo->addItem("CSV", DataParser::Format_CSV);
    m_formatCombo->addItem("Text", DataParser::Format_TXT);
    m_formatCombo->addItem("JSON", DataParser::Format_JSON);
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportDialog::updatePreview);
    outputLayout->addWidget(m_formatCombo, 0, 1);

    outputLayout->addWidget(new QLabel("Output file:"), 1, 0);
    QHBoxLayout *fileLayout = new QHBoxLayout();
    m_outputEdit = new QLineEdit();
    m_browseBtn = new QPushButton("Browse...");
    connect(m_browseBtn, &QPushButton::clicked,
            this, &ExportDialog::browseOutput);
    fileLayout->addWidget(m_outputEdit);
    fileLayout->addWidget(m_browseBtn);
    outputLayout->addLayout(fileLayout, 1, 1);

    mainLayout->addWidget(outputGroup);

    QGroupBox *optionsGroup = new QGroupBox("Options");
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);

    m_exportAllCheck = new QCheckBox("Export all loaded spectra");
    m_exportAllCheck->setChecked(false);
    m_exportAllCheck->setEnabled(false);
    connect(m_exportAllCheck, &QCheckBox::toggled,
            this, &ExportDialog::updatePreview);
    optionsLayout->addWidget(m_exportAllCheck);

    m_includeHeaderCheck = new QCheckBox("Include header metadata");
    m_includeHeaderCheck->setChecked(true);
    optionsLayout->addWidget(m_includeHeaderCheck);

    m_includeLinesCheck = new QCheckBox("Include spectral line data");
    m_includeLinesCheck->setChecked(true);
    optionsLayout->addWidget(m_includeLinesCheck);

    m_includeContinuumCheck = new QCheckBox("Include continuum");
    m_includeContinuumCheck->setChecked(true);
    optionsLayout->addWidget(m_includeContinuumCheck);

    m_includeErrorCheck = new QCheckBox("Include error bars");
    m_includeErrorCheck->setChecked(true);
    optionsLayout->addWidget(m_includeErrorCheck);

    mainLayout->addWidget(optionsGroup);

    QGroupBox *previewGroup = new QGroupBox("Preview");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

    m_previewLabel = new QLabel("No spectrum selected");
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setStyleSheet("QLabel { padding: 10px; }");
    previewLayout->addWidget(m_previewLabel);

    m_sizeEstimateLabel = new QLabel("");
    previewLayout->addWidget(m_sizeEstimateLabel);

    mainLayout->addWidget(previewGroup);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_exportBtn = new QPushButton("Export");
    m_exportBtn->setDefault(true);
    m_cancelBtn = new QPushButton("Cancel");

    connect(m_exportBtn, &QPushButton::clicked,
            this, &ExportDialog::exportData);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);

    btnLayout->addStretch();
    btnLayout->addWidget(m_exportBtn);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}

bool ExportDialog::validateInput()
{
    if (m_outputEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Missing Output",
                             "Please specify an output file.");
        return false;
    }

    QFileInfo info(m_outputEdit->text());
    QDir dir = info.dir();
    if (!dir.exists()) {
        QMessageBox::warning(this, "Invalid Directory",
                             "The output directory does not exist.");
        return false;
    }

    if (!m_exportAllCheck->isChecked() && !m_currentSpectrum.isValid()) {
        QMessageBox::warning(this, "No Spectrum",
                             "No spectrum data to export.");
        return false;
    }

    if (m_exportAllCheck->isChecked() && m_spectra.isEmpty()) {
        QMessageBox::warning(this, "No Spectra",
                             "No spectra data to export.");
        return false;
    }

    return true;
}
