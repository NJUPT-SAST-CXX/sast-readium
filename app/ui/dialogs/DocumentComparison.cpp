#include "DocumentComparison.h"
#include <QApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPainter>
#include <QSplitter>
#include <QTextStream>
// // #include <QtConcurrent> // Not available in this MSYS2 setup // Not
// available in this MSYS2 setup
#include <QDebug>

DocumentComparison::DocumentComparison(QWidget* parent)
    : QWidget(parent),
      m_document1(nullptr),
      m_document2(nullptr),
      m_currentDifferenceIndex(-1),
      m_isComparing(false),
      m_comparisonWatcher(nullptr),
      m_progressTimer(nullptr) {
    setupUI();
    setupConnections();
}

void DocumentComparison::setupUI() {
    m_mainLayout = new QVBoxLayout(this);

    // Toolbar
    m_toolbarLayout = new QHBoxLayout();
    m_compareButton = new QPushButton("Compare Documents", this);
    m_stopButton = new QPushButton("Stop", this);
    m_stopButton->setEnabled(false);
    m_optionsButton = new QPushButton("Options", this);
    m_exportButton = new QPushButton("Export Results", this);
    m_exportButton->setEnabled(false);

    m_viewModeCombo = new QComboBox(this);
    m_viewModeCombo->addItems({"Side by Side", "Overlay", "Differences Only"});

    m_statusLabel = new QLabel("Ready", this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);

    m_toolbarLayout->addWidget(m_compareButton);
    m_toolbarLayout->addWidget(m_stopButton);
    m_toolbarLayout->addWidget(m_optionsButton);
    m_toolbarLayout->addWidget(m_exportButton);
    m_toolbarLayout->addWidget(new QLabel("View Mode:"));
    m_toolbarLayout->addWidget(m_viewModeCombo);
    m_toolbarLayout->addStretch();
    m_toolbarLayout->addWidget(m_statusLabel);
    m_toolbarLayout->addWidget(m_progressBar);

    m_mainLayout->addLayout(m_toolbarLayout);

    // Options panel (initially hidden)
    m_optionsGroup = new QGroupBox("Comparison Options", this);
    m_optionsGroup->setVisible(false);

    QGridLayout* optionsLayout = new QGridLayout(m_optionsGroup);

    m_compareTextCheck = new QCheckBox("Compare Text", this);
    m_compareTextCheck->setChecked(true);
    m_compareImagesCheck = new QCheckBox("Compare Images", this);
    m_compareImagesCheck->setChecked(true);
    m_compareLayoutCheck = new QCheckBox("Compare Layout", this);
    m_compareAnnotationsCheck = new QCheckBox("Compare Annotations", this);
    m_compareAnnotationsCheck->setChecked(true);

    m_ignoreWhitespaceCheck = new QCheckBox("Ignore Whitespace", this);
    m_ignoreWhitespaceCheck->setChecked(true);
    m_ignoreCaseCheck = new QCheckBox("Ignore Case", this);

    m_similaritySlider = new QSlider(Qt::Horizontal, this);
    m_similaritySlider->setRange(50, 100);
    m_similaritySlider->setValue(90);

    m_maxDifferencesSpinBox = new QSpinBox(this);
    m_maxDifferencesSpinBox->setRange(1, 1000);
    m_maxDifferencesSpinBox->setValue(50);

    optionsLayout->addWidget(m_compareTextCheck, 0, 0);
    optionsLayout->addWidget(m_compareImagesCheck, 0, 1);
    optionsLayout->addWidget(m_compareLayoutCheck, 1, 0);
    optionsLayout->addWidget(m_compareAnnotationsCheck, 1, 1);
    optionsLayout->addWidget(m_ignoreWhitespaceCheck, 2, 0);
    optionsLayout->addWidget(m_ignoreCaseCheck, 2, 1);
    optionsLayout->addWidget(new QLabel("Similarity Threshold:"), 3, 0);
    optionsLayout->addWidget(m_similaritySlider, 3, 1);
    optionsLayout->addWidget(new QLabel("Max Differences:"), 4, 0);
    optionsLayout->addWidget(m_maxDifferencesSpinBox, 4, 1);

    m_mainLayout->addWidget(m_optionsGroup);

    // Content area
    m_contentLayout = new QHBoxLayout();

    // Results panel
    m_resultsSplitter = new QSplitter(Qt::Vertical, this);

    m_differencesTree = new QTreeWidget(this);
    m_differencesTree->setHeaderLabels(
        {"Type", "Page", "Description", "Confidence"});
    m_differencesTree->header()->setStretchLastSection(true);

    m_differenceDetails = new QTextEdit(this);
    m_differenceDetails->setReadOnly(true);
    m_differenceDetails->setMaximumHeight(150);

    m_resultsSplitter->addWidget(m_differencesTree);
    m_resultsSplitter->addWidget(m_differenceDetails);
    m_resultsSplitter->setSizes({300, 150});

    // Comparison view
    m_viewSplitter = new QSplitter(Qt::Horizontal, this);

    m_leftView = new QScrollArea(this);
    m_rightView = new QScrollArea(this);
    m_leftImageLabel = new QLabel("Document 1", this);
    m_rightImageLabel = new QLabel("Document 2", this);

    m_leftImageLabel->setAlignment(Qt::AlignCenter);
    m_rightImageLabel->setAlignment(Qt::AlignCenter);
    m_leftImageLabel->setStyleSheet(
        "border: 1px solid gray; background: white;");
    m_rightImageLabel->setStyleSheet(
        "border: 1px solid gray; background: white;");

    m_leftView->setWidget(m_leftImageLabel);
    m_rightView->setWidget(m_rightImageLabel);

    m_viewSplitter->addWidget(m_leftView);
    m_viewSplitter->addWidget(m_rightView);
    m_viewSplitter->setSizes({400, 400});

    // Add to content layout
    m_contentLayout->addWidget(m_resultsSplitter, 1);
    m_contentLayout->addWidget(m_viewSplitter, 2);

    m_mainLayout->addLayout(m_contentLayout);

    // Initialize progress timer
    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(100);

    // Initialize comparison watcher
    m_comparisonWatcher = new QFutureWatcher<ComparisonResults>(this);
}

