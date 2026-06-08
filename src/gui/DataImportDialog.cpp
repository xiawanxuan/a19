#include "DataImportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QGroupBox>
#include <QHeaderView>

DataImportDialog::DataImportDialog(QWidget *parent)
    : QDialog(parent)
    , m_parser(new DataParser(this))
{
    setWindowTitle("Import Spectra");
    setMinimumSize(600, 500);

    connect(m_parser, &DataParser::progress,
            this, &DataImportDialog::onImportProgress);
    connect(m_parser, &DataParser::fileParsed,
            this, &DataImportDialog::onFileParsed);

    setupUi();
}

QVector<SpectrumData> DataImportDialog::importedSpectra() const
{
    return m_importedSpectra;
}

void DataImportDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *filesGroup = new QGroupBox("Files to Import");
    QVBoxLayout *filesLayout = new QVBoxLayout(filesGroup);

    m_fileList = new QListWidget();
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    filesLayout->addWidget(m_fileList);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_addFilesBtn = new QPushButton("Add Files...");
    m_addDirBtn = new QPushButton("Add Directory...");
    m_removeBtn = new QPushButton("Remove Selected");
    m_clearBtn = new QPushButton("Clear All");

    connect(m_addFilesBtn, &QPushButton::clicked,
            this, &DataImportDialog::addFiles);
    connect(m_addDirBtn, &QPushButton::clicked,
            this, &DataImportDialog::addDirectory);
    connect(m_removeBtn, &QPushButton::clicked,
            this, &DataImportDialog::removeSelected);
    connect(m_clearBtn, &QPushButton::clicked,
            this, &DataImportDialog::clearAll);

    btnLayout->addWidget(m_addFilesBtn);
    btnLayout->addWidget(m_addDirBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addWidget(m_clearBtn);
    btnLayout->addStretch();

    filesLayout->addLayout(btnLayout);

    mainLayout->addWidget(filesGroup);

    QGroupBox *optionsGroup = new QGroupBox("Options");
    QGridLayout *optionsLayout = new QGridLayout(optionsGroup);

    m_recursiveCheck = new QCheckBox("Recursive directory scanning");
    m_recursiveCheck->setChecked(false);

    QLabel *formatLabel = new QLabel("File format:");
    m_formatCombo = new QComboBox();
    m_formatCombo->addItem("Auto-detect", DataParser::Format_Unknown);
    m_formatCombo->addItem("FITS", DataParser::Format_FITS);
    m_formatCombo->addItem("CSV", DataParser::Format_CSV);
    m_formatCombo->addItem("TSV", DataParser::Format_TSV);
    m_formatCombo->addItem("Text", DataParser::Format_TXT);
    m_formatCombo->addItem("JSON", DataParser::Format_JSON);

    optionsLayout->addWidget(m_recursiveCheck, 0, 0, 1, 2);
    optionsLayout->addWidget(formatLabel, 1, 0);
    optionsLayout->addWidget(m_formatCombo, 1, 1);

    mainLayout->addWidget(optionsGroup);

    QGroupBox *progressGroup = new QGroupBox("Progress");
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_statusLabel = new QLabel("Ready");

    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(progressGroup);

    QHBoxLayout *dialogBtnLayout = new QHBoxLayout();
    m_importBtn = new QPushButton("Import");
    m_importBtn->setDefault(true);
    m_cancelBtn = new QPushButton("Cancel");

    connect(m_importBtn, &QPushButton::clicked,
            this, &DataImportDialog::startImport);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);

    dialogBtnLayout->addStretch();
    dialogBtnLayout->addWidget(m_importBtn);
    dialogBtnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(dialogBtnLayout);
}

void DataImportDialog::addFiles()
{
    QStringList filters = DataParser::supportedFilters();

    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Select Spectrum Files",
        QString(),
        filters.join(";;")
    );

    if (!files.isEmpty()) {
        for (const QString &file : files) {
            if (!m_filePaths.contains(file)) {
                m_filePaths.append(file);
            }
        }
        updateFileList();
    }
}

void DataImportDialog::addDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Directory",
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        QDir directory(dir);
        QStringList filters;
        filters << "*.fits" << "*.fit" << "*.fts"
                << "*.csv" << "*.tsv"
                << "*.txt" << "*.dat" << "*.spec"
                << "*.json";

        QDir::Filters filterFlags = QDir::Files | QDir::NoDotAndDotDot;
        if (m_recursiveCheck->isChecked()) {
            filterFlags |= QDir::AllDirs;
        }

        QStringList files = directory.entryList(filters, filterFlags);
        for (const QString &file : files) {
            QString fullPath = directory.absoluteFilePath(file);
            if (!m_filePaths.contains(fullPath)) {
                m_filePaths.append(fullPath);
            }
        }

        if (m_recursiveCheck->isChecked()) {
            QStringList subdirs = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &subdir : subdirs) {
                QDir subDir(directory.absoluteFilePath(subdir));
                QStringList subFiles = subDir.entryList(filters, QDir::Files);
                for (const QString &file : subFiles) {
                    QString fullPath = subDir.absoluteFilePath(file);
                    if (!m_filePaths.contains(fullPath)) {
                        m_filePaths.append(fullPath);
                    }
                }
            }
        }

        updateFileList();
    }
}

void DataImportDialog::removeSelected()
{
    QList<QListWidgetItem *> selected = m_fileList->selectedItems();
    for (QListWidgetItem *item : selected) {
        QString filePath = item->data(Qt::UserRole).toString();
        m_filePaths.removeAll(filePath);
    }
    updateFileList();
}

void DataImportDialog::clearAll()
{
    m_filePaths.clear();
    updateFileList();
}

void DataImportDialog::startImport()
{
    if (!validateInput()) {
        return;
    }

    m_importBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Importing...");

    QApplication::processEvents();

    m_importedSpectra = m_parser->parseFiles(m_filePaths);

    m_statusLabel->setText(QString("Imported %1 of %2 files")
                               .arg(m_importedSpectra.size())
                               .arg(m_filePaths.size()));

    if (m_importedSpectra.isEmpty()) {
        QMessageBox::warning(this, "Import Failed",
                             "No valid spectrum files were imported.");
        m_importBtn->setEnabled(true);
        m_cancelBtn->setEnabled(true);
        return;
    }

    accept();
}

void DataImportDialog::onImportProgress(int current, int total)
{
    if (total > 0) {
        m_progressBar->setValue((current * 100) / total);
    }
    QApplication::processEvents();
}

void DataImportDialog::onFileParsed(const QString &filePath, bool success)
{
    QFileInfo info(filePath);
    QString status = success ? "OK" : "Failed";
    m_statusLabel->setText(QString("[%1] %2").arg(status, info.fileName()));
    QApplication::processEvents();
}

void DataImportDialog::updateFileList()
{
    m_fileList->clear();

    for (const QString &path : m_filePaths) {
        QFileInfo info(path);
        QListWidgetItem *item = new QListWidgetItem(info.fileName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);

        DataParser::FileFormat fmt = DataParser::detectFormat(path);
        item->setData(Qt::UserRole + 1, DataParser::formatName(fmt));

        m_fileList->addItem(item);
    }

    m_importBtn->setEnabled(!m_filePaths.isEmpty());
    m_statusLabel->setText(QString("%1 files selected").arg(m_filePaths.size()));
}

bool DataImportDialog::validateInput()
{
    if (m_filePaths.isEmpty()) {
        QMessageBox::warning(this, "No Files",
                             "Please add at least one file to import.");
        return false;
    }
    return true;
}
