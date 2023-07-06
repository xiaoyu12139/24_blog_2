#include "mainwindow.h"
#include <QDir>
#include <QDebug>
#include "ui_mainwindow.h"
#include "clickablelabel.h"
#include "ui_choose_time.h"
#include <QThread>
#include <QMessageBox>
#include "searchitem.h"
#include <QtConcurrent/QtConcurrent>

bool checkPath(const QString &path, QString error_msg, bool flag);
extern BlogDatabase *blogDB;
QThread *work_thread = nullptr;
MainWindow *mainwindow = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mainwindow = this;
    this->setWindowTitle("blogsearch");
    this->setWindowIcon(QIcon(":/img/24_blog_logo.png"));
    chooseTime = new ChooseTime(this);
    connect(ui->time, &ClickableLabel::clicked, [ = ]()
    {
        chooseTime->ui->start_date->setDate(QDate::fromString(chooseTime->startDate, "yyyy/MM/dd"));
        chooseTime->ui->start_time->setTime(QTime::fromString(chooseTime->startTime, "HH:mm:ss"));
        chooseTime->ui->end_date->setDate(QDate::fromString(chooseTime->endDate, "yyyy/MM/dd"));
        chooseTime->ui->end_time->setTime(QTime::fromString(chooseTime->endTime, "HH:mm:ss"));
        chooseTime->show();
    });
    qint64 default_time_start = MainWindow::getMillsByYMD(2000, 12, 1);
    qint64 default_time_end = MainWindow::getMillsByYMD(2200, 12, 1);
    search_pojo = new SEARCHPojo("", "0", "0", "0", "", "title_content"
                                 , QString::number(default_time_start), QString::number(default_time_end), "DESC");
    ui->time->setToolTip(MainWindow::millsToDateAllStr(search_pojo->time_start.toLongLong())
                         + "-" + MainWindow::millsToDateAllStr(search_pojo->time_end.toLongLong()));
    ui->sum->setText("共0条");
    ui->page->setText("0/0");
    ui->time->setText("2000/12/01-2200/12/01");
    connect(chooseTime, &ChooseTime::timeSearchBlogSignal, this, &MainWindow::timeSearchSlot);
    QList<QString> *classifyList = blogDB->getClassifyList();
    qDebug() << "classify_list: " + QString::number(classifyList->size());
    ui->classify->clear();
    ui->classify->addItem("任意");
    ui->classify->addItem("default");
    QList<QString>::const_iterator it;
    for (it = classifyList->constBegin(); it != classifyList->constEnd(); ++it)
    {
        QString value = *it;
        // 处理value
        ui->classify->addItem(value);
    }
    bool classify_cb = true;
    bool time_cb = true;
    bool target_cb = true;
    connect(ui->classify, &QComboBox::currentTextChanged, [&](QString text)
    {
//        if (classify_cb)
//        {
//            classify_cb = false;
//            return;
//        }
        emit classifySearchSignal(text);
    });
    connect(ui->target, &QComboBox::currentTextChanged, [ & ](QString text)
    {
//        if (target_cb)
//        {
//            target_cb = false;
//            return;
//        }
        emit locateSearchSignal(text);
    });
    connect(ui->time_sort, &QComboBox::currentTextChanged, [ & ](QString text)
    {
//        if (time_cb)
//        {
//            time_cb = false;
//            return;
//        }
        emit timeSortSearchSignal(text);
    });
    connect(this, &MainWindow::timeSortSearchSignal, this, &MainWindow::timeSortSearchSlot);
    connect(this, &MainWindow::locateSearchSignal, this, &MainWindow::locateSearchSlot);
    connect(this, &MainWindow::classifySearchSignal, this, &MainWindow::classifySearchSlot);
    connect(this, &MainWindow::keywordSearchSignal, this, &MainWindow::keywordSearchSlot);
    connect(this, &MainWindow::refreshUISignal, this, &MainWindow::refreshUISlot);
    connect(this, &MainWindow::pageBtSearchRefreshUISignal, this, &MainWindow::pageBtSearchRefreshUISlot);
    connect(ui->up_bt, &QPushButton::clicked, [ = ]()
    {
        int bt = search_pojo->page.toInt() - 1 == 0 ? 1 : search_pojo->page.toInt() - 1;
        int index = 1;
        SearchPageButton tmp;
        QList<SearchPageButton *> forDelete_bt = ui->bottom->findChildren<SearchPageButton *>();
        foreach(SearchPageButton *forDeleteItem_bt, forDelete_bt)
        {
            if(forDeleteItem_bt->text() == bt)
            {
                index = this->ui->page_bts_list->indexOf(forDeleteItem_bt);
            }
        }
        emit tmp.pageClicked(QString::number(bt), index);
    });
    connect(ui->down_bt, &QPushButton::clicked, [ = ]()
    {
        int bt = search_pojo->page.toInt() + 1 == search_pojo->pageSize.toInt()
                 ? search_pojo->pageSize.toInt() : search_pojo->page.toInt() + 1;
        int index = 1;
        SearchPageButton tmp;
        QList<SearchPageButton *> forDelete_bt = ui->bottom->findChildren<SearchPageButton *>();
        foreach(SearchPageButton *forDeleteItem_bt, forDelete_bt)
        {
            if(forDeleteItem_bt->text() == bt)
            {
                index = this->ui->page_bts_list->indexOf(forDeleteItem_bt);
            }
        }
        emit tmp.pageClicked(QString::number(bt), index);
    });
    SearchItem *item = new SearchItem(nullptr, ui->are);
