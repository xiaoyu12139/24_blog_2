#ifndef SEARCHITEM_H
#define SEARCHITEM_H

#include <QWidget>
#include "blogdatabase.h"
#include <QLabel>

namespace Ui
{
    class SearchItem;
}

class SearchItem : public QWidget
{
    Q_OBJECT

public:
    bool eventFilter(QObject *obj, QEvent *event);
    explicit SearchItem(MDPojo *pojo, QWidget *parent = nullptr);
    ~SearchItem();
    MDPojo *pojo;
    void setPojo(MDPojo *pojo)
    {
        this->pojo = pojo;
    }
//    void setIcon(QLabel *lab, QChar chr, int size, QColor color = nullptr);

private:
    Ui::SearchItem *ui;
};

#endif // SEARCHITEM_H
