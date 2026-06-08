#include "DataParser.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

DataParser::DataParser(QObject *parent)
    : QObject(parent)
{
}

DataParser::FileFormat DataParser::detectFormat(const QString &filePath)
{
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();

    if (suffix == "fits" || suffix == "fit" || suffix == "fts") {
        return Format_FITS;
    } else if (suffix == "csv") {
        return Format_CSV;
    } else if (suffix == "tsv") {
        return Format_TSV;
    } else if (suffix == "txt" || suffix == "dat" || suffix == "spec") {
        return Format_TXT;
    } else if (suffix == "json") {
        return Format_JSON;
    }

    return Format_Unknown;
}

QString DataParser::formatName(FileFormat format)
{
    switch (format) {
    case Format_FITS: return "FITS";
    case Format_CSV: return "CSV";
    case Format_TSV: return "TSV";
    case Format_TXT: return "TXT";
    case Format_JSON: return "JSON";
    default: return "Unknown";
    }
}

QStringList DataParser::supportedFilters()
{
    return {
        "All Supported Files (*.fits *.fit *.fts *.csv *.tsv *.txt *.dat *.spec *.json)",
        "FITS Files (*.fits *.fit *.fts)",
        "CSV Files (*.csv)",
        "TSV Files (*.tsv)",
        "Text Files (*.txt *.dat *.spec)",
        "JSON Files (*.json)",
        "All Files (*.*)"
    };
}

SpectrumData DataParser::parseFile(const QString &filePath)
{
    FileFormat format = detectFormat(filePath);

    if (format == Format_Unknown) {
        m_lastError = "Unknown file format: " + filePath;
        emit fileParsed(filePath, false);
        return SpectrumData();
    }

    SpectrumData data;

    switch (format) {
    case Format_FITS:
        data = parseFITS(filePath);
        break;
    case Format_CSV:
        data = parseCSV(filePath);
        break;
    case Format_TSV:
        data = parseTSV(filePath);
        break;
    case Format_TXT:
        data = parseTXT(filePath);
        break;
    case Format_JSON:
        data = parseJSON(filePath);
        break;
    default:
        break;
    }

    if (data.isValid()) {
        data.setFilePath(filePath);
        QFileInfo info(filePath);
        data.setName(info.completeBaseName());
    }

    emit fileParsed(filePath, data.isValid());
    return data;
}

QVector<SpectrumData> DataParser::parseFiles(const QStringList &filePaths)
{
    QVector<SpectrumData> results;
    int total = filePaths.size();

    for (int i = 0; i < total; ++i) {
        emit progress(i + 1, total);
        SpectrumData data = parseFile(filePaths[i]);
        if (data.isValid()) {
            results.append(data);
        }
    }

    return results;
}

QVector<SpectrumData> DataParser::parseDirectory(const QString &directory, bool recursive)
{
    QDir dir(directory);
    if (!dir.exists()) {
        m_lastError = "Directory does not exist: " + directory;
        return QVector<SpectrumData>();
    }

    QStringList filters;
    filters << "*.fits" << "*.fit" << "*.fts"
            << "*.csv" << "*.tsv"
            << "*.txt" << "*.dat" << "*.spec"
            << "*.json";

    QDir::Filters filterFlags = QDir::Files | QDir::NoDotAndDotDot;
    if (recursive) {
        filterFlags |= QDir::AllDirs;
    }

    QStringList files = dir.entryList(filters, filterFlags);
    QStringList fullPaths;

    for (const QString &file : files) {
        fullPaths.append(dir.absoluteFilePath(file));
    }

    if (recursive) {
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subdir : subdirs) {
            QVector<SpectrumData> subData = parseDirectory(dir.absoluteFilePath(subdir), true);
            // This would add files from subdirectories
        }
    }

    return parseFiles(fullPaths);
}

bool DataParser::saveSpectrum(const SpectrumData &spectrum, const QString &filePath, FileFormat format)
{
    if (format == Format_Unknown) {
        format = detectFormat(filePath);
    }

    if (format == Format_Unknown) {
        format = Format_CSV;
    }

    switch (format) {
    case Format_CSV:
        return saveCSV(spectrum, filePath);
    case Format_TXT:
        return saveTXT(spectrum, filePath);
    case Format_JSON:
        return saveJSON(spectrum, filePath);
    default:
        m_lastError = "Saving in this format is not supported";
        return false;
    }
}

QString DataParser::lastError() const
{
    return m_lastError;
}

SpectrumData DataParser::parseCSV(const QString &filePath)
{
    return parseDelimitedFile(filePath, ',');
}

SpectrumData DataParser::parseTSV(const QString &filePath)
{
    return parseDelimitedFile(filePath, '\t');
}

