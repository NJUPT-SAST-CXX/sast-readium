#include <QApplication>
#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include "../app/ui/viewer/PDFViewer.h"

class QGraphicsPDFDemo : public QMainWindow {
    Q_OBJECT

public:
    QGraphicsPDFDemo(QWidget* parent = nullptr)
        : QMainWindow(parent), m_pdfViewer(nullptr), m_document(nullptr) {
        setupUI();
        connectSignals();
        updateControls();
    }

    ~QGraphicsPDFDemo() {
        if (m_document) {
            delete m_document;
        }
    }

private slots:
    void openPDF() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open PDF", "",
                                                        "PDF Files (*.pdf)");

        if (!fileName.isEmpty()) {
            loadPDF(fileName);
        }
    }

    void toggleQGraphicsMode() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        bool enabled = m_qgraphicsCheck->isChecked();
        m_pdfViewer->setQGraphicsRenderingEnabled(enabled);

        QString mode = enabled ? "QGraphics Enhanced" : "Traditional";
        m_statusLabel->setText(QString("Rendering Mode: %1").arg(mode));

        // Enable/disable QGraphics-specific controls
        m_highQualityCheck->setEnabled(enabled);

        if (enabled && m_highQualityCheck->isChecked()) {
            m_pdfViewer->setQGraphicsHighQualityRendering(true);
        }
#endif
    }

    void toggleHighQuality() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        if (m_pdfViewer->isQGraphicsRenderingEnabled()) {
            m_pdfViewer->setQGraphicsHighQualityRendering(
                m_highQualityCheck->isChecked());
        }
#endif
    }

private:
    void setupUI() {
        setWindowTitle("QGraphics PDF Demo");
        setMinimumSize(800, 600);

        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

        // Create splitter for resizable layout
        QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

        // PDF Viewer
        m_pdfViewer = new PDFViewer(this);
        splitter->addWidget(m_pdfViewer);

        // Control Panel
        QWidget* controlPanel = new QWidget(this);
        controlPanel->setMaximumWidth(250);
        controlPanel->setMinimumWidth(200);

        QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);

        // File operations
        QPushButton* openButton = new QPushButton("Open PDF", this);
        controlLayout->addWidget(openButton);
        connect(openButton, &QPushButton::clicked, this,
                &QGraphicsPDFDemo::openPDF);

        controlLayout->addWidget(new QLabel("Rendering Options:", this));

        // QGraphics toggle
        m_qgraphicsCheck = new QCheckBox("Enable QGraphics Rendering", this);
        controlLayout->addWidget(m_qgraphicsCheck);

        // High quality toggle
        m_highQualityCheck = new QCheckBox("High Quality Rendering", this);
        m_highQualityCheck->setChecked(true);
        controlLayout->addWidget(m_highQualityCheck);

        controlLayout->addStretch();

        // Status
        m_statusLabel = new QLabel("Status: Ready", this);
        m_statusLabel->setWordWrap(true);
        controlLayout->addWidget(m_statusLabel);

        // Build info
        QLabel* buildInfo = new QLabel(this);
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        buildInfo->setText("Build: QGraphics support ENABLED");
        buildInfo->setStyleSheet("color: green; font-weight: bold;");
#else
        buildInfo->setText("Build: QGraphics support DISABLED");
        buildInfo->setStyleSheet("color: orange; font-weight: bold;");
#endif
        controlLayout->addWidget(buildInfo);

        splitter->addWidget(controlPanel);
        splitter->setSizes({600, 200});

        mainLayout->addWidget(splitter);
    }

    void connectSignals() {
        connect(m_qgraphicsCheck, &QCheckBox::toggled, this,
                &QGraphicsPDFDemo::toggleQGraphicsMode);
        connect(m_highQualityCheck, &QCheckBox::toggled, this,
                &QGraphicsPDFDemo::toggleHighQuality);
    }

    void updateControls() {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        m_qgraphicsCheck->setEnabled(true);
        m_qgraphicsCheck->setToolTip(
            "Toggle between QGraphics and traditional rendering");
#else
        m_qgraphicsCheck->setEnabled(false);
        m_qgraphicsCheck->setToolTip("QGraphics support not compiled in");
        m_highQualityCheck->setEnabled(false);
#endif

        m_statusLabel->setText("Rendering Mode: Traditional");
    }

    void loadPDF(const QString& fileName) {
        if (m_document) {
            delete m_document;
            m_document = nullptr;
        }

        m_document = Poppler::Document::load(fileName);

        if (!m_document) {
            QMessageBox::warning(this, "Error", "Failed to load PDF file");
            return;
        }

        if (m_document->isLocked()) {
            QMessageBox::warning(this, "Error",
                                 "PDF file is password protected");
            delete m_document;
            m_document = nullptr;
            return;
        }

        // Configure document for optimal rendering
        m_document->setRenderHint(Poppler::Document::Antialiasing, true);
        m_document->setRenderHint(Poppler::Document::TextAntialiasing, true);

        m_pdfViewer->setDocument(m_document);

        m_statusLabel->setText(QString("Loaded: %1 (%2 pages)")
                                   .arg(QFileInfo(fileName).baseName())
                                   .arg(m_document->numPages()));
    }

private:
    PDFViewer* m_pdfViewer;
    Poppler::Document* m_document;

    QCheckBox* m_qgraphicsCheck;
    QCheckBox* m_highQualityCheck;
    QLabel* m_statusLabel;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QGraphicsPDFDemo demo;
    demo.show();

    return app.exec();
}

#include "qgraphics_pdf_demo.moc"
