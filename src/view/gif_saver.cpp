#include "view/gif_saver.h"

#include <QByteArray>
#include <QFile>
#include <QImage>
#include <QString>
#include <QVector>

#include "3rdpart/gif.h"

namespace s21
{

  static QByteArray makeTightRGBA(const QImage &src, int W, int H)
  {
    QImage img = src;
    if (img.width() != W || img.height() != H)
    {
      img = img.scaled(W, H, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    if (img.format() != QImage::Format_RGBA8888)
    {
      img = img.convertToFormat(QImage::Format_RGBA8888);
    }
#else
    if (img.format() != QImage::Format_ARGB32)
    {
      img = img.convertToFormat(QImage::Format_ARGB32);
    }
#endif

    QByteArray tight;
    tight.resize(W * H * 4);

    uint8_t *dst = reinterpret_cast<uint8_t *>(tight.data());
    const int row_bytes = W * 4;

    for (int y = 0; y < H; ++y)
    {
      const uint8_t *src_row = img.constScanLine(y);
      memcpy(dst + y * row_bytes, src_row, row_bytes);
    }

    return tight;
  }

  bool SaveGif(const QVector<QImage> &frames,
               const QString &path,
               int delayCs,
               int loop,
               int targetW,
               int targetH)
  {
    if (frames.isEmpty())
    {
      return false;
    }

    const int W = targetW;
    const int H = targetH;

    const QByteArray fname = QFile::encodeName(path);

    GifWriter wr{};
    if (!GifBegin(&wr, fname.constData(), W, H, delayCs, loop))
    {
      return false;
    }

    bool ok = true;

    for (const QImage &frame : frames)
    {
      QByteArray tight = makeTightRGBA(frame, W, H);
      const uint8_t *rgba =
          reinterpret_cast<const uint8_t *>(tight.constData());

      if (!GifWriteFrame(&wr, rgba, W, H, delayCs))
      {
        ok = false;
        break;
      }
    }

    if (!GifEnd(&wr))
    {
      ok = false;
    }

    return ok;
  }

} // namespace s21
