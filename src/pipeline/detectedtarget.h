#ifndef DETECTEDTARGET_H
#define DETECTEDTARGET_H

#include <QMetaType>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>

// One tracked target as it leaves the pipeline.
//
// Two geometries are carried because they answer different questions.
// displayBox is in widget coordinates after crop / resize / letterbox and is
// what click-to-follow and auto-follow use. sourceBox is in full source-frame
// coordinates, which is the only frame where the lens FOV is meaningful, so
// range and bearing must be derived from it and never from displayBox.
struct DetectedTarget {
    int trackId = -1;
    int classId = -1;
    // Resolved on the pipeline thread, which owns the detector and the model
    // selection. Consumers must not reach back into DisplaySlot for these.
    QString className;
    int modelIdx = 0;
    float confidence = 0.0f;
    bool confirmed = false;
    // Set for exactly one update: the track was dropped this frame.
    bool lost = false;
    QRect displayBox;              // empty when cropped out of the display
    QRect sourceBox;
    QSize sourceSize;
};

Q_DECLARE_METATYPE(DetectedTarget)
Q_DECLARE_METATYPE(QVector<DetectedTarget>)

#endif // DETECTEDTARGET_H
