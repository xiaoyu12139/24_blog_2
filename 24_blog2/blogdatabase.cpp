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
extern HWND edit_hwnd;
extern HWND main_hwnd;
extern MDPojo *main_mdpojo;
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
//    db = QSqlDatabase::addDatabase("QSQLITE");
    //设置数据库路径，不存在则创建
    db.setDatabaseName("blogsql.db");
    //db.setUserName("gongjianbo");  //SQLite不需要用户名和密码
    //db.setPassword("qq654344883");
    //打开数据库
    if(db.open())
    {
        qDebug() << "blogdb open success.";
        QSqlQuery query(db);
        query.exec("PRAGMA journal_mode = WAL;");
        query.clear();
        query.finish();
    }
    else
    {
        qDebug() << "blogdb open failed.";
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

bool BlogDatabase::addClassifyToDB(QString name)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO " + classify_table + " (id, name) VALUES (:id, :name)");
    query.bindValue(":id", createMDId(name));
    query.bindValue(":name", name);
    qDebug() << "query string: " << query.lastQuery();
    if (!query.exec())
    {
        qDebug() << "Failed to insert classify into database";
        qDebug() << query.lastError().text();
        return false;
    }
    qDebug() << "保存成功分类: " + name;
    return true;
}

bool BlogDatabase::deleteClassifyFromDB(QString name)
{
    //判断当前分类是否可删除，即看当前分类下是否有blog
    QSqlQuery query(db);
    query.prepare("SELECT * "
                  "FROM mdtable "
                  "WHERE mdtable.classify = :name");
    query.bindValue(":name", name);
    qDebug() << "query string: " << query.lastQuery();
    if(query.exec())
    {
        if(query.next())
        {
            return false;
        }
    }
    else
    {
        qDebug() << "Error executing query: " << query.lastError().text();
        return false;
    }

    QSqlQuery query1(db);
    query1.prepare("DELETE FROM classifytable "
                   "WHERE classifytable.name = :name");
    query1.bindValue(":name", name);
    qDebug() << "query string: " << query1.lastQuery();
    if (!query1.exec())
    {
        qWarning() << "Failed to delete classify:" << query1.lastError().text();
        return false;
    }
    return true;
}

bool BlogDatabase::updateMDToDB(QString filePath, QString mdid, QString classify)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "read false";
        return false;
    }
    qDebug() << filePath;

    QSqlQuery query(db);
    query.prepare("SELECT mdtable.id, mdtable.title, mdtable.content, imgtable.id AS imgid, imgtable.type, imgtable.content AS imgcontent "
                  "FROM mdtable LEFT JOIN imgtable ON mdtable.id = imgtable.mdid "
                  "WHERE mdtable.id = :id");
    query.bindValue(":id", main_mdpojo->id);
    qDebug() << "query string: " << query.lastQuery();
    if(query.exec())
    {
        while(query.next())
        {
//            qDebug() << "while";
            QString mdid = query.value(0).toString();
            QString mdtitle = query.value(1).toString();
            QString mdcontent = query.value(2).toString();
            QString imgid = query.value(3).toString();
            QString imgtype = query.value(4).toString();
            QByteArray imgcontent = query.value(5).toByteArray();
//            IMGPojo *imgpojo = new IMGPojo(imgid, imgtype, imgcontent, mdid);
//            qDebug() << imgid;
            QString filePath = g_data_path + QDir::separator() + "forshow";
            if(!checkPath(filePath, filePath + "不存在", true))
            {
                return false;
            }
            deleteIMGFromDB(imgid);
        }
        deleteMDFromDB(main_mdpojo->id);
    }
    else
    {
        qDebug() << "Error executing delete md query: " << query.lastError().text();
        return false;
    }
    bool flag = addMDToDB(filePath, mdid, classify);
    main_mdpojo = getMDToDB(main_mdpojo->id);
    bool flag2 = main_mdpojo != nullptr;
    return flag && flag2;
}

bool BlogDatabase::uploadMDToDB(QString filePath, QString classify)
{
    //将文件上传到数据库中后，将文件移动到load文件夹，转换成加载文件
    //上传文件前先检查文件是否在数据库中存在
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }
    QFileInfo fileInfo(filePath);
    QString mdid = createMDId(fileInfo.fileName());
    qDebug() << checkIfIdExists(mdid, md_table);
    if(main_mdpojo != nullptr && checkIfIdExists(main_mdpojo->id, md_table))

    {
        qDebug() << "更新数据：当前文件的名称在数据库中已经存在: " + filePath;
        return updateMDToDB(filePath, mdid, classify);
    }
    if(main_mdpojo == nullptr && checkIfIdExists(mdid, md_table))
    {
        qDebug() << "上传数据：当前文件的名称在数据库中已经存在: " + filePath;
        return false;
    }
    qDebug() << "新增数据: " + filePath;
    return addMDToDB(filePath, mdid, classify);
}