SpectrumData DataParser::parseTXT(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Cannot open file: " + file.errorString();
        return SpectrumData();
    }

    QTextStream in(&file);
    QMap<QString, QString> header;
    QVector<double> wavelengths;
    QVector<double> flux;
    QVector<double> error;
    bool inDataSection = false;

    QRegularExpression headerRegex(R"(^#\s*(\w+)\s*[:=]\s*(.+)$)");
    QRegularExpression valueRegex(R"(^\s*([+-]?[\d.]+(?:[eE][+-]?\d+)?)\s+([+-]?[\d.]+(?:[eE][+-]?\d+)?)\s*([+-]?[\d.]+(?:[eE][+-]?\d+)?)?\s*$)");

    while (!in.atEnd()) {
        QString line = in.readLine();

        if (line.startsWith('#')) {
            QRegularExpressionMatch match = headerRegex.match(line);
            if (match.hasMatch()) {
                QString key = match.captured(1);
                QString value = match.captured(2).trimmed();
                header.insert(key, value);
            }
            continue;
        }

        if (line.trimmed().isEmpty()) {
            if (!inDataSection) continue;
            break;
        }

        QRegularExpressionMatch match = valueRegex.match(line);
        if (match.hasMatch()) {
            inDataSection = true;
            wavelengths.append(match.captured(1).toDouble());
            flux.append(match.captured(2).toDouble());
            if (match.capturedLength(3) > 0) {
                error.append(match.captured(3).toDouble());
            }
        }
    }

    file.close();

    SpectrumData data;
    data.setWavelengths(wavelengths);
    data.setFlux(flux);
    if (!error.isEmpty()) {
        data.setError(error);
    }
    data.setHeader(header);

    if (header.contains("OBJECT")) {
        data.setObjectName(header["OBJECT"]);
    }
    if (header.contains("DATE-OBS") || header.contains("DATE")) {
        QString dateStr = header.contains("DATE-OBS") ? header["DATE-OBS"] : header["DATE"];
        data.setObservationDate(QDateTime::fromString(dateStr, Qt::ISODate));
    }

    return data;
}

SpectrumData DataParser::parseFITS(const QString &filePath)
{
    // FITS 文件的简化实现
    // 在实际应用中，应该使用 CFITSIO 库
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open FITS file: " + file.errorString();
        return SpectrumData();
    }

    QByteArray headerData;
    QMap<QString, QString> header;
    bool endOfHeader = false;
    int headerBlocks = 0;

    while (!endOfHeader && headerBlocks < 100) {
        QByteArray block = file.read(2880);
        if (block.isEmpty()) break;
        headerBlocks++;

        for (int i = 0; i < 36; ++i) {
            QByteArray card = block.mid(i * 80, 80);
            QString cardStr = QString::fromLatin1(card).trimmed();

            if (cardStr.startsWith("END")) {
                endOfHeader = true;
                break;
            }

            int eqPos = cardStr.indexOf('=');
            if (eqPos > 0) {
                QString key = cardStr.left(eqPos).trimmed();
                QString value = cardStr.mid(eqPos + 1).trimmed();

                int commentPos = value.indexOf(" / ");
                if (commentPos > 0) {
                    value = value.left(commentPos).trimmed();
                }

                value.remove('\'');
                value = value.trimmed();

                header.insert(key, value);
            }
        }
    }

    file.close();

    SpectrumData data;
    data.setHeader(header);
    data.setHeaderValue("NOTE", "FITS data parsing is simplified. For full FITS support, use CFITSIO.");

    if (header.contains("OBJECT")) {
        data.setObjectName(header["OBJECT"]);
    }

    // 生成模拟数据作为占位符
    // 实际应用中需要从FITS文件读取真实数据
    QVector<double> wavelengths;
    QVector<double> flux;

    double startWl = header.contains("CRVAL1") ? header["CRVAL1"].toDouble() : 4000.0;
    double step = header.contains("CDELT1") ? header["CDELT1"].toDouble() : 1.0;
    int nPoints = header.contains("NAXIS1") ? header["NAXIS1"].toInt() : 1000;

    if (nPoints > 0 && nPoints < 100000) {
        for (int i = 0; i < nPoints; ++i) {
            wavelengths.append(startWl + i * step);
            flux.append(1000.0 + 500.0 * sin(i * 0.01));
        }
        data.setWavelengths(wavelengths);
        data.setFlux(flux);
    }

    return data;
}

