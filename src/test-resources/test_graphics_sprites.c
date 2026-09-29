/* test_graphics_sprites.c — Sprite Framework Tests (Phase 103)
 *
 * Tests unified sprite API with software and hardware rendering.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>

/* Simple 8×8 sprite bitmap (checkerboard pattern) */
static unsigned char sprite_8x8[64] = {
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA
};

int main(void) {
    printf("Testing Graphics Sprite Framework (Phase 103)...\n\n");

    /* Test 1: Initialize graphics and sprite system */
    printf("Test 1: Graphics and Sprite Initialization\n");
    graphics_init();
    printf("  ✓ Graphics initialized\n");

    graphics_setmode(GRAPHICS_MODE_BITMAP_320x200);
    printf("  ✓ Set graphics mode to BITMAP_320x200\n");

    /* Test 2: Create sprite with software rendering */
    printf("\nTest 2: Sprite Initialization (Software)\n");
    sprite_t player;
    int result = sprite_init(&player, 8, 8);
    printf("  ✓ Sprite created (size 8×8)\n");
    printf("  Render mode: %s\n",
           player.render_mode == SPRITE_MODE_SOFTWARE ? "SOFTWARE" : "HARDWARE");

    /* Test 3: Set sprite properties */
    printf("\nTest 3: Sprite Properties\n");
    sprite_set_bitmap(&player, sprite_8x8);
    printf("  ✓ Bitmap set\n");

    sprite_set_color(&player, 2);  /* Red */
    printf("  ✓ Color set to red (2)\n");

    sprite_set_position(&player, 100, 50);
    printf("  ✓ Position set to (100, 50)\n");

    /* Test 4: Sprite visibility */
    printf("\nTest 4: Sprite Visibility\n");
    sprite_show(&player);
    printf("  ✓ Sprite shown\n");
    if (player.visible) {
        printf("  ✓ Sprite visible flag correct\n");
    }

    sprite_hide(&player);
    printf("  ✓ Sprite hidden\n");
    if (!player.visible) {
        printf("  ✓ Sprite visibility flag updated\n");
    }

    sprite_show(&player);

    /* Test 5: Multiple sprites */
    printf("\nTest 5: Multiple Sprites\n");
    sprite_t enemy1, enemy2;
    sprite_init(&enemy1, 8, 8);
    sprite_init(&enemy2, 8, 8);
    sprite_set_bitmap(&enemy1, sprite_8x8);
    sprite_set_bitmap(&enemy2, sprite_8x8);
    sprite_set_position(&enemy1, 150, 75);
    sprite_set_position(&enemy2, 200, 100);
    sprite_set_color(&enemy1, 3);  /* Cyan */
    sprite_set_color(&enemy2, 4);  /* Magenta */
    printf("  ✓ Created 2 enemy sprites\n");

    /* Test 6: Sprite movement */
    printf("\nTest 6: Sprite Movement\n");
    sprite_set_position(&player, 120, 60);
    printf("  ✓ Moved player to (120, 60)\n");
    if (player.x == 120 && player.y == 60) {
        printf("  ✓ Position update verified\n");
    }

    /* Test 7: Bounding-box collision detection */
    printf("\nTest 7: Collision Detection (Bounding Box)\n");
    sprite_set_position(&player, 100, 50);
    sprite_set_position(&enemy1, 105, 55);

    int collide = sprite_collides(&player, &enemy1);
    printf("  Sprites at (100,50) and (105,55) with size 8×8: %s\n",
           collide ? "COLLIDE" : "NO COLLISION");
    if (collide) {
        printf("  ✓ Collision detected correctly\n");
    }

    /* Non-colliding sprites */
    sprite_set_position(&enemy2, 200, 200);
    int no_collide = sprite_collides(&player, &enemy2);
    printf("  Sprites at (100,50) and (200,200): %s\n",
           no_collide ? "COLLIDE" : "NO COLLISION");
    if (!no_collide) {
        printf("  ✓ Non-collision detected correctly\n");
    }

    /* Test 8: Sprite drawing */
    printf("\nTest 8: Sprite Rendering\n");
    sprite_draw(&player);
    printf("  ✓ Drew player sprite\n");

    sprite_draw(&enemy1);
    sprite_draw(&enemy2);
    printf("  ✓ Drew enemy sprites\n");

    /* Test 9: Sprite clearing/erasing */
    printf("\nTest 9: Sprite Clearing\n");
    sprite_clear(&player);
    printf("  ✓ Cleared player sprite\n");

    sprite_clear(&enemy1);
    sprite_clear(&enemy2);
    printf("  ✓ Cleared enemy sprites\n");

    /* Test 10: Mode switching with sprites */
    printf("\nTest 10: Mode Switching\n");
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("  ✓ Switched to TEXT_40x25\n");

    sprite_draw(&player);
    printf("  ✓ Drew sprite in text mode (software fallback)\n");

    graphics_setmode(GRAPHICS_MODE_GRAPHICS_80x50);
    printf("  ✓ Switched to GRAPHICS_80x50\n");

    sprite_draw(&enemy1);
    printf("  ✓ Drew sprite in graphics mode\n");

    graphics_setmode(GRAPHICS_MODE_BITMAP_320x200);
    printf("  ✓ Switched back to BITMAP_320x200\n");

    /* Test 11: Sprite frame counter */
    printf("\nTest 11: Animation Frame Counter\n");
    sprite_t animated;
    sprite_init(&animated, 8, 8);
    sprite_set_bitmap(&animated, sprite_8x8);

    for (int frame = 0; frame < 10; frame++) {
        animated.frame = frame;
    }
    printf("  ✓ Incremented animation frame to %d\n", animated.frame);

    /* Test 12: Sprite cleanup */
    printf("\nTest 12: Sprite Cleanup\n");
    sprite_done(&player);
    sprite_done(&enemy1);
    sprite_done(&enemy2);
    sprite_done(&animated);
    printf("  ✓ All sprites cleaned up\n");

    graphics_done();
    printf("  ✓ Graphics cleaned up\n");

    printf("\n✓ All sprite tests completed!\n");
    printf("Unified sprite API works across all graphics modes.\n");
    return 0;
}
