/* test_phase39_blend_spaces.cpp — Blend Spaces Tests
 *
 * Tests 2D parameter-based animation blending system.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <cmath>

extern "C" {
#include "../../lib/include/sprite_blend_spaces.h"
#include "../../lib/include/sprite_animation_trees.h"
#include "../../lib/include/sprites.h"
}

class BlendSpaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        sprite_init();
    }
};

/* ============================================================================
 * BLEND SPACE CREATION TESTS
 * ========================================================================== */

TEST_F(BlendSpaceTest, Create1DBlendSpace) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 1);

    ASSERT_NE(space, INVALID_BLEND_SPACE);

    sprite_blend_space_info_t info;
    EXPECT_EQ(sprite_blend_space_get_info(space, &info), 1);
    EXPECT_EQ(info.dimension_count, 1);
    EXPECT_EQ(info.point_count, 0);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, Create2DBlendSpace) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    ASSERT_NE(space, INVALID_BLEND_SPACE);

    sprite_blend_space_info_t info;
    EXPECT_EQ(sprite_blend_space_get_info(space, &info), 1);
    EXPECT_EQ(info.dimension_count, 2);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, Create3DBlendSpace) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 3);

    ASSERT_NE(space, INVALID_BLEND_SPACE);

    sprite_blend_space_info_t info;
    EXPECT_EQ(sprite_blend_space_get_info(space, &info), 1);
    EXPECT_EQ(info.dimension_count, 3);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * BLEND MODE TESTS
 * ========================================================================== */

TEST_F(BlendSpaceTest, SetBlendMode) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_space_set_mode(space, BLEND_MODE_RADIAL);
    EXPECT_EQ(sprite_blend_space_get_mode(space), BLEND_MODE_RADIAL);

    sprite_blend_space_set_mode(space, BLEND_MODE_CARTESIAN);
    EXPECT_EQ(sprite_blend_space_get_mode(space), BLEND_MODE_CARTESIAN);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * BLEND POINT MANAGEMENT TESTS
 * ========================================================================== */

TEST_F(BlendSpaceTest, AddBlendPoint) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_point_t point = sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, "idle");
    ASSERT_NE(point, INVALID_BLEND_POINT);

    EXPECT_EQ(sprite_blend_space_get_point_count(space), 1);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, AddMultiplePoints) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, "idle");
    sprite_blend_space_add_point(space, 1.0f, 0.0f, 0.0f, "forward");
    sprite_blend_space_add_point(space, 1.0f, 180.0f, 0.0f, "backward");

    EXPECT_EQ(sprite_blend_space_get_point_count(space), 3);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, RemovePoint) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_point_t p1 = sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, "idle");
    sprite_blend_space_add_point(space, 1.0f, 0.0f, 0.0f, "forward");

    EXPECT_EQ(sprite_blend_space_get_point_count(space), 2);

    EXPECT_EQ(sprite_blend_space_remove_point(space, p1), 1);
    EXPECT_EQ(sprite_blend_space_get_point_count(space), 1);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, FindPoint) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, "idle");
    sprite_blend_space_add_point(space, 1.0f, 0.0f, 0.0f, "forward");
    sprite_blend_space_add_point(space, 1.0f, 180.0f, 0.0f, "backward");

    sprite_blend_point_t forward = sprite_blend_space_find_point(space, "forward");
    ASSERT_NE(forward, INVALID_BLEND_POINT);

    sprite_blend_point_t notfound = sprite_blend_space_find_point(space, "nonexistent");
    EXPECT_EQ(notfound, INVALID_BLEND_POINT);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, GetPointInfo) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_space_add_point(space, 2.5f, 45.0f, 0.0f, "run_northeast");

    sprite_blend_point_info_t info;
    EXPECT_EQ(sprite_blend_space_get_point(space, 0, &info), 1);
    EXPECT_NEAR(info.x, 2.5f, 0.001f);
    EXPECT_NEAR(info.y, 45.0f, 0.001f);
    EXPECT_EQ(strcmp(info.state_name, "run_northeast"), 0);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, MovePoint) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_point_t point = sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, "idle");

    EXPECT_EQ(sprite_blend_space_move_point(space, point, 1.0f, 90.0f, 0.0f), 1);

    sprite_blend_point_info_t info;
    sprite_blend_space_get_point(space, 0, &info);
    EXPECT_NEAR(info.x, 1.0f, 0.001f);
    EXPECT_NEAR(info.y, 90.0f, 0.001f);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * PARAMETER CONTROL TESTS
 * ========================================================================== */

