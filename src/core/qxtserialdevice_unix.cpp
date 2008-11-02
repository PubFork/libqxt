#include "qxtserialdevice.h"
#include "qxtserialdevice_p.h"
#include <QVarLengthArray>
#include <QSocketNotifier>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#if defined(Q_OS_BSD4) || defined(Q_OS_DARWIN) || defined(Q_OS_BSDI) || defined(Q_OS_NETBSD) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
// definition of FIONREAD
#include <sys/filio.h>
#endif

/*class QxtSerialDevicePrivate;
class QXT_CORE_EXPORT QxtSerialDevice : public QIODevice {
Q_OBJECT
public:
    enum BaudRate {
        Baud50, Baud75, Baud110, Baud134, Baud150, Baud200, Baud300, Baud600, Baud1200,
        Baud2400, Baud4800, Baud9600, Baud19200, Baud38400, Baud57600, Baud115200, Baud230400
    };

    QxtSerialDevice(const QString& device, QObject* parent = 0);
    QxtSerialDevice(QObject* parent = 0);

    virtual bool atEnd() const;
    virtual qint64 bytesAvailable() const;
    virtual bool canReadLine() const;
    virtual void close();
    virtual bool isSequential() const;
    bool open(const QString& device, OpenMode mode);
    virtual bool open(OpenMode mode);
    int handle() const;

    void setDeviceName(const QString& device);
    QString deviceName() const;

    void setBaud(BaudRate rate);
    BaudRate baud() const;

protected:
    virtual qint64 readData(char* data, qint64 maxSize);
    virtual qint64 writeData(const char* data, qint64 maxSize);

private:
    QXT_DECLARE_PRIVATE(QxtSerialDevice);
}; */

qint64 QxtSerialDevicePrivate::deviceBuffer() const {
    int bytes;
    ioctl(fd, FIONREAD, &bytes);
    return bytes;
}

qint64 QxtSerialDevice::bytesAvailable() const {
    return QIODevice::bytesAvailable() + qxt_d().buffer.length() + qxt_d().deviceBuffer();
}

void QxtSerialDevice::close() {
    if(isOpen()) {
        tcsetattr(qxt_d().fd, TCSANOW, &qxt_d().reset);
        ::close(qxt_d().fd);
    }
}

bool QxtSerialDevice::open(OpenMode mode) {
    if(qxt_d().notifier) {
        delete qxt_d().notifier;
        qxt_d().notifier = 0;
    }
    int m;
    if(mode & QIODevice::ReadOnly && mode & QIODevice::WriteOnly) { // r/w
        m = O_RDWR;
    } else if(mode & QIODevice::ReadOnly) {
        m = O_WRONLY;
    } else {
        m = O_RDONLY;
    }
    qxt_d().fd = ::open(qxt_d().device.toLocal8Bit().constData(), m | O_NOCTTY);
    if(qxt_d().fd < 0) {
        // TODO: set error message
        return false;
    }
    fcntl(qxt_d().fd, F_SETFL, O_NONBLOCK);
    tcgetattr(qxt_d().fd, &qxt_d().reset);
    memset(&qxt_d().settings, 0, sizeof(termios)); 
    qxt_d().settings.c_cflag = qxt_d().baud;

    qxt_d().notifier = new QSocketNotifier(qxt_d().fd, QSocketNotifier::Read, this);
    if(mode & QIODevice::Unbuffered) {
        QObject::connect(qxt_d().notifier, SIGNAL(activated(int)), this, SIGNAL(readyRead()));
    } else {
        QObject::connect(qxt_d().notifier, SIGNAL(activated(int)), &qxt_d(), SLOT(fillBuffer()));
    }
    setOpenMode(mode);
    return true;
}

int QxtSerialDevicePrivate::constFillBuffer() const {
    notifier->setEnabled(false);
    int sz = deviceBuffer();
    QVarLengthArray<char, 1024> buf(sz);
    int rv = read(fd, buf.data(), sz);
    if(rv < 0) {
        notifier->setEnabled(true);
        if(errno == EAGAIN) return 0; // harmless
        return errno;
    }
    buffer += QByteArray(buf.constData(), rv);
    notifier->setEnabled(true);
    return 0;
}

int QxtSerialDevicePrivate::fillBuffer() {
    int sz = buffer.length();
    int rv = constFillBuffer();
    if(rv)
        qxt_p().setErrorString(strerror(rv));
    if(buffer.length() != sz)
        QMetaObject::invokeMethod(&qxt_p(), "readyRead", Qt::QueuedConnection);
    return rv;
}

