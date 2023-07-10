#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include "config.h"
#include <QPropertyAnimation>
#include "windows.h"
#include <QDir>
#include <QThread>
#include <QProcess>
#include "blogdatabase.h"
extern HWND edit_hwnd;
extern HWND main_hwnd;
HWND tmp_main_hwnd;
extern qint64 edit_pid;
extern BlogDatabase *blogDB;
bool checkPath(const QString &path, QString error_msg, bool flag);
MainWindow *mainWindow = nullptr;
void showMSG(QString error_msg);
bool confirmMSG(QString error_msg);
void deleteFile(QString filePath);
MDPojo *main_mdpojo = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mainWindow = this;
    this->setWindowFlags(windowFlags() | Qt::Tool);
    this->setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    this->setAttribute(Qt::WA_QuitOnClose, true);
    this->setWindowTitle("blog");
    this->resize(80, 40);
//    this->setMinimumSize(QSize(80, 40));
//    this->setMaximumSize(QSize(80, 40));
    connect(ui->open_close, &QPushButton::clicked, [ = ]()
    {
        // 获取包含边框和标题栏的矩形区域
        QRect frameRect = this->frameGeometry();
        // 将左上角坐标转换为屏幕坐标
        int titleBarHeight = frameRect.height() - geometry().height();
        int x = frameRect.x();
        int y = frameRect.y() + titleBarHeight;
        QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
        if(ui->open_close->text() == "折叠")
        {
            animation->setDuration(500);
            animation->setStartValue(QRect(x, y, this->width(), this->height()));
            animation->setEndValue(QRect(x, y, this->width(), 40));
            animation->start();
            ui->open_close->setText("展开");
        }
        else
        {
            animation->setDuration(500);
            animation->setStartValue(QRect(x, y, this->width(), this->height()));
            animation->setEndValue(QRect(x, y, this->width(), 200));
            animation->start();
            ui->open_close->setText("折叠");
        }

    });
    connect(ui->save, &QPushButton::clicked, this, &MainWindow::saveClicked);
    connect(ui->delete_bt, &QPushButton::clicked, this, &MainWindow::deleteClicked);
    connect(ui->search, &QPushButton::clicked, this, &MainWindow::searchClicked);
    connect(ui->other, &QPushButton::clicked, this, &MainWindow::otherClicked);
    blogClassifyDialog = new BlogClassifyDialog(this);
    connect(blogClassifyDialog, &BlogClassifyDialog::saveSignal, this, &MainWindow::saveSlot);
//    blogClassifyDialog = new BlogClassifyDialog(nullptr);
}

// 禁止窗口缩放
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if(this->size().height() > 200)
    {
        this->resize(80, 200);
        ui->open_close->setText("折叠");
    }
    if(this->size().height() < 40)
    {
        this->resize(80, 40);
        ui->open_close->setText("展开");
    }
    if(this->size().width() != 80)
    {
        this->resize(80, this->height());
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (event->spontaneous())
    {
        // 点击窗口关闭按钮关闭窗口
        qDebug() << "Window closed by clicking close button";
        QCoreApplication::quit();
        QMainWindow::closeEvent(event);
    }
    else
    {
        // 调用this->close()关闭窗口
        qDebug() << "Window closed by calling this->close()";
        // 将窗口设为不可见
        // 等待一小段时间，让窗口变得不可见
        tmp_main_hwnd = main_hwnd;
        main_hwnd = NULL;
        Sleep(100); // 暂停 100 毫秒
        // 关闭窗口
        this->hide();
        event->ignore();
    }

    // 执行其他关闭处理
    // ...

    // 调用基类的closeEvent函数继续执行默认的关闭操作

}

MainWindow::~MainWindow()
{
    delete ui;
    delete blogClassifyDialog;
    qDebug() << "main window destroy";
}

void MainWindow::modifySaveButtonTextSlot(QString)
{
    ui->save->setText("更新");
}

/**
 * @brief MainWindow::saveSlot
 * @param select_classify
 * @param classify_list
 * 更新分类列表
 * 读取数据库中的分类列表，对比数据库中的分类列表和当前的分类列表的差别
 * 如果数据库中有的，但当前没有则可删除（如果当前分类下没有文件则删除，否则删除不了）contains来判断qlist是否存在
 * 如果数据库中没有，但当前有了则添加
 * 选中分类不在当前列表中，则更新选中分类为default
 *
 * 保存文件至指定分类
 */
