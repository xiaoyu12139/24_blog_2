#ifndef CONFIG_H
#define CONFIG_H
#include <QString>

extern QString g_editapp_path; //编辑软件路径
extern int g_main_window_x_adjust;//操作窗口相对与编辑窗口的左上角的x偏差
extern int g_main_window_y_adjust;//操作窗口相对与编辑窗口的左上角的y偏差
extern int g_wait_open_editapp_time;//设置几就是几秒
extern int g_search_page_size;//搜索时每页的数量
extern QString g_data_path;//设置生成的数据文件存储的路径
extern QString g_uuid;

void init_config();

#endif // CONFIG_H