void DocumentComparison::setupConnections() {
    connect(m_compareButton, &QPushButton::clicked, this,
            &DocumentComparison::startComparison);
    connect(m_stopButton, &QPushButton::clicked, this,
            &DocumentComparison::stopComparison);
    connect(m_optionsButton, &QPushButton::clicked, [this]() {
        m_optionsGroup->setVisible(!m_optionsGroup->isVisible());
    });
    connect(m_exportButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(
            this, "Export Results", "", "JSON Files (*.json)");
        if (!fileName.isEmpty()) {
            exportResultsToFile(fileName);
        }
    });

    connect(m_viewModeCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DocumentComparison::onViewModeChanged);

    connect(m_differencesTree, &QTreeWidget::itemClicked, this,
            &DocumentComparison::onDifferenceClicked);

    connect(m_comparisonWatcher, &QFutureWatcher<ComparisonResults>::finished,
            this, &DocumentComparison::onComparisonFinished);

    connect(m_progressTimer, &QTimer::timeout, this,
            &DocumentComparison::updateProgress);

    // Options change connections
    connect(m_compareTextCheck, &QCheckBox::toggled, this,
            &DocumentComparison::onOptionsChanged);
    connect(m_compareImagesCheck, &QCheckBox::toggled, this,
            &DocumentComparison::onOptionsChanged);
    connect(m_compareLayoutCheck, &QCheckBox::toggled, this,
            &DocumentComparison::onOptionsChanged);
    connect(m_compareAnnotationsCheck, &QCheckBox::toggled, this,
            &DocumentComparison::onOptionsChanged);
    connect(m_ignoreWhitespaceCheck, &QCheckBox::toggled, this,
            &DocumentComparison::onOptionsChanged);
    connect(m_ignoreCaseCheck, &QCheckBox::toggled, this,
            &DocumentComparison::onOptionsChanged);
    connect(m_similaritySlider, &QSlider::valueChanged, this,
            &DocumentComparison::onOptionsChanged);
    connect(m_maxDifferencesSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged), this,
            &DocumentComparison::onOptionsChanged);
}

void DocumentComparison::setDocuments(Poppler::Document* doc1,
                                      Poppler::Document* doc2) {
    m_document1 = doc1;
    m_document2 = doc2;

    if (doc1 && doc2) {
        m_compareButton->setEnabled(true);
        m_statusLabel->setText(QString("Ready to compare %1 vs %2 pages")
                                   .arg(doc1->numPages())
                                   .arg(doc2->numPages()));
    } else {
        m_compareButton->setEnabled(false);
        m_statusLabel->setText("No documents loaded");
    }
}

void DocumentComparison::setDocumentPaths(const QString& path1,
                                          const QString& path2) {
    m_documentPath1 = path1;
    m_documentPath2 = path2;
}

void DocumentComparison::startComparison() {
    if (!m_document1 || !m_document2) {
        QMessageBox::warning(this, "Warning",
                             "Please load both documents first.");
        return;
    }

    if (m_isComparing) {
        return;
    }

    m_isComparing = true;
    m_compareButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_exportButton->setEnabled(false);

    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Starting comparison...");

    m_progressTimer->start();

    // Start comparison in background thread (simplified without QtConcurrent)
    // For now, run synchronously - could be improved with QThread later
    ComparisonResults results = compareDocuments();
    m_results = results;
    QTimer::singleShot(0, this, &DocumentComparison::onComparisonFinished);

    emit comparisonStarted();
}

