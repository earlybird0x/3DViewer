#include "mainwindow.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QtGlobal>
#include <algorithm>
#include <utility>
#include <vector>

#include "controller/controller.h"
#include "ui_mainwindow.h"
#include "view/frame_recorder.h"
#include "view/gif_saver.h"

namespace
{

  QString loadLastDir()
  {
    QSettings st("s21", "3DViewer");
    QString d = st.value("UI/lastDir").toString();
    if (d.isEmpty())
    {
      d = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
      if (d.isEmpty())
      {
        d = QDir::homePath();
      }
    }
    return d;
  }

  void saveLastDirFromPath(const QString &filePath)
  {
    const QString dir = QFileInfo(filePath).absolutePath();
    if (!dir.isEmpty())
    {
      QSettings st("s21", "3DViewer");
      st.setValue("UI/lastDir", dir);
    }
  }

} // namespace

namespace s21
{

  MainWindow::MainWindow(QWidget *parent)
      : QMainWindow(parent), ui_(new Ui::MainWindow)
  {
    ui_->setupUi(this);
    setWindowTitle("3DViewer by GRANCETI & ANGIELYN");

    controller_ = new s21::Controller(this);
    recorder_ = new s21::FrameRecorder(ui_->openGLWidget, this);

    ui_->statusLabel->setWordWrap(true);
    ui_->statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui_->rotateXbutton->setEnabled(false);
    ui_->rotateYbutton->setEnabled(false);
    ui_->rotateZbutton->setEnabled(false);
    ui_->zoomInButton->setEnabled(false);
    ui_->zoomOutButton->setEnabled(false);
    ui_->resetButton->setEnabled(false);

    {
      QSettings st("s21", "3DViewer");
      RenderSettings settings = ui_->openGLWidget->settings();
      settings.Load(st);
      ui_->edgeTypeCombo->setCurrentIndex(
          ui_->openGLWidget->settings().edgeType);
      ui_->openGLWidget->SetSettings(settings);
      ui_->edgeWidthSpin->setValue(settings.edgeWidth);
      ui_->projectionCombo->setCurrentIndex(settings.projectionType);
    }

    {
      RenderSettings settings = ui_->openGLWidget->settings();
      ui_->vertexColorButton->setStyleSheet(QString());
      ui_->vertexColorButton->setEnabled(settings.vertexType != 0);
    }

    {
      RenderSettings settings = ui_->openGLWidget->settings();
      int edge_type = settings.edgeType;
      if (edge_type < 0 || edge_type > 1)
      {
        edge_type = 0;
      }
      ui_->edgeTypeCombo->setCurrentIndex(edge_type);
    }

    {
      RenderSettings settings = ui_->openGLWidget->settings();
      ui_->vertexSizeSpin->setValue(settings.vertexSize);
      ui_->vertexSizeSpin->setEnabled(settings.vertexType != 0);
    }

    {
      RenderSettings settings = ui_->openGLWidget->settings();
      int type = settings.vertexType;
      if (type < 0 || type > 2)
      {
        type = 0;
      }
      ui_->vertexTypeCombo->setCurrentIndex(type);
    }

    connect(ui_->pushButton_2, &QPushButton::clicked, this,
            &MainWindow::HandleFileOpen);

    connect(ui_->rotateXbutton, &QPushButton::clicked, this, [this]()
            { ui_->openGLWidget->RotateX(10.0F); });

    connect(ui_->rotateYbutton, &QPushButton::clicked, this, [this]()
            { ui_->openGLWidget->RotateY(10.0F); });

    connect(ui_->rotateZbutton, &QPushButton::clicked, this, [this]()
            { ui_->openGLWidget->RotateZ(10.0F); });

    connect(ui_->zoomInButton, &QPushButton::clicked, this, [this]()
            { ui_->openGLWidget->Scale(1.1F); });

    connect(ui_->zoomOutButton, &QPushButton::clicked, this, [this]()
            { ui_->openGLWidget->Scale(0.9F); });

    connect(ui_->moveApplyButton, &QPushButton::clicked, this, [this]()
            { ui_->openGLWidget->Translate(
                  static_cast<float>(ui_->moveXSpin->value()),
                  static_cast<float>(ui_->moveYSpin->value()),
                  static_cast<float>(ui_->moveZSpin->value())); });

    connect(ui_->resetButton, &QPushButton::clicked, ui_->openGLWidget,
            &GLWidget::ResetTransform);

    connect(ui_->vertexTypeCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
              RenderSettings settings = ui_->openGLWidget->settings();
              settings.vertexType = index;
              ui_->openGLWidget->SetSettings(settings);
              QSettings st("s21", "3DViewer");
              settings.Save(st);
              ui_->vertexSizeSpin->setEnabled(index != 0);
              ui_->vertexColorButton->setEnabled(index != 0);
            });

