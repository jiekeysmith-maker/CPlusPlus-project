#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QString>
#include <vector>

#include "manage.h"

class QCloseEvent;
class QEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QAction;
class QToolBar;
class Ui_MainWindow;
class PaperDialog;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onSidebarItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void onSearchTextChanged(const QString &text);
    void onSearchClicked();
    void onShowAllClicked();
    void onAddPaper();
    void onEditPaper();
    void onDeletePaper();
    void onAddCatalog();
    void onDeleteCatalog();
    void onCatalogContextMenu(const QPoint &pos);
    void onAddPublication(const QString &forceType = QString());
    void onEditSourceFromToolbar();
    void onDeleteSourceFromToolbar();
    void onOpenPaperAttachment(int row, int column);
    void onViewPaperDetail();
    void onPaperSelectionChanged();
    void onOpenDetailAttachment();
    void onSave();
    void onLoad();

private:
    enum class LibraryNodeType {
        System,
        Catalog,
        PublicationRoot,
        PublicationGroup
    };

    struct LibraryNodeInfo {
        LibraryNodeType type = LibraryNodeType::System;
        IdType catalogId = INVALID_ID;
        QString key;
        bool deletable = false;
    };

    void setupUi();
    void setupMenuBar();
    void setupToolbar();
    void setupDetailPanel();
    void setupSourceTable();
    void setupSourceToolbar();
    void rebuildCatalogTree();
    void refreshPaperTable();
    void refreshSourceTable(const QString &typeFilter);
    void updatePaperTable(const std::vector<Paper> &papers);
    void updateDetailPanel(IdType paperId);
    void showSourceDetail(IdType sourceId);
    void clearDetailPanel();
    void restoreDetailPanelLabels();
    std::vector<Paper> collectCurrentPapers() const;
    std::vector<Paper> filterPapers(const std::vector<Paper> &papers, const QString &keyword) const;
    QString paperAuthorsText(const Paper &paper) const;
    QString paperKeywordsText(const Paper &paper) const;
    QString paperCatalogText(const Paper &paper) const;
    QString paperUploadTimeText(const Paper &paper) const;
    void ensureUploadTime(Paper &paper) const;
    void addPaperToCurrentCatalog(IdType paperId) const;
    bool moveSelectedPaperToCatalog(IdType catalogId);
    IdType selectedPaperId() const;
    IdType selectedSourceId() const;
    void adjustAuthorColumnWidth();
    bool isUserCatalogNode(const QTreeWidgetItem *item) const;
    bool isSystemNode(const QTreeWidgetItem *item) const;
    IdType currentCatalogId() const;
    QString currentNodeKey() const;
    void selectSystemNode(const QString &key);
    void showStatus(const QString &msg);
    void restoreSelectionAfterReload(const QString &key);
    void applyTreeStyle();
    bool saveDefaultData();
    bool loadDefaultData();
    bool migrateLegacyDataIfNeeded();
    void migrateLegacyPaperFiles(const QString &legacyDataPath);
    QString defaultDataFilePath() const;
    QString defaultUploadTime() const;
    QTreeWidgetItem *findNodeByKey(const QString &key) const;

    Ui::MainWindow *ui = nullptr;
    QTreeWidget *m_sidebar = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QTableWidget *m_table = nullptr;
    QTableWidget *m_sourceTable = nullptr;
    QToolBar *m_sourceToolBar = nullptr;
    QWidget *m_detailPanel = nullptr;
    QLabel *m_detailPanelTitle = nullptr;
    QLabel *m_detailTitleLabel = nullptr;
    QLabel *m_detailAuthorsLabel = nullptr;
    QLabel *m_detailSourceLabel = nullptr;
    QLabel *m_detailPublishDateLabel = nullptr;
    QLabel *m_detailUploadTimeLabel = nullptr;
    QLabel *m_detailKeywordsLabel = nullptr;
    QLabel *m_detailFilePathLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QString m_defaultDataPath;
    QString m_currentNodeKey;
    IdType m_detailPaperId = INVALID_ID;
    QAction *m_addPaperAction = nullptr;
    QAction *m_editPaperAction = nullptr;
    QAction *m_deletePaperAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_loadAction = nullptr;
    QPushButton *m_viewDetailButton = nullptr;
    QPushButton *m_openDetailAttachmentButton = nullptr;
    QPushButton *m_searchButton = nullptr;
    QPushButton *m_showAllButton = nullptr;
};

#endif // MAINWINDOW_H
