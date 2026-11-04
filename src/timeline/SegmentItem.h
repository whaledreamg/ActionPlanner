#ifndef SEGMENT_ITEM_H
#define SEGMENT_ITEM_H

#include <QObject>

class SegmentItem : public QObject
{
    Q_OBJECT
public:
    explicit SegmentItem(QObject *parent = nullptr);
};

#endif
