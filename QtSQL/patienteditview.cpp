#include "patienteditview.h"
#include "ui_patienteditview.h"
#include "idatabase.h"

#include <QDate>
#include <QDateTime>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSqlTableModel>

PatientEditView::PatientEditView(QWidget *parent, int index, bool isNew)
    : QWidget(parent)
    , ui(new Ui::PatientEditView)
    , m_dataMapper(new QDataWidgetMapper(this))
    , m_rowIndex(index)
    , m_isNew(isNew)
{
    ui->setupUi(this);
    setWindowTitle(isNew ? "新增患者信息" : "编辑患者信息");

    // 性别下拉选项：索引 0 -> 女，索引 1 -> 男（与数据库 SEX INTEGER 字段对应）
    ui->dbComboSex->clear();
    ui->dbComboSex->addItem("女", 0);
    ui->dbComboSex->addItem("男", 1);

    ui->dbDateEditDOB->setMaximumDate(QDate::currentDate());

    IDatabase &db = IDatabase::getInstance();
    QSqlTableModel *tabModel = db.patientTabModel;

    // 采用 ManualSubmit 策略，确保校验通过后再写入 Model 缓冲
    m_dataMapper->setModel(tabModel);
    m_dataMapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

    m_dataMapper->addMapping(ui->dbEditID, tabModel->fieldIndex("ID"));
    m_dataMapper->addMapping(ui->dbEditName, tabModel->fieldIndex("NAME"));
    m_dataMapper->addMapping(ui->dbEditIDCard, tabModel->fieldIndex("ID_CARD"));
    m_dataMapper->addMapping(ui->dbComboSex, tabModel->fieldIndex("SEX"), "currentIndex");
    m_dataMapper->addMapping(ui->dbDateEditDOB, tabModel->fieldIndex("DOB"));
    m_dataMapper->addMapping(ui->dbSpinHeight, tabModel->fieldIndex("HEIGHT"));
    m_dataMapper->addMapping(ui->dbSpinWeight, tabModel->fieldIndex("WEIGHT"));
    m_dataMapper->addMapping(ui->dbEditMobile, tabModel->fieldIndex("MOBILEPHONE"));

    m_dataMapper->setCurrentIndex(index);

    m_originalId = ui->dbEditID->text().trimmed();
    if (!m_isNew) {
        ui->dbEditID->setReadOnly(true);
    }

    // 动态联动计算年龄预览
    auto refreshAgeLabel = [this](const QDate &dob) {
        ui->labelAgePreview->setText(QString("自动计算年龄：%1 岁").arg(calculateAge(dob)));
    };
    refreshAgeLabel(ui->dbDateEditDOB->date());
    connect(ui->dbDateEditDOB, &QDateEdit::dateChanged, this, refreshAgeLabel);
}

PatientEditView::~PatientEditView()
{
    delete ui;
}

int PatientEditView::calculateAge(const QDate &dob) const
{
    const QDate today = QDate::currentDate();
    int age = today.year() - dob.year();
    if (today.month() < dob.month() ||
        (today.month() == dob.month() && today.day() < dob.day())) {
        --age;
    }
    return qMax(0, age);
}

void PatientEditView::cancelPendingEdit()
{
    IDatabase::getInstance().revertPatientEdit();
}

void PatientEditView::on_btSave_clicked()
{
    IDatabase &db = IDatabase::getInstance();

    const QString id = ui->dbEditID->text().trimmed();
    const QString name = ui->dbEditName->text().trimmed();
    const QString idCard = ui->dbEditIDCard->text().trimmed().toUpper();
    const QString mobile = ui->dbEditMobile->text().trimmed();

    ui->dbEditID->setText(id);
    ui->dbEditName->setText(name);
    ui->dbEditIDCard->setText(idCard);
    ui->dbEditMobile->setText(mobile);

    // 必要字段与业务合法性检查
    if (id.isEmpty()) {
        QMessageBox::warning(this, "字段校验失败", "患者 ID 不能为空！");
        ui->dbEditID->setFocus();
        return;
    }
    if (m_isNew && db.isPatientIdExists(id)) {
        QMessageBox::warning(this, "数据重复", QString("患者 ID \"%1\" 已存在，请更换其他 ID！").arg(id));
        ui->dbEditID->setFocus();
        return;
    }
    if (name.isEmpty()) {
        QMessageBox::warning(this, "字段校验失败", "患者姓名不能为空！");
        ui->dbEditName->setFocus();
        return;
    }
    if (idCard.isEmpty()) {
        QMessageBox::warning(this, "字段校验失败", "身份证号不能为空！");
        ui->dbEditIDCard->setFocus();
        return;
    }

    QRegularExpression idCardRegex("^(?:\\d{15}|\\d{17}[\\dX])$");
    if (!idCardRegex.match(idCard).hasMatch()) {
        QMessageBox::warning(this, "格式校验失败", "请输入有效的 15 位或 18 位身份证号！");
        ui->dbEditIDCard->setFocus();
        return;
    }

    if (db.isIdCardExists(idCard, m_isNew ? QString() : m_originalId)) {
        QMessageBox::warning(this, "数据重复", QString("身份证号 \"%1\" 已登记在系统中，请勿重复建档！").arg(idCard));
        ui->dbEditIDCard->setFocus();
        return;
    }

    QRegularExpression mobileRegex("^1[3-9]\\d{9}$");
    if (!mobileRegex.match(mobile).hasMatch()) {
        QMessageBox::warning(this, "格式校验失败", "请输入有效的 11 位手机号码！");
        ui->dbEditMobile->setFocus();
        return;
    }

    if (ui->dbSpinHeight->value() <= 0 || ui->dbSpinWeight->value() <= 0) {
        QMessageBox::warning(this, "数值校验失败", "身高和体重必须大于 0！");
        return;
    }

    // 提交控件数据至 QSqlTableModel 缓冲
    if (!m_dataMapper->submit()) {
        QMessageBox::warning(this, "提交失败", "映射表单数据到模型失败，请检查输入！");
        return;
    }

    // 显式写入规范化的出生日期、计算年龄与建档时间
    QSqlTableModel *tabModel = db.patientTabModel;
    const QDate dob = ui->dbDateEditDOB->date();
    tabModel->setData(tabModel->index(m_rowIndex, tabModel->fieldIndex("DOB")), dob.toString("yyyy-MM-dd"));
    tabModel->setData(tabModel->index(m_rowIndex, tabModel->fieldIndex("AGE")), calculateAge(dob));
    if (m_isNew) {
        tabModel->setData(tabModel->index(m_rowIndex, tabModel->fieldIndex("CREATEDTIMESTAMP")),
                          QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    }

    if (!db.submitPatientEdit()) {
        QMessageBox::critical(this, "数据库保存失败",
                              QString("写入数据库时出错：%1").arg(db.lastError()));
        return;
    }

    db.addHistoryEvent(QString("%1患者记录：%2 (ID: %3)")
                           .arg(m_isNew ? "新增" : "修改", name, id));
    emit goPreviousView();
}

void PatientEditView::on_btCancel_clicked()
{
    cancelPendingEdit();
    emit goPreviousView();
}
