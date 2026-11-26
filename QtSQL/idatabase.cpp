#include "idatabase.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>

IDatabase &IDatabase::getInstance()
{
    static IDatabase instance;
    return instance;
}

IDatabase::IDatabase(QObject *parent)
    : QObject(parent)
    , patientTabModel(nullptr)
    , thePatientSelection(nullptr)
    , doctorTabModel(nullptr)
    , theDoctorSelection(nullptr)
    , departmentTabModel(nullptr)
    , theDepartmentSelection(nullptr)
    , historyQueryModel(nullptr)
    , m_connected(false)
{
}

IDatabase::~IDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

QString IDatabase::locateDatabaseFile(const QString &customPath) const
{
    if (!customPath.isEmpty()) {
        return customPath;
    }

    QStringList candidates;
    const QString appDir = QCoreApplication::applicationDirPath();
    candidates << appDir + "/hospital.db";
    candidates << QDir::currentPath() + "/hospital.db";
    candidates << QDir(appDir).absoluteFilePath("../hospital.db");
    candidates << QDir(appDir).absoluteFilePath("../../hospital.db");

    // 兼容 Qt Creator Shadow Build：定位源码目录下的 hospital.db，保证与 Navicat 共享同一份数据
    const QString srcDirUtf8 = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath();
    const QString srcDirLocal = QFileInfo(QString::fromLocal8Bit(__FILE__)).absolutePath();
    candidates << srcDirUtf8 + "/hospital.db";
    candidates << srcDirLocal + "/hospital.db";
    candidates << QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + QString::fromUtf8("/qt代码/hospital.db");

    for (const QString &path : candidates) {
        QFileInfo info(path);
        if (info.exists() && info.isFile()) {
            return info.absoluteFilePath();
        }
    }
    return QString();
}

bool IDatabase::verifySchema(QString *errorMsg) const
{
    QSqlQuery query(m_database);
    if (!query.exec("PRAGMA foreign_keys = ON;")) {
        if (errorMsg) {
            *errorMsg = QString("数据库文件无效或已损坏：%1").arg(query.lastError().text());
        }
        return false;
    }

    const QStringList requiredTables = {"User", "Patient", "Doctor", "Department", "History"};
    for (const QString &table : requiredTables) {
        QSqlQuery checkQuery(m_database);
        checkQuery.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=:name");
        checkQuery.bindValue(":name", table);
        if (!checkQuery.exec() || !checkQuery.next()) {
            if (errorMsg) {
                *errorMsg = QString("数据库缺少必要的数据表 \"%1\"，请检查 Navicat 表结构设计。").arg(table);
            }
            return false;
        }
    }
    return true;
}

bool IDatabase::initDatabase(const QString &customPath, QString *errorMsg)
{
    m_connected = false;
    m_lastError.clear();

    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        m_lastError = "未找到 Qt SQLite 数据库驱动 (QSQLITE)！";
        if (errorMsg) *errorMsg = m_lastError;
        return false;
    }

    m_dbPath = locateDatabaseFile(customPath);
    if (m_dbPath.isEmpty() || !QFileInfo::exists(m_dbPath)) {
        m_lastError = customPath.isEmpty()
            ? "未找到数据库文件 hospital.db，请确认数据库文件已放置在程序目录或项目目录下。"
            : QString("指定的数据库文件不存在：%1").arg(customPath);
        if (errorMsg) *errorMsg = m_lastError;
        return false;
    }

    if (QSqlDatabase::contains("HospitalConnection")) {
        m_database = QSqlDatabase::database("HospitalConnection");
        if (m_database.isOpen()) {
            m_database.close();
        }
    } else {
        m_database = QSqlDatabase::addDatabase("QSQLITE", "HospitalConnection");
    }

    m_database.setDatabaseName(m_dbPath);
    if (!m_database.open()) {
        m_lastError = QString("无法打开数据库：%1").arg(m_database.lastError().text());
        if (errorMsg) *errorMsg = m_lastError;
        return false;
    }

    if (!verifySchema(&m_lastError)) {
        m_database.close();
        if (errorMsg) *errorMsg = m_lastError;
        return false;
    }

    m_connected = true;
    return true;
}

