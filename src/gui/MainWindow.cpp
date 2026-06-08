#include "MainWindow.h"
#include "gui/DataImportDialog.h"
#include "gui/CalibrationDialog.h"
#include "gui/LineDetectionDialog.h"
#include "gui/ExportDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QListWidget>
#include <QListView>
#include <QTableView>
#include <QDockWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QTabWidget>
#include <QMessageBox>
#include <QHeaderView>
#include <QApplication>
#include <QIcon>
#include <QInputDialog>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_dataParser(new DataParser(this))
    , m_processor(new SpectrumProcessor(this))
    , m_spectrumPlot(nullptr)
    , m_spectrumModel(new SpectrumListModel(this))
    , m_lineModel(new SpectralLineTableModel(this))
    , m_spectrumListView(nullptr)
    , m_lineTableView(nullptr)
    , m_spectrumDock(nullptr)
    , m_linesDock(nullptr)
    , m_infoDock(nullptr)
    , m_currentIndex(-1)
{
    setWindowTitle("Astro Spectrum Analyzer");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    setupUi();
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWidgets();

    connect(m_processor, &SpectrumProcessor::processingProgress,
            this, &MainWindow::onProcessingProgress);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    m_spectrumPlot = new SpectrumPlot(this);
    m_spectrumPlot->setTitle("Spectrum");

    setCentralWidget(m_spectrumPlot);

    connect(m_spectrumPlot, &SpectrumPlot::cursorPositionChanged,
            this, &MainWindow::onCursorPositionChanged);
}

