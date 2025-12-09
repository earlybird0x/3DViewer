#include "view/frame_recorder.h"

#include <QDebug>

#include "view/glwidget.h"

namespace s21 {

FrameRecorder::FrameRecorder(s21::GLWidget *widget, QObject *parent)
    : QObject(parent), widget_(widget) {
  connect(&timer_, &QTimer::timeout, this, &FrameRecorder::OnTick);
}

void FrameRecorder::Start(int fps, int duration_sec) {
  if (!widget_) {
    emit Error("Нет GLWidget");
    return;
  }
  if (fps <= 0 || duration_sec <= 0) {
    emit Error("Неверные параметры записи");
    return;
  }

  frames_.clear();
  fps_ = fps;
  max_frames_ = fps * duration_sec;
  captured_ = 0;
  angleStepDeg_ =
      (autoRotate_ && max_frames_ > 0) ? (360.0 / double(max_frames_)) : 0.0;

  const int interval_ms = static_cast<int>(1000.0 / fps_);
  timer_.start(interval_ms);
  emit Started();
}

void FrameRecorder::Stop() {
  if (timer_.isActive()) timer_.stop();
  emit Finished();
}

void FrameRecorder::OnTick() {
  if (!widget_) {
    Stop();
    return;
  }

  if (autoRotate_ && angleStepDeg_ != 0.0) {
    widget_->RotateY(static_cast<float>(angleStepDeg_));
  }

  QImage img = widget_->GrabFrame();
  img = img.convertToFormat(QImage::Format_RGBA8888);
  frames_.push_back(std::move(img));

  ++captured_;
  emit Progress(captured_, max_frames_);

  if (captured_ >= max_frames_) {
    Stop();
  }
}

}  // namespace s21
