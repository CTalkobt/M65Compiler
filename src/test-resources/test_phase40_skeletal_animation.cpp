#include <gtest/gtest.h>
#include "sprite_skeletal_animation.h"

class SkeletalAnimationTest : public ::testing::Test {
protected:
    sprite_skeleton_t skeleton;

    void SetUp() override {
        skeleton = sprite_skeleton_create();
        ASSERT_NE(skeleton, INVALID_SKELETON);
    }

    void TearDown() override {
        sprite_skeleton_destroy(skeleton);
    }
};

TEST_F(SkeletalAnimationTest, CreateDestroy) {
    sprite_skeleton_t skel = sprite_skeleton_create();
    EXPECT_NE(skel, INVALID_SKELETON);
    sprite_skeleton_destroy(skel);
}

TEST_F(SkeletalAnimationTest, GetInfo) {
    sprite_skeleton_info_t info;
    int result = sprite_skeleton_get_info(skeleton, &info);
    EXPECT_EQ(result, 1);
    EXPECT_EQ(info.bone_count, 0);
    EXPECT_EQ(info.track_count, 0);
    EXPECT_EQ(info.is_playing, 0);
}

TEST_F(SkeletalAnimationTest, AddBone) {
    sprite_bone_t root = sprite_skeleton_add_bone(skeleton, "root", NULL);
    EXPECT_NE(root, INVALID_BONE);

    sprite_skeleton_info_t info;
    sprite_skeleton_get_info(skeleton, &info);
    EXPECT_EQ(info.bone_count, 1);
}

TEST_F(SkeletalAnimationTest, AddMultipleBones) {
    sprite_bone_t root = sprite_skeleton_add_bone(skeleton, "root", NULL);
    sprite_bone_t left_arm = sprite_skeleton_add_bone(skeleton, "left_arm", root);
    sprite_bone_t right_arm = sprite_skeleton_add_bone(skeleton, "right_arm", root);

    sprite_skeleton_info_t info;
    sprite_skeleton_get_info(skeleton, &info);
    EXPECT_EQ(info.bone_count, 3);
}

TEST_F(SkeletalAnimationTest, FindBone) {
    sprite_bone_t root = sprite_skeleton_add_bone(skeleton, "root", NULL);
    sprite_bone_t found = sprite_skeleton_find_bone(skeleton, "root");
    EXPECT_EQ(found, root);
}

TEST_F(SkeletalAnimationTest, GetBoneByIndex) {
    sprite_bone_t root = sprite_skeleton_add_bone(skeleton, "root", NULL);
    sprite_bone_t retrieved = sprite_skeleton_get_bone(skeleton, 0);
    EXPECT_EQ(retrieved, root);
}

TEST_F(SkeletalAnimationTest, GetBoneCount) {
    sprite_skeleton_add_bone(skeleton, "bone1", NULL);
    sprite_skeleton_add_bone(skeleton, "bone2", NULL);

    int count = sprite_skeleton_get_bone_count(skeleton);
    EXPECT_EQ(count, 2);
}

TEST_F(SkeletalAnimationTest, SetGetBonePosition) {
    sprite_bone_t bone = sprite_skeleton_add_bone(skeleton, "bone", NULL);
    sprite_skeleton_set_bone_position(skeleton, bone, 10.0f, 20.0f);

    float x, y;
    sprite_skeleton_get_bone_position(skeleton, bone, &x, &y);
    EXPECT_NEAR(x, 10.0f, 0.001f);
    EXPECT_NEAR(y, 20.0f, 0.001f);
}

TEST_F(SkeletalAnimationTest, SetGetBoneRotation) {
    sprite_bone_t bone = sprite_skeleton_add_bone(skeleton, "bone", NULL);
    sprite_skeleton_set_bone_rotation(skeleton, bone, 45.0f);

    float rotation = sprite_skeleton_get_bone_rotation(skeleton, bone);
    EXPECT_NEAR(rotation, 45.0f, 0.001f);
}

TEST_F(SkeletalAnimationTest, SetGetBoneScale) {
    sprite_bone_t bone = sprite_skeleton_add_bone(skeleton, "bone", NULL);
    sprite_skeleton_set_bone_scale(skeleton, bone, 2.0f, 3.0f);

    float sx, sy;
    sprite_skeleton_get_bone_scale(skeleton, bone, &sx, &sy);
    EXPECT_NEAR(sx, 2.0f, 0.001f);
    EXPECT_NEAR(sy, 3.0f, 0.001f);
}

TEST_F(SkeletalAnimationTest, AddTrack) {
    sprite_bone_t bone = sprite_skeleton_add_bone(skeleton, "bone", NULL);
    sprite_track_t track = sprite_skeleton_add_track(skeleton, bone, TRACK_POSITION);
    EXPECT_NE(track, INVALID_TRACK);

    int count = sprite_skeleton_get_track_count(skeleton);
    EXPECT_EQ(count, 1);
}

