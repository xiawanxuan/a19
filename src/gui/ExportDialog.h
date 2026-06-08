#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QString>
#include "data_parser/SpectrumData.h"
#include "data_parser/DataParser.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QGroupBox;
class QLabel;

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(QWidget *parent = nullptr);

    void setSpectrum(const SpectrumData &spectrum);
    void setSpectra(const QVector<SpectrumData> &spectra);

    QString outputFile() const;
    DataParser::FileFormat format() const;
    bool exportAll() const;

private slots:
    void browseOutput();
    void exportData();
    void updatePreview();

private:
    void setupUi();
    bool validateInput();

    QComboBox *m_formatCombo;
    QLineEdit *m_outputEdit;
    QPushButton *m_browseBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_cancelBtn;

    QCheckBox *m_exportAllCheck;
    QCheckBox *m_includeHeaderCheck;
    QCheckBox *m_includeLinesCheck;
    QCheckBox *m_includeContinuumCheck;
    QCheckBox *m_includeErrorCheck;

    QLabel *m_previewLabel;
    QLabel *m_sizeEstimateLabel;

    QVector<SpectrumData> m_spectra;
    SpectrumData m_currentSpectrum;
};

#endif // EXPORTDIALOG_H
