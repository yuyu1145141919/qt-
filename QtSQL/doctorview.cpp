#include "doctorview.h"
#include "ui_doctorview.h"
#include "idatabase.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QSqlRelationalDelegate>

DoctorView::DoctorView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DoctorView)
{
    ui->setupUi(this);
    setWindowTitle("医生管理");

    IDatabase &db = IDatabase::getInstance();
    if (db.initDoctorModel()) {
        ui->tableView->setModel(db.doctorTabModel);
        ui->tableView->setSelectionModel(db.theDoctorSelection);
        ui->tableView->setItemDelegate(new QSqlRelationalDelegate(ui->tableView));

        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setAlternatingRowColors(true);
        ui->tableView->verticalHeader()->setVisible(false);
        ui->tableView->verticalHeader()->setDefaultSectionSize(36);
        ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        connect(db.theDoctorSelection, &QItemSelectionModel::selectionChanged,
                this, &DoctorView::updateActionStates);
    }

    connect(ui->txtSearch, &QLineEdit::returnPressed, this, &DoctorView::on_btSearch_clicked);
    updateActionStates();
}

DoctorView::~DoctorView()
{
    delete ui;
}

void DoctorView::refreshTable()
{
    if (IDatabase::getInstance().doctorTabModel) {
        IDatabase::getInstance().doctorTabModel->select();
    }
    updateActionStates();
}

void DoctorView::updateActionStates()
{
    IDatabase &db = IDatabase::getInstance();
    int total = db.doctorTabModel ? db.doctorTabModel->rowCount() : 0;
    bool hasSel = db.theDoctorSelection && db.theDoctorSelection->currentIndex().isValid();
    ui->btDelete->setEnabled(hasSel);
    ui->btEdit->setEnabled(hasSel);
    ui->labelStatus->setText(total == 0 ? "未查询到匹配的医生信息"
                                        : QString("当前共显示 %1 名医生信息").arg(total));
}

void DoctorView::on_btSearch_clicked()
{
    QString kw = ui->txtSearch->text().trimmed();
    kw.replace("'", "''");
    QString filter;
    if (!kw.isEmpty()) {
        filter = QString("Doctor.NAME LIKE '%%1%' OR Doctor.EMPLOYEENO LIKE '%%1%' OR Doctor.ID LIKE '%%1%'").arg(kw);
    }
    IDatabase::getInstance().searchDoctor(filter);
    updateActionStates();
}

void DoctorView::on_btReset_clicked()
{
    ui->txtSearch->clear();
    IDatabase::getInstance().searchDoctor("");
    refreshTable();
}

void DoctorView::on_btAdd_clicked()
{
    const QString nextId = IDatabase::getInstance().generateNextId("Doctor", "DOC", 3);
    openDoctorDialog(true, nextId);
}

void DoctorView::on_btEdit_clicked()
{
    IDatabase &db = IDatabase::getInstance();
    if (!db.theDoctorSelection || !db.theDoctorSelection->currentIndex().isValid()) return;
    int row = db.theDoctorSelection->currentIndex().row();
    QString id = db.doctorTabModel->data(db.doctorTabModel->index(row, 0)).toString();
    QString empNo = db.doctorTabModel->data(db.doctorTabModel->index(row, 1)).toString();
    QString name = db.doctorTabModel->data(db.doctorTabModel->index(row, 2)).toString();
    QString deptName = db.doctorTabModel->data(db.doctorTabModel->index(row, 3)).toString();
    openDoctorDialog(false, id, empNo, name, deptName);
}

void DoctorView::on_btDelete_clicked()
{
    IDatabase &db = IDatabase::getInstance();
    if (!db.theDoctorSelection || !db.theDoctorSelection->currentIndex().isValid()) return;
    int row = db.theDoctorSelection->currentIndex().row();
    QString name = db.doctorTabModel->data(db.doctorTabModel->index(row, 2)).toString();

    if (QMessageBox::question(this, "确认删除", QString("确定要删除医生 \"%1\" 吗？").arg(name),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        if (!db.deleteCurrentDoctor()) {
            QMessageBox::critical(this, "删除失败", db.lastError());
        } else {
            updateActionStates();
        }
    }
}

void DoctorView::openDoctorDialog(bool isNew, const QString &id, const QString &empNo,
                                  const QString &name, const QString &deptName)
{
    auto depts = IDatabase::getInstance().getDepartmentList();
    if (depts.isEmpty()) {
        QMessageBox::warning(this, "提示", "系统中暂无科室信息，请先在“科室管理”中添加科室！");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? "新增医生" : "修改医生信息");
    dlg.setMinimumWidth(340);

    QFormLayout *form = new QFormLayout(&dlg);
    form->setSpacing(12);
    form->setContentsMargins(20, 20, 20, 20);

    QLineEdit *editId = new QLineEdit(id, &dlg);
    editId->setReadOnly(!isNew);
    QLineEdit *editEmpNo = new QLineEdit(empNo, &dlg);
    editEmpNo->setPlaceholderText("例如：D1005");
    QLineEdit *editName = new QLineEdit(name, &dlg);
    editName->setPlaceholderText("请输入医生姓名");

    QComboBox *comboDept = new QComboBox(&dlg);
    int selectIdx = 0;
    for (int i = 0; i < depts.size(); ++i) {
        comboDept->addItem(depts[i].second, depts[i].first);
        if (depts[i].second == deptName) selectIdx = i;
    }
    comboDept->setCurrentIndex(selectIdx);

    form->addRow("医生 ID：", editId);
    form->addRow("工    号：", editEmpNo);
    form->addRow("姓    名：", editName);
    form->addRow("所属科室：", comboDept);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    box->button(QDialogButtonBox::Ok)->setText("保存");
    box->button(QDialogButtonBox::Cancel)->setText("取消");
    form->addRow(box);

    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(box, &QDialogButtonBox::accepted, &dlg, [&]() {
        if (editId->text().trimmed().isEmpty() ||
            editEmpNo->text().trimmed().isEmpty() ||
            editName->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "校验失败", "医生 ID、工号和姓名均不能为空！");
            return;
        }
        QString err;
        bool ok = isNew
            ? IDatabase::getInstance().addDoctor(editId->text(), editEmpNo->text(), editName->text(),
                                                 comboDept->currentData().toString(), &err)
            : IDatabase::getInstance().updateDoctor(editId->text(), editEmpNo->text(), editName->text(),
                                                    comboDept->currentData().toString(), &err);
        if (!ok) {
            QMessageBox::warning(&dlg, "保存失败", err);
            return;
        }
        updateActionStates();
        dlg.accept();
    });

    dlg.exec();
}
