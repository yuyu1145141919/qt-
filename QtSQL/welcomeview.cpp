#include "welcomeview.h"
#include "ui_welcomeview.h"
#include "idatabase.h"

WelcomeView::WelcomeView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WelcomeView)
{
    ui->setupUi(this);
    setWindowTitle("欢迎");
    refreshUserGreeting();
}

WelcomeView::~WelcomeView()
{
    delete ui;
}

void WelcomeView::refreshUserGreeting()
{
    const QString fullName = IDatabase::getInstance().currentUserFullName();
    if (!fullName.isEmpty()) {
        ui->labelGreeting->setText(QString("欢迎回来，%1！请选择功能模块：").arg(fullName));
    } else {
        ui->labelGreeting->setText("欢迎使用医院诊疗信息管理系统，请选择功能模块：");
    }
}

void WelcomeView::on_btDepartment_clicked() { emit goDepartmentView(); }
void WelcomeView::on_btDoctor_clicked()     { emit goDoctorView(); }
void WelcomeView::on_btPatient_clicked()    { emit goPatientView(); }
void WelcomeView::on_btHistory_clicked()    { emit goHistoryView(); }