bool IDatabase::isConnected() const
{
    return m_connected && m_database.isOpen();
}

QString IDatabase::databasePath() const
{
    return m_dbPath;
}

QString IDatabase::lastError() const
{
    return m_lastError;
}

QString IDatabase::userLogin(const QString &username, const QString &password)
{
    if (!isConnected()) {
        return "dbError";
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT ID, FULLNAME, USERNAME, PASSWORD FROM User WHERE USERNAME = :username");
    query.bindValue(":username", username.trimmed());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return "dbError";
    }

    if (!query.next()) {
        return "wrongUsername";
    }

    const QString dbPassword = query.value("PASSWORD").toString();
    if (dbPassword != password) {
        return "wrongPassword";
    }

    m_currentUserId = query.value("ID").toString();
    m_currentFullName = query.value("FULLNAME").toString();
    m_currentUsername = query.value("USERNAME").toString();

    addHistoryEvent(QString("用户登录系统：%1 (%2)").arg(m_currentFullName, m_currentUsername));
    return "loginOK";
}

bool IDatabase::registerUser(const QString &username, const QString &password,
                             const QString &fullname, QString *errorMsg)
{
    if (!isConnected()) {
        if (errorMsg) *errorMsg = "数据库未连接。";
        return false;
    }

    QSqlQuery checkQuery(m_database);
    checkQuery.prepare("SELECT COUNT(*) FROM User WHERE USERNAME = :username");
    checkQuery.bindValue(":username", username.trimmed());
    if (!checkQuery.exec() || !checkQuery.next()) {
        if (errorMsg) *errorMsg = checkQuery.lastError().text();
        return false;
    }
    if (checkQuery.value(0).toInt() > 0) {
        if (errorMsg) *errorMsg = QString("用户名 \"%1\" 已被注册，请更换用户名。").arg(username.trimmed());
        return false;
    }

    const QString newId = generateNextId("User", "U", 3);
    QSqlQuery insertQuery(m_database);
    insertQuery.prepare("INSERT INTO User (ID, FULLNAME, USERNAME, PASSWORD) "
                        "VALUES (:id, :fullname, :username, :password)");
    insertQuery.bindValue(":id", newId);
    insertQuery.bindValue(":fullname", fullname.trimmed());
    insertQuery.bindValue(":username", username.trimmed());
    insertQuery.bindValue(":password", password);

    if (!insertQuery.exec()) {
        if (errorMsg) *errorMsg = insertQuery.lastError().text();
        return false;
    }

    // 记录注册事件
    QSqlQuery logQuery(m_database);
    logQuery.prepare("INSERT INTO History (ID, USER_ID, EVENT, TIMESTAMP) VALUES (:id, :uid, :evt, :ts)");
    logQuery.bindValue(":id", generateNextId("History", "H", 4));
    logQuery.bindValue(":uid", newId);
    logQuery.bindValue(":evt", QString("新用户注册：%1 (%2)").arg(fullname.trimmed(), username.trimmed()));
    logQuery.bindValue(":ts", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    logQuery.exec();

    return true;
}

void IDatabase::logout()
{
    if (!m_currentUserId.isEmpty() && isConnected()) {
        addHistoryEvent(QString("用户注销退出：%1 (%2)").arg(m_currentFullName, m_currentUsername));
    }
    m_currentUserId.clear();
    m_currentUsername.clear();
    m_currentFullName.clear();
}

QString IDatabase::currentUserId() const { return m_currentUserId; }
QString IDatabase::currentUserName() const { return m_currentUsername; }
QString IDatabase::currentUserFullName() const { return m_currentFullName; }

// ==================== 患者管理 Model ====================

bool IDatabase::initPatientModel()
{
    if (!isConnected()) return false;

    if (!patientTabModel) {
        patientTabModel = new QSqlTableModel(this, m_database);
        patientTabModel->setTable("Patient");
        patientTabModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
        patientTabModel->setSort(patientTabModel->fieldIndex("ID"), Qt::AscendingOrder);

        patientTabModel->setHeaderData(patientTabModel->fieldIndex("ID"), Qt::Horizontal, "患者ID");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("ID_CARD"), Qt::Horizontal, "身份证号");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("NAME"), Qt::Horizontal, "姓名");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("SEX"), Qt::Horizontal, "性别");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("DOB"), Qt::Horizontal, "出生日期");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("HEIGHT"), Qt::Horizontal, "身高(cm)");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("WEIGHT"), Qt::Horizontal, "体重(kg)");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("MOBILEPHONE"), Qt::Horizontal, "手机号");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("AGE"), Qt::Horizontal, "年龄");
        patientTabModel->setHeaderData(patientTabModel->fieldIndex("CREATEDTIMESTAMP"), Qt::Horizontal, "建档时间");

        thePatientSelection = new QItemSelectionModel(patientTabModel, this);
    }

    if (!patientTabModel->select()) {
        m_lastError = patientTabModel->lastError().text();
        return false;
    }
    return true;
}

