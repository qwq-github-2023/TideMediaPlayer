#include "TideMediaHandle.h"
#include <thread>
#include <QMimeDatabase>
#include <QFileInfo>
#include <QDateTime>
#include <vector>
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
    avcodec_free_context(&audioCodecCtx);
    avcodec_free_context(&vedioCodecCtx);
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
}

bool TideMediaHandle::mediaFileInit()
{
    this->reset();
    QFile* audioFile = this;
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
    return true;
}

bool TideMediaHandle::loadAudio()
{
    if (formatContext == nullptr) {
        qDebug() << "Format context is null, cannot load audio.";
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
    audioCodecCtx = avcodec_alloc_context3(codec);
    if (!audioCodecCtx)
    {
        qDebug() << "Could not allocate codec context.";
        return false;
    }
    avcodec_parameters_to_context(audioCodecCtx, st->codecpar);
    avcodec_open2(audioCodecCtx, codec, nullptr);
    qDebug() << "audio duration:" << formatContext->duration;
}

bool TideMediaHandle::loadVideo()
{
    if (formatContext == nullptr) {
        qDebug() << "Format context is null, cannot load vedio.";
        return false;
    }
    vedioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    AVStream* st = formatContext->streams[vedioIndex];
    //找到解码器
    const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec)
    {
        qDebug() << "Could not find codec.";
        return false;
    }
    vedioCodecCtx = avcodec_alloc_context3(codec);
    if (!vedioCodecCtx)
    {
        qDebug() << "Could not allocate codec context.";
        return false;
    }
    avcodec_parameters_to_context(vedioCodecCtx, st->codecpar);
    avcodec_open2(vedioCodecCtx, codec, nullptr);
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
        mediaFileInit();
        loadAudio();
    } else if (type.name().startsWith("image/"))  {
        mediaType = TMH_IMAGE;
    } else if (type.name().startsWith("video/"))  {
        mediaType = TMH_VIDEO;
        mediaFileInit();
        loadAudio();
		loadVideo();
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
    // 校验文件类型
    if (mediaType != TMH_AUDIO && mediaType != TMH_VIDEO) {
        qDebug() << "decodeAudioToQBuffer called on non-audio/video media type.";
		return nullptr;
    }
    AVChannelLayout out_channel_layout;
    AVChannelLayout in_channel_layout;
    av_channel_layout_default(&out_channel_layout, 2);

    av_channel_layout_copy(&in_channel_layout, &audioCodecCtx->ch_layout);
    // 初始化SwrContext
    SwrContext* swr_ctx = nullptr;
    int ret = swr_alloc_set_opts2(
        &swr_ctx,
        &out_channel_layout,          // 输出通道布局
        AV_SAMPLE_FMT_S16,            // 输出采样格式
        audioCodecCtx->sample_rate,       // 输出采样率
        &audioCodecCtx->ch_layout,        // 输入通道布局
        audioCodecCtx->sample_fmt,        // 输入采样格式
        audioCodecCtx->sample_rate,       // 输入采样率
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
    avcodec_flush_buffers(audioCodecCtx);
    QBuffer* pcmBuffer = new QBuffer();
    pcmBuffer->open(QIODevice::WriteOnly);
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    uint8_t* buf = (uint8_t*)av_malloc(192000); // 足够大

    int64_t decoded_duration = 0;
    bool decoding = true;

    int errorTry = 0;

    while (true) {
		int ret = av_read_frame(formatContext, pkt);
        if (ret == AVERROR_EOF)
            break;
        if (!decoding || (ret < 0)) {
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
            if (avcodec_send_packet(audioCodecCtx, pkt) == 0) {
                while (avcodec_receive_frame(audioCodecCtx, frame) == 0) {
                    // 计算当前帧的时长
                    int64_t frame_duration = int64_t(frame->nb_samples) * 1000000 / frame->sample_rate;

                    // 转换音频为S16 PCM格式
                   int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
					int converted = swr_convert(swr_ctx, &buf, out_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
                    int bufferSize = av_samples_get_buffer_size(nullptr, audioCodecCtx->ch_layout.nb_channels, converted, AV_SAMPLE_FMT_S16, 1);
                    if (converted == 0 || bufferSize == 0) continue;
					// qDebug() << "bufferSize:" << bufferSize;
                    pcmBuffer->write(reinterpret_cast<const char*>(buf), bufferSize);
                    //av_freep(&out_data[0]);
                    decoded_duration += frame_duration;
                    if (decoded_duration >= preDecodingSec/*(preDecodingSec / 1000.0)*/) {
                        decoding = false;
                        break;
                    }
                }
                
            }
        }
    }

    pcmBuffer->close();
    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr_ctx);
    av_channel_layout_uninit(&in_channel_layout);
    av_channel_layout_uninit(&out_channel_layout);
	av_free(buf);
    return pcmBuffer;
}

VedioData TideMediaHandle::decodeVedioToQBuffer(uint64_t preDecodingSec)
{
    // 校验文件类型
    if (mediaType != TMH_VIDEO) {
        qDebug() << "decodeAudioToQBuffer called on non-audio/video media type.";
        return VedioData();
    }

    // 校验文件类型
    if (mediaType != TMH_AUDIO && mediaType != TMH_VIDEO) {
        qDebug() << "decodeAudioToQBuffer called on non-audio/video media type.";
        return VedioData();
    }
    AVChannelLayout out_channel_layout;
    AVChannelLayout in_channel_layout;
    av_channel_layout_default(&out_channel_layout, 2);

    av_channel_layout_copy(&in_channel_layout, &audioCodecCtx->ch_layout);
    // 初始化SwrContext
    SwrContext* swr_ctx = nullptr;
    int ret = swr_alloc_set_opts2(
        &swr_ctx,
        &out_channel_layout,          // 输出通道布局
        AV_SAMPLE_FMT_S16,            // 输出采样格式
        audioCodecCtx->sample_rate,       // 输出采样率
        &audioCodecCtx->ch_layout,        // 输入通道布局
        audioCodecCtx->sample_fmt,        // 输入采样格式
        audioCodecCtx->sample_rate,       // 输入采样率
        0,                            // 日志偏移量（通常为0）
        nullptr                       // 日志上下文（通常为nullptr）
    );
    if (ret < 0 || !swr_ctx) {
        qDebug() << "Failed to allocate SwrContext\n";
        av_channel_layout_uninit(&in_channel_layout);
        return VedioData();
    }

    if ((ret = swr_init(swr_ctx)) < 0) {
        qDebug() << "Failed to initialize SwrContext\n";
        swr_free(&swr_ctx);
        av_channel_layout_uninit(&in_channel_layout);
        return VedioData();
    }
    avcodec_flush_buffers(audioCodecCtx);
    AVPacket* pkt = av_packet_alloc();
    QBuffer* pcmBuffer = new QBuffer();
    pcmBuffer->open(QIODevice::WriteOnly);
    AVFrame* frame = av_frame_alloc();
    uint8_t* buf = (uint8_t*)av_malloc(192000); // 足够大

    int64_t decoded_duration = 0;
    bool decoding = true;

    int errorTry = 0;

    AVFrame* rgb_frame = av_frame_alloc();
    int width = vedioCodecCtx->width, height = vedioCodecCtx->height;
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, width, height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(num_bytes);
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer, AV_PIX_FMT_RGB32, width, height, 1);
    SwsContext* sws_ctx = sws_getContext(
        width, height, vedioCodecCtx->pix_fmt,
        width, height, AV_PIX_FMT_RGB32,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    VedioData vedioData;
	vedioData.pcmBuffer = pcmBuffer;
    vedioData.image = new std::deque<QPixmap>;

    while (true) {
        int ret = av_read_frame(formatContext, pkt);
        if (ret == AVERROR_EOF)
            break;
        if (!decoding || (ret < 0)) {
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
            if (avcodec_send_packet(audioCodecCtx, pkt) == 0) {
                while (avcodec_receive_frame(audioCodecCtx, frame) == 0) {
                    // 计算当前帧的时长
                    int64_t frame_duration = int64_t(frame->nb_samples) * 1000000 / frame->sample_rate;

                    // 转换音频为S16 PCM格式
                    int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                    int converted = swr_convert(swr_ctx, &buf, out_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
                    int bufferSize = av_samples_get_buffer_size(nullptr, audioCodecCtx->ch_layout.nb_channels, converted, AV_SAMPLE_FMT_S16, 1);
                    if (converted == 0 || bufferSize == 0) continue;
                    // qDebug() << "bufferSize:" << bufferSize;
                    pcmBuffer->write(reinterpret_cast<const char*>(buf), bufferSize);
                    //av_freep(&out_data[0]);
                    decoded_duration += frame_duration;
                    if (decoded_duration >= preDecodingSec/*(preDecodingSec / 1000.0)*/) {
                        decoding = false;
                        break;
                    }
                }

            }
        }
        else if (pkt->stream_index == vedioIndex) {
            if (avcodec_send_packet(vedioCodecCtx, pkt) == 0) {
                while (avcodec_receive_frame(vedioCodecCtx, frame) == 0) {
                    // 转换视频帧到RGB格式
                    sws_scale(sws_ctx, frame->data, frame->linesize, 0, height, rgb_frame->data, rgb_frame->linesize);
                    // 这里可以将rgb_frame保存到VedioData中
                    // 例如，使用QImage或其他方式处理rgb_frame
                    QImage image(rgb_frame->data[0], width, height, rgb_frame->linesize[0], QImage::Format_RGB32);
                    QImage copy = image.copy();
					vedioData.image->push_back(QPixmap::fromImage(copy));
                }

            }
		}
    }
    return VedioData();
}

QAudioFormat TideMediaHandle::getAudioInfo()
{
    QAudioFormat format;
    if (mediaType != TMH_AUDIO && mediaType != TMH_VIDEO) {
        qDebug() << "getAudioInfo called on non-audio/video media type.";
        return format;
    }
    format.setSampleRate(audioCodecCtx->sample_rate);
    format.setChannelCount(audioCodecCtx->ch_layout.nb_channels);
    format.setSampleFormat(QAudioFormat::Int16);
    return format;
}

bool TideMediaHandle::setPlayTimestamp(uint64_t timestamp) {
    AVRational tb = formatContext->streams[audioIndex]->time_base;
    int64_t ts = av_rescale_q(timestamp, AV_TIME_BASE_Q, tb);
    if (av_seek_frame(formatContext, audioIndex, ts, AVSEEK_FLAG_ANY) < 0) {
        qDebug() << stderr << "Failed to seek.";
        return false;
    }
    avcodec_flush_buffers(audioCodecCtx);
    return true;
}

int64_t TideMediaHandle::getDuration()
{
    if (mediaType == TMH_AUDIO || mediaType == TMH_VIDEO) return formatContext ? formatContext->duration : 0;
	qDebug() << "getDuration called on non-audio/video media type.";
    return 0;
}