//        qDebug() << ui->are->children().size() - 2;
    ui->list->insertWidget(ui->are->children().size() - 2, item);
    connect(ui->search_button, &QPushButton::clicked, [ = ]()
    {
        QString text = ui->search_input->text();
        text = text.trimmed();
        if(text == "")
        {
            QMessageBox::information(this, "提示", "输入不能为空");
            return;
        }
        emit keywordSearchSignal(text);
    });
    connect(ui->search_input, &QLineEdit::returnPressed, [ = ]()
    {
        QString text = ui->search_input->text();
        text = text.trimmed();
        if(text == "")
        {
            QMessageBox::information(this, "提示", "输入不能为空");
            return;
        }
        emit keywordSearchSignal(text);
    });
    connect(ui->ok_input_buton, &QPushButton::clicked, [ = ]()
    {
        QString text = ui->search_input->text();
        search_pojo->key_word = text.trimmed();
    });
    // 创建等待界面窗口
    progressDialog = new QProgressDialog(this);
    progressDialog->setMinimumWidth(300);
    progressDialog->setWindowTitle("loading");
    progressDialog->setLabelText("正在查询数据库，请稍候...");
    progressDialog->setWindowModality(Qt::WindowModal); // 设置为模态窗口
    progressDialog->close();
    connect(progressDialog, &QProgressDialog::canceled, [ = ]()
    {
        qDebug() << "Dialog canceled!";
        // 在这里执行取消操作的逻辑
        if(work_thread != nullptr)
        {
            work_thread->quit();
            work_thread->wait();
        }
    });
    searchRes = new QList<MDPojo *>();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::refreshUISlot()
{
    ui->search_input->setText(search_pojo->key_word);
    search_pojo->page = "1";
    ui->page->setText(search_pojo->page + "/" + search_pojo->pageSize);
    ui->sum->setText("共" + search_pojo->docSize + "条");
    progressDialog->setValue(51);//设置进度条
    //添加搜索结果
    QList<SearchItem *> forDelete = ui->are->findChildren<SearchItem *>();
    foreach(SearchItem *forDeleteItem, forDelete)
    {
        forDeleteItem->deleteLater();
    }
    for(auto it = searchRes->begin(); it != searchRes->end(); ++it)
    {
        SearchItem *item = new SearchItem(*it, ui->are);
//        qDebug() << ui->are->children().size() - 2;
        ui->list->insertWidget(ui->are->children().size() - 2, item);
    }

    QList<SearchPageButton *> forDelete_bt = ui->bottom->findChildren<SearchPageButton *>();
    foreach(SearchPageButton *forDeleteItem_bt, forDelete_bt)
    {
        forDeleteItem_bt->deleteLater();
    }
    qint64 n = search_pojo->pageSize.toInt() > 10 ? 10 : search_pojo->pageSize.toInt();
    for(qint64 i = 1; i <= n; i ++)
    {
        SearchPageButton *bt = new SearchPageButton;
        bt->setText(QString::number(i, 10));
        ui->page_bts_list->addWidget(bt);
        if(i == 1)
        {
            bt->setStyleSheet("background-color: rgba(170, 187, 203, 150);");
        }
    }
    progressDialog->setValue(100);//设置进度条
    progressDialog->close();
}

