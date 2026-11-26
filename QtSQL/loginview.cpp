#include "loginview.h"
#include "ui_loginview.h"
#include "idatabase.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>

LoginView::LoginView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginView)
{
    ui->setupUi(this);
    setWindowTitle("登录");

    // 预填默认演示账号方便测试，支持回车直接登录
    ui->inputUserName->setText("admin");
    ui->inputUserPassword->setText("123456");

    connect(ui->inputUserPassword, &QLineEdit::returnPressed, this, &LoginView::on_btSignIn_clicked);
    connect(ui->inputUserName, &QLineEdit::returnPressed, this, [this]() {
        ui->inputUserPassword->setFocus();
    });
}

LoginView::~LoginView()
{
    delete ui;
}

void LoginView::resetInputs()
{
    ui->inputUserPassword->clear();
    ui->inputUserName->setFocus();
}

void LoginView::setLoginEnabled(bool enabled)
{
    ui->btSignIn->setEnabled(enabled);
    ui->btSignUp->setEnabled(enabled);
}

void LoginView::on_btSignIn_clicked()
{
    const QString username = ui->inputUserName->text().trimmed();
    const QString password = ui->inputUserPassword->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "输入提示", "用户名和密码均不能为空！");
        return;
    }

    const QString status = IDatabase::getInstance().userLogin(username, password);
    if (status == "loginOK") {
        emit loginSuccess();
    } else if (status == "wrongUsername") {
        QMessageBox::warning(this, "登录失败", QString("用户名 \"%1\" 不存在，请先注册或检查拼写！").arg(username));
        ui->inputUserName->setFocus();
    } else if (status == "wrongPassword") {
        QMessageBox::warning(this, "登录失败", "密码错误，请重新输入！");
        ui->inputUserPassword->clear();
        ui->inputUserPassword->setFocus();
    } else {
        QMessageBox::critical(this, "数据库错误",
                              QString("无法完成登录校验：%1").arg(IDatabase::getInstance().lastError()));
    }
}

void LoginView::on_btSignUp_clicked()
{
    if (!IDatabase::getInstance().isConnected()) {
        QMessageBox::critical(this, "数据库未连接", "当前数据库未就绪，无法注册新用户！");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("用户注册");
    dlg.setMinimumWidth(340);

    QFormLayout *form = new QFormLayout(&dlg);
    form->setSpacing(12);
    form->setContentsMargins(20, 20, 20, 20);

    QLineEdit *editFullName = new QLineEdit(&dlg);
    editFullName->setPlaceholderText("例如：余昊哲");
    QLineEdit *editUserName = new QLineEdit(&dlg);
    editUserName->setPlaceholderText("请输入登录账号");
    QLineEdit *editPassword = new QLineEdit(&dlg);
    editPassword->setEchoMode(QLineEdit::Password);
    editPassword->setPlaceholderText("至少6位字符");
    QLineEdit *editConfirm = new QLineEdit(&dlg);
    editConfirm->setEchoMode(QLineEdit::Password);
    editConfirm->setPlaceholderText("请再次输入密码");

    form->addRow("真实姓名：", editFullName);
    form->addRow("用 户 名：", editUserName);
    form->addRow("密    码：", editPassword);
    form->addRow("确认密码：", editConfirm);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btnBox->button(QDialogButtonBox::Ok)->setText("立即注册");
    btnBox->button(QDialogButtonBox::Cancel)->setText("取消");
    form->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, [&]() {
        const QString full = editFullName->text().trimmed();
        const QString user = editUserName->text().trimmed();
        const QString pwd = editPassword->text();
        const QString confirm = editConfirm->text();

        if (full.isEmpty() || user.isEmpty() || pwd.isEmpty()) {
            QMessageBox::warning(&dlg, "校验失败", "姓名、用户名和密码均为必填项！");
            return;
        }
        if (pwd.length() < 6) {
            QMessageBox::warning(&dlg, "校验失败", "密码长度不能少于 6 位！");
            return;
        }
        if (pwd != confirm) {
            QMessageBox::warning(&dlg, "校验失败", "两次输入的密码不一致！");
            return;
        }

        QString errMsg;
        if (!IDatabase::getInstance().registerUser(user, pwd, full, &errMsg)) {
            QMessageBox::warning(&dlg, "注册失败", errMsg);
            return;
        }

        QMessageBox::information(&dlg, "注册成功", QString("账号 \"%1\" 注册成功，现在可以登录了！").arg(user));
        ui->inputUserName->setText(user);
        ui->inputUserPassword->clear();
        ui->inputUserPassword->setFocus();
        dlg.accept();
    });

    dlg.exec();
}