bool IDatabase::searchPatient(const QString &filter)
{
    if (!patientTabModel) return false;
    patientTabModel->setFilter(filter);
    return patientTabModel->select();
}

int IDatabase::addNewPatient()
{
    if (!patientTabModel) return -1;

    // 新增前先取消过滤，确保新增行立即可见且索引有效
    patientTabModel->setFilter("");
    patientTabModel->select();

    QSqlRecord curRec = patientTabModel->record();
    const QString newId = generateNextId("Patient", "P", 4);
    curRec.setValue("ID", newId);
    curRec.setValue("ID_CARD", "");
    curRec.setValue("NAME", "");
    curRec.setValue("SEX", 1);
    curRec.setValue("DOB", "2000-01-01");
    curRec.setValue("HEIGHT", 170.0);
    curRec.setValue("WEIGHT", 60.0);
    curRec.setValue("MOBILEPHONE", "");
    curRec.setValue("AGE", QDate::currentDate().year() - 2000);
    curRec.setValue("CREATEDTIMESTAMP", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    const int newRow = patientTabModel->rowCount();
    patientTabModel->insertRecord(newRow, curRec);
    return newRow;
}

bool IDatabase::deleteCurrentPatient()
{
    if (!patientTabModel || !thePatientSelection) return false;

    QModelIndex curIndex = thePatientSelection->currentIndex();
    if (!curIndex.isValid()) return false;

    const QString id = patientTabModel->data(patientTabModel->index(curIndex.row(), patientTabModel->fieldIndex("ID"))).toString();
    const QString name = patientTabModel->data(patientTabModel->index(curIndex.row(), patientTabModel->fieldIndex("NAME"))).toString();

    patientTabModel->removeRow(curIndex.row());
    if (!patientTabModel->submitAll()) {
        m_lastError = patientTabModel->lastError().text();
        patientTabModel->revertAll();
        return false;
    }

    patientTabModel->select();
    addHistoryEvent(QString("删除患者记录：%1 (ID: %2)").arg(name, id));
    return true;
}

bool IDatabase::submitPatientEdit()
{
    if (!patientTabModel) return false;

    if (!patientTabModel->submitAll()) {
        m_lastError = patientTabModel->lastError().text();
        return false;
    }
    patientTabModel->select();
    return true;
}

void IDatabase::revertPatientEdit()
{
    if (patientTabModel) {
        patientTabModel->revertAll();
        patientTabModel->select();
    }
}

bool IDatabase::isPatientIdExists(const QString &id) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM Patient WHERE ID = :id");
    query.bindValue(":id", id.trimmed());
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

bool IDatabase::isIdCardExists(const QString &idCard, const QString &excludeId) const
{
    QSqlQuery query(m_database);
    if (excludeId.isEmpty()) {
        query.prepare("SELECT COUNT(*) FROM Patient WHERE ID_CARD = :idCard");
    } else {
        query.prepare("SELECT COUNT(*) FROM Patient WHERE ID_CARD = :idCard AND ID != :excludeId");
        query.bindValue(":excludeId", excludeId.trimmed());
    }
    query.bindValue(":idCard", idCard.trimmed());
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

// ==================== 医生管理 Model (QSqlRelationalTableModel) ====================

bool IDatabase::initDoctorModel()
{
    if (!isConnected()) return false;

    if (!doctorTabModel) {
        doctorTabModel = new QSqlRelationalTableModel(this, m_database);
        doctorTabModel->setTable("Doctor");
        doctorTabModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

        const int deptCol = doctorTabModel->fieldIndex("DEPARTMENT_ID");
        doctorTabModel->setRelation(deptCol, QSqlRelation("Department", "ID", "NAME"));
        doctorTabModel->setJoinMode(QSqlRelationalTableModel::LeftJoin);

        doctorTabModel->setHeaderData(doctorTabModel->fieldIndex("ID"), Qt::Horizontal, "医生ID");
        doctorTabModel->setHeaderData(doctorTabModel->fieldIndex("EMPLOYEENO"), Qt::Horizontal, "工号");
        doctorTabModel->setHeaderData(doctorTabModel->fieldIndex("NAME"), Qt::Horizontal, "姓名");
        doctorTabModel->setHeaderData(deptCol, Qt::Horizontal, "所属科室");

        theDoctorSelection = new QItemSelectionModel(doctorTabModel, this);
    }

    if (!doctorTabModel->select()) {
        m_lastError = doctorTabModel->lastError().text();
        return false;
    }
    return true;
}

bool IDatabase::searchDoctor(const QString &filter)
{
    if (!doctorTabModel) return false;
    doctorTabModel->setFilter(filter);
    return doctorTabModel->select();
}

bool IDatabase::addDoctor(const QString &id, const QString &empNo, const QString &name,
                          const QString &deptId, QString *errorMsg)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO Doctor (ID, EMPLOYEENO, NAME, DEPARTMENT_ID) "
                  "VALUES (:id, :empNo, :name, :deptId)");
    query.bindValue(":id", id.trimmed());
    query.bindValue(":empNo", empNo.trimmed());
    query.bindValue(":name", name.trimmed());
    query.bindValue(":deptId", deptId);

    if (!query.exec()) {
        if (errorMsg) *errorMsg = query.lastError().text();
        return false;
    }
    if (doctorTabModel) doctorTabModel->select();
    addHistoryEvent(QString("新增医生：%1 (工号: %2)").arg(name.trimmed(), empNo.trimmed()));
    return true;
}

