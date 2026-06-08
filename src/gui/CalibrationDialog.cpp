#include "CalibrationDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>

CalibrationDialog::CalibrationDialog(QWidget *parent)
    : QDialog(parent)
    , m_calibrator(new WavelengthCalibrator(this))
    , m_ownsCalibrator(true)
{
    setWindowTitle("Wavelength Calibration");
    setMinimumSize(700, 500);

    setupUi();
}

void CalibrationDialog::setSpectrum(const SpectrumData &spectrum)
{
    m_spectrum = spectrum;
    m_calibratedSpectrum = spectrum;
}

SpectrumData CalibrationDialog::calibratedSpectrum() const
{
    return m_calibratedSpectrum;
}

void CalibrationDialog::setCalibrator(WavelengthCalibrator *calibrator)
{
    if (m_ownsCalibrator && m_calibrator) {
        delete m_calibrator;
    }
    m_calibrator = calibrator;
    m_ownsCalibrator = false;
    updateTable();
    updateCalibrationInfo();
}

void CalibrationDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *pointsGroup = new QGroupBox("Calibration Points");
    QVBoxLayout *pointsLayout = new QVBoxLayout(pointsGroup);

    m_pointsTable = new QTableWidget(0, 3);
    m_pointsTable->setHorizontalHeaderLabels({"Pixel", "Wavelength (Å)", "Line Name"});
    m_pointsTable->horizontalHeader()->setStretchLastSection(true);
    m_pointsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pointsLayout->addWidget(m_pointsTable);

    QHBoxLayout *pointBtnLayout = new QHBoxLayout();
    m_addPointBtn = new QPushButton("Add Point");
    m_removePointBtn = new QPushButton("Remove Selected");
    m_clearBtn = new QPushButton("Clear All");
    m_loadLinesBtn = new QPushButton("Load Known Lines");

    connect(m_addPointBtn, &QPushButton::clicked,
            this, &CalibrationDialog::addCalibrationPoint);
    connect(m_removePointBtn, &QPushButton::clicked,
            this, &CalibrationDialog::removeSelectedPoint);
    connect(m_clearBtn, &QPushButton::clicked,
            this, &CalibrationDialog::clearAllPoints);
    connect(m_loadLinesBtn, &QPushButton::clicked,
            this, &CalibrationDialog::loadKnownLines);

    pointBtnLayout->addWidget(m_addPointBtn);
    pointBtnLayout->addWidget(m_removePointBtn);
    pointBtnLayout->addWidget(m_clearBtn);
    pointBtnLayout->addWidget(m_loadLinesBtn);
    pointBtnLayout->addStretch();

    pointsLayout->addLayout(pointBtnLayout);

    QHBoxLayout *knownLinesLayout = new QHBoxLayout();
    knownLinesLayout->addWidget(new QLabel("Quick add:"));
    m_knownLinesCombo = new QComboBox();

    QMap<QString, double> lines = WavelengthCalibrator::knownEmissionLines();
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        m_knownLinesCombo->addItem(it.key(), it.value());
    }

    knownLinesLayout->addWidget(m_knownLinesCombo, 1);
    pointsLayout->addLayout(knownLinesLayout);

    mainLayout->addWidget(pointsGroup);

    QGroupBox *methodGroup = new QGroupBox("Calibration Method");
    QGridLayout *methodLayout = new QGridLayout(methodGroup);

    methodLayout->addWidget(new QLabel("Method:"), 0, 0);
    m_methodCombo = new QComboBox();
    m_methodCombo->addItem("Linear", WavelengthCalibrator::Linear);
    m_methodCombo->addItem("Polynomial", WavelengthCalibrator::Polynomial);
    m_methodCombo->addItem("Spline", WavelengthCalibrator::Spline);
    connect(m_methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CalibrationDialog::methodChanged);
    methodLayout->addWidget(m_methodCombo, 0, 1);

    methodLayout->addWidget(new QLabel("Polynomial order:"), 1, 0);
    m_polyOrderSpin = new QDoubleSpinBox();
    m_polyOrderSpin->setRange(1, 10);
    m_polyOrderSpin->setValue(3);
    m_polyOrderSpin->setDecimals(0);
    m_polyOrderSpin->setEnabled(false);
    connect(m_polyOrderSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CalibrationDialog::updateCalibration);
    methodLayout->addWidget(m_polyOrderSpin, 1, 1);

    mainLayout->addWidget(methodGroup);

    QGroupBox *infoGroup = new QGroupBox("Calibration Info");
    QGridLayout *infoLayout = new QGridLayout(infoGroup);

    infoLayout->addWidget(new QLabel("Number of points:"), 0, 0);
    m_numPointsLabel = new QLabel("0");
    infoLayout->addWidget(m_numPointsLabel, 0, 1);

    infoLayout->addWidget(new QLabel("Calibration error:"), 1, 0);
    m_errorLabel = new QLabel("N/A");
    infoLayout->addWidget(m_errorLabel, 1, 1);

    mainLayout->addWidget(infoGroup);

    QHBoxLayout *dialogBtnLayout = new QHBoxLayout();
    m_applyBtn = new QPushButton("Apply Calibration");
    m_applyBtn->setDefault(true);
    m_applyBtn->setEnabled(false);
    m_cancelBtn = new QPushButton("Cancel");

    connect(m_applyBtn, &QPushButton::clicked,
            this, &CalibrationDialog::applyCalibration);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &QDialog::reject);

    dialogBtnLayout->addStretch();
    dialogBtnLayout->addWidget(m_applyBtn);
    dialogBtnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(dialogBtnLayout);
}

