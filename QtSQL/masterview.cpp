#include "masterview.h"
#include "ui_masterview.h"
#include "idatabase.h"
#include "loginview.h"
#include "welcomeview.h"
#include "patientview.h"
#include "patienteditview.h"
#include "doctorview.h"
#include "departmentview.h"
#include "historyview.h"

#include <QMessageBox>
#include <QSizePolicy>
#include <QStyle>

MasterView::MasterView(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MasterView)
    , m_loginView(nullptr)
    , m_welcomeView(nullptr)
    , m_patientView(nullptr)
    , m_doctorView(nullptr)
    , m_departmentView(nullptr)
    , m_historyView(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("医院诊疗管理系统 - MVC数据库应用");
    resize(960, 640);

    // 1. 通过代码化方法定制顶部工具栏与状态栏
    setupCustomToolBar();
    setupStatusBarInfo();
    applyStyleSheet();

    connect(ui->stackedWidget, &QStackedWidget::currentChanged,
            this, &MasterView::onCurrentViewChanged);

    // 2. 初始化数据库连接（含明确失败提示）
    QString dbErr;
    bool dbOk = IDatabase::getInstance().initDatabase(QString(), &dbErr);

    // 3. 进入登录页面
    goLoginView();

    if (dbOk) {
        m_statusDbLabel->setText("数据库状态：已连接 (SQLite)");
        m_statusDbLabel->setStyleSheet("color: #15803D; font-weight: 600;");
    } else {
        m_statusDbLabel->setText("数据库状态：连接失败");
        m_statusDbLabel->setStyleSheet("color: #DC2626; font-weight: 600;");
        if (m_loginView) {
            m_loginView->setLoginEnabled(false);
        }
        QMessageBox::critical(this, "数据库连接失败",
                              QString("无法连接到系统数据库！\n\n原因：%1\n\n请检查 hospital.db 文件及 Navicat 表结构后重新启动程序。").arg(dbErr));
    }
}

MasterView::~MasterView()
{
    delete ui;
}

void MasterView::setupCustomToolBar()
{
    m_topToolBar = new QToolBar("导航工具栏", this);
    m_topToolBar->setObjectName("topNavToolBar");
    m_topToolBar->setMovable(false);
    m_topToolBar->setFloatable(false);
    addToolBar(Qt::TopToolBarArea, m_topToolBar);

    // 左侧：返回按钮
    m_btBack = new QToolButton(this);
    m_btBack->setObjectName("btNavBack");
    m_btBack->setText(" 返回");
    m_btBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    m_btBack->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_btBack->setCursor(Qt::PointingHandCursor);
    connect(m_btBack, &QToolButton::clicked, this, &MasterView::onBackClicked);
    m_topToolBar->addWidget(m_btBack);

    // 左侧弹性占位符
    QWidget *leftSpacer = new QWidget(this);
    leftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_topToolBar->addWidget(leftSpacer);

    // 居中：当前页标题
    m_labelTitle = new QLabel("登录", this);
    m_labelTitle->setObjectName("labelNavTitle");
    m_topToolBar->addWidget(m_labelTitle);

    // 右侧弹性占位符
    QWidget *rightSpacer = new QWidget(this);
    rightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_topToolBar->addWidget(rightSpacer);

    // 右侧：当前登录用户提示与注销按钮
    m_labelCurrentUser = new QLabel("未登录", this);
    m_labelCurrentUser->setObjectName("labelCurrentUser");
    m_topToolBar->addWidget(m_labelCurrentUser);

    m_btLogout = new QToolButton(this);
    m_btLogout->setObjectName("btNavLogout");
    m_btLogout->setText(" 注销");
    m_btLogout->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    m_btLogout->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_btLogout->setCursor(Qt::PointingHandCursor);
    connect(m_btLogout, &QToolButton::clicked, this, &MasterView::onLogoutClicked);
    m_topToolBar->addWidget(m_btLogout);
}

void MasterView::setupStatusBarInfo()
{
    m_statusDbLabel = new QLabel("数据库状态：检测中...", this);
    m_statusDeveloperLabel = new QLabel("开发者：余昊哲   学号：2024414290339   班级：软件工程3班", this);
    m_statusDeveloperLabel->setStyleSheet("color: #334155; font-weight: 600; padding-right: 10px;");

    ui->statusbar->addWidget(m_statusDbLabel);
    ui->statusbar->addPermanentWidget(m_statusDeveloperLabel);
}

void MasterView::pushWidgetToStackView(QWidget *widget)
{
    ui->stackedWidget->addWidget(widget);
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->count() - 1);
}

