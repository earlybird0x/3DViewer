#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui
{
  class MainWindow;
}
QT_END_NAMESPACE

#include "controller/controller.h"

namespace s21
{

  class FrameRecorder;

  class MainWindow : public QMainWindow
  {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private slots:
    void HandleFileOpen();

  private:
    Ui::MainWindow *ui_ = nullptr;
    Controller *controller_ = nullptr;
    FrameRecorder *recorder_ = nullptr;
  };

} // namespace s21

#endif
