#pragma once
#include <cstdio>
#include <iostream>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <qbuffer.h>
#include <QAudioFormat>

#include <deque>

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
#pragma comment(lib, "swscale.lib")


#define TMH_UNKNOWN -1
#define TMH_VIDEO 0
#define TMH_IMAGE 1
#define TMH_AUDIO 2


struct VedioData {
	std::deque<QPixmap>* image = nullptr;
	QBuffer* pcmBuffer = nullptr;
};


class TideMediaHandle : public QFile
{
	Q_OBJECT;
public:
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
	int getMediaType();
	QPixmap getImagePixmap();
	QBuffer* decodeAudioToQBuffer(uint64_t preDecodingSec);
	QBuffer* decodeAudioToQBuffer(uint64_t preDecodingSec, float tempo);
	VedioData decodeVedioToQBuffer(uint64_t preDecodingSec);
	QAudioFormat getAudioInfo();
	bool setPlayTimestamp(uint64_t timestamp);
	int64_t getDuration();

	
private:
	bool mediaFileInit();
	bool loadAudio();
	bool loadVideo();

	int mediaType = TMH_UNKNOWN;
	AVFormatContext* formatContext = nullptr;
	AVCodecContext* audioCodecCtx = nullptr;
	AVCodecContext* vedioCodecCtx = nullptr;
	int audioIndex;
	int vedioIndex;

	const static int MAXERRORTRY = 10;
};

