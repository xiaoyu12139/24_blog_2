#include "config.h"
#include <QCoreApplication>
#include <QDir>

//设置读取配置文件的默认值
QString g_editapp_path = "C:\\Windows\\System32\\notepad.exe";
int g_main_window_x_adjust = 0;
int g_main_window_y_adjust =  0;
int g_wait_open_editapp_time = 2;
int g_search_page_size = 10;
QString g_data_path = "";
QString g_uuid = "";

void init_config()
{
    g_data_path = QCoreApplication::applicationDirPath() + "/data";
    g_data_path = g_data_path.replace("/", QDir::separator());
}
