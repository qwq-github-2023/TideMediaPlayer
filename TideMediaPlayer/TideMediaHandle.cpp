#include "TideMediaHandle.h"
#include <QMimeDatabase>
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