bool BlogDatabase::addMDToDB(QString filePath, QString mdid, QString classify)
{
    qDebug() << "save mdid: " + mdid;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // 文件打开失败
        qDebug() << "文件打开失败";
        return false;
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString markdown = in.readAll();
    qDebug() << "markdown: " + markdown;
    file.close();

    QFile *imgFile = nullptr;

    QRegularExpression regex("\\!\\[[^\\]]*\\]\\(([^\\)]*)\\)");

    QRegularExpressionMatchIterator iter = regex.globalMatch(markdown);
    while (iter.hasNext())
    {
        QRegularExpressionMatch match = iter.next();
        QString imageUrl = match.captured(1);
        bool islocal = true;
        if (imageUrl.startsWith("http") || imageUrl.startsWith("https"))
        {
            islocal = false;
            // 如果图片链接是网络链接，下载图片
            QUrl url(imageUrl);
            QNetworkAccessManager manager;
            QNetworkReply *reply = manager.get(QNetworkRequest(url));
            QEventLoop loop;
            QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();
            if (reply->error() == QNetworkReply::NoError)
            {
                QString downimg_path = g_data_path + QDir::separator() + "forup" + QDir::separator() + "downimg";
                QDir dir(downimg_path);
                if(!dir.exists())
                {
                    dir.mkpath(".");
                }
                QStringList files = dir.entryList(QStringList() << "*", QDir::Files);
                int numFiles = files.count();
                QString localPath = downimg_path + QDir::separator() + QString::number(numFiles);
                imgFile = new QFile(localPath);
                if (imgFile->open(QIODevice::WriteOnly))
                {
                    imgFile->write(reply->readAll());
                    imgFile->close();
                }
            }
            else
            {
                qDebug() << "Failed to download image:" << reply->errorString();
                return false;
            }
            reply->deleteLater();
        }
        else
        {
            // 如果图片链接是本地链接，判断本地文件是否存在
            QString tmp_imageUrl = imageUrl;
            QDir dir(tmp_imageUrl);
            if(dir.isRelative())
            {
                QString tmpPath = filePath + "";
                tmpPath = tmpPath.replace("/", QDir::separator());
                int index = tmpPath.lastIndexOf(QDir::separator());  // 找到最后一个斜杠的位置
                QString newPath = tmpPath.left(index);  // 获取斜杠之前的部分
                QFileInfo tmpInfo2(newPath + QDir::separator() + tmp_imageUrl);
                tmp_imageUrl = tmpInfo2.filePath();
            }
            qDebug() << "save本地img: " << tmp_imageUrl;
            imgFile = new QFile(tmp_imageUrl);
            if (!imgFile->exists())
            {
                qDebug() << "File not exists:" << tmp_imageUrl;
                return false;
            }
        }

        QFileInfo info(*imgFile);


        IMGPojo *imgPojo = addIMGToDB(info.filePath(), mdid, islocal);
        if(nullptr == imgPojo)
        {
            qDebug() << info.filePath() + "文件写入数据库失败";
            return false;
        }
        if(imgPojo->type == "")
        {
            markdown = markdown.replace(imageUrl, imgPojo->id);
        }
        else
        {
            markdown = markdown.replace(imageUrl, imgPojo->id + "." + imgPojo->type);
        }

    }
    QFileInfo fileInfo(filePath);
    QString title = fileInfo.fileName();
    QSqlQuery query(db);
    query.prepare("INSERT INTO " + md_table + " (id, classify, title, content, view_count, edit_time) "
                  "VALUES (:id, :classify, :title, :content, :view_count, :edit_time)");
    query.bindValue(":id", mdid);
    query.bindValue(":classify", classify);
    query.bindValue(":title", title);
    query.bindValue(":content", markdown);
    if(main_mdpojo != nullptr && main_mdpojo->title == title)
    {
        query.bindValue(":view_count", main_mdpojo->view_count);
    }
    else
    {
        query.bindValue(":view_count", 1);
    }

    qint64 edit_time = QDateTime::currentMSecsSinceEpoch();
    query.bindValue(":edit_time", edit_time);
    if (!query.exec())
    {
        qDebug() << "Failed to insert markdown into database";
        qDebug() << query.lastError().text();
        return false;
    }
    qDebug() << "保存成功";
    if(main_mdpojo != nullptr && main_mdpojo->title == title)
    {

        main_mdpojo->classify = classify;
        main_mdpojo->content = markdown;
        main_mdpojo->edit_time = QString::number(edit_time);
    }
//    else
//    {
//        main_mdpojo = new MDPojo(mdid, classify, title, markdown);
//    }
    return true;
}