bool IDatabase::updateDoctor(const QString &id, const QString &empNo, const QString &name,
                             const QString &deptId, QString *errorMsg)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE Doctor SET EMPLOYEENO = :empNo, NAME = :name, DEPARTMENT_ID = :deptId "
                  "WHERE ID = :id");
    query.bindValue(":empNo", empNo.trimmed());
    query.bindValue(":name", name.trimmed());
    query.bindValue(":deptId", deptId);
    query.bindValue(":id", id.trimmed());

    if (!query.exec()) {
        if (errorMsg) *errorMsg = query.lastError().text();
        return false;
    }
    if (doctorTabModel) doctorTabModel->select();
    addHistoryEvent(QString("修改医生信息：%1 (ID: %2)").arg(name.trimmed(), id.trimmed()));
    return true;
}

bool IDatabase::deleteCurrentDoctor()
{
    if (!doctorTabModel || !theDoctorSelection) return false;
    QModelIndex curIndex = theDoctorSelection->currentIndex();
    if (!curIndex.isValid()) return false;

    const QString id = doctorTabModel->data(doctorTabModel->index(curIndex.row(), doctorTabModel->fieldIndex("ID"))).toString();
    const QString name = doctorTabModel->data(doctorTabModel->index(curIndex.row(), doctorTabModel->fieldIndex("NAME"))).toString();

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Doctor WHERE ID = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    doctorTabModel->select();
    addHistoryEvent(QString("删除医生：%1 (ID: %2)").arg(name, id));
    return true;
}

