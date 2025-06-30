#include "TideMediaHandle.h"
#include <QMimeDatabase>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")

TideMediaHandle::TideMediaHandle()
{
}

TideMediaHandle::~TideMediaHandle()
{
}

bool TideMediaHandle::loadMedia()
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(this->fileName());
    qDebug() << type.name();
    if (type.name().startsWith("audio/")) {
        mediaType = TMH_AUDIO;
    } else if (type.name().startsWith("image/"))  {
        mediaType = TMH_IMAGE;
    } else if (type.name().startsWith("video/"))  {
        mediaType = TMH_VIDEO;
    } else {
        mediaType = TMH_UNKNOWN;
    }
    this->open(QIODevice::ReadOnly);
    return mediaType != TMH_UNKNOWN ? true : false;
}

int TideMediaHandle::getMediaType() {
    if (this)  // 否则会产生构建前引用错误
        return mediaType;
    else
        return TMH_UNKNOWN;
}

QPixmap TideMediaHandle::getPixmap()
{
    this->reset();
    QByteArray imageData = this->readAll();
    QPixmap pixmap;
    if (!pixmap.loadFromData(imageData)) {
        qDebug() << "Unable load image.";
        QMessageBox::critical(nullptr, "错误", "加载图片失败！");
        return QPixmap();
    }
    return pixmap;
}
