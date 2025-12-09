#ifndef S21_RENDER_SETTINGS_H
#define S21_RENDER_SETTINGS_H

#include <QColor>
#include <QSettings>

namespace s21
{

  struct RenderSettings
  {
    QColor background{23, 23, 28};
    QColor edgeColor{255, 255, 255};
    float edgeWidth{1.5F};
    int projectionType{0};
    int edgeType{0};
    int vertexType{1};
    float vertexSize{4.0F};
    QColor vertexColor{255, 255, 255};

    void Save(QSettings &st) const;
    void Load(QSettings &st);
  };

} // namespace s21

#endif // S21_RENDER_SETTINGS_H
