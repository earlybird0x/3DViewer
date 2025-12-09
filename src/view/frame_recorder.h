#ifndef S21_VIEW_FRAME_RECORDER_H
#define S21_VIEW_FRAME_RECORDER_H

#include <QImage>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>

namespace s21 {

class GLWidget;

class FrameRecorder : public QObject {
  Q_OBJECT
 public:
  explicit FrameRecorder(s21::GLWidget *widget, QObject *parent = nullptr);

  void Start(int fps, int duration_sec);
  void Stop();

  const QVector<QImage> &frames() const { return frames_; }
  void Clear() { frames_.clear(); }

  void SetAutoRotate(bool enabled) { autoRotate_ = enabled; }

 signals:
  void Started();
  void Progress(int captured, int total);
  void Finished();
  void Error(const QString &msg);

 private slots:
  void OnTick();

 private:
  s21::GLWidget *widget_ = nullptr;
  QTimer timer_;
  QVector<QImage> frames_;
  int fps_ = 10;
  int max_frames_ = 0;
  int captured_ = 0;

  bool autoRotate_ = true;
  double angleStepDeg_ = 0.0;
};

}  // namespace s21

#endif  // S21_VIEW_FRAME_RECORDER_H
