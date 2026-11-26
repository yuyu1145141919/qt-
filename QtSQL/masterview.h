#ifndef MASTERVIEW_H
#define MASTERVIEW_H

#include <QMainWindow>
#include <QLabel>
#include <QToolButton>
#include <QToolBar>

class LoginView;
class WelcomeView;
class PatientView;
class PatientEditView;
class DoctorView;
class DepartmentView;
class HistoryView;

namespace Ui {
class MasterView;
}

/**
 * @brief 主控制窗口 (MVC导航容器)
 * 采用 QStackedWidget 管理子视图切换，通过代码化方式定制 QToolBar 与状态栏
 */
class MasterView : public QMainWindow
{
    Q_OBJECT

public:
    explicit MasterView(QWidget *parent = nullptr);
    ~MasterView();

public slots:
    void goLoginView();
    void goWelcomeView();
    void goPatientView();
    void goPatientEditView(int rowNo, bool isNew);
    void goDoctorView();
    void goDepartmentView();
    void goHistoryView();
    void goPreviousView();

private slots:
    void onBackClicked();
    void onLogoutClicked();
    void onCurrentViewChanged(int index);

private:
    Ui::MasterView *ui;

    // 代码化定制工具栏控件
    QToolBar *m_topToolBar;
    QToolButton *m_btBack;
    QLabel *m_labelTitle;
    QLabel *m_labelCurrentUser;
    QToolButton *m_btLogout;

    // 状态栏控件
    QLabel *m_statusDbLabel;
    QLabel *m_statusDeveloperLabel;

    // 子界面指针
    LoginView *m_loginView;
    WelcomeView *m_welcomeView;
    PatientView *m_patientView;
    DoctorView *m_doctorView;
    DepartmentView *m_departmentView;
    HistoryView *m_historyView;

    void setupCustomToolBar();
    void setupStatusBarInfo();
    void pushWidgetToStackView(QWidget *widget);
    void applyStyleSheet();
};

#endif // MASTERVIEW_H
