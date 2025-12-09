#include "view/glwidget.h"

#include <QDebug>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector4D>
#include <QWheelEvent>
#include <chrono>
#include <memory>
#include <utility>

namespace s21
{

  GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent)
  {
    transform_.setToIdentity();
    view_.setToIdentity();
    view_.translate(0.0F, 0.0F, -3.0F);

    projStrategy_ = std::make_unique<PerspectiveProjection>(45.0F, 0.01F, 100.0F);
  }

  void GLWidget::wheelEvent(QWheelEvent *event)
  {
    const int delta_y = event->angleDelta().y();
    if (!model_)
    {
      event->ignore();
      return;
    }

    if (delta_y > 0)
    {
      Scale(1.1F);
    }
    else if (delta_y < 0)
    {
      Scale(0.9F);
    }

    event->accept();
  }

  void GLWidget::mousePressEvent(QMouseEvent *event)
  {
    if (!model_)
    {
      event->ignore();
      return;
    }

    if (event->button() == Qt::LeftButton ||
        event->button() == Qt::RightButton)
    {
      lastMousePos_ = event->pos();
      event->accept();
    }
    else
    {
      event->ignore();
    }
  }

  void GLWidget::mouseMoveEvent(QMouseEvent *event)
  {
    if (!model_)
    {
      event->ignore();
      return;
    }

    const bool left_held = event->buttons() & Qt::LeftButton;
    const bool right_held = event->buttons() & Qt::RightButton;
    if (!left_held && !right_held)
    {
      event->ignore();
      return;
    }

    const QPoint current = event->pos();
    const int dx = current.x() - lastMousePos_.x();
    const int dy = current.y() - lastMousePos_.y();

    if (left_held)
    {
      if (dx)
      {
        transform_.rotate(dx * rotateSensitivity_, 0.0F, 1.0F, 0.0F);
      }
      if (dy)
      {
        transform_.rotate(dy * rotateSensitivity_, 1.0F, 0.0F, 0.0F);
      }
    }

    if (right_held)
    {
      const float tx = dx * translateSensitivity_;
      const float ty = -dy * translateSensitivity_;
      transform_.translate(tx, ty, 0.0F);
    }

    lastMousePos_ = current;
    update();
    event->accept();
  }

  void GLWidget::Translate(float dx, float dy, float dz)
  {
    if (!model_)
    {
      return;
    }
    transform_.translate(dx, dy, dz);
    update();
  }

  void GLWidget::ResetTransform()
  {
    transform_.setToIdentity();
    update();
  }

  void GLWidget::RotateX(float angle)
  {
    if (!model_)
    {
      return;
    }
    transform_.rotate(angle, 1.0F, 0.0F, 0.0F);
    update();
  }

  void GLWidget::RotateY(float angle)
  {
    if (!model_)
    {
      return;
    }
    transform_.rotate(angle, 0.0F, 1.0F, 0.0F);
    update();
  }

  void GLWidget::RotateZ(float angle)
  {
    if (!model_)
    {
      return;
    }
    transform_.rotate(angle, 0.0F, 0.0F, 1.0F);
    update();
  }

  void GLWidget::Scale(float factor)
  {
    if (!model_)
    {
      return;
    }
    transform_.scale(factor);
    update();
  }

  void GLWidget::ToggleProjection()
  {
    if (dynamic_cast<PerspectiveProjection *>(projStrategy_.get()))
    {
      projStrategy_ = std::make_unique<OrthoProjection>(1.0F, -100.0F, 100.0F);
      settings_.projectionType = 1;
    }
    else
    {
      projStrategy_ =
          std::make_unique<PerspectiveProjection>(45.0F, 0.01F, 100.0F);
      settings_.projectionType = 0;
    }
    updateProjectionMatrix(width(), height());
  }

  void GLWidget::initializeGL()
  {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);

    glLineWidth(settings_.edgeWidth);
    glClearColor(settings_.background.redF(),
                 settings_.background.greenF(),
                 settings_.background.blueF(),
                 1.0F);

    const char *vs_lines = R"(#version 330 core
        layout (location=0) in vec3 aPos;
        uniform mat4 uMVP;
        void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
    )";

    const char *fs_lines = R"(#version 330 core
    uniform vec4 uColor;
    uniform int  uDash;    // 0 = сплошные, 1 = пунктир
    out vec4 FragColor;
    void main(){
        if (uDash == 1) {
            // простой экранный пунктир (по диагонали)
            float m = mod(floor(gl_FragCoord.x + gl_FragCoord.y), 8.0);
            if (m < 4.0) discard;      // 4px «пусто», 4px «рисуем»
        }
        FragColor = uColor;
    }
)";

    bool ok = true;
    ok = ok && program_.addShaderFromSourceCode(QOpenGLShader::Vertex, vs_lines);
    ok = ok &&
         program_.addShaderFromSourceCode(QOpenGLShader::Fragment, fs_lines);
    ok = ok && program_.link();
    if (!ok)
    {
      qWarning() << "Shader compile/link error:" << program_.log();
    }

    u_mvp_ = program_.uniformLocation("uMVP");
    u_color_ = program_.uniformLocation("uColor");
    u_dash_ = program_.uniformLocation("uDash");

    const char *vs_pts = R"(#version 330 core
        layout (location=0) in vec3 aPos;
        uniform mat4  uMVP;
        uniform float uPointSize;
        void main(){
            gl_Position = uMVP * vec4(aPos, 1.0);
            gl_PointSize = uPointSize;   // размер точки в пикселях
        }
    )";

    const char *fs_pts = R"(#version 330 core
        uniform vec4 uColor;
        uniform int  uCircle;      // 1 = круг, 0 = квадрат
        out vec4 FragColor;
        void main(){
            if (uCircle == 1) {
                // gl_PointCoord ∈ [0,1]^2 — координата внутри квадрата точки
                vec2 p = gl_PointCoord * 2.0 - 1.0;  // центр в (0,0)
                if (dot(p,p) > 1.0) discard;         // обрезаем до круга
            }
            FragColor = uColor;
        }
    )";

    bool ok_pts = true;
    ok_pts =
        ok_pts &&
        program_pts_.addShaderFromSourceCode(QOpenGLShader::Vertex, vs_pts);
    ok_pts = ok_pts && program_pts_.addShaderFromSourceCode(
                           QOpenGLShader::Fragment, fs_pts);
    ok_pts = ok_pts && program_pts_.link();
    if (!ok_pts)
    {
      qWarning() << "Point shader compile/link error:" << program_pts_.log();
    }

    u_mvp_pts_ = program_pts_.uniformLocation("uMVP");
    u_color_pts_ = program_pts_.uniformLocation("uColor");
    u_psize_pts_ = program_pts_.uniformLocation("uPointSize");
    u_circle_pts_ = program_pts_.uniformLocation("uCircle");

    glEnable(GL_PROGRAM_POINT_SIZE);