void MainWindow::createActions()
{
    m_importAct = new QAction(tr("&Import Files..."), this);
    m_importAct->setShortcut(QKeySequence::Open);
    m_importAct->setStatusTip(tr("Import spectrum files"));
    connect(m_importAct, &QAction::triggered,
            this, &MainWindow::onImportFiles);

    m_importDirAct = new QAction(tr("Import &Directory..."), this);
    m_importDirAct->setStatusTip(tr("Import spectra from directory"));
    connect(m_importDirAct, &QAction::triggered,
            this, &MainWindow::onImportDirectory);

    m_exportAct = new QAction(tr("&Export..."), this);
    m_exportAct->setShortcut(QKeySequence::Save);
    m_exportAct->setStatusTip(tr("Export current spectrum"));
    m_exportAct->setEnabled(false);
    connect(m_exportAct, &QAction::triggered,
            this, &MainWindow::onExport);

    m_exitAct = new QAction(tr("E&xit"), this);
    m_exitAct->setShortcut(QKeySequence::Quit);
    m_exitAct->setStatusTip(tr("Exit the application"));
    connect(m_exitAct, &QAction::triggered,
            this, &MainWindow::onExit);

    m_calibrateAct = new QAction(tr("&Wavelength Calibration..."), this);
    m_calibrateAct->setStatusTip(tr("Calibrate wavelength axis"));
    m_calibrateAct->setEnabled(false);
    connect(m_calibrateAct, &QAction::triggered,
            this, &MainWindow::onWavelengthCalibration);

    m_detectLinesAct = new QAction(tr("&Detect & Lines..."), this);
    m_detectLinesAct->setStatusTip(tr("Detect and fit spectral lines"));
    m_detectLinesAct->setEnabled(false);
    connect(m_detectLinesAct, &QAction::triggered,
            this, &MainWindow::onDetectLines);

    m_redshiftAct = new QAction(tr("&Redshift..."), this);
    m_redshiftAct->setStatusTip(tr("Calculate redshift"));
    m_redshiftAct->setEnabled(false);
    connect(m_redshiftAct, &QAction::triggered,
            this, &MainWindow::onCalculateRedshift);

    m_smoothAct = new QAction(tr("&Smooth..."), this);
    m_smoothAct->setStatusTip(tr("Smooth the spectrum"));
    m_smoothAct->setEnabled(false);
    connect(m_smoothAct, &QAction::triggered,
            this, &MainWindow::onSmoothSpectrum);

    m_normalizeAct = new QAction(tr("&Normalize"), this);
    m_normalizeAct->setStatusTip(tr("Normalize the spectrum"));
    m_normalizeAct->setEnabled(false);
    connect(m_normalizeAct, &QAction::triggered,
            this, &MainWindow::onNormalizeSpectrum);

    m_subtractContinuumAct = new QAction(tr("Subtract &Continuum"), this);
    m_subtractContinuumAct->setStatusTip(tr("Subtract continuum from spectrum"));
    m_subtractContinuumAct->setEnabled(false);
    connect(m_subtractContinuumAct, &QAction::triggered,
            this, &MainWindow::onSubtractContinuum);

    m_zoomInAct = new QAction(tr("Zoom &In"), this);
    m_zoomInAct->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAct->setStatusTip(tr("Zoom in"));
    connect(m_zoomInAct, &QAction::triggered,
            this, &MainWindow::onZoomIn);

    m_zoomOutAct = new QAction(tr("Zoom &Out"), this);
    m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAct->setStatusTip(tr("Zoom out"));
    connect(m_zoomOutAct, &QAction::triggered,
            this, &MainWindow::onZoomOut);

    m_resetZoomAct = new QAction(tr("&Reset Zoom"), this);
    m_resetZoomAct->setShortcut(tr("R"));
    m_resetZoomAct->setStatusTip(tr("Reset zoom to full view"));
    connect(m_resetZoomAct, &QAction::triggered,
            this, &MainWindow::onResetZoom);

    m_showGridAct = new QAction(tr("Show &Grid"), this);
    m_showGridAct->setCheckable(true);
    m_showGridAct->setChecked(true);
    m_showGridAct->setStatusTip(tr("Show/hide grid"));
    connect(m_showGridAct, &QAction::toggled,
            this, &MainWindow::onShowGrid);

    m_showContinuumAct = new QAction(tr("Show &Continuum"), this);
    m_showContinuumAct->setCheckable(true);
    m_showContinuumAct->setChecked(false);
    m_showContinuumAct->setStatusTip(tr("Show/hide continuum"));
    connect(m_showContinuumAct, &QAction::toggled,
            this, &MainWindow::onShowContinuum);

    m_showLinesAct = new QAction(tr("Show &Lines"), this);
    m_showLinesAct->setCheckable(true);
    m_showLinesAct->setChecked(true);
    m_showLinesAct->setStatusTip(tr("Show/hide spectral lines"));
    connect(m_showLinesAct, &QAction::toggled,
            this, &MainWindow::onShowLines);

    m_aboutAct = new QAction(tr("&About"), this);
    m_aboutAct->setStatusTip(tr("About this application"));
    connect(m_aboutAct, &QAction::triggered,
            this, &MainWindow::onAbout);

    m_docsAct = new QAction(tr("&Documentation"), this);
    m_docsAct->setStatusTip(tr("View documentation"));
    connect(m_docsAct, &QAction::triggered,
            this, &MainWindow::onDocumentation);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_importAct);
    fileMenu->addAction(m_importDirAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exportAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAct);

    QMenu *processingMenu = menuBar()->addMenu(tr("&Processing"));
    processingMenu->addAction(m_calibrateAct);
    processingMenu->addAction(m_detectLinesAct);
    processingMenu->addAction(m_redshiftAct);
    processingMenu->addSeparator();
    processingMenu->addAction(m_smoothAct);
    processingMenu->addAction(m_normalizeAct);
    processingMenu->addAction(m_subtractContinuumAct);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_zoomInAct);
    viewMenu->addAction(m_zoomOutAct);
    viewMenu->addAction(m_resetZoomAct);
    viewMenu->addSeparator();
    viewMenu->addAction(m_showGridAct);
    viewMenu->addAction(m_showContinuumAct);
    viewMenu->addAction(m_showLinesAct);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_docsAct);
    helpMenu->addSeparator();
    helpMenu->addAction(m_aboutAct);
}

