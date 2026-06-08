#include "SpectralLibraryMatchDialog.h"
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

SpectralLibraryMatchDialog::SpectralLibraryMatchDialog(QWidget *parent)
    : QDialog(parent)
    , m_matcher(new SpectralLibraryMatcher(this))
{
    setWindowTitle(tr("Spectral Library Matching"));
    setMinimumSize(700, 500);
    setupUi();

    connect(m_matcher, &SpectralLibraryMatcher::matchingProgress,
            this, &SpectralLibraryMatchDialog::onMatchingProgress);
    connect(m_matcher, &SpectralLibraryMatcher::matchingComplete,
            this, &SpectralLibraryMatchDialog::onMatchingComplete);
}

void SpectralLibraryMatchDialog::setSpectrum(const SpectrumData &spectrum)
{
    m_spectrum = spectrum;
    m_bestMatchLabel->setText(tr("-"));
    m_bestTypeLabel->setText(tr("-"));
    m_bestScoreLabel->setText(tr("-"));
    m_resultTable->setRowCount(0);
    m_results.clear();
}

QList<SpectralLibraryMatcher::MatchResult> SpectralLibraryMatchDialog::results() const
{
    return m_results;
}

SpectralLibraryMatcher::MatchResult SpectralLibraryMatchDialog::bestMatch() const
{
    if (m_results.isEmpty()) {
        return SpectralLibraryMatcher::MatchResult();
    }
    return m_results.first();
}

void SpectralLibraryMatchDialog::startMatching()
{
    if (!m_spectrum.isValid()) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("No valid spectrum loaded."));
        return;
    }

    m_matchBtn->setEnabled(false);
    m_applyBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);

    SpectralLibraryMatcher::MatchMethod method =
        static_cast<SpectralLibraryMatcher::MatchMethod>(
            m_matchMethodCombo->currentData().toInt());

    m_matcher->setMatchMethod(method);
    m_matcher->setRedshiftRange(m_redshiftMinSpin->value(),
                                m_redshiftMaxSpin->value());
    m_matcher->setSearchStep(
        (m_redshiftMaxSpin->value() - m_redshiftMinSpin->value())
        / m_redshiftStepsSpin->value());
    m_matcher->setTopN(m_topNSpin->value());

    m_results = m_matcher->matchSpectrum(m_spectrum);
    updateResultDisplay();

    m_matchBtn->setEnabled(true);
    m_applyBtn->setEnabled(!m_results.isEmpty());
    m_progressBar->setVisible(false);
}

void SpectralLibraryMatchDialog::onMatchingProgress(int current, int total)
{
    if (total > 0) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(current);
    }
}

void SpectralLibraryMatchDialog::onMatchingComplete()
{
    m_progressBar->setVisible(false);
}

void SpectralLibraryMatchDialog::updateResultDisplay()
{
    m_resultTable->setRowCount(m_results.size());

    for (int i = 0; i < m_results.size(); ++i) {
        const SpectralLibraryMatcher::MatchResult &r = m_results[i];

        m_resultTable->setItem(i, 0, new QTableWidgetItem(QString::number(r.rank)));
        m_resultTable->setItem(i, 1, new QTableWidgetItem(r.templateName));
        m_resultTable->setItem(i, 2, new QTableWidgetItem(
            SpectralLibraryMatcher::objectTypeName(r.objectType)));
        m_resultTable->setItem(i, 3, new QTableWidgetItem(
            QString::number(r.matchScore, 'f', 4)));
        m_resultTable->setItem(i, 4, new QTableWidgetItem(
            QString::number(r.correlationCoefficient, 'f', 4)));
        m_resultTable->setItem(i, 5, new QTableWidgetItem(
            QString("z = %1").arg(r.redshift, 0, 'f', 5)));

        for (int c = 0; c < m_resultTable->columnCount(); ++c) {
            QTableWidgetItem *item = m_resultTable->item(i, c);
            if (item) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }
    }

    if (!m_results.isEmpty()) {
        const SpectralLibraryMatcher::MatchResult &best = m_results.first();
        m_bestMatchLabel->setText(best.templateName);
        m_bestTypeLabel->setText(
            SpectralLibraryMatcher::objectTypeName(best.objectType));
        m_bestScoreLabel->setText(QString::number(best.matchScore, 'f', 4));
    }
}

void SpectralLibraryMatchDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_methodGroup = new QGroupBox(tr("Matching Parameters"), this);
    QFormLayout *formLayout = new QFormLayout(m_methodGroup);

    m_matchMethodCombo = new QComboBox(m_methodGroup);
    m_matchMethodCombo->addItem(tr("Cross Correlation"),
        SpectralLibraryMatcher::CrossCorrelation);
    m_matchMethodCombo->addItem(tr("Chi-Squared"),
        SpectralLibraryMatcher::ChiSquared);
    m_matchMethodCombo->addItem(tr("Correlation Coefficient"),
        SpectralLibraryMatcher::CorrelationCoefficient);
    m_matchMethodCombo->addItem(tr("Euclidean Distance"),
        SpectralLibraryMatcher::EuclideanDistance);
    formLayout->addRow(tr("Match Method:"), m_matchMethodCombo);

    QHBoxLayout *redshiftLayout = new QHBoxLayout();
    m_redshiftMinSpin = new QDoubleSpinBox(m_methodGroup);
    m_redshiftMinSpin->setRange(-0.1, 10.0);
    m_redshiftMinSpin->setDecimals(5);
    m_redshiftMinSpin->setSingleStep(0.01);
    m_redshiftMinSpin->setValue(0.0);
    redshiftLayout->addWidget(m_redshiftMinSpin);

    redshiftLayout->addWidget(new QLabel(" ~ "));

    m_redshiftMaxSpin = new QDoubleSpinBox(m_methodGroup);
    m_redshiftMaxSpin->setRange(0.0, 20.0);
    m_redshiftMaxSpin->setDecimals(5);
    m_redshiftMaxSpin->setSingleStep(0.01);
    m_redshiftMaxSpin->setValue(1.0);
    redshiftLayout->addWidget(m_redshiftMaxSpin);

    formLayout->addRow(tr("Redshift Range:"), redshiftLayout);

    m_redshiftStepsSpin = new QSpinBox(m_methodGroup);
    m_redshiftStepsSpin->setRange(10, 1000);
    m_redshiftStepsSpin->setValue(200);
    formLayout->addRow(tr("Redshift Steps:"), m_redshiftStepsSpin);

    m_topNSpin = new QSpinBox(m_methodGroup);
    m_topNSpin->setRange(1, 50);
    m_topNSpin->setValue(10);
    formLayout->addRow(tr("Top N Results:"), m_topNSpin);

    m_templateLibraryCombo = new QComboBox(m_methodGroup);
    m_templateLibraryCombo->addItem(tr("Built-in Templates"), 0);
    formLayout->addRow(tr("Template Library:"), m_templateLibraryCombo);

    mainLayout->addWidget(m_methodGroup);

    QGroupBox *bestMatchGroup = new QGroupBox(tr("Best Match"), this);
    QFormLayout *bestForm = new QFormLayout(bestMatchGroup);
    m_bestMatchLabel = new QLabel(tr("-"), bestMatchGroup);
    m_bestMatchLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2980b9;");
    bestForm->addRow(tr("Template:"), m_bestMatchLabel);

    m_bestTypeLabel = new QLabel(tr("-"), bestMatchGroup);
    m_bestTypeLabel->setStyleSheet("font-weight: bold; color: #27ae60;");
    bestForm->addRow(tr("Object Type:"), m_bestTypeLabel);

    m_bestScoreLabel = new QLabel(tr("-"), bestMatchGroup);
    bestForm->addRow(tr("Match Score:"), m_bestScoreLabel);

    mainLayout->addWidget(bestMatchGroup);

    QGroupBox *resultGroup = new QGroupBox(tr("All Results"), this);
    QVBoxLayout *resultLayout = new QVBoxLayout(resultGroup);

    m_resultTable = new QTableWidget(resultGroup);
    m_resultTable->setColumnCount(6);
    QStringList headers;
    headers << tr("Rank") << tr("Template") << tr("Object Type")
            << tr("Score") << tr("Correlation") << tr("Redshift");
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultLayout->addWidget(m_resultTable);

    mainLayout->addWidget(resultGroup, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_matchBtn = new QPushButton(tr("Start Matching"), this);
    m_matchBtn->setDefault(true);
    connect(m_matchBtn, &QPushButton::clicked,
            this, &SpectralLibraryMatchDialog::startMatching);
    buttonLayout->addWidget(m_matchBtn);

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

void SpectralLibraryMatchDialog::populateObjectTypes()
{
}
