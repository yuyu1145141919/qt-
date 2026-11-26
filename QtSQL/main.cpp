#include "masterview.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("医院诊疗系统用户信息管理应用");
    a.setApplicationVersion("1.0");

    MasterView w;
    w.show();

    return a.exec();
}
