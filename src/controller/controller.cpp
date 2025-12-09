#include "controller/controller.h"

#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <chrono>

namespace s21
{

  Controller::Controller(QObject *parent) : QObject(parent) {}

  bool Controller::LoadFromFile(const QString &path, QString *error)
  {
    std::string err;
    if (!loader_.Load(path.toStdString(), model_, &err))
    {
      if (error)
      {
        *error = QString::fromStdString(
            err.empty() ? "Ошибка загрузки файла" : err);
      }
      return false;
    }

    std::vector<uint32_t> edges;
    model_.BuildEdges(edges);
    return true;
  }

  void Controller::LoadAsync(const QString &path)
  {
    using ResultT =
        std::tuple<Model, std::vector<uint32_t>, double, std::string>;

    auto future =
        QtConcurrent::run([pathStr = path.toStdString(), this]() -> ResultT
                          {
        auto t0 = std::chrono::steady_clock::now();

        Model m;
        std::string err;
        if (!loader_.Load(pathStr, m, &err)) {
          return {Model{}, std::vector<uint32_t>{}, 0.0,
                  err.empty() ? "Не удалось загрузить файл" : err};
        }

        std::vector<uint32_t> edges;
        m.BuildEdges(edges);

        auto t1 = std::chrono::steady_clock::now();
        double ms =
            std::chrono::duration<double, std::milli>(t1 - t0).count();

        return {std::move(m), std::move(edges), ms, std::string{}}; });

    auto *watcher = new QFutureWatcher<ResultT>(this);

    connect(watcher, &QFutureWatcher<ResultT>::finished, this,
            [this, watcher]()
            {
              auto [m, edges, ms, err] = watcher->future().result();
              watcher->deleteLater();

              if (!err.empty())
              {
                emit Failed(QString::fromStdString(err));
                return;
              }

              model_ = std::move(m);
              emit Loaded(&model_, std::move(edges), ms);
            });

    watcher->setFuture(future);
  }

  void Controller::ApplyTranslate(double dx, double dy, double dz)
  {
    model_.Translate(dx, dy, dz);
    std::vector<uint32_t> edges;
    model_.BuildEdges(edges);
    emit Updated(&model_, std::move(edges));
  }

  void Controller::ApplyScale(double k)
  {
    model_.Scale(k);
    std::vector<uint32_t> edges;
    model_.BuildEdges(edges);
    emit Updated(&model_, std::move(edges));
  }

  void Controller::ApplyRotateX(double deg)
  {
    model_.RotateX(deg);
    std::vector<uint32_t> edges;
    model_.BuildEdges(edges);
    emit Updated(&model_, std::move(edges));
  }

  void Controller::ApplyRotateY(double deg)
  {
    model_.RotateY(deg);
    std::vector<uint32_t> edges;
    model_.BuildEdges(edges);
    emit Updated(&model_, std::move(edges));
  }

  void Controller::ApplyRotateZ(double deg)
  {
    model_.RotateZ(deg);
    std::vector<uint32_t> edges;
    model_.BuildEdges(edges);
    emit Updated(&model_, std::move(edges));
  }

} // namespace s21
