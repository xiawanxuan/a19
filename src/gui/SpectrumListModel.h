#ifndef SPECTRUMLISTMODEL_H
#define SPECTRUMLISTMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include "data_parser/SpectrumData.h"

class SpectrumListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        FilePathRole,
        ObjectNameRole,
        SizeRole,
        WavelengthRangeRole,
        FluxRangeRole,
        RedshiftRole,
        CalibratedRole,
        LineCountRole,
        SpectrumDataRole
    };

    explicit SpectrumListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setSpectra(const QVector<SpectrumData> &spectra);
    QVector<SpectrumData> spectra() const;

    void addSpectrum(const SpectrumData &spectrum);
    void removeSpectrum(int index);
    void clear();

    SpectrumData spectrumAt(int index) const;
    void updateSpectrum(int index, const SpectrumData &spectrum);

    int indexOfName(const QString &name) const;

private:
    QVector<SpectrumData> m_spectra;
};

#endif // SPECTRUMLISTMODEL_H
