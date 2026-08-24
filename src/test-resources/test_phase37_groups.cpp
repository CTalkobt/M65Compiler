/* test_phase37_groups.cpp — Sprite Groups & Hierarchies Tests
 *
 * Tests sprite grouping, parent-child relationships, and hierarchical transforms.
 */

#include <gtest/gtest.h>
#include <cstdint>

extern "C" {
#include "../../lib/include/sprite_groups.h"
#include "../../lib/include/sprite_transform.h"
#include "../../lib/include/sprites.h"
}

class SpriteGroupTest : public ::testing::Test {
protected:
    void SetUp() override {
        sprite_init();
    }
};

/* ============================================================================
 * GROUP CREATION TESTS
 * ========================================================================== */

TEST_F(SpriteGroupTest, CreateGroup) {
    sprite_group_t group = sprite_group_create(100, 100);
    ASSERT_NE(group, INVALID_GROUP);

    int x, y;
    EXPECT_EQ(sprite_group_get_position(group, &x, &y), 1);
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, 100);

    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, GetGroupInfo) {
    sprite_group_t group = sprite_group_create(50, 75);
    sprite_group_info_t info;

    EXPECT_EQ(sprite_group_get_info(group, &info), 1);
    EXPECT_EQ(info.x, 50);
    EXPECT_EQ(info.y, 75);
    EXPECT_EQ(info.visible, 1);
    EXPECT_EQ(info.member_count, 0);

    sprite_group_destroy(group);
}

/* ============================================================================
 * GROUP MEMBERSHIP TESTS
 * ========================================================================== */

TEST_F(SpriteGroupTest, AddSprite) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t sprite = sprite_create(50, 50, 32, 32);

    EXPECT_EQ(sprite_group_add_sprite(group, sprite), 1);
    EXPECT_EQ(sprite_group_get_member_count(group), 1);
    EXPECT_EQ(sprite_group_get_member(group, 0), sprite);

    sprite_destroy(sprite);
    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, AddMultipleSprites) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t s1 = sprite_create(0, 0, 32, 32);
    sprite_t s2 = sprite_create(40, 0, 32, 32);
    sprite_t s3 = sprite_create(0, 40, 32, 32);

    sprite_group_add_sprite(group, s1);
    sprite_group_add_sprite(group, s2);
    sprite_group_add_sprite(group, s3);

    EXPECT_EQ(sprite_group_get_member_count(group), 3);

    sprite_destroy(s1);
    sprite_destroy(s2);
    sprite_destroy(s3);
    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, RemoveSprite) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t sprite = sprite_create(50, 50, 32, 32);

    sprite_group_add_sprite(group, sprite);
    EXPECT_EQ(sprite_group_get_member_count(group), 1);

    EXPECT_EQ(sprite_group_remove_sprite(group, sprite), 1);
    EXPECT_EQ(sprite_group_get_member_count(group), 0);

    sprite_destroy(sprite);
    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, ClearGroup) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t s1 = sprite_create(0, 0, 32, 32);
    sprite_t s2 = sprite_create(40, 0, 32, 32);

    sprite_group_add_sprite(group, s1);
    sprite_group_add_sprite(group, s2);

    int cleared = sprite_group_clear(group);
    EXPECT_EQ(cleared, 2);
    EXPECT_EQ(sprite_group_get_member_count(group), 0);

    sprite_destroy(s1);
    sprite_destroy(s2);
    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, IsInGroup) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t sprite = sprite_create(50, 50, 32, 32);
    sprite_t other = sprite_create(100, 100, 32, 32);

    sprite_group_add_sprite(group, sprite);

    EXPECT_EQ(sprite_is_in_group(group, sprite), 1);
    EXPECT_EQ(sprite_is_in_group(group, other), 0);

    sprite_destroy(sprite);
    sprite_destroy(other);
    sprite_group_destroy(group);
}

/* ============================================================================
 * GROUP POSITIONING TESTS
 * ========================================================================== */

