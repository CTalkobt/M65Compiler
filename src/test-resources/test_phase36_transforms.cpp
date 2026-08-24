/* test_phase36_transforms.cpp — Sprite Transform Tests
 *
 * Tests 2D affine transformations for sprites.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cmath>

extern "C" {
#include "../../lib/include/sprite_transform.h"
#include "../../lib/include/sprites.h"
}

#define EPSILON 0.001f

class SpriteTransformTest : public ::testing::Test {
protected:
    void SetUp() override {
        sprite_init();
    }
};

/* ============================================================================
 * SCALE TESTS
 * ========================================================================== */

TEST_F(SpriteTransformTest, ScaleUniform) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale_uniform(s, 2.0f);

    float sx, sy;
    sprite_get_scale(s, &sx, &sy);
    EXPECT_NEAR(sx, 2.0f, EPSILON);
    EXPECT_NEAR(sy, 2.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, ScaleNonUniform) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 1.5f, 3.0f);

    float sx, sy;
    sprite_get_scale(s, &sx, &sy);
    EXPECT_NEAR(sx, 1.5f, EPSILON);
    EXPECT_NEAR(sy, 3.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, ScaleHalf) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 0.5f, 0.5f);

    float sx, sy;
    sprite_get_scale(s, &sx, &sy);
    EXPECT_NEAR(sx, 0.5f, EPSILON);
    EXPECT_NEAR(sy, 0.5f, EPSILON);

    sprite_destroy(s);
}

/* ============================================================================
 * ROTATION TESTS
 * ========================================================================== */

TEST_F(SpriteTransformTest, RotationBasic) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_rotate(s, 45.0f);

    float rot = sprite_get_rotation(s);
    EXPECT_NEAR(rot, 45.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, RotationWrap) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_rotate(s, 450.0f);

    float rot = sprite_get_rotation(s);
    EXPECT_NEAR(rot, 90.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, RotationNegative) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_rotate(s, -90.0f);

    float rot = sprite_get_rotation(s);
    EXPECT_NEAR(rot, 270.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, RotateRelative) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_rotate(s, 30.0f);
    sprite_rotate_relative(s, 15.0f);

    float rot = sprite_get_rotation(s);
    EXPECT_NEAR(rot, 45.0f, EPSILON);

    sprite_destroy(s);
}

/* ============================================================================
 * SKEW TESTS
 * ========================================================================== */

TEST_F(SpriteTransformTest, SkewBasic) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_skew(s, 15.0f, 0.0f);

    float sx, sy;
    sprite_get_skew(s, &sx, &sy);
    EXPECT_NEAR(sx, 15.0f, EPSILON);
    EXPECT_NEAR(sy, 0.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, SkewBoth) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_skew(s, 10.0f, 20.0f);

    float sx, sy;
    sprite_get_skew(s, &sx, &sy);
    EXPECT_NEAR(sx, 10.0f, EPSILON);
    EXPECT_NEAR(sy, 20.0f, EPSILON);

    sprite_destroy(s);
}

/* ============================================================================
 * TRANSFORM ORIGIN TESTS
 * ========================================================================== */

TEST_F(SpriteTransformTest, TransformOrigin) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_set_transform_origin(s, 16, 16);

    int ox, oy;
    sprite_get_transform_origin(s, &ox, &oy);
    EXPECT_EQ(ox, 16);
    EXPECT_EQ(oy, 16);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, TransformOriginCentered) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_set_transform_origin_centered(s);

    int ox, oy;
    sprite_get_transform_origin(s, &ox, &oy);
    EXPECT_EQ(ox, 16);
    EXPECT_EQ(oy, 16);

    sprite_destroy(s);
}

/* ============================================================================
 * MATRIX OPERATIONS
 * ========================================================================== */

TEST_F(SpriteTransformTest, ResetTransform) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 2.0f, 2.0f);
    sprite_rotate(s, 45.0f);
    sprite_reset_transform(s);

    float sx, sy;
    sprite_get_scale(s, &sx, &sy);
    EXPECT_NEAR(sx, 1.0f, EPSILON);
    EXPECT_NEAR(sy, 1.0f, EPSILON);

    float rot = sprite_get_rotation(s);
    EXPECT_NEAR(rot, 0.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, GetTransformMatrix) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 2.0f, 2.0f);

    sprite_matrix_t m;
    int result = sprite_get_transform_matrix(s, &m);
    EXPECT_EQ(result, 1);

    EXPECT_NEAR(m.a, 2.0f, EPSILON);
    EXPECT_NEAR(m.d, 2.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, ComposeTransforms) {
    sprite_matrix_t m1 = {2.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f};
    sprite_matrix_t m2 = {0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f};
    sprite_matrix_t result;

    sprite_compose_transforms(&m1, &m2, &result);

    EXPECT_NEAR(result.a, 1.0f, EPSILON);
    EXPECT_NEAR(result.d, 1.0f, EPSILON);
}