TEST_F(SkeletalAnimationTest, AddKeyframe) {
    sprite_bone_t bone = sprite_skeleton_add_bone(skeleton, "bone", NULL);
    sprite_track_t track = sprite_skeleton_add_track(skeleton, bone, TRACK_POSITION);

    sprite_keyframe_t kf = sprite_skeleton_add_keyframe(skeleton, track,
                                                        0.0f, 10.0f, 20.0f, 0.0f);
    EXPECT_NE(kf, INVALID_KEYFRAME);

    int count = sprite_skeleton_get_keyframe_count(skeleton, track);
    EXPECT_EQ(count, 1);
}

TEST_F(SkeletalAnimationTest, MultipleKeyframes) {
    sprite_bone_t bone = sprite_skeleton_add_bone(skeleton, "bone", NULL);
    sprite_track_t track = sprite_skeleton_add_track(skeleton, bone, TRACK_POSITION);

    sprite_skeleton_add_keyframe(skeleton, track, 0.0f, 0.0f, 0.0f, 0.0f);
    sprite_skeleton_add_keyframe(skeleton, track, 100.0f, 50.0f, 50.0f, 0.0f);
    sprite_skeleton_add_keyframe(skeleton, track, 200.0f, 100.0f, 100.0f, 0.0f);

    int count = sprite_skeleton_get_keyframe_count(skeleton, track);
    EXPECT_EQ(count, 3);
}

TEST_F(SkeletalAnimationTest, PlaybackControl) {
    sprite_skeleton_play(skeleton);
    EXPECT_EQ(sprite_skeleton_is_playing(skeleton), 1);

    sprite_skeleton_pause(skeleton);
    EXPECT_EQ(sprite_skeleton_is_playing(skeleton), 0);

    sprite_skeleton_play(skeleton);
    EXPECT_EQ(sprite_skeleton_is_playing(skeleton), 1);

    sprite_skeleton_stop(skeleton);
    EXPECT_EQ(sprite_skeleton_is_playing(skeleton), 0);
    EXPECT_EQ(sprite_skeleton_get_animation_time(skeleton), 0);
}

TEST_F(SkeletalAnimationTest, AnimationTime) {
    sprite_skeleton_set_animation_time(skeleton, 500);
    int time = sprite_skeleton_get_animation_time(skeleton);
    EXPECT_EQ(time, 500);
}

TEST_F(SkeletalAnimationTest, PlaybackSpeed) {
    sprite_skeleton_set_playback_speed(skeleton, 2.0f);
    sprite_skeleton_play(skeleton);
    sprite_skeleton_update(skeleton, 16);

    int time = sprite_skeleton_get_animation_time(skeleton);
    EXPECT_GT(time, 16);
}

TEST_F(SkeletalAnimationTest, AnimationDuration) {
    sprite_bone_t bone = sprite_skeleton_add_bone(skeleton, "bone", NULL);
    sprite_track_t track = sprite_skeleton_add_track(skeleton, bone, TRACK_POSITION);

    sprite_skeleton_add_keyframe(skeleton, track, 0.0f, 0.0f, 0.0f, 0.0f);
    sprite_skeleton_add_keyframe(skeleton, track, 1000.0f, 50.0f, 50.0f, 0.0f);

    float duration = sprite_skeleton_get_animation_duration(skeleton);
    EXPECT_NEAR(duration, 1000.0f, 1.0f);
}

TEST_F(SkeletalAnimationTest, IKConstraint) {
    sprite_bone_t root = sprite_skeleton_add_bone(skeleton, "root", NULL);
    sprite_bone_t child = sprite_skeleton_add_bone(skeleton, "child", root);
    sprite_bone_t target = sprite_skeleton_add_bone(skeleton, "target", NULL);

    int result = sprite_skeleton_add_ik_constraint(skeleton, root, child, target,
                                                   2, 0.01f);
    EXPECT_EQ(result, 1);
}

TEST_F(SkeletalAnimationTest, UpdateAnimation) {
    sprite_bone_t bone = sprite_skeleton_add_bone(skeleton, "bone", NULL);
    sprite_track_t track = sprite_skeleton_add_track(skeleton, bone, TRACK_POSITION);

    sprite_skeleton_add_keyframe(skeleton, track, 0.0f, 0.0f, 0.0f, 0.0f);
    sprite_skeleton_add_keyframe(skeleton, track, 100.0f, 100.0f, 100.0f, 0.0f);

    sprite_skeleton_play(skeleton);
    int updated = sprite_skeleton_update(skeleton, 16);
    EXPECT_GT(updated, 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