void DocumentComparison::stopComparison() {
    if (m_comparisonWatcher && m_comparisonWatcher->isRunning()) {
        m_comparisonWatcher->cancel();
    }

    m_isComparing = false;
    m_compareButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_progressBar->setVisible(false);
    m_progressTimer->stop();
    m_statusLabel->setText("Comparison stopped");
}

ComparisonOptions DocumentComparison::getComparisonOptions() const {
    ComparisonOptions options;
    options.compareText = m_compareTextCheck->isChecked();
    options.compareImages = m_compareImagesCheck->isChecked();
    options.compareLayout = m_compareLayoutCheck->isChecked();
    options.compareAnnotations = m_compareAnnotationsCheck->isChecked();
    options.ignoreWhitespace = m_ignoreWhitespaceCheck->isChecked();
    options.ignoreCaseChanges = m_ignoreCaseCheck->isChecked();
    options.textSimilarityThreshold = m_similaritySlider->value() / 100.0;
    options.imageSimilarityThreshold = m_similaritySlider->value() / 100.0;
    options.maxDifferencesPerPage = m_maxDifferencesSpinBox->value();
    return options;
}

void DocumentComparison::setComparisonOptions(
    const ComparisonOptions& options) {
    m_compareTextCheck->setChecked(options.compareText);
    m_compareImagesCheck->setChecked(options.compareImages);
    m_compareLayoutCheck->setChecked(options.compareLayout);
    m_compareAnnotationsCheck->setChecked(options.compareAnnotations);
    m_ignoreWhitespaceCheck->setChecked(options.ignoreWhitespace);
    m_ignoreCaseCheck->setChecked(options.ignoreCaseChanges);
    m_similaritySlider->setValue(
        static_cast<int>(options.textSimilarityThreshold * 100));
    m_maxDifferencesSpinBox->setValue(options.maxDifferencesPerPage);
    m_options = options;
}

ComparisonResults DocumentComparison::compareDocuments() {
    ComparisonResults results;

    if (!m_document1 || !m_document2) {
        return results;
    }

    results.totalPages1 = m_document1->numPages();
    results.totalPages2 = m_document2->numPages();
    results.pagesCompared = qMin(results.totalPages1, results.totalPages2);

    QElapsedTimer timer;
    timer.start();

    m_options = getComparisonOptions();

    // Compare pages
    for (int i = 0; i < results.pagesCompared; ++i) {
        if (m_comparisonWatcher && m_comparisonWatcher->isCanceled()) {
            break;
        }

        QList<DocumentDifference> pageDiffs = comparePages(i, i);
        results.differences.append(pageDiffs);

        // Update progress
        emit comparisonProgress((i + 1) * 100 / results.pagesCompared,
                                QString("Comparing page %1 of %2")
                                    .arg(i + 1)
                                    .arg(results.pagesCompared));
    }

    results.comparisonTime = timer.elapsed();

    // Calculate statistics
    for (const auto& diff : results.differences) {
        results.differenceCountByType[diff.type]++;
    }

    // Calculate overall similarity (simplified)
    int totalDifferences = results.differences.size();
    results.overallSimilarity =
        totalDifferences > 0
            ? qMax(0.0,
                   1.0 - (totalDifferences / (results.pagesCompared * 10.0)))
            : 1.0;

    results.summary = QString("Found %1 differences across %2 pages in %3ms")
                          .arg(totalDifferences)
                          .arg(results.pagesCompared)
                          .arg(results.comparisonTime);

    return results;
}

QList<DocumentDifference> DocumentComparison::comparePages(int page1,
                                                           int page2) {
    QList<DocumentDifference> differences;

    if (!m_document1 || !m_document2 || page1 >= m_document1->numPages() ||
        page2 >= m_document2->numPages()) {
        return differences;
    }

    try {
        std::unique_ptr<Poppler::Page> popplerPage1(m_document1->page(page1));
        std::unique_ptr<Poppler::Page> popplerPage2(m_document2->page(page2));

        if (!popplerPage1 || !popplerPage2) {
            return differences;
        }

        // Compare text if enabled
        if (m_options.compareText) {
            QString text1 = popplerPage1->text(QRectF());
            QString text2 = popplerPage2->text(QRectF());
            differences.append(compareText(text1, text2, page1, page2));
        }

        // Compare images if enabled
        if (m_options.compareImages) {
            QImage image1 = popplerPage1->renderToImage(150, 150);
            QImage image2 = popplerPage2->renderToImage(150, 150);
            QPixmap pixmap1 = QPixmap::fromImage(image1);
            QPixmap pixmap2 = QPixmap::fromImage(image2);
            differences.append(compareImages(pixmap1, pixmap2, page1, page2));
        }

    } catch (const std::exception& e) {
        qDebug() << "Error comparing pages" << page1 << "and" << page2 << ":"
                 << e.what();
    } catch (...) {
        qDebug() << "Unknown error comparing pages" << page1 << "and" << page2;
    }

    return differences;
}

