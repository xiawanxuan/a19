#ifndef DATAPARSER_H
#define DATAPARSER_H

#include <QObject>
#include <QStringList>
#include <QVector>
#include "SpectrumData.h"

class DataParser : public QObject
{
    Q_OBJECT

public:
    explicit DataParser(QObject *parent = nullptr);

    enum FileFormat {
        Format_Unknown,
        Format_FITS,
        Format_CSV,
        Format_TSV,
        Format_TXT,
        Format_JSON
    };

    static FileFormat detectFormat(const QString &filePath);
    static QString formatName(FileFormat format);
    static QStringList supportedFilters();

    SpectrumData parseFile(const QString &filePath);
    QVector<SpectrumData> parseFiles(const QStringList &filePaths);
    QVector<SpectrumData> parseDirectory(const QString &directory, bool recursive = false);

    bool saveSpectrum(const SpectrumData &spectrum, const QString &filePath, FileFormat format);

    QString lastError() const;

signals:
    void progress(int current, int total);
    void fileParsed(const QString &filePath, bool success);

private:
    SpectrumData parseCSV(const QString &filePath);
    SpectrumData parseTSV(const QString &filePath);
    SpectrumData parseTXT(const QString &filePath);
    SpectrumData parseFITS(const QString &filePath);
    SpectrumData parseJSON(const QString &filePath);

    bool saveCSV(const SpectrumData &spectrum, const QString &filePath);
    bool saveTXT(const SpectrumData &spectrum, const QString &filePath);
    bool saveJSON(const SpectrumData &spectrum, const QString &filePath);

    SpectrumData parseDelimitedFile(const QString &filePath, QChar delimiter);

    QString m_lastError;
};

#endif // DATAPARSER_H
