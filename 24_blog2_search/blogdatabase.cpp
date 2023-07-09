#include "blogdatabase.h"
#include <QSqlDatabase>
#include <QDebug>
#include <QSqlQuery>
#include <QUuid>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>
#include <QtNetwork>
#include <QSqlError>
#include "config.h"
#include <QImage>
#include <QImageReader>
#include <QSqlDriver>
#include <QSqlField>
#include <QList>
extern bool checkPath(const QString &path, QString error_msg, bool flag);
void removeFolder(const QString &path);

MDPojo::MDPojo(QString id, QString classify, QString title, QString content, QString view_count, QString edit_time)
{
    this->id = id;
    this->classify = classify;
    this->title = title;
    this->content = content;
    this->view_count = view_count;
    this->edit_time = edit_time;
}

IMGPojo::IMGPojo(QString id, QString type, QByteArray content, QString mdid)
{
    this->id = id;
    this->type = type;
    this->content = content;
    this->mdid = mdid;
}

CLASSFIYPojo::CLASSFIYPojo(int id, QString name)
{
    this->id = id;
    this->name = name;
}

SEARCHPojo::SEARCHPojo(QString key_word, QString page, QString pageSize, QString docSize,
                       QString classify, QString search_position,
                       QString time_start, QString time_end, QString time_sort)
{
    this->key_word = key_word;
    this->page = page;
    this->pageSize = pageSize;
    this->docSize = docSize;
    this->classify = classify;
    this->search_position = search_position;
    this->time_start = time_start;
    this->time_end = time_end;
    this->time_sort = time_sort;
}

BlogDatabase::BlogDatabase()
{
    qDebug() << "支持的数据库驱动：" << QSqlDatabase::drivers();
    //检测已连接的方式 - 默认连接名
    //QSqlDatabase::contains(QSqlDatabase::defaultConnection)
    if(QSqlDatabase::contains("qt_sql_default_connection"))
    {
        db = QSqlDatabase::database("qt_sql_default_connection");
    }
    else
    {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }
    //检测已连接的方式 - 自定义连接名
    /*if(QSqlDatabase::contains("mysql_connection"))
        db = QSqlDatabase::database("mysql_connection");
    else
        db = QSqlDatabase::addDatabase("QSQLITE","mysql_connection");*/
    //设置数据库路径，不存在则创建
    db.setDatabaseName("blogsql.db");
    //db.setUserName("gongjianbo");  //SQLite不需要用户名和密码
    //db.setPassword("qq654344883");

    //打开数据库
    if(db.open())
    {
        qDebug() << "open success";
        QSqlQuery query(db);
        query.exec("PRAGMA journal_mode = WAL;");
        query.clear();
        query.finish();
    }
}

BlogDatabase::~BlogDatabase()
{
    //关闭数据库
    db.close();
}

void BlogDatabase::createMDTable()
{
    QSqlQuery query(db);
    query.exec("CREATE TABLE IF NOT EXISTS " + md_table + " ("
               "id TEXT PRIMARY KEY NOT NULL, "
               "classify TEXT, "
               "title TEXT, "
               "content TEXT, "
               "view_count TEXT, "
               "edit_time INTEGER, "
               "FOREIGN KEY (classify) REFERENCES " + classify_table + "(name)"
               ")");
}

void BlogDatabase::createIMGTable()
{
    QSqlQuery query(db);
    query.exec("CREATE TABLE IF NOT EXISTS " + img_table + " ("
               "id TEXT PRIMARY KEY NOT NULL, "
               "type TEXT,"
               "content BLOB,"
               "mdid TEXT,"
               "FOREIGN KEY (mdid) REFERENCES " + md_table + "(id)"
               ")");
}

void BlogDatabase::createCLASSIFYTable()
{
    QSqlQuery query(db);
    query.exec("CREATE TABLE IF NOT EXISTS " + classify_table + " ("
               "id TEXT PRIMARY KEY NOT NULL, "
               "name TEXT"
               ")");
}

void BlogDatabase::transaction()
{
    // 开启事务
    db.transaction();
}

void BlogDatabase::commit()
{
    // 提交事务
    db.commit();
}

QList<QString> *BlogDatabase::getClassifyList()
{
    QSqlQuery query(db);

    query.prepare("SELECT name FROM classifytable");
//    query.bindValue(":id", id);
    QList<QString> *classifyList = new QList<QString>();
    if(query.exec())
    {
        while(query.next())

        {
            QString name = query.value(0).toString();
//            qDebug() << name;
            classifyList->append(name);
        }
    }
    else
    {
        qDebug() << "Error executing delete md query: " << query.lastError().text();
    }
    return classifyList;
}

