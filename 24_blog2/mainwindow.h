#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include "blogclassifydialog.h"
QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
//    bool checkPath(const QString &path, QString error_msg);
    BlogClassifyDialog *blogClassifyDialog;

public slots:
    void saveClicked();
    void deleteClicked();
    void searchClicked();
    void otherClicked();
    void saveSlot(QString select_classify, QList<QString> *classify_list);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
