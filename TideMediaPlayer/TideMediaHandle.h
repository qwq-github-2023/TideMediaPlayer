#pragma once
#include <cstdio>
#include <iostream>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <QSqlDatabase>
#include <qsqlquery.h>
#include <qbuffer.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")

#define TMH_UNKNOWN -1
#define TMH_VIDEO 0
#define TMH_IMAGE 1
#define TMH_AUDIO 2

namespace Tide {
	QSqlDatabase db;
}


class TideMediaHandle : public QFile
{
public:
	using QFile::QFile;
	TideMediaHandle();
	~TideMediaHandle();

	static void sqlInit();
	static void init();

	bool loadMedia();
	bool loadAudio();
	int getMediaType();
	QPixmap getImagePixmap();
	QBuffer* decodeAudioToQBuffer(uint64_t start_time, uint64_t preDecodingSec, bool isCache = false);
	QBuffer* getPCMAudio(uint64_t start_time, uint64_t preDecodingSec);

	void setCacheAudioNULL();
private:
	int mediaType = TMH_UNKNOWN;
	AVFormatContext* formatContext = nullptr;
	AVCodecContext* codec_ctx = nullptr;
	int audioIndex;
	QBuffer* cacheAudio = nullptr;
};

