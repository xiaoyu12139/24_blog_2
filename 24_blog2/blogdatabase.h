#ifndef BLOGDATABASE_H
#define BLOGDATABASE_H
#include <QSqlDatabase>

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
    MDPojo *getMDToDB(QString id);
    void getIMGToDB(QString id);
    bool addMDToDB(QString filePath, QString mdid, QString classify);
    bool uploadMDToDB(QString filePath, QString classify);
    IMGPojo *addIMGToDB(QString imgPath, QString mdid, bool islocal);
    bool updateMDToDB(QString filePath, QString mdid, QString classify);
    bool deleteMD(QString id);
    void updateIMGToDB(QString imgPath);
    bool deleteMDFromDB(QString id);
    bool deleteIMGFromDB(QString id);
    QList<QString> *getClassifyList();
    bool addClassifyToDB(QString name);
    bool deleteClassifyFromDB(QString name);
    void transaction();
    void commit();
    QString createUUID();
    QString createIMGId();
    QString createMDId(QString title);
    bool checkIfIdExists(QString id, QString table_name);
};

#endif // BLOGDATABASE_H
