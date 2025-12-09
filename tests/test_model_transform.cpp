// clazy:excludeall=non-pod-global-static
#include <gtest/gtest.h>

#include <string>

#include "model/obj_model.h"
#include "test_utils.h"

namespace {

constexpr double kEps = 1e-5;

TEST(ModelTransform, Translate) {
  constexpr const char kObj[] = "v 0 0 0\n";
  s21::Model m;
  std::string err;
  ASSERT_TRUE(LoadModelFromObjString(kObj, m, &err, "tr.obj")) << err;

  m.Translate(1.0, 2.0, 3.0);

  auto aabb = m.ComputeAabb();
  EXPECT_NEAR(aabb.min.x, 1.0, kEps);
  EXPECT_NEAR(aabb.min.y, 2.0, kEps);
  EXPECT_NEAR(aabb.min.z, 3.0, kEps);
  EXPECT_NEAR(aabb.max.x, 1.0, kEps);
  EXPECT_NEAR(aabb.max.y, 2.0, kEps);
  EXPECT_NEAR(aabb.max.z, 3.0, kEps);
}

TEST(ModelTransform, Scale) {
  constexpr const char kObj[] =
      "v 1 2 3\n"
      "v -1 -2 -3\n";
  s21::Model m;
  std::string err;
  ASSERT_TRUE(LoadModelFromObjString(kObj, m, &err, "sc.obj")) << err;

  m.Scale(2.0);

  auto aabb = m.ComputeAabb();
  EXPECT_NEAR(aabb.min.x, -2.0, kEps);
  EXPECT_NEAR(aabb.min.y, -4.0, kEps);
  EXPECT_NEAR(aabb.min.z, -6.0, kEps);
  EXPECT_NEAR(aabb.max.x, 2.0, kEps);
  EXPECT_NEAR(aabb.max.y, 4.0, kEps);
  EXPECT_NEAR(aabb.max.z, 6.0, kEps);
}

TEST(ModelTransform, RotateZ_90deg) {
  // центр уже (0,0,0) — две точки симметрично
  constexpr const char kObj[] =
      "v 1 0 0\n"
      "v -1 0 0\n";
  s21::Model m;
  std::string err;
  ASSERT_TRUE(LoadModelFromObjString(kObj, m, &err, "rz2.obj")) << err;

  m.RotateZ(90.0);

  auto aabb = m.ComputeAabb();
  // ожидание: (0, 1, 0) и (0, -1, 0)
  EXPECT_NEAR(aabb.min.x, 0.0, 1e-4);
  EXPECT_NEAR(aabb.max.x, 0.0, 1e-4);
  EXPECT_NEAR(aabb.min.y, -1.0, 1e-4);
  EXPECT_NEAR(aabb.max.y, 1.0, 1e-4);
  EXPECT_NEAR(aabb.min.z, 0.0, 1e-4);
  EXPECT_NEAR(aabb.max.z, 0.0, 1e-4);
}

// ... внизу после RotateZ_90deg

// Scale(1) ничего не меняет
TEST(ModelTransform, ScaleByOneDoesNothing) {
  constexpr const char kObj[] =
      "v -1 -1 -1\n"
      "v  1  1  1\n";
  s21::Model m;
  std::string err;
  ASSERT_TRUE(LoadModelFromObjString(kObj, m, &err, "scale1.obj")) << err;

  auto before = m.ComputeAabb();
  m.Scale(1.0);
  auto after = m.ComputeAabb();

  EXPECT_DOUBLE_EQ(before.min.x, after.min.x);
  EXPECT_DOUBLE_EQ(before.max.x, after.max.x);
  EXPECT_DOUBLE_EQ(before.min.y, after.min.y);
  EXPECT_DOUBLE_EQ(before.max.y, after.max.y);
  EXPECT_DOUBLE_EQ(before.min.z, after.min.z);
  EXPECT_DOUBLE_EQ(before.max.z, after.max.z);
}

// Translate(0,0,0) ничего не меняет
TEST(ModelTransform, TranslateZeroDoesNothing) {
  constexpr const char kObj[] = "v 5 5 5\n";
  s21::Model m;
  std::string err;
  ASSERT_TRUE(LoadModelFromObjString(kObj, m, &err, "tr0.obj")) << err;

  auto before = m.ComputeAabb();
  m.Translate(0.0, 0.0, 0.0);
  auto after = m.ComputeAabb();

  EXPECT_EQ(before.min.x, after.min.x);
  EXPECT_EQ(before.min.y, after.min.y);
  EXPECT_EQ(before.min.z, after.min.z);
  EXPECT_EQ(before.max.x, after.max.x);
  EXPECT_EQ(before.max.y, after.max.y);
  EXPECT_EQ(before.max.z, after.max.z);
}

}  // namespace
