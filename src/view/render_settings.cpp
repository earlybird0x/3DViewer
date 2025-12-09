#include "view/render_settings.h"

namespace s21 {

void RenderSettings::Save(QSettings &st) const {
  st.beginGroup("RenderSettings");

  st.setValue("bg_r", background.red());
  st.setValue("bg_g", background.green());
  st.setValue("bg_b", background.blue());

  st.setValue("edge_r", edgeColor.red());
  st.setValue("edge_g", edgeColor.green());
  st.setValue("edge_b", edgeColor.blue());
  st.setValue("edge_w", edgeWidth);
  st.setValue("edge_type", edgeType);

  st.setValue("proj", projectionType);

  st.setValue("v_type", vertexType);
  st.setValue("v_size", vertexSize);
  st.setValue("v_r", vertexColor.red());
  st.setValue("v_g", vertexColor.green());
  st.setValue("v_b", vertexColor.blue());

  st.endGroup();
}

void RenderSettings::Load(QSettings &st) {
  st.beginGroup("RenderSettings");

  background.setRed(st.value("bg_r", background.red()).toInt());
  background.setGreen(st.value("bg_g", background.green()).toInt());
  background.setBlue(st.value("bg_b", background.blue()).toInt());

  edgeColor.setRed(st.value("edge_r", edgeColor.red()).toInt());
  edgeColor.setGreen(st.value("edge_g", edgeColor.green()).toInt());
  edgeColor.setBlue(st.value("edge_b", edgeColor.blue()).toInt());
  edgeWidth = st.value("edge_w", edgeWidth).toFloat();
  edgeType = st.value("edge_type", edgeType).toInt();

  projectionType = st.value("proj", projectionType).toInt();

  vertexType = st.value("v_type", vertexType).toInt();
  vertexSize = st.value("v_size", vertexSize).toFloat();
  vertexColor.setRed(st.value("v_r", vertexColor.red()).toInt());
  vertexColor.setGreen(st.value("v_g", vertexColor.green()).toInt());
  vertexColor.setBlue(st.value("v_b", vertexColor.blue()).toInt());

  st.endGroup();
}

}  // namespace s21