void MainWindow::createToolBars()
{
    m_fileToolBar = addToolBar(tr("File"));
    m_fileToolBar->addAction(m_importAct);
    m_fileToolBar->addAction(m_exportAct);
    m_fileToolBar->setMovable(false);

    m_processingToolBar = addToolBar(tr("Processing"));
    m_processingToolBar->addAction(m_calibrateAct);
    m_processingToolBar->addAction(m_detectLinesAct);
    m_processingToolBar->addAction(m_redshiftAct);
    m_processingToolBar->addSeparator();
    m_processingToolBar->addAction(m_smoothAct);
    m_processingToolBar->addAction(m_normalizeAct);
    m_processingToolBar->setMovable(false);

    m_viewToolBar = addToolBar(tr("View"));
    m_viewToolBar->addAction(m_zoomInAct);
    m_viewToolBar->addAction(m_zoomOutAct);
    m_viewToolBar->addAction(m_resetZoomAct);
    m_viewToolBar->addSeparator();
    m_viewToolBar->addAction(m_showGridAct);
    m_viewToolBar->addAction(m_showContinuumAct);
    m_viewToolBar->addAction(m_showLinesAct);
    m_viewToolBar->setMovable(false);
}

void MainWindow::createStatusBar()
{
    m_statusBar = statusBar();

    m_statusLabel = new QLabel(tr("Ready"));
    m_cursorLabel = new QLabel(tr(""));
    m_cursorLabel->setMinimumWidth(200);

    m_statusBar->addWidget(m_statusLabel, 1);
    m_statusBar->addPermanentWidget(m_cursorLabel);
}

void MainWindow::createDockWidgets()
{
    m_spectrumDock = new QDockWidget(tr("Spectra"), this);
    m_spectrumDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_spectrumDock->setFeatures(QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable);

    m_spectrumListView = new QListView();
    m_spectrumListView->setModel(m_spectrumModel);
    m_spectrumListView->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_spectrumListView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this, &MainWindow::onSpectrumSelected);

    m_spectrumDock->setWidget(m_spectrumListView);
    addDockWidget(Qt::LeftDockWidgetArea, m_spectrumDock);

    m_linesDock = new QDockWidget(tr("Spectral Lines"), this);
    m_linesDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_linesDock->setFeatures(QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable);

    m_lineTableView = new QTableView();
    m_lineTableView->setModel(m_lineModel);
    m_lineTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_lineTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_lineTableView->horizontalHeader()->setStretchLastSection(true);
    m_lineTableView->setSortingEnabled(true);
    m_lineTableView->resizeColumnsToContents();
    connect(m_lineTableView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this, &MainWindow::onLineSelected);

    m_linesDock->setWidget(m_lineTableView);
    addDockWidget(Qt::BottomDockWidgetArea, m_linesDock);

    m_infoDock = new QDockWidget(tr("Info"), this);
    m_infoDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    QWidget *infoWidget = new QWidget();
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);

    QGroupBox *infoGroup = new QGroupBox(tr("Spectrum Info"));
    QFormLayout *formLayout = new QFormLayout(infoGroup);

    QLabel *nameLabel = new QLabel(tr("-"));
    nameLabel->setObjectName("infoName");
    formLayout->addRow(tr("Name:"), nameLabel);

    QLabel *objectLabel = new QLabel(tr("-"));
    objectLabel->setObjectName("infoObject");
    formLayout->addRow(tr("Object:"), objectLabel);

    QLabel *pointsLabel = new QLabel(tr("-"));
    pointsLabel->setObjectName("infoPoints");
    formLayout->addRow(tr("Data points:"), pointsLabel);

    QLabel *wlRangeLabel = new QLabel(tr("-"));
    wlRangeLabel->setObjectName("infoWlRange");
    formLayout->addRow(tr("λ range:"), wlRangeLabel);

    QLabel *fluxRangeLabel = new QLabel(tr("-"));
    fluxRangeLabel->setObjectName("infoFluxRange");
    formLayout->addRow(tr("Flux range:"), fluxRangeLabel);

    QLabel *zLabel = new QLabel(tr("-"));
    zLabel->setObjectName("infoRedshift");
    formLayout->addRow(tr("Redshift:"), zLabel);

    QLabel *calibratedLabel = new QLabel(tr("-"));
    calibratedLabel->setObjectName("infoCalibrated");
    formLayout->addRow(tr("Calibrated:"), calibratedLabel);

    infoLayout->addWidget(infoGroup);
    infoLayout->addStretch();

    m_infoDock->setWidget(infoWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_infoDock);
}

void MainWindow::updateCurrentSpectrum(const SpectrumData &spectrum)
{
    if (m_currentIndex >= 0 && m_currentIndex < m_spectra.size()) {
        m_spectra[m_currentIndex] = spectrum;
        m_spectrumModel->updateSpectrum(m_currentIndex, spectrum);
    }
    m_spectrumPlot->setSpectrum(spectrum);
    m_lineModel->setSpectralLines(spectrum.spectralLines());
    updateStatusInfo();
}