// ==================== 科室管理 Model ====================

bool IDatabase::initDepartmentModel()
{
    if (!isConnected()) return false;

    if (!departmentTabModel) {
        departmentTabModel = new QSqlTableModel(this, m_database);
        departmentTabModel->setTable("Department");
        departmentTabModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
        departmentTabModel->setSort(departmentTabModel->fieldIndex("ID"), Qt::AscendingOrder);

        departmentTabModel->setHeaderData(departmentTabModel->fieldIndex("ID"), Qt::Horizontal, "科室ID");
        departmentTabModel->setHeaderData(departmentTabModel->fieldIndex("NAME"), Qt::Horizontal, "科室名称");

        theDepartmentSelection = new QItemSelectionModel(departmentTabModel, this);
    }

    if (!departmentTabModel->select()) {
        m_lastError = departmentTabModel->lastError().text();
        return false;
    }
    return true;
}

bool IDatabase::searchDepartment(const QString &filter)
{
    if (!departmentTabModel) return false;
    departmentTabModel->setFilter(filter);
    return departmentTabModel->select();
}

bool IDatabase::addDepartment(const QString &id, const QString &name, QString *errorMsg)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO Department (ID, NAME) VALUES (:id, :name)");
    query.bindValue(":id", id.trimmed());
    query.bindValue(":name", name.trimmed());

    if (!query.exec()) {
        if (errorMsg) *errorMsg = query.lastError().text();
        return false;
    }
    if (departmentTabModel) departmentTabModel->select();
    addHistoryEvent(QString("新增科室：%1 (ID: %2)").arg(name.trimmed(), id.trimmed()));
    return true;
}

bool IDatabase::updateDepartment(const QString &id, const QString &name, QString *errorMsg)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE Department SET NAME = :name WHERE ID = :id");
    query.bindValue(":name", name.trimmed());
    query.bindValue(":id", id.trimmed());

    if (!query.exec()) {
        if (errorMsg) *errorMsg = query.lastError().text();
        return false;
    }
    if (departmentTabModel) departmentTabModel->select();
    if (doctorTabModel) doctorTabModel->select();
    addHistoryEvent(QString("修改科室名称：%1 (ID: %2)").arg(name.trimmed(), id.trimmed()));
    return true;
}

bool IDatabase::deleteCurrentDepartment(QString *errorMsg)
{
    if (!departmentTabModel || !theDepartmentSelection) return false;
    QModelIndex curIndex = theDepartmentSelection->currentIndex();
    if (!curIndex.isValid()) return false;

    const QString id = departmentTabModel->data(departmentTabModel->index(curIndex.row(), departmentTabModel->fieldIndex("ID"))).toString();
    const QString name = departmentTabModel->data(departmentTabModel->index(curIndex.row(), departmentTabModel->fieldIndex("NAME"))).toString();

    // 外键约束检查：若该科室下仍有医生，不允许直接删除
    QSqlQuery refQuery(m_database);
    refQuery.prepare("SELECT COUNT(*) FROM Doctor WHERE DEPARTMENT_ID = :deptId");
    refQuery.bindValue(":deptId", id);
    if (refQuery.exec() && refQuery.next() && refQuery.value(0).toInt() > 0) {
        if (errorMsg) {
            *errorMsg = QString("科室 \"%1\" 下仍有 %2 名在职医生，请先调整或删除相关医生后再删除该科室！")
                            .arg(name)
                            .arg(refQuery.value(0).toInt());
        }
        return false;
    }

    QSqlQuery delQuery(m_database);
    delQuery.prepare("DELETE FROM Department WHERE ID = :id");
    delQuery.bindValue(":id", id);
    if (!delQuery.exec()) {
        if (errorMsg) *errorMsg = delQuery.lastError().text();
        return false;
    }

    departmentTabModel->select();
    addHistoryEvent(QString("删除科室：%1 (ID: %2)").arg(name, id));
    return true;
}

