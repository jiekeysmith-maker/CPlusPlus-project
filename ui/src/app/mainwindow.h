#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class QListWidget;
class QStackedWidget;
class QLabel;
class QCloseEvent;
class AuthorPage;
class SourcePage;
class PaperPage;
class AttachmentPage;
class CatalogPage;

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

private slots:
    void onSidebarChanged(int row);
    void onSave();
    void onLoad();

private:
    void setupUi();
    void setupMenuBar();
    void createPages();
    void showStatus(const QString &msg);
    void refreshAllPages();
    QString defaultDataFilePath() const;
    bool saveDefaultData();
    bool loadDefaultData();

    Ui::MainWindow *ui;
    QListWidget    *m_sidebar;
    QStackedWidget *m_stack;
    QString         m_defaultDataPath;

    AuthorPage     *m_authorPage;
    SourcePage     *m_sourcePage;
    PaperPage      *m_paperPage;
    AttachmentPage *m_attachmentPage;
    CatalogPage    *m_catalogPage;
};

#endif // MAINWINDOW_H
