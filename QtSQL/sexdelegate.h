#ifndef SEXDELEGATE_H
#define SEXDELEGATE_H

#include <QStyledItemDelegate>

/**
 * @brief 自定义性别列代理类
 * 将数据库中的整型性别值 (0: 女, 1: 男) 映射为中文文本展示，并在编辑时提供下拉选择框
 */
class SexDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SexDelegate(QObject *parent = nullptr);

    QString displayText(const QVariant &value, const QLocale &locale) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
};

#endif // SEXDELEGATE_H
