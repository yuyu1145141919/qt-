#ifndef DOCTORVIEW_H
#define DOCTORVIEW_H

#include <QWidget>

namespace Ui {
class DoctorView;
}

class DoctorView : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorView(QWidget *parent = nullptr);
    ~DoctorView();

    void refreshTable();

private slots:
    void on_btSearch_clicked();
    void on_btReset_clicked();
    void on_btAdd_clicked();
    void on_btDelete_clicked();
    void on_btEdit_clicked();
    void updateActionStates();

private:
    Ui::DoctorView *ui;
    void openDoctorDialog(bool isNew, const QString &id = QString(),
                          const QString &empNo = QString(),
                          const QString &name = QString(),
                          const QString &deptName = QString());
};

#endif // DOCTORVIEW_H
