#ifndef IDATABASE_H
#define IDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QSqlQueryModel>
#include <QItemSelectionModel>
#include <QList>
#include <QPair>

/**
 * @brief 数据库管理单例类 (MVC中的Model/数据访问层)
 * 采用 Meyers' Singleton 模式保证全局唯一数据库连接与数据模型实例
 */
class IDatabase : public QObject
{
    Q_OBJECT

public:
    static IDatabase &getInstance();

    // 初始化与连接检查
    bool initDatabase(const QString &customPath = QString(), QString *errorMsg = nullptr);
    bool isConnected() const;
    QString databasePath() const;
    QString lastError() const;

    // 登录、注册与会话状态
    QString userLogin(const QString &username, const QString &password);
    bool registerUser(const QString &username, const QString &password,
                      const QString &fullname, QString *errorMsg = nullptr);
    void logout();
    QString currentUserId() const;
    QString currentUserName() const;
    QString currentUserFullName() const;

    // 患者数据模型与CRUD
    QSqlTableModel *patientTabModel;
    QItemSelectionModel *thePatientSelection;
    bool initPatientModel();
    bool searchPatient(const QString &filter);
    int addNewPatient();
    bool deleteCurrentPatient();
    bool submitPatientEdit();
    void revertPatientEdit();
    bool isPatientIdExists(const QString &id) const;
    bool isIdCardExists(const QString &idCard, const QString &excludeId = QString()) const;

    // 医生数据模型与CRUD (支持外键关联科室)
    QSqlRelationalTableModel *doctorTabModel;
    QItemSelectionModel *theDoctorSelection;
    bool initDoctorModel();
    bool searchDoctor(const QString &filter);
    bool addDoctor(const QString &id, const QString &empNo, const QString &name,
                   const QString &deptId, QString *errorMsg = nullptr);
    bool updateDoctor(const QString &id, const QString &empNo, const QString &name,
                      const QString &deptId, QString *errorMsg = nullptr);
    bool deleteCurrentDoctor();

    // 科室数据模型与CRUD
    QSqlTableModel *departmentTabModel;
    QItemSelectionModel *theDepartmentSelection;
    bool initDepartmentModel();
    bool searchDepartment(const QString &filter);
    bool addDepartment(const QString &id, const QString &name, QString *errorMsg = nullptr);
    bool updateDepartment(const QString &id, const QString &name, QString *errorMsg = nullptr);
    bool deleteCurrentDepartment(QString *errorMsg = nullptr);
    QList<QPair<QString, QString>> getDepartmentList() const;

    // 用户操作记录数据保存和查询
    QSqlQueryModel *historyQueryModel;
    bool initHistoryModel(const QString &keyword = QString());
    void addHistoryEvent(const QString &eventText);

    // 生成有序主键
    QString generateNextId(const QString &tableName, const QString &prefix, int width = 4) const;

private:
    explicit IDatabase(QObject *parent = nullptr);
    ~IDatabase() override;
    IDatabase(const IDatabase &) = delete;
    IDatabase &operator=(const IDatabase &) = delete;

    QString locateDatabaseFile(const QString &customPath) const;
    bool verifySchema(QString *errorMsg) const;

    QSqlDatabase m_database;
    QString m_dbPath;
    QString m_lastError;
    bool m_connected;

    QString m_currentUserId;
    QString m_currentUsername;
    QString m_currentFullName;
};

#endif // IDATABASE_H
