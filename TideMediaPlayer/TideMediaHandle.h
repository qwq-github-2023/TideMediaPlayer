#pragma once
#include <cstdio>
#include <iostream>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <qbuffer.h>
#include <QAudioFormat>

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


class TideMediaHandle : public QFile
{
	Q_OBJECT;
public:
	//using QFile::QFile;
	TideMediaHandle()
		: QFile() {
	}

	explicit TideMediaHandle(QObject* parent)
		: QFile(parent) {
	}

	explicit TideMediaHandle(const QString& name)
		: QFile(name) {
	}

	explicit TideMediaHandle(const std::filesystem::path& name)
		: QFile(name) {
	}

	TideMediaHandle(const QString& name, QObject* parent)
		: QFile(name, parent) {
	}

	TideMediaHandle(const std::filesystem::path& name, QObject* parent)
		: QFile(name, parent) {
	}
	
	~TideMediaHandle();

	//static void sqlInit();
	static void init();

	bool loadMedia();
	bool loadAudio();
	int getMediaType();
	QPixmap getImagePixmap();
	QBuffer* decodeAudioToQBuffer(uint64_t preDecodingSec);
	// QBuffer* getPCMAudio(uint64_t startTime, uint64_t preDecodingSec);
	//void setCacheAudioNULL();
	QAudioFormat getAudioInfo();
	bool setPlayTimestamp(uint64_t timestamp);
	int64_t getDuration();
	
private:
	int mediaType = TMH_UNKNOWN;
	AVFormatContext* formatContext = nullptr;
	AVCodecContext* codec_ctx = nullptr;
	int audioIndex;

	const static int MAXERRORTRY = 10;
};

