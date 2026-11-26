#ifndef LOGINVIEW_H
#define LOGINVIEW_H

#include <QWidget>

namespace Ui {
class LoginView;
}

class LoginView : public QWidget
{
    Q_OBJECT

public:
    explicit LoginView(QWidget *parent = nullptr);
    ~LoginView();

    void resetInputs();
    void setLoginEnabled(bool enabled);

signals:
    void loginSuccess();

private slots:
    void on_btSignIn_clicked();
    void on_btSignUp_clicked();

private:
    Ui::LoginView *ui;
};

#endif // LOGINVIEW_H