    connect(ui_->projectionCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
              RenderSettings settings = ui_->openGLWidget->settings();
              settings.projectionType = (index == 1) ? 1 : 0;
              ui_->openGLWidget->SetSettings(settings);
              QSettings st("s21", "3DViewer");
              settings.Save(st);
            });

    connect(ui_->vertexColorButton, &QPushButton::clicked, this, [this]()
            {
    RenderSettings settings = ui_->openGLWidget->settings();
    const QColor chosen =
        QColorDialog::getColor(settings.vertexColor, this,
                               "Выберите цвет вершин",
                               QColorDialog::ShowAlphaChannel);
    if (!chosen.isValid()) {
      return;
    }

    settings.vertexColor = chosen;
    ui_->openGLWidget->SetSettings(settings);
    ui_->vertexColorButton->setStyleSheet(QString());

    QSettings st("s21", "3DViewer");
    settings.Save(st); });

    connect(ui_->resetSettingsButton, &QPushButton::clicked, this, [this]()
            {
    const auto reply = QMessageBox::question(
        this, "Сброс настроек",
        "Вернуть настройки к заводским значениям?\n"
        "Текущие пользовательские настройки будут удалены.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) {
      return;
    }

    QSettings st("s21", "3DViewer");
    st.remove("RenderSettings");

    RenderSettings defaults;
    ui_->openGLWidget->SetSettings(defaults);
    defaults.Save(st);

    ui_->projectionCombo->setCurrentIndex(defaults.projectionType);
    ui_->edgeTypeCombo->setCurrentIndex(defaults.edgeType);
    ui_->edgeWidthSpin->setValue(defaults.edgeWidth);
    ui_->statusLabel->setText("Настройки сброшены к значениям по умолчанию"); });

    connect(ui_->edgeTypeCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
              RenderSettings settings = ui_->openGLWidget->settings();
              settings.edgeType = (index == 1) ? 1 : 0;
              ui_->openGLWidget->SetSettings(settings);
              QSettings st("s21", "3DViewer");
              settings.Save(st);
            });

    connect(ui_->bgColorButton, &QPushButton::clicked, this, [this]()
            {
    if (!ui_->openGLWidget) {
      return;
    }

    RenderSettings settings = ui_->openGLWidget->settings();
    const QColor color =
        QColorDialog::getColor(settings.background, this,
                               "Выберите цвет фона");
    if (color.isValid()) {
      settings.background = color;
      ui_->openGLWidget->SetSettings(settings);
      QSettings st("s21", "3DViewer");
      settings.Save(st);
      st.sync();
    } });

    connect(ui_->edgeColorButton, &QPushButton::clicked, this, [this]()
            {
    if (!ui_->openGLWidget) {
      return;
    }

    RenderSettings settings = ui_->openGLWidget->settings();
    const QColor color =
        QColorDialog::getColor(settings.edgeColor, this,
                               "Выберите цвет рёбер");
    if (color.isValid()) {
      settings.edgeColor = color;
      ui_->openGLWidget->SetSettings(settings);
      QSettings st("s21", "3DViewer");
      settings.Save(st);
      st.sync();
    } });

    connect(ui_->edgeWidthSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value)
            {
              if (!ui_->openGLWidget)
              {
                return;
              }

              RenderSettings settings = ui_->openGLWidget->settings();
              settings.edgeWidth = static_cast<float>(value);
              ui_->openGLWidget->SetSettings(settings);
              QSettings st("s21", "3DViewer");
              settings.Save(st);
              st.sync();
            });

    connect(ui_->saveSnapshotButton, &QPushButton::clicked, this, [this]()
            {
    if (!ui_->openGLWidget) {
      return;
    }

    QString selected_filter;
    const QString suggested =
        QDir(loadLastDir()).filePath("screenshot");
    QString path = QFileDialog::getSaveFileName(
        this, "Сохранить снимок", suggested,
        "PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;BMP Image (*.bmp)",
        &selected_filter);
    if (path.isEmpty()) {
      return;
    }

    QString format;
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix.isEmpty()) {
      const QString selected = selected_filter.toLower();
      if (selected.contains("*.png")) {
        format = "PNG";
        path += ".png";
      } else if (selected.contains("*.jpg")) {
        format = "JPG";
        path += ".jpg";
      } else if (selected.contains("*.bmp")) {
        format = "BMP";
        path += ".bmp";
      } else {
        format = "PNG";
        path += ".png";
      }
    } else {
      if (suffix == "png") {
        format = "PNG";
      } else if (suffix == "jpg" || suffix == "jpeg") {
        format = "JPG";
      } else if (suffix == "bmp") {
        format = "BMP";
      } else {
        format = "PNG";
        path += ".png";
      }
    }

    QImage image = ui_->openGLWidget->GrabFrame();
    if (format == "JPG" &&
        image.format() != QImage::Format_RGB888) {
      image = image.convertToFormat(QImage::Format_RGB888);
    }

    const bool ok = image.save(path, format.toUtf8().constData());
    if (ok) {
      saveLastDirFromPath(path);
    }
    ui_->statusLabel->setText(ok ? "Снимок сохранён: " + path
                                 : "Не удалось сохранить снимок"); });

    connect(ui_->vertexSizeSpin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value)
            {
              RenderSettings settings = ui_->openGLWidget->settings();
              settings.vertexSize =
                  static_cast<float>(std::clamp(value, 1.0, 64.0));
              ui_->openGLWidget->SetSettings(settings);
              QSettings st("s21", "3DViewer");
              settings.Save(st);
            });

    ui_->gifRecordButton->setEnabled(true);

    connect(recorder_, &FrameRecorder::Started, this, [this]()
            {
    ui_->statusLabel->setText("Запись GIF началась…");
    ui_->gifRecordButton->setEnabled(false); });

    connect(recorder_, &FrameRecorder::Progress, this,
            [this](int current, int total)
            {
              ui_->statusLabel->setText(
                  QString("GIF: кадров %1 / %2")
                      .arg(current)
                      .arg(total));
            });

    connect(recorder_, &FrameRecorder::Finished, this, [this]()
            {
    ui_->gifRecordButton->setEnabled(true);

    const auto& frames = recorder_->frames();
    if (frames.isEmpty()) {
      ui_->statusLabel->setText("Запись остановлена");
      return;
    }

    frames.first().save("debug_before_gif.png");

    QString selected_filter;
    const QString suggested =
        QDir(loadLastDir()).filePath("animation.gif");
    QString path = QFileDialog::getSaveFileName(
        this, "Сохранить GIF", suggested,
        "GIF Animation (*.gif)", &selected_filter);
    if (path.isEmpty()) {
      recorder_->Clear();
      ui_->statusLabel->setText("Сохранение отменено");
      return;
    }

    if (QFileInfo(path).suffix().toLower() != "gif") {
      path += ".gif";
    }

    const bool ok = SaveGif(frames, path, /*delayCs=*/10, /*loop=*/0,
                            /*W=*/640, /*H=*/480);
    if (ok) {
      saveLastDirFromPath(path);
    }

    ui_->statusLabel->setText(ok ? "GIF сохранён"
                                 : "Не удалось сохранить GIF");
    recorder_->Clear(); });

    connect(recorder_, &FrameRecorder::Error, this,
            [this](const QString &message)
            {
              QMessageBox::warning(this, "Запись GIF", message);
            });

    connect(ui_->gifRecordButton, &QPushButton::clicked, this, [this]()
            {
    if (!recorder_) {
      return;
    }
    recorder_->SetAutoRotate(true);
    recorder_->Start(/*fps=*/10, /*sec=*/5); });

    connect(
        controller_, &Controller::Loaded, this,
        [this](const Model *model, std::vector<uint32_t> edges,
               double total_ms)
        {
          ui_->statusLabel->setText(
              QString("Вершин: %1\nРёбер (факт): %2\nЗагрузка+разбор+рёбра: %3 мс")
                  .arg(model->GetNumVertices())
                  .arg(edges.size() / 2)
                  .arg(total_ms, 0, 'f', 1));

          ui_->openGLWidget->SetModelAndEdges(model, std::move(edges));
          ui_->openGLWidget->update();

          ui_->rotateXbutton->setEnabled(true);
          ui_->rotateYbutton->setEnabled(true);
          ui_->rotateZbutton->setEnabled(true);
          ui_->zoomInButton->setEnabled(true);
          ui_->zoomOutButton->setEnabled(true);
          ui_->resetButton->setEnabled(true);
        });

    connect(controller_, &Controller::Failed, this,
            [this](const QString &error)
            {
              ui_->statusLabel->setText("Ошибка: " + error);
            });

    connect(controller_, &Controller::Updated, this,
            [this](const Model *model, std::vector<uint32_t> edges)
            {
              ui_->statusLabel->setText(
                  QString("Вершин: %1\nРёбер (факт): %2")
                      .arg(model->GetNumVertices())
                      .arg(edges.size() / 2));

              ui_->openGLWidget->SetModelAndEdges(model, std::move(edges));
              ui_->openGLWidget->update();
            });
  }

  MainWindow::~MainWindow()
  {
    delete ui_;
  }

  void MainWindow::HandleFileOpen()
  {
    const QString start_dir = loadLastDir();
    const QString file_name = QFileDialog::getOpenFileName(
        this, "Выберите .obj файл", start_dir, "OBJ Files (*.obj)");
    if (file_name.isEmpty())
    {
      ui_->statusLabel->setText("Выбор отменён");
      return;
    }

    saveLastDirFromPath(file_name);

    ui_->statusLabel->setText("Загрузка…");
    ui_->rotateXbutton->setEnabled(false);
    ui_->rotateYbutton->setEnabled(false);
    ui_->rotateZbutton->setEnabled(false);
    ui_->zoomInButton->setEnabled(false);
    ui_->zoomOutButton->setEnabled(false);
    ui_->resetButton->setEnabled(false);
    ui_->openGLWidget->SetModel(nullptr);

    controller_->LoadAsync(file_name);
  }

} // namespace s21
