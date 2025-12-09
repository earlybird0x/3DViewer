#ifndef OBJ_MODEL_H
#define OBJ_MODEL_H

#include <cmath>
#include <cstdint>
#include <vector>

namespace s21 {

class Model {
 public:
  class Vertex {
   public:
    double x, y, z;
    Vertex(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
  };

  class Polygon {
   public:
    std::vector<int> points_indices;
    Polygon(const std::vector<int> &indices = {}) : points_indices(indices) {}
    int count_of_vertices() const {
      return static_cast<int>(points_indices.size());
    }
  };

  Model() = default;

  // Доступ к данным
  const std::vector<Vertex> &GetVertices() const { return vertices_; }
  const std::vector<Polygon> &GetPolygons() const { return polygons_; }
  int GetNumVertices() const { return num_vertices_; }
  int GetNumEdges() const { return num_edges_; }

  // Геометрия/служебное
  void BuildEdges(std::vector<uint32_t> &out_edges) const;

 public:
  struct Aabb {
    Vertex min, max;
    Vertex center() const {
      return Vertex{(min.x + max.x) / 2.0, (min.y + max.y) / 2.0,
                    (min.z + max.z) / 2.0};
    }
    Vertex size() const {
      return Vertex{(max.x - min.x), (max.y - min.y), (max.z - min.z)};
    }
  };

  Aabb ComputeAabb() const;

  void Translate(double dx, double dy, double dz);
  void Scale(double k);
  void RotateX(double deg);
  void RotateY(double deg);
  void RotateZ(double deg);

 private:
  std::vector<Vertex> vertices_;
  std::vector<Polygon> polygons_;
  int num_vertices_ = 0;
  int num_edges_ = 0;

  friend class ObjParser;
};

}  // namespace s21

#endif  // OBJ_MODEL_H
