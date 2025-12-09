#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "model/obj_model.h"
#include "model/obj_parser.h"

namespace {

std::string WriteTempObj(const std::string &content, const std::string &name) {
  std::ofstream f(name, std::ios::binary | std::ios::trunc);
  f << content;
  return name;
}

TEST(ObjParser, ParseMinimalTriangle) {
  constexpr const char kObj[] =
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 1 2 3\n";

  const std::string path = WriteTempObj(kObj, "min_triangle.obj");

  s21::Model model;
  std::string err;
  s21::ObjParser parser;

  bool ok = parser.Load(path, model, &err);
  ASSERT_TRUE(ok) << "Parser failed: " << err;

  EXPECT_EQ(model.GetNumVertices(), 3u);
  ASSERT_EQ(model.GetPolygons().size(), 1u);
  EXPECT_EQ(model.GetPolygons()[0].count_of_vertices(), 3u);

  std::vector<uint32_t> edges;
  model.BuildEdges(edges);
  EXPECT_GE(edges.size(), 6u);
}

TEST(ObjParser, NonexistentFileFails) {
  // крайне маловероятный путь — точно не существует
  const std::string path = "__no_such_dir__/__no_such_file__.obj";

  s21::Model model;
  std::string err;
  s21::ObjParser parser;

  bool ok = parser.Load(path, model, &err);
  EXPECT_FALSE(ok) << "Load() must fail for nonexistent file";

  EXPECT_EQ(model.GetNumVertices(), 0u);
  EXPECT_EQ(model.GetPolygons().size(), 0u);
}

TEST(ObjParser, EmptyFile) {
  const std::string path = WriteTempObj("", "empty.obj");

  s21::Model model;
  std::string err;
  s21::ObjParser parser;

  bool ok = parser.Load(path, model, &err);
  EXPECT_TRUE(ok);  // пустой .obj считаем валидным
  EXPECT_EQ(model.GetNumVertices(), 0u);  // модель пустая
  EXPECT_TRUE(model.GetPolygons().empty());
}

}  // namespace
