/* test_phase38_animation_trees.cpp — Animation Trees Tests
 *
 * Tests hierarchical animation system with state machines and transitions.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>

extern "C" {
#include "../../lib/include/sprite_animation_trees.h"
#include "../../lib/include/sprite_groups.h"
#include "../../lib/include/sprites.h"
}

class AnimationTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        sprite_init();
    }
};

/* ============================================================================
 * TREE CREATION TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, CreateTreeFromSprite) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    ASSERT_NE(tree, INVALID_ANIM_TREE);

    sprite_anim_tree_info_t info;
    EXPECT_EQ(sprite_anim_tree_get_info(tree, &info), 1);
    EXPECT_EQ(info.is_playing, 0);
    EXPECT_NEAR(info.time_scale, 1.0f, 0.001f);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, CreateTreeFromGroup) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_anim_tree_t tree = sprite_anim_tree_create(group, 1);

    ASSERT_NE(tree, INVALID_ANIM_TREE);

    sprite_anim_tree_info_t info;
    EXPECT_EQ(sprite_anim_tree_get_info(tree, &info), 1);

    sprite_anim_tree_destroy(tree);
    sprite_group_destroy(group);
}

/* ============================================================================
 * PLAYBACK CONTROL TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, PlaybackControl) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    EXPECT_EQ(sprite_anim_tree_is_playing(tree), 0);

    sprite_anim_tree_play(tree);
    EXPECT_EQ(sprite_anim_tree_is_playing(tree), 1);

    sprite_anim_tree_pause(tree);
    EXPECT_EQ(sprite_anim_tree_is_playing(tree), 0);

    sprite_anim_tree_play(tree);
    sprite_anim_tree_stop(tree);
    EXPECT_EQ(sprite_anim_tree_is_playing(tree), 0);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, TimeScale) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    sprite_anim_tree_set_time_scale(tree, 2.0f);
    EXPECT_NEAR(sprite_anim_tree_get_time_scale(tree), 2.0f, 0.001f);

    sprite_anim_tree_set_time_scale(tree, 0.5f);
    EXPECT_NEAR(sprite_anim_tree_get_time_scale(tree), 0.5f, 0.001f);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * LAYER MANAGEMENT TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, AddLayer) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);
    ASSERT_NE(layer, INVALID_ANIM_LAYER);

    EXPECT_EQ(sprite_anim_layer_get_count(tree), 1);
    EXPECT_EQ(sprite_anim_layer_get(tree, 0), layer);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, LayerWeight) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 0.8f, ANIM_LAYER_BASE);

    sprite_anim_layer_set_weight(layer, 0.5f);
    EXPECT_NEAR(sprite_anim_layer_get_weight(layer), 0.5f, 0.001f);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, MultipleLayes) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    sprite_anim_layer_t base = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);
    sprite_anim_layer_t override = sprite_anim_layer_add(tree, "override", 0.5f, ANIM_LAYER_OVERRIDE);

    EXPECT_EQ(sprite_anim_layer_get_count(tree), 2);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * STATE MANAGEMENT TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, AddState) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_t idle = sprite_anim_state_add(layer, "idle");
    ASSERT_NE(idle, INVALID_ANIM_STATE);

    EXPECT_EQ(sprite_anim_state_get_count(layer), 1);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, FindState) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_add(layer, "idle");
    sprite_anim_state_add(layer, "run");
    sprite_anim_state_add(layer, "jump");

    sprite_anim_state_t run = sprite_anim_state_find(layer, "run");
    ASSERT_NE(run, INVALID_ANIM_STATE);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, MultipleStates) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_add(layer, "idle");
    sprite_anim_state_add(layer, "run");
    sprite_anim_state_add(layer, "jump");

    EXPECT_EQ(sprite_anim_state_get_count(layer), 3);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * ANIMATION CLIP TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, AddClip) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);
    sprite_anim_state_t idle = sprite_anim_state_add(layer, "idle");

    uint8_t *frames[4] = {0, 0, 0, 0};
    sprite_anim_clip_t clip = sprite_anim_state_add_clip(idle, frames, 4, 2);

    ASSERT_NE(clip, INVALID_ANIM_CLIP);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * STATE PLAYBACK TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, PlayState) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_add(layer, "idle");
    sprite_anim_state_add(layer, "run");

    EXPECT_EQ(sprite_anim_tree_play_state(tree, "idle", ANIM_TRANSITION_IMMEDIATE), 1);
    EXPECT_EQ(sprite_anim_tree_is_playing(tree), 1);

    char *current = sprite_anim_tree_get_current_state(tree);
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(strcmp(current, "idle"), 0);
    free(current);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, StateNotFound) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_add(layer, "idle");

    EXPECT_EQ(sprite_anim_tree_play_state(tree, "nonexistent", ANIM_TRANSITION_IMMEDIATE), 0);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, QueueState) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_add(layer, "idle");
    sprite_anim_state_add(layer, "run");

    EXPECT_EQ(sprite_anim_tree_queue_state(tree, "run"), 1);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * TRANSITION TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, AddTransition) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_t idle = sprite_anim_state_add(layer, "idle");
    sprite_anim_state_t run = sprite_anim_state_add(layer, "run");

    sprite_anim_transition_t trans = sprite_anim_transition_add(idle, run, "start_running",
                                                                 ANIM_TRANSITION_IMMEDIATE);
    ASSERT_NE(trans, INVALID_ANIM_TRANSITION);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, TriggerTransition) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_add(layer, "idle");
    sprite_anim_state_add(layer, "run");

    sprite_anim_transition_add(sprite_anim_state_find(layer, "idle"),
                               sprite_anim_state_find(layer, "run"),
                               "start_running", ANIM_TRANSITION_IMMEDIATE);

    EXPECT_EQ(sprite_anim_tree_trigger_transition(tree, "start_running"), 1);
    EXPECT_EQ(sprite_anim_tree_trigger_transition(tree, "nonexistent"), 0);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * SYNCHRONIZATION TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, SynchronizeChildren) {
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t child1 = sprite_create(0, 0, 32, 32);
    sprite_t child2 = sprite_create(40, 0, 32, 32);

    sprite_group_add_sprite(group, child1);
    sprite_group_add_sprite(group, child2);

    sprite_anim_tree_t tree = sprite_anim_tree_create(group, 1);

    sprite_anim_tree_synchronize_children(tree);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(child1);
    sprite_destroy(child2);
    sprite_group_destroy(group);
}

/* ============================================================================
 * PLAYBACK TIME TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, PlaybackTime) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);

    EXPECT_EQ(sprite_anim_tree_get_playback_time(tree), 0);

    sprite_anim_tree_set_playback_time(tree, 500);
    EXPECT_EQ(sprite_anim_tree_get_playback_time(tree), 500);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

TEST_F(AnimationTreeTest, Update) {
    sprite_t sprite = sprite_create(100, 100, 32, 32);
    sprite_anim_tree_t tree = sprite_anim_tree_create(sprite, 0);
    sprite_anim_layer_t layer = sprite_anim_layer_add(tree, "base", 1.0f, ANIM_LAYER_BASE);

    sprite_anim_state_add(layer, "idle");

    sprite_anim_tree_play(tree);
    sprite_anim_tree_play_state(tree, "idle", ANIM_TRANSITION_IMMEDIATE);

    int updated = sprite_anim_tree_update(tree, 16);
    EXPECT_GE(updated, 0);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(sprite);
}

/* ============================================================================
 * COMPLEX HIERARCHY TESTS
 * ========================================================================== */

