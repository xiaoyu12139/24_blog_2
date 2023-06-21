#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QMouseEvent>
#include <QLabel>
#include <QDateTime>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    qint64 last_time;
    explicit ClickableLabel(QWidget *parent = nullptr): QLabel(parent)
    {
        last_time = 0;
    }
    ~ClickableLabel() {}


signals:
    void clicked();

private:
    void mouseReleaseEvent(QMouseEvent *event)
    {
        QDateTime currentTime = QDateTime::currentDateTime();
        qint64 timestamp = currentTime.toMSecsSinceEpoch();
        if(event->button() == Qt::LeftButton)
        {
            if(timestamp - last_time > 500)
            {
                emit clicked();
                last_time = timestamp;
            }
            event->accept();
        }
    }
};

#endif // CLICKABLELABEL_H