TEST_F(BlendSpaceTest, SetGetParameters) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    EXPECT_EQ(sprite_blend_space_set_parameters(space, 1.5f, 45.0f, 0.0f), 1);

    float x, y, z;
    sprite_blend_space_get_parameters(space, &x, &y, &z);
    EXPECT_NEAR(x, 1.5f, 0.001f);
    EXPECT_NEAR(y, 45.0f, 0.001f);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, SetGetParametersIndividual) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 3);

    sprite_blend_space_set_parameter_x(space, 2.0f);
    sprite_blend_space_set_parameter_y(space, 45.0f);
    sprite_blend_space_set_parameter_z(space, 1.0f);

    EXPECT_NEAR(sprite_blend_space_get_parameter_x(space), 2.0f, 0.001f);
    EXPECT_NEAR(sprite_blend_space_get_parameter_y(space), 45.0f, 0.001f);
    EXPECT_NEAR(sprite_blend_space_get_parameter_z(space), 1.0f, 0.001f);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * DISTANCE & NEAREST POINT TESTS
 * ========================================================================== */

TEST_F(BlendSpaceTest, DistanceToPoint) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_point_t point = sprite_blend_space_add_point(space, 1.0f, 0.0f, 0.0f, "idle");

    sprite_blend_space_set_parameters(space, 0.0f, 0.0f, 0.0f);
    float dist = sprite_blend_space_distance_to_point(space, point);
    EXPECT_NEAR(dist, 1.0f, 0.001f);

    sprite_blend_space_set_parameters(space, 1.0f, 0.0f, 0.0f);
    dist = sprite_blend_space_distance_to_point(space, point);
    EXPECT_NEAR(dist, 0.0f, 0.001f);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, GetNearestPoint) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, "idle");
    sprite_blend_space_add_point(space, 1.0f, 0.0f, 0.0f, "forward");
    sprite_blend_space_add_point(space, 1.0f, 180.0f, 0.0f, "backward");

    sprite_blend_space_set_parameters(space, 1.0f, 10.0f, 0.0f);
    sprite_blend_point_t nearest = sprite_blend_space_get_nearest_point(space);
    ASSERT_NE(nearest, INVALID_BLEND_POINT);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * BLEND WEIGHTS TESTS
 * ========================================================================== */

TEST_F(BlendSpaceTest, GetBlendWeights) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);

    sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, "idle");
    sprite_blend_space_add_point(space, 1.0f, 0.0f, 0.0f, "forward");

    sprite_blend_space_set_parameters(space, 0.5f, 0.0f, 0.0f);

    float weights[2];
    int count = sprite_blend_space_get_blend_weights(space, weights, 2);
    EXPECT_EQ(count, 2);

    float total = weights[0] + weights[1];
    EXPECT_NEAR(total, 1.0f, 0.001f);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * CONVENIENCE PATTERN TESTS
 * ========================================================================== */

TEST_F(BlendSpaceTest, Create1DSpeed) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    sprite_blend_space_t space = sprite_blend_space_create_1d_speed(
        tree, 2.0f, 5.0f, "idle", "walk", "run");

    ASSERT_NE(space, INVALID_BLEND_SPACE);
    EXPECT_EQ(sprite_blend_space_get_point_count(space), 3);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(BlendSpaceTest, Create2DMovement) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    sprite_blend_space_t space = sprite_blend_space_create_2d_movement(
        tree, "forward", "backward", "left", "right", "idle");

    ASSERT_NE(space, INVALID_BLEND_SPACE);
    EXPECT_EQ(sprite_blend_space_get_point_count(space), 5);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * ACTIVE STATE TESTS
 * ========================================================================== */

TEST_F(BlendSpaceTest, GetActiveState) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_blend_space_t space = sprite_blend_space_create(tree, 1);

    sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, "idle");
    sprite_blend_space_add_point(space, 5.0f, 0.0f, 0.0f, "run");

    sprite_blend_space_set_parameters(space, 0.0f, 0.0f, 0.0f);
    char *active = sprite_blend_space_get_active_state(space);
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(strcmp(active, "idle"), 0);
    free(active);

    sprite_blend_space_destroy(space);
    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
