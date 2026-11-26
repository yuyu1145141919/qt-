#include "historyview.h"
#include "ui_historyview.h"
#include "idatabase.h"

#include <QHeaderView>

HistoryView::HistoryView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HistoryView)
{
    ui->setupUi(this);
    setWindowTitle("操作记录");

    IDatabase &db = IDatabase::getInstance();
    if (db.initHistoryModel()) {
        ui->tableView->setModel(db.historyQueryModel);
        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setAlternatingRowColors(true);
        ui->tableView->verticalHeader()->setVisible(false);
        ui->tableView->verticalHeader()->setDefaultSectionSize(34);
        ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        ui->tableView->horizontalHeader()->setStretchLastSection(true);
        ui->tableView->resizeColumnsToContents();
    }

    connect(ui->txtSearch, &QLineEdit::returnPressed, this, &HistoryView::on_btSearch_clicked);
    refreshTable();
}

HistoryView::~HistoryView()
{
    delete ui;
}

void HistoryView::refreshTable()
{
    IDatabase &db = IDatabase::getInstance();
    db.initHistoryModel(ui->txtSearch->text().trimmed());
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    int total = db.historyQueryModel ? db.historyQueryModel->rowCount() : 0;
    ui->labelStatus->setText(total == 0 ? "未查询到符合条件的操作记录"
                                        : QString("当前共显示 %1 条用户操作审计记录").arg(total));
}

void HistoryView::on_btSearch_clicked()
{
    refreshTable();
}

void HistoryView::on_btReset_clicked()
{
    ui->txtSearch->clear();
    refreshTable();
}
