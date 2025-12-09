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

namespace s21 {

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent) {
  // Матрица «камеры»
  transform_.setToIdentity();
  view_.setToIdentity();
  view_.translate(0.f, 0.f, -3.f);

  projStrategy_ = std::make_unique<PerspectiveProjection>(45.f, 0.01f, 100.f);
}

/* =========================
 *     События ввода
 * ========================= */

void GLWidget::wheelEvent(QWheelEvent *e) {
  const int dy = e->angleDelta().y();
  if (!model_) {
    e->ignore();
    return;
  }
  if (dy > 0)
    Scale(1.1f);
  else if (dy < 0)
    Scale(0.9f);
  e->accept();
}

void GLWidget::mousePressEvent(QMouseEvent *e) {
  if (!model_) {
    e->ignore();
    return;
  }
  if (e->button() == Qt::LeftButton || e->button() == Qt::RightButton) {
    lastMousePos_ = e->pos();
    e->accept();
  } else {
    e->ignore();
  }
}

void GLWidget::mouseMoveEvent(QMouseEvent *e) {
  if (!model_) {
    e->ignore();
    return;
  }

  const bool leftHeld = e->buttons() & Qt::LeftButton;
  const bool rightHeld = e->buttons() & Qt::RightButton;
  if (!leftHeld && !rightHeld) {
    e->ignore();
    return;
  }

  const QPoint cur = e->pos();
  const int dx = cur.x() - lastMousePos_.x();
  const int dy = cur.y() - lastMousePos_.y();

  if (leftHeld) {
    if (dx) transform_.rotate(dx * rotateSensitivity_, 0.f, 1.f, 0.f);
    if (dy) transform_.rotate(dy * rotateSensitivity_, 1.f, 0.f, 0.f);
  }
  if (rightHeld) {
    const float tx = dx * translateSensitivity_;
    const float ty = -dy * translateSensitivity_;
    transform_.translate(tx, ty, 0.f);
  }

  lastMousePos_ = cur;
  update();
  e->accept();
}

/* =========================
 *        Слоты
 * ========================= */

void GLWidget::Translate(float dx, float dy, float dz) {
  if (!model_) return;
  transform_.translate(dx, dy, dz);
  update();
}

void GLWidget::ResetTransform() {
  transform_.setToIdentity();
  update();
}

void GLWidget::RotateX(float angle) {
  if (!model_) return;
  transform_.rotate(angle, 1.f, 0.f, 0.f);
  update();
}

void GLWidget::RotateY(float angle) {
  if (!model_) return;
  transform_.rotate(angle, 0.f, 1.f, 0.f);
  update();
}

void GLWidget::RotateZ(float angle) {
  if (!model_) return;
  transform_.rotate(angle, 0.f, 0.f, 1.f);
  update();
}

void GLWidget::Scale(float factor) {
  if (!model_) return;
  transform_.scale(factor);
  update();
}

// Переключение перспективной/ортографической проекции
void GLWidget::ToggleProjection() {
  if (dynamic_cast<PerspectiveProjection *>(projStrategy_.get())) {
    projStrategy_ = std::make_unique<OrthoProjection>(1.0f, -100.f, 100.f);
    settings_.projectionType = 1;
  } else {
    projStrategy_ = std::make_unique<PerspectiveProjection>(45.f, 0.01f, 100.f);
    settings_.projectionType = 0;
  }
  updateProjectionMatrix(width(), height());
}

/* =========================
 *    Инициализация OpenGL
 * ========================= */

void GLWidget::initializeGL() {
  initializeOpenGLFunctions();

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glDisable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);

  glLineWidth(settings_.edgeWidth);
  glClearColor(settings_.background.redF(), settings_.background.greenF(),
               settings_.background.blueF(), 1.0f);

  // --- Шейдер каркаса (линии) ---
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
  ok =
      ok && program_.addShaderFromSourceCode(QOpenGLShader::Fragment, fs_lines);
  ok = ok && program_.link();
  if (!ok) qWarning() << "Shader compile/link error:" << program_.log();
  u_mvp_ = program_.uniformLocation("uMVP");
  u_color_ = program_.uniformLocation("uColor");
  u_dash_ = program_.uniformLocation("uDash");  // ← ВАЖНО

  // --- Шейдер вершин (точки) ---
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
  ok_pts = ok_pts &&
           program_pts_.addShaderFromSourceCode(QOpenGLShader::Vertex, vs_pts);
  ok_pts = ok_pts && program_pts_.addShaderFromSourceCode(
                         QOpenGLShader::Fragment, fs_pts);
  ok_pts = ok_pts && program_pts_.link();
  if (!ok_pts)
    qWarning() << "Point shader compile/link error:" << program_pts_.log();
  u_mvp_pts_ = program_pts_.uniformLocation("uMVP");
  u_color_pts_ = program_pts_.uniformLocation("uColor");
  u_psize_pts_ = program_pts_.uniformLocation("uPointSize");
  u_circle_pts_ = program_pts_.uniformLocation("uCircle");

  // Разрешаем задавать размер точек из шейдера и координату точки
  glEnable(GL_PROGRAM_POINT_SIZE);
