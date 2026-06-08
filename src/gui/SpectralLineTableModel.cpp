#include "SpectralLineTableModel.h"
#include <QBrush>
#include <QColor>
#include <QFont>

SpectralLineTableModel::SpectralLineTableModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_sortColumn(ColWavelength)
    , m_sortOrder(Qt::AscendingOrder)
{
}

int SpectralLineTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_lines.size();
}

int SpectralLineTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColCount;
}

QVariant SpectralLineTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_lines.size())
        return QVariant();

    const SpectralLine &line = m_lines[index.row()];

    if (role == Qt::BackgroundRole) {
        if (line.isEmission) {
            return QColor(255, 100, 100, 30);
        } else {
            return QColor(100, 200, 100, 30);
        }
    }

    if (role == Qt::ForegroundRole) {
        return QColor(220, 220, 220);
    }

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }

    if (role != Qt::DisplayRole && role != Qt::ToolTipRole) {
        return QVariant();
    }

    switch (index.column()) {
    case ColName:
        return line.name;

    case ColWavelength:
        return QString::number(line.wavelength, 'f', 3);

    case ColRestWavelength:
        if (line.restWavelength > 0) {
            return QString::number(line.restWavelength, 'f', 3);
        }
        return "-";

    case ColFlux:
        return QString::number(line.flux, 'g', 4);

    case ColContinuum:
        return QString::number(line.continuum, 'g', 4);

    case ColPeakHeight:
        return QString::number(line.peakHeight, 'g', 4);

    case ColFWHM:
        return QString::number(line.fwhm, 'f', 3);

    case ColEquivalentWidth:
        return QString::number(line.equivalentWidth, 'f', 3);

    case ColType:
        return line.isEmission ? "Emission" : "Absorption";

    case ColElement:
        return line.element;

    case ColRedshift:
        if (line.redshift != 0.0 || line.restWavelength > 0) {
            return QString::number(line.redshift, 'f', 5);
        }
        return "-";

    default:
        return QVariant();
    }
}

QVariant SpectralLineTableModel::headerData(int section, Qt::Orientation orientation,
                                             int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ColName: return "Name";
        case ColWavelength: return "λ (Å)";
        case ColRestWavelength: return "Rest λ (Å)";
        case ColFlux: return "Flux";
        case ColContinuum: return "Continuum";
        case ColPeakHeight: return "Peak Height";
        case ColFWHM: return "FWHM (Å)";
        case ColEquivalentWidth: return "EW (Å)";
        case ColType: return "Type";
        case ColElement: return "Element";
        case ColRedshift: return "z";
        default: return QVariant();
        }
    }

    if (orientation == Qt::Horizontal && role == Qt::ForegroundRole) {
        return QColor(255, 255, 255);
    }

    return QVariant();
}

Qt::ItemFlags SpectralLineTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void SpectralLineTableModel::setSpectralLines(const QVector<SpectralLine> &lines)
{
    beginResetModel();
    m_lines = lines;
    sort(m_sortColumn, m_sortOrder);
    endResetModel();
}

QVector<SpectralLine> SpectralLineTableModel::spectralLines() const
{
    return m_lines;
}

void SpectralLineTableModel::addLine(const SpectralLine &line)
{
    beginInsertRows(QModelIndex(), m_lines.size(), m_lines.size());
    m_lines.append(line);
    sort(m_sortColumn, m_sortOrder);
    endInsertRows();
}

void SpectralLineTableModel::removeLine(int index)
{
    if (index < 0 || index >= m_lines.size()) return;

    beginRemoveRows(QModelIndex(), index, index);
    m_lines.removeAt(index);
    endRemoveRows();
}

void SpectralLineTableModel::clear()
{
    beginResetModel();
    m_lines.clear();
    endResetModel();
}

SpectralLine SpectralLineTableModel::lineAt(int index) const
{
    if (index < 0 || index >= m_lines.size())
        return SpectralLine();
    return m_lines[index];
}

void SpectralLineTableModel::updateLine(int index, const SpectralLine &line)
{
    if (index < 0 || index >= m_lines.size()) return;

    m_lines[index] = line;
    emit dataChanged(createIndex(index, 0), createIndex(index, ColCount - 1));
}

void SpectralLineTableModel::sort(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;

    std::sort(m_lines.begin(), m_lines.end(),
              [column, order](const SpectralLine &a, const SpectralLine &b) {
        bool less = false;

        switch (column) {
        case ColName:
            less = a.name < b.name;
            break;
        case ColWavelength:
            less = a.wavelength < b.wavelength;
            break;
        case ColRestWavelength:
            less = a.restWavelength < b.restWavelength;
            break;
        case ColFlux:
            less = a.flux < b.flux;
            break;
        case ColContinuum:
            less = a.continuum < b.continuum;
            break;
        case ColPeakHeight:
            less = a.peakHeight < b.peakHeight;
            break;
        case ColFWHM:
            less = a.fwhm < b.fwhm;
            break;
        case ColEquivalentWidth:
            less = a.equivalentWidth < b.equivalentWidth;
            break;
        case ColType:
            less = a.isEmission < b.isEmission;
            break;
        case ColElement:
            less = a.element < b.element;
            break;
        case ColRedshift:
            less = a.redshift < b.redshift;
            break;
        default:
            less = a.wavelength < b.wavelength;
        }

        return (order == Qt::AscendingOrder) ? less : !less;
    });

    emit dataChanged(createIndex(0, 0), createIndex(m_lines.size() - 1, ColCount - 1));
}