void MasterView::goLoginView()
{
    if (!m_loginView) {
        m_loginView = new LoginView(this);
        connect(m_loginView, &LoginView::loginSuccess, this, &MasterView::goWelcomeView);
    }
    pushWidgetToStackView(m_loginView);
    m_labelCurrentUser->setText("未登录");
}

void MasterView::goWelcomeView()
{
    m_welcomeView = new WelcomeView(this);
    connect(m_welcomeView, &WelcomeView::goDepartmentView, this, &MasterView::goDepartmentView);
    connect(m_welcomeView, &WelcomeView::goDoctorView, this, &MasterView::goDoctorView);
    connect(m_welcomeView, &WelcomeView::goPatientView, this, &MasterView::goPatientView);
    connect(m_welcomeView, &WelcomeView::goHistoryView, this, &MasterView::goHistoryView);

    const QString userFull = IDatabase::getInstance().currentUserFullName();
    m_labelCurrentUser->setText(QString("当前用户：%1  ").arg(userFull));
    pushWidgetToStackView(m_welcomeView);
}

void MasterView::goPatientView()
{
    m_patientView = new PatientView(this);
    connect(m_patientView, &PatientView::goPatientEditView, this, &MasterView::goPatientEditView);
    pushWidgetToStackView(m_patientView);
}

void MasterView::goPatientEditView(int rowNo, bool isNew)
{
    PatientEditView *editView = new PatientEditView(this, rowNo, isNew);
    connect(editView, &PatientEditView::goPreviousView, this, &MasterView::goPreviousView);
    pushWidgetToStackView(editView);
}

void MasterView::goDoctorView()
{
    m_doctorView = new DoctorView(this);
    pushWidgetToStackView(m_doctorView);
}

void MasterView::goDepartmentView()
{
    m_departmentView = new DepartmentView(this);
    pushWidgetToStackView(m_departmentView);
}

void MasterView::goHistoryView()
{
    m_historyView = new HistoryView(this);
    pushWidgetToStackView(m_historyView);
}

void MasterView::goPreviousView()
{
    int count = ui->stackedWidget->count();
    if (count > 1) {
        QWidget *curWidget = ui->stackedWidget->widget(count - 1);
        ui->stackedWidget->setCurrentIndex(count - 2);
        ui->stackedWidget->removeWidget(curWidget);
        if (curWidget != m_loginView) {
            curWidget->deleteLater();
        }

        // 返回列表页后强制刷新最新数据，保证界面与数据库一致
        QWidget *nowWidget = ui->stackedWidget->currentWidget();
        if (auto *pv = qobject_cast<PatientView *>(nowWidget)) {
            pv->refreshTable();
        } else if (auto *dv = qobject_cast<DoctorView *>(nowWidget)) {
            dv->refreshTable();
        } else if (auto *dpv = qobject_cast<DepartmentView *>(nowWidget)) {
            dpv->refreshTable();
        } else if (auto *hv = qobject_cast<HistoryView *>(nowWidget)) {
            hv->refreshTable();
        }
    }
}

void MasterView::onBackClicked()
{
    // 若在患者编辑页面点击顶部返回按钮，自动回滚未提交修改，防止残留空记录
    if (auto *editView = qobject_cast<PatientEditView *>(ui->stackedWidget->currentWidget())) {
        editView->cancelPendingEdit();
    }
    goPreviousView();
}

