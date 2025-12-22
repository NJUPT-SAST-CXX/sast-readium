#pragma once

#include <QObject>
#include <QSplitter>
#include <QWidget>

/**
 * @brief Manages split view for documents (Feature 14)
 */
class SplitViewManager : public QObject {
    Q_OBJECT

public:
    static SplitViewManager& instance() {
        static SplitViewManager instance(nullptr);
        return instance;
    }

    enum SplitMode { None, Horizontal, Vertical };

    explicit SplitViewManager(QWidget* parentWidget, QObject* parent = nullptr);
    ~SplitViewManager() = default;

    void setSplitMode(SplitMode mode);
    SplitMode getSplitMode() const { return m_splitMode; }

    void setLeftDocument(int docIndex);
    void setRightDocument(int docIndex);

    void syncScroll(bool enable) { m_syncScroll = enable; }
    bool isSyncScrollEnabled() const { return m_syncScroll; }

signals:
    void splitModeChanged(SplitMode mode);
    void documentChanged(int leftDoc, int rightDoc);

private:
    SplitMode m_splitMode;
    QWidget* m_parentWidget;
    QSplitter* m_splitter;
    int m_leftDocIndex;
    int m_rightDocIndex;
    bool m_syncScroll;
};