TEST_F(SpriteTransformTest, InvertTransform) {
    sprite_matrix_t m = {2.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f};
    sprite_matrix_t inv;

    int result = sprite_invert_transform(&m, &inv);
    EXPECT_EQ(result, 1);

    EXPECT_NEAR(inv.a, 0.5f, EPSILON);
    EXPECT_NEAR(inv.d, 1.0f/3.0f, EPSILON);
}

TEST_F(SpriteTransformTest, InvertSingular) {
    sprite_matrix_t m = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    sprite_matrix_t inv;

    int result = sprite_invert_transform(&m, &inv);
    EXPECT_EQ(result, 0);
}

/* ============================================================================
 * COORDINATE TRANSFORMATION
 * ========================================================================== */

TEST_F(SpriteTransformTest, TransformPoint) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 2.0f, 2.0f);

    float wx, wy;
    sprite_transform_point(s, 10.0f, 10.0f, &wx, &wy);

    EXPECT_NEAR(wx, 120.0f, EPSILON);
    EXPECT_NEAR(wy, 120.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, TransformPointWithOrigin) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_set_transform_origin(s, 16, 16);
    sprite_scale(s, 2.0f, 2.0f);

    float wx, wy;
    sprite_transform_point(s, 16.0f, 16.0f, &wx, &wy);

    EXPECT_NEAR(wx, 100.0f, EPSILON);
    EXPECT_NEAR(wy, 100.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, InverseTransformPoint) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 2.0f, 2.0f);

    float lx, ly;
    int result = sprite_inverse_transform_point(s, 120.0f, 120.0f, &lx, &ly);

    EXPECT_EQ(result, 1);
    EXPECT_NEAR(lx, 10.0f, EPSILON);
    EXPECT_NEAR(ly, 10.0f, EPSILON);

    sprite_destroy(s);
}

/* ============================================================================
 * ANIMATION/INTERPOLATION TESTS
 * ========================================================================== */

TEST_F(SpriteTransformTest, LerpTransform) {
    sprite_transform_t src = {1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0, 0};
    sprite_transform_t dst = {2.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0, 0};
    sprite_transform_t result;

    sprite_lerp_transform(&src, &dst, 0.5f, &result);

    EXPECT_NEAR(result.scale_x, 1.5f, EPSILON);
    EXPECT_NEAR(result.scale_y, 1.5f, EPSILON);
}

TEST_F(SpriteTransformTest, AnimateTransform) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 1.0f, 1.0f);

    int animating = sprite_animate_transform(s, 2.0f, 2.0f, 0.0f, 0.1f);
    EXPECT_EQ(animating, 1);

    float sx, sy;
    sprite_get_scale(s, &sx, &sy);
    EXPECT_GT(sx, 1.0f);
    EXPECT_LT(sx, 2.0f);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, AnimateTransformComplete) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 2.0f, 2.0f);

    int animating = sprite_animate_transform(s, 2.0f, 2.0f, 0.0f, 0.1f);
    EXPECT_EQ(animating, 0);

    sprite_destroy(s);
}

/* ============================================================================
 * COLLISION WITH TRANSFORMS
 * ========================================================================== */

TEST_F(SpriteTransformTest, CollidesPointBasic) {
    sprite_t s = sprite_create(100, 100, 32, 32);

    int hit = sprite_collides_point_transformed(s, 110, 110);
    EXPECT_EQ(hit, 1);

    int miss = sprite_collides_point_transformed(s, 50, 50);
    EXPECT_EQ(miss, 0);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, CollidesPointScaled) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 2.0f, 2.0f);

    int hit = sprite_collides_point_transformed(s, 132, 132);
    EXPECT_EQ(hit, 1);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, GetBoundsTransformed) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_scale(s, 2.0f, 2.0f);

    int x1, y1, x2, y2;
    int result = sprite_get_bounds_transformed(s, &x1, &y1, &x2, &y2);

    EXPECT_EQ(result, 1);
    EXPECT_LT(x1, 100);
    EXPECT_GT(x2, 164);

    sprite_destroy(s);
}

/* ============================================================================
 * COMBINED TRANSFORM TESTS
 * ========================================================================== */

TEST_F(SpriteTransformTest, ScaleRotateCombined) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_set_transform_origin_centered(s);
    sprite_scale(s, 1.5f, 1.5f);
    sprite_rotate(s, 90.0f);

    float sx, sy;
    sprite_get_scale(s, &sx, &sy);
    EXPECT_NEAR(sx, 1.5f, EPSILON);

    float rot = sprite_get_rotation(s);
    EXPECT_NEAR(rot, 90.0f, EPSILON);

    sprite_destroy(s);
}

TEST_F(SpriteTransformTest, AllTransformsCombined) {
    sprite_t s = sprite_create(100, 100, 32, 32);
    sprite_set_transform_origin_centered(s);
    sprite_scale(s, 1.5f, 1.5f);
    sprite_rotate(s, 45.0f);
    sprite_skew(s, 10.0f, 0.0f);

    sprite_matrix_t m;
    int result = sprite_get_transform_matrix(s, &m);
    EXPECT_EQ(result, 1);

    sprite_destroy(s);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
