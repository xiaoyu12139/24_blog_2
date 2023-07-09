#ifndef BLOGCLASSIFYDIALOG_H
#define BLOGCLASSIFYDIALOG_H

#include <QWidget>

namespace Ui
{
    class BlogClassifyDialog;
}

class BlogClassifyDialog : public QWidget
{
    Q_OBJECT

public:
    explicit BlogClassifyDialog(QWidget *parent = nullptr);
    ~BlogClassifyDialog();
    void refreshClassifyList();
    void removeLayoutFromLayout(QLayout *layoutToRemove, QLayout *parentLayout, QList<QWidget *> *aa);
    void addRemoveLayout(QLayout *layoutToRemove, QLayout *parentLayout, QList<QWidget *> *widgetsToRemove);
    void closeEvent(QCloseEvent *event) override;
    void selectClassify(QString classify);

signals:
    void closeWindow();
    void saveSignal(QString select_classify, QList<QString> *classify_list);

private:
    Ui::BlogClassifyDialog *ui;
};

#endif // BLOGCLASSIFYDIALOG_H