QList<DocumentDifference> DocumentComparison::compareText(const QString& text1,
                                                          const QString& text2,
                                                          int page1,
                                                          int page2) {
    QList<DocumentDifference> differences;

    QString processedText1 = text1;
    QString processedText2 = text2;

    if (m_options.ignoreWhitespace) {
        processedText1 = processedText1.simplified();
        processedText2 = processedText2.simplified();
    }

    if (m_options.ignoreCaseChanges) {
        processedText1 = processedText1.toLower();
        processedText2 = processedText2.toLower();
    }

    double similarity = calculateTextSimilarity(processedText1, processedText2);

    if (similarity < m_options.textSimilarityThreshold) {
        DocumentDifference diff;
        diff.type = DifferenceType::TextModified;
        diff.pageNumber1 = page1;
        diff.pageNumber2 = page2;
        diff.oldText = text1;
        diff.newText = text2;
        diff.confidence = 1.0 - similarity;
        diff.description = QString("Text differs (similarity: %1%)")
                               .arg(similarity * 100, 0, 'f', 1);
        differences.append(diff);
    }

    return differences;
}

QList<DocumentDifference> DocumentComparison::compareImages(
    const QPixmap& image1, const QPixmap& image2, int page1, int page2) {
    QList<DocumentDifference> differences;

    if (image1.isNull() || image2.isNull()) {
        return differences;
    }

    double similarity = calculateImageSimilarity(image1, image2);

    if (similarity < m_options.imageSimilarityThreshold) {
        DocumentDifference diff;
        diff.type = DifferenceType::ImageModified;
        diff.pageNumber1 = page1;
        diff.pageNumber2 = page2;
        diff.confidence = 1.0 - similarity;
        diff.description = QString("Image differs (similarity: %1%)")
                               .arg(similarity * 100, 0, 'f', 1);
        differences.append(diff);
    }

    return differences;
}

double DocumentComparison::calculateTextSimilarity(const QString& text1,
                                                   const QString& text2) {
    if (text1 == text2)
        return 1.0;
    if (text1.isEmpty() && text2.isEmpty())
        return 1.0;
    if (text1.isEmpty() || text2.isEmpty())
        return 0.0;

    // Simple Levenshtein distance-based similarity
    int maxLen = qMax(text1.length(), text2.length());
    int distance = 0;

    // Simplified calculation for performance
    int minLen = qMin(text1.length(), text2.length());
    for (int i = 0; i < minLen; ++i) {
        if (text1[i] != text2[i]) {
            distance++;
        }
    }
    distance += qAbs(text1.length() - text2.length());

    return qMax(0.0, 1.0 - (static_cast<double>(distance) / maxLen));
}

double DocumentComparison::calculateImageSimilarity(const QPixmap& image1,
                                                    const QPixmap& image2) {
    if (image1.size() != image2.size()) {
        return 0.5;  // Different sizes, moderate similarity
    }

    // Convert to images for pixel comparison
    QImage img1 = image1.toImage();
    QImage img2 = image2.toImage();

    if (img1.format() != img2.format()) {
        img1 = img1.convertToFormat(QImage::Format_RGB32);
        img2 = img2.convertToFormat(QImage::Format_RGB32);
    }

    int width = img1.width();
    int height = img1.height();
    int totalPixels = width * height;
    int differentPixels = 0;

    // Sample pixels for performance (every 4th pixel)
    for (int y = 0; y < height; y += 4) {
        for (int x = 0; x < width; x += 4) {
            if (img1.pixel(x, y) != img2.pixel(x, y)) {
                differentPixels++;
            }
        }
    }

    int sampledPixels = (width / 4) * (height / 4);
    return sampledPixels > 0
               ? 1.0 - (static_cast<double>(differentPixels) / sampledPixels)
               : 1.0;
}

void DocumentComparison::onComparisonFinished() {
    m_isComparing = false;
    m_compareButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_exportButton->setEnabled(true);
    m_progressBar->setVisible(false);
    m_progressTimer->stop();

    if (m_comparisonWatcher->isCanceled()) {
        m_statusLabel->setText("Comparison cancelled");
        return;
    }

    m_results = m_comparisonWatcher->result();
    updateDifferencesList();

    m_statusLabel->setText(
        QString("Found %1 differences").arg(m_results.differences.size()));

    emit comparisonFinished(m_results);
}

