#ifndef QT6_TEXTCODEC_COMPAT_H
#define QT6_TEXTCODEC_COMPAT_H

#include <QString>
#include <QByteArray>
#include <QStringConverter>
#include <QTextStream>

// Simple Qt6 compatibility wrapper for QTextCodec functionality
class QTextCodec
{
public:
    static QTextCodec* codecForName(const char* name) {
        return new QTextCodec(name);
    }
    
    static QTextCodec* codecForLocale() {
        return new QTextCodec("UTF-8");
    }
    
    static QTextCodec* codecForHtml(const QByteArray& data, QTextCodec* defaultCodec = nullptr) {
        Q_UNUSED(data);
        return defaultCodec ? defaultCodec : codecForName("UTF-8");
    }
    
    static void setCodecForLocale(QTextCodec* codec) {
        // Qt6: This is a no-op since locale codec is always UTF-8
        Q_UNUSED(codec);
    }
    
    QString toUnicode(const QByteArray& data) const {
        if (m_encoding == "UTF-8" || m_encoding == "utf8") {
            return QString::fromUtf8(data);
        } else if (m_encoding == "ISO-8859-8" || m_encoding == "ISO-88598") {
            return QString::fromLatin1(data);
        } else {
            // Default to UTF-8
            return QString::fromUtf8(data);
        }
    }
    
    QByteArray fromUnicode(const QString& str) const {
        if (m_encoding == "UTF-8" || m_encoding == "utf8") {
            return str.toUtf8();
        } else if (m_encoding == "ISO-8859-8" || m_encoding == "ISO-88598") {
            return str.toLatin1();
        } else {
            // Default to UTF-8
            return str.toUtf8();
        }
    }
    
    QStringConverter::Encoding getEncoding() const {
        if (m_encoding == "UTF-8" || m_encoding == "utf8") {
            return QStringConverter::Utf8;
        } else if (m_encoding == "ISO-8859-8" || m_encoding == "ISO-88598") {
            return QStringConverter::Latin1;
        } else {
            return QStringConverter::Utf8;
        }
    }
    
private:
    explicit QTextCodec(const char* name) : m_encoding(name) {}
    QString m_encoding;
};

// Qt6 compatibility for QTextStream::setCodec
namespace Qt6Compat {
    inline void setCodec(QTextStream& stream, QTextCodec* codec) {
        if (codec) {
            stream.setEncoding(codec->getEncoding());
        }
    }
}

// Macro to replace QTextStream::setCodec calls
#define SET_TEXTSTREAM_CODEC(stream, codec) Qt6Compat::setCodec(stream, codec)

#endif // QT6_TEXTCODEC_COMPAT_H