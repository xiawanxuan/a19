#include "LineDetectionDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QMessageBox>
#include <QApplication>

LineDetectionDialog::LineDetectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_lineFitter(new SpectralLineFitter(this))
{
    setWindowTitle("Spectral Line Detection & Fitting");
    setMinimumSize(500, 450);

    connect(m_lineFitter, &SpectralLineFitter::fittingProgress,
            this, &LineDetectionDialog::onDetectionProgress);

    setupUi();
}

void LineDetectionDialog::setSpectrum(const SpectrumData &spectrum)
{
    m_spectrum = spectrum;
    m_processedSpectrum = spectrum;
}

SpectrumData LineDetectionDialog::processedSpectrum() const
{
    return m_processedSpectrum;
}

void LineDetectionDialog::detectLines()
{
    if (!m_spectrum.isValid()) {
        QMessageBox::warning(this, "No Spectrum",
                             "Please load a spectrum first.");
        return;
    }

    m_lineFitter->setDetectionMethod(
        static_cast<SpectralLineFitter::DetectionMethod>(
            m_detectionMethodCombo->currentData().toInt()));

    m_lineFitter->setLineType(
        static_cast<SpectralLineFitter::LineType>(
            m_lineTypeCombo->currentData().toInt()));

    m_lineFitter->setNoiseThreshold(m_noiseThresholdSpin->value());
    m_lineFitter->setMinLineWidth(m_minWidthSpin->value());
    m_lineFitter->setMaxLineWidth(m_maxWidthSpin->value());
    m_lineFitter->setMaxLines(m_maxLinesSpin->value());
    m_lineFitter->setContinuumOrder(m_continuumOrderSpin->value());

    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    QApplication::processEvents();

    m_processedSpectrum = m_spectrum.copy();

    if (m_fitContinuumCheck->isChecked()) {
        QVector<double> cont = m_lineFitter->fitContinuum(m_processedSpectrum);
        m_processedSpectrum.setContinuum(cont);
    }

    bool success = m_lineFitter->fitAllLines(m_processedSpectrum);

    if (success) {
        int numLines = m_processedSpectrum.spectralLines().size();
        m_numLinesLabel->setText(QString("%1 lines detected").arg(numLines));
        m_applyBtn->setEnabled(numLines > 0);
    } else {
        m_numLinesLabel->setText("Detection failed");
        m_applyBtn->setEnabled(false);
    }

    m_progressBar->setValue(100);
}

void LineDetectionDialog::fitSelectedLine()
{
    detectLines();
}

void LineDetectionDialog::onDetectionProgress(int current, int total)
{
    if (total > 0) {
        m_progressBar->setValue((current * 100) / total);
    }
    QApplication::processEvents();
}

void LineDetectionDialog::updatePreview()
{
}

void LineDetectionDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_detectionGroup = new QGroupBox("Detection Parameters");
    QGridLayout *detLayout = new QGridLayout(m_detectionGroup);

    detLayout->addWidget(new QLabel("Detection method:"), 0, 0);
    m_detectionMethodCombo = new QComboBox();
    m_detectionMethodCombo->addItem("Continuum Subtraction",
                                     SpectralLineFitter::ContinuumSubtraction);
    m_detectionMethodCombo->addItem("Peak Detection",
                                     SpectralLineFitter::PeakDetection);
    m_detectionMethodCombo->addItem("Derivative Method",
                                     SpectralLineFitter::DerivativeMethod);
    detLayout->addWidget(m_detectionMethodCombo, 0, 1);

    detLayout->addWidget(new QLabel("Line type:"), 1, 0);
    m_lineTypeCombo = new QComboBox();
    m_lineTypeCombo->addItem("Gaussian", SpectralLineFitter::Gaussian);
    m_lineTypeCombo->addItem("Lorentzian", SpectralLineFitter::Lorentzian);
    m_lineTypeCombo->addItem("Voigt", SpectralLineFitter::Voigt);
    detLayout->addWidget(m_lineTypeCombo, 1, 1);

    detLayout->addWidget(new QLabel("Noise threshold (σ):"), 2, 0);
    m_noiseThresholdSpin = new QDoubleSpinBox();
    m_noiseThresholdSpin->setRange(0.5, 20.0);
    m_noiseThresholdSpin->setValue(3.0);
    m_noiseThresholdSpin->setSingleStep(0.5);
    detLayout->addWidget(m_noiseThresholdSpin, 2, 1);

    detLayout->addWidget(new QLabel("Min line width (Å):"), 3, 0);
    m_minWidthSpin = new QDoubleSpinBox();
    m_minWidthSpin->setRange(0.1, 1000.0);
    m_minWidthSpin->setValue(1.0);
    m_minWidthSpin->setDecimals(2);
    detLayout->addWidget(m_minWidthSpin, 3, 1);

    detLayout->addWidget(new QLabel("Max line width (Å):"), 4, 0);
    m_maxWidthSpin = new QDoubleSpinBox();
    m_maxWidthSpin->setRange(1.0, 10000.0);
    m_maxWidthSpin->setValue(100.0);
    m_maxWidthSpin->setDecimals(1);
    detLayout->addWidget(m_maxWidthSpin, 4, 1);

    detLayout->addWidget(new QLabel("Max lines:"), 5, 0);
    m_maxLinesSpin = new QSpinBox();
    m_maxLinesSpin->setRange(1, 1000);
    m_maxLinesSpin->setValue(100);
    detLayout->addWidget(m_maxLinesSpin, 5, 1);

    detLayout->addWidget(new QLabel("Continuum order:"), 6, 0);
    m_continuumOrderSpin = new QSpinBox();
    m_continuumOrderSpin->setRange(1, 20);
    m_continuumOrderSpin->setValue(5);
    detLayout->addWidget(m_continuumOrderSpin, 6, 1);

    m_fitContinuumCheck = new QCheckBox("Fit continuum");
    m_fitContinuumCheck->setChecked(true);
    detLayout->addWidget(m_fitContinuumCheck, 7, 0, 1, 2);

    mainLayout->addWidget(m_detectionGroup);

    QGroupBox *progressGroup = new QGroupBox("Progress");
    QVBoxLayout *progLayout = new QVBoxLayout(progressGroup);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    progLayout->addWidget(m_progressBar);

    m_numLinesLabel = new QLabel("Ready");
    progLayout->addWidget(m_numLinesLabel);

    mainLayout->addWidget(progressGroup);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_detectBtn = new QPushButton("Detect Lines");
    m_fitBtn = new QPushButton("Fit All Lines");
    m_applyBtn = new QPushButton("Apply");
    m_applyBtn->setDefault(true);
    m_applyBtn->setEnabled(false);
    m_cancelBtn = new QPushButton("Cancel");

    connect(m_detectBtn, &QPushButton::clicked,
            this, &LineDetectionDialog::detectLines);
    connect(m_fitBtn, &QPushButton::clicked,
            this, &LineDetectionDialog::fitSelectedLine);
    connect(m_applyBtn, &QPushButton::clicked,
            this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);

    btnLayout->addWidget(m_detectBtn);
    btnLayout->addWidget(m_fitBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_applyBtn);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}