void DocumentComparison::updateProgress() {
    // Update progress bar if needed
    if (m_isComparing && m_progressBar->isVisible()) {
        // Progress is updated via signals from the comparison thread
    }
}

void DocumentComparison::updateDifferencesList() {
    m_differencesTree->clear();

    for (int i = 0; i < m_results.differences.size(); ++i) {
        const DocumentDifference& diff = m_results.differences[i];

        QTreeWidgetItem* item = new QTreeWidgetItem(m_differencesTree);

        QString typeStr;
        switch (diff.type) {
            case DifferenceType::TextAdded:
                typeStr = "Text Added";
                break;
            case DifferenceType::TextRemoved:
                typeStr = "Text Removed";
                break;
            case DifferenceType::TextModified:
                typeStr = "Text Modified";
                break;
            case DifferenceType::ImageAdded:
                typeStr = "Image Added";
                break;
            case DifferenceType::ImageRemoved:
                typeStr = "Image Removed";
                break;
            case DifferenceType::ImageModified:
                typeStr = "Image Modified";
                break;
            case DifferenceType::LayoutChanged:
                typeStr = "Layout Changed";
                break;
            case DifferenceType::AnnotationAdded:
                typeStr = "Annotation Added";
                break;
            case DifferenceType::AnnotationRemoved:
                typeStr = "Annotation Removed";
                break;
            case DifferenceType::AnnotationModified:
                typeStr = "Annotation Modified";
                break;
        }

        item->setText(0, typeStr);
        item->setText(1, QString("%1/%2")
                             .arg(diff.pageNumber1 + 1)
                             .arg(diff.pageNumber2 + 1));
        item->setText(2, diff.description);
        item->setText(3, QString("%1%").arg(diff.confidence * 100, 0, 'f', 1));
        item->setData(0, Qt::UserRole, i);  // Store difference index
    }

    m_differencesTree->resizeColumnToContents(0);
    m_differencesTree->resizeColumnToContents(1);
    m_differencesTree->resizeColumnToContents(3);
}

void DocumentComparison::onDifferenceClicked(QTreeWidgetItem* item,
                                             int column) {
    Q_UNUSED(column)

    if (!item)
        return;

    int index = item->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < m_results.differences.size()) {
        goToDifference(index);
    }
}

void DocumentComparison::goToDifference(int index) {
    if (index < 0 || index >= m_results.differences.size()) {
        return;
    }

    m_currentDifferenceIndex = index;
    const DocumentDifference& diff = m_results.differences[index];

    // Update difference details
    QString details = QString("Type: %1\n").arg(static_cast<int>(diff.type));
    details += QString("Pages: %1 / %2\n")
                   .arg(diff.pageNumber1 + 1)
                   .arg(diff.pageNumber2 + 1);
    details +=
        QString("Confidence: %1%\n").arg(diff.confidence * 100, 0, 'f', 1);
    details += QString("Description: %1\n").arg(diff.description);

    if (!diff.oldText.isEmpty() || !diff.newText.isEmpty()) {
        details += QString("\nOld Text: %1\n").arg(diff.oldText.left(200));
        details += QString("New Text: %1\n").arg(diff.newText.left(200));
    }

    m_differenceDetails->setText(details);

    // Highlight the difference in the view
    highlightDifference(diff);

    emit differenceSelected(diff);
}

