#include "choosetime.h"
#include "ui_choose_time.h"
#include <QDebug>

ChooseTime::ChooseTime(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChooseTime)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    this->setWindowModality(Qt::WindowModal);
    this->setWindowIcon(QIcon(":/img/24_blog_logo.png"));
    this->setWindowTitle(QString("设置时间"));
    this->setFixedSize(260, 109);
    ui->start_date->setDisplayFormat("yyyy/MM/dd");
    ui->start_time->setDisplayFormat("HH:mm:ss");
    ui->end_date->setDisplayFormat("yyyy/MM/dd");
    ui->end_time->setDisplayFormat("HH:mm:ss");
    connect(ui->cancel, &QPushButton::clicked, [ = ]()
    {
        this->close();
    });
    connect(ui->ok, &QPushButton::clicked, [ = ]()
    {
        qDebug() << "choose time click";
        QDate startDate = ui->start_date->date();
        QTime startTime = ui->start_time->time();
        QDate endDate = ui->end_date->date();
        QTime endTime = ui->end_time->time();
        emit timeSearchBlogSignal(startDate, startTime, endDate, endTime);
        this->close();
    });
}

ChooseTime::~ChooseTime()
{
    delete ui;
}
