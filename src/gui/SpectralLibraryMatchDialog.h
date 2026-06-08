#ifndef SPECTRALLIBRARYMATCHDIALOG_H
#define SPECTRALLIBRARYMATCHDIALOG_H

#include <QDialog>
#include "data_parser/SpectrumData.h"
#include "analysis/SpectralLibraryMatcher.h"

class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QSpinBox;
class QGroupBox;
class QLabel;
class QProgressBar;
class QTableWidget;
class QListWidget;
class QCheckBox;

class SpectralLibraryMatchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpectralLibraryMatchDialog(QWidget *parent = nullptr);

    void setSpectrum(const SpectrumData &spectrum);
    QList<SpectralLibraryMatcher::MatchResult> results() const;
    SpectralLibraryMatcher::MatchResult bestMatch() const;

private slots:
    void startMatching();
    void onMatchingProgress(int current, int total);
    void onMatchingComplete();
    void updateResultDisplay();

private:
    void setupUi();
    void populateObjectTypes();

    QGroupBox *m_methodGroup;
    QComboBox *m_matchMethodCombo;
    QDoubleSpinBox *m_redshiftMinSpin;
    QDoubleSpinBox *m_redshiftMaxSpin;
    QSpinBox *m_redshiftStepsSpin;
    QComboBox *m_templateLibraryCombo;
    QSpinBox *m_topNSpin;

    QPushButton *m_matchBtn;
    QPushButton *m_applyBtn;
    QPushButton *m_closeBtn;

    QProgressBar *m_progressBar;
    QTableWidget *m_resultTable;
    QLabel *m_bestMatchLabel;
    QLabel *m_bestTypeLabel;
    QLabel *m_bestScoreLabel;

    SpectrumData m_spectrum;
    SpectralLibraryMatcher *m_matcher;
    QList<SpectralLibraryMatcher::MatchResult> m_results;
};

#endif // SPECTRALLIBRARYMATCHDIALOG_H