QList<MDPojo *> *BlogDatabase::searchBlog(SEARCHPojo *pojo)
{
    //声明保存结果列表
    QList<MDPojo *> *res = new QList<MDPojo *>();
    //查询符合条件的总数
    QSqlQuery query(db);
    //多关键字分隔
    QString keyword = pojo->key_word.trimmed();
    keyword = keyword.replace(QRegularExpression("[\\s\\t]+"), " ");
    QStringList keywords = keyword.split(" ");
    QStringList placeholders_title;
    for (const QString &keyword : keywords)
    {
        QString tmp = QString("title LIKE %1").arg("'%" + keyword + "%'");
        placeholders_title.append(tmp);
    }
    QString title_keyword = "(" + placeholders_title.join(" OR ") + ")";
    QStringList placeholders_content;
    for (const QString &keyword : keywords)
    {
        QString tmp = QString("content LIKE %1").arg("'%" + keyword + "%'");
        placeholders_content.append(tmp);
    }
    QString content_keyword = "(" + placeholders_content.join(" OR ") + ")";

    //分类
    QString classify;
    if(pojo->classify == "")
    {
        classify = "";
    }
    else
    {
        classify = QString("AND classify = '%1'").arg(pojo->classify);
    }

    QString query_str;
    QString time_range = QString("edit_time >= %1 AND edit_time <= %2").arg(pojo->time_start).arg(pojo->time_end);
    if(pojo->search_position == "title_content")
    {
        query_str = QString("SELECT COUNT(*) FROM mdtable "
                            "WHERE (%1 OR %2) AND (%3) %4 "
                            "ORDER BY edit_time %5")
                    .arg(title_keyword)
                    .arg(content_keyword)
                    .arg(time_range)
                    .arg(classify)
                    .arg(pojo->time_sort);
    }
    else if(pojo->search_position == "title")
    {
        query_str = QString("SELECT COUNT(*) FROM mdtable "
                            "WHERE %1 AND (%2) %3 "
                            "ORDER BY edit_time %4")
                    .arg(title_keyword)
                    .arg(time_range)
                    .arg(classify)
                    .arg(pojo->time_sort);
    }
    else if(pojo->search_position == "content")
    {
        query_str = QString("SELECT COUNT(*) FROM mdtable "
                            "WHERE %1 AND (%2) %3 "
                            "ORDER BY edit_time %4")
                    .arg(content_keyword)
                    .arg(time_range)
                    .arg(classify)
                    .arg(pojo->time_sort);
    }
    qDebug() << query_str;
//    query.prepare(query_str);
    if(query.exec(query_str))
    {
        if (query.next())
        {
            int totalCount = query.value(0).toInt();
            qDebug() << "Total count: " << totalCount;
            pojo->docSize = QString::number(totalCount);
            pojo->pageSize = QString::number(pojo->docSize.toInt() / g_search_page_size);
            if(pojo->docSize.toInt() % g_search_page_size > 0)
            {
                pojo->pageSize = QString::number(pojo->pageSize.toInt() + 1);
            }
        }
        else
        {
            qDebug() << "Error: No result found.";
            return res;
        }
    }
    else
    {
        qDebug() << "Error executing md query: " << query.lastError().text();
        return res;
    }

    //分页查询
    QSqlQuery query2(db);
    int pageNumber = pojo->page.toInt();
    int pageSize = g_search_page_size;
    int offset = (pageNumber - 1) * pageSize;

    QString sqlQuery;

    if(pojo->search_position == "title_content")
    {
        sqlQuery = QString("SELECT id, title, classify, content, view_count, edit_time FROM mdtable "
                           "WHERE (%1 OR %2) AND (%3) %4 "
                           "ORDER BY edit_time %5 "
                           "LIMIT %6 OFFSET %7")
                   .arg(title_keyword)
                   .arg(content_keyword)
                   .arg(time_range)
                   .arg(classify)
                   .arg(pojo->time_sort)
                   .arg(pageSize)
                   .arg(offset);
    }
    else if(pojo->search_position == "title")
    {
        sqlQuery = QString("SELECT id, title, classify, content, view_count, edit_time FROM mdtable "
                           "WHERE %1 AND (%2) "
                           "ORDER BY edit_time %3 %4 "
                           "LIMIT %5 OFFSET %6")
                   .arg(title_keyword)
                   .arg(time_range)
                   .arg(pojo->time_sort)
                   .arg(classify)
                   .arg(pageSize)
                   .arg(offset);
    }
    else if(pojo->search_position == "content")
    {
        sqlQuery = QString("SELECT id, title, classify, content, view_count, edit_time FROM mdtable "
                           "WHERE %1 AND (%2) "
                           "ORDER BY edit_time %3 %4"
                           "LIMIT %5 OFFSET %6")
                   .arg(content_keyword)
                   .arg(time_range)
                   .arg(pojo->time_sort)
                   .arg(classify)
                   .arg(pageSize)
                   .arg(offset);
    }
    qDebug() << sqlQuery;

    if (query.exec(sqlQuery))
    {
        while (query.next())
        {
            // 处理每一行数据
            QString id = query.value("id").toString();
            QString title = query.value("title").toString();;
            QString classify = query.value("classify").toString();;
            QString content = query.value("content").toString();;
            QString view_count = query.value("view_count").toString();;
            QString edit_time = query.value("edit_time").toString();;
            MDPojo *pojo = new MDPojo(id, classify, title, content, view_count, edit_time);
            res->append(pojo);
        }
    }
    else
    {
        qDebug() << "Query failed:" << query.lastError().text();
    }
    return res;
}

