#include "TideIODevice.h"
#include <QDebug>
#include <cstring>
#include <thread>
TideIODevice::TideIODevice(qint64 capacityBytes, QObject* parent)
    : QIODevice(parent), capacity(capacityBytes) {
    buffer = new char[capacity];
    open(QIODevice::ReadWrite);
}
TideIODevice::~TideIODevice() {
    delete buffer;
}

qint64 TideIODevice::readData(char* data, qint64 maxSize) {
    qint64 readBytes = qMin(usedBytes(), maxSize);
    if (tail + readBytes < capacity) {
        memcpy(data, buffer + tail, readBytes);
        tail = tail + readBytes;
    }
    else {
        memcpy(data, buffer + tail, capacity - tail);
        memcpy(data + capacity - tail, buffer, readBytes - (capacity - tail));
        tail = readBytes - (capacity - tail);
    }
    return readBytes;
}

qint64 TideIODevice::writeData(const char* data, qint64 size) {
    qint64 writeBytes = qMin(availableSpace(), size);
    if (head + writeBytes < capacity) {
        memcpy(buffer + head, data, writeBytes);
        head = head + writeBytes;
    }
    else {
        memcpy(buffer + head, data, capacity - head);
        memcpy(buffer, data + capacity - head, writeBytes - (capacity - head));
        head = writeBytes - (capacity - head);
    }
    return writeBytes;
}
qint64 TideIODevice::bytesAvailable() const {
    return head - tail >= 0 ? head - tail : capacity + head - tail;
}
qint64 TideIODevice::usedBytes() const {
    return head - tail >= 0 ? head - tail : capacity + head - tail;
}
qint64 TideIODevice::availableSpace() const {
    return capacity - (head - tail >= 0 ? head - tail : capacity + head - tail);
}

qint64 TideIODevice::capacitySize() const
{
    return capacity;
}

void TideIODevice::printStatus() const
{
    qDebug() << "capacity: " << capacity;
    qDebug() << "head: " << head;
    qDebug() << "tail: " << tail;
    qDebug() << "usedBytes(): " << usedBytes();
    qDebug() << "availableSpace(): " << availableSpace();
}
