#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include "data_parser/SpectrumData.h"
#include "data_parser/DataParser.h"
#include "spectrum_processing/SpectrumProcessor.h"
#include "visualization/SpectrumPlot.h"
#include "gui/SpectrumListModel.h"
#include "gui/SpectralLineTableModel.h"

class QListWidget;
class QListView;
class QTableView;
class QDockWidget;
class QToolBar;
class QStatusBar;
class QLabel;
class QAction;
class QTabWidget;
class QSplitter;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onImportFiles();
    void onImportDirectory();
    void onExport();
    void onExit();

    void onWavelengthCalibration();
    void onDetectLines();
    void onCalculateRedshift();
    void onSmoothSpectrum();
    void onNormalizeSpectrum();
    void onSubtractContinuum();

    void onZoomIn();
    void onZoomOut();
    void onResetZoom();

    void onShowGrid(bool show);
    void onShowContinuum(bool show);
    void onShowLines(bool show);

    void onSpectrumSelected(const QModelIndex &index);
    void onLineSelected(const QModelIndex &index);

    void onCursorPositionChanged(double wavelength, double flux);
    void onProcessingProgress(int current, int total);

    void onAbout();
    void onDocumentation();

private:
    void setupUi();
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWidgets();

    void updateCurrentSpectrum(const SpectrumData &spectrum);
    void refreshSpectrumList();
    void refreshLineTable();
    void updateStatusInfo();

    SpectrumData& currentSpectrum();
    int currentSpectrumIndex() const;

    DataParser *m_dataParser;
    SpectrumProcessor *m_processor;

    SpectrumPlot *m_spectrumPlot;

    SpectrumListModel *m_spectrumModel;
    SpectralLineTableModel *m_lineModel;

    QListView *m_spectrumListView;
    QTableView *m_lineTableView;

    QDockWidget *m_spectrumDock;
    QDockWidget *m_linesDock;
    QDockWidget *m_infoDock;

    QToolBar *m_fileToolBar;
    QToolBar *m_processingToolBar;
    QToolBar *m_viewToolBar;

    QStatusBar *m_statusBar;
    QLabel *m_statusLabel;
    QLabel *m_cursorLabel;

    QVector<SpectrumData> m_spectra;
    int m_currentIndex;

    QAction *m_importAct;
    QAction *m_importDirAct;
    QAction *m_exportAct;
    QAction *m_exitAct;

    QAction *m_calibrateAct;
    QAction *m_detectLinesAct;
    QAction *m_redshiftAct;
    QAction *m_smoothAct;
    QAction *m_normalizeAct;
    QAction *m_subtractContinuumAct;

    QAction *m_zoomInAct;
    QAction *m_zoomOutAct;
    QAction *m_resetZoomAct;
    QAction *m_showGridAct;
    QAction *m_showContinuumAct;
    QAction *m_showLinesAct;

    QAction *m_aboutAct;
    QAction *m_docsAct;
};

#endif // MAINWINDOW_H