TEST_F(SpriteGroupTest, GroupPositioning) {
    sprite_group_t group = sprite_group_create(100, 100);
    sprite_group_set_position(group, 200, 200);

    int x, y;
    sprite_group_get_position(group, &x, &y);
    EXPECT_EQ(x, 200);
    EXPECT_EQ(y, 200);

    sprite_group_move(group, 50, -50);
    sprite_group_get_position(group, &x, &y);
    EXPECT_EQ(x, 250);
    EXPECT_EQ(y, 150);

    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, GroupBounds) {
    sprite_group_t group = sprite_group_create(100, 100);
    sprite_t s1 = sprite_create(0, 0, 32, 32);
    sprite_t s2 = sprite_create(40, 40, 32, 32);

    sprite_group_add_sprite(group, s1);
    sprite_group_add_sprite(group, s2);

    int x1, y1, x2, y2;
    EXPECT_EQ(sprite_group_get_bounds(group, &x1, &y1, &x2, &y2), 1);

    sprite_destroy(s1);
    sprite_destroy(s2);
    sprite_group_destroy(group);
}

/* ============================================================================
 * VISIBILITY & ATTRIBUTES TESTS
 * ========================================================================== */

TEST_F(SpriteGroupTest, GroupVisibility) {
    sprite_group_t group = sprite_group_create(0, 0);

    EXPECT_EQ(sprite_group_is_visible(group), 1);

    sprite_group_set_visible(group, 0);
    EXPECT_EQ(sprite_group_is_visible(group), 0);

    sprite_group_set_visible(group, 1);
    EXPECT_EQ(sprite_group_is_visible(group), 1);

    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, GroupLayer) {
    sprite_group_t group = sprite_group_create(0, 0);

    sprite_group_set_layer(group, 50);
    EXPECT_EQ(sprite_group_get_layer(group), 50);

    sprite_group_set_layer(group, 200);
    EXPECT_EQ(sprite_group_get_layer(group), 200);

    sprite_group_destroy(group);
}

/* ============================================================================
 * GROUP TRANSFORM TESTS
 * ========================================================================== */

TEST_F(SpriteGroupTest, GroupScale) {
    sprite_group_t group = sprite_group_create(0, 0);

    sprite_group_scale(group, 2.0f, 2.0f);

    float sx, sy;
    sprite_group_get_scale(group, &sx, &sy);
    EXPECT_NEAR(sx, 2.0f, 0.001f);
    EXPECT_NEAR(sy, 2.0f, 0.001f);

    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, GroupScaleUniform) {
    sprite_group_t group = sprite_group_create(0, 0);

    sprite_group_scale_uniform(group, 1.5f);

    float sx, sy;
    sprite_group_get_scale(group, &sx, &sy);
    EXPECT_NEAR(sx, 1.5f, 0.001f);
    EXPECT_NEAR(sy, 1.5f, 0.001f);

    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, GroupRotation) {
    sprite_group_t group = sprite_group_create(0, 0);

    sprite_group_rotate(group, 45.0f);
    EXPECT_NEAR(sprite_group_get_rotation(group), 45.0f, 0.001f);

    sprite_group_rotate_relative(group, 45.0f);
    EXPECT_NEAR(sprite_group_get_rotation(group), 90.0f, 0.001f);

    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, GroupResetTransform) {
    sprite_group_t group = sprite_group_create(0, 0);

    sprite_group_scale(group, 2.0f, 2.0f);
    sprite_group_rotate(group, 45.0f);

    sprite_group_reset_transform(group);

    float sx, sy;
    sprite_group_get_scale(group, &sx, &sy);
    EXPECT_NEAR(sx, 1.0f, 0.001f);
    EXPECT_NEAR(sy, 1.0f, 0.001f);
    EXPECT_NEAR(sprite_group_get_rotation(group), 0.0f, 0.001f);

    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, GroupTransformOrigin) {
    sprite_group_t group = sprite_group_create(0, 0);

    sprite_group_set_transform_origin(group, 50, 50);

    int x, y;
    sprite_group_get_transform_origin(group, &x, &y);
    EXPECT_EQ(x, 50);
    EXPECT_EQ(y, 50);

    sprite_group_destroy(group);
}

/* ============================================================================
 * PARENT-CHILD RELATIONSHIP TESTS
 * ========================================================================== */

TEST_F(SpriteGroupTest, SetParent) {
    sprite_t parent = sprite_create(0, 0, 32, 32);
    sprite_t child = sprite_create(50, 50, 32, 32);

    EXPECT_EQ(sprite_set_parent(child, parent), 1);
    EXPECT_EQ(sprite_get_parent(child), parent);

    sprite_destroy(parent);
    sprite_destroy(child);
}

