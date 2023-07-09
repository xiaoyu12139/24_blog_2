#ifndef STARTEDITAPP_H
#define STARTEDITAPP_H

#include <QWidget>
#include <QProcess>
#include <QWindow>
#include <windows.h>
#include "blogdatabase.h"


class StartEditApp: public QObject
{
    Q_OBJECT
public:
    QWidget *qt_w;//qt窗口
    QApplication *q_app;
    QProcess *process;// 创建QProcess对象
    HWINEVENTHOOK hHook_pos;
    HWINEVENTHOOK hHook_z;
    HWINEVENTHOOK hHook_min;
    HWINEVENTHOOK hHook_dragwinend;
//    QThread *thread;
    StartEditApp(QWidget &w, QApplication &q_app, QObject *parent = nullptr);
    ~StartEditApp();
    int start();
//    LRESULT CALLBACK CallWndRetProc(int nCode, WPARAM wParam, LPARAM lParam);
public slots:
    void waitProcessFinish(int exitCode);
signals:
    void modifySaveButtonText(QString);
};

QWindow *getWindowFromProcessId(qint64 processId);
HWND GetHwndByPid(DWORD dwProcessID);

#endif // STARTEDITAPP_H
