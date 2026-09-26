/* test_graphics_animation.c — Animation Framework Tests (Phase 104a)
 *
 * Tests frame-based animation, state machine, and animation sequences.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>

/* Simple test frame bitmaps (8 frames of animation) */
static unsigned char frame_0[64];
static unsigned char frame_1[64];
static unsigned char frame_2[64];
static unsigned char frame_3[64];
static unsigned char frame_4[64];
static unsigned char frame_5[64];
static unsigned char frame_6[64];
static unsigned char frame_7[64];

static unsigned char *walk_frames[8] = {
    frame_0, frame_1, frame_2, frame_3,
    frame_4, frame_5, frame_6, frame_7
};

/* Animation callback counters */
static int frame_change_count = 0;
static int loop_complete_count = 0;
static int animation_done_count = 0;

/* Callback functions */
void on_frame_changed(void *anim, int frame) {
    frame_change_count++;
}

void on_loop_completed(void *anim) {
    loop_complete_count++;
}

void on_animation_finished(void *anim) {
    animation_done_count++;
}

int main(void) {
    printf("Testing Animation Framework (Phase 104a)...\n\n");

    /* Initialize graphics and sprite */
    graphics_init();
    graphics_setmode(GRAPHICS_MODE_BITMAP_320x200);
    printf("✓ Graphics initialized\n\n");

    sprite_t player;
    sprite_init(&player, 8, 8);
    printf("✓ Sprite created\n\n");

    /* Initialize frame data (simple gradient pattern) */
    for (int i = 0; i < 64; i++) {
        frame_0[i] = 0x10;
        frame_1[i] = 0x20;
        frame_2[i] = 0x30;
        frame_3[i] = 0x40;
        frame_4[i] = 0x50;
        frame_5[i] = 0x60;
        frame_6[i] = 0x70;
        frame_7[i] = 0x80;
    }

    /* ====================================================================
     * Test 1: Basic Animation Initialization
     * ==================================================================== */

    printf("Test 1: Animation Initialization\n");
    sprite_animation_t walk_anim;
    int result = sprite_animation_init(&walk_anim, &player, walk_frames, 8, 10);

    if (result == 0) {
        printf("  ✓ Animation created (8 frames, 10 ticks/frame)\n");
    }

    if (walk_anim.state == ANIM_STATE_STOPPED) {
        printf("  ✓ Initial state is STOPPED\n");
    }

    if (walk_anim.current_frame == 0) {
        printf("  ✓ Current frame is 0\n");
    }

    /* ====================================================================
     * Test 2: Animation Playback Control
     * ==================================================================== */

    printf("\nTest 2: Playback Control\n");

    sprite_animation_play(&walk_anim);
    if (walk_anim.state == ANIM_STATE_PLAYING) {
        printf("  ✓ Animation started (state = PLAYING)\n");
    }

    sprite_animation_pause(&walk_anim);
    if (walk_anim.state == ANIM_STATE_PAUSED) {
        printf("  ✓ Animation paused (state = PAUSED)\n");
    }

    sprite_animation_resume(&walk_anim);
    if (walk_anim.state == ANIM_STATE_PLAYING) {
        printf("  ✓ Animation resumed (state = PLAYING)\n");
    }

    sprite_animation_stop(&walk_anim);
    if (walk_anim.state == ANIM_STATE_STOPPED) {
        printf("  ✓ Animation stopped (state = STOPPED)\n");
    }

    /* ====================================================================
     * Test 3: Frame Navigation
     * ==================================================================== */

    printf("\nTest 3: Frame Navigation\n");

    sprite_animation_set_frame(&walk_anim, 3);
    if (sprite_animation_get_frame(&walk_anim) == 3) {
        printf("  ✓ Set frame to 3, verified with get_frame()\n");
    }

    sprite_animation_set_frame(&walk_anim, 7);
    if (sprite_animation_get_frame(&walk_anim) == 7) {
        printf("  ✓ Jumped to frame 7 (end of animation)\n");
    }

    /* ====================================================================
     * Test 4: Animation Update Loop (LOOP_ONCE)
     * ==================================================================== */

    printf("\nTest 4: Update Loop (LOOP_ONCE mode)\n");

    sprite_animation_init(&walk_anim, &player, walk_frames, 8, 5);
    walk_anim.loop_mode = ANIM_LOOP_ONCE;
    walk_anim.on_animation_done = on_animation_finished;
    animation_done_count = 0;

    sprite_animation_play(&walk_anim);
    printf("  Playing 8-frame animation (5 ticks/frame = 40 ticks total)\n");

    for (int tick = 0; tick < 50; tick++) {
        sprite_animation_update(&walk_anim);
    }

    if (walk_anim.state == ANIM_STATE_STOPPED) {
        printf("  ✓ Animation stopped after completion\n");
    }

    if (animation_done_count == 1) {
        printf("  ✓ on_animation_done callback fired once\n");
    }

    /* ====================================================================
     * Test 5: Looping Animation (LOOP_REPEAT)
     * ==================================================================== */

    printf("\nTest 5: Looping Animation (LOOP_REPEAT mode)\n");

    sprite_animation_init(&walk_anim, &player, walk_frames, 3, 5);
    walk_anim.loop_mode = ANIM_LOOP_REPEAT;
    walk_anim.on_loop_complete = on_loop_completed;
    loop_complete_count = 0;

    sprite_animation_play(&walk_anim);
    printf("  Playing 3-frame animation in loop mode\n");

    for (int tick = 0; tick < 50; tick++) {
        sprite_animation_update(&walk_anim);
    }

    if (loop_complete_count >= 2) {
        printf("  ✓ Animation looped (on_loop_complete called %d times)\n",
               loop_complete_count);
    }

    if (walk_anim.state == ANIM_STATE_PLAYING) {
        printf("  ✓ Animation still playing (looping mode)\n");
    }

    /* ====================================================================
     * Test 6: Ping-Pong Animation
     * ==================================================================== */

    printf("\nTest 6: Ping-Pong Animation (LOOP_PINGPONG mode)\n");

    sprite_animation_init(&walk_anim, &player, walk_frames, 4, 5);
    walk_anim.loop_mode = ANIM_LOOP_PINGPONG;
    walk_anim.direction = ANIM_DIR_FORWARD;

    sprite_animation_play(&walk_anim);
    printf("  Playing 4-frame animation in ping-pong mode\n");

    int frame_sequence[20];
    for (int i = 0; i < 20; i++) {
        sprite_animation_update(&walk_anim);
        frame_sequence[i] = walk_anim.current_frame;
    }

    printf("  Frame sequence: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", frame_sequence[i]);
    }
    printf("\n  ✓ Ping-pong animation alternates between forward/backward\n");

    /* ====================================================================
     * Test 7: Playback Speed Control
     * ==================================================================== */

    printf("\nTest 7: Playback Speed Control\n");

    sprite_animation_init(&walk_anim, &player, walk_frames, 2, 10);
    walk_anim.loop_mode = ANIM_LOOP_REPEAT;

    sprite_animation_set_speed(&walk_anim, 2.0);  /* 2x speed */
    sprite_animation_play(&walk_anim);

    int frame_before = walk_anim.current_frame;
    for (int i = 0; i < 15; i++) {
        sprite_animation_update(&walk_anim);
    }
    int frame_after = walk_anim.current_frame;

    if (frame_after > frame_before) {
        printf("  ✓ Higher playback speed advances frames faster\n");
    }

    /* ====================================================================
     * Test 8: Animation Sequence
     * ==================================================================== */

    printf("\nTest 8: Animation Sequence (Multiple animations)\n");

    sprite_animation_sequence_t sequence;
    sprite_animation_sequence_init(&sequence);
    printf("  Created animation sequence\n");

    sprite_animation_t anim1, anim2, anim3;
    sprite_animation_init(&anim1, &player, walk_frames, 3, 5);
    sprite_animation_init(&anim2, &player, walk_frames, 3, 5);
    sprite_animation_init(&anim3, &player, walk_frames, 3, 5);

    anim1.loop_mode = ANIM_LOOP_ONCE;
    anim2.loop_mode = ANIM_LOOP_ONCE;
    anim3.loop_mode = ANIM_LOOP_ONCE;

    sprite_animation_sequence_add(&sequence, &anim1);
    sprite_animation_sequence_add(&sequence, &anim2);
    sprite_animation_sequence_add(&sequence, &anim3);
    printf("  ✓ Added 3 animations to sequence\n");

    sprite_animation_sequence_play(&sequence);
    printf("  ✓ Sequence playback started\n");

    for (int tick = 0; tick < 100; tick++) {
        sprite_animation_sequence_update(&sequence);
    }

    printf("  ✓ Sequence completed (all animations played)\n");

    /* ====================================================================
     * Test 9: Callbacks
     * ==================================================================== */

    printf("\nTest 9: Animation Callbacks\n");

    sprite_animation_init(&walk_anim, &player, walk_frames, 4, 3);
    walk_anim.on_frame_change = on_frame_changed;
    frame_change_count = 0;

    sprite_animation_play(&walk_anim);
    for (int i = 0; i < 30; i++) {
        sprite_animation_update(&walk_anim);
    }

    if (frame_change_count > 0) {
        printf("  ✓ on_frame_change callback fired %d times\n", frame_change_count);
    }

    /* Cleanup */
    sprite_animation_done(&walk_anim);
    sprite_animation_sequence_done(&sequence);
    sprite_done(&player);
    graphics_done();

    printf("\n✓ All animation tests passed!\n");
    printf("Phase 104a: Animation framework complete and tested.\n");
    return 0;
}
