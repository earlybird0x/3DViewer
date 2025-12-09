#ifndef S21_VIEW_PROJECTION_H
#define S21_VIEW_PROJECTION_H

#include <QMatrix4x4>

namespace s21
{

  class IProjection
  {
  public:
    virtual ~IProjection() = default;

    virtual QMatrix4x4 Make(float aspect) const = 0;
  };

  class PerspectiveProjection : public IProjection
  {
  public:
    explicit PerspectiveProjection(float fov_deg = 45.0F,
                                   float z_near = 0.01F,
                                   float z_far = 100.0F)
        : fov_(fov_deg), zn_(z_near), zf_(z_far) {}

    QMatrix4x4 Make(float aspect) const override;

  private:
    float fov_;
    float zn_;
    float zf_;
  };

  class OrthoProjection : public IProjection
  {
  public:
    explicit OrthoProjection(float scale = 1.0F,
                             float z_near = -100.0F,
                             float z_far = 100.0F)
        : scale_(scale), zn_(z_near), zf_(z_far) {}

    QMatrix4x4 Make(float aspect) const override;

    void SetScale(float scale) { scale_ = scale; }

  private:
    float scale_;
    float zn_;
    float zf_;
  };

} // namespace s21

#endif // S21_VIEW_PROJECTION_H