QList<QPair<QString, QString>> IDatabase::getDepartmentList() const
{
    QList<QPair<QString, QString>> list;
    QSqlQuery query(m_database);
    if (query.exec("SELECT ID, NAME FROM Department ORDER BY ID ASC")) {
        while (query.next()) {
            list.append(qMakePair(query.value(0).toString(), query.value(1).toString()));
        }
    }
    return list;
}

// ==================== 用户操作日志 Model ====================

bool IDatabase::initHistoryModel(const QString &keyword)
{
    if (!isConnected()) return false;

    if (!historyQueryModel) {
        historyQueryModel = new QSqlQueryModel(this);
    }

    QSqlQuery query(m_database);
    if (keyword.trimmed().isEmpty()) {
        query.prepare("SELECT h.ID, COALESCE(u.USERNAME, '系统') AS USERNAME, h.EVENT, h.TIMESTAMP "
                      "FROM History h LEFT JOIN User u ON h.USER_ID = u.ID "
                      "ORDER BY h.TIMESTAMP DESC, h.ID DESC");
    } else {
        query.prepare("SELECT h.ID, COALESCE(u.USERNAME, '系统') AS USERNAME, h.EVENT, h.TIMESTAMP "
                      "FROM History h LEFT JOIN User u ON h.USER_ID = u.ID "
                      "WHERE u.USERNAME LIKE :kw OR h.EVENT LIKE :kw "
                      "ORDER BY h.TIMESTAMP DESC, h.ID DESC");
        query.bindValue(":kw", QString("%%1%").arg(keyword.trimmed()));
    }

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    historyQueryModel->setQuery(query);
    historyQueryModel->setHeaderData(0, Qt::Horizontal, "记录ID");
    historyQueryModel->setHeaderData(1, Qt::Horizontal, "操作用户");
    historyQueryModel->setHeaderData(2, Qt::Horizontal, "操作内容");
    historyQueryModel->setHeaderData(3, Qt::Horizontal, "操作时间");
    return true;
}

void IDatabase::addHistoryEvent(const QString &eventText)
{
    if (!isConnected()) return;

    QSqlQuery query(m_database);
    query.prepare("INSERT INTO History (ID, USER_ID, EVENT, TIMESTAMP) "
                  "VALUES (:id, :uid, :event, :ts)");
    query.bindValue(":id", generateNextId("History", "H", 4));
    query.bindValue(":uid", m_currentUserId.isEmpty() ? QVariant(QVariant::String) : m_currentUserId);
    query.bindValue(":event", eventText);
    query.bindValue(":ts", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.exec();
}

QString IDatabase::generateNextId(const QString &tableName, const QString &prefix, int width) const
{
    int maxNum = 0;
    QSqlQuery query(m_database);
    if (query.exec(QString("SELECT ID FROM %1").arg(tableName))) {
        while (query.next()) {
            const QString id = query.value(0).toString();
            if (id.startsWith(prefix)) {
                bool ok = false;
                const int num = id.mid(prefix.length()).toInt(&ok);
                if (ok && num > maxNum) {
                    maxNum = num;
                }
            }
        }
    }
    return prefix + QString("%1").arg(maxNum + 1, width, 10, QLatin1Char('0'));
}
