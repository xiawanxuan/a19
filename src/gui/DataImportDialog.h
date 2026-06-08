#ifndef DATAIMPORTDIALOG_H
#define DATAIMPORTDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QVector>
#include "data_parser/SpectrumData.h"
#include "data_parser/DataParser.h"

class QListWidget;
class QPushButton;
class QProgressBar;
class QLabel;
class QCheckBox;
class QComboBox;

class DataImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataImportDialog(QWidget *parent = nullptr);

    QVector<SpectrumData> importedSpectra() const;

private slots:
    void addFiles();
    void addDirectory();
    void removeSelected();
    void clearAll();
    void startImport();
    void onImportProgress(int current, int total);
    void onFileParsed(const QString &filePath, bool success);

private:
    void setupUi();
    void updateFileList();
    bool validateInput();

    QListWidget *m_fileList;
    QPushButton *m_addFilesBtn;
    QPushButton *m_addDirBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_clearBtn;
    QPushButton *m_importBtn;
    QPushButton *m_cancelBtn;

    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;

    QCheckBox *m_recursiveCheck;
    QComboBox *m_formatCombo;

    QStringList m_filePaths;
    QVector<SpectrumData> m_importedSpectra;
    DataParser *m_parser;
};

#endif // DATAIMPORTDIALOG_H
