#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QString>
#include <vector>

#include "manage.h"

class QCloseEvent;
class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QFormLayout;
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
    void onSelectAllClicked();
    void onAddPaper();
    void onAddAuthor();
    void onEditPaper();
    void onDeletePaper();
    void onAddCatalog();
    void onDeleteCatalog();
    void onAddAuthorCatalog();
    void onDeleteAuthorCatalog();
    void onCatalogContextMenu(const QPoint &pos);
    void onAddPublication(const QString &forceType = QString());
    void onEditSourceFromToolbar();
    void onDeleteSourceFromToolbar();
    void onOpenPaperAttachment(int row, int column);
    void onViewPaperDetail();
    void onPaperSelectionChanged();
    void onOpenFullText();
    void onUploadPaperNote();
    void onOpenSelectedPaperNote();
    void onSave();
    void onLoad();
    void onImport();

private:
    enum class LibraryNodeType {
        PaperSystem,
        PaperCatalog,
        AuthorSystem,
        AuthorCatalog,
        PublicationRoot,
        PublicationGroup
    };

    enum class MainContentMode {
        Papers,
        Authors,
        Publications
    };

    struct LibraryNodeInfo {
        LibraryNodeType type = LibraryNodeType::PaperSystem;
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
    void updateAuthorTable(const std::vector<Author> &authors);
    void updateDetailPanel(IdType paperId);
    void showSourceDetail(IdType sourceId);
    void updateAuthorDetailPanel(IdType authorId);
    void clearDetailPanel();
    void restoreDetailPanelLabels();
    std::vector<Paper> collectCurrentPapers() const;
    std::vector<Author> collectCurrentAuthors() const;
    std::vector<Paper> filterPapers(const std::vector<Paper> &papers, const QString &keyword) const;
    std::vector<Author> filterAuthors(const std::vector<Author> &authors, const QString &keyword) const;
    QString paperAuthorsText(const Paper &paper) const;
    QString paperKeywordsText(const Paper &paper) const;
    QString paperCatalogText(const Paper &paper) const;
    QString paperUploadTimeText(const Paper &paper) const;
    QString authorResearchAreasText(const Author &author) const;
    QString authorPapersText(IdType authorId) const;
    void ensureUploadTime(Paper &paper) const;
    void addPaperToCurrentCatalog(IdType paperId) const;
    void addAuthorToCurrentCatalog(IdType authorId) const;
    bool moveSelectedPaperToCatalog(IdType catalogId);
    IdType selectedPaperId() const;
    IdType selectedSourceId() const;
    IdType selectedAuthorId() const;
    std::vector<IdType> selectedPaperIds() const;
    std::vector<IdType> selectedAuthorIds() const;
    void configureTableForMode(MainContentMode mode);
    bool isUserCatalogNode(const QTreeWidgetItem *item) const;
    bool isUserAuthorCatalogNode(const QTreeWidgetItem *item) const;
    bool isSystemNode(const QTreeWidgetItem *item) const;
    IdType currentCatalogId() const;
    IdType currentAuthorCatalogId() const;
    QString currentNodeKey() const;
    bool isAuthorMode() const;
    bool isPublicationMode() const;
    void updateContentModeForNode(const QTreeWidgetItem *item);
    void updateToolbarForMode();
    void setDetailRowLabel(QWidget *field, const QString &text);
    void setDetailRowVisible(QWidget *field, bool visible);
    void setAttachmentControlsVisible(bool visible);
    void refreshNoteControls(IdType paperId);
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
    QLabel *m_detailHeadingLabel = nullptr;
    QLabel *m_attachmentSectionLabel = nullptr;
    QFormLayout *m_detailForm = nullptr;
    QWidget *m_fullTextRowWidget = nullptr;
    QWidget *m_noteRowWidget = nullptr;
    QPushButton *m_openFullTextButton = nullptr;
    QComboBox *m_noteComboBox = nullptr;
    QPushButton *m_uploadNoteButton = nullptr;
    QPushButton *m_openNoteButton = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QString m_defaultDataPath;
    QString m_currentNodeKey;
    MainContentMode m_contentMode = MainContentMode::Papers;
    MainContentMode m_lastConfiguredTableMode = MainContentMode::Papers;
    IdType m_detailPaperId = INVALID_ID;
    IdType m_detailAuthorId = INVALID_ID;
    QAction *m_addPaperAction = nullptr;
    QAction *m_addAuthorAction = nullptr;
    QAction *m_editPaperAction = nullptr;
    QAction *m_deletePaperAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_loadAction = nullptr;
    QAction *m_importAction = nullptr;
    QPushButton *m_viewDetailButton = nullptr;
    QPushButton *m_searchButton = nullptr;
    QPushButton *m_showAllButton = nullptr;
    QPushButton *m_selectAllButton = nullptr;
};

#endif // MAINWINDOW_H
