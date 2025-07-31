#pragma once

#include <QIODevice>
#include <QMutex>
#include <QByteArray>
#include <QDebug>
#include <QThread>

class TideIODevice : public QIODevice {
    Q_OBJECT
public:
    explicit TideIODevice(qint64 capacityBytes, QObject* parent = nullptr);
    ~TideIODevice();
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 size) override;
    qint64 bytesAvailable() const override;
    qint64 usedBytes() const;
    qint64 availableSpace() const;
    qint64 capacitySize() const;
    void printStatus() const;
private:
    char* buffer;
    qint64 head = 0;
    qint64 tail = 0;

    qint64 capacity;
    
    // mutable QMutex mutex;
    
};