bool QxtSerialDevice::setBaud(BaudRate rate) {
    switch(rate) {
    case Baud110: qxt_d().baud = B110; break;
    case Baud300: qxt_d().baud = B300; break;
    case Baud600: qxt_d().baud = B600; break;
    case Baud1200: qxt_d().baud = B1200; break;
    case Baud2400: qxt_d().baud = B2400; break;
    case Baud4800: qxt_d().baud = B4800; break;
    case Baud9600: qxt_d().baud = B9600; break;
    case Baud19200: qxt_d().baud = B19200; break;
    case Baud38400: qxt_d().baud = B38400; break;
    case Baud57600: qxt_d().baud = B57600; break;
    case Baud115200: qxt_d().baud = B115200; break;
    };
    if(isOpen()) {
        qxt_d().settings.c_cflag = qxt_d().baud | qxt_d().flow | qxt_d().format | CLOCAL | CREAD; 
        tcflush(qxt_d().fd, TCIFLUSH);
        if(tcsetattr(qxt_d().fd, TCSANOW, &qxt_d().settings)) {
            setErrorString(strerror(errno));
            return false;
        }
    }
    return true;
}

QxtSerialDevice::BaudRate QxtSerialDevice::baud() const {
    switch(qxt_d().baud) {
    case B110: return Baud110;
    case B300: return Baud300;
    case B600: return Baud600;
    case B1200: return Baud1200;
    case B2400: return Baud2400;
    case B4800: return Baud4800;
    case B19200: return Baud19200;
    case B38400: return Baud38400;
    case B57600: return Baud57600;
    case B115200: return Baud115200;
    default: return Baud9600;
    };
}

qint64 QxtSerialDevice::readData(char* data, qint64 maxSize) {
    int b = bytesAvailable();
    if(maxSize > b) maxSize = b;
    if(!(openMode() & QIODevice::Unbuffered)) {
        if(qxt_d().fillBuffer()) return 0;
    }
    int bufsize = qxt_d().buffer.length();
    int rv = 0;
    if(maxSize <= bufsize) {
        memcpy(data, qxt_d().buffer.constData(), maxSize);
        qxt_d().buffer = qxt_d().buffer.mid(maxSize);
        return maxSize;
    } else {
        memcpy(data, qxt_d().buffer.constData(), bufsize);
        qxt_d().buffer.clear();
        rv = bufsize;
        data += bufsize;
        maxSize -= bufsize;
    }
    int readVal = ::read(qxt_d().fd, data, maxSize);
    if(readVal < 0) {
        setErrorString(strerror(errno));
    } else {
        rv += readVal;
    }
    return rv;
}

qint64 QxtSerialDevice::writeData(const char* data, qint64 maxSize) {
    int rv = ::write(qxt_d().fd, data, maxSize);
    if(rv < 0) {
        setErrorString(strerror(errno));
    }
    return rv;
}

bool QxtSerialDevicePrivate::setPortSettings(QxtSerialDevice::PortSettings setup) {
    int bits = setup & QxtSerialDevice::BitMask;
    switch(bits) {
    case QxtSerialDevice::Bit8: format = CS8; break;
    case QxtSerialDevice::Bit7: format = CS7; break;
    case QxtSerialDevice::Bit6: format = CS6; break;
    case QxtSerialDevice::Bit5: format = CS5; break;
    };
    if(setup & QxtSerialDevice::Stop2)
        format |= CSTOPB;
    int parity = setup & QxtSerialDevice::ParityMask;
    if(parity != QxtSerialDevice::ParityNone) {
        format |= PARENB;
        if(parity == QxtSerialDevice::ParityOdd) {
            format |= PARODD;
        } else if(parity == QxtSerialDevice::ParityMark || parity == QxtSerialDevice::ParitySpace) {
#ifdef CMSPAR
            format |= CMSPAR;
            if(parity == QxtSerialDevice::ParityMark)
                format |= PARODD;
#else
            qxt_p().setErrorString("Space/Mark parity not supported");
            return false;
#endif
        }
    }
    int flowbits = setup & QxtSerialDevice::FlowMask;
    if(flowbits == QxtSerialDevice::FlowRtsCts) {
#ifdef CRTSCTS
        flow = CRTSCTS;
#else
        qxt_p().setErrorString("Hardware flow control not supported");
        return false;
#endif
    } else if(flowbits == QxtSerialDevice::FlowXonXoff) {
        flow = IXON | IXOFF;
    }
    return true;
}