void MainWindow::refreshSpectrumList()
{
    m_spectrumModel->setSpectra(m_spectra);
}

void MainWindow::refreshLineTable()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_spectra.size()) {
        m_lineModel->setSpectralLines(m_spectra[m_currentIndex].spectralLines());
    }
}

void MainWindow::updateStatusInfo()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_spectra.size()) {
        const SpectrumData &spec = m_spectra[m_currentIndex];
        m_statusLabel->setText(
            QString("%1 - %2 points")
                .arg(spec.name())
                .arg(spec.size())
        );
    } else {
        m_statusLabel->setText(tr("Ready"));
    }

    m_exportAct->setEnabled(m_currentIndex >= 0);
    m_calibrateAct->setEnabled(m_currentIndex >= 0);
    m_detectLinesAct->setEnabled(m_currentIndex >= 0);
    m_redshiftAct->setEnabled(m_currentIndex >= 0);
    m_smoothAct->setEnabled(m_currentIndex >= 0);
    m_normalizeAct->setEnabled(m_currentIndex >= 0);
    m_subtractContinuumAct->setEnabled(m_currentIndex >= 0);
}

SpectrumData& MainWindow::currentSpectrum()
{
    static SpectrumData empty;
    if (m_currentIndex >= 0 && m_currentIndex < m_spectra.size()) {
        return m_spectra[m_currentIndex];
    }
    return empty;
}

int MainWindow::currentSpectrumIndex() const
{
    return m_currentIndex;
}

void MainWindow::onImportFiles()
{
    DataImportDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QVector<SpectrumData> imported = dialog.importedSpectra();
        for (const SpectrumData &spec : imported) {
            m_spectra.append(spec);
        }
        refreshSpectrumList();

        if (!imported.isEmpty() && m_currentIndex < 0) {
            m_spectrumListView->setCurrentIndex(
                m_spectrumModel->index(m_spectra.size() - imported.size(), 0));
        }

        m_statusLabel->setText(
            QString("Imported %1 spectra").arg(imported.size()));
    }
}

void MainWindow::onImportDirectory()
{
    onImportFiles();
}

