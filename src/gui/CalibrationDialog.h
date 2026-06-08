#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include <QVector>
#include "data_parser/SpectrumData.h"
#include "spectrum_processing/WavelengthCalibrator.h"

class QTableWidget;
class QPushButton;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QGroupBox;

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget *parent = nullptr);

    void setSpectrum(const SpectrumData &spectrum);
    SpectrumData calibratedSpectrum() const;

    void setCalibrator(WavelengthCalibrator *calibrator);

private slots:
    void addCalibrationPoint();
    void removeSelectedPoint();
    void clearAllPoints();
    void loadKnownLines();
    void applyCalibration();
    void methodChanged(int index);
    void updateCalibration();

private:
    void setupUi();
    void updateTable();
    void updateCalibrationInfo();

    QTableWidget *m_pointsTable;
    QPushButton *m_addPointBtn;
    QPushButton *m_removePointBtn;
    QPushButton *m_clearBtn;
    QPushButton *m_loadLinesBtn;
    QPushButton *m_applyBtn;
    QPushButton *m_cancelBtn;

    QComboBox *m_methodCombo;
    QDoubleSpinBox *m_polyOrderSpin;

    QLabel *m_errorLabel;
    QLabel *m_numPointsLabel;
    QComboBox *m_knownLinesCombo;

    SpectrumData m_spectrum;
    SpectrumData m_calibratedSpectrum;
    WavelengthCalibrator *m_calibrator;
    bool m_ownsCalibrator;
};

#endif // CALIBRATIONDIALOG_H
