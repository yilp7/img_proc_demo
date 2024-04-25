#include "videosurface.h"

VideoSurface::VideoSurface(QObject *parent) : QAbstractVideoSurface{parent}, frame_count(0){}

VideoSurface::~VideoSurface() {}

QList<QVideoFrame::PixelFormat> VideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> listPixelFormats;

    listPixelFormats << QVideoFrame::Format_ARGB32
                     << QVideoFrame::Format_ARGB32_Premultiplied
                     << QVideoFrame::Format_RGB32
                     << QVideoFrame::Format_RGB24
                     << QVideoFrame::Format_RGB565
                     << QVideoFrame::Format_RGB555
                     << QVideoFrame::Format_ARGB8565_Premultiplied
                     << QVideoFrame::Format_BGRA32
                     << QVideoFrame::Format_BGRA32_Premultiplied
                     << QVideoFrame::Format_BGR32
                     << QVideoFrame::Format_BGR24
                     << QVideoFrame::Format_BGR565
                     << QVideoFrame::Format_BGR555
                     << QVideoFrame::Format_BGRA5658_Premultiplied
                     << QVideoFrame::Format_Jpeg
                     << QVideoFrame::Format_CameraRaw
                     << QVideoFrame::Format_AdobeDng;

    return listPixelFormats;
}

bool VideoSurface::present(const QVideoFrame &frame)
{
    if (this->surfaceFormat().pixelFormat() != frame.pixelFormat() || this->surfaceFormat().frameSize() != frame.size())
    {
        setError(IncorrectFormatError);
        stop();
        qDebug() << "error format";
        return false;
    }
    QVideoFrame cloneFrame(frame);
    emit frameAvailable(cloneFrame);

    return true;
}
