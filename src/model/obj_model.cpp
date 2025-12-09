#include "model/obj_model.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace s21
{

  void Model::BuildEdges(std::vector<uint32_t> &out_edges) const
  {
    out_edges.clear();
    for (const auto &poly : polygons_)
    {
      const auto &idx = poly.points_indices;
      if (idx.size() < 2)
        continue;

      for (size_t i = 0; i < idx.size(); ++i)
      {
        uint32_t a = static_cast<uint32_t>(idx[i]);
        uint32_t b = static_cast<uint32_t>(idx[(i + 1) % idx.size()]);
        out_edges.push_back(a);
        out_edges.push_back(b);
      }
    }
  }

  Model::Aabb Model::ComputeAabb() const
  {
    Model::Aabb box{};
    if (vertices_.empty())
    {
      box.min = box.max = Vertex{0, 0, 0};
      return box;
    }

    box.min = box.max = vertices_.front();

    for (const auto &v : vertices_)
    {
      if (v.x < box.min.x)
        box.min.x = v.x;
      if (v.y < box.min.y)
        box.min.y = v.y;
      if (v.z < box.min.z)
        box.min.z = v.z;

      if (v.x > box.max.x)
        box.max.x = v.x;
      if (v.y > box.max.y)
        box.max.y = v.y;
      if (v.z > box.max.z)
        box.max.z = v.z;
    }

    return box;
  }

  void Model::Translate(double dx, double dy, double dz)
  {
    for (auto &v : vertices_)
    {
      v.x += dx;
      v.y += dy;
      v.z += dz;
    }
  }

  void Model::Scale(double k)
  {
    if (vertices_.empty())
      return;

    const auto c = ComputeAabb().center();
    for (auto &v : vertices_)
    {
      v.x = (v.x - c.x) * k + c.x;
      v.y = (v.y - c.y) * k + c.y;
      v.z = (v.z - c.z) * k + c.z;
    }
  }

  static inline double rad(double deg)
  {
    return deg * M_PI / 180.0;
  }

  void Model::RotateX(double deg)
  {
    if (vertices_.empty())
      return;

    const auto c = ComputeAabb().center();
    const double s = std::sin(rad(deg));
    const double cs = std::cos(rad(deg));

    for (auto &v : vertices_)
    {
      double y = v.y - c.y;
      double z = v.z - c.z;

      double y2 = y * cs - z * s;
      double z2 = y * s + z * cs;

      v.y = y2 + c.y;
      v.z = z2 + c.z;
    }
  }

  void Model::RotateY(double deg)
  {
    if (vertices_.empty())
      return;

    const auto c = ComputeAabb().center();
    const double s = std::sin(rad(deg));
    const double cs = std::cos(rad(deg));

    for (auto &v : vertices_)
    {
      double x = v.x - c.x;
      double z = v.z - c.z;

      double x2 = x * cs + z * s;
      double z2 = -x * s + z * cs;

      v.x = x2 + c.x;
      v.z = z2 + c.z;
    }
  }

  void Model::RotateZ(double deg)
  {
    if (vertices_.empty())
      return;

    const auto c = ComputeAabb().center();
    const double s = std::sin(rad(deg));
    const double cs = std::cos(rad(deg));

    for (auto &v : vertices_)
    {
      double x = v.x - c.x;
      double y = v.y - c.y;

      double x2 = x * cs - y * s;
      double y2 = x * s + y * cs;

      v.x = x2 + c.x;
      v.y = y2 + c.y;
    }
  }

} // namespace s21