void MainWindow::pageBtSearchRefreshUISlot(QString bt, int index)
{
    ui->search_input->setText(search_pojo->key_word);
    ui->page->setText(search_pojo->page + "/" + search_pojo->pageSize);
    ui->sum->setText("共" + search_pojo->docSize + "条");
    progressDialog->setValue(51);//设置进度条
    //添加搜索结果
    QList<SearchItem *> forDelete = ui->are->findChildren<SearchItem *>();
    foreach(SearchItem *forDeleteItem, forDelete)
    {
        forDeleteItem->deleteLater();
    }
    for(auto it = searchRes->begin(); it != searchRes->end(); ++it)
    {
        SearchItem *item = new SearchItem(*it, ui->are);
//        qDebug() << ui->are->children().size() - 2;
        ui->list->insertWidget(ui->are->children().size() - 2, item);
    }
    //设置页面切换按钮，清空按钮列表中的按钮
    QList<SearchPageButton *> forDelete_bt = ui->bottom->findChildren<SearchPageButton *>();
    foreach(SearchPageButton *forDeleteItem_bt, forDelete_bt)
    {
        forDeleteItem_bt->deleteLater();
    }
    //当前点击的按钮为bt,index为bt的位置
    int curPage = bt.toInt();
    int headPage = curPage - index;
    int tailPage = headPage + 9;
    if(index > 4)
    {
        int dif = index - 4;
        headPage += dif;
        tailPage += dif;
        if(tailPage > search_pojo->pageSize.toInt())
        {
            headPage = search_pojo->pageSize.toInt() - 9;
            tailPage = search_pojo->pageSize.toInt();
        }
    }
    else
    {
        int dif = 4 - index;
        headPage -= dif;
        tailPage -= dif;
        if(headPage < 1)
        {
            headPage = 1;
            tailPage = 10 > search_pojo->pageSize.toInt() ? search_pojo->pageSize.toInt() : 10;
        }
    }
    for(int i = headPage; i <= tailPage; i++)
    {
        SearchPageButton *bt = new SearchPageButton;
        bt->setText(QString::number(i, 10));
        ui->page_bts_list->addWidget(bt);
        if(i == curPage)
        {
            bt->setStyleSheet("background-color: rgba(170, 187, 203, 150);");
        }
    }
    progressDialog->setValue(100);//设置进度条
    progressDialog->close();
}

void MainWindow::pageBtSearchSlot(QString bt, int index)
{
    search_pojo->page = bt;
    work_thread = QThread::create([ = ]()
    {
        //进行相关查询后更新数据，然后发送信号更新ui
        delete searchRes;
        searchRes = blogDB->searchBlog(search_pojo);
        emit pageBtSearchRefreshUISignal(bt, index);
    });
    progressDialog->show();
    work_thread->start();
}

void MainWindow::keywordSearchSlot(QString keyword)
{
    search_pojo->key_word = keyword;
    work_thread = QThread::create([ = ]()
    {
        //进行相关查询后更新数据，然后发送信号更新ui
        delete searchRes;
        searchRes = blogDB->searchBlog(search_pojo);
        emit refreshUISignal();
    });
    progressDialog->show();
    work_thread->start();
}