IMGPojo *BlogDatabase::addIMGToDB(QString imgPath, QString mdid, bool islocal)
{
    QFile file(imgPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Failed to open file";
        return nullptr;
    }

    QFileInfo info(imgPath);
    QImageReader reader(info.filePath());
    QString format = reader.format(); // 获取图片格式
    QString type = "";
    if (!format.isNull())
    {
        type = format;
    }
    else
    {
        qDebug() << info.fileName() + "不是有效的图片格式";
        return nullptr;
    }
    if(islocal)
    {
        QStringList parts = imgPath.split(".");
        if(parts.size() > 1)
        {
            type = parts.at(parts.size() - 1);
        }
        else
        {
            type = "";
        }
    }

    QByteArray imageData = file.readAll();
    file.close();

    QString id = createIMGId();

    QSqlQuery query(db);
    query.prepare("INSERT INTO " + img_table + " (id, type, content, mdid) VALUES (:id, :type, :content, :mdid)");
    query.bindValue(":id", id);
    query.bindValue(":type", type);
    query.bindValue(":content", imageData);
    query.bindValue(":mdid", mdid);
    if (!query.exec())
    {
        qDebug() << "Failed to insert image into database";
        qDebug() << query.lastError().text();
        return nullptr;
    }
    return new IMGPojo(id, type, imageData, mdid);
}

bool BlogDatabase::deleteMD(QString id)
{
    QSqlQuery query(db);

    query.prepare("SELECT mdtable.id, mdtable.title, mdtable.content, imgtable.id AS imgid, imgtable.type, imgtable.content AS imgcontent "
                  "FROM mdtable LEFT JOIN imgtable ON mdtable.id = imgtable.mdid "
                  "WHERE mdtable.id = :id");
    query.bindValue(":id", id);
    qDebug() << "query string: " << query.lastQuery();
    if(query.exec())
    {
        while(query.next())
        {
//            qDebug() << "while";
            QString mdid = query.value(0).toString();
            QString mdtitle = query.value(1).toString();
            QString mdcontent = query.value(2).toString();
            QString imgid = query.value(3).toString();
            QString imgtype = query.value(4).toString();
            QByteArray imgcontent = query.value(5).toByteArray();
//            IMGPojo *imgpojo = new IMGPojo(imgid, imgtype, imgcontent, mdid);
//            qDebug() << imgid;
            QString filePath = g_data_path + QDir::separator() + "forshow";
            if(!checkPath(filePath, filePath + "不存在", true))
            {
                return false;
            }
            filePath = filePath + QDir::separator() + mdid;
            if(checkPath(filePath, filePath + "不存在", false))
            {
                removeFolder(filePath);
            }
            deleteIMGFromDB(imgid);
        }
        deleteMDFromDB(id);
    }
    else
    {
        qDebug() << "Error executing delete md query: " << query.lastError().text();
        return false;
    }
    return true;
}

bool BlogDatabase::deleteMDFromDB(QString id)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM mdtable WHERE id = :id");
    query.bindValue(":id", id);
    qDebug() << "query string: " << query.lastQuery();
    if (!query.exec())
    {
        qWarning() << "Failed to delete md:" << query.lastError().text();
        return false;
    }
    return true;
}

bool BlogDatabase::deleteIMGFromDB(QString id)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM imgtable WHERE id = :id");
    query.bindValue(":id", id);
    qDebug() << "query string: " << query.lastQuery();
    if (!query.exec())
    {
        qWarning() << "Failed to delete img:" << query.lastError().text();
        return false;
    }
    qDebug() << "delete img_id: " << id;
    return true;
}

void BlogDatabase::updateIMGToDB(QString imgPath)
{

}

MDPojo *BlogDatabase::getMDToDB(QString id)
{
    QSqlQuery query(db);

    query.prepare("SELECT mdtable.id, mdtable.classify, mdtable.title, mdtable.content, mdtable.view_count, mdtable.edit_time, "
                  "imgtable.id AS imgid, imgtable.type, imgtable.content AS imgcontent "
                  "FROM mdtable LEFT JOIN imgtable ON mdtable.id = imgtable.mdid "
                  "WHERE mdtable.id = :id");
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
            if(mdpojo == nullptr)
            {
                mdpojo = new MDPojo(mdid, mdclassify, mdtitle, mdcontent, mdview_count, mdedit_time);
                break;
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
    QSqlQuery update_query;
    update_query.prepare("UPDATE mdtable SET edit_time = :value1, view_count = :value2 WHERE id = :id");
    update_query.bindValue(":value1", QDateTime::currentMSecsSinceEpoch());
    update_query.bindValue(":value2", mdpojo->view_count.toInt() + 1);
    update_query.bindValue(":id", mdpojo->id);
    if (update_query.exec())
    {
        qDebug() << "update date success when open file.";
    }
    else
    {
        qDebug() << "update date failed when open file.";
    }
    return mdpojo;
}
void BlogDatabase::getIMGToDB(QString id)
{

}

QString BlogDatabase::createUUID()
{
    QUuid uuid = QUuid::createUuid();
//    qDebug() << "create UUID: " << uuid.toString();
    return uuid.toString();
}

QString BlogDatabase::createIMGId()
{
    QString id = createUUID();
    while(checkIfIdExists(id, img_table))
    {
        id = createUUID();
    }
    return id;
}

QString BlogDatabase::createMDId(QString title)
{
    QByteArray bytes = title.toUtf8();
    QString base64 = bytes.toBase64();
    return base64;
}

bool BlogDatabase::checkIfIdExists(QString id, QString table_name)
{
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM " + table_name + " WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next())
    {
        int count = query.value(0).toInt();
        if (count > 0)
        {
            return true;
        }
    }
    return false;
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







