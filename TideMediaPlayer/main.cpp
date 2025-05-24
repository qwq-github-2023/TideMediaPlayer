#include "TideMediaPlayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TideMediaPlayer w;
    w.show();
    return a.exec();
}