TEST_F(AnimationTreeTest, ComplexAnimationHierarchy) {
    // Create a complex animation tree with multiple layers and states
    sprite_group_t group = sprite_group_create(0, 0);
    sprite_t character = sprite_create(0, 0, 32, 32);
    sprite_t weapon = sprite_create(20, 5, 16, 16);

    sprite_group_add_sprite(group, character);
    sprite_group_add_sprite(group, weapon);

    sprite_anim_tree_t tree = sprite_anim_tree_create(group, 1);

    // Add multiple layers
    sprite_anim_layer_t body_layer = sprite_anim_layer_add(tree, "body", 1.0f, ANIM_LAYER_BASE);
    sprite_anim_layer_t weapon_layer = sprite_anim_layer_add(tree, "weapon", 0.8f, ANIM_LAYER_ADDITIVE);

    // Add states to body layer
    sprite_anim_state_t body_idle = sprite_anim_state_add(body_layer, "idle");
    sprite_anim_state_t body_run = sprite_anim_state_add(body_layer, "run");
    sprite_anim_state_t body_jump = sprite_anim_state_add(body_layer, "jump");

    // Add states to weapon layer
    sprite_anim_state_t weapon_idle = sprite_anim_state_add(weapon_layer, "idle");
    sprite_anim_state_t weapon_fire = sprite_anim_state_add(weapon_layer, "fire");

    // Add transitions
    sprite_anim_transition_add(body_idle, body_run, "start_moving", ANIM_TRANSITION_CROSSFADE);
    sprite_anim_transition_add(body_run, body_idle, "stop_moving", ANIM_TRANSITION_CROSSFADE);
    sprite_anim_transition_add(weapon_idle, weapon_fire, "fire_weapon", ANIM_TRANSITION_IMMEDIATE);

    // Verify structure
    EXPECT_EQ(sprite_anim_layer_get_count(tree), 2);
    EXPECT_EQ(sprite_anim_state_get_count(body_layer), 3);
    EXPECT_EQ(sprite_anim_state_get_count(weapon_layer), 2);

    // Play and trigger transitions
    EXPECT_EQ(sprite_anim_tree_play_state(tree, "idle", ANIM_TRANSITION_IMMEDIATE), 1);
    EXPECT_EQ(sprite_anim_tree_trigger_transition(tree, "start_moving"), 1);

    sprite_anim_tree_destroy(tree);
    sprite_destroy(character);
    sprite_destroy(weapon);
    sprite_group_destroy(group);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
