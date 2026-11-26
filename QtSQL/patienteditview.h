#ifndef PATIENTEDITVIEW_H
#define PATIENTEDITVIEW_H

#include <QWidget>
#include <QDataWidgetMapper>

namespace Ui {
class PatientEditView;
}

class PatientEditView : public QWidget
{
    Q_OBJECT

public:
    explicit PatientEditView(QWidget *parent = nullptr, int index = 0, bool isNew = false);
    ~PatientEditView();

    void cancelPendingEdit();

signals:
    void goPreviousView();

private slots:
    void on_btSave_clicked();
    void on_btCancel_clicked();

private:
    Ui::PatientEditView *ui;
    QDataWidgetMapper *m_dataMapper;
    int m_rowIndex;
    bool m_isNew;
    QString m_originalId;

    int calculateAge(const QDate &dob) const;
};

#endif // PATIENTEDITVIEW_H
