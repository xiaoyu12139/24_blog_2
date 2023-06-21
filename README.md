介绍

```
该软件打开同时，启动设置好的markdown编辑器，悬浮位于编辑器上方，可展开和收缩
在点击保存前，先使用编辑器保存至软件路径下的(DATAS_PATH)下的for_up文件夹下
然后在点击保存，文本数据会保存至blogsql.db数据库中
使用24_blog2_search软件可搜索查询该数据库中的文件，并打开
数据库以文件名为id
```

配置文件：config.ini

```
//markdown编辑器的路径
EDIT_PATH=C:\\Program Files\\Typora\\Typora.exe
//工具窗口的位置
MAINWINDOW_X_ADJUST=835
MAINWINDOW_Y_ADJUST=105
//等待编辑窗口的时长
WAIT_EDITAPP_TIME=2
SEARCH_PAGE_SIZE=10
// 数据文件夹的位置，默认为注释掉了的
# DATA_PATH=
```

目录结构

```
--data 
----for_up
----for_show
--24_blog2.exe
--24_blog2_search.exe
--blogsql.db
--config.ini
--log.txt
--log_search.txt
```



blog_search 可能会闪退，原因可能是markdown文件中的图片格式问题，建议换成常见的格式如png等