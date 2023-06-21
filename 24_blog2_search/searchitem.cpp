#include "searchitem.h"
#include "mainwindow.h"
#include "ui_search_item.h"
#include <QDebug>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>

extern BlogDatabase *blogDB;
extern MainWindow *mainwindow;
SearchItem::SearchItem(MDPojo *pojo, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SearchItem)
{
    this->pojo = pojo;
    this->setAttribute(Qt::WA_Hover, true);//开启悬停事件
    this->installEventFilter(this);       //安装事件过滤器
    ui->setupUi(this);
    if(pojo != nullptr)
    {
        QString title = pojo->title;
        int index = title.lastIndexOf(".md");
        if (index != -1)
        {
            title.remove(index, 3); // 移除扩展名 ".md"
        }
        ui->title_light->setText("<h3 style=' margin-top:14px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;'>" \
                                 + title +
                                 "</h3>");
        QString content = pojo->content;
        content = content.mid(0, 100);
        ui->content_label->setText("<p>" + content + "</p>");
        ui->tool_num->setText(pojo->view_count);
        ui->tool_time->setText(MainWindow::millsToDateStr(pojo->edit_time.toLongLong()));
        ui->tool_classify->setText(pojo->classify);
    }
    //设置Icon
    MainWindow::setIcon(ui->view_icon, QChar(0xf06e), 8);
    MainWindow::setIcon(ui->classify_icon, QChar(0xf00b), 8);
    //设置可复制的Label
    ui->title_light->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->content_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->title_light->setContextMenuPolicy(Qt::NoContextMenu);
    ui->content_label->setContextMenuPolicy(Qt::NoContextMenu);
    ui->content_label->adjustSize();
    connect(ui->title_light, &ClickableLabel::clicked, this, [ = ]()
    {
        QThread *work_thread2 = QThread::create([ = ]()
        {
            if(pojo == nullptr)
            {
                return;
            }
            QString id = pojo->id;
            MDPojo *flag = blogDB->getMDToDB(id);
            if(flag == nullptr)
            {
                QMessageBox::information(mainwindow, "提示", "没查询到记录");
                return;
            }
            //然后按照指定路径，使用view_blog打开文件
            QString filePath = g_data_path + QDir::separator() + "forshow";
            filePath = filePath + QDir::separator() + pojo->title;
            QString mdPath = filePath + QDir::separator() + pojo->title;
            //24_blog2.exe mdPath打开文件
//            QCoreApplication::applicationDirPath()
            QProcess process;
            QString programPath = QCoreApplication::applicationDirPath() + "/24_blog2.exe";
            programPath = programPath.replace("/", QDir::separator());
            qDebug() << programPath;
            process.setProgram(programPath);
            QStringList arguments;
            arguments.append(mdPath);
            arguments.append(pojo->id);
            qDebug() << "search_args: " << arguments;
            process.setArguments(arguments);
            process.start();
            if (!process.waitForStarted())
            {
                qDebug() << "Failed to start program: " << process.errorString();
                return ;
            }
            process.waitForFinished(-1);
        });
        work_thread2->start();
    });
}

bool SearchItem::eventFilter(QObject *obj, QEvent *event)
{
    return QWidget::eventFilter(obj, event);
}

SearchItem::~SearchItem()
{
    delete pojo;
    delete ui;
}


