#ifndef SPECTRALLINETABLEMODEL_H
#define SPECTRALLINETABLEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "data_parser/SpectrumData.h"

class SpectralLineTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColName = 0,
        ColWavelength,
        ColRestWavelength,
        ColFlux,
        ColContinuum,
        ColPeakHeight,
        ColFWHM,
        ColEquivalentWidth,
        ColType,
        ColElement,
        ColRedshift,
        ColCount
    };

    explicit SpectralLineTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setSpectralLines(const QVector<SpectralLine> &lines);
    QVector<SpectralLine> spectralLines() const;

    void addLine(const SpectralLine &line);
    void removeLine(int index);
    void clear();

    SpectralLine lineAt(int index) const;
    void updateLine(int index, const SpectralLine &line);

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

private:
    QVector<SpectralLine> m_lines;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

#endif // SPECTRALLINETABLEMODEL_H
