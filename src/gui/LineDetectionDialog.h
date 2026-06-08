#ifndef LINEDETECTIONDIALOG_H
#define LINEDETECTIONDIALOG_H

#include <QDialog>
#include "data_parser/SpectrumData.h"
#include "spectrum_processing/SpectralLineFitter.h"

class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;
class QSpinBox;
class QGroupBox;
class QLabel;
class QProgressBar;

class LineDetectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LineDetectionDialog(QWidget *parent = nullptr);

    void setSpectrum(const SpectrumData &spectrum);
    SpectrumData processedSpectrum() const;

private slots:
    void detectLines();
    void fitSelectedLine();
    void onDetectionProgress(int current, int total);
    void updatePreview();

private:
    void setupUi();

    QGroupBox *m_detectionGroup;
    QComboBox *m_detectionMethodCombo;
    QComboBox *m_lineTypeCombo;
    QDoubleSpinBox *m_noiseThresholdSpin;
    QDoubleSpinBox *m_minWidthSpin;
    QDoubleSpinBox *m_maxWidthSpin;
    QSpinBox *m_maxLinesSpin;
    QSpinBox *m_continuumOrderSpin;
    QCheckBox *m_fitContinuumCheck;

    QPushButton *m_detectBtn;
    QPushButton *m_fitBtn;
    QPushButton *m_applyBtn;
    QPushButton *m_cancelBtn;

    QLabel *m_numLinesLabel;
    QProgressBar *m_progressBar;

    SpectrumData m_spectrum;
    SpectrumData m_processedSpectrum;
    SpectralLineFitter *m_lineFitter;
};

#endif // LINEDETECTIONDIALOG_H