void MainWindow::onExport()
{
    if (m_currentIndex < 0) return;

    ExportDialog dialog(this);
    dialog.setSpectra(m_spectra);
    if (dialog.exec() == QDialog::Accepted) {
    }
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onWavelengthCalibration()
{
    if (m_currentIndex < 0) return;

    CalibrationDialog dialog(this);
    dialog.setSpectrum(m_spectra[m_currentIndex]);
    dialog.setCalibrator(m_processor->wavelengthCalibrator());

    if (dialog.exec() == QDialog::Accepted) {
        SpectrumData calibrated = dialog.calibratedSpectrum();
        m_spectra[m_currentIndex] = calibrated;
        m_spectrumModel->updateSpectrum(m_currentIndex, calibrated);
        m_spectrumPlot->setSpectrum(calibrated);
        m_statusLabel->setText(tr("Wavelength calibration applied"));
    }
}

void MainWindow::onDetectLines()
{
    if (m_currentIndex < 0) return;

    LineDetectionDialog dialog(this);
    dialog.setSpectrum(m_spectra[m_currentIndex]);

    if (dialog.exec() == QDialog::Accepted) {
        SpectrumData processed = dialog.processedSpectrum();
        m_spectra[m_currentIndex] = processed;
        m_spectrumModel->updateSpectrum(m_currentIndex, processed);
        m_spectrumPlot->setSpectrum(processed);
        m_lineModel->setSpectralLines(processed.spectralLines());
        m_statusLabel->setText(
            QString("Detected %1 spectral lines")
                .arg(processed.spectralLines().size()));
    }
}

void MainWindow::onCalculateRedshift()
{
    if (m_currentIndex < 0) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    SpectrumData &spec = m_spectra[m_currentIndex];
    double z = m_processor->redshiftCalculator()->calculateRedshift(spec);

    if (m_spectra[m_currentIndex].spectralLines().isEmpty()) {
        m_processor->lineFitter()->fitAllLines(m_spectra[m_currentIndex]);
    }

    QVector<SpectralLine> lines = m_spectra[m_currentIndex].spectralLines();
    for (SpectralLine &line : lines) {
        line.redshift = z;
        line.restWavelength = line.wavelength / (1 + z);
    }
    m_spectra[m_currentIndex].setSpectralLines(lines);
    m_spectra[m_currentIndex].setRedshift(z);

    m_spectrumModel->updateSpectrum(m_currentIndex, m_spectra[m_currentIndex]);
    m_spectrumPlot->setSpectrum(m_spectra[m_currentIndex]);
    m_lineModel->setSpectralLines(lines);

    QApplication::restoreOverrideCursor();

    QMessageBox::information(this, tr("Redshift Calculation"),
        QString("Redshift: z = %1\nConfidence: %2%")
            .arg(z, 0, 'f', 5)
            .arg(m_processor->redshiftCalculator()->correlationCoefficient() * 100, 0, 'f', 1));

    m_statusLabel->setText(QString("Redshift: z = " + QString::number(z, 'f', 5)));
}

void MainWindow::onSmoothSpectrum()
{
    if (m_currentIndex < 0) return;

    bool ok;
    int windowSize = QInputDialog::getInt(
        this, tr("Smooth Spectrum"),
        tr("Window size:"), 5, 3, 101, 2, &ok);

    if (ok) {
        SpectrumData smoothed = m_processor->smooth(m_spectra[m_currentIndex], windowSize);
        m_spectra[m_currentIndex] = smoothed;
        m_spectrumModel->updateSpectrum(m_currentIndex, smoothed);
        m_spectrumPlot->setSpectrum(smoothed);
        m_statusLabel->setText(tr("Smoothing applied (window = %1)").arg(windowSize));
    }
}

void MainWindow::onNormalizeSpectrum()
{
    if (m_currentIndex < 0) return;

    SpectrumData normalized = m_processor->normalize(m_spectra[m_currentIndex]);
    m_spectra[m_currentIndex] = normalized;
    m_spectrumModel->updateSpectrum(m_currentIndex, normalized);
    m_spectrumPlot->setSpectrum(normalized);
    m_statusLabel->setText(tr("Spectrum normalized"));
}

void MainWindow::onSubtractContinuum()
{
    if (m_currentIndex < 0) return;

    SpectrumData result = m_processor->subtractContinuum(m_spectra[m_currentIndex]);
    m_spectra[m_currentIndex] = result;
    m_spectrumModel->updateSpectrum(m_currentIndex, result);
    m_spectrumPlot->setSpectrum(result);
    m_spectrumPlot->setShowContinuum(false);
    m_showContinuumAct->setChecked(false);
    m_statusLabel->setText(tr("Continuum subtracted"));
}

void MainWindow::onZoomIn()
{
    m_spectrumPlot->zoomIn();
}

void MainWindow::onZoomOut()
{
    m_spectrumPlot->zoomOut();
}

void MainWindow::onResetZoom()
{
    m_spectrumPlot->resetZoom();
}

void MainWindow::onShowGrid(bool show)
{
    m_spectrumPlot->setShowGrid(show);
}

void MainWindow::onShowContinuum(bool show)
{
    m_spectrumPlot->setShowContinuum(show);
}

void MainWindow::onShowLines(bool show)
{
    m_spectrumPlot->setShowSpectralLines(show);
}

void MainWindow::onSpectrumSelected(const QModelIndex &index)
{
    if (!index.isValid()) {
        m_currentIndex = -1;
        updateStatusInfo();
        return;
    }

    m_currentIndex = index.row();
    if (m_currentIndex >= 0 && m_currentIndex < m_spectra.size()) {
        m_spectrumPlot->setSpectrum(m_spectra[m_currentIndex]);
        m_lineModel->setSpectralLines(m_spectra[m_currentIndex].spectralLines());
    }
    updateStatusInfo();
}

void MainWindow::onLineSelected(const QModelIndex &index)
{
    if (!index.isValid()) return;

    SpectralLine line = m_lineModel->lineAt(index.row());
    if (line.wavelength > 0) {
        m_spectrumPlot->setXRange(line.wavelength - line.fwhm * 5,
                                   line.wavelength + line.fwhm * 5);
    }
}

void MainWindow::onCursorPositionChanged(double wavelength, double flux)
{
    m_cursorLabel->setText(
        QString("λ: %1 Å  |  Flux: %2")
            .arg(wavelength, 0, 'f', 2)
            .arg(flux, 0, 'g', 4));
}

void MainWindow::onProcessingProgress(int current, int total)
{
    Q_UNUSED(current)
    Q_UNUSED(total)
    QApplication::processEvents();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About Astro Spectrum Analyzer"),
        tr("<h3>Astro Spectrum Analyzer</h3>"
           "<p>Version 1.0.0</p>"
           "<p>A powerful tool for astronomical spectral data processing and analysis.</p>"
           "<p><b>Features:</b></p>"
           "<ul>"
           "<li>Batch import of spectral data (FITS, CSV, TXT, JSON)</li>"
           "<li>Wavelength calibration</li>"
           "<li>Spectral line detection and fitting</li>"
           "<li>Redshift measurement</li>"
           "<li>Interactive visualization</li>"
           "<li>Analysis results export</li>"
           "</ul>"
           "<p>Built with Qt framework</p>"));
}

