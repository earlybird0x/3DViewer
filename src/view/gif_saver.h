#ifndef S21_VIEW_GIF_SAVER_H
#define S21_VIEW_GIF_SAVER_H

#include <QImage>
#include <QString>
#include <QVector>

namespace s21
{

    bool SaveGif(const QVector<QImage> &frames,
                 const QString &path,
                 int delayCs,
                 int loop = 0,
                 int targetW = 640,
                 int targetH = 480);

} // namespace s21

#endif // S21_VIEW_GIF_SAVER_H