void DocumentComparison::highlightDifference(const DocumentDifference& diff) {
    // Update the comparison view to show the pages with the difference
    if (!m_document1 || !m_document2)
        return;

    try {
        if (diff.pageNumber1 >= 0 &&
            diff.pageNumber1 < m_document1->numPages()) {
            std::unique_ptr<Poppler::Page> page1(
                m_document1->page(diff.pageNumber1));
            if (page1) {
                QImage image1 = page1->renderToImage(150, 150);
                m_leftImageLabel->setPixmap(QPixmap::fromImage(image1));
            }
        }

        if (diff.pageNumber2 >= 0 &&
            diff.pageNumber2 < m_document2->numPages()) {
            std::unique_ptr<Poppler::Page> page2(
                m_document2->page(diff.pageNumber2));
            if (page2) {
                QImage image2 = page2->renderToImage(150, 150);
                m_rightImageLabel->setPixmap(QPixmap::fromImage(image2));
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "Error highlighting difference:" << e.what();
    } catch (...) {
        qDebug() << "Unknown error highlighting difference";
    }
}

void DocumentComparison::clearHighlights() {
    m_leftImageLabel->clear();
    m_rightImageLabel->clear();
    m_leftImageLabel->setText("Document 1");
    m_rightImageLabel->setText("Document 2");
}

void DocumentComparison::nextDifference() {
    if (m_currentDifferenceIndex < m_results.differences.size() - 1) {
        goToDifference(m_currentDifferenceIndex + 1);
    }
}

void DocumentComparison::previousDifference() {
    if (m_currentDifferenceIndex > 0) {
        goToDifference(m_currentDifferenceIndex - 1);
    }
}

void DocumentComparison::onViewModeChanged() { updateComparisonView(); }

void DocumentComparison::updateComparisonView() {
    QString mode = m_viewModeCombo->currentText();

    if (mode == "Side by Side") {
        m_viewSplitter->setOrientation(Qt::Horizontal);
        m_leftView->setVisible(true);
        m_rightView->setVisible(true);
    } else if (mode == "Overlay") {
        // For overlay mode, we could implement image blending
        m_viewSplitter->setOrientation(Qt::Horizontal);
        m_leftView->setVisible(true);
        m_rightView->setVisible(true);
    } else if (mode == "Differences Only") {
        // Show only the differences
        m_viewSplitter->setOrientation(Qt::Horizontal);
        m_leftView->setVisible(true);
        m_rightView->setVisible(true);
    }
}

void DocumentComparison::onOptionsChanged() {
    // Options have changed, could trigger re-comparison if needed
    m_options = getComparisonOptions();
}

void DocumentComparison::showDifferenceDetails(bool show) {
    m_differenceDetails->setVisible(show);
}

void DocumentComparison::setViewMode(const QString& mode) {
    int index = m_viewModeCombo->findText(mode);
    if (index >= 0) {
        m_viewModeCombo->setCurrentIndex(index);
    }
}

QString DocumentComparison::generateComparisonReport() const {
    QString report;
    QTextStream stream(&report);

    stream << "Document Comparison Report\n";
    stream << "==========================\n\n";

    stream << "Documents:\n";
    stream << "  Document 1: " << m_documentPath1 << " ("
           << m_results.totalPages1 << " pages)\n";
    stream << "  Document 2: " << m_documentPath2 << " ("
           << m_results.totalPages2 << " pages)\n\n";

    stream << "Comparison Summary:\n";
    stream << "  Pages compared: " << m_results.pagesCompared << "\n";
    stream << "  Total differences: " << m_results.differences.size() << "\n";
    stream << "  Overall similarity: " << (m_results.overallSimilarity * 100)
           << "%\n";
    stream << "  Comparison time: " << m_results.comparisonTime << " ms\n\n";

    stream << "Differences by type:\n";
    for (auto it = m_results.differenceCountByType.begin();
         it != m_results.differenceCountByType.end(); ++it) {
        stream << "  " << static_cast<int>(it.key()) << ": " << it.value()
               << "\n";
    }

    stream << "\nDetailed differences:\n";
    for (int i = 0; i < m_results.differences.size(); ++i) {
        const DocumentDifference& diff = m_results.differences[i];
        stream << QString("  %1. %2 on pages %3/%4 (confidence: %5%)\n")
                      .arg(i + 1)
                      .arg(diff.description)
                      .arg(diff.pageNumber1 + 1)
                      .arg(diff.pageNumber2 + 1)
                      .arg(diff.confidence * 100, 0, 'f', 1);
    }

    return report;
}

bool DocumentComparison::exportResultsToFile(const QString& filePath) const {
    QJsonObject root;
    root["documentPath1"] = m_documentPath1;
    root["documentPath2"] = m_documentPath2;
    root["totalPages1"] = m_results.totalPages1;
    root["totalPages2"] = m_results.totalPages2;
    root["pagesCompared"] = m_results.pagesCompared;
    root["comparisonTime"] = m_results.comparisonTime;
    root["overallSimilarity"] = m_results.overallSimilarity;
    root["summary"] = m_results.summary;

    QJsonArray differencesArray;
    for (const DocumentDifference& diff : m_results.differences) {
        QJsonObject diffObj;
        diffObj["type"] = static_cast<int>(diff.type);
        diffObj["pageNumber1"] = diff.pageNumber1;
        diffObj["pageNumber2"] = diff.pageNumber2;
        diffObj["description"] = diff.description;
        diffObj["confidence"] = diff.confidence;
        diffObj["oldText"] = diff.oldText;
        diffObj["newText"] = diff.newText;
        differencesArray.append(diffObj);
    }
    root["differences"] = differencesArray;

    QJsonDocument doc(root);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        return true;
    }

    return false;
}

// Additional comparison functions
bool DocumentComparison::compareDocumentMetadata(Poppler::Document* doc1,
                                                 Poppler::Document* doc2) {
    if (!doc1 || !doc2)
        return false;

    DocumentDifference diff;
    diff.type = DifferenceType::LayoutChanged;
    diff.pageNumber1 = -1;  // Metadata applies to whole document
    diff.pageNumber2 = -1;
    diff.confidence = 1.0;

    bool hasMetadataDifferences = false;

    // Compare page counts
    if (doc1->numPages() != doc2->numPages()) {
        diff.description = QString("Page count differs: %1 vs %2")
                               .arg(doc1->numPages())
                               .arg(doc2->numPages());
        m_results.differences.append(diff);
        hasMetadataDifferences = true;
    }

    // Compare document info
    QString title1 = doc1->info("Title");
    QString title2 = doc2->info("Title");
    if (title1 != title2) {
        diff.description =
            QString("Title differs: '%1' vs '%2'").arg(title1, title2);
        m_results.differences.append(diff);
        hasMetadataDifferences = true;
    }

    QString author1 = doc1->info("Author");
    QString author2 = doc2->info("Author");
    if (author1 != author2) {
        diff.description =
            QString("Author differs: '%1' vs '%2'").arg(author1, author2);
        m_results.differences.append(diff);
        hasMetadataDifferences = true;
    }

    return !hasMetadataDifferences;
}

QList<DocumentDifference> DocumentComparison::comparePageLayouts(int page1,
                                                                 int page2) {
    QList<DocumentDifference> differences;

    if (!m_document1 || !m_document2 || page1 >= m_document1->numPages() ||
        page2 >= m_document2->numPages()) {
        return differences;
    }

    try {
        std::unique_ptr<Poppler::Page> popplerPage1(m_document1->page(page1));
        std::unique_ptr<Poppler::Page> popplerPage2(m_document2->page(page2));

        if (!popplerPage1 || !popplerPage2) {
            return differences;
        }

        // Compare page sizes
        QSizeF size1 = popplerPage1->pageSizeF();
        QSizeF size2 = popplerPage2->pageSizeF();

        if (size1 != size2) {
            DocumentDifference diff;
            diff.type = DifferenceType::LayoutChanged;
            diff.pageNumber1 = page1;
            diff.pageNumber2 = page2;
            diff.confidence = 1.0;
            diff.description = QString("Page size differs: %1x%2 vs %3x%4")
                                   .arg(size1.width())
                                   .arg(size1.height())
                                   .arg(size2.width())
                                   .arg(size2.height());
            differences.append(diff);
        }

    } catch (const std::exception& e) {
        qDebug() << "Error comparing page layouts:" << e.what();
    } catch (...) {
        qDebug() << "Unknown error comparing page layouts";
    }

    return differences;
}

void DocumentComparison::generateDetailedReport() {
    QString report = generateComparisonReport();

    // Add more detailed analysis
    report += "\n\nDetailed Analysis:\n";
    report += "==================\n\n";

    // Analyze difference patterns
    QMap<DifferenceType, int> typeCount;
    QMap<int, int> pageCount;

    for (const DocumentDifference& diff : m_results.differences) {
        typeCount[diff.type]++;
        pageCount[diff.pageNumber1]++;
    }

    report += "Difference Distribution by Type:\n";
    for (auto it = typeCount.begin(); it != typeCount.end(); ++it) {
        QString typeName = getDifferenceTypeName(it.key());
        report +=
            QString("  %1: %2 occurrences\n").arg(typeName).arg(it.value());
    }

    report += "\nPages with Most Differences:\n";
    QList<QPair<int, int>> sortedPages;
    for (auto it = pageCount.begin(); it != pageCount.end(); ++it) {
        sortedPages.append(qMakePair(it.value(), it.key()));
    }
    std::sort(sortedPages.begin(), sortedPages.end(),
              std::greater<QPair<int, int>>());

    for (int i = 0; i < qMin(5, sortedPages.size()); ++i) {
        report += QString("  Page %1: %2 differences\n")
                      .arg(sortedPages[i].second + 1)
                      .arg(sortedPages[i].first);
    }

    // Save detailed report
    QString fileName =
        QString("detailed_comparison_%1.txt")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << report;
        emit detailedReportGenerated(fileName);
    }
}

