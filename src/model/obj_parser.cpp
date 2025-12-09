#include "model/obj_parser.h"

#include <QDebug>
#include <QFile>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace s21 {

// --- утилиты парсинга (переносим из старого obj_model.cpp) ---
static inline void trim_left(const char *&p) {
  while (*p == ' ' || *p == '\t' || *p == '\r') ++p;
}

static inline const char *skip_token(const char *p, const char *end) {
  while (p < end && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') ++p;
  return p;
}

static inline void parse_vertex_line_mm(const char *s,
                                        const char * /*line_end*/,
                                        std::vector<Model::Vertex> &out_vs) {
  char *e = nullptr;
  double x = strtod(s, &e);
  s = e;
  trim_left(s);
  double y = strtod(s, &e);
  s = e;
  trim_left(s);
  double z = strtod(s, &e);  // s = e;
  out_vs.emplace_back(x, y, z);
}

static inline void parse_face_line_mm(const char *s, const char *line_end,
                                      std::vector<int> &out_indices,
                                      size_t num_vertices) {
  while (s < line_end) {
    trim_left(s);
    if (s >= line_end) break;
    char *e = nullptr;
    long a = strtol(s, &e, 10);
    if (s == e) {
      s = skip_token(s, line_end);
      continue;
    }
    while (e < line_end && *e != ' ' && *e != '\t' && *e != '\r' && *e != '\n')
      ++e;

    if (a < 0)
      a = static_cast<long>(num_vertices) + 1 + a;  // отрицательные индексы
    if (a >= 1 && a <= static_cast<long>(num_vertices))
      out_indices.push_back(static_cast<int>(a - 1));

    s = e;
  }
}

bool ObjParser::Load(const std::string &filename, s21::Model &out,
                     std::string *err) {
  // Сбрасываем объект (на всякий)
  out.vertices_.clear();
  out.polygons_.clear();
  out.num_vertices_ = 0;
  out.num_edges_ = 0;

  QFile qf(QString::fromStdString(filename));
  if (!qf.open(QIODevice::ReadOnly)) {
    if (err) *err = "Unable to open file: " + filename;
    return false;
  }
  const qint64 fsz = qf.size();
  if (fsz <= 0) {
    // пустой файл — считаем успешной загрузкой пустой модели
    qf.close();
    return true;
  }

  uchar *data = qf.map(0, fsz);  // memory-mapped файл
  if (!data) {
    // fallback: обычное чтение
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
      if (err) *err = "Unable to open file (fallback): " + filename;
      return false;
    }

    // Грубые резервирования
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    in.seekg(0, std::ios::beg);
    const size_t approx_vertices =
        (size > 0) ? static_cast<size_t>(size) / 40 : 0;
    out.vertices_.reserve(approx_vertices);
    out.polygons_.reserve(approx_vertices / 2);

    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() || line[0] == '#') continue;
      if (line.size() >= 2 && line[0] == 'v' && line[1] == ' ') {
        const char *s = line.c_str() + 2;
        parse_vertex_line_mm(s, s + line.size(), out.vertices_);
      } else if (line.size() >= 2 && line[0] == 'f' && line[1] == ' ') {
        const char *s = line.c_str() + 2;
        Model::Polygon poly;
        poly.points_indices.reserve(8);
        parse_face_line_mm(s, s + line.size(), poly.points_indices,
                           out.vertices_.size());
        if (!poly.points_indices.empty())
          out.polygons_.push_back(std::move(poly));
      }
    }
    out.num_vertices_ = static_cast<int>(out.vertices_.size());
    std::vector<uint32_t> tmp_edges;
    out.BuildEdges(tmp_edges);
    out.num_edges_ = static_cast<int>(tmp_edges.size() / 2);
    return true;
  }

  const char *begin = reinterpret_cast<const char *>(data);
  const char *end = begin + fsz;

  // 1-й проход: точные reserve
  size_t v_cnt = 0, f_cnt = 0;
  for (const char *p = begin; p < end;) {
    const char *nl = static_cast<const char *>(memchr(p, '\n', end - p));
    const char *line_end = nl ? nl : end;
    if (line_end - p >= 2) {
      const char c0 = p[0], c1 = p[1];
      if (c0 == 'v' && c1 == ' ')
        ++v_cnt;
      else if (c0 == 'f' && c1 == ' ')
        ++f_cnt;
    }
    p = nl ? nl + 1 : end;
  }
  out.vertices_.reserve(v_cnt);
  out.polygons_.reserve(f_cnt);

  // 2-й проход: парсим
  for (const char *p = begin; p < end;) {
    const char *nl = static_cast<const char *>(memchr(p, '\n', end - p));
    const char *line_end = nl ? nl : end;

    if (line_end - p >= 2) {
      if (p[0] == 'v' && p[1] == ' ') {
        parse_vertex_line_mm(p + 2, line_end, out.vertices_);
      } else if (p[0] == 'f' && p[1] == ' ') {
        Model::Polygon poly;
        poly.points_indices.reserve(8);
        parse_face_line_mm(p + 2, line_end, poly.points_indices,
                           out.vertices_.size());
        if (!poly.points_indices.empty())
          out.polygons_.push_back(std::move(poly));
      }
    }
    p = nl ? nl + 1 : end;
  }

  qf.unmap(data);
  qf.close();

  out.num_vertices_ = static_cast<int>(out.vertices_.size());
  std::vector<uint32_t> tmp_edges;
  out.BuildEdges(tmp_edges);
  out.num_edges_ = static_cast<int>(tmp_edges.size() / 2);

  return true;
}

}  // namespace s21
