#include "patientview.h"
#include "ui_patientview.h"
#include "idatabase.h"
#include "sexdelegate.h"

#include <QHeaderView>
#include <QMessageBox>

PatientView::PatientView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PatientView)
{
    ui->setupUi(this);
    setWindowTitle("患者管理");

    IDatabase &db = IDatabase::getInstance();
    if (db.initPatientModel()) {
        ui->tableView->setModel(db.patientTabModel);
        ui->tableView->setSelectionModel(db.thePatientSelection);

        // 使用自定义代理完成性别列（0/1）的中文渲染与下拉编辑
        SexDelegate *sexDelegate = new SexDelegate(this);
        const int sexCol = db.patientTabModel->fieldIndex("SEX");
        ui->tableView->setItemDelegateForColumn(sexCol, sexDelegate);
        connect(sexDelegate, &QAbstractItemDelegate::commitData, this, [&]() {
            if (!IDatabase::getInstance().submitPatientEdit()) {
                QMessageBox::warning(this, "保存失败", IDatabase::getInstance().lastError());
                IDatabase::getInstance().revertPatientEdit();
            } else {
                IDatabase::getInstance().addHistoryEvent("通过表格代理更新患者信息");
                updateActionStates();
            }
        });

        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked);
        ui->tableView->setAlternatingRowColors(true);
        ui->tableView->verticalHeader()->setVisible(false);
        ui->tableView->verticalHeader()->setDefaultSectionSize(36);
        ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        ui->tableView->horizontalHeader()->setStretchLastSection(true);
        ui->tableView->resizeColumnsToContents();

        connect(db.thePatientSelection, &QItemSelectionModel::selectionChanged,
                this, &PatientView::updateActionStates);
    }

    connect(ui->txtSearch, &QLineEdit::returnPressed, this, &PatientView::on_btSearch_clicked);
    updateActionStates();
}

PatientView::~PatientView()
{
    delete ui;
}

void PatientView::refreshTable()
{
    IDatabase &db = IDatabase::getInstance();
    if (db.patientTabModel) {
        db.patientTabModel->select();
        ui->tableView->resizeColumnsToContents();
        ui->tableView->horizontalHeader()->setStretchLastSection(true);
    }
    updateActionStates();
}

void PatientView::updateActionStates()
{
    IDatabase &db = IDatabase::getInstance();
    int totalRows = db.patientTabModel ? db.patientTabModel->rowCount() : 0;
    bool hasSelection = db.thePatientSelection && db.thePatientSelection->currentIndex().isValid();

    ui->btDelete->setEnabled(hasSelection);
    ui->btEdit->setEnabled(hasSelection);

    if (totalRows == 0) {
        ui->labelStatus->setText("当前没有匹配的患者记录，请更换关键字或点击“重置”显示全部。");
        ui->labelStatus->setStyleSheet("color: #DC2626; font-weight: 600;");
    } else {
        ui->labelStatus->setText(QString("当前共显示 %1 条患者记录（提示：选中记录可点修改，或双击单元格使用代理直接编辑性别）").arg(totalRows));
        ui->labelStatus->setStyleSheet("color: #475569;");
    }
}

void PatientView::on_btSearch_clicked()
{
    QString keyword = ui->txtSearch->text().trimmed();
    keyword.replace("'", "''"); // 防止单引号破坏 SQL 过滤条件

    QString filter;
    if (!keyword.isEmpty()) {
        filter = QString("NAME LIKE '%%1%' OR ID LIKE '%%1%' OR ID_CARD LIKE '%%1%' OR MOBILEPHONE LIKE '%%1%'")
                     .arg(keyword);
    }

    if (!IDatabase::getInstance().searchPatient(filter)) {
        QMessageBox::warning(this, "查询失败", IDatabase::getInstance().lastError());
        return;
    }

    if (IDatabase::getInstance().patientTabModel->rowCount() == 0 && !keyword.isEmpty()) {
        QMessageBox::information(this, "查询结果", QString("未找到包含 \"%1\" 的患者记录！").arg(ui->txtSearch->text().trimmed()));
    }
    updateActionStates();
}

void PatientView::on_btReset_clicked()
{
    ui->txtSearch->clear();
    IDatabase::getInstance().searchPatient("");
    refreshTable();
}

void PatientView::on_btAdd_clicked()
{
    ui->txtSearch->clear();
    int curRow = IDatabase::getInstance().addNewPatient();
    if (curRow < 0) {
        QMessageBox::critical(this, "错误", "无法创建新患者记录，请检查数据库连接！");
        return;
    }
    emit goPatientEditView(curRow, true);
}

void PatientView::on_btDelete_clicked()
{
    IDatabase &db = IDatabase::getInstance();
    if (!db.thePatientSelection || !db.thePatientSelection->currentIndex().isValid()) {
        QMessageBox::information(this, "提示", "请先选择要删除的患者记录！");
        return;
    }

    const int row = db.thePatientSelection->currentIndex().row();
    const QString id = db.patientTabModel->data(db.patientTabModel->index(row, db.patientTabModel->fieldIndex("ID"))).toString();
    const QString name = db.patientTabModel->data(db.patientTabModel->index(row, db.patientTabModel->fieldIndex("NAME"))).toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        QString("确定要删除患者 \"%1\" (ID: %2) 吗？\n删除后将无法恢复！").arg(name, id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (!db.deleteCurrentPatient()) {
            QMessageBox::critical(this, "删除失败", db.lastError());
        } else {
            updateActionStates();
        }
    }
}

void PatientView::on_btEdit_clicked()
{
    IDatabase &db = IDatabase::getInstance();
    if (!db.thePatientSelection || !db.thePatientSelection->currentIndex().isValid()) {
        QMessageBox::information(this, "提示", "请先选择要修改的患者记录！");
        return;
    }

    int curRow = db.thePatientSelection->currentIndex().row();
    emit goPatientEditView(curRow, false);
}
