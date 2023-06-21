#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QProgressDialog>
#include "choosetime.h"
#include "blogdatabase.h"

extern QFont *iconFont;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
signals:
    void refreshUISignal();
    void pageBtSearchRefreshUISignal(QString bt, int index);
    void locateSearchSignal(QString sort);
    void timeSortSearchSignal(QString sort);
    void classifySearchSignal(QString classify);
    void keywordSearchSignal(QString text);
public slots:
    void refreshUISlot();
    void pageBtSearchRefreshUISlot(QString bt, int index);
    void pageBtSearchSlot(QString bt, int index);
    void keywordSearchSlot(QString keyword);
    void timeSearchSlot(QDate startDate, QTime startTime, QDate endDate, QTime endTime);
    void locateSearchSlot(QString sort);
    void timeSortSearchSlot(QString sort);
    void classifySearchSlot(QString classify);
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QProgressDialog *progressDialog;
    ChooseTime *chooseTime;
    SEARCHPojo *search_pojo;
    QList<MDPojo *> *searchRes;
    static void setIcon(QLabel *lab, QChar chr, int size, QColor color = nullptr)
    {
        if(color != nullptr)
        {
            QPalette pe;
            pe.setColor(QPalette::WindowText, color);
            lab->setPalette(pe);
        }
        iconFont->setPointSize(size);
        lab->setFont(*iconFont);
        lab->setText(chr);
    }
    //时间戳转字符串
    static QString millsToDateStr(qint64 mills)
    {
        //yyyy-MM-dd hh:mm:ss.zzz
        mills += 8 * 60 * 60 * 1000;
        QString str = QDateTime::fromMSecsSinceEpoch(mills, Qt::UTC).toString("yyyy-MM-dd");
        return str;
    }
    static QString millsToDateAllStr(qint64 mills)
    {
        //yyyy-MM-dd hh:mm:ss.zzz
        mills += 8 * 60 * 60 * 1000;
        QString str = QDateTime::fromMSecsSinceEpoch(mills, Qt::UTC).toString("yyyy-MM-dd hh:mm:ss");
        return str;
    }
    //根据指定的年月日获取时间戳
    static qint64 getMillsByYMD(int year, int month, int day)
    {
        QTime time(0, 0, 0);
        QDate date(year, month, day);
        QDateTime dateTime(date, time, Qt::UTC);
        QDateTime res = dateTime.addSecs(-8 * 60 * 60);
//        qDebug() << "time:" << res.toMSecsSinceEpoch();
        return res.toMSecsSinceEpoch();
    }
    static qint64 getMillsByDT(QDate date, QTime time)
    {

        QDateTime dateTime(date, time, Qt::UTC);
        QDateTime res = dateTime.addSecs(-8 * 60 * 60);
        //        qDebug() << "time:" << res.toMSecsSinceEpoch();
        return res.toMSecsSinceEpoch();
    }


public:
    Ui::MainWindow *ui;
};

class SearchPageButton : public QPushButton
{
    Q_OBJECT

public:
    explicit SearchPageButton(QWidget *parent = nullptr);

signals:
    void pageClicked(QString bt, int index);
public slots:
    void pageClickedSlot();
private:
};
#endif // MAINWINDOW_H