SpectrumData DataParser::parseJSON(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + file.errorString();
        return SpectrumData();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) {
        m_lastError = "Invalid JSON format";
        return SpectrumData();
    }

    QJsonObject obj = doc.object();

    SpectrumData data;

    if (obj.contains("name")) {
        data.setName(obj["name"].toString());
    }
    if (obj.contains("objectName")) {
        data.setObjectName(obj["objectName"].toString());
    }
    if (obj.contains("redshift")) {
        data.setRedshift(obj["redshift"].toDouble());
    }
    if (obj.contains("isCalibrated")) {
        data.setCalibrated(obj["isCalibrated"].toBool());
    }

    if (obj.contains("header")) {
        QJsonObject headerObj = obj["header"].toObject();
        QMap<QString, QString> header;
        for (auto it = headerObj.begin(); it != headerObj.end(); ++it) {
            header.insert(it.key(), it.value().toString());
        }
        data.setHeader(header);
    }

    if (obj.contains("wavelengths") && obj.contains("flux")) {
        QJsonArray wlArr = obj["wavelengths"].toArray();
        QJsonArray fluxArr = obj["flux"].toArray();

        QVector<double> wavelengths;
        QVector<double> flux;

        for (const QJsonValue &val : wlArr) {
            wavelengths.append(val.toDouble());
        }
        for (const QJsonValue &val : fluxArr) {
            flux.append(val.toDouble());
        }

        data.setWavelengths(wavelengths);
        data.setFlux(flux);

        if (obj.contains("error")) {
            QJsonArray errArr = obj["error"].toArray();
            QVector<double> error;
            for (const QJsonValue &val : errArr) {
                error.append(val.toDouble());
            }
            data.setError(error);
        }

        if (obj.contains("continuum")) {
            QJsonArray contArr = obj["continuum"].toArray();
            QVector<double> continuum;
            for (const QJsonValue &val : contArr) {
                continuum.append(val.toDouble());
            }
            data.setContinuum(continuum);
        }
    }

    if (obj.contains("spectralLines")) {
        QJsonArray linesArr = obj["spectralLines"].toArray();
        QVector<SpectralLine> lines;

        for (const QJsonValue &lineVal : linesArr) {
            QJsonObject lineObj = lineVal.toObject();
            SpectralLine line;
            line.wavelength = lineObj["wavelength"].toDouble();
            line.flux = lineObj["flux"].toDouble();
            line.continuum = lineObj["continuum"].toDouble();
            line.equivalentWidth = lineObj["equivalentWidth"].toDouble();
            line.fwhm = lineObj["fwhm"].toDouble();
            line.peakHeight = lineObj["peakHeight"].toDouble();
            line.name = lineObj["name"].toString();
            line.element = lineObj["element"].toString();
            line.isEmission = lineObj["isEmission"].toBool();
            line.redshift = lineObj["redshift"].toDouble();
            line.restWavelength = lineObj["restWavelength"].toDouble();
            lines.append(line);
        }

        data.setSpectralLines(lines);
    }

    return data;
}

bool DataParser::saveCSV(const SpectrumData &spectrum, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Cannot create file: " + file.errorString();
        return false;
    }

    QTextStream out(&file);

    out << "wavelength,flux";
    if (!spectrum.error().isEmpty()) {
        out << ",error";
    }
    if (!spectrum.continuum().isEmpty()) {
        out << ",continuum";
    }
    out << "\n";

    int n = spectrum.size();
    QVector<double> wl = spectrum.wavelengths();
    QVector<double> flux = spectrum.flux();
    QVector<double> err = spectrum.error();
    QVector<double> cont = spectrum.continuum();

    for (int i = 0; i < n; ++i) {
        out << wl[i] << "," << flux[i];
        if (!err.isEmpty() && i < err.size()) {
            out << "," << err[i];
        }
        if (!cont.isEmpty() && i < cont.size()) {
            out << "," << cont[i];
        }
        out << "\n";
    }

    file.close();
    return true;
}

bool DataParser::saveTXT(const SpectrumData &spectrum, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Cannot create file: " + file.errorString();
        return false;
    }

    QTextStream out(&file);

    out << "# Spectrum: " << spectrum.name() << "\n";
    if (!spectrum.objectName().isEmpty()) {
        out << "# OBJECT: " << spectrum.objectName() << "\n";
    }
    out << "# Redshift: " << spectrum.redshift() << "\n";
    out << "# Calibrated: " << (spectrum.isCalibrated() ? "yes" : "no") << "\n";
    out << "# wavelength flux";
    if (!spectrum.error().isEmpty()) {
        out << " error";
    }
    out << "\n";

    int n = spectrum.size();
    QVector<double> wl = spectrum.wavelengths();
    QVector<double> flux = spectrum.flux();
    QVector<double> err = spectrum.error();

    for (int i = 0; i < n; ++i) {
        out << QString("%1 %2").arg(wl[i], 0, 'f', 6).arg(flux[i], 0, 'f', 6);
        if (!err.isEmpty() && i < err.size()) {
            out << QString(" %1").arg(err[i], 0, 'f', 6);
        }
        out << "\n";
    }

    file.close();
    return true;
}