#ifdef GL_POINT_SPRITE
  glEnable(GL_POINT_SPRITE);  // требуется некоторым драйверам для gl_PointCoord
#endif

  // --- VAO/VBO/EBO: формат атрибута позиции (location=0) ---
  vao_.create();
  vbo_.create();
  ebo_.create();

  vao_.bind();
  vbo_.bind();
  // данные загрузим позже (в buildGpuBuffers*), здесь описываем формат
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  vbo_.release();
  vao_.release();

  glReady_ = true;
}

void GLWidget::resizeGL(int w, int h) {
  glViewport(0, 0, w, h);
  updateProjectionMatrix(w, h);
}

// Вычислить и применить матрицу проекции из выбранной стратегии
void GLWidget::updateProjectionMatrix(int w, int h) {
  const float aspect = (h == 0) ? 1.f : float(w) / float(h);
  if (!projStrategy_) {
    projStrategy_ = std::make_unique<PerspectiveProjection>(45.f, 0.01f, 100.f);
  }
  proj_ = projStrategy_->Make(aspect);
  update();
}

/* =========================
 *     Построение буферов
 * ========================= */

void GLWidget::rebuildEdges() {
  cpuEdges_.clear();
  if (!model_) return;

  auto t0 = std::chrono::steady_clock::now();

  std::vector<uint64_t> keys;
  size_t estimate = 0;
  const auto &polys = model_->GetPolygons();
  for (const auto &p : polys) estimate += p.points_indices.size();
  keys.reserve(estimate);

  for (const auto &poly : polys) {
    const auto &idx = poly.points_indices;
    const size_t n = idx.size();
    if (n < 2) continue;
    for (size_t i = 0; i < n; ++i) {
      uint32_t a = static_cast<uint32_t>(idx[i]);
      uint32_t b = static_cast<uint32_t>(idx[(i + 1) % n]);
      if (a == b) continue;
      if (a > b) std::swap(a, b);
      keys.push_back((uint64_t(a) << 32) | uint64_t(b));
    }
  }

  std::sort(keys.begin(), keys.end());
  keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

  cpuEdges_.resize(keys.size() * 2);
  for (size_t i = 0; i < keys.size(); ++i) {
    cpuEdges_[2 * i] = static_cast<uint32_t>(keys[i] >> 32);
    cpuEdges_[2 * i + 1] = static_cast<uint32_t>(keys[i] & 0xffffffffull);
  }

  auto t1 = std::chrono::steady_clock::now();
  qDebug() << "[Perf] rebuildEdges:"
           << std::chrono::duration<double, std::milli>(t1 - t0).count()
           << "ms";
}

// Полный цикл (когда рёбра считает сам виджет) — для обратной совместимости
void GLWidget::buildGpuBuffers() {
  cpuVertices_.clear();
  if (!model_) return;

  auto t0 = std::chrono::steady_clock::now();

  // Вершины (double -> float, один раз)
  const auto &vs = model_->GetVertices();
  cpuVertices_.reserve(vs.size() * 3);
  for (const auto &v : vs) {
    cpuVertices_.push_back(static_cast<float>(v.x));
    cpuVertices_.push_back(static_cast<float>(v.y));
    cpuVertices_.push_back(static_cast<float>(v.z));
  }

  // Рёбра (медленно для больших моделей — лучше не в UI потоке)
  rebuildEdges();

  // Аплоад в GPU
  vao_.bind();

  vbo_.bind();
  if (!cpuVertices_.empty())
    vbo_.allocate(cpuVertices_.data(),
                  static_cast<int>(cpuVertices_.size() * sizeof(float)));
  else
    vbo_.allocate(nullptr, 0);
  vbo_.release();

  ebo_.bind();
  if (!cpuEdges_.empty())
    ebo_.allocate(cpuEdges_.data(),
                  static_cast<int>(cpuEdges_.size() * sizeof(uint32_t)));
  else
    ebo_.allocate(nullptr, 0);
  ebo_.release();

  vao_.release();

  auto t1 = std::chrono::steady_clock::now();
  qDebug() << "[Perf] buildGpuBuffers:"
           << std::chrono::duration<double, std::milli>(t1 - t0).count()
           << "ms";
}

