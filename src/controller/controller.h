#ifndef S21_CONTROLLER_H
#define S21_CONTROLLER_H

#include <QObject>
#include <QString>
#include <cstdint>
#include <vector>

#include "model/obj_model.h"
#include "model/obj_parser.h"

namespace s21
{

  class Controller : public QObject
  {
    Q_OBJECT

  public:
    explicit Controller(QObject *parent = nullptr);

    bool LoadFromFile(const QString &path, QString *error = nullptr);
    void LoadAsync(const QString &path);

    void ApplyTranslate(double dx, double dy, double dz);
    void ApplyScale(double k);
    void ApplyRotateX(double deg);
    void ApplyRotateY(double deg);
    void ApplyRotateZ(double deg);

    const Model *model() const { return &model_; }

  signals:
    void Loaded(const Model *model,
                std::vector<uint32_t> edges,
                double total_ms);
    void Failed(const QString &error);
    void Updated(const Model *model,
                 std::vector<uint32_t> edges);

  private:
    Model model_;
    ObjParser loader_;
  };

} // namespace s21

#endif // S21_CONTROLLER_H
