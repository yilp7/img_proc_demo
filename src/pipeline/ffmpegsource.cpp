#include "ffmpegsource.h"

#include <cmath>
#include <ctime>
#include <memory>

#include <QDebug>
#include <QElapsedTimer>

#include "util/util.h"  // pulls in FFmpeg + opencv via extern "C" block

namespace {

// Interrupt callback context: a pointer to FfmpegSource's stop flag. Returns
// 1 to abort the blocking FFmpeg call; FFmpeg polls this between IO chunks.
struct InterruptCtx {
    std::atomic<bool>* should_stop;
    time_t             last_activity;
};

int ffmpeg_interrupt(void* opaque) {
    InterruptCtx* ctx = static_cast<InterruptCtx*>(opaque);
    if (!ctx) return 0;
    if (ctx->should_stop && ctx->should_stop->load()) return 1;
    // Hard 30 s ceiling on blocked IO — matches the legacy guard. Networks
    // sometimes wedge a connect() for minutes without ever erroring; the
    // stop flag alone doesn't bound that, since the user hasn't asked to
    // stop.
    if (ctx->last_activity > 0 && time(NULL) - ctx->last_activity > 30) return 1;
    return 0;
}

void init_filter_graph(AVFormatContext* format_context, int video_stream_idx,
                       AVFilterGraph* filter_graph, AVCodecContext* codec_context,
                       AVFilterContext** buffersink_ctx, AVFilterContext** buffersrc_ctx) {
    char args[512];
    const AVFilter* buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut*  outputs    = avfilter_inout_alloc();
    AVFilterInOut*  inputs     = avfilter_inout_alloc();
    // Guards cover every early-return path below. avfilter_inout_free is a
    // no-op on NULL, so safe if avfilter_graph_parse_ptr already consumed
    // the lists.
    std::shared_ptr<AVFilterInOut*> deleter_outputs(&outputs, avfilter_inout_free);
    std::shared_ptr<AVFilterInOut*> deleter_inputs(&inputs, avfilter_inout_free);

    AVRational time_base = format_context->streams[video_stream_idx]->time_base;
    static enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

    if (!outputs || !inputs || !filter_graph) return;

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             codec_context->width, codec_context->height, codec_context->pix_fmt,
             time_base.num, time_base.den,
             codec_context->sample_aspect_ratio.num, codec_context->sample_aspect_ratio.den);

    if (avfilter_graph_create_filter(buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph) < 0) return;
    if (avfilter_graph_create_filter(buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph) < 0) return;
    if (av_opt_set_int_list(*buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0) return;

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = *buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = *buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if (avfilter_graph_parse_ptr(filter_graph, "format=gray", &inputs, &outputs, NULL) < 0) return;
    if (avfilter_graph_config(filter_graph, NULL) < 0) return;
}

} // namespace

FfmpegSource::FfmpegSource(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<FfmpegSource::VideoInfo>("FfmpegSource::VideoInfo");
    qRegisterMetaType<StreamId>("StreamId");
    qRegisterMetaType<cv::Mat>("cv::Mat");
}

FfmpegSource::~FfmpegSource() {
    stop();
}

void FfmpegSource::start(StreamId target, QString url, bool format_gray) {
    stop();
    const int session = m_session.fetch_add(1) + 1;
    m_should_stop = false;
    m_running     = true;
    m_target      = target;
    qDebug() << "[FfmpegSource] start: target=" << static_cast<int>(target)
             << "session=" << session << "url=" << url;
    m_thread = std::thread(&FfmpegSource::run_thread, this, target, session, url, format_gray);
}

void FfmpegSource::stop(int timeout_ms) {
    const bool was_joinable = m_thread.joinable();
    if (was_joinable) qDebug() << "[FfmpegSource] stop: joining previous worker";
    m_should_stop = true;
    if (m_thread.joinable()) {
        // The worker checks m_should_stop after each av_read_frame and at
        // every decoded frame, and the interrupt callback aborts blocking
        // IO. Joining without a hard timeout is intentional — std::thread
        // has no portable timed join. The interrupt callback's 30 s ceiling
        // bounds the worst case if FFmpeg ignores the stop flag.
        (void)timeout_ms;
        m_thread.join();
    }
    if (was_joinable) qDebug() << "[FfmpegSource] stop: previous worker joined";
    m_running = false;
}