void MainWindow::timeSearchSlot(QDate startDate, QTime startTime, QDate endDate, QTime endTime)
{
    chooseTime->startDate = startDate.toString("yyyy/MM/dd");
    chooseTime->startTime = startTime.toString("HH:mm:ss");
    chooseTime->endDate = endDate.toString("yyyy/MM/dd");
    chooseTime->endTime = endTime.toString("HH:mm:ss");
    ui->time->setText(chooseTime->startDate + "-" + chooseTime->endDate);
    qint64 start = getMillsByDT(startDate, startTime);
    qint64 end = getMillsByDT(endDate, endTime);
    search_pojo->time_start = QString::number(start);
    search_pojo->time_end = QString::number(end);
    ui->time->setToolTip(MainWindow::millsToDateAllStr(search_pojo->time_start.toLongLong())
                         + "-" + MainWindow::millsToDateAllStr(search_pojo->time_end.toLongLong()));

    work_thread = QThread::create([ = ]()
    {
        //进行相关查询后更新数据，然后发送信号更新ui
        delete searchRes;
        searchRes = blogDB->searchBlog(search_pojo);
        emit refreshUISignal();
    });
    progressDialog->show();
    work_thread->start();
}

void MainWindow::locateSearchSlot(QString sort)
{
    if(sort == "任意")
    {
        search_pojo->search_position = "title_content";
    }
    else if(sort == "标题")
    {
        search_pojo->search_position = "title";
    }
    else
    {
        search_pojo->search_position = "content";
    }

    work_thread = QThread::create([ = ]()
    {
        //进行相关查询后更新数据，然后发送信号更新ui
        delete searchRes;
        searchRes = blogDB->searchBlog(search_pojo);
        emit refreshUISignal();
    });
    progressDialog->show();
    work_thread->start();
}

void MainWindow::timeSortSearchSlot(QString sort)
{
    if(sort == "时间降序")
    {
        search_pojo->time_sort = "DESC";
    }
    else
    {
        search_pojo->time_sort = "ASC";
    }
    work_thread = QThread::create([ = ]()
    {
        //进行相关查询后更新数据，然后发送信号更新ui
        delete searchRes;
        searchRes = blogDB->searchBlog(search_pojo);
        emit refreshUISignal();
    });
    progressDialog->show();
    work_thread->start();
}

void MainWindow::classifySearchSlot(QString classify)
{
    if(classify == "任意")
    {
        search_pojo->classify = "";
    }
    else
    {
        search_pojo->classify = classify;
    }
    work_thread = QThread::create([ = ]()
    {
        //进行相关查询后更新数据，然后发送信号更新ui
        delete searchRes;
        qDebug() << "keyword: " << search_pojo->key_word;
        searchRes = blogDB->searchBlog(search_pojo);
        emit refreshUISignal();
    });
    progressDialog->show();
    work_thread->start();
}

SearchPageButton::SearchPageButton(QWidget *parent)
    : QPushButton(parent)
{
    this->setMinimumWidth(35);
    connect(this, &SearchPageButton::clicked, this, &SearchPageButton::pageClickedSlot);
    connect(this, &SearchPageButton::pageClicked, mainwindow, &MainWindow::pageBtSearchSlot);
}
void SearchPageButton::pageClickedSlot()
{
    int index = mainwindow->ui->page_bts_list->indexOf(this);
    emit pageClicked(this->text(), index);
}

bool checkPath(const QString &path, QString error_msg, bool flag)
{
    QDir dir(path);
    if (dir.exists())
    {
        return true;
    }
    else
    {
        if(flag)
        {
//            showMSG(error_msg);
            QMessageBox::information(mainwindow, "提示", error_msg);
        }
//        qDebug() << error_msg;
        return false;
    }
}


