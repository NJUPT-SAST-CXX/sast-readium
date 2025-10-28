#include "SplitViewManager.h"

SplitViewManager::SplitViewManager(QWidget* parentWidget, QObject* parent)
    : QObject(parent),
      m_splitMode(None),
      m_parentWidget(parentWidget),
      m_splitter(nullptr),
      m_leftDocIndex(-1),
      m_rightDocIndex(-1),
      m_syncScroll(false) {}

void SplitViewManager::setSplitMode(SplitMode mode) {
    if (m_splitMode != mode) {
        m_splitMode = mode;

        if (mode == None) {
            if (m_splitter) {
                m_splitter->deleteLater();
                m_splitter = nullptr;
            }
        } else {
            if (!m_splitter) {
                m_splitter = new QSplitter(m_parentWidget);
                m_splitter->setOrientation(mode == Horizontal ? Qt::Horizontal
                                                              : Qt::Vertical);
            } else {
                m_splitter->setOrientation(mode == Horizontal ? Qt::Horizontal
                                                              : Qt::Vertical);
            }
        }

        emit splitModeChanged(mode);
    }
}

void SplitViewManager::setLeftDocument(int docIndex) {
    m_leftDocIndex = docIndex;
    emit documentChanged(m_leftDocIndex, m_rightDocIndex);
}

void SplitViewManager::setRightDocument(int docIndex) {
    m_rightDocIndex = docIndex;
    emit documentChanged(m_leftDocIndex, m_rightDocIndex);
}