// Быстрый путь: когда рёбра уже посчитаны в фоне
void GLWidget::buildGpuBuffersWithEdges(std::vector<uint32_t> &&edges) {
  cpuVertices_.clear();
  cpuEdges_ = std::move(edges);
  if (!model_) return;

  // Вершины (double -> float)
  const auto &vs = model_->GetVertices();
  cpuVertices_.reserve(vs.size() * 3);
  for (const auto &v : vs) {
    cpuVertices_.push_back(static_cast<float>(v.x));
    cpuVertices_.push_back(static_cast<float>(v.y));
    cpuVertices_.push_back(static_cast<float>(v.z));
  }

  // Аплоад в GPU
  vao_.bind();

  vbo_.bind();
  if (!cpuVertices_.empty())
    vbo_.allocate(cpuVertices_.data(),
                  static_cast<int>(cpuVertices_.size() * sizeof(float)));
  else
    vbo_.allocate(nullptr, 0);
  vbo_.release();

  ebo_.bind();
  if (!cpuEdges_.empty())
    ebo_.allocate(cpuEdges_.data(),
                  static_cast<int>(cpuEdges_.size() * sizeof(uint32_t)));
  else
    ebo_.allocate(nullptr, 0);
  ebo_.release();

  vao_.release();
}

/* =========================
 *  Отрисовка одного кадра
 * ========================= */

void GLWidget::paintGL() {
  // Цвет фона из настроек (каждый кадр — чтобы изменения применялись сразу)
  glClearColor(settings_.background.redF(), settings_.background.greenF(),
               settings_.background.blueF(), 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef NDEBUG
  if (!model_) {
    qDebug() << "[Paint] no model";
    return;
  }
  if (cpuEdges_.empty()) {
    qDebug() << "[Paint] no edges";
    return;
  }
#else
  if (!model_ || cpuEdges_.empty()) return;
#endif

  // Толщина линий из настроек
  glLineWidth(settings_.edgeWidth);

  const QMatrix4x4 mvp = proj_ * view_ * transform_;

  // --- ЛИНИИ (каркас) ---
  program_.bind();
  program_.setUniformValue(u_mvp_, mvp);
  program_.setUniformValue(
      u_color_,
      QVector4D(settings_.edgeColor.redF(), settings_.edgeColor.greenF(),
                settings_.edgeColor.blueF(), 1.0f));
  program_.setUniformValue(u_dash_, settings_.edgeType == 1 ? 1 : 0);  // ← это

  vao_.bind();
  ebo_.bind();
  program_.setUniformValue(u_dash_, settings_.edgeType == 1 ? 1 : 0);
  glDrawElements(GL_LINES, static_cast<GLsizei>(cpuEdges_.size()),
                 GL_UNSIGNED_INT, nullptr);
  ebo_.release();
  vao_.release();
  program_.release();

  // --- ВЕРШИНЫ (точки), если включено ---
  // vertexType: 0=off, 1=circle, 2=square
  if (settings_.vertexType != 0 && model_) {
    program_pts_.bind();
    program_pts_.setUniformValue(u_mvp_pts_, mvp);
    program_pts_.setUniformValue(
        u_color_pts_,
        QVector4D(settings_.vertexColor.redF(), settings_.vertexColor.greenF(),
                  settings_.vertexColor.blueF(), 1.0f));
    program_pts_.setUniformValue(u_psize_pts_, settings_.vertexSize);
    const int isCircle = (settings_.vertexType == 1) ? 1 : 0;
    program_pts_.setUniformValue(u_circle_pts_, isCircle);

    vao_.bind();  // атрибут location=0 уже описан в VAO
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(model_->GetNumVertices()));
    vao_.release();

    program_pts_.release();
  }
}

/* =========================
 *     Привязка модели
 * ========================= */

void GLWidget::SetModel(const s21::Model *model) {
  model_ = model;
  ResetTransform();
  if (model_) {
    buildGpuBuffers();  // медленнее — считает рёбра здесь
  } else {
    // очистка GPU-буферов
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

void GLWidget::SetModelAndEdges(const s21::Model *model,
                                std::vector<uint32_t> &&edges) {
  model_ = model;
  ResetTransform();
  if (model_) {
    buildGpuBuffersWithEdges(std::move(edges));  // быстрый путь
  } else {
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

void GLWidget::SetSettings(const RenderSettings &s) {
  const bool projChanged = (s.projectionType != settings_.projectionType);
  settings_ = s;

  if (projChanged) {
    if (settings_.projectionType == 0) {
      projStrategy_ =
          std::make_unique<PerspectiveProjection>(45.f, 0.01f, 100.f);
    } else {
      projStrategy_ = std::make_unique<OrthoProjection>(1.0f, -100.f, 100.f);
    }
    updateProjectionMatrix(width(), height());
  }

  update();
}

QImage GLWidget::GrabFrame() { return this->grabFramebuffer(); }

}  // namespace s21
