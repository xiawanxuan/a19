#include "SpectrumListModel.h"
#include <QIcon>
#include <QFont>
#include <QBrush>

SpectrumListModel::SpectrumListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SpectrumListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_spectra.size();
}

QVariant SpectrumListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_spectra.size())
        return QVariant();

    const SpectrumData &spectrum = m_spectra[index.row()];

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return spectrum.name();

    case Qt::ToolTipRole:
        return QString("%1\n%2\n%3 points\nλ: %4 - %5 Å")
            .arg(spectrum.name())
            .arg(spectrum.filePath())
            .arg(spectrum.size())
            .arg(spectrum.minWavelength(), 0, 'f', 2)
            .arg(spectrum.maxWavelength(), 0, 'f', 2);

    case Qt::DecorationRole:
        if (spectrum.isCalibrated()) {
            return QColor(100, 200, 100);
        } else {
            return QColor(200, 150, 100);
        }

    case FilePathRole:
        return spectrum.filePath();

    case ObjectNameRole:
        return spectrum.objectName();

    case SizeRole:
        return spectrum.size();

    case WavelengthRangeRole:
        return QString("%1 - %2 Å")
            .arg(spectrum.minWavelength(), 0, 'f', 2)
            .arg(spectrum.maxWavelength(), 0, 'f', 2);

    case FluxRangeRole:
        return QString("%1 - %2")
            .arg(spectrum.minFlux(), 0, 'g', 4)
            .arg(spectrum.maxFlux(), 0, 'g', 4);

    case RedshiftRole:
        return spectrum.redshift();

    case CalibratedRole:
        return spectrum.isCalibrated();

    case LineCountRole:
        return spectrum.spectralLines().size();

    case SpectrumDataRole:
        return QVariant::fromValue(spectrum);

    default:
        return QVariant();
    }
}

QVariant SpectrumListModel::headerData(int section, Qt::Orientation orientation,
                                        int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return "Spectrum";
        default: return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags SpectrumListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void SpectrumListModel::setSpectra(const QVector<SpectrumData> &spectra)
{
    beginResetModel();
    m_spectra = spectra;
    endResetModel();
}

QVector<SpectrumData> SpectrumListModel::spectra() const
{
    return m_spectra;
}

void SpectrumListModel::addSpectrum(const SpectrumData &spectrum)
{
    beginInsertRows(QModelIndex(), m_spectra.size(), m_spectra.size());
    m_spectra.append(spectrum);
    endInsertRows();
}

void SpectrumListModel::removeSpectrum(int index)
{
    if (index < 0 || index >= m_spectra.size()) return;

    beginRemoveRows(QModelIndex(), index, index);
    m_spectra.removeAt(index);
    endRemoveRows();
}

void SpectrumListModel::clear()
{
    beginResetModel();
    m_spectra.clear();
    endResetModel();
}

SpectrumData SpectrumListModel::spectrumAt(int index) const
{
    if (index < 0 || index >= m_spectra.size())
        return SpectrumData();
    return m_spectra[index];
}

void SpectrumListModel::updateSpectrum(int index, const SpectrumData &spectrum)
{
    if (index < 0 || index >= m_spectra.size()) return;

    m_spectra[index] = spectrum;
    emit dataChanged(createIndex(index, 0), createIndex(index, 0));
}

int SpectrumListModel::indexOfName(const QString &name) const
{
    for (int i = 0; i < m_spectra.size(); ++i) {
        if (m_spectra[i].name() == name) {
            return i;
        }
    }
    return -1;
}
