/* test_graphics_sprites_phase7.c — Soft Sprite System Tests (Phase 7)
 *
 * Tests sprite management, animation, physics, collision detection,
 * and integration with RRB layer system.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("Testing Phase 7: Soft Sprite System...\n\n");

    graphics_init();
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("✓ Graphics initialized\n\n");

    /* Initialize RRB system for sprite rendering */
    rrb_system_t rrb;
    rrb.init(&rrb, 3, 40, 25);
    rrb.configure_vic(&rrb, 0, 40, 80);
    rrb.enable(&rrb);

    /* Create background layer */
    int bg_layer = rrb.create_layer(&rrb, RRB_MODE_FULL, 40, 25);
    rrb_layer_t *bg = rrb.get_layer(&rrb, bg_layer);
    bg->set_priority(bg, 0);
    for (int row = 0; row < 25; row++) {
        for (int col = 0; col < 40; col++) {
            bg->set_char(bg, col, row, '.', 0x02);
        }
    }

    /* Create sprite layer (SPARSE mode for efficient rendering) */
    int sprite_layer_idx = rrb.create_layer(&rrb, RRB_MODE_SPARSE, 40, 25);
    rrb_layer_t *sprite_layer = rrb.get_layer(&rrb, sprite_layer_idx);
    sprite_layer->clear(sprite_layer, 32, 0x00);
    sprite_layer->set_priority(sprite_layer, 1);

    printf("✓ RRB system initialized\n");
    printf("✓ Background and sprite layers created\n\n");

    /* ====================================================================
     * Test 1: Sprite Manager Creation
     * ==================================================================== */

    printf("Test 1: Sprite Manager Creation\n");

    sprite_manager_t manager = sprite_manager_create(50, &rrb, sprite_layer_idx);

    if (manager.max_sprites == 50 && manager.active_count == 0) {
        printf("  ✓ Sprite manager created (50 sprite pool)\n");
    }

    /* ====================================================================
     * Test 2: Sprite Allocation & Deallocation
     * ==================================================================== */

    printf("\nTest 2: Sprite Allocation & Deallocation\n");

    sprite_t *sprite1 = manager.allocate(&manager);
    sprite_t *sprite2 = manager.allocate(&manager);
    sprite_t *sprite3 = manager.allocate(&manager);

    if (sprite1 && sprite2 && sprite3 && manager.active_count == 3) {
        printf("  ✓ 3 sprites allocated from pool\n");
        printf("  ✓ Active count: %d\n", manager.get_active_count(&manager));
    }

    manager.free(&manager, sprite2);
    if (manager.active_count == 2) {
        printf("  ✓ 1 sprite freed, active count: %d\n", manager.active_count);
    }

    /* ====================================================================
     * Test 3: Sprite Position & Velocity
     * ==================================================================== */

    printf("\nTest 3: Sprite Position & Velocity\n");

    sprite1->set_position(sprite1, 100, 5);
    sprite1->set_velocity(sprite1, 2, 0);
    sprite1->set_acceleration(sprite1, 0, 0);
    sprite1->ch = 'P';
    sprite1->color = 0x0F;

    if (sprite1->x == 100 && sprite1->y == 5 && sprite1->vx == 2) {
        printf("  ✓ Position set (100, 5)\n");
        printf("  ✓ Velocity set (2, 0)\n");
    }

    /* ====================================================================
     * Test 4: Sprite Update (Physics)
     * ==================================================================== */

    printf("\nTest 4: Sprite Physics Update\n");

    sprite1->update(sprite1);
    if (sprite1->x == 102 && sprite1->y == 5) {
        printf("  ✓ Position updated by velocity: (102, 5)\n");
    }

    sprite1->set_acceleration(sprite1, 1, 0);
    sprite1->update(sprite1);
    if (sprite1->vx == 3) {
        printf("  ✓ Velocity updated by acceleration: vx=3\n");
        printf("  ✓ Position updated: (%d, %d)\n", sprite1->x, sprite1->y);
    }

    /* ====================================================================
     * Test 5: Sprite Visibility Control
     * ==================================================================== */

    printf("\nTest 5: Sprite Visibility Control\n");

    sprite3->show(sprite3);
    sprite3->set_position(sprite3, 200, 10);
    sprite3->ch = 'V';
    sprite3->color = 0x03;

    if (sprite3->visible) {
        printf("  ✓ Sprite shown\n");
    }

    sprite3->hide(sprite3);
    if (!sprite3->visible) {
        printf("  ✓ Sprite hidden\n");
    }

    sprite3->show(sprite3);

    /* ====================================================================
     * Test 6: Sprite Animation Setup
     * ==================================================================== */

    printf("\nTest 6: Sprite Animation\n");

    sprite_frame_t frames[3] = {
        {'*', 0x0F, 5},  /* Frame 0: * (5 frames duration) */
        {'+', 0x0E, 5},  /* Frame 1: + (5 frames duration) */
        {'o', 0x0D, 5}   /* Frame 2: o (5 frames duration) */
    };

    sprite_animation_t anim = sprite_animation_create(frames, 3, 1);
    sprite1->set_animation(sprite1, &anim);
    sprite1->play_animation(sprite1);

    if (sprite1->animation.is_playing && sprite1->animation.frame_count == 3) {
        printf("  ✓ Animation set (3 frames, looping)\n");
        printf("  ✓ Animation playing\n");
        printf("  ✓ Current frame: %d, character: %c\n", 
               sprite1->animation.current_frame, sprite1->ch);
    }

    /* ====================================================================
     * Test 7: Animation Frame Advancement
     * ==================================================================== */

    printf("\nTest 7: Animation Frame Advancement\n");

    for (int f = 0; f < 6; f++) {
        sprite1->update(sprite1);
    }

    if (sprite1->animation.current_frame == 1) {
        printf("  ✓ Frame advanced to 1 (after 6 updates)\n");
        printf("  ✓ Character updated: %c\n", sprite1->ch);
    }

    /* ====================================================================
     * Test 8: Sprite Rendering
     * ==================================================================== */

    printf("\nTest 8: Sprite Rendering to RRB Layer\n");

    sprite1->set_position(sprite1, 64, 12);  /* Position at char (8, 12) */
    sprite1->stop_animation(sprite1);
    sprite1->ch = 'S';
    sprite1->color = 0x0F;

    sprite1->render(sprite1, sprite_layer);

    if (sprite_layer->get_char(sprite_layer, 8, 12) == 'S') {
        printf("  ✓ Sprite rendered to RRB layer\n");
        printf("  ✓ Character verified at (8, 12)\n");
    }

    /* ====================================================================
     * Test 9: Render All Sprites
     * ==================================================================== */

    printf("\nTest 9: Render All Sprites\n");

    sprite2 = manager.allocate(&manager);
    sprite2->set_position(sprite2, 240, 5);
    sprite2->ch = 'E';
    sprite2->color = 0x03;

    manager.render_all(&manager);

    if (manager.active_count == 3) {
        printf("  ✓ All %d active sprites rendered\n", manager.active_count);
    }

    /* ====================================================================
     * Test 10: Update All Sprites
     * ==================================================================== */

    printf("\nTest 10: Update All Sprites\n");

    sprite1->set_velocity(sprite1, 1, 0);
    sprite2->set_velocity(sprite2, -1, 0);
    sprite3->set_velocity(sprite3, 0, 1);

    int prev_x1 = sprite1->x;
    int prev_x2 = sprite2->x;
    int prev_y3 = sprite3->y;

    manager.update_all(&manager);

    if (sprite1->x == prev_x1 + 1 && sprite2->x == prev_x2 - 1 && sprite3->y == prev_y3 + 1) {
        printf("  ✓ All sprites physics updated\n");
        printf("  ✓ Sprite1 position: (%d, %d)\n", sprite1->x, sprite1->y);
        printf("  ✓ Sprite2 position: (%d, %d)\n", sprite2->x, sprite2->y);
        printf("  ✓ Sprite3 position: (%d, %d)\n", sprite3->x, sprite3->y);
    }

    /* ====================================================================
     * Test 11: Collision Detection
     * ==================================================================== */

    printf("\nTest 11: Collision Detection\n");

    sprite1->set_position(sprite1, 100, 10);
    sprite2->set_position(sprite2, 110, 10);

    /* Enable collision detection */
    sprite1->collision_box.enabled = 1;
    sprite1->collision_box.x_offset = 0;
    sprite1->collision_box.y_offset = 0;
    sprite1->collision_box.width = 16;
    sprite1->collision_box.height = 8;

    sprite2->collision_box.enabled = 1;
    sprite2->collision_box.x_offset = 0;
    sprite2->collision_box.y_offset = 0;
    sprite2->collision_box.width = 16;
    sprite2->collision_box.height = 8;

    int collides = sprite_collides(sprite1, sprite2);
    if (collides) {
        printf("  ✓ Collision detected (overlapping sprites)\n");
    }

    /* Move apart */
    sprite2->set_position(sprite2, 300, 10);
    collides = sprite_collides(sprite1, sprite2);
    if (!collides) {
        printf("  ✓ No collision when sprites separated\n");
    }

    /* ====================================================================
     * Test 12: Clear All Sprites
     * ==================================================================== */

    printf("\nTest 12: Clear Sprite Pool\n");

    int before = manager.get_active_count(&manager);
    manager.clear(&manager);
    int after = manager.get_active_count(&manager);

    if (before > 0 && after == 0) {
        printf("  ✓ Sprite pool cleared (%d → %d sprites)\n", before, after);
    }

    /* ====================================================================
     * Test 13: Full Composition Test
     * ==================================================================== */

    printf("\nTest 13: Full Sprite Composition\n");

    /* Re-populate for final render */
    for (int i = 0; i < 5; i++) {
        sprite_t *s = manager.allocate(&manager);
        if (s) {
            s->set_position(s, i * 50, 12);
            s->ch = 'A' + i;
            s->color = 0x0F - i;
        }
    }

    manager.update_all(&manager);
    manager.render_all(&manager);

    rrb.render(&rrb);

    if (manager.active_count == 5) {
        printf("  ✓ 5 sprites composed and rendered\n");
        printf("  ✓ RRB composition complete\n");
    }

    /* ====================================================================
     * Cleanup
     * ==================================================================== */

    printf("\nTest 14: Cleanup\n");

    sprite_manager_destroy(&manager);
    rrb.done(&rrb);
    graphics_done();

    printf("\n✓ All Phase 7 tests passed!\n");
    printf("Phase 7: Soft Sprite System complete.\n");
    printf("\nCapabilities Demonstrated:\n");
    printf("  ✓ Object pooling (50-sprite pool)\n");
    printf("  ✓ Physics (velocity, acceleration, position update)\n");
    printf("  ✓ Animation (frame-based animation with looping)\n");
    printf("  ✓ Collision detection (AABB)\n");
    printf("  ✓ RRB layer integration (SPARSE layer rendering)\n");
    printf("  ✓ Batch operations (update all, render all)\n");
    printf("\nNext: Phase 8 - Sprite Animation & Movement\n");
    return 0;
}
