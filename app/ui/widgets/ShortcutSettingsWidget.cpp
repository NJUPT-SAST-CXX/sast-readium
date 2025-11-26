#include "ShortcutSettingsWidget.h"

#include <QEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QSettings>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaTableView.h"
#include "ElaText.h"

#include "logging/SimpleLogging.h"

// ============================================================================
// ShortcutSettingsWidget Implementation
// ============================================================================

ShortcutSettingsWidget::ShortcutSettingsWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_searchEdit(nullptr),
      m_categoryFilter(nullptr),
      m_shortcutsTable(nullptr),
      m_shortcutsModel(nullptr),
      m_resetAllBtn(nullptr),
      m_resetSelectedBtn(nullptr) {
    setupUi();
    loadShortcuts();
}

ShortcutSettingsWidget::~ShortcutSettingsWidget() = default;

void ShortcutSettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(12);

    // Search and filter section
    auto* filterArea = new ElaScrollPageArea(this);
    filterArea->setFixedHeight(50);
    auto* filterLayout = new QHBoxLayout(filterArea);
    filterLayout->setContentsMargins(12, 8, 12, 8);

    m_searchEdit = new ElaLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search shortcuts..."));
    m_searchEdit->setFixedWidth(200);
    filterLayout->addWidget(m_searchEdit);

    m_categoryFilter = new ElaComboBox(this);
    m_categoryFilter->addItem(tr("All Categories"), "all");
    m_categoryFilter->addItem(tr("File"), "file");
    m_categoryFilter->addItem(tr("Edit"), "edit");
    m_categoryFilter->addItem(tr("View"), "view");
    m_categoryFilter->addItem(tr("Navigation"), "navigation");
    m_categoryFilter->addItem(tr("Zoom"), "zoom");
    m_categoryFilter->addItem(tr("Tools"), "tools");
    filterLayout->addWidget(m_categoryFilter);
    filterLayout->addStretch();

    m_mainLayout->addWidget(filterArea);

    // Shortcuts table using ElaTableView
    m_shortcutsModel = new QStandardItemModel(this);
    m_shortcutsModel->setHorizontalHeaderLabels(
        {tr("Action"), tr("Shortcut"), tr("Category")});

    m_shortcutsTable = new ElaTableView(this);
    m_shortcutsTable->setModel(m_shortcutsModel);
    m_shortcutsTable->horizontalHeader()->setStretchLastSection(true);
    m_shortcutsTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    m_shortcutsTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Fixed);
    m_shortcutsTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::Fixed);
    m_shortcutsTable->setColumnWidth(1, 150);
    m_shortcutsTable->setColumnWidth(2, 100);
    m_shortcutsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_shortcutsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_shortcutsTable->setAlternatingRowColors(true);
    m_shortcutsTable->verticalHeader()->setVisible(false);
    m_mainLayout->addWidget(m_shortcutsTable);

    // Buttons
    auto* buttonArea = new ElaScrollPageArea(this);
    buttonArea->setFixedHeight(50);
    auto* buttonLayout = new QHBoxLayout(buttonArea);
    buttonLayout->setContentsMargins(12, 8, 12, 8);

    auto* hintText = new ElaText(tr("Double-click to edit shortcut"), this);
    hintText->setTextPixelSize(12);
    buttonLayout->addWidget(hintText);
    buttonLayout->addStretch();

    m_resetSelectedBtn = new ElaPushButton(tr("Reset Selected"), this);
    buttonLayout->addWidget(m_resetSelectedBtn);

    m_resetAllBtn = new ElaPushButton(tr("Reset All"), this);
    buttonLayout->addWidget(m_resetAllBtn);

    m_mainLayout->addWidget(buttonArea);

    // Connect signals
    connect(m_searchEdit, &ElaLineEdit::textChanged, this,
            &ShortcutSettingsWidget::onFilterTextChanged);
    connect(m_categoryFilter,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged), this,
            &ShortcutSettingsWidget::onCategoryFilterChanged);
    connect(m_shortcutsTable, &ElaTableView::doubleClicked, this,
            [this](const QModelIndex& index) {
                onShortcutCellClicked(index.row(), index.column());
            });
    connect(m_resetAllBtn, &ElaPushButton::clicked, this,
            &ShortcutSettingsWidget::onResetAllClicked);
    connect(m_resetSelectedBtn, &ElaPushButton::clicked, this,
            &ShortcutSettingsWidget::onResetSelectedClicked);
}