void MainWindow::saveSlot(QString select_classify, QList<QString> *classify_list)
{
//    qDebug() << classify_list->size();
    // 开启事务
    blogDB->transaction();
    QList<QString> *db_classify_list = blogDB->getClassifyList();
    QList<QString>::const_iterator it;
    //查找在list但不在db_list中的item
    for (it = classify_list->constBegin(); it != classify_list->constEnd(); ++it)
    {
        QString value = *it;
        if(!db_classify_list->contains(value))
        {
            if(!blogDB->addClassifyToDB(value))
            {
                showMSG("新增分类: " + value + " 时出错");
                return;
            }
        }
    }
    //查找在db_list但不在list中的item
    for (it = db_classify_list->constBegin(); it != db_classify_list->constEnd(); ++it)
    {
        QString value = *it;
        if(!classify_list->contains(value))
        {
            if(!blogDB->deleteClassifyFromDB(value))
            {
                showMSG("删除分类: " + value + " 时出错，可能当前分类下存在文档无法删除成功");
                return;
            }
        }
    }

    if(!classify_list->contains(select_classify))
    {
        select_classify = "default";
    }

    //    this->setWindowTitle("blog");
    QString save_path = g_data_path + QDir::separator() + "forup";
    qDebug() << save_path;
    if(!checkPath(save_path, "数据文件夹: " + save_path + " 路径不存在", true))
    {
        return;
    }
    //遍历文件夹
    QDir dir(save_path);

    // 设置过滤器，只显示文件
    dir.setFilter(QDir::Files);

    // 设置排序方式，按名称升序排序
    dir.setSorting(QDir::Name);

    bool update_data = false;
    if(main_mdpojo != nullptr)
    {
        qDebug() << "更新数据";
        update_data = true;
        QString filePath = save_path + QDir::separator() + "123.md";
        int index = filePath.lastIndexOf(QDir::separator());  // 找到最后一个斜杠的位置
        QString newPath = filePath.left(index);  // 获取斜杠之前的部分
        index = newPath.lastIndexOf(QDir::separator());
        newPath = newPath.left(index);
        QString tmp = "";
        newPath.append(tmp + QDir::separator() + "forshow");
        newPath.append(tmp + QDir::separator() + main_mdpojo->title);
        newPath.append(tmp + QDir::separator() + main_mdpojo->title);
        qDebug() << "更新数据: " + newPath;
        if(!blogDB->uploadMDToDB(newPath, select_classify))
        {
            showMSG("更新失败，可能图片链接下载失败、图片文件不存在、当前文件资源被占用");
        }
        else
        {
            showMSG("文件更新成功");
        }
        blogDB->commit();
        return;
    }

    bool flag = false;
    // 遍历文件夹中的文件
    QList<QString> *upSuccess = new QList<QString>();
    QFileInfoList fileInfoList = dir.entryInfoList();
    foreach (QFileInfo fileInfo, fileInfoList)
    {
        if (fileInfo.suffix() == "md")
        {
            flag = true;
            // 文件后缀为.md，执行相应操作
            QString filePath = fileInfo.absoluteFilePath();
            qDebug() << "Found markdown file: " << filePath;
            // 将文件上传到数据库
            if(!blogDB->uploadMDToDB(filePath, select_classify))
            {
                QString msg = filePath + "上传失败，可能文件名已存在、图片链接下载失败、图片文件不存在、当前文件资源被占用";
                if(update_data)
                {
                    msg = "当前文件更新成功，但" + msg;
                }
                showMSG(msg);
                return;
            }
            upSuccess->append(filePath);
        }
    }
    // 提交事务
    blogDB->commit();
    for (it = upSuccess->constBegin(); it != upSuccess->constEnd(); ++it)
    {
        QString value = *it;
        deleteFile(value);
    }
    if(update_data && flag)
    {
        showMSG("文件上传更新成功");
    }
    else if(update_data)
    {
        showMSG("文件更新成功");
    }
    else if(flag)
    {
        showMSG("文件上传成功");
    }
    else
    {
        showMSG("文件上传失败: 路径" + save_path + "下没有markdown文件，请确保当前编辑markdown文件已经保存至该路径下");
    }
}

/**
 * @brief MainWindow::saveClicked 保存markdown文件到数据库，生成对应的uuid
 * 先在markdown编辑器中将当前编辑的markdown文件保存到指定的文件夹，然后在读取该文件并保存到数据库中
 */
void MainWindow::saveClicked()
{
    qDebug() << main_hwnd;
    mainWindow->close();
//    QMessageBox::information(this, "Title", "Message");
    blogClassifyDialog->refreshClassifyList();
    if(mainWindow != nullptr)
    {
        blogClassifyDialog->selectClassify(main_mdpojo->classify);
    }
    blogClassifyDialog->show();
    // 创建事件循环
    QEventLoop loop;
    // 连接窗口的关闭信号到事件循环的退出槽
    QObject::connect(blogClassifyDialog, &BlogClassifyDialog::closeWindow, &loop, &QEventLoop::quit);
    // 进入事件循环，直到窗口关闭
    loop.exec();
    Sleep(100);
    mainWindow->show();
    main_hwnd = tmp_main_hwnd;
}

