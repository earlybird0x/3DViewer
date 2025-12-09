// clazy:excludeall=non-pod-global-static
#include <gtest/gtest.h>

#include <vector>

#include "model/obj_model.h"
#include "test_utils.h" // WriteTempObj, LoadModelFromObjString

namespace
{

  TEST(ModelEdges, TriangleHasThreeEdges)
  {
    // минимальный треугольник
    constexpr const char kObj[] =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1 2 3\n";

    s21::Model m;
    std::string err;
    ASSERT_TRUE(LoadModelFromObjString(kObj, m, &err, "tri.obj")) << err;

    // рёбра
    std::vector<uint32_t> edges;
    m.BuildEdges(edges);

    // Для треугольника ожидаем 3 ребра (часто хранятся парами индексов => 6
    // чисел)
    ASSERT_EQ(m.GetNumVertices(), 3u);
    EXPECT_EQ(edges.size(), 6u);
  }

  TEST(ModelAabb, MinMaxCenterSize)
  {
    // Три точки: (-1,-2,-3), (2,3,4), (0,0,0)
    constexpr const char kObj[] =
        "v -1 -2 -3\n"
        "v 2 3 4\n"
        "v 0 0 0\n";

    s21::Model m;
    std::string err;
    ASSERT_TRUE(LoadModelFromObjString(kObj, m, &err, "aabb.obj")) << err;

    auto aabb = m.ComputeAabb();
    EXPECT_NEAR(aabb.min.x, -1.0, 1e-6);
    EXPECT_NEAR(aabb.min.y, -2.0, 1e-6);
    EXPECT_NEAR(aabb.min.z, -3.0, 1e-6);
    EXPECT_NEAR(aabb.max.x, 2.0, 1e-6);
    EXPECT_NEAR(aabb.max.y, 3.0, 1e-6);
    EXPECT_NEAR(aabb.max.z, 4.0, 1e-6);

    auto c = aabb.center();
    EXPECT_NEAR(c.x, 0.5, 1e-6);
    EXPECT_NEAR(c.y, 0.5, 1e-6);
    EXPECT_NEAR(c.z, 0.5, 1e-6);

    auto s = aabb.size();
    EXPECT_NEAR(s.x, 3.0, 1e-6);
    EXPECT_NEAR(s.y, 5.0, 1e-6);
    EXPECT_NEAR(s.z, 7.0, 1e-6);
  }

  TEST(ModelEdges, NoFacesHasNoEdges)
  {
    const char *kObj =
        "v 0 0 0\n"
        "v 1 1 1\n";
    s21::Model m;
    std::string err;
    ASSERT_TRUE(LoadModelFromObjString(kObj, m, &err, "noface.obj")) << err;

    std::vector<uint32_t> edges;
    m.BuildEdges(edges);
    EXPECT_TRUE(edges.empty());
  }

} // namespace