void MainWindow::onDocumentation()
{
    QDialog *docsDialog = new QDialog(this);
    docsDialog->setWindowTitle(tr("Documentation"));
    docsDialog->resize(600, 500);

    QVBoxLayout *layout = new QVBoxLayout(docsDialog);

    QTextEdit *textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setHtml(
        "<h1>Astro Spectrum Analyzer - Documentation</h1>"

        "<h2>1. Getting Started</h2>"
        "<p>Welcome to Astro Spectrum Analyzer, a comprehensive tool for processing and analyzing astronomical spectral data.</p>"

        "<h2>2. Importing Data</h2>"
        "<p>Use <b>File > Import Files</b> or <b>File > Import Directory</b> to load spectral data. "
        "Supported formats include:</p>"
        "<ul>"
        "<li><b>FITS</b> - Flexible Image Transport System</li>"
        "<li><b>CSV</b> - Comma-separated values</li>"
        "<li><b>TSV</b> - Tab-separated values</li>"
        "<li><b>TXT/DAT</b> - Text files</li>"
        "<li><b>JSON</b> - JavaScript Object Notation</li>"
        "</ul>"

        "<h2>3. Wavelength Calibration</h2>"
        "<p>Go to <b>Processing > Wavelength Calibration</b> to calibrate the wavelength axis. "
        "You can use linear, polynomial, or spline calibration methods.</p>"

        "<h2>4. Spectral Line Detection</h2>"
        "<p>Use <b>Processing > Detect Lines</b> to automatically detect and fit spectral lines. "
        "Adjust parameters like noise threshold, line width range, and line profile type (Gaussian, Lorentzian, Voigt).</p>"

        "<h2>5. Redshift Measurement</h2>"
        "<p>Calculate redshift using <b>Processing > Redshift</b>. The tool uses cross-correlation or line matching methods.</p>"

        "<h2>6. Visualization</h2>"
        "<ul>"
        "<li><b>Zoom:</b> Scroll with mouse wheel</li>"
        "<li><b>Pan:</b> Left-click and drag</li>"
        "<li><b>Reset View:</b> Press R or right-click</li>"
        "<li><b>Grid:</b> Toggle via View menu</li>"
        "<li><b>Continuum:</b> Toggle display</li>"
        "<li><b>Spectral Lines:</b> Toggle display and labels</li>"
        "</ul>"

        "<h2>7. Exporting Results</h2>"
        "<p>Use <b>File > Export</b> to save the current spectrum or all loaded spectra in various formats.</p>"

        "<h2>8. Keyboard Shortcuts</h2>"
        "<ul>"
        "<li><b>Ctrl+O:</b> Import files</li>"
        "<li><b>Ctrl+S:</b> Export</li>"
        "<li><b>Ctrl++:</b> Zoom in</li>"
        "<li><b>Ctrl+-:</b> Zoom out</li>"
        "<li><b>R:</b> Reset zoom</li>"
        "<li><b>Ctrl+Q:</b> Quit</li>"
        "</ul>"
    );

    layout->addWidget(textEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted,
            docsDialog, &QDialog::accept);
    layout->addWidget(buttonBox);

    docsDialog->exec();
}