void ShortcutSettingsWidget::loadShortcuts() {
    m_shortcuts.clear();

    // File operations
    m_shortcuts.append({"file_open", tr("Open File"), tr("File"),
                        QKeySequence::Open, QKeySequence::Open});
    m_shortcuts.append({"file_save", tr("Save"), tr("File"), QKeySequence::Save,
                        QKeySequence::Save});
    m_shortcuts.append({"file_print", tr("Print"), tr("File"),
                        QKeySequence::Print, QKeySequence::Print});
    m_shortcuts.append({"file_close", tr("Close"), tr("File"),
                        QKeySequence::Close, QKeySequence::Close});

    // Edit operations
    m_shortcuts.append({"edit_copy", tr("Copy"), tr("Edit"), QKeySequence::Copy,
                        QKeySequence::Copy});
    m_shortcuts.append({"edit_find", tr("Find"), tr("Edit"), QKeySequence::Find,
                        QKeySequence::Find});

    // View operations
    m_shortcuts.append({"view_fullscreen", tr("Toggle Fullscreen"), tr("View"),
                        QKeySequence(Qt::Key_F11), QKeySequence(Qt::Key_F11)});
    m_shortcuts.append({"view_sidebar", tr("Toggle Sidebar"), tr("View"),
                        QKeySequence(Qt::Key_F9), QKeySequence(Qt::Key_F9)});

    // Navigation
    m_shortcuts.append({"nav_next_page", tr("Next Page"), tr("Navigation"),
                        QKeySequence(Qt::Key_PageDown),
                        QKeySequence(Qt::Key_PageDown)});
    m_shortcuts.append({"nav_prev_page", tr("Previous Page"), tr("Navigation"),
                        QKeySequence(Qt::Key_PageUp),
                        QKeySequence(Qt::Key_PageUp)});

    // Zoom
    m_shortcuts.append({"zoom_in", tr("Zoom In"), tr("Zoom"),
                        QKeySequence::ZoomIn, QKeySequence::ZoomIn});
    m_shortcuts.append({"zoom_out", tr("Zoom Out"), tr("Zoom"),
                        QKeySequence::ZoomOut, QKeySequence::ZoomOut});

    // Load custom shortcuts from settings
    QSettings settings("SAST", "Readium");
    settings.beginGroup("Shortcuts");
    for (auto& entry : m_shortcuts) {
        if (settings.contains(entry.actionId)) {
            entry.currentShortcut =
                QKeySequence(settings.value(entry.actionId).toString());
        }
    }
    settings.endGroup();

    populateShortcutsTable();
}

void ShortcutSettingsWidget::populateShortcutsTable() {
    m_shortcutsModel->setRowCount(m_shortcuts.size());

    for (int i = 0; i < m_shortcuts.size(); ++i) {
        const auto& entry = m_shortcuts[i];

        auto* actionItem = new QStandardItem(entry.description);
        actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);
        actionItem->setData(entry.actionId, Qt::UserRole);
        m_shortcutsModel->setItem(i, 0, actionItem);

        auto* shortcutItem = new QStandardItem(
            entry.currentShortcut.toString(QKeySequence::NativeText));
        shortcutItem->setFlags(shortcutItem->flags() & ~Qt::ItemIsEditable);
        m_shortcutsModel->setItem(i, 1, shortcutItem);

        auto* categoryItem = new QStandardItem(entry.category);
        categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
        m_shortcutsModel->setItem(i, 2, categoryItem);
    }
}

void ShortcutSettingsWidget::saveShortcuts() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("Shortcuts");
    for (const auto& entry : m_shortcuts) {
        if (entry.currentShortcut != entry.defaultShortcut) {
            settings.setValue(entry.actionId, entry.currentShortcut.toString());
        } else {
            settings.remove(entry.actionId);
        }
    }
    settings.endGroup();
    emit shortcutsChanged();
}

void ShortcutSettingsWidget::resetToDefaults() {
    for (auto& entry : m_shortcuts) {
        entry.currentShortcut = entry.defaultShortcut;
    }
    populateShortcutsTable();
    emit shortcutsChanged();
}

void ShortcutSettingsWidget::onShortcutCellClicked(int row, int column) {
    if (column != 1 || row < 0 || row >= m_shortcuts.size()) {
        return;
    }
    // TODO: Implement inline shortcut editing with ElaKeyBinder
}

void ShortcutSettingsWidget::onFilterTextChanged(const QString& text) {
    Q_UNUSED(text)
    filterShortcuts();
}

void ShortcutSettingsWidget::onCategoryFilterChanged(int index) {
    Q_UNUSED(index)
    filterShortcuts();
}

void ShortcutSettingsWidget::filterShortcuts() {
    QString searchText = m_searchEdit->text().toLower();
    QString category = m_categoryFilter->currentData().toString();

    for (int i = 0; i < m_shortcutsModel->rowCount(); ++i) {
        bool visible = true;
        if (!searchText.isEmpty()) {
            QString actionText = m_shortcutsModel->item(i, 0)->text().toLower();
            visible = actionText.contains(searchText);
        }
        if (visible && category != "all") {
            QString itemCategory =
                m_shortcutsModel->item(i, 2)->text().toLower();
            visible = (itemCategory == category);
        }
        m_shortcutsTable->setRowHidden(i, !visible);
    }
}

void ShortcutSettingsWidget::onResetAllClicked() { resetToDefaults(); }

void ShortcutSettingsWidget::onResetSelectedClicked() {
    int row = m_shortcutsTable->currentIndex().row();
    if (row >= 0 && row < m_shortcuts.size()) {
        m_shortcuts[row].currentShortcut = m_shortcuts[row].defaultShortcut;
        m_shortcutsModel->item(row, 1)->setText(
            m_shortcuts[row].currentShortcut.toString(
                QKeySequence::NativeText));
        emit shortcutsChanged();
    }
}

bool ShortcutSettingsWidget::isValidShortcut(const QKeySequence& keySequence) {
    return !keySequence.isEmpty();
}

bool ShortcutSettingsWidget::hasConflict(const QKeySequence& keySequence,
                                         int currentRow) {
    for (int i = 0; i < m_shortcuts.size(); ++i) {
        if (i != currentRow && m_shortcuts[i].currentShortcut == keySequence) {
            return true;
        }
    }
    return false;
}

void ShortcutSettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void ShortcutSettingsWidget::retranslateUi() {
    m_searchEdit->setPlaceholderText(tr("Search shortcuts..."));
    m_shortcutsModel->setHorizontalHeaderLabels(
        {tr("Action"), tr("Shortcut"), tr("Category")});
    m_resetSelectedBtn->setText(tr("Reset Selected"));
    m_resetAllBtn->setText(tr("Reset All"));
}
