#include "sexdelegate.h"

#include <QComboBox>

SexDelegate::SexDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QString SexDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)
    bool ok = false;
    int sex = value.toInt(&ok);
    if (ok) {
        return (sex == 1) ? "男" : "女";
    }
    return value.toString();
}

QWidget *SexDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    QComboBox *combo = new QComboBox(parent);
    combo->addItem("女", 0);
    combo->addItem("男", 1);
    return combo;
}

void SexDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox *>(editor);
    if (!combo) return;
    int val = index.model()->data(index, Qt::EditRole).toInt();
    int idx = combo->findData(val);
    combo->setCurrentIndex(idx >= 0 ? idx : 1);
}

void SexDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                               const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox *>(editor);
    if (!combo) return;
    model->setData(index, combo->currentData().toInt(), Qt::EditRole);
}

void SexDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}
