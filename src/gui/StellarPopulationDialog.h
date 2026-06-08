#ifndef STELLARPOPULATIONDIALOG_H
#define STELLARPOPULATIONDIALOG_H

#include <QDialog>
#include "data_parser/SpectrumData.h"
#include "analysis/StellarPopulationFitter.h"

class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QSpinBox;
class QGroupBox;
class QLabel;
class QProgressBar;
class QTableWidget;
class QCheckBox;

class StellarPopulationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StellarPopulationDialog(QWidget *parent = nullptr);

    void setSpectrum(const SpectrumData &spectrum);
    StellarPopulationFitter::PopulationResult result() const;

private slots:
    void startFitting();
    void onFittingProgress(int current, int total);
    void onFittingComplete();
    void updateResultDisplay();

private:
    void setupUi();

    QGroupBox *m_paramsGroup;
    QComboBox *m_imfCombo;
    QComboBox *m_fitMethodCombo;
    QDoubleSpinBox *m_minAgeSpin;
    QDoubleSpinBox *m_maxAgeSpin;
    QSpinBox *m_nAgeBinsSpin;
    QDoubleSpinBox *m_minZSpin;
    QDoubleSpinBox *m_maxZSpin;
    QSpinBox *m_nZbinsSpin;
    QCheckBox *m_dustLawCheck;

    QPushButton *m_fitBtn;
    QPushButton *m_applyBtn;
    QPushButton *m_closeBtn;

    QProgressBar *m_progressBar;
    QLabel *m_meanAgeLabel;
    QLabel *m_massAgeLabel;
    QLabel *m_metallicityLabel;
    QLabel *m_totalMassLabel;
    QLabel *m_chiSquaredLabel;
    QTableWidget *m_componentTable;

    SpectrumData m_spectrum;
    StellarPopulationFitter *m_fitter;
    StellarPopulationFitter::PopulationResult m_result;
};

#endif // STELLARPOPULATIONDIALOG_H