void MainWindow::deleteClicked()
{
    qDebug() << main_hwnd;
    if(confirmMSG("请确定是否删除当前文件?"))
    {
        if(main_mdpojo == nullptr)
        {
            showMSG("删除失败，main_mdpojo为空，读取相关信息失败，可能文件未保存至数据库");
            return;
        }
        blogDB->transaction();
        blogDB->deleteMD(main_mdpojo->id);
        blogDB->commit();
        showMSG("删除成功");
    }
    else
    {
        qDebug() << "取消删除";
    }
}

void MainWindow::searchClicked()
{
//Zm9ydGVzdF91cF9tYXJrZG93bi5tZA==
    blogDB->transaction();
//    blogDB->getMDToDB("Zm9ydGVzdF91cF9tYXJrZG93bi5tZA==");
    QThread *work_thread2 = QThread::create([ = ]()
    {
        QProcess process;
        QString programPath = QCoreApplication::applicationDirPath() + "/24_blog2_search.exe";
        programPath = programPath.replace("/", QDir::separator());
        qDebug() << programPath;
        process.setProgram(programPath);
        process.start();
        if (!process.waitForStarted())
        {
            qDebug() << "Failed to start program: " << process.errorString();
            return ;
        }
        process.waitForFinished(-1);
    });
    work_thread2->start();
    blogDB->commit();
}

void MainWindow::otherClicked()
{
    QString msg = "如果当前文件为新增数据则显示null，如果为更新或查看数据则为相关信息\n";
    if(main_mdpojo == nullptr)
    {
        msg += "main_mdpojo为nullptr，当前文件可能未保存";
    }
    else
    {
        msg += "id: " + main_mdpojo->id + "\n";
//        QByteArray byteArray = QByteArray::fromBase64(main_mdpojo->classify.toUtf8());
//        QString text = QString::fromUtf8(byteArray);
        msg += "classify: " + main_mdpojo->classify + "\n";
        msg += "title: " + main_mdpojo->title + "\n";
        msg += "字符数: " + QString::number(main_mdpojo->content.length()) + "\n";
    }
    showMSG(msg);
}

// 判断路径是否存在，不存在则弹窗提示
//存在返回true
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
            showMSG(error_msg);
        }
//        qDebug() << error_msg;
        return false;
    }
}

bool confirmMSG(QString error_msg)
{
    if(main_hwnd == NULL && tmp_main_hwnd != NULL)
    {
        main_hwnd = tmp_main_hwnd;
    }
    mainWindow->close();
    std::wstring wstr = error_msg.toStdWString();
    const wchar_t *wCharStr = wstr.c_str();
    int result = MessageBox(NULL, wCharStr, TEXT("提示"), MB_ICONQUESTION | MB_YESNO);
    Sleep(100);
    mainWindow->show();
    main_hwnd = tmp_main_hwnd;
    if (result == IDYES)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void showMSG(QString error_msg)
{
//    mainWindow->setWindowTitle("错误");
    qDebug() << error_msg;
    if(main_hwnd == NULL && tmp_main_hwnd != NULL)
    {
        //说明当前mainwindow已经关闭过了，所以状态要重置为未关闭，然后在执行关闭就ok
        main_hwnd = tmp_main_hwnd;
    }
    mainWindow->close();
    std::wstring wstr = error_msg.toStdWString();
    const wchar_t *wCharStr = wstr.c_str();
//    QMessageBox::warning(nullptr, "警告", "这是一个警告消息！", QMessageBox::Ok);
    int result = MessageBox(NULL, wCharStr, TEXT("提示"), MB_OK | MB_ICONINFORMATION);
    //弹窗窗口关闭后将主窗口重置
    if(result == IDOK)
    {
//        qDebug() << "click bt ok";
        // 暂停 100 毫秒
        Sleep(100);
        mainWindow->show();
        main_hwnd = tmp_main_hwnd;
    }
}

void deleteFile(QString filePath)
{
    QFile file(filePath);

    if (file.exists())
    {
        if (file.remove())
        {
            qDebug() << "File: " + filePath + " removed successfully";
        }
        else
        {
            qDebug() << "Error removing file: " << file.errorString();
        }
    }
    else
    {
        qDebug() << "File does not exist";
    }
}




