#include "blogclassifydialog.h"
#include "ui_blogclassifydialog.h"
#include <QDebug>
#include <QCloseEvent>
#include "blogdatabase.h"

extern BlogDatabase *blogDB;

BlogClassifyDialog::BlogClassifyDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BlogClassifyDialog)
{
    ui->setupUi(this);
    this->setWindowFlag(Qt::Dialog);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowMinMaxButtonsHint);
    this->setWindowModality(Qt::ApplicationModal);
    QList<QWidget *> *widgetsToRemoveM = new QList<QWidget *>();
    QList<QWidget *> *widgetsToRemoveI = new QList<QWidget *>();
    removeLayoutFromLayout(this->ui->input_classify_name, this->ui->verticalLayout, widgetsToRemoveI);
    this->setWindowTitle("blog分类");
    //初始化分类列表
    this->refreshClassifyList();
    connect(ui->cancel_save, &QPushButton::clicked, [ = ]()
    {
        this->close();
    });
    connect(ui->cancel_input, &QPushButton::clicked, [ = ]()
    {
        removeLayoutFromLayout(ui->input_classify_name, ui->verticalLayout, widgetsToRemoveI);
        addRemoveLayout(ui->manage_classify, ui->verticalLayout, widgetsToRemoveM);
    });
    connect(ui->add, &QPushButton::clicked, [ = ]()
    {
        removeLayoutFromLayout(ui->manage_classify, ui->verticalLayout, widgetsToRemoveM);
        addRemoveLayout(ui->input_classify_name, ui->verticalLayout, widgetsToRemoveI);
        ui->lineEdit->setText("新建分类");
    });
    connect(ui->sure_input, &QPushButton::clicked, [ = ]()
    {
        QString name = ui->lineEdit->text();
        name = name.trimmed();
        if(name == "")
        {
            ui->lineEdit->setText("新建分类");
            return;
        }
        //判断当前列表中是否存在
        bool flag = true;
        int itemCount = ui->listWidget->count();
        for (int i = 0; i < itemCount; i++)
        {
            QListWidgetItem *item = ui->listWidget->item(i);
            if (item)
            {
                // 对每个项进行处理
                QString itemText = item->text();
                if(name == itemText)
                {
                    flag = false;
                    break;
                }
            }
        }
        if(flag && name != "default")
        {
            QListWidgetItem *newItem = new QListWidgetItem(name);
            ui->listWidget->addItem(newItem);
        }
        removeLayoutFromLayout(ui->input_classify_name, ui->verticalLayout, widgetsToRemoveI);
        addRemoveLayout(ui->manage_classify, ui->verticalLayout, widgetsToRemoveM);
    });
    connect(ui->delete_item, &QPushButton::clicked, [ = ]()
    {
        // 删除选中的项
        QList<QListWidgetItem *> selectedItems = ui->listWidget->selectedItems();
        for (QListWidgetItem *item : selectedItems)
        {
            if(item->text() == "default")
            {
                continue;
            }
            ui->listWidget->takeItem(ui->listWidget->row(item));
            delete item;  // 可选，如果需要释放内存
        }
    });
    connect(ui->save, &QPushButton::clicked, [ = ]()
    {
        //执行保存操作，并将分类修改结果进行反馈
        this->close();
        QList<QString> *classifyList = new QList<QString>();
        // 遍历 listWidget 中的项
        int itemCount = ui->listWidget->count();
        for (int i = 0; i < itemCount; i++)
        {
            QListWidgetItem *item = ui->listWidget->item(i);
            if (item)
            {
                // 对每个项进行处理
                QString itemText = item->text();
                classifyList->append(itemText);
            }
        }
        QList<QListWidgetItem *> selectedItems = ui->listWidget->selectedItems();
        QString select_classify = "default";
        if(selectedItems.size() != 0)
        {
            select_classify = selectedItems.at(0)->text();
        }
        emit this->saveSignal(select_classify, classifyList);
    });
}

void BlogClassifyDialog::refreshClassifyList()
{
    ui->listWidget->clear();
    QList<QString> *classify_list = blogDB->getClassifyList();
    QList<QString>::const_iterator it;
    for (it = classify_list->constBegin(); it != classify_list->constEnd(); ++it)
    {
        QString value = *it;
        // 处理每个元素的逻辑
        QListWidgetItem *newItem = new QListWidgetItem(value);
        ui->listWidget->addItem(newItem);
    }
}

void BlogClassifyDialog::removeLayoutFromLayout(QLayout *layoutToRemove, QLayout *parentLayout,  QList<QWidget *> *widgetsToRemove)
{
    // 从父布局中移除布局
    if (parentLayout)
    {
        parentLayout->removeItem(layoutToRemove);
    }

    // 从父布局中移除控件
    QLayoutItem *item;
    while ((item = layoutToRemove->takeAt(0)))
    {
        QWidget *widget = item->widget();
        if (widget)
        {
//            qDebug() << "delete";
            widget->setParent(nullptr);
            layoutToRemove->removeWidget(widget);
            widgetsToRemove->append(widget);
        }
        delete item; // 释放布局项的内存
    }
}

void BlogClassifyDialog::addRemoveLayout(QLayout *layoutToRemove, QLayout *parentLayout, QList<QWidget *> *widgetsToRemove)
{
    // 将移除的布局及其控件重新添加
    parentLayout->addItem(layoutToRemove); // 添加布局回父布局
    layoutToRemove->setParent(parentLayout);

    foreach (QWidget *widget, *widgetsToRemove)
    {
//        qDebug() << "add";
        layoutToRemove->addWidget(widget); // 添加控件回父布局
    }
    widgetsToRemove->clear();
    if(layoutToRemove == ui->manage_classify)
    {
        QHBoxLayout *tmp = (QHBoxLayout *)layoutToRemove;
        tmp->insertItem(2, new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    }
}

void BlogClassifyDialog::closeEvent(QCloseEvent *event)
{
//    if (event->spontaneous())
//    {
//        // 点击窗口关闭按钮关闭窗口
//        qDebug() << "Window closed by clicking close button";
//        this->hide();
//        event->ignore();
//        emit this->closeWindow();
//    }
//    else
//    {
//        // 调用this->close()关闭窗口
//        qDebug() << "Window closed by calling this->close()";
//        QCoreApplication::quit();
//        BlogClassifyDialog::closeEvent(event);
//    }
    this->hide();
    event->ignore();
    emit this->closeWindow();
}

BlogClassifyDialog::~BlogClassifyDialog()
{
    delete ui;
}
