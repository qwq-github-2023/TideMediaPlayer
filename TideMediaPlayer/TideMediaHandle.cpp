#include "TideMediaHandle.h"
#include <thread>
#include <QMimeDatabase>
#include <QFileInfo>
#include <QDateTime>
#include <vector>
#include <QSqlError>
#include <qbuffer.h>

namespace Tide
{
    static int read_packet(void* opaque, uint8_t* buf, int buf_size) {
        QFile* file = reinterpret_cast<QFile*>(opaque);
        int ret = file->read(reinterpret_cast<char*>(buf), buf_size);
        if (ret == 0) return AVERROR_EOF;
        if (ret < 0) return -1;
        return ret;
    }


    static int64_t seek(void* opaque, int64_t offset, int whence) {
        QFile* file = reinterpret_cast<QFile*>(opaque);

        if (whence == AVSEEK_SIZE) {
            return file->size();
        }

        if (!file->seek(offset)) {
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

    QFile* audioFile = this;
    //if (!audioFile || !audioFile->isOpen()) {
    //    qDebug() << "QFile pointer is null or file not opened.";
    //    return false;
    //}
    constexpr int bufferSize = 4096;
    unsigned char* buffer = (unsigned char*)av_malloc(bufferSize);

    // 创建AVIOContext，以QFile* audioFile (即this) 为参数
    AVIOContext* avioCtx = avio_alloc_context(buffer, bufferSize, 0, audioFile, Tide::read_packet, nullptr, Tide::seek);

    AVFormatContext* fmtCtx = avformat_alloc_context();
    fmtCtx->pb = avioCtx;

    if (avformat_open_input(&fmtCtx, nullptr, nullptr, nullptr) < 0) {
        qDebug() << "Could not open input.";
        return false;
    }


    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        qDebug() << "Could not find stream info.";
        avformat_close_input(&formatContext);
        return false;
    }
    audioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

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

QBuffer* TideMediaHandle::decodeAudioToQBuffer(uint64_t startTime, uint64_t preDecodingSec, bool isCache) {
    

    AVChannelLayout out_channel_layout = AV_CHANNEL_LAYOUT_STEREO;
    AVChannelLayout in_channel_layout;


    av_channel_layout_copy(&in_channel_layout, &codec_ctx->ch_layout);
    // 初始化SwrContext
    SwrContext* swr_ctx = nullptr;
    int ret = swr_alloc_set_opts2(
        &swr_ctx,
        &out_channel_layout,          // 输出通道布局
        AV_SAMPLE_FMT_S16,            // 输出采样格式
        codec_ctx->sample_rate,       // 输出采样率
        &in_channel_layout,           // 输入通道布局
        codec_ctx->sample_fmt,        // 输入采样格式
        codec_ctx->sample_rate,       // 输入采样率
        0,                            // 日志偏移量（通常为0）
        nullptr                       // 日志上下文（通常为nullptr）
    );
    if (ret < 0 || !swr_ctx) {
        qDebug() << "Failed to allocate SwrContext\n";
        av_channel_layout_uninit(&in_channel_layout);
        return NULL;
    }

    if ((ret = swr_init(swr_ctx)) < 0) {
        qDebug() << "Failed to initialize SwrContext\n";
        swr_free(&swr_ctx);
        av_channel_layout_uninit(&in_channel_layout);
        return NULL;
    }

    int64_t target_pts = startTime * (AV_TIME_BASE / 1000);
    if (av_seek_frame(formatContext, -1, target_pts, AVSEEK_FLAG_BACKWARD) < 0) {
        fprintf(stderr, "Failed to seek.\n");
    }
    avcodec_flush_buffers(codec_ctx);
    QBuffer* pcmBuffer = new QBuffer();
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    double decoded_duration = 0.0;
    bool decoding = true;

    while (decoding && av_read_frame(formatContext, pkt) >= 0) {
        if (pkt->stream_index == audioIndex) {
            if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // 计算当前帧的时长
                    double frame_duration = (double)frame->nb_samples / codec_ctx->sample_rate;

                    // 转换音频为S16 PCM格式
                    uint8_t* out_data[1] = { nullptr };
                    int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                    int out_buffer_size = av_samples_alloc(out_data, nullptr, 2, out_samples, AV_SAMPLE_FMT_S16, 0);

                    int converted = swr_convert(swr_ctx, out_data, out_samples,
                        (const uint8_t**)frame->data, frame->nb_samples);
                    
                    // fwrite(out_data[0], 1, converted * codec_ctx->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16), pcm_file);
                    pcmBuffer->write(reinterpret_cast<const char*>(&out_data[0]), converted * codec_ctx->ch_layout.nb_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
                    av_freep(&out_data[0]);
                    decoded_duration += frame_duration;

                    if (decoded_duration >= (preDecodingSec / 1000.0)) {
                        decoding = false;
                        break;
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }


    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr_ctx);
    av_channel_layout_uninit(&in_channel_layout);
    if (isCache) {
        cacheAudio = pcmBuffer;
    }
    return pcmBuffer;

 //   QFileInfo fileInfo(*this);
 //   QDateTime modified = fileInfo.lastModified();
 //   QString comm = QString("INSERT INTO %1 (startTime, durationTime, data) VALUES (:startTime, :durationTime, :data)").arg(modified.toString());
 //   QSqlQuery query;
 //   query.prepare(comm);
 //   query.bindValue(":startTime", startTime);
 //   query.bindValue(":durationTime", preDecodingSec);
 //   query.bindValue(":data", pcmBuffer.readAll());

 //   if (!query.exec()) {
 //       qDebug() << "Failed to insert data: " << query.lastError().text();
 //       Tide::db.rollback();
 //       return false;
	//}
	//Tide::db.commit();
}

QBuffer* TideMediaHandle::getPCMAudio(uint64_t start_time, uint64_t preDecodingSec)
{
    if (start_time > formatContext->duration) {
		qDebug() << "Start time exceeds media duration.";
        return NULL;
    }
    if (start_time + preDecodingSec > formatContext->duration) {
		preDecodingSec = formatContext->duration - start_time;
    }
    if (cacheAudio) {
        QBuffer* ret = cacheAudio;
		std::thread(&TideMediaHandle::decodeAudioToQBuffer, this, start_time + preDecodingSec, preDecodingSec, true).detach();
        return ret;
    }
    else {
        std::thread(&TideMediaHandle::decodeAudioToQBuffer, this, start_time + preDecodingSec, preDecodingSec, true).detach();
		return decodeAudioToQBuffer(start_time, preDecodingSec);
    }
}

void TideMediaHandle::setCacheAudioNULL() {
	if (cacheAudio == nullptr) return;
    delete cacheAudio;
    cacheAudio = nullptr;
}

QAudioFormat TideMediaHandle::getAudioInfo()
{
    QAudioFormat format;
    format.setSampleRate(codec_ctx->sample_rate);
    format.setChannelCount(codec_ctx->ch_layout.nb_channels);
    QAudioFormat::SampleFormat sampleFormat;
    int sampleSize = 0;
    switch (codec_ctx->sample_fmt) {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        sampleFormat = QAudioFormat::UInt8;
        break;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        sampleFormat = QAudioFormat::Int16;
        break;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        sampleFormat = QAudioFormat::Int32;
        break;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        sampleFormat = QAudioFormat::Float;
        break;
    default:
        sampleFormat = QAudioFormat::Unknown;
        break;
    }
    format.setSampleFormat(sampleFormat);
    return format;
}
