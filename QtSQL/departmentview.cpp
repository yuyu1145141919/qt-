#include "departmentview.h"
#include "ui_departmentview.h"
#include "idatabase.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>

DepartmentView::DepartmentView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DepartmentView)
{
    ui->setupUi(this);
    setWindowTitle("科室管理");

    IDatabase &db = IDatabase::getInstance();
    if (db.initDepartmentModel()) {
        ui->tableView->setModel(db.departmentTabModel);
        ui->tableView->setSelectionModel(db.theDepartmentSelection);

        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setAlternatingRowColors(true);
        ui->tableView->verticalHeader()->setVisible(false);
        ui->tableView->verticalHeader()->setDefaultSectionSize(36);
        ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        connect(db.theDepartmentSelection, &QItemSelectionModel::selectionChanged,
                this, &DepartmentView::updateActionStates);
    }

    connect(ui->txtSearch, &QLineEdit::returnPressed, this, &DepartmentView::on_btSearch_clicked);
    updateActionStates();
}

DepartmentView::~DepartmentView()
{
    delete ui;
}

void DepartmentView::refreshTable()
{
    if (IDatabase::getInstance().departmentTabModel) {
        IDatabase::getInstance().departmentTabModel->select();
    }
    updateActionStates();
}

void DepartmentView::updateActionStates()
{
    IDatabase &db = IDatabase::getInstance();
    int total = db.departmentTabModel ? db.departmentTabModel->rowCount() : 0;
    bool hasSel = db.theDepartmentSelection && db.theDepartmentSelection->currentIndex().isValid();
    ui->btDelete->setEnabled(hasSel);
    ui->btEdit->setEnabled(hasSel);
    ui->labelStatus->setText(total == 0 ? "未查询到匹配的科室信息"
                                        : QString("当前共显示 %1 个科室").arg(total));
}

void DepartmentView::on_btSearch_clicked()
{
    QString kw = ui->txtSearch->text().trimmed();
    kw.replace("'", "''");
    QString filter;
    if (!kw.isEmpty()) {
        filter = QString("NAME LIKE '%%1%' OR ID LIKE '%%1%'").arg(kw);
    }
    IDatabase::getInstance().searchDepartment(filter);
    updateActionStates();
}

void DepartmentView::on_btReset_clicked()
{
    ui->txtSearch->clear();
    IDatabase::getInstance().searchDepartment("");
    refreshTable();
}

void DepartmentView::on_btAdd_clicked()
{
    const QString nextId = IDatabase::getInstance().generateNextId("Department", "DEP", 3);
    openDeptDialog(true, nextId);
}

void DepartmentView::on_btEdit_clicked()
{
    IDatabase &db = IDatabase::getInstance();
    if (!db.theDepartmentSelection || !db.theDepartmentSelection->currentIndex().isValid()) return;
    int row = db.theDepartmentSelection->currentIndex().row();
    QString id = db.departmentTabModel->data(db.departmentTabModel->index(row, 0)).toString();
    QString name = db.departmentTabModel->data(db.departmentTabModel->index(row, 1)).toString();
    openDeptDialog(false, id, name);
}

void DepartmentView::on_btDelete_clicked()
{
    IDatabase &db = IDatabase::getInstance();
    if (!db.theDepartmentSelection || !db.theDepartmentSelection->currentIndex().isValid()) return;
    int row = db.theDepartmentSelection->currentIndex().row();
    QString name = db.departmentTabModel->data(db.departmentTabModel->index(row, 1)).toString();

    if (QMessageBox::question(this, "确认删除", QString("确定要删除科室 \"%1\" 吗？").arg(name),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        QString err;
        if (!db.deleteCurrentDepartment(&err)) {
            QMessageBox::warning(this, "无法删除", err);
        } else {
            updateActionStates();
        }
    }
}

void DepartmentView::openDeptDialog(bool isNew, const QString &id, const QString &name)
{
    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? "新增科室" : "修改科室");
    dlg.setMinimumWidth(320);

    QFormLayout *form = new QFormLayout(&dlg);
    form->setSpacing(12);
    form->setContentsMargins(20, 20, 20, 20);

    QLineEdit *editId = new QLineEdit(id, &dlg);
    editId->setReadOnly(!isNew);
    QLineEdit *editName = new QLineEdit(name, &dlg);
    editName->setPlaceholderText("例如：心内科");

    form->addRow("科室 ID：", editId);
    form->addRow("科室名称：", editName);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    box->button(QDialogButtonBox::Ok)->setText("保存");
    box->button(QDialogButtonBox::Cancel)->setText("取消");
    form->addRow(box);

    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(box, &QDialogButtonBox::accepted, &dlg, [&]() {
        if (editId->text().trimmed().isEmpty() || editName->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "校验失败", "科室 ID 和名称不能为空！");
            return;
        }
        QString err;
        bool ok = isNew
            ? IDatabase::getInstance().addDepartment(editId->text(), editName->text(), &err)
            : IDatabase::getInstance().updateDepartment(editId->text(), editName->text(), &err);
        if (!ok) {
            QMessageBox::warning(&dlg, "保存失败", err);
            return;
        }
        updateActionStates();
        dlg.accept();
    });

    dlg.exec();
}
