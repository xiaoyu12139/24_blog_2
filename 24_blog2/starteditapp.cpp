#include "starteditapp.h"
#include <windows.h>
#include <iostream>
#include <QCoreApplication>
#include <QProcess>
#include <QWindow>
#include <QScreen>
#include <QDebug>
#include <QGuiApplication>
#include <QApplication>
#include <QThread>
#include <QMetaType>
#include "config.h"
#include "mainwindow.h"
//窗口位置改变回调函数
void CALLBACK WinEventProcPos(HWINEVENTHOOK hHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
void CALLBACK WinEventProcDragWinExit(HWINEVENTHOOK hHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
//窗口z轴改变回调函数
void CALLBACK WinEventProcZ(HWINEVENTHOOK hHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
void CALLBACK WinEventProcMin(HWINEVENTHOOK hHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
HWND edit_hwnd = nullptr;
HWND main_hwnd = nullptr;
bool z_main_flag = false;
bool z_edit_flag = false;
QWidget *win = nullptr;
qint64 edit_pid = 0;
HWND getFirstVisibleWindowHandle();
// 定义全局变量用于保存窗口移动前的位置
RECT g_OldEditWindowRect;
void refreshAjustXY();
extern BlogDatabase *blogDB;
extern MDPojo *main_mdpojo;

StartEditApp::StartEditApp(QWidget &w, QApplication &q_app, QObject *parent)
    : QObject(parent)
{
    //监听位置变化
    // 设置WinEvent钩子，监听EVENT_OBJECT_LOCATIONCHANGE事件
    hHook_pos = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, NULL, WinEventProcPos, 0, 0, WINEVENT_OUTOFCONTEXT);
    hHook_z = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, WinEventProcZ, 0, 0, WINEVENT_OUTOFCONTEXT);
    hHook_min = SetWinEventHook(EVENT_SYSTEM_MINIMIZEEND, EVENT_SYSTEM_MINIMIZEEND, NULL, WinEventProcMin, 0, 0, WINEVENT_OUTOFCONTEXT);
    hHook_dragwinend = SetWinEventHook(EVENT_SYSTEM_MOVESIZEEND, EVENT_SYSTEM_MOVESIZEEND, NULL, WinEventProcDragWinExit, 0, 0, WINEVENT_OUTOFCONTEXT);
    this->qt_w = &w;
    this->q_app = &q_app;
    win = &w;
    this->process = new QProcess(qt_w);
//    this->thread = new QThread(qt_w);
//    qRegisterMetaType<QProcess::ExitStatus> ("QProcess::ExitStatus");
    connect(process, SIGNAL(finished(int)), this, SLOT(waitProcessFinish(int)));
    connect(this, SIGNAL(modifySaveButtonText(QString)), qt_w, SLOT(modifySaveButtonTextSlot(QString)));
    main_hwnd = getFirstVisibleWindowHandle();
    SetWindowPos(main_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

StartEditApp::~StartEditApp()
{
    // 清理钩子
    UnhookWinEvent(hHook_pos);
    UnhookWinEvent(hHook_z);
    UnhookWinEvent(hHook_min);
    UnhookWinEvent(hHook_dragwinend);
}

int StartEditApp::start()
{
    // 要启动的程序路径
    QString programPath = "\"" + g_editapp_path + "\"";

    // 启动程序
    process->setProgram(programPath);
    QStringList arguments = q_app->arguments();
    arguments.removeFirst();
    if(arguments.size() > 1)
    {
        QString id = arguments.at(arguments.size() - 1);
        main_mdpojo = blogDB->getMDToDB(id);
        //main_mdpojo 不为空此时点击保存按钮只能更新数据
        emit this->modifySaveButtonText("更新");
        arguments.removeLast();
    }
    qDebug() << "args: " << arguments;
    if(arguments.size() != 0)
    {
        process->setArguments(arguments);
    }
    process->start();
    if (!process->waitForStarted())
    {
        qDebug() << "Failed to start program: " << process->errorString();
        return 1;
    }
    //获取窗口对象
    QWindow *editWindow = nullptr;
    int waitTime = 10 * g_wait_open_editapp_time;//此处可以配置文件中配置
    while(waitTime > 0 && editWindow == nullptr)
    {
        waitTime--;
        qDebug() << "睡眠100毫秒";
        QThread::msleep(100);
        qDebug() << "睡眠结束，开始判断查找窗口";
        editWindow = getWindowFromProcessId(process->processId());
    }
    if(editWindow == nullptr)
    {
        qDebug() << "编辑窗口对象获取失败,程序直接退出";
        this->qt_w->close();
        return -1;
    }
    else
    {
        qDebug() << "编辑窗口对象获取成功";
//        qDebug() << editWindow;
    }

    edit_pid = process->processId();

    // 获取窗口相关信息
    qDebug() << "Window Title:" << editWindow->title();
    qDebug() << "Window Size:" << editWindow->size();
    qDebug() << "Window Position:" << editWindow->position();
    editWindow->setTitle("this is title");

    // 获取窗口相对于整个屏幕的坐标和大小
    RECT rect;
    GetWindowRect(edit_hwnd, &rect);

    // 获取标题栏的高度
    int titleBarHeight = GetSystemMetrics(SM_CYCAPTION);

    // 计算窗口包含标题栏的位置
    int x = rect.left;
    int y = rect.top + titleBarHeight;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top - titleBarHeight;
    if(g_main_window_x_adjust > width || g_main_window_y_adjust > height + titleBarHeight
            || g_main_window_x_adjust < 0 || g_main_window_y_adjust < -titleBarHeight)
    {
        g_main_window_x_adjust = 0;
        g_main_window_y_adjust = 0;
    }

    this->qt_w->move(x + g_main_window_x_adjust, y + g_main_window_y_adjust);
    GetWindowRect(edit_hwnd, &g_OldEditWindowRect);
    return 0;
}

// 获取第一个可见窗口并获取句柄
HWND getFirstVisibleWindowHandle()
{
    // 获取所有顶级窗口
    QWidgetList widgets = QApplication::topLevelWidgets();

    // 遍历所有顶级窗口
    for (QWidget *widget : widgets)
    {
        // 判断窗口是否可见
        if (widget->isVisible())
        {
            // 获取窗口句柄
            return reinterpret_cast<HWND>(widget->winId());
        }
    }

    return NULL;
}


void StartEditApp::waitProcessFinish(int exitCode)
{
//    // 等待程序退出
//    if (!process->waitForFinished(-1))
//    {
//        qDebug() << "Failed to wait for program: " << process->errorString();
//        return 1;
//    }

//    // 获取程序退出代码
//    int exitCode = process->exitCode();
    this->qt_w->close();
    QCoreApplication::quit();
    qDebug() << "Program exit with code " << exitCode;
}

//刷新工具窗口相对于编辑窗口的位置
void refreshAjustXY()
{
    RECT mainWindowRect;
    GetWindowRect(main_hwnd, &mainWindowRect);
    RECT editWindowRect;
    GetWindowRect(edit_hwnd, &editWindowRect);
    int titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
    g_main_window_x_adjust = mainWindowRect.left - editWindowRect.left;
    g_main_window_y_adjust = mainWindowRect.top - editWindowRect.top - titleBarHeight;
    qDebug() << "关闭时g_main_window_x_adjust: " << g_main_window_x_adjust;
    qDebug() << "关闭时g_main_window_y_adjust: " << g_main_window_y_adjust;
}

// 回调函数
void CALLBACK WinEventProcDragWinExit(HWINEVENTHOOK hHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hwnd != nullptr && event == EVENT_SYSTEM_MOVESIZEEND)
    {
//        qDebug() << "move window end";
        if(main_hwnd == NULL)
        {
            return;
        }
        if(hwnd == main_hwnd)
        {
//            qDebug() << "main Window " << hwnd << " changed position";
            //控制移动工具栏窗口在编辑窗口之间
            RECT mainWindowRect;
            GetWindowRect(main_hwnd, &mainWindowRect);
            RECT editWindowRect;
            GetWindowRect(edit_hwnd, &editWindowRect);
            int titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
            int x = editWindowRect.left;
            int y = editWindowRect.top - titleBarHeight;
            int width = editWindowRect.right - editWindowRect.left;
            int height = editWindowRect.bottom - editWindowRect.top;
            int mainWindowX = mainWindowRect.left;
            int mainWindowY = mainWindowRect.top;
            bool flag = false;
            if(mainWindowX < x)
            {
                flag = true;
                mainWindowX = x;
            }
            if(mainWindowRect.right > x + width)
            {
                flag = true;
                mainWindowX = x + width - mainWindowRect.right + mainWindowRect.left;
            }
            if(mainWindowY < y)
            {
                flag = true;
                mainWindowY = y;
            }
            if(mainWindowY > y + height)
            {
                flag = true;
                mainWindowY = y + height - mainWindowRect.bottom + mainWindowRect.top + titleBarHeight;
            }
            if(flag)
            {
                SetWindowPos(main_hwnd, NULL, mainWindowX, mainWindowY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            refreshAjustXY();
        }
    }
}

void CALLBACK WinEventProcPos(HWINEVENTHOOK hHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hwnd != NULL && event == EVENT_OBJECT_LOCATIONCHANGE) // 当窗口位置改变时
    {
        if(hwnd == edit_hwnd)
        {
//            qDebug() << "edit Window " << hwnd << " changed position";
            // 获取窗口移动后的位置
            RECT newWindowRect;
            GetWindowRect(hwnd, &newWindowRect);
            // 计算窗口移动的距离
            int deltaX = newWindowRect.left - g_OldEditWindowRect.left;
            int deltaY = newWindowRect.top - g_OldEditWindowRect.top;

            // 更新窗口移动前的位置
            g_OldEditWindowRect = newWindowRect;

            //移动工具窗口，跟随编辑窗口
            RECT mainWindowRect;
            if(main_hwnd == NULL)
            {
                return;
            }
            GetWindowRect(main_hwnd, &mainWindowRect);
            SetWindowPos(main_hwnd, NULL, mainWindowRect.left + deltaX, mainWindowRect.top + deltaY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

}

void CALLBACK WinEventProcMin(HWINEVENTHOOK hHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hwnd != NULL && event == EVENT_SYSTEM_MINIMIZEEND)
    {
//        qDebug() << "Window " << hwnd << " min over";
        if(main_hwnd == NULL)
        {
            return;
        }
        if(hwnd == edit_hwnd)
        {
            ShowWindow(main_hwnd, SW_SHOW);
            SetWindowPos(main_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        else
        {
            ShowWindow(main_hwnd, SW_HIDE);
        }

    }
}

void CALLBACK WinEventProcZ(HWINEVENTHOOK hHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    //控制主窗口始终显示在编辑窗口上一层
    if(hwnd != NULL  && event == EVENT_SYSTEM_FOREGROUND)  //监听窗口的Z顺序变化
    {
//        qDebug() << "window z change";
        if(hwnd == main_hwnd)
        {
            if(z_main_flag && z_edit_flag)
            {
                return;
            }
            z_main_flag = true;
//            qDebug() << "main_hwnd z change";
            SetWindowPos(edit_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        else if(hwnd == edit_hwnd)
        {
            if(z_main_flag && z_edit_flag)
            {
                return;
            }
            z_edit_flag = true;
//            qDebug() << "edit hwnd z change";
            ShowWindow(main_hwnd, SW_SHOW);
            SetWindowPos(main_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        else
        {
            z_edit_flag = false;
            z_main_flag = false;
//            qDebug() << "edit hwnd other change";
            ShowWindow(main_hwnd, SW_HIDE);
        }
    }
}

/**
 * @brief getWindowFromProcessId 根据pid获取qwindow对象
 * @param processId
 * @return
 */
QWindow *getWindowFromProcessId(qint64 processId)
{
    HWND hwnd = GetHwndByPid(processId);
    edit_hwnd = hwnd;
    qDebug() << "根据pid查找到的hwnd: " << hwnd;
    return hwnd ? QWindow::fromWinId(reinterpret_cast<WId>(hwnd)) : nullptr;
}

/**
 * @brief GetHwndByPid  根据pid获取HWND窗口句柄
 * @param dwProcessID
 * @return
 */
HWND GetHwndByPid(DWORD dwProcessID)
{
    qDebug() << "pid:" << dwProcessID;
    HWND h = GetTopWindow(0);
    HWND retHwnd = NULL;
    while (h)
    {
        DWORD pid = 0;
        DWORD dwTheardId = GetWindowThreadProcessId(h, &pid);
//        qDebug() << pid << " " << ::IsWindowVisible(h);
        if (dwTheardId != 0)
        {
            if (pid == dwProcessID && GetParent(h) == NULL && ::IsWindowVisible(h))
            {
                retHwnd = h;    //会有多个相等值
                //    /*
                //char buf[MAX_PATH] = { 0 };
                //sprintf_s(buf, "%0x", h);
                //MessageBox(NULL, buf, "提示", MB_OK);
                //    */
            }
        }
        h = GetNextWindow(h, GW_HWNDNEXT);
    }
    return retHwnd;
}


