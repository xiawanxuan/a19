cmake_minimum_required(VERSION 3.16)
project(AstroSpectrumAnalyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(BUILD_TESTS "Build tests" OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Charts PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Charts PrintSupport)

set(SOURCES
    src/main.cpp
    src/data_parser/SpectrumData.cpp
    src/data_parser/DataParser.cpp
    src/spectrum_processing/WavelengthCalibrator.cpp
    src/spectrum_processing/SpectralLineFitter.cpp
    src/spectrum_processing/RedshiftCalculator.cpp
    src/spectrum_processing/SpectrumProcessor.cpp
    src/visualization/SpectrumChartView.cpp
    src/visualization/SpectrumPlot.cpp
    src/gui/MainWindow.cpp
    src/gui/DataImportDialog.cpp
    src/gui/CalibrationDialog.cpp
    src/gui/LineDetectionDialog.cpp
    src/gui/ExportDialog.cpp
    src/gui/SpectralLineTableModel.cpp
    src/gui/SpectrumListModel.cpp
)

set(HEADERS
    src/data_parser/SpectrumData.h
    src/data_parser/DataParser.h
    src/spectrum_processing/WavelengthCalibrator.h
    src/spectrum_processing/SpectralLineFitter.h
    src/spectrum_processing/RedshiftCalculator.h
    src/spectrum_processing/SpectrumProcessor.h
    src/visualization/SpectrumChartView.h
    src/visualization/SpectrumPlot.h
    src/gui/MainWindow.h
    src/gui/DataImportDialog.h
    src/gui/CalibrationDialog.h
    src/gui/LineDetectionDialog.h
    src/gui/ExportDialog.h
    src/gui/SpectralLineTableModel.h
    src/gui/SpectrumListModel.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(AstroSpectrumAnalyzer
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_include_directories(AstroSpectrumAnalyzer PRIVATE
    src
)

target_link_libraries(AstroSpectrumAnalyzer PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

if(WIN32)
    set_target_properties(AstroSpectrumAnalyzer PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

if(APPLE)
    set_target_properties(AstroSpectrumAnalyzer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Astro Spectrum Analyzer"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
