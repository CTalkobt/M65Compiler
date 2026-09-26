#include <gtest/gtest.h>
#include "sprite_ragdoll_physics.h"

class RagdollPhysicsTest : public ::testing::Test {
protected:
    sprite_skeleton_t skeleton;
    sprite_ragdoll_t ragdoll;

    void SetUp() override {
        skeleton = sprite_skeleton_create();
        sprite_skeleton_add_bone(skeleton, "root", NULL);
        ragdoll = sprite_ragdoll_create(skeleton);
        ASSERT_NE(ragdoll, INVALID_RAGDOLL);
    }

    void TearDown() override {
        sprite_skeleton_destroy(skeleton);
        sprite_ragdoll_destroy(ragdoll);
    }
};

TEST_F(RagdollPhysicsTest, CreateDestroy) {
    EXPECT_NE(ragdoll, INVALID_RAGDOLL);
}

TEST_F(RagdollPhysicsTest, GetInfo) {
    sprite_ragdoll_info_t info;
    int result = sprite_ragdoll_get_info(ragdoll, &info);
    EXPECT_EQ(result, 1);
    EXPECT_GT(info.bone_count, 0);
}

TEST_F(RagdollPhysicsTest, SetGetGravity) {
    sprite_ragdoll_set_gravity(ragdoll, 5.0f, 10.0f);
    float gx, gy;
    sprite_ragdoll_get_gravity(ragdoll, &gx, &gy);
    EXPECT_NEAR(gx, 5.0f, 0.001f);
    EXPECT_NEAR(gy, 10.0f, 0.001f);
}

TEST_F(RagdollPhysicsTest, SetGetDamping) {
    sprite_ragdoll_set_damping(ragdoll, 0.95f);
    float damping = sprite_ragdoll_get_damping(ragdoll);
    EXPECT_NEAR(damping, 0.95f, 0.001f);
}

TEST_F(RagdollPhysicsTest, SetGetBoneMass) {
    sprite_bone_t bone = sprite_skeleton_get_bone(skeleton, 0);
    sprite_ragdoll_set_bone_mass(ragdoll, bone, 5.0f);
    float mass = sprite_ragdoll_get_bone_mass(ragdoll, bone);
    EXPECT_NEAR(mass, 5.0f, 0.001f);
}

TEST_F(RagdollPhysicsTest, SetGetBoneVelocity) {
    sprite_bone_t bone = sprite_skeleton_get_bone(skeleton, 0);
    sprite_ragdoll_set_bone_velocity(ragdoll, bone, 2.0f, 3.0f);
    float vx, vy;
    sprite_ragdoll_get_bone_velocity(ragdoll, bone, &vx, &vy);
    EXPECT_NEAR(vx, 2.0f, 0.001f);
    EXPECT_NEAR(vy, 3.0f, 0.001f);
}

TEST_F(RagdollPhysicsTest, ApplyForce) {
    sprite_bone_t bone = sprite_skeleton_get_bone(skeleton, 0);
    sprite_ragdoll_apply_force(ragdoll, bone, 10.0f, 5.0f);
    // Force applied internally
}

TEST_F(RagdollPhysicsTest, ApplyImpulse) {
    sprite_bone_t bone = sprite_skeleton_get_bone(skeleton, 0);
    sprite_ragdoll_set_bone_mass(ragdoll, bone, 1.0f);
    sprite_ragdoll_apply_impulse(ragdoll, bone, 1.0f, 0.0f);
    float vx, vy;
    sprite_ragdoll_get_bone_velocity(ragdoll, bone, &vx, &vy);
    EXPECT_GT(vx, 0.0f);
}

TEST_F(RagdollPhysicsTest, AddConstraint) {
    sprite_bone_t bone1 = sprite_skeleton_get_bone(skeleton, 0);
    sprite_bone_t bone2 = sprite_skeleton_add_bone(skeleton, "child", bone1);

    sprite_constraint_t constraint = sprite_ragdoll_add_constraint(
        ragdoll, CONSTRAINT_DISTANCE, bone1, bone2, 10.0f, 0.0f);
    EXPECT_NE(constraint, INVALID_CONSTRAINT);
}

TEST_F(RagdollPhysicsTest, GetConstraintCount) {
    int count = sprite_ragdoll_get_constraint_count(ragdoll);
    EXPECT_GE(count, 0);
}

TEST_F(RagdollPhysicsTest, ActivateDeactivate) {
    sprite_ragdoll_deactivate(ragdoll);
    EXPECT_EQ(sprite_ragdoll_is_active(ragdoll), 0);

    sprite_ragdoll_activate(ragdoll);
    EXPECT_EQ(sprite_ragdoll_is_active(ragdoll), 1);
}

TEST_F(RagdollPhysicsTest, Update) {
    sprite_ragdoll_activate(ragdoll);
    int updated = sprite_ragdoll_update(ragdoll, 16);
    EXPECT_GT(updated, 0);
}

TEST_F(RagdollPhysicsTest, SelfCollision) {
    int result1 = sprite_ragdoll_enable_self_collision(ragdoll);
    EXPECT_EQ(result1, 1);

    int result2 = sprite_ragdoll_disable_self_collision(ragdoll);
    EXPECT_EQ(result2, 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
