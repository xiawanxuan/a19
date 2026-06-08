#include "ReportGenerator.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QPixmap>
#include <QPainter>
#include <QPrinter>
#include <QTextDocument>
#include <QFileInfo>
#include <QDir>
#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QtMath>

ReportGenerator::ReportGenerator(QObject *parent)
    : QObject(parent)
{
    m_config.date = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

void ReportGenerator::setConfig(const ReportConfig &config)
{
    m_config = config;
}

ReportGenerator::ReportConfig ReportGenerator::config() const
{
    return m_config;
}

bool ReportGenerator::generateReport(const SpectrumData &spectrum,
                                      const QString &outputPath,
                                      ReportFormat format)
{
    QList<SpectralLibraryMatcher::MatchResult> emptyMatches;
    StellarPopulationFitter::PopulationResult emptyPop;

    if (format == PDF) {
        QString html = generateHtmlReport(spectrum, emptyMatches, emptyPop, false);
        return savePdf(html, outputPath);
    } else if (format == HTML) {
        QString html = generateHtmlReport(spectrum, emptyMatches, emptyPop, false);
        return saveHtml(html, outputPath);
    } else {
        QString text = generateTextReport(spectrum, emptyMatches, emptyPop, false);
        return saveText(text, outputPath);
    }
}

bool ReportGenerator::generateReport(const SpectrumData &spectrum,
                                      const QList<SpectralLibraryMatcher::MatchResult> &matches,
                                      const QString &outputPath,
                                      ReportFormat format)
{
    StellarPopulationFitter::PopulationResult emptyPop;

    if (format == PDF) {
        QString html = generateHtmlReport(spectrum, matches, emptyPop, false);
        return savePdf(html, outputPath);
    } else if (format == HTML) {
        QString html = generateHtmlReport(spectrum, matches, emptyPop, false);
        return saveHtml(html, outputPath);
    } else {
        QString text = generateTextReport(spectrum, matches, emptyPop, false);
        return saveText(text, outputPath);
    }
}

bool ReportGenerator::generateReport(const SpectrumData &spectrum,
                                      const StellarPopulationFitter::PopulationResult &popResult,
                                      const QString &outputPath,
                                      ReportFormat format)
{
    QList<SpectralLibraryMatcher::MatchResult> emptyMatches;

    if (format == PDF) {
        QString html = generateHtmlReport(spectrum, emptyMatches, popResult, true);
        return savePdf(html, outputPath);
    } else if (format == HTML) {
        QString html = generateHtmlReport(spectrum, emptyMatches, popResult, true);
        return saveHtml(html, outputPath);
    } else {
        QString text = generateTextReport(spectrum, emptyMatches, popResult, true);
        return saveText(text, outputPath);
    }
}

bool ReportGenerator::generateFullReport(const SpectrumData &spectrum,
                                          const QList<SpectralLibraryMatcher::MatchResult> &matches,
                                          const StellarPopulationFitter::PopulationResult &popResult,
                                          const QString &outputPath,
                                          ReportFormat format)
{
    if (format == PDF) {
        QString html = generateHtmlReport(spectrum, matches, popResult, true);
        return savePdf(html, outputPath);
    } else if (format == HTML) {
        QString html = generateHtmlReport(spectrum, matches, popResult, true);
        return saveHtml(html, outputPath);
    } else {
        QString text = generateTextReport(spectrum, matches, popResult, true);
        return saveText(text, outputPath);
    }
}

QString ReportGenerator::generateHtmlReport(
    const SpectrumData &spectrum,
    const QList<SpectralLibraryMatcher::MatchResult> &matches,
    const StellarPopulationFitter::PopulationResult &popResult,
    bool includePop)
{
    QString html;
    QTextStream stream(&html);

    stream << "<!DOCTYPE html>\n";
    stream << "<html>\n<head>\n";
    stream << "<meta charset=\"UTF-8\">\n";
    stream << "<title>" << m_config.title << "</title>\n";
    stream << "<style>\n";
    stream << "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 40px; color: #333; line-height: 1.6; }\n";
    stream << "h1 { color: #1a5276; border-bottom: 3px solid #2980b9; padding-bottom: 10px; }\n";
    stream << "h2 { color: #2874a6; border-bottom: 2px solid #3498db; padding-bottom: 5px; margin-top: 30px; }\n";
    stream << "h3 { color: #2e86c1; }\n";
    stream << "table { border-collapse: collapse; width: 100%; margin: 15px 0; }\n";
    stream << "th, td { border: 1px solid #ddd; padding: 10px; text-align: left; }\n";
    stream << "th { background-color: #2980b9; color: white; }\n";
    stream << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
    stream << ".info-box { background-color: #eaf2f8; border-left: 4px solid #2980b9; padding: 15px; margin: 15px 0; }\n";
    stream << ".warning-box { background-color: #fef9e7; border-left: 4px solid #f39c12; padding: 15px; margin: 15px 0; }\n";
    stream << ".success-box { background-color: #eafaf1; border-left: 4px solid #27ae60; padding: 15px; margin: 15px 0; }\n";
    stream << ".meta { color: #666; font-size: 0.9em; }\n";
    stream << ".value { font-weight: bold; color: #1a5276; }\n";
    stream << "img { max-width: 100%; height: auto; border: 1px solid #ddd; border-radius: 5px; }\n";
    stream << "</style>\n";
    stream << "</head>\n<body>\n";

    stream << generateHeaderHtml();
    stream << generateSpectrumInfoHtml(spectrum);

    if (m_config.includeSpectrumPlot) {
        QPixmap plot = generateSpectrumPlot(spectrum, 800, 400);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        plot.save(&buffer, "PNG");
        QString base64 = QString::fromLatin1(byteArray.toBase64().data());

        stream << "<h2>Spectrum Plot</h2>\n";
        stream << "<img src=\"data:image/png;base64," << base64 << "\" ";
        stream << "alt=\"Spectrum\" />\n";
    }

    if (m_config.includeLineTable) {
        stream << generateSpectralLinesHtml(spectrum);
    }

    if (m_config.includeRedshift) {
        stream << generateRedshiftHtml(spectrum);
    }

    if (m_config.includeLibraryMatch && !matches.isEmpty()) {
        stream << generateLibraryMatchHtml(matches);
    }

    if (includePop && m_config.includePopulationFit) {
        stream << generatePopulationFitHtml(popResult);
    }

    stream << generateFooterHtml();

    stream << "</body>\n</html>\n";

    return html;
}

QString ReportGenerator::generateTextReport(
    const SpectrumData &spectrum,
    const QList<SpectralLibraryMatcher::MatchResult> &matches,
    const StellarPopulationFitter::PopulationResult &popResult,
    bool includePop)
{
    QString text;
    QTextStream stream(&text);

    stream << "============================================================\n";
    stream << "  " << m_config.title << "\n";
    stream << "============================================================\n\n";

    stream << "Generated: " << m_config.date << "\n";
    stream << "Author: " << m_config.author << "\n\n";

    stream << "------------------------------------------------------------\n";
    stream << "SPECTRUM INFORMATION\n";
    stream << "------------------------------------------------------------\n";

    stream << QString("  Name:        %1\n").arg(spectrum.name());
    stream << QString("  Object:      %1\n").arg(spectrum.objectName());
    stream << QString("  File:        %1\n").arg(spectrum.filePath());
    stream << QString("  Data points: %1\n").arg(spectrum.size());
    stream << QString("  Calibrated:  %1\n").arg(spectrum.isCalibrated() ? "Yes" : "No");
    stream << QString("  Redshift:    %1\n").arg(spectrum.redshift(), 0, 'f', 5);
    stream << QString("  Wavelength:  %1 - %2 Å\n")
              .arg(spectrum.minWavelength(), 0, 'f', 2)
              .arg(spectrum.maxWavelength(), 0, 'f', 2);
    stream << QString("  Flux range:  %1 - %2\n")
              .arg(spectrum.minFlux(), 0, 'g', 4)
              .arg(spectrum.maxFlux(), 0, 'g', 4);
    stream << "\n";

    if (m_config.includeLineTable && !spectrum.spectralLines().isEmpty()) {
        stream << "------------------------------------------------------------\n";
        stream << "SPECTRAL LINES\n";
        stream << "------------------------------------------------------------\n\n";

        QVector<SpectralLine> lines = spectrum.spectralLines();
        stream << QString("Total lines detected: %1\n\n").arg(lines.size());

        stream << QString("%1  %2  %3  %4  %5\n")
                  .arg("Wavelength", -12)
                  .arg("FWHM", -10)
                  .arg("Height", -12)
                  .arg("EW", -10)
                  .arg("Type", -12);
        stream << QString("-" * 60) << "\n";

        for (int i = 0; i < qMin(20, lines.size()); ++i) {
            const SpectralLine &line = lines[i];
            stream << QString("%1  %2  %3  %4  %5\n")
                      .arg(line.wavelength, 10, 'f', 2)
                      .arg(line.fwhm, 8, 'f', 3)
                      .arg(line.peakHeight, 10, 'g', 4)
                      .arg(line.equivalentWidth, 8, 'f', 2)
                      .arg(line.isEmission ? "Emission" : "Absorption", -10);
        }

        if (lines.size() > 20) {
            stream << QString("... and %1 more lines\n").arg(lines.size() - 20);
        }
        stream << "\n";
    }

    if (m_config.includeRedshift && spectrum.redshift() != 0.0) {
        stream << "------------------------------------------------------------\n";
        stream << "REDSHIFT ANALYSIS\n";
        stream << "------------------------------------------------------------\n\n";

        stream << QString("  Redshift z:      %1\n").arg(spectrum.redshift(), 0, 'f', 5);
        stream << QString("  Velocity (km/s): %1\n")
                  .arg(spectrum.redshift() * 299792.458, 0, 'f', 2);
        stream << "\n";
    }

    if (m_config.includeLibraryMatch && !matches.isEmpty()) {
        stream << "------------------------------------------------------------\n";
        stream << "SPECTRAL LIBRARY MATCHING\n";
        stream << "------------------------------------------------------------\n\n";

        stream << "Top matches:\n\n";
        for (int i = 0; i < matches.size(); ++i) {
            const SpectralLibraryMatcher::MatchResult &m = matches[i];
            stream << QString("  %1. %2 (score: %3)\n")
                      .arg(i + 1)
                      .arg(m.templateName, -20)
                      .arg(m.matchScore, 0, 'f', 4);
        }
        stream << "\n";
    }

    if (includePop && m_config.includePopulationFit) {
        stream << "------------------------------------------------------------\n";
        stream << "STELLAR POPULATION ANALYSIS\n";
        stream << "------------------------------------------------------------\n\n";

        stream << QString("  Mean age:            %1\n")
                  .arg(StellarPopulationFitter::ageToString(popResult.meanAge));
        stream << QString("  Mass-weighted age:   %1\n")
                  .arg(StellarPopulationFitter::ageToString(popResult.massWeightedAge));
        stream << QString("  Mean metallicity:    %1\n").arg(popResult.meanMetallicity, 0, 'f', 2);
        stream << QString("  Total mass:          %1 M☉\n").arg(popResult.totalMass, 0, 'e', 2);
        stream << QString("  Reduced χ²:          %1\n").arg(popResult.reducedChiSquared, 0, 'g', 4);
        stream << "\n";
    }

    stream << "============================================================\n";
    stream << "  End of Report\n";
    stream << "============================================================\n";

    return text;
}

QString ReportGenerator::generateHeaderHtml()
{
    QString html;
    QTextStream stream(&html);

    stream << "<h1>" << m_config.title << "</h1>\n";
    stream << "<p class=\"meta\">\n";
    stream << "<strong>Generated:</strong> " << m_config.date << " | ";
    stream << "<strong>Author:</strong> " << m_config.author;
    stream << "</p>\n";

    return html;
}

QString ReportGenerator::generateSpectrumInfoHtml(const SpectrumData &spectrum)
{
    QString html;
    QTextStream stream(&html);

    stream << "<h2>Spectrum Information</h2>\n";
    stream << "<div class=\"info-box\">\n";

    stream << "<table>\n";
    stream << "<tr><th>Property</th><th>Value</th></tr>\n";
    stream << "<tr><td>Name</td><td>" << spectrum.name() << "</td></tr>\n";
    stream << "<tr><td>Object</td><td>" << spectrum.objectName() << "</td></tr>\n";
    stream << "<tr><td>File</td><td>" << spectrum.filePath() << "</td></tr>\n";
    stream << "<tr><td>Data Points</td><td class=\"value\">" << spectrum.size() << "</td></tr>\n";
    stream << "<tr><td>Calibrated</td><td>"
           << (spectrum.isCalibrated() ? "<span style='color:green'>✓ Yes</span>" : "<span style='color:red'>✗ No</span>")
           << "</td></tr>\n";
    stream << "<tr><td>Wavelength Range</td><td>"
           << QString("%1 - %2 Å").arg(spectrum.minWavelength(), 0, 'f', 2).arg(spectrum.maxWavelength(), 0, 'f', 2)
           << "</td></tr>\n";
    stream << "<tr><td>Flux Range</td><td>"
           << QString("%1 - %2").arg(spectrum.minFlux(), 0, 'g', 4).arg(spectrum.maxFlux(), 0, 'g', 4)
           << "</td></tr>\n";
    stream << "<tr><td>Redshift</td><td class=\"value\">"
           << QString("z = %1").arg(spectrum.redshift(), 0, 'f', 5)
           << "</td></tr>\n";
    stream << "</table>\n";

    stream << "</div>\n";

    return html;
}

QString ReportGenerator::generateSpectralLinesHtml(const SpectrumData &spectrum)
{
    QString html;
    QTextStream stream(&html);

    QVector<SpectralLine> lines = spectrum.spectralLines();

    stream << "<h2>Spectral Lines</h2>\n";

    if (lines.isEmpty()) {
        stream << "<div class=\"warning-box\">\n";
        stream << "<p><strong>No spectral lines detected.</strong></p>\n";
        stream << "<p>Use the line detection tool to identify spectral lines.</p>\n";
        stream << "</div>\n";
        return html;
    }

    stream << QString("<p><strong>Total lines detected:</strong> %1</p>\n").arg(lines.size());

    stream << "<table>\n";
    stream << "<tr><th>#</th><th>Name</th><th>Wavelength (Å)</th><th>FWHM (Å)</th>"
           << "<th>Peak Height</th><th>EW (Å)</th><th>Type</th></tr>\n";

    for (int i = 0; i < qMin(50, lines.size()); ++i) {
        const SpectralLine &line = lines[i];
        QString typeClass = line.isEmission ? "background:#fde8e8" : "background:#e8fde8";
        QString typeText = line.isEmission ? "Emission" : "Absorption";

        stream << "<tr style=\"" << typeClass << "\">\n";
        stream << "<td>" << (i + 1) << "</td>\n";
        stream << "<td>" << line.name << "</td>\n";
        stream << "<td class=\"value\">" << QString::number(line.wavelength, 'f', 3) << "</td>\n";
        stream << "<td>" << QString::number(line.fwhm, 'f', 3) << "</td>\n";
        stream << "<td>" << QString::number(line.peakHeight, 'g', 4) << "</td>\n";
        stream << "<td>" << QString::number(line.equivalentWidth, 'f', 3) << "</td>\n";
        stream << "<td>" << typeText << "</td>\n";
        stream << "</tr>\n";
    }

    stream << "</table>\n";

    if (lines.size() > 50) {
        stream << QString("<p>... and %1 more lines (total: %2)</p>\n")
                  .arg(lines.size() - 50).arg(lines.size());
    }

    return html;
}

QString ReportGenerator::generateRedshiftHtml(const SpectrumData &spectrum)
{
    QString html;
    QTextStream stream(&html);

    stream << "<h2>Redshift Analysis</h2>\n";
    stream << "<div class=\"success-box\">\n";

    double z = spectrum.redshift();
    double v = z * 299792.458;
    double distMpc = z * 3000.0 / 70.0;

    stream << "<table>\n";
    stream << "<tr><th>Parameter</th><th>Value</th></tr>\n";
    stream << "<tr><td>Redshift z</td><td class=\"value\">"
           << QString::number(z, 'f', 5) << "</td></tr>\n";
    stream << "<tr><td>Radial Velocity</td><td>"
           << QString("%1 km/s").arg(v, 0, 'f', 2) << "</td></tr>\n";
    stream << "<tr><td>Distance (approx.)</td><td>"
           << QString("%1 Mpc").arg(distMpc, 0, 'f', 1) << "</td></tr>\n";
    stream << "</table>\n";

    stream << "</div>\n";

    return html;
}

QString ReportGenerator::generateLibraryMatchHtml(
    const QList<SpectralLibraryMatcher::MatchResult> &matches)
{
    QString html;
    QTextStream stream(&html);

    stream << "<h2>Spectral Library Matching</h2>\n";

    if (matches.isEmpty()) {
        stream << "<div class=\"warning-box\">\n";
        stream << "<p><strong>No matching results.</strong></p>\n";
        stream << "</div>\n";
        return html;
    }

    stream << "<table>\n";
    stream << "<tr><th>Rank</th><th>Template</th><th>Object Type</th>"
           << "<th>Match Score</th><th>Redshift</th></tr>\n";

    for (int i = 0; i < matches.size(); ++i) {
        const SpectralLibraryMatcher::MatchResult &m = matches[i];
        QString typeName = SpectralLibraryMatcher::objectTypeName(m.objectType);

        stream << "<tr>\n";
        stream << "<td class=\"value\">" << m.rank << "</td>\n";
        stream << "<td>" << m.templateName << "</td>\n";
        stream << "<td>" << typeName << "</td>\n";
        stream << "<td class=\"value\">" << QString::number(m.matchScore, 'f', 4) << "</td>\n";
        stream << "<td>" << QString("z = %1").arg(m.redshift, 0, 'f', 4) << "</td>\n";
        stream << "</tr>\n";
    }

    stream << "</table>\n";

    if (!matches.isEmpty()) {
        const SpectralLibraryMatcher::MatchResult &best = matches.first();
        QString description = SpectralLibraryMatcher::objectTypeDescription(best.objectType);

        stream << "<div class=\"info-box\">\n";
        stream << "<p><strong>Best Match:</strong> " << best.templateName << "</p>\n";
        stream << "<p><strong>Object Type:</strong> "
               << SpectralLibraryMatcher::objectTypeName(best.objectType) << "</p>\n";
        stream << "<p><strong>Description:</strong> " << description << "</p>\n";
        stream << "</div>\n";
    }

    return html;
}

QString ReportGenerator::generatePopulationFitHtml(
    const StellarPopulationFitter::PopulationResult &result)
{
    QString html;
    QTextStream stream(&html);

    stream << "<h2>Stellar Population Analysis</h2>\n";

    if (result.totalMass <= 0) {
        stream << "<div class=\"warning-box\">\n";
        stream << "<p><strong>Population fit not available.</strong></p>\n";
        stream << "</div>\n";
        return html;
    }

    stream << "<div class=\"info-box\">\n";
    stream << "<table>\n";
    stream << "<tr><th>Parameter</th><th>Value</th></tr>\n";
    stream << "<tr><td>Mean Age</td><td class=\"value\">"
           << StellarPopulationFitter::ageToString(result.meanAge) << "</td></tr>\n";
    stream << "<tr><td>Mass-Weighted Age</td><td>"
           << StellarPopulationFitter::ageToString(result.massWeightedAge) << "</td></tr>\n";
    stream << "<tr><td>Mean Metallicity</td><td>"
           << QString("[Fe/H] = %1").arg(result.meanMetallicity, 0, 'f', 2) << "</td></tr>\n";
    stream << "<tr><td>Total Stellar Mass</td><td>"
           << QString("%1 M☉").arg(result.totalMass, 0, 'e', 2) << "</td></tr>\n";
    stream << "<tr><td>Age Spread</td><td>"
           << StellarPopulationFitter::ageToString(result.ageSpread) << "</td></tr>\n";
    stream << "<tr><td>Reduced χ²</td><td>"
           << QString::number(result.reducedChiSquared, 'g', 4) << "</td></tr>\n";
    stream << "</table>\n";
    stream << "</div>\n";

    return html;
}

QString ReportGenerator::generateFooterHtml()
{
    QString html;
    QTextStream stream(&html);

    stream << "<hr style=\"margin-top: 40px; border: 0; border-top: 1px solid #ddd;\">\n";
    stream << "<p style=\"color: #999; font-size: 0.8em; text-align: center;\">\n";
    stream << "Generated by Astro Spectrum Analyzer - Astronomical Spectral Data Processing Tool\n";
    stream << "</p>\n";

    return html;
}

QPixmap ReportGenerator::generateSpectrumPlot(const SpectrumData &spectrum,
                                                 int width, int height)
{
    QPixmap pixmap(width, height);
    pixmap.fill(QColor(30, 30, 35));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!spectrum.isValid()) {
        painter.setPen(Qt::white);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "No spectrum data");
        return pixmap;
    }

    int leftMargin = 60;
    int rightMargin = 20;
    int topMargin = 30;
    int bottomMargin = 40;
    int plotWidth = width - leftMargin - rightMargin;
    int plotHeight = height - topMargin - bottomMargin;

    QVector<double> wl = spectrum.wavelengths();
    QVector<double> flux = spectrum.flux();

    if (wl.size() < 2) {
        return pixmap;
    }

    double xMin = wl.first();
    double xMax = wl.last();
    double yMin = 1e30;
    double yMax = -1e30;

    for (double f : flux) {
        if (f < yMin) yMin = f;
        if (f > yMax) yMax = f;
    }

    if (yMin == yMax) {
        yMin -= 1;
        yMax += 1;
    }

    double yPad = (yMax - yMin) * 0.1;
    yMin -= yPad;
    yMax += yPad;

    painter.fillRect(leftMargin, topMargin, plotWidth, plotHeight, QColor(50, 50, 55));

    painter.setPen(QPen(QColor(80, 80, 90), 1, Qt::DashLine));
    int numDivisions = 8;
    for (int i = 0; i <= numDivisions; ++i) {
        double x = leftMargin + static_cast<double>(i) / numDivisions * plotWidth;
        painter.drawLine(QPointF(x, topMargin), QPointF(x, height - bottomMargin));
    }
    for (int i = 0; i <= 6; ++i) {
        double y = topMargin + static_cast<double>(i) / 6.0 * plotHeight;
        painter.drawLine(QPointF(leftMargin, y), QPointF(width - rightMargin, y));
    }

    QPainterPath path;
    bool firstPoint = true;

    for (int i = 0; i < wl.size(); ++i) {
        double xNorm = (wl[i] - xMin) / (xMax - xMin);
        double yNorm = (flux[i] - yMin) / (yMax - yMin);

        double x = leftMargin + xNorm * plotWidth;
        double y = height - bottomMargin - yNorm * plotHeight;

        if (firstPoint) {
            path.moveTo(x, y);
            firstPoint = false;
        } else {
            path.lineTo(x, y);
        }
    }

    painter.setPen(QPen(QColor(100, 180, 255), 1.5));
    painter.drawPath(path);

    if (!firstPoint) {
        QPainterPath fillPath = path;
        fillPath.lineTo(width - rightMargin, height - bottomMargin);
        fillPath.lineTo(leftMargin, height - bottomMargin);
        fillPath.closeSubpath();

        QLinearGradient gradient(0, topMargin, 0, height - bottomMargin);
        gradient.setColorAt(0.0, QColor(100, 180, 255, 80));
        gradient.setColorAt(1.0, QColor(100, 180, 255, 10));
        painter.fillPath(fillPath, gradient);
    }

    painter.setPen(QPen(QColor(200, 200, 200), 2));
    painter.drawLine(leftMargin, topMargin, leftMargin, height - bottomMargin);
    painter.drawLine(leftMargin, height - bottomMargin, width - rightMargin, height - bottomMargin);

    painter.setPen(QColor(220, 220, 220));
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    for (int i = 0; i <= numDivisions; ++i) {
        double x = leftMargin + static_cast<double>(i) / numDivisions * plotWidth;
        double value = xMin + static_cast<double>(i) / numDivisions * (xMax - xMin);
        QString label = QString::number(value, 'f', 0);
        QFontMetrics fm(font);
        painter.drawText(QPointF(x - fm.horizontalAdvance(label) / 2.0,
                                  height - bottomMargin + 18), label);
    }

    for (int i = 0; i <= 6; ++i) {
        double y = topMargin + static_cast<double>(i) / 6.0 * plotHeight;
        double value = yMax - static_cast<double>(i) / 6.0 * (yMax - yMin);
        QString label = QString::number(value, 'g', 3);
        QFontMetrics fm(font);
        painter.drawText(QPointF(leftMargin - fm.horizontalAdvance(label) - 8,
                                  y + 4), label);
    }

    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    QFontMetrics fmTitle(font);
    painter.drawText(QPointF((width - fmTitle.horizontalAdvance(spectrum.name())) / 2.0, 20),
                      spectrum.name());

    return pixmap;
}

bool ReportGenerator::savePdf(const QString &html, const QString &outputPath)
{
    QTextDocument document;
    document.setHtml(html);

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(outputPath);
    printer.setPageSize(QPrinter::A4);
    printer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);

    document.print(&printer);

    QFileInfo info(outputPath);
    return info.exists() && info.size() > 0;
}

bool ReportGenerator::saveHtml(const QString &html, const QString &outputPath)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << html;
    file.close();

    return true;
}

bool ReportGenerator::saveText(const QString &text, const QString &outputPath)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << text;
    file.close();

    return true;
}