QString DocumentComparison::getDifferenceTypeName(DifferenceType type) const {
    switch (type) {
        case DifferenceType::TextAdded:
            return "Text Added";
        case DifferenceType::TextRemoved:
            return "Text Removed";
        case DifferenceType::TextModified:
            return "Text Modified";
        case DifferenceType::ImageAdded:
            return "Image Added";
        case DifferenceType::ImageRemoved:
            return "Image Removed";
        case DifferenceType::ImageModified:
            return "Image Modified";
        case DifferenceType::LayoutChanged:
            return "Layout Changed";
        case DifferenceType::AnnotationAdded:
            return "Annotation Added";
        case DifferenceType::AnnotationRemoved:
            return "Annotation Removed";
        case DifferenceType::AnnotationModified:
            return "Annotation Modified";
        default:
            return "Unknown";
    }
}

void DocumentComparison::exportDifferencesToCSV(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);

    // Write CSV header
    out << "Type,Page1,Page2,Description,Confidence,OldText,NewText\n";

    // Write differences
    for (const DocumentDifference& diff : m_results.differences) {
        out << getDifferenceTypeName(diff.type) << "," << (diff.pageNumber1 + 1)
            << "," << (diff.pageNumber2 + 1) << ","
            << "\"" << QString(diff.description).replace("\"", "\"\"") << "\","
            << diff.confidence << ","
            << "\"" << QString(diff.oldText).replace("\"", "\"\"") << "\","
            << "\"" << QString(diff.newText).replace("\"", "\"\"") << "\"\n";
    }

    emit differencesExportedToCSV(filePath);
}

