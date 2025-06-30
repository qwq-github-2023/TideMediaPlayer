#include "TideMediaPlayer.h"
#include <QtWidgets/QApplication>


int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("TideMediaPlayer Contributors");
    QCoreApplication::setOrganizationDomain("https://github.com/qwq-github-2023/TideMediaPlayer");
    QCoreApplication::setApplicationName("TideMediaPlayer");

    QApplication a(argc, argv);
    TideMediaPlayer w;
    w.show();
    return a.exec();
}