void CalibrationDialog::addCalibrationPoint()
{
    bool ok;
    double pixel = QInputDialog::getDouble(this, "Add Calibration Point",
                                            "Pixel position:", 0, -10000, 10000, 2, &ok);
    if (!ok) return;

    double wavelength = QInputDialog::getDouble(this, "Add Calibration Point",
                                                  "Wavelength (Å):", 5000, 1, 100000, 3, &ok);
    if (!ok) return;

    QString name = QInputDialog::getText(this, "Add Calibration Point",
                                          "Line name (optional):");

    m_calibrator->addCalibrationPoint(pixel, wavelength, name);
    updateTable();
    updateCalibrationInfo();
    m_applyBtn->setEnabled(m_calibrator->isReady());
}

void CalibrationDialog::removeSelectedPoint()
{
    QList<int> selectedRows;
    for (QModelIndex index : m_pointsTable->selectionModel()->selectedRows()) {
        selectedRows.append(index.row());
    }

    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<int>());

    QVector<WavelengthCalibrator::CalibrationPoint> points = m_calibrator->calibrationPoints();
    for (int row : selectedRows) {
        if (row < points.size()) {
            points.removeAt(row);
        }
    }

    m_calibrator->setCalibrationPoints(points);
    updateTable();
    updateCalibrationInfo();
    m_applyBtn->setEnabled(m_calibrator->isReady());
}

void CalibrationDialog::clearAllPoints()
{
    m_calibrator->clearCalibrationPoints();
    updateTable();
    updateCalibrationInfo();
    m_applyBtn->setEnabled(false);
}

void CalibrationDialog::loadKnownLines()
{
    QMap<QString, double> lines = WavelengthCalibrator::knownEmissionLines();

    QVector<WavelengthCalibrator::CalibrationPoint> points = m_calibrator->calibrationPoints();

    int i = 0;
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        WavelengthCalibrator::CalibrationPoint p;
        p.pixel = it.value();
        p.wavelength = it.value();
        p.lineName = it.key();
        points.append(p);
        i++;
        if (i >= 5) break;
    }

    m_calibrator->setCalibrationPoints(points);
    updateTable();
    updateCalibrationInfo();
    m_applyBtn->setEnabled(m_calibrator->isReady());
}

void CalibrationDialog::applyCalibration()
{
    if (!m_calibrator->isReady()) {
        QMessageBox::warning(this, "Not Ready",
                             "Please add at least 2 calibration points.");
        return;
    }

    m_calibratedSpectrum = m_spectrum.copy();
    if (m_calibrator->calibrate(m_calibratedSpectrum)) {
        accept();
    } else {
        QMessageBox::critical(this, "Calibration Failed",
                              "Wavelength calibration failed.");
    }
}

void CalibrationDialog::methodChanged(int index)
{
    WavelengthCalibrator::CalibrationMethod method =
        static_cast<WavelengthCalibrator::CalibrationMethod>(
            m_methodCombo->itemData(index).toInt());

    m_calibrator->setMethod(method);
    m_polyOrderSpin->setEnabled(method == WavelengthCalibrator::Polynomial);
    updateCalibrationInfo();
}

void CalibrationDialog::updateCalibration()
{
    int order = static_cast<int>(m_polyOrderSpin->value());
    m_calibrator->setPolynomialOrder(order);
    updateCalibrationInfo();
}

void CalibrationDialog::updateTable()
{
    QVector<WavelengthCalibrator::CalibrationPoint> points = m_calibrator->calibrationPoints();

    m_pointsTable->setRowCount(points.size());

    for (int i = 0; i < points.size(); ++i) {
        m_pointsTable->setItem(i, 0, new QTableWidgetItem(QString::number(points[i].pixel, 'f', 2)));
        m_pointsTable->setItem(i, 1, new QTableWidgetItem(QString::number(points[i].wavelength, 'f', 3)));
        m_pointsTable->setItem(i, 2, new QTableWidgetItem(points[i].lineName));
    }
}

void CalibrationDialog::updateCalibrationInfo()
{
    QVector<WavelengthCalibrator::CalibrationPoint> points = m_calibrator->calibrationPoints();
    m_numPointsLabel->setText(QString::number(points.size()));

    if (m_calibrator->isReady()) {
        m_errorLabel->setText(QString("%1 Å").arg(m_calibrator->calibrationError(), 0, 'f', 4));
    } else {
        m_errorLabel->setText("N/A (need more points)");
    }
}