void DocumentComparison::createVisualDifferenceMap() {
    if (!m_document1 || !m_document2)
        return;

    int maxPages = qMax(m_document1->numPages(), m_document2->numPages());

    // Create a visual representation of differences across pages
    QPixmap differenceMap(maxPages * 10, 100);
    differenceMap.fill(Qt::white);

    QPainter painter(&differenceMap);

    // Count differences per page
    QMap<int, int> pageDifferences;
    for (const DocumentDifference& diff : m_results.differences) {
        if (diff.pageNumber1 >= 0) {
            pageDifferences[diff.pageNumber1]++;
        }
    }

    // Draw difference intensity
    for (int page = 0; page < maxPages; ++page) {
        int diffCount = pageDifferences.value(page, 0);
        if (diffCount > 0) {
            // Color intensity based on difference count
            int intensity = qMin(255, diffCount * 50);
            QColor color(255, 255 - intensity, 255 - intensity);
            painter.fillRect(page * 10, 0, 10, 100, color);
        }
    }

    painter.end();

    // Save the difference map
    QString fileName =
        QString("difference_map_%1.png")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    differenceMap.save(fileName);

    emit visualDifferenceMapCreated(fileName);
}

bool DocumentComparison::saveComparisonSession(const QString& filePath) {
    QJsonObject session;
    session["documentPath1"] = m_documentPath1;
    session["documentPath2"] = m_documentPath2;
    session["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Save comparison options
    QJsonObject optionsObj;
    optionsObj["compareText"] = m_options.compareText;
    optionsObj["compareImages"] = m_options.compareImages;
    optionsObj["compareLayout"] = m_options.compareLayout;
    optionsObj["compareAnnotations"] = m_options.compareAnnotations;
    optionsObj["ignoreWhitespace"] = m_options.ignoreWhitespace;
    optionsObj["ignoreCaseChanges"] = m_options.ignoreCaseChanges;
    optionsObj["textSimilarityThreshold"] = m_options.textSimilarityThreshold;
    optionsObj["imageSimilarityThreshold"] = m_options.imageSimilarityThreshold;
    optionsObj["maxDifferencesPerPage"] = m_options.maxDifferencesPerPage;
    session["options"] = optionsObj;

    // Save results summary
    QJsonObject resultsObj;
    resultsObj["totalPages1"] = m_results.totalPages1;
    resultsObj["totalPages2"] = m_results.totalPages2;
    resultsObj["pagesCompared"] = m_results.pagesCompared;
    resultsObj["comparisonTime"] = m_results.comparisonTime;
    resultsObj["overallSimilarity"] = m_results.overallSimilarity;
    resultsObj["differenceCount"] = m_results.differences.size();
    session["results"] = resultsObj;

    QJsonDocument doc(session);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        emit comparisonSessionSaved(filePath);
        return true;
    }

    return false;
}

bool DocumentComparison::loadComparisonSession(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject session = doc.object();

    // Load document paths
    m_documentPath1 = session["documentPath1"].toString();
    m_documentPath2 = session["documentPath2"].toString();

    // Load comparison options
    QJsonObject optionsObj = session["options"].toObject();
    ComparisonOptions options;
    options.compareText = optionsObj["compareText"].toBool();
    options.compareImages = optionsObj["compareImages"].toBool();
    options.compareLayout = optionsObj["compareLayout"].toBool();
    options.compareAnnotations = optionsObj["compareAnnotations"].toBool();
    options.ignoreWhitespace = optionsObj["ignoreWhitespace"].toBool();
    options.ignoreCaseChanges = optionsObj["ignoreCaseChanges"].toBool();
    options.textSimilarityThreshold =
        optionsObj["textSimilarityThreshold"].toDouble();
    options.imageSimilarityThreshold =
        optionsObj["imageSimilarityThreshold"].toDouble();
    options.maxDifferencesPerPage = optionsObj["maxDifferencesPerPage"].toInt();

    setComparisonOptions(options);

    emit comparisonSessionLoaded(filePath);
    return true;
}
