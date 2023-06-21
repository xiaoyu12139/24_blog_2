#ifndef BLOGDATABASE_H
#define BLOGDATABASE_H
#include <QSqlDatabase>
#include "config.h"

class MDPojo
{
public:
    MDPojo(QString id, QString classify, QString title, QString content, QString view_count, QString edit_time);
    QString id;
    QString title;
    QString classify;
    QString content;
    QString view_count;
    QString edit_time;
};

class IMGPojo
{
public:
    IMGPojo(QString id, QString type, QByteArray content, QString mdid);
    QString id;
    QString type;
    QByteArray content;
    QString mdid;
};

class CLASSFIYPojo
{
public:
    CLASSFIYPojo(int id, QString name);
    int id;
    QString name;
};

class SEARCHPojo
{
public:
    SEARCHPojo(QString key_word, QString page, QString pageSize, QString docSize,
               QString classify, QString search_position,
               QString time_start, QString time_end, QString time_sort);
    QString key_word;//搜索关键词
    QString page;//当前页
    QString pageSize;//总页数
    QString docSize;//文章总数
    QString classify;//分类
    QString search_position;//查询内容位置：title_content-标题和内容，title-标题，content-内容
    QString time_start;
    QString time_end;
    QString time_sort;//时间排序方式, ASC-升序 DESC-降序
};

class BlogDatabase
{
public:
    QString img_table = "imgtable";
    QString md_table = "mdtable";
    QString classify_table = "classifytable";
    BlogDatabase();
    ~BlogDatabase();
    QSqlDatabase db;
    void createMDTable();
    void createIMGTable();
    void createCLASSIFYTable();
    void transaction();
    void commit();

    QList<QString> *getClassifyList();
    MDPojo *getMDToDB(QString id);
    QList<MDPojo *> *searchBlog(SEARCHPojo *pojo);
};

#endif // BLOGDATABASE_H