#ifdef GL_POINT_SPRITE
    glEnable(GL_POINT_SPRITE);
#endif

    vao_.create();
    vbo_.create();
    ebo_.create();

    vao_.bind();
    vbo_.bind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);
    vbo_.release();
    vao_.release();

    glReady_ = true;
  }

  void GLWidget::resizeGL(int w, int h)
  {
    glViewport(0, 0, w, h);
    updateProjectionMatrix(w, h);
  }

  void GLWidget::updateProjectionMatrix(int w, int h)
  {
    const float aspect = (h == 0) ? 1.0F : static_cast<float>(w) / h;
    if (!projStrategy_)
    {
      projStrategy_ =
          std::make_unique<PerspectiveProjection>(45.0F, 0.01F, 100.0F);
    }
    proj_ = projStrategy_->Make(aspect);
    update();
  }

  void GLWidget::rebuildEdges()
  {
    cpuEdges_.clear();
    if (!model_)
    {
      return;
    }

    auto t0 = std::chrono::steady_clock::now();

    std::vector<uint64_t> keys;
    size_t estimate = 0;

    const auto &polys = model_->GetPolygons();
    for (const auto &polygon : polys)
    {
      estimate += polygon.points_indices.size();
    }
    keys.reserve(estimate);

    for (const auto &polygon : polys)
    {
      const auto &indices = polygon.points_indices;
      const size_t count = indices.size();
      if (count < 2)
      {
        continue;
      }

      for (size_t i = 0; i < count; ++i)
      {
        uint32_t a = static_cast<uint32_t>(indices[i]);
        uint32_t b = static_cast<uint32_t>(indices[(i + 1) % count]);
        if (a == b)
        {
          continue;
        }
        if (a > b)
        {
          std::swap(a, b);
        }
        keys.push_back((static_cast<uint64_t>(a) << 32) |
                       static_cast<uint64_t>(b));
      }
    }

    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    cpuEdges_.resize(keys.size() * 2);
    for (size_t i = 0; i < keys.size(); ++i)
    {
      cpuEdges_[2 * i] = static_cast<uint32_t>(keys[i] >> 32);
      cpuEdges_[2 * i + 1] =
          static_cast<uint32_t>(keys[i] & 0xffffffffULL);
    }

    auto t1 = std::chrono::steady_clock::now();
    qDebug() << "[Perf] rebuildEdges:"
             << std::chrono::duration<double, std::milli>(t1 - t0).count()
             << "ms";
  }

  void GLWidget::buildGpuBuffers()
  {
    cpuVertices_.clear();
    if (!model_)
    {
      return;
    }

    auto t0 = std::chrono::steady_clock::now();

    const auto &vertices = model_->GetVertices();
    cpuVertices_.reserve(vertices.size() * 3);
    for (const auto &vertex : vertices)
    {
      cpuVertices_.push_back(static_cast<float>(vertex.x));
      cpuVertices_.push_back(static_cast<float>(vertex.y));
      cpuVertices_.push_back(static_cast<float>(vertex.z));
    }

    rebuildEdges();

    vao_.bind();

    vbo_.bind();
    if (!cpuVertices_.empty())
    {
      vbo_.allocate(cpuVertices_.data(),
                    static_cast<int>(cpuVertices_.size() * sizeof(float)));
    }
    else
    {
      vbo_.allocate(nullptr, 0);
    }
    vbo_.release();

    ebo_.bind();
    if (!cpuEdges_.empty())
    {
      ebo_.allocate(cpuEdges_.data(),
                    static_cast<int>(cpuEdges_.size() * sizeof(uint32_t)));
    }
    else
    {
      ebo_.allocate(nullptr, 0);
    }
    ebo_.release();

    vao_.release();

    auto t1 = std::chrono::steady_clock::now();
    qDebug() << "[Perf] buildGpuBuffers:"
             << std::chrono::duration<double, std::milli>(t1 - t0).count()
             << "ms";
  }

  void GLWidget::buildGpuBuffersWithEdges(std::vector<uint32_t> &&edges)
  {
    cpuVertices_.clear();
    cpuEdges_ = std::move(edges);
    if (!model_)
    {
      return;
    }

    const auto &vertices = model_->GetVertices();
    cpuVertices_.reserve(vertices.size() * 3);
    for (const auto &vertex : vertices)
    {
      cpuVertices_.push_back(static_cast<float>(vertex.x));
      cpuVertices_.push_back(static_cast<float>(vertex.y));
      cpuVertices_.push_back(static_cast<float>(vertex.z));
    }

    vao_.bind();

    vbo_.bind();
    if (!cpuVertices_.empty())
    {
      vbo_.allocate(cpuVertices_.data(),
                    static_cast<int>(cpuVertices_.size() * sizeof(float)));
    }
    else
    {
      vbo_.allocate(nullptr, 0);
    }
    vbo_.release();

    ebo_.bind();
    if (!cpuEdges_.empty())
    {
      ebo_.allocate(cpuEdges_.data(),
                    static_cast<int>(cpuEdges_.size() * sizeof(uint32_t)));
    }
    else
    {
      ebo_.allocate(nullptr, 0);
    }
    ebo_.release();

    vao_.release();
  }

  void GLWidget::paintGL()
  {
    glClearColor(settings_.background.redF(),
                 settings_.background.greenF(),
                 settings_.background.blueF(),
                 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef NDEBUG
    if (!model_)
    {
      qDebug() << "[Paint] no model";
      return;
    }
    if (cpuEdges_.empty())
    {
      qDebug() << "[Paint] no edges";
      return;
    }
#else
    if (!model_ || cpuEdges_.empty())
    {
      return;
    }
#endif

    glLineWidth(settings_.edgeWidth);

    const QMatrix4x4 mvp = proj_ * view_ * transform_;

    program_.bind();
    program_.setUniformValue(u_mvp_, mvp);
    program_.setUniformValue(
        u_color_,
        QVector4D(settings_.edgeColor.redF(),
                  settings_.edgeColor.greenF(),
                  settings_.edgeColor.blueF(),
                  1.0F));
    program_.setUniformValue(u_dash_, settings_.edgeType == 1 ? 1 : 0);

    vao_.bind();
    ebo_.bind();
    glDrawElements(GL_LINES,
                   static_cast<GLsizei>(cpuEdges_.size()),
                   GL_UNSIGNED_INT,
                   nullptr);
    ebo_.release();
    vao_.release();
    program_.release();

    if (settings_.vertexType != 0 && model_)
    {
      program_pts_.bind();
      program_pts_.setUniformValue(u_mvp_pts_, mvp);
      program_pts_.setUniformValue(
          u_color_pts_,
          QVector4D(settings_.vertexColor.redF(),
                    settings_.vertexColor.greenF(),
                    settings_.vertexColor.blueF(),
                    1.0F));
      program_pts_.setUniformValue(u_psize_pts_, settings_.vertexSize);
      const int is_circle = (settings_.vertexType == 1) ? 1 : 0;
      program_pts_.setUniformValue(u_circle_pts_, is_circle);

      vao_.bind();
      glDrawArrays(GL_POINTS,
                   0,
                   static_cast<GLsizei>(model_->GetNumVertices()));
      vao_.release();

      program_pts_.release();
    }
  }

  void GLWidget::SetModel(const Model *model)
  {
    model_ = model;
    ResetTransform();
    if (model_)
    {
      buildGpuBuffers();
    }
    else
    {
      vao_.bind();
      vbo_.bind();
      vbo_.allocate(nullptr, 0);
      vbo_.release();
      ebo_.bind();
      ebo_.allocate(nullptr, 0);
      ebo_.release();
      vao_.release();
    }
  }

  void GLWidget::SetModelAndEdges(const Model *model,
                                  std::vector<uint32_t> &&edges)
  {
    model_ = model;
    ResetTransform();
    if (model_)
    {
      buildGpuBuffersWithEdges(std::move(edges));
    }
    else
    {
      vao_.bind();
      vbo_.bind();
      vbo_.allocate(nullptr, 0);
      vbo_.release();
      ebo_.bind();
      ebo_.allocate(nullptr, 0);
      ebo_.release();
      vao_.release();
    }
  }

  void GLWidget::SetSettings(const RenderSettings &settings)
  {
    const bool projection_changed =
        (settings.projectionType != settings_.projectionType);
    settings_ = settings;

    if (projection_changed)
    {
      if (settings_.projectionType == 0)
      {
        projStrategy_ =
            std::make_unique<PerspectiveProjection>(45.0F, 0.01F, 100.0F);
      }
      else
      {
        projStrategy_ =
            std::make_unique<OrthoProjection>(1.0F, -100.0F, 100.0F);
      }
      updateProjectionMatrix(width(), height());
    }

    update();
  }

  QImage GLWidget::GrabFrame()
  {
    return grabFramebuffer();
  }

} // namespace s21