MDPojo *BlogDatabase::getMDToDB(QString id)
{
    QSqlQuery query(db);

    query.prepare("SELECT mdtable.id, mdtable.classify, mdtable.title, mdtable.content, mdtable.view_count, mdtable.edit_time, "
                  "imgtable.id AS imgid, imgtable.type, imgtable.content AS imgcontent "
                  "FROM mdtable LEFT JOIN imgtable ON mdtable.id = imgtable.mdid "
                  "WHERE mdtable.id = :id");
//    QString sql = "SELECT * FROM mdtable WHERE mdtable.id = ?";
//    query.prepare(sql);
//    query.addBindValue(id);
//    qDebug() << "query string: " << query.lastQuery();

//    QString sql = "SELECT * FROM mdtable WHERE mdtable.id = :id";
//    query.prepare(sql);
    query.bindValue(":id", id);
    qDebug() << "query string: " << query.lastQuery();
    qDebug() << id;
    MDPojo *mdpojo = nullptr;
    if(query.exec())
    {
        while(query.next())
        {
            QString mdid = query.value(0).toString();
            QString mdclassify = query.value(1).toString();
            QString mdtitle = query.value(2).toString();
            QString mdcontent = query.value(3).toString();
            QString mdview_count = query.value(4).toString();
            QString mdedit_time = query.value(5).toString();

            QString imgid = query.value(6).toString();
            QString imgtype = query.value(7).toString();
            QByteArray imgcontent = query.value(8).toByteArray();
            qDebug() << mdid;
//            qDebug() << mdclassify;
//            qDebug() << mdtitle;
//            qDebug() << mdcontent;
//            qDebug() << mdview_count;
//            qDebug() << mdedit_time;
            qDebug() << imgid;
//            qDebug() << imgcontent;
//            IMGPojo *imgpojo = new IMGPojo(imgid, imgtype, imgcontent, mdid);
            // 保存图片到本地
            QString filePath = g_data_path + QDir::separator() + "forshow";
            if(!checkPath(filePath, filePath + "不存在", true))
            {
                return nullptr;
            }
            filePath = filePath + QDir::separator() + mdtitle;
            if(mdpojo == nullptr)
            {
                if(checkPath(filePath, filePath + "不存在", false))
                {
                    removeFolder(filePath);
                }
                QDir dir;
                bool success = dir.mkpath(filePath);
                if (success)
                {
                    qDebug() << "Path created successfully: " << filePath;
                }
                else
                {
                    qDebug() << "Failed to create path: " << filePath;
                }
            }
            QString dirPath = filePath;
            QString mdPath = dirPath + QDir::separator() + mdtitle;
            qDebug() << "show: " << mdPath;
            QString imgPath = dirPath + QDir::separator() + imgid + "." + imgtype;
            if(imgtype == "")
            {
                imgPath = dirPath + QDir::separator() + imgid;
            }
            if(mdpojo == nullptr)
            {
                mdpojo = new MDPojo(mdid, mdclassify, mdtitle, mdcontent, mdview_count, mdedit_time);
                //将mdpojo中的content保存到本地文件
                QFile file(mdPath);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&file);
                    out.setCodec("UTF-8");
                    out << mdcontent;
                    file.close();
                    qDebug() << mdPath + "显示文件保存成功";
                }
                else
                {
                    qDebug() << mdPath + "显示文件保存失败";
                }
            }
            if(imgcontent != "")
            {
                // 从QByteArray中加载图片
                QImage image;
                image.loadFromData(imgcontent);
                if(image.save(imgPath))
                {
                    qDebug() << "Image: " + imgPath + " saved successfully";
                }
                else
                {
                    qDebug() << "Failed to save image: " + imgPath;
                }
            }
        }
        qDebug() << "加载完成";
    }
    else
    {
        qDebug() << "Error executing query: " << query.lastError().text();
        return nullptr;
    }
    query.clear();
    query.finish();
    return mdpojo;
}

void removeFolder(const QString &path)
{
    QDir dir(path);

    if (!dir.exists())
    {
        return;
    }

    foreach (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs))
    {
        if (info.isDir())
        {
            QDir subDir(info.absoluteFilePath());
            subDir.removeRecursively();
        }
        else
        {
            QFile file(info.absoluteFilePath());
            file.remove();
        }
    }

    dir.rmdir(".");
}







