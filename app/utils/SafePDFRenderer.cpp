#include "SafePDFRenderer.h"
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QPainter>
#include <QRegularExpression>
#include <QThread>
#include <QtCore>

SafePDFRenderer& SafePDFRenderer::instance() {
    static SafePDFRenderer instance;
    return instance;
}

void SafePDFRenderer::setRenderConfig(const RenderConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_config = config;
}

const SafePDFRenderer::RenderConfig& SafePDFRenderer::getRenderConfig() const {
    QMutexLocker locker(&m_mutex);
    return m_config;
}

QImage SafePDFRenderer::safeRenderPage(Poppler::Page* page, double dpi,
                                       RenderInfo* info) {
    if (!page) {
        if (info) {
            info->success = false;
            info->errorMessage = "Invalid page pointer";
        }
        return QImage();
    }

    RenderInfo renderInfo;
    if (!info) {
        info = &renderInfo;
    }

    info->attemptCount = 0;
    info->actualDPI = dpi;
    info->usedFallback = false;

    QElapsedTimer timer;
    timer.start();

    try {
        // Validate page first
        if (!validatePage(page)) {
            info->success = false;
            info->errorMessage = "Page validation failed";
            return createPlaceholderImage(QSize(400, 300), "Invalid Page");
        }

        // Check compatibility if enabled
        if (m_config.enableCompatibilityCheck) {
            info->compatibility = checkPageCompatibility(page);
            if (info->compatibility == CompatibilityResult::QtGenerated) {
                Logger::instance().warning(
                    "[SafePDFRenderer] Qt-generated PDF detected, using safe "
                    "rendering");
                // Use conservative settings for Qt PDFs
                info->actualDPI = qMin(dpi, m_config.fallbackDPI);
                info->usedFallback = true;
            } else if (info->compatibility == CompatibilityResult::Corrupted) {
                Logger::instance().warning(
                    "[SafePDFRenderer] Corrupted PDF detected, cannot render");
                info->success = false;
                info->errorMessage = "PDF appears corrupted";
                return createPlaceholderImage(QSize(400, 300), "Corrupted PDF");
            }
        }

        // Validate DPI
        if (!isSafeDPI(info->actualDPI)) {
            info->actualDPI = qMin(info->actualDPI, m_config.maxDPI);
            info->usedFallback = true;
            Logger::instance().warning(
                "[SafePDFRenderer] DPI adjusted to safe value: {}",
                info->actualDPI);
        }

        // Attempt rendering with retries
        QImage result;
        for (int attempt = 1; attempt <= m_config.maxRetries; ++attempt) {
            info->attemptCount = attempt;

            try {
                result = safeRenderPageInternal(page, info->actualDPI, QRectF(),
                                                info);

                if (!result.isNull()) {
                    // Validate image size
                    if (!isSafeImageSize(result.size())) {
                        Logger::instance().warning(
                            "[SafePDFRenderer] Rendered image too large, "
                            "scaling down");
                        result = result.scaled(
                            qMin(result.width(), m_config.maxImageSize.width()),
                            qMin(result.height(),
                                 m_config.maxImageSize.height()),
                            Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        info->usedFallback = true;
                    }

                    info->success = true;
                    info->renderedSize = result.size();
                    info->renderTimeMs = timer.elapsed();

                    // Update statistics
                    {
                        QMutexLocker locker(&m_statsMutex);
                        m_statistics["successfulRenders"] =
                            m_statistics.value("successfulRenders", 0).toInt() +
                            1;
                        m_statistics["totalRenderTime"] =
                            m_statistics.value("totalRenderTime", 0)
                                .toLongLong() +
                            info->renderTimeMs;
                    }

                    Logger::instance().debug(
                        "[SafePDFRenderer] Successfully rendered page in {}ms",
                        info->renderTimeMs);
                    return result;
                }

            } catch (const std::exception& e) {
                Logger::instance().warning(
                    "[SafePDFRenderer] Render attempt {} failed: {}", attempt,
                    e.what());
                info->errorMessage = QString("Render failed: %1").arg(e.what());
            } catch (...) {
                Logger::instance().warning(
                    "[SafePDFRenderer] Render attempt {} failed with unknown "
                    "error",
                    attempt);
                info->errorMessage = "Unknown rendering error";
            }

            // Apply fallback strategy for failed attempts
            if (attempt < m_config.maxRetries) {
                if (m_config.fallbackStrategy ==
                    FallbackStrategy::TryLowResolution) {
                    QImage fallbackResult =
                        tryLowDpiRender(page, QRectF(), info);
                    if (!fallbackResult.isNull()) {
                        info->success = true;
                        info->usedFallback = true;
                        info->renderedSize = fallbackResult.size();
                        info->renderTimeMs = timer.elapsed();
                        return fallbackResult;
                    }
                }

                // Small delay between retries
                QThread::msleep(50 * attempt);
            }
        }

        // All attempts failed - apply final fallback strategy
        info->success = false;
        info->renderTimeMs = timer.elapsed();

        switch (m_config.fallbackStrategy) {
            case FallbackStrategy::UsePlaceholder:
                Logger::instance().warning(
                    "[SafePDFRenderer] All render attempts failed, using "
                    "placeholder");
                return createPlaceholderImage(QSize(400, 300), "Render Failed");

            case FallbackStrategy::TryLowResolution:
                Logger::instance().warning(
                    "[SafePDFRenderer] All render attempts failed, trying "
                    "final low-res attempt");
                {
                    QImage finalResult = tryLowDpiRender(page, QRectF(), info);
                    if (!finalResult.isNull()) {
                        info->success = true;
                        info->usedFallback = true;
                        return finalResult;
                    }
                }
                return createPlaceholderImage(QSize(400, 300), "Render Failed");

            case FallbackStrategy::Fail:
            default:
                Logger::instance().error(
                    "[SafePDFRenderer] All render attempts failed, returning "
                    "null image");
                return QImage();
        }

    } catch (const std::exception& e) {
        Logger::instance().error(
            "[SafePDFRenderer] Critical error in safeRenderPage: {}", e.what());
        info->success = false;
        info->errorMessage = QString("Critical error: %1").arg(e.what());
        info->renderTimeMs = timer.elapsed();
        return createPlaceholderImage(QSize(400, 300), "Critical Error");
    } catch (...) {
        Logger::instance().error(
            "[SafePDFRenderer] Unknown critical error in safeRenderPage");
        info->success = false;
        info->errorMessage = "Unknown critical error";
        info->renderTimeMs = timer.elapsed();
        return createPlaceholderImage(QSize(400, 300), "Unknown Error");
    }
}

QPixmap SafePDFRenderer::safeRenderPageToPixmap(Poppler::Page* page, double dpi,
                                                RenderInfo* info) {
    QImage image = safeRenderPage(page, dpi, info);
    return QPixmap::fromImage(image);
}

QImage SafePDFRenderer::safeRenderPageRegion(Poppler::Page* page,
                                             const QRectF& region, double dpi,
                                             RenderInfo* info) {
    if (!page || region.isEmpty()) {
        if (info) {
            info->success = false;
            info->errorMessage = "Invalid page or region";
        }
        return QImage();
    }

    RenderInfo renderInfo;
    if (!info) {
        info = &renderInfo;
    }

    // Use the same logic as full page rendering but for a specific region
    QElapsedTimer timer;
    timer.start();

    try {
        info->attemptCount = 1;
        info->actualDPI = dpi;

        // Validate inputs
        if (!validatePage(page)) {
            info->success = false;
            info->errorMessage = "Page validation failed";
            return QImage();
        }

        if (!isSafeDPI(dpi)) {
            info->actualDPI = qMin(dpi, m_config.maxDPI);
            info->usedFallback = true;
        }

        QImage result =
            safeRenderPageInternal(page, info->actualDPI, region, info);

        if (!result.isNull()) {
            info->success = true;
            info->renderedSize = result.size();
            info->renderTimeMs = timer.elapsed();
            return result;
        }
        info->success = false;
        info->errorMessage = "Region render failed";
        return QImage();

    } catch (const std::exception& e) {
        Logger::instance().error(
            "[SafePDFRenderer] Error in safeRenderPageRegion: {}", e.what());
        info->success = false;
        info->errorMessage = QString("Region render error: %1").arg(e.what());
        return QImage();
    }
}

SafePDFRenderer::CompatibilityResult SafePDFRenderer::checkCompatibility(
    Poppler::Document* document) {
    if (!document) {
        return CompatibilityResult::Unknown;
    }

    try {
        // Check for Qt-specific metadata and characteristics
        if (isQtGeneratedPDF(document)) {
            return CompatibilityResult::QtGenerated;
        }

        // Try to load and validate first page
        if (document->numPages() > 0) {
            std::unique_ptr<Poppler::Page> firstPage(document->page(0));
            if (firstPage) {
                CompatibilityResult pageResult =
                    checkPageCompatibility(firstPage.get());
                if (pageResult == CompatibilityResult::Corrupted) {
                    return CompatibilityResult::Corrupted;
                }
                if (pageResult == CompatibilityResult::QtGenerated) {
                    return CompatibilityResult::QtGenerated;
                }
            } else {
                return CompatibilityResult::Corrupted;
            }
        }

        // Try to extract text from first page as a basic sanity check
        if (document->numPages() > 0) {
            std::unique_ptr<Poppler::Page> testPage(document->page(0));
            if (testPage) {
                QString testText = testPage->text(QRectF());
                // If we can extract text without crashing, it's probably
                // compatible
                return CompatibilityResult::Compatible;
            }
        }

    } catch (const std::exception& e) {
        Logger::instance().warning(
            "[SafePDFRenderer] Error during compatibility check: {}", e.what());
        return CompatibilityResult::Unknown;
    } catch (...) {
        Logger::instance().warning(
            "[SafePDFRenderer] Unknown error during compatibility check");
        return CompatibilityResult::Unknown;
    }

    return CompatibilityResult::Compatible;
}

SafePDFRenderer::CompatibilityResult SafePDFRenderer::checkCompatibility(
    const std::shared_ptr<Poppler::Document>& document) {
    return checkCompatibility(document.get());
}

SafePDFRenderer::CompatibilityResult SafePDFRenderer::checkPageCompatibility(
    Poppler::Page* page) {
    if (!page) {
        return CompatibilityResult::Unknown;
    }

    try {
        // Check for Qt-specific content patterns
        if (hasQtSpecificContent(page)) {
            return CompatibilityResult::QtGenerated;
        }

        // Try a very low-resolution render test
        QImage testImage = page->renderToImage(36.0, 36.0, -1, -1, -1, -1);
        if (testImage.isNull()) {
            // If even low-res rendering fails, it might be corrupted
            return CompatibilityResult::Corrupted;
        }

        // Check if the rendered image looks reasonable
        if (testImage.width() <= 0 || testImage.height() <= 0) {
            return CompatibilityResult::Corrupted;
        }

        // Try to extract text as another sanity check
        QString testText = page->text(QRectF());
        // Empty text might indicate image-only PDF or corruption, but not
        // necessarily Qt-generated

        return CompatibilityResult::Compatible;

    } catch (const std::exception& e) {
        Logger::instance().warning(
            "[SafePDFRenderer] Error during page compatibility check: {}",
            e.what());
        return CompatibilityResult::Corrupted;
    } catch (...) {
        Logger::instance().warning(
            "[SafePDFRenderer] Unknown error during page compatibility check");
        return CompatibilityResult::Corrupted;
    }
}

QImage SafePDFRenderer::createPlaceholderImage(const QSize& size,
                                               const QString& text) {
    QImage image(size, QImage::Format_RGB32);
    image.fill(QColor(240, 240, 240));

    QPainter painter(&image);
    painter.setPen(QColor(100, 100, 100));
    painter.setFont(QFont("Arial", 12));

    if (!text.isEmpty()) {
        QRect textRect = image.rect();
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    }

    // Add a border
    painter.setPen(QColor(180, 180, 180));
    painter.drawRect(0, 0, size.width() - 1, size.height() - 1);

    return image;
}

bool SafePDFRenderer::canRenderSafely(Poppler::Page* page, RenderInfo* info) {
    if (!page) {
        return false;
    }

    CompatibilityResult compatibility = checkPageCompatibility(page);

    if (info) {
        info->compatibility = compatibility;
        info->success = (compatibility != CompatibilityResult::Corrupted);
    }

    return compatibility != CompatibilityResult::Corrupted;
}

QVariantMap SafePDFRenderer::getStatistics() const {
    QMutexLocker locker(&m_statsMutex);
    return m_statistics;
}

void SafePDFRenderer::resetStatistics() {
    QMutexLocker locker(&m_statsMutex);
    m_statistics.clear();
    m_statistics["successfulRenders"] = 0;
    m_statistics["failedRenders"] = 0;
    m_statistics["totalRenderTime"] = 0;
}

QImage SafePDFRenderer::safeRenderPageInternal(Poppler::Page* page, double dpi,
                                               const QRectF& region,
                                               RenderInfo* info) {
    QElapsedTimer timer;
    timer.start();

    try {
        QImage result;

        if (region.isEmpty()) {
            // Full page rendering
            result = page->renderToImage(dpi, dpi);
        } else {
            // Region rendering
            result = page->renderToImage(dpi, dpi, region.x(), region.y(),
                                         region.width(), region.height());
        }

        if (result.isNull()) {
            Logger::instance().warning(
                "[SafePDFRenderer] Poppler renderToImage returned null image");
            return QImage();
        }

        Logger::instance().debug(
            "[SafePDFRenderer] Internal render completed in {}ms",
            timer.elapsed());
        return result;

    } catch (const std::exception& e) {
        Logger::instance().error(
            "[SafePDFRenderer] Exception in safeRenderPageInternal: {}",
            e.what());
        if (info) {
            info->errorMessage =
                QString("Internal render error: %1").arg(e.what());
        }
        return QImage();
    } catch (...) {
        Logger::instance().error(
            "[SafePDFRenderer] Unknown exception in safeRenderPageInternal");
        if (info) {
            info->errorMessage = "Unknown internal render error";
        }
        return QImage();
    }
}

QImage SafePDFRenderer::tryLowDpiRender(Poppler::Page* page,
                                        const QRectF& region,
                                        RenderInfo* info) {
    Logger::instance().debug(
        "[SafePDFRenderer] Trying low DPI fallback render");

    double lowDpi = m_config.fallbackDPI;
    if (info && info->actualDPI > 72.0) {
        lowDpi = 72.0;  // Very conservative DPI
    }

    QImage result = safeRenderPageInternal(page, lowDpi, region, info);
    if (!result.isNull() && info) {
        info->usedFallback = true;
        info->actualDPI = lowDpi;
    }

    return result;
}

bool SafePDFRenderer::isSafeDPI(double dpi) const {
    return dpi > 0 && dpi <= m_config.maxDPI;
}

bool SafePDFRenderer::isSafeImageSize(const QSize& size) const {
    return size.width() > 0 && size.height() > 0 &&
           size.width() <= m_config.maxImageSize.width() &&
           size.height() <= m_config.maxImageSize.height();
}

bool SafePDFRenderer::validatePage(Poppler::Page* page) {
    if (!page) {
        return false;
    }

    try {
        // Basic page validation
        QSizeF pageSize = page->pageSizeF();
        if (pageSize.width() <= 0 || pageSize.height() <= 0) {
            return false;
        }

        // Try to get page orientation
        page->orientation();

        // Try to extract some text (this will throw if page is corrupted)
        page->text(QRectF());

        return true;

    } catch (const std::exception& e) {
        Logger::instance().warning(
            "[SafePDFRenderer] Page validation failed: {}", e.what());
        return false;
    } catch (...) {
        Logger::instance().warning(
            "[SafePDFRenderer] Unknown error during page validation");
        return false;
    }
}

QString SafePDFRenderer::extractPDFMetadata(Poppler::Document* document) {
    if (!document) {
        return QString();
    }

    try {
        QString metadata;
        metadata += document->info("Title") + "|";
        metadata += document->info("Author") + "|";
        metadata += document->info("Creator") + "|";
        metadata += document->info("Producer") + "|";
        metadata += document->info("CreationDate") + "|";
        metadata += document->info("ModDate");
        return metadata.toLower();

    } catch (const std::exception& e) {
        Logger::instance().warning(
            "[SafePDFRenderer] Error extracting metadata: {}", e.what());
        return QString();
    }
}

bool SafePDFRenderer::isQtGeneratedPDF(Poppler::Document* document) {
    if (!document) {
        return false;
    }

    try {
        QString metadata = extractPDFMetadata(document);

        // Check for Qt-specific signatures in metadata
        QStringList qtSignatures = {
            "qt",           "qpdfwriter", "qprinter",      "qpaintengine",
            "qpaintdevice", "qt company", "the qt company"};

        for (const QString& signature : qtSignatures) {
            if (metadata.contains(signature, Qt::CaseInsensitive)) {
                Logger::instance().debug(
                    "[SafePDFRenderer] Qt signature found in metadata: {}",
                    signature.toStdString());
                return true;
            }
        }

        // Additional checks for Qt PDF characteristics
        QString creator = document->info("Creator").toLower();
        QString producer = document->info("Producer").toLower();

        if (creator.contains("qt") || producer.contains("qt")) {
            Logger::instance().debug(
                "[SafePDFRenderer] Qt PDF detected by creator/producer");
            return true;
        }

        return false;

    } catch (const std::exception& e) {
        Logger::instance().warning(
            "[SafePDFRenderer] Error checking Qt PDF signature: {}", e.what());
        return false;
    }
}

bool SafePDFRenderer::hasQtSpecificContent(Poppler::Page* page) {
    if (!page) {
        return false;
    }

    try {
        // Extract text and check for Qt-specific patterns
        QString pageText = page->text(QRectF()).toLower();

        // Qt-generated PDFs often have specific text patterns or rendering
        // artifacts
        QStringList qtPatterns = {
            "qpdfwriter", "qprinter", "qpaintengine",
            // Add more patterns as discovered
        };

        for (const QString& pattern : qtPatterns) {
            if (pageText.contains(pattern)) {
                return true;
            }
        }

        // Additional content analysis could be added here
        // For now, focus on metadata which is more reliable

        return false;

    } catch (const std::exception& e) {
        Logger::instance().warning(
            "[SafePDFRenderer] Error checking Qt-specific content: {}",
            e.what());
        return false;
    }
}

QImage SafePDFRenderer::threadSafeRender(Poppler::Page* page, double dpi,
                                         const QRectF& region) {
    // This would be used for multi-threaded rendering scenarios
    // For now, just delegate to the internal method
    return safeRenderPageInternal(page, dpi, region, nullptr);
}