void FfmpegSource::run_thread(StreamId target, int session, QString url, bool format_gray) {
    bool is_rtsp_stream = url.contains("rtsp://");
    bool is_http_stream = url.contains("http://") || url.contains("https://");
    bool is_local_file  = !is_rtsp_stream && !is_http_stream;

    AVFormatContext* format_context = avformat_alloc_context();
    std::shared_ptr<AVFormatContext*> closer_format_context(&format_context, avformat_close_input);
    AVCodecParameters* codec_param = NULL;
    const AVCodec* codec = NULL;
    AVCodecContext* codec_context = avcodec_alloc_context3(NULL);
    std::shared_ptr<AVCodecContext*> closer_codec_context(&codec_context, avcodec_free_context);
    AVFrame* frame = av_frame_alloc();
    std::shared_ptr<AVFrame*> deleter_frame(&frame, av_frame_free);
    AVFrame* frame_sw = av_frame_alloc();
    std::shared_ptr<AVFrame*> deleter_frame_sw(&frame_sw, av_frame_free);
    AVFrame* frame_result = av_frame_alloc();
    std::shared_ptr<AVFrame*> deleter_frame_result(&frame_result, av_frame_free);
    AVFrame* frame_filter = av_frame_alloc();
    std::shared_ptr<AVFrame*> deleter_frame_filter(&frame_filter, av_frame_free);
    uint8_t* buffer = NULL;
    // Address-of pattern: deleter reads the current value at scope exit. The
    // value form (buffer, av_free) would capture the NULL we have now and
    // leak the later av_malloc.
    std::shared_ptr<uint8_t*> deleter_buffer(&buffer, [](uint8_t** p) { av_free(*p); });
    SwsContext* sws_context = NULL;
    std::shared_ptr<SwsContext*> deleter_sws_context(&sws_context, [](SwsContext** p) { sws_freeContext(*p); });
    AVFilterContext* buffersink_ctx = nullptr;
    AVFilterContext* buffersrc_ctx = nullptr;
    AVFilterGraph* filter_graph = avfilter_graph_alloc();
    std::shared_ptr<AVFilterGraph*> deleter_filter_graph(&filter_graph, avfilter_graph_free);

    InterruptCtx interrupt_ctx{ &m_should_stop, 0 };
    format_context->interrupt_callback.callback = ffmpeg_interrupt;
    format_context->interrupt_callback.opaque   = &interrupt_ctx;

    auto finish = [&](bool ok, const char* reason) {
        m_running = false;
        qDebug() << "[FfmpegSource] worker exiting: target=" << static_cast<int>(target)
                 << "session=" << session << "ok=" << ok << "reason=" << reason;
        if (ok) emit finished(target, session);
        else    emit failed(target, session, QString::fromLatin1(reason));
    };

    AVInputFormat* input_format = nullptr;
    if (url.contains("video=") || url.contains("audio="))
        input_format = (AVInputFormat*)av_find_input_format("dshow");

    AVDictionary* format_opts = nullptr;
    if (is_rtsp_stream) {
        av_dict_set(&format_opts, "rtsp_transport", "tcp",     0);
        av_dict_set(&format_opts, "buffer_size",    "262144",  0);
        av_dict_set(&format_opts, "fflags",         "nobuffer",0);
        av_dict_set(&format_opts, "max_delay",      "200000",  0);
        av_dict_set(&format_opts, "analyzeduration","500000",  0);
        av_dict_set(&format_opts, "probesize",      "100000",  0);
    }
    else if (is_http_stream) {
        av_dict_set(&format_opts, "http_persistent", "1", 0);
        av_dict_set(&format_opts, "http_multiple",   "1", 0);
        av_dict_set(&format_opts, "seg_max_retry",   "3", 0);
    }

    interrupt_ctx.last_activity = time(NULL);
    if (avformat_open_input(&format_context, url.toUtf8().constData(), input_format, &format_opts) != 0) {
        av_dict_free(&format_opts);
        finish(false, "open_failed");
        return;
    }
    av_dict_free(&format_opts);

    if (avformat_find_stream_info(format_context, NULL) < 0) { finish(false, "stream_info_failed"); return; }

    int video_stream_idx = -1;
    int max_res_w = 0;
    for (uint i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            format_context->streams[i]->codecpar->width > max_res_w) {
            video_stream_idx = i;
            max_res_w = format_context->streams[i]->codecpar->width;
        }
    }
    if (video_stream_idx == -1) { finish(false, "no_video_stream"); return; }

    codec_param = format_context->streams[video_stream_idx]->codecpar;

    // FPS extraction: prefer avg_frame_rate (computed during the probe), fall
    // back to r_frame_rate. Reject implausibly-high values (RTP's 90 kHz
    // sentinel when SDP carries no a=framerate line poisons VideoWriter).
    double fps = 0.0;
    {
        AVRational fr = format_context->streams[video_stream_idx]->avg_frame_rate;
        if (fr.num == 0 || fr.den == 0) fr = format_context->streams[video_stream_idx]->r_frame_rate;
        const double v = av_q2d(fr);
        if (v > 0.0 && v <= 1000.0) fps = std::round(v);
        else qDebug() << "[FfmpegSource] rejecting bogus fps" << v
                      << "(avg=" << format_context->streams[video_stream_idx]->avg_frame_rate.num
                      << "/"     << format_context->streams[video_stream_idx]->avg_frame_rate.den
                      << "r="    << format_context->streams[video_stream_idx]->r_frame_rate.num
                      << "/"     << format_context->streams[video_stream_idx]->r_frame_rate.den
                      << ")";
    }

    codec = avcodec_find_decoder(codec_param->codec_id);
    if (codec == NULL) { finish(false, "codec_unsupported"); return; }
    if (avcodec_parameters_to_context(codec_context, codec_param) < 0) { finish(false, "codec_param_failed"); return; }

    // HW decode for local files only — streams skip HW because the GPU→CPU
    // readback bottleneck causes burst-then-stall ("PowerPoint" effect).
    AVBufferRef* hw_device_ctx = nullptr;
    bool hw_decoding = false;
    if (is_local_file &&
        av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) == 0) {
        codec_context->hw_device_ctx = av_buffer_ref(hw_device_ctx);
        hw_decoding = true;
        qDebug() << "[FfmpegSource] using D3D11VA hardware decoding (local file)";
    } else {
        qDebug() << "[FfmpegSource] using SW decoding ("
                 << (is_local_file ? "HW unavailable" : "stream") << ")";
    }
    if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);

    codec_context->thread_count = 0;
    codec_context->thread_type  = FF_THREAD_FRAME | FF_THREAD_SLICE;
    if (is_rtsp_stream || is_http_stream) {
        codec_context->flags  |= AV_CODEC_FLAG_LOW_DELAY;
        codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
    }

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        if (hw_decoding) {
            qDebug() << "[FfmpegSource] HW codec open failed, retrying with software decoding";
            hw_decoding = false;
            avcodec_free_context(&codec_context);
            codec_context = avcodec_alloc_context3(NULL);
            if (avcodec_parameters_to_context(codec_context, codec_param) < 0) { finish(false, "codec_param_failed"); return; }
            codec_context->thread_count = 0;
            codec_context->thread_type  = FF_THREAD_FRAME | FF_THREAD_SLICE;
            if (is_rtsp_stream || is_http_stream) {
                codec_context->flags  |= AV_CODEC_FLAG_LOW_DELAY;
                codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
            }
            if (avcodec_open2(codec_context, codec, NULL) < 0) { finish(false, "decoder_open_failed"); return; }
        } else {
            finish(false, "decoder_open_failed"); return;
        }
    }

    cv::Mat cv_frame(codec_context->height, codec_context->width, CV_MAKETYPE(CV_8U, format_gray ? 1 : 3));

    AVPixelFormat pixel_fmt = format_gray ? AV_PIX_FMT_NV12 : AV_PIX_FMT_RGB24;
    int byte_num = av_image_get_buffer_size(pixel_fmt, codec_context->width, codec_context->height, 32);
    buffer = (uint8_t*)av_malloc(byte_num * sizeof(uint8_t));
    av_image_fill_arrays(frame_result->data, frame_result->linesize, buffer, pixel_fmt,
                         codec_context->width, codec_context->height, 32);

    AVPixelFormat sws_src_fmt = AV_PIX_FMT_NONE;

    // Notify GUI: open succeeded. UserPanel populates StreamSlot::meta + sets
    // up the display widget before frames flow.
    VideoInfo info;
    info.width    = codec_context->width;
    info.height   = codec_context->height;
    info.fps      = fps;
    info.is_color = !format_gray;
    emit opened(target, session, info);

    AVPacket packet;
    if (format_gray) init_filter_graph(format_context, video_stream_idx, filter_graph,
                                       codec_context, &buffersink_ctx, &buffersrc_ctx);

    AVRational time_base   = format_context->streams[video_stream_idx]->time_base;
    AVRational time_base_q = { 1, AV_TIME_BASE };
    long long  last_pts    = AV_NOPTS_VALUE;
    long long  delay;
    QElapsedTimer elapsed_timer;
    int result;

    while (!m_should_stop.load()) {
        interrupt_ctx.last_activity = time(NULL);
        if ((result = av_read_frame(format_context, &packet)) < 0) {
            if (result == AVERROR_EOF) break;
            continue;
        }

        std::shared_ptr<AVPacket> deleter_packet(&packet, av_packet_unref);

        if (packet.stream_index == video_stream_idx) {
            avcodec_send_packet(codec_context, &packet);
            // Drain every frame the decoder produced for this packet. B-frames
            // + threaded decode buffer multiple outputs per send; a single
            // receive leaves frames queued and accumulates latency.
            bool stream_done = false;
            while (avcodec_receive_frame(codec_context, frame) == 0) {
                AVFrame* sw_frame = frame;
                if (hw_decoding && frame->format != AV_PIX_FMT_NONE && frame->hw_frames_ctx) {
                    av_frame_unref(frame_sw);
                    if (av_hwframe_transfer_data(frame_sw, frame, 0) < 0) {
                        av_frame_unref(frame);
                        continue;
                    }
                    sw_frame = frame_sw;
                }

                if (format_gray) {
                    if (av_buffersrc_add_frame_flags(buffersrc_ctx, sw_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) { stream_done = true; break; }
                    av_frame_unref(frame_filter);
                    int ret = av_buffersink_get_frame(buffersink_ctx, frame_filter);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { stream_done = true; break; }
                }
                else {
                    // Use frame->pts (decoder output) not sw_frame->pts:
                    // av_hwframe_transfer_data does not copy metadata, so
                    // sw_frame->pts is unset (0) under HW decoding, making
                    // the throttle a no-op.
                    if (frame->pts != AV_NOPTS_VALUE) {
                        if (last_pts != AV_NOPTS_VALUE) {
                            delay = av_rescale_q(frame->pts - last_pts, time_base, time_base_q) - elapsed_timer.nsecsElapsed() / 1000;
                            if (delay > 0 && delay < 1000000) av_usleep(delay);
                            elapsed_timer.restart();
                        }
                        last_pts = frame->pts;
                    }
                    if (sws_src_fmt != (AVPixelFormat)sw_frame->format) {
                        if (sws_context) sws_freeContext(sws_context);
                        sws_src_fmt = (AVPixelFormat)sw_frame->format;
                        sws_context = sws_getContext(codec_context->width, codec_context->height, sws_src_fmt,
                                                     codec_context->width, codec_context->height, pixel_fmt,
                                                     SWS_BILINEAR, NULL, NULL, NULL);
                    }
                    sws_scale(sws_context, sw_frame->data, sw_frame->linesize, 0, codec_context->height,
                              frame_result->data, frame_result->linesize);
                }

                cv_frame.data = (format_gray ? frame_filter : frame_result)->data[0];
                // Emit a clone — the cv::Mat data lives on stack buffers
                // owned by av_image_fill_arrays; the consumer holds the
                // frame past this iteration and the next pass overwrites
                // the buffer.
                emit frame_decoded(target, session, cv_frame.clone());
                av_frame_unref(frame);

                if (m_should_stop.load()) { stream_done = true; break; }
            }
            if (stream_done) break;
        }
    }

    finish(true, "");
}
