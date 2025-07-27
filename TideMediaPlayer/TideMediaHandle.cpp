#include "TideMediaHandle.h"
#include <thread>
#include <QMimeDatabase>
#include <QFileInfo>
#include <QDateTime>
#include <vector>
#include <QSqlError>
#include <qbuffer.h>
#include <qmutex.h>

extern "C" void qt_ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vl) {
    if (level > av_log_get_level())
        return;
    char msg[2048];
    vsnprintf(msg, sizeof(msg), fmt, vl);
    msg[sizeof(msg) - 1] = '\0';

    // 你可以根据level转发到不同Qt日志级别
    if (level <= AV_LOG_PANIC)   qCritical() << "[ffmpeg][PANIC]" << msg;
    else if (level <= AV_LOG_FATAL)   qCritical() << "[ffmpeg][FATAL]" << msg;
    else if (level <= AV_LOG_ERROR)   qCritical() << "[ffmpeg][ERROR]" << msg;
    else if (level <= AV_LOG_WARNING) qWarning() << "[ffmpeg][WARNING]" << msg;
    else if (level <= AV_LOG_INFO)    qDebug() << "[ffmpeg][INFO]" << msg;
    else if (level <= AV_LOG_VERBOSE) qDebug() << "[ffmpeg][VERBOSE]" << msg;
    else if (level <= AV_LOG_DEBUG)   qDebug() << "[ffmpeg][DEBUG]" << msg;
    else                             qDebug() << "[ffmpeg][TRACE]" << msg;
}


namespace Tide
{
    static int read_packet(void* opaque, uint8_t* buf, int buf_size) {
        QFile* file = reinterpret_cast<QFile*>(opaque);
        int ret = file->read(reinterpret_cast<char*>(buf), buf_size);
        if (ret == 0) return AVERROR_EOF;
        if (ret < 0) {
            auto err = file->error();
            qDebug() << "Read error:" << file->errorString() << "QFile error enum" << static_cast<int>(err);
        }
        return ret;
    }


    static int64_t seek(void* opaque, int64_t offset, int whence) {
        QFile* file = reinterpret_cast<QFile*>(opaque);

        if (whence == AVSEEK_SIZE) {
            return file->size();
        }

        switch (whence) {
        case SEEK_SET:
            if (!file->seek(offset)) {
                qDebug() << "SEEK_SET error";
                return -1;
            }
            return file->pos();

        case SEEK_CUR:
            if (!file->seek(file->pos() + offset)) {
                qDebug() << "SEEK_CUR error";
                return -1;
            }
            return file->pos();

        case SEEK_END:
            if (!file->seek(file->size() + offset)) {
                qDebug() << "SEEK_END error";
                return -1;
            }
            return file->pos();

        case AVSEEK_SIZE:
            return file->size();

        default:
			qDebug() << "Unsupported seek mode:" << whence;
            return -1;
        }
        return file->pos();
    }
}


TideMediaHandle::~TideMediaHandle()
{
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&formatContext);
}

//void TideMediaHandle::sqlInit() {
//    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
//    db.setDatabaseName("cache.db");
//    bool ok = db.open();
//    if (!ok) {
//        qDebug() << "Database connection failed: ";
//    }
//    else {
//        qDebug() << "Database connection successful.";
//    }
//	Tide::db = db;
//}

void TideMediaHandle::init() {
    //sqlInit();
}

