#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H

#include "utils.h"

class VideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    VideoSurface(QObject *parent = nullptr);
    ~VideoSurface();

signals:
    void frameAvailable(QVideoFrame &frame);

private:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;
//    bool isFormatSupported(const QVideoSurfaceFormat &format) const;
//    bool start(const QVideoSurfaceFormat &format);
    bool present(const QVideoFrame &frame);

public:
    QImage::Format imageFormat;
    QPixmap        imageCaptured;
    QVideoFrame    currentFrame;

    int            frame_count;
};

#endif // VIDEOSURFACE_H
