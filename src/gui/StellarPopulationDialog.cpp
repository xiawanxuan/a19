#include "StellarPopulationDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QCheckBox>

StellarPopulationDialog::StellarPopulationDialog(QWidget *parent)
    : QDialog(parent)
    , m_fitter(new StellarPopulationFitter(this))
{
    setWindowTitle(tr("Stellar Population Fitting"));
    setMinimumSize(750, 600);
    setupUi();

    connect(m_fitter, &StellarPopulationFitter::fittingProgress,
            this, &StellarPopulationDialog::onFittingProgress);
    connect(m_fitter, &StellarPopulationFitter::fittingComplete,
            this, &StellarPopulationDialog::onFittingComplete);
}

void StellarPopulationDialog::setSpectrum(const SpectrumData &spectrum)
{
    m_spectrum = spectrum;
    m_meanAgeLabel->setText(tr("-"));
    m_massAgeLabel->setText(tr("-"));
    m_metallicityLabel->setText(tr("-"));
    m_totalMassLabel->setText(tr("-"));
    m_chiSquaredLabel->setText(tr("-"));
    m_componentTable->setRowCount(0);
}

StellarPopulationFitter::PopulationResult StellarPopulationDialog::result() const
{
    return m_result;
}

void StellarPopulationDialog::startFitting()
{
    if (!m_spectrum.isValid()) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("No valid spectrum loaded."));
        return;
    }

    m_fitBtn->setEnabled(false);
    m_applyBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);

    StellarPopulationFitter::IMF imf =
        static_cast<StellarPopulationFitter::IMF>(
            m_imfCombo->currentData().toInt());
    m_fitter->setIMF(imf);

    StellarPopulationFitter::FitMethod method =
        static_cast<StellarPopulationFitter::FitMethod>(
            m_fitMethodCombo->currentData().toInt());
    m_fitter->setFitMethod(method);

    m_fitter->setAgeRange(m_minAgeSpin->value(), m_maxAgeSpin->value());
    m_fitter->setMetallicityRange(m_minZSpin->value(), m_maxZSpin->value());
    m_fitter->setNAgeBins(m_nAgeBinsSpin->value());
    m_fitter->setNMetallicityBins(m_nZbinsSpin->value());
    m_fitter->setDustLaw(m_dustLawCheck->isChecked());

    m_fitter->generateSSPLibrary();
    m_result = m_fitter->fitPopulation(m_spectrum);
    updateResultDisplay();

    m_fitBtn->setEnabled(true);
    m_applyBtn->setEnabled(m_result.totalMass > 0);
    m_progressBar->setVisible(false);
}

void StellarPopulationDialog::onFittingProgress(int current, int total)
{
    if (total > 0) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(current);
    }
}

void StellarPopulationDialog::onFittingComplete()
{
    m_progressBar->setVisible(false);
}

void StellarPopulationDialog::updateResultDisplay()
{
    if (m_result.totalMass <= 0) {
        return;
    }

    m_meanAgeLabel->setText(
        StellarPopulationFitter::ageToString(m_result.meanAge));
    m_massAgeLabel->setText(
        StellarPopulationFitter::ageToString(m_result.massWeightedAge));
    m_metallicityLabel->setText(
        QString("[Fe/H] = %1").arg(m_result.meanMetallicity, 0, 'f', 2));
    m_totalMassLabel->setText(
        QString("%1 M☉").arg(m_result.totalMass, 0, 'e', 2));
    m_chiSquaredLabel->setText(
        QString::number(m_result.reducedChiSquared, 'g', 4));

    QList<StellarPopulationFitter::SSPModel> ssps = m_fitter->sspLibrary();
    m_componentTable->setRowCount(m_result.componentMasses.size());

    int row = 0;
    QMapIterator<QString, double> it(m_result.componentMasses);
    while (it.hasNext()) {
        it.next();
        m_componentTable->setItem(row, 0,
            new QTableWidgetItem(it.key()));
        m_componentTable->setItem(row, 1,
            new QTableWidgetItem(QString::number(it.value() / m_result.totalMass * 100.0, 'f', 2) + " %"));
        m_componentTable->setItem(row, 2,
            new QTableWidgetItem(QString::number(it.value(), 'e', 3) + " M☉"));

        for (int c = 0; c < m_componentTable->columnCount(); ++c) {
            QTableWidgetItem *item = m_componentTable->item(row, c);
            if (item) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }
        row++;
    }

    m_componentTable->sortItems(1, Qt::DescendingOrder);
}

void StellarPopulationDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_paramsGroup = new QGroupBox(tr("Fitting Parameters"), this);
    QFormLayout *formLayout = new QFormLayout(m_paramsGroup);

    m_imfCombo = new QComboBox(m_paramsGroup);
    m_imfCombo->addItem(tr("Salpeter (1955)"), StellarPopulationFitter::Salpeter);
    m_imfCombo->addItem(tr("Kroupa (2001)"), StellarPopulationFitter::Kroupa);
    m_imfCombo->addItem(tr("Chabrier (2003)"), StellarPopulationFitter::Chabrier);
    m_imfCombo->setCurrentIndex(2);
    formLayout->addRow(tr("Initial Mass Function:"), m_imfCombo);

    m_fitMethodCombo = new QComboBox(m_paramsGroup);
    m_fitMethodCombo->addItem(tr("Chi-Squared"),
        StellarPopulationFitter::ChiSquared);
    m_fitMethodCombo->addItem(tr("Maximum Likelihood"),
        StellarPopulationFitter::MaximumLikelihood);
    m_fitMethodCombo->addItem(tr("NNLS"),
        StellarPopulationFitter::NonNegativeLeastSquares);
    formLayout->addRow(tr("Fit Method:"), m_fitMethodCombo);

    QHBoxLayout *ageLayout = new QHBoxLayout();
    m_minAgeSpin = new QDoubleSpinBox(m_paramsGroup);
    m_minAgeSpin->setRange(0.001, 20.0);
    m_minAgeSpin->setDecimals(4);
    m_minAgeSpin->setSingleStep(0.1);
    m_minAgeSpin->setValue(0.01);
    m_minAgeSpin->setSuffix(" Gyr");
    ageLayout->addWidget(m_minAgeSpin);
    ageLayout->addWidget(new QLabel(" ~ "));
    m_maxAgeSpin = new QDoubleSpinBox(m_paramsGroup);
    m_maxAgeSpin->setRange(0.1, 30.0);
    m_maxAgeSpin->setDecimals(2);
    m_maxAgeSpin->setSingleStep(0.5);
    m_maxAgeSpin->setValue(15.0);
    m_maxAgeSpin->setSuffix(" Gyr");
    ageLayout->addWidget(m_maxAgeSpin);
    formLayout->addRow(tr("Age Range:"), ageLayout);

    m_nAgeBinsSpin = new QSpinBox(m_paramsGroup);
    m_nAgeBinsSpin->setRange(3, 30);
    m_nAgeBinsSpin->setValue(10);
    formLayout->addRow(tr("Age Bins:"), m_nAgeBinsSpin);

    QHBoxLayout *zLayout = new QHBoxLayout();
    m_minZSpin = new QDoubleSpinBox(m_paramsGroup);
    m_minZSpin->setRange(-5.0, 2.0);
    m_minZSpin->setDecimals(2);
    m_minZSpin->setSingleStep(0.1);
    m_minZSpin->setValue(-2.5);
    zLayout->addWidget(m_minZSpin);
    zLayout->addWidget(new QLabel(" ~ "));
    m_maxZSpin = new QDoubleSpinBox(m_paramsGroup);
    m_maxZSpin->setRange(-3.0, 2.0);
    m_maxZSpin->setDecimals(2);
    m_maxZSpin->setSingleStep(0.1);
    m_maxZSpin->setValue(0.5);
    zLayout->addWidget(m_maxZSpin);
    formLayout->addRow(tr("Metallicity Range [Fe/H]:"), zLayout);

    m_nZbinsSpin = new QSpinBox(m_paramsGroup);
    m_nZbinsSpin->setRange(2, 10);
    m_nZbinsSpin->setValue(5);
    formLayout->addRow(tr("Metallicity Bins:"), m_nZbinsSpin);

    m_dustLawCheck = new QCheckBox(tr("Include dust extinction law"), m_paramsGroup);
    m_dustLawCheck->setChecked(true);
    formLayout->addRow(tr("Dust Correction:"), m_dustLawCheck);

    mainLayout->addWidget(m_paramsGroup);

    QGroupBox *resultGroup = new QGroupBox(tr("Results"), this);
    QFormLayout *resultForm = new QFormLayout(resultGroup);

    m_meanAgeLabel = new QLabel(tr("-"), resultGroup);
    m_meanAgeLabel->setStyleSheet("font-weight: bold; color: #2980b9;");
    resultForm->addRow(tr("Mean Age:"), m_meanAgeLabel);

    m_massAgeLabel = new QLabel(tr("-"), resultGroup);
    m_massAgeLabel->setStyleSheet("font-weight: bold; color: #2980b9;");
    resultForm->addRow(tr("Mass-Weighted Age:"), m_massAgeLabel);

    m_metallicityLabel = new QLabel(tr("-"), resultGroup);
    m_metallicityLabel->setStyleSheet("font-weight: bold; color: #27ae60;");
    resultForm->addRow(tr("Mean Metallicity:"), m_metallicityLabel);

    m_totalMassLabel = new QLabel(tr("-"), resultGroup);
    m_totalMassLabel->setStyleSheet("font-weight: bold;");
    resultForm->addRow(tr("Total Stellar Mass:"), m_totalMassLabel);

    m_chiSquaredLabel = new QLabel(tr("-"), resultGroup);
    resultForm->addRow(tr("Reduced χ²:"), m_chiSquaredLabel);

    mainLayout->addWidget(resultGroup);

    QGroupBox *compGroup = new QGroupBox(tr("Population Components"), this);
    QVBoxLayout *compLayout = new QVBoxLayout(compGroup);

    m_componentTable = new QTableWidget(compGroup);
    m_componentTable->setColumnCount(3);
    QStringList headers;
    headers << tr("Component") << tr("Mass Fraction") << tr("Mass");
    m_componentTable->setHorizontalHeaderLabels(headers);
    m_componentTable->horizontalHeader()->setStretchLastSection(true);
    m_componentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_componentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    compLayout->addWidget(m_componentTable);

    mainLayout->addWidget(compGroup, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_fitBtn = new QPushButton(tr("Start Fitting"), this);
    m_fitBtn->setDefault(true);
    connect(m_fitBtn, &QPushButton::clicked,
            this, &StellarPopulationDialog::startFitting);
    buttonLayout->addWidget(m_fitBtn);

    m_applyBtn = new QPushButton(tr("Apply"), this);
    m_applyBtn->setEnabled(false);
    connect(m_applyBtn, &QPushButton::clicked,
            this, &QDialog::accept);
    buttonLayout->addWidget(m_applyBtn);

    m_closeBtn = new QPushButton(tr("Close"), this);
    connect(m_closeBtn, &QPushButton::clicked,
            this, &QDialog::reject);
    buttonLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(buttonLayout);
}