bool TideMediaHandle::loadAudio()
{
    this->reset();
    //QFileInfo fileInfo(*this);

    //QDateTime modified = fileInfo.lastModified();
    //QString createTableSql = QString(R"(
    //    CREATE TABLE IF NOT EXISTS %1 (
    //        startTime INTEGER PRIMARY KEY AUTOINCREMENT,
    //        durationTime INTEGER,
    //        data BLOB
    //    )
    //)").arg(modified.toString());


    //QSqlQuery query;
    //if (!query.exec(createTableSql)) {
    //    qDebug() << "Failed to create table: " << query.lastError().text();
    //}
    //Tide::db.commit();
    //av_log_set_level(AV_LOG_ERROR);
    //av_log_set_callback(qt_ffmpeg_log_callback);
    QFile* audioFile = this;
    //if (!audioFile || !audioFile->isOpen()) {
    //    qDebug() << "QFile pointer is null or file not opened.";
    //    return false;
    //}
    constexpr int bufferSize = 8192;
    uint8_t* buffer = (uint8_t*)av_malloc(bufferSize);

    // 创建AVIOContext，以QFile* audioFile (即this) 为参数
    AVIOContext* avioCtx = avio_alloc_context(buffer, bufferSize, 0, audioFile, Tide::read_packet, nullptr, Tide::seek);

    formatContext = avformat_alloc_context();
    formatContext->pb = avioCtx;

    if (avformat_open_input(&formatContext, nullptr, nullptr, nullptr) < 0) {
        qDebug() << "Could not open input.";
        return false;
    }


    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        qDebug() << "Could not find stream info.";
        avformat_close_input(&formatContext);
        return false;
    }
    audioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    AVStream* st = formatContext->streams[audioIndex];
    //找到解码器
    const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec)
    {
        qDebug() << "Could not find codec.";
        return false;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        qDebug() << "Could not allocate codec context.";
        return false;
    }
    avcodec_parameters_to_context(codec_ctx, st->codecpar);
    avcodec_open2(codec_ctx, codec, nullptr);
    qDebug() << "vedio duration:" << formatContext->duration;
}

bool TideMediaHandle::loadMedia()
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(this->fileName());
    qDebug() << type.name();
    this->open(QIODevice::ReadOnly);
    if (type.name().startsWith("audio/")) {
        mediaType = TMH_AUDIO;
        loadAudio();
    } else if (type.name().startsWith("image/"))  {
        mediaType = TMH_IMAGE;
    } else if (type.name().startsWith("video/"))  {
        mediaType = TMH_VIDEO;
    } else {
        mediaType = TMH_UNKNOWN;
    }
    return mediaType != TMH_UNKNOWN ? true : false;
}

int TideMediaHandle::getMediaType() {
    if (this)  // 否则会产生构建前引用错误
        return mediaType;
    else
        return TMH_UNKNOWN;
}

QPixmap TideMediaHandle::getImagePixmap()
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