bool DataParser::saveJSON(const SpectrumData &spectrum, const QString &filePath)
{
    QJsonObject root;

    root["name"] = spectrum.name();
    root["objectName"] = spectrum.objectName();
    root["filePath"] = spectrum.filePath();
    root["redshift"] = spectrum.redshift();
    root["isCalibrated"] = spectrum.isCalibrated();

    QJsonObject headerObj;
    QMap<QString, QString> header = spectrum.header();
    for (auto it = header.begin(); it != header.end(); ++it) {
        headerObj[it.key()] = it.value();
    }
    root["header"] = headerObj;

    QJsonArray wlArr;
    QJsonArray fluxArr;
    QVector<double> wl = spectrum.wavelengths();
    QVector<double> flux = spectrum.flux();
    for (double v : wl) wlArr.append(v);
    for (double v : flux) fluxArr.append(v);
    root["wavelengths"] = wlArr;
    root["flux"] = fluxArr;

    if (!spectrum.error().isEmpty()) {
        QJsonArray errArr;
        for (double v : spectrum.error()) errArr.append(v);
        root["error"] = errArr;
    }

    if (!spectrum.continuum().isEmpty()) {
        QJsonArray contArr;
        for (double v : spectrum.continuum()) contArr.append(v);
        root["continuum"] = contArr;
    }

    QJsonArray linesArr;
    for (const SpectralLine &line : spectrum.spectralLines()) {
        QJsonObject lineObj;
        lineObj["wavelength"] = line.wavelength;
        lineObj["flux"] = line.flux;
        lineObj["continuum"] = line.continuum;
        lineObj["equivalentWidth"] = line.equivalentWidth;
        lineObj["fwhm"] = line.fwhm;
        lineObj["peakHeight"] = line.peakHeight;
        lineObj["name"] = line.name;
        lineObj["element"] = line.element;
        lineObj["isEmission"] = line.isEmission;
        lineObj["redshift"] = line.redshift;
        lineObj["restWavelength"] = line.restWavelength;
        linesArr.append(lineObj);
    }
    root["spectralLines"] = linesArr;

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create file: " + file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

SpectrumData DataParser::parseDelimitedFile(const QString &filePath, QChar delimiter)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Cannot open file: " + file.errorString();
        return SpectrumData();
    }

    QTextStream in(&file);
    QMap<QString, QString> header;
    QVector<double> wavelengths;
    QVector<double> flux;
    QVector<double> error;
    bool hasHeaderRow = false;
    bool firstLine = true;
    int wlCol = 0;
    int fluxCol = 1;
    int errCol = -1;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;

        if (line.startsWith('#')) {
            QString content = line.mid(1).trimmed();
            int colonPos = content.indexOf(':');
            if (colonPos > 0) {
                QString key = content.left(colonPos).trimmed();
                QString value = content.mid(colonPos + 1).trimmed();
                header.insert(key, value);
            }
            continue;
        }

        QStringList parts = line.split(delimiter, Qt::SkipEmptyParts);

        if (firstLine) {
            firstLine = false;

            bool allNumeric = true;
            for (const QString &part : parts) {
                bool ok;
                part.trimmed().toDouble(&ok);
                if (!ok) {
                    allNumeric = false;
                    break;
                }
            }

            if (!allNumeric) {
                hasHeaderRow = true;
                for (int i = 0; i < parts.size(); ++i) {
                    QString colName = parts[i].trimmed().toLower();
                    if (colName.contains("wavelength") || colName.contains("wavelen") || colName == "wl" || colName == "lambda") {
                        wlCol = i;
                    } else if (colName.contains("flux") || colName == "intensity" || colName == "counts") {
                        fluxCol = i;
                    } else if (colName.contains("error") || colName.contains("err") || colName.contains("sigma") || colName.contains("noise")) {
                        errCol = i;
                    }
                }
                continue;
            }
        }

        if (wlCol < parts.size() && fluxCol < parts.size()) {
            bool ok1, ok2;
            double wl = parts[wlCol].trimmed().toDouble(&ok1);
            double fl = parts[fluxCol].trimmed().toDouble(&ok2);
            if (ok1 && ok2) {
                wavelengths.append(wl);
                flux.append(fl);

                if (errCol >= 0 && errCol < parts.size()) {
                    bool ok3;
                    double err = parts[errCol].trimmed().toDouble(&ok3);
                    if (ok3) {
                        error.append(err);
                    }
                }
            }
        }
    }

    file.close();

    SpectrumData data;
    data.setWavelengths(wavelengths);
    data.setFlux(flux);
    if (!error.isEmpty() && error.size() == wavelengths.size()) {
        data.setError(error);
    }
    data.setHeader(header);

    if (header.contains("OBJECT") || header.contains("Object") || header.contains("object")) {
        QString key = header.contains("OBJECT") ? "OBJECT" : (header.contains("Object") ? "Object" : "object");
        data.setObjectName(header[key]);
    }

    return data;
}