void MasterView::onLogoutClicked()
{
    if (ui->stackedWidget->currentWidget() == m_loginView) {
        return;
    }

    if (QMessageBox::question(this, "注销确认", "确定要注销当前账号并返回登录界面吗？",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 清理除 LoginView 外的所有视图栈
    while (ui->stackedWidget->count() > 1) {
        QWidget *w = ui->stackedWidget->widget(ui->stackedWidget->count() - 1);
        if (auto *editView = qobject_cast<PatientEditView *>(w)) {
            editView->cancelPendingEdit();
        }
        ui->stackedWidget->removeWidget(w);
        if (w != m_loginView) {
            w->deleteLater();
        }
    }

    IDatabase::getInstance().logout();
    if (m_loginView) {
        m_loginView->resetInputs();
    }
    m_labelCurrentUser->setText("未登录");
    ui->stackedWidget->setCurrentWidget(m_loginView);
}

void MasterView::onCurrentViewChanged(int index)
{
    Q_UNUSED(index)
    QWidget *cur = ui->stackedWidget->currentWidget();
    if (!cur) return;

    m_labelTitle->setText(cur->windowTitle());

    const bool isLogin = (cur == m_loginView);
    const int count = ui->stackedWidget->count();

    // 在登录页与欢迎首页不显示顶部返回按钮（或登录页禁用注销）
    m_btBack->setEnabled(!isLogin && count > 2);
    m_btBack->setVisible(!isLogin && count > 2);
    m_btLogout->setEnabled(!isLogin);
    m_btLogout->setVisible(!isLogin);
}

void MasterView::applyStyleSheet()
{
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #F4F7FA;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 14px;
            color: #1E293B;
        }
        QToolBar#topNavToolBar {
            background-color: #FFFFFF;
            border: none;
            border-bottom: 1px solid #DCE3EC;
            padding: 6px 14px;
            spacing: 8px;
        }
        QLabel#labelNavTitle {
            font-size: 18px;
            font-weight: 700;
            color: #0F172A;
        }
        QLabel#labelCurrentUser {
            font-size: 13px;
            color: #475569;
        }
        QToolButton#btNavBack, QToolButton#btNavLogout {
            background-color: #F1F5F9;
            border: 1px solid #CBD5E1;
            border-radius: 6px;
            padding: 5px 12px;
            color: #334155;
            font-weight: 600;
        }
        QToolButton#btNavBack:hover, QToolButton#btNavLogout:hover {
            background-color: #E2E8F0;
        }
        QFrame#loginCard, QFrame#editCard {
            background-color: #FFFFFF;
            border: 1px solid #DCE3EC;
            border-radius: 12px;
        }
        QLabel#labelWelcomeTitle {
            font-size: 22px;
            font-weight: 700;
            color: #1D4ED8;
            padding-bottom: 6px;
        }
        QLabel#labelGreeting {
            font-size: 18px;
            font-weight: 600;
            color: #1E293B;
        }
        QLabel#labelTip {
            font-size: 12px;
            color: #64748B;
        }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 6px;
            padding: 6px 10px;
            min-height: 22px;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QDoubleSpinBox:focus {
            border-color: #2563EB;
        }
        QLineEdit:read-only {
            background-color: #F1F5F9;
            color: #64748B;
        }
        QPushButton {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 600;
            color: #334155;
        }
        QPushButton:hover {
            background-color: #F1F5F9;
            border-color: #94A3B8;
        }
        QPushButton:disabled {
            background-color: #F1F5F9;
            color: #94A3B8;
            border-color: #E2E8F0;
        }
        QPushButton#btSignIn, QPushButton#btSave, QPushButton#btSearch {
            background-color: #2563EB;
            border: 1px solid #2563EB;
            color: #FFFFFF;
        }
        QPushButton#btSignIn:hover, QPushButton#btSave:hover, QPushButton#btSearch:hover {
            background-color: #1D4ED8;
        }
        QPushButton#btAdd {
            background-color: #16A34A;
            border: 1px solid #16A34A;
            color: #FFFFFF;
        }
        QPushButton#btAdd:hover {
            background-color: #15803D;
        }
        QPushButton#btDelete {
            background-color: #DC2626;
            border: 1px solid #DC2626;
            color: #FFFFFF;
        }
        QPushButton#btDelete:hover {
            background-color: #B91C1C;
        }
        QPushButton#btDelete:disabled {
            background-color: #FCA5A5;
            border-color: #FCA5A5;
            color: #FFFFFF;
        }
        QPushButton#btEdit {
            background-color: #0284C7;
            border: 1px solid #0284C7;
            color: #FFFFFF;
        }
        QPushButton#btEdit:hover {
            background-color: #0369A1;
        }
        QPushButton#btEdit:disabled {
            background-color: #BAE6FD;
            border-color: #BAE6FD;
            color: #FFFFFF;
        }
        QPushButton#btDepartment, QPushButton#btDoctor, QPushButton#btPatient, QPushButton#btHistory {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 12px;
            font-size: 16px;
            font-weight: 700;
            color: #1E293B;
        }
        QPushButton#btDepartment:hover, QPushButton#btDoctor:hover,
        QPushButton#btPatient:hover, QPushButton#btHistory:hover {
            background-color: #EFF6FF;
            border: 2px solid #2563EB;
            color: #1D4ED8;
        }
        QTableView {
            background-color: #FFFFFF;
            alternate-background-color: #F8FAFC;
            border: 1px solid #DCE3EC;
            border-radius: 8px;
            gridline-color: #E2E8F0;
            selection-background-color: #DBEAFE;
            selection-color: #1E3A8A;
        }
        QHeaderView::section {
            background-color: #EFF6FF;
            color: #1E3A8A;
            font-weight: 700;
            padding: 8px;
            border: none;
            border-bottom: 1px solid #BFDBFE;
            border-right: 1px solid #E2E8F0;
        }
        QStatusBar {
            background-color: #FFFFFF;
            border-top: 1px solid #DCE3EC;
        }
    )");
}