QBuffer* TideMediaHandle::decodeAudioToQBuffer(uint64_t preDecodingSec) {
    //this->reset();
    AVChannelLayout out_channel_layout;
    AVChannelLayout in_channel_layout;
    av_channel_layout_default(&out_channel_layout, 2);

    av_channel_layout_copy(&in_channel_layout, &codec_ctx->ch_layout);
    // 初始化SwrContext
    SwrContext* swr_ctx = nullptr;
    int ret = swr_alloc_set_opts2(
        &swr_ctx,
        &out_channel_layout,          // 输出通道布局
        AV_SAMPLE_FMT_S16,            // 输出采样格式
        codec_ctx->sample_rate,       // 输出采样率
        &codec_ctx->ch_layout,        // 输入通道布局
        codec_ctx->sample_fmt,        // 输入采样格式
        codec_ctx->sample_rate,       // 输入采样率
        0,                            // 日志偏移量（通常为0）
        nullptr                       // 日志上下文（通常为nullptr）
    );
    if (ret < 0 || !swr_ctx) {
        qDebug() << "Failed to allocate SwrContext\n";
        av_channel_layout_uninit(&in_channel_layout);
        return nullptr;
    }

    if ((ret = swr_init(swr_ctx)) < 0) {
        qDebug() << "Failed to initialize SwrContext\n";
        swr_free(&swr_ctx);
        av_channel_layout_uninit(&in_channel_layout);
        return nullptr;
    }

    //uint64_t target_pts = startTime; // * (AV_TIME_BASE / 1000);
    //if (av_seek_frame(formatContext, audioIndex, target_pts, AVSEEK_FLAG_BACKWARD) < 0) {
    //    qDebug() << stderr << "Failed to seek.";
    //}
    avcodec_flush_buffers(codec_ctx);
    QBuffer* pcmBuffer = new QBuffer();
    pcmBuffer->open(QIODevice::WriteOnly);
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    uint8_t* buf = (uint8_t*)av_malloc(192000); // 足够大

    uint64_t decoded_duration = 0;
    bool decoding = true;

    int errorTry = 0;

    while (true) {
		int ret = av_read_frame(formatContext, pkt);
        if (ret == AVERROR_EOF)
            break;
        if (decoding && (ret < 0)) {
            if (errorTry >= MAXERRORTRY) {
				qDebug() << "Too many errors, stopping decoding.";
                break;
            }
            else {
                ++errorTry;
                continue;
            }
			qDebug() << "Error reading frame:" << ret;
        }
        if (pkt->stream_index == audioIndex) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                // uint8_t* out_data[1] = { nullptr };
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // 计算当前帧的时长
                    uint64_t frame_duration = (uint64_t)frame->nb_samples * 1000000 / codec_ctx->sample_rate;

                    // 转换音频为S16 PCM格式
                   int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                  
                    /*if (av_samples_alloc(&out_data[0], nullptr, codec_ctx->ch_layout.nb_channels, out_samples, AV_SAMPLE_FMT_S16, 0) < 1) {
                        qDebug() << "Failed to allocate output buffer for audio conversion.";
                        return nullptr;
                    }*/
                    /*int converted = swr_convert(swr_ctx, &out_data[0], out_samples,
                        (const uint8_t**)frame->data, frame->nb_samples);*/
					int converted = swr_convert(swr_ctx, &buf, out_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
                    int bufferSize = av_samples_get_buffer_size(nullptr, codec_ctx->ch_layout.nb_channels, converted, AV_SAMPLE_FMT_S16, 1);
                    if (converted == 0 || bufferSize == 0) continue;
					// qDebug() << "bufferSize:" << bufferSize;
                    // fwrite(out_data[0], 1, converted * codec_ctx->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16), pcm_file);
                    // pcmBuffer->write(reinterpret_cast<const char*>(out_data[0]), converted * codec_ctx->ch_layout.nb_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
                    pcmBuffer->write(reinterpret_cast<const char*>(buf), bufferSize);
                    //av_freep(&out_data[0]);
                    decoded_duration += frame_duration;

                    if (decoded_duration >= preDecodingSec/*(preDecodingSec / 1000.0)*/) {
                        decoding = false;
                        break;
                    }
                    //out_data[0] = nullptr ;
                }
                
            }
        }
    }

    pcmBuffer->close();
    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr_ctx);
    av_channel_layout_uninit(&in_channel_layout);
    return pcmBuffer;
}

//QBuffer* TideMediaHandle::getPCMAudio(uint64_t startTime, uint64_t preDecodingSec)
//{
//    startTime *= 1000;
//    preDecodingSec *= 1000;
//    
//    if (startTime > formatContext->duration) {
//		qWarning() << "Start time exceeds media duration.";
//        return nullptr;
//    }
//
//    if (startTime + preDecodingSec > formatContext->duration) {
//        preDecodingSec = formatContext->duration - startTime;
//        cacheAudio = nullptr;
//    }
//    else {
//        if (startTime + 2 * preDecodingSec > formatContext->duration) {
//            std::thread(&TideMediaHandle::decodeAudioToQBuffer, this, startTime + preDecodingSec, formatContext->duration - startTime - preDecodingSec, true).detach();
//        }
//        else {
//            std::thread(&TideMediaHandle::decodeAudioToQBuffer, this, startTime + preDecodingSec, preDecodingSec, true).detach();
//        }
//    }
//    return t ? t : decodeAudioToQBuffer(startTime, preDecodingSec);
//}

//void TideMediaHandle::setCacheAudioNULL() {
//	if (cacheAudio) delete cacheAudio;
//    cacheAudio = nullptr;
//    return;
//}

QAudioFormat TideMediaHandle::getAudioInfo()
{
    QAudioFormat format;
    format.setSampleRate(codec_ctx->sample_rate);
    format.setChannelCount(codec_ctx->ch_layout.nb_channels);
    format.setSampleFormat(QAudioFormat::Int16);
    return format;
}

bool TideMediaHandle::setPlayTimestamp(uint64_t timestamp) {
    if (av_seek_frame(formatContext, audioIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
        qDebug() << stderr << "Failed to seek.";
        return false;
    }
    return true;
}

int64_t TideMediaHandle::getDuration()
{
	return formatContext ? formatContext->duration : 0;
}
