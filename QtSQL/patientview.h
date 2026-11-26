#ifndef PATIENTVIEW_H
#define PATIENTVIEW_H

#include <QWidget>

namespace Ui {
class PatientView;
}

class PatientView : public QWidget
{
    Q_OBJECT

public:
    explicit PatientView(QWidget *parent = nullptr);
    ~PatientView();

    void refreshTable();

signals:
    void goPatientEditView(int index, bool isNew);

private slots:
    void on_btSearch_clicked();
    void on_btReset_clicked();
    void on_btAdd_clicked();
    void on_btDelete_clicked();
    void on_btEdit_clicked();
    void updateActionStates();

private:
    Ui::PatientView *ui;
};

#endif // PATIENTVIEW_H
