#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPoint>
#include <QWheelEvent>
#include <cstdint>
#include <vector>

#include "model/obj_model.h"
#include "view/projection.h"
#include "view/render_settings.h"

namespace s21 {

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT
 public:
  explicit GLWidget(QWidget *parent = nullptr);

  void SetModel(const s21::Model *model);
  void SetModelAndEdges(const s21::Model *model, std::vector<uint32_t> &&edges);

  QImage GrabFrame();
  // ← ДОБАВЬ СЮДА (до public slots:)

 public slots:
  void RotateX(float angle);
  void RotateY(float angle);
  void RotateZ(float angle);
  void Scale(float factor);
  void ResetTransform();
  void Translate(float dx, float dy, float dz);
  void ToggleProjection();

 public:
  const RenderSettings &settings() const { return settings_; }
  void SetSettings(const RenderSettings &s);

 protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void wheelEvent(QWheelEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;

 private:
  bool glReady_ = false;  // ← станет true в конце initializeGL()
  const s21::Model *model_ = nullptr;
  QMatrix4x4 transform_;
  QPoint lastMousePos_;
  float rotateSensitivity_ = 0.3f;
  float translateSensitivity_ = 0.02f;

  QOpenGLVertexArrayObject vao_;
  QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer ebo_{QOpenGLBuffer::IndexBuffer};
  QOpenGLShaderProgram program_;
  QOpenGLShaderProgram program_pts_;
  int u_mvp_pts_ = -1;
  int u_color_pts_ = -1;
  int u_psize_pts_ = -1;
  int u_circle_pts_ = -1;
  int u_dash_ = -1;  // уже есть: 0=сплошные, 1=пунктир
  int u_dash_period_ = -1;  // период пунктира в пикселях
  int u_dash_on_ = -1;  // длина «включённого» участка в пикселях

  int u_mvp_ = -1;
  int u_color_ = -1;

  std::vector<float> cpuVertices_;
  std::vector<uint32_t> cpuEdges_;
  QMatrix4x4 view_;
  QMatrix4x4 proj_;

  std::unique_ptr<IProjection> projStrategy_;
  RenderSettings settings_;

  void rebuildEdges();
  void buildGpuBuffers();
  void buildGpuBuffersWithEdges(std::vector<uint32_t> &&edges);

  void updateProjectionMatrix(int w, int h);
};

}  // namespace s21
#endif
