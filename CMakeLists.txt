cmake_minimum_required(VERSION 3.20)
project(LocalPDF VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Pdf PdfWidgets)

set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/DocumentView.cpp
    src/ui/ThumbnailPanel.cpp
    src/ui/PropertyPanel.cpp
    src/document/Document.cpp
    src/document/Page.cpp
    src/document/ProjectSerializer.cpp
    src/editor/OverlayItem.cpp
    src/editor/TextItem.cpp
    src/editor/ImageItem.cpp
    src/editor/PenItem.cpp
    src/editor/HighlightItem.cpp
    src/editor/RectangleItem.cpp
    src/pdf/PdfBackend.cpp
    src/pdf/PdfRenderer.cpp
    src/pdf/PdfExporter.cpp
    src/pdf/PdfCompressor.cpp
)

set(HEADERS
    src/ui/MainWindow.h
    src/ui/DocumentView.h
    src/ui/ThumbnailPanel.h
    src/ui/PropertyPanel.h
    src/document/Document.h
    src/document/Page.h
    src/document/ProjectSerializer.h
    src/editor/OverlayItem.h
    src/editor/TextItem.h
    src/editor/ImageItem.h
    src/editor/PenItem.h
    src/editor/HighlightItem.h
    src/editor/RectangleItem.h
    src/pdf/PdfBackend.h
    src/pdf/PdfRenderer.h
    src/pdf/PdfExporter.h
    src/pdf/PdfCompressor.h
)

set(RESOURCES
    resources/app.rc
    resources/resources.qrc
)

add_executable(LocalPDF WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(LocalPDF PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Pdf
    Qt6::PdfWidgets
)

target_include_directories(LocalPDF PRIVATE src)