TEST_F(SpriteGroupTest, ChildCount) {
    sprite_t parent = sprite_create(0, 0, 32, 32);
    sprite_t child1 = sprite_create(50, 50, 32, 32);
    sprite_t child2 = sprite_create(100, 100, 32, 32);

    sprite_set_parent(child1, parent);
    sprite_set_parent(child2, parent);

    EXPECT_EQ(sprite_get_child_count(parent), 2);
    EXPECT_EQ(sprite_get_child(parent, 0), child1);
    EXPECT_EQ(sprite_get_child(parent, 1), child2);

    sprite_destroy(parent);
    sprite_destroy(child1);
    sprite_destroy(child2);
}

TEST_F(SpriteGroupTest, RemoveParent) {
    sprite_t parent = sprite_create(0, 0, 32, 32);
    sprite_t child = sprite_create(50, 50, 32, 32);

    sprite_set_parent(child, parent);
    EXPECT_EQ(sprite_get_parent(child), parent);

    sprite_set_parent(child, NULL);
    EXPECT_EQ(sprite_get_parent(child), INVALID_SPRITE);
    EXPECT_EQ(sprite_get_child_count(parent), 0);

    sprite_destroy(parent);
    sprite_destroy(child);
}

TEST_F(SpriteGroupTest, IsChildOf) {
    sprite_t parent = sprite_create(0, 0, 32, 32);
    sprite_t child = sprite_create(50, 50, 32, 32);
    sprite_t other = sprite_create(100, 100, 32, 32);

    sprite_set_parent(child, parent);

    EXPECT_EQ(sprite_is_child_of(child, parent), 1);
    EXPECT_EQ(sprite_is_child_of(child, other), 0);

    sprite_destroy(parent);
    sprite_destroy(child);
    sprite_destroy(other);
}

TEST_F(SpriteGroupTest, AncestorCheck) {
    sprite_t root = sprite_create(0, 0, 32, 32);
    sprite_t child = sprite_create(50, 50, 32, 32);
    sprite_t grandchild = sprite_create(100, 100, 32, 32);

    sprite_set_parent(child, root);
    sprite_set_parent(grandchild, child);

    EXPECT_EQ(sprite_is_ancestor_of(root, grandchild), 1);
    EXPECT_EQ(sprite_is_ancestor_of(child, grandchild), 1);
    EXPECT_EQ(sprite_is_ancestor_of(grandchild, root), 0);

    sprite_destroy(root);
    sprite_destroy(child);
    sprite_destroy(grandchild);
}

/* ============================================================================
 * BATCH OPERATIONS TESTS
 * ========================================================================== */

TEST_F(SpriteGroupTest, GroupCollisionPoint) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t s1 = sprite_create(50, 50, 32, 32);
    sprite_t s2 = sprite_create(100, 100, 32, 32);

    sprite_group_add_sprite(group, s1);
    sprite_group_add_sprite(group, s2);

    // Test collision with first sprite
    EXPECT_EQ(sprite_group_collides_point(group, 60, 60), 1);
    // Test no collision
    EXPECT_EQ(sprite_group_collides_point(group, 200, 200), 0);

    sprite_destroy(s1);
    sprite_destroy(s2);
    sprite_group_destroy(group);
}

TEST_F(SpriteGroupTest, GroupEnumerate) {
    sprite_group_t g1 = sprite_group_create(0, 0);
    sprite_group_t g2 = sprite_group_create(100, 100);

    int count = sprite_count_all_groups();
    EXPECT_GE(count, 2);

    sprite_group_destroy(g1);
    sprite_group_destroy(g2);
}

/* ============================================================================
 * COMPLEX HIERARCHY TESTS
 * ========================================================================== */

TEST_F(SpriteGroupTest, ComplexHierarchy) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t root = sprite_create(0, 0, 32, 32);
    sprite_t child1 = sprite_create(50, 0, 32, 32);
    sprite_t child2 = sprite_create(100, 0, 32, 32);
    sprite_t grandchild = sprite_create(150, 0, 32, 32);

    sprite_group_add_sprite(group, root);
    sprite_group_add_sprite(group, child1);
    sprite_group_add_sprite(group, child2);
    sprite_group_add_sprite(group, grandchild);

    sprite_set_parent(child1, root);
    sprite_set_parent(child2, root);
    sprite_set_parent(grandchild, child1);

    EXPECT_EQ(sprite_group_get_member_count(group), 4);
    EXPECT_EQ(sprite_get_child_count(root), 2);
    EXPECT_EQ(sprite_get_child_count(child1), 1);
    EXPECT_EQ(sprite_get_child_count(child2), 0);

    sprite_destroy(root);
    sprite_destroy(child1);
    sprite_destroy(child2);
    sprite_destroy(grandchild);
    sprite_group_destroy(group);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
