#include "mainwindow.h"
#include "starteditapp.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QLoggingCategory>
#include <QDateTime>
#include <QSettings>
#include <QTimer>
#include <QSqlDatabase>
#include "config.h"
#include "blogdatabase.h"

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void setLogging();
bool readConfig();
bool writeConfig();
BlogDatabase *blogDB = nullptr;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    setLogging();//设置日志文件
    blogDB = new BlogDatabase();
    blogDB->transaction();
    blogDB->createMDTable();
    blogDB->createIMGTable();
    blogDB->createCLASSIFYTable();
    blogDB->commit();
    init_config();
    if(!readConfig())//读取配置文件
    {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }
    MainWindow mainWindow;
    mainWindow.show();
//    QObject::connect(&mainWindow, &QMainWindow::destroyed, &app, &QCoreApplication::quit);
    QObject::connect(&mainWindow, &QMainWindow::destroyed, [ = ]()
    {
        writeConfig();

    });
    StartEditApp s(mainWindow, app);
    s.start();
    return app.exec();
}

bool writeConfig()
{
    QString configFilePath = "config.ini";
    QFile file(configFilePath);
    if(!file.exists())
    {
        qDebug() << configFilePath << "配置文件丢失，写入失败";
        return false;
    }
    QSettings settings(configFilePath, QSettings::IniFormat);
    settings.beginGroup("BLOG_SETTING");
    settings.setValue("EDIT_PATH", g_editapp_path);
    settings.setValue("MAINWINDOW_X_ADJUST", QString::number(g_main_window_x_adjust));
    settings.setValue("MAINWINDOW_Y_ADJUST", QString::number(g_main_window_y_adjust));
    settings.setValue("WAIT_EDITAPP_TIME", QString::number(g_wait_open_editapp_time));
    settings.endGroup();
    return true;
}

bool readConfig()
{
    QString configFilePath = "config.ini";
    QFile file(configFilePath);
    if(!file.exists())
    {
        qDebug() << configFilePath << "配置文件丢失，启动失败";
        return false;
    }
    QSettings settings(configFilePath, QSettings::IniFormat);
    settings.beginGroup("BLOG_SETTING");
    g_editapp_path = settings.value("EDIT_PATH", g_editapp_path).toString();
    g_main_window_x_adjust = settings.value("MAINWINDOW_X_ADJUST", g_main_window_x_adjust).toInt();
    g_main_window_y_adjust = settings.value("MAINWINDOW_Y_ADJUST", g_main_window_y_adjust).toInt();
    g_wait_open_editapp_time = settings.value("WAIT_EDITAPP_TIME", g_wait_open_editapp_time).toInt();
    g_data_path = settings.value("DATA_PATH", g_data_path).toString();
    qDebug() << "EDIT_PATH: " << g_editapp_path;
    qDebug() << "MAINWINDOW_X_ADJUST: " << g_main_window_x_adjust;
    qDebug() << "MAINWINDOW_Y_ADJUST: " << g_main_window_y_adjust;
    qDebug() << "WAIT_EDITAPP_TIME: " << g_wait_open_editapp_time;
    qDebug() << "DATA_PATH: " << g_data_path;
    settings.endGroup();
    return true;
}

/**
 * 设置输出日志文件
 */
QFile file("log.txt");

void setLogging()
{
    if(file.exists())
    {
        qDebug() << "File log.txt exists";
        if (!file.open(QIODevice::Append | QIODevice::Text))
        {
            qDebug() << "File log.txt can not write";
            return ;
        }
    }
    else
    {
        qDebug() << "File log.txt does not exist";
        if(file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream stream(&file);
            stream << "New log file created." << endl;
            file.close();
        }
        else
        {
            qDebug() << "File log.txt can not write";
            return;
        }
    }
    qInstallMessageHandler(customMessageHandler);  // 设置消息处理函数
}


void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTextStream out(stdout);
    out << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "] ";
    switch (type)
    {
        case QtDebugMsg:
            out << "DEBUG: " << msg << endl;
            break;
        case QtWarningMsg:
            out << "WARNING: " << msg << endl;
            break;
        case QtCriticalMsg:
            out << "CRITICAL: " << msg << endl;
            break;
        case QtFatalMsg:
            out << "FATAL: " << msg << endl;
            abort();
    }
    out.flush();

    // 将消息输出到文件
    QTextStream fileOut(&file);
    fileOut << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "] ";
    switch (type)
    {
        case QtDebugMsg:
            fileOut << "DEBUG: " << msg << endl;
            break;
        case QtWarningMsg:
            fileOut << "WARNING: " << msg << endl;
            break;
        case QtCriticalMsg:
            fileOut << "CRITICAL: " << msg << endl;
            break;
        case QtFatalMsg:
            fileOut << "FATAL: " << msg << endl;
            abort();
    }
    fileOut.flush();
}
