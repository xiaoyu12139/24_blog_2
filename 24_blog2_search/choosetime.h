#ifndef CHOOSETIME_H
#define CHOOSETIME_H

#include <QWidget>
#include <QDate>

namespace Ui
{
    class ChooseTime;
}

class ChooseTime : public QWidget
{
    Q_OBJECT

public:
    explicit ChooseTime(QWidget *parent = nullptr);
    ~ChooseTime();
    QString startDate = "2000/01/01";
    QString startTime = "00:00:00";
    QString endDate = "2500/01/01";
    QString endTime = "00:00:00";
signals:
    void timeSearchBlogSignal(QDate startDate, QTime startTime, QDate endDate, QTime endTime);
public:
    Ui::ChooseTime *ui;
};

#endif // CHOOSETIME_H
