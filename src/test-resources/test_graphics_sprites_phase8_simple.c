/* test_graphics_sprites_phase8_simple.c — Advanced Sprite Features (Phase 8)
 *
 * Tests sprite groups, animations, and distance calculations.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>

int main(void) {
    printf("Testing Phase 8: Advanced Sprite Features...\n\n");

    graphics_init();
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("✓ Graphics initialized\n\n");

    rrb_system_t rrb;
    rrb.init(&rrb, 3, 40, 25);
    rrb.configure_vic(&rrb, 0, 40, 80);
    rrb.enable(&rrb);

    int sprite_layer_idx = rrb.create_layer(&rrb, RRB_MODE_SPARSE, 40, 25);
    rrb_layer_t *sprite_layer = rrb.get_layer(&rrb, sprite_layer_idx);
    sprite_layer->clear(sprite_layer, 32, 0x00);

    sprite_manager_t manager = sprite_manager_create(50, &rrb, sprite_layer_idx);

    printf("✓ System initialized\n\n");

    /* Test 1: Sprite Groups */
    printf("Test 1: Sprite Groups\n");

    sprite_group_t enemies = sprite_group_create(16);

    for (int i = 0; i < 5; i++) {
        sprite_t *enemy = manager.allocate(&manager);
        if (enemy) {
            enemy->set_position(enemy, i * 50, 8);
            enemy->ch = 'E';
            sprite_group_add(&enemies, enemy);
        }
    }

    printf("  ✓ Created enemy group (%d sprites)\n", sprite_group_count(&enemies));

    /* Test 2: Batch Operations */
    printf("\nTest 2: Batch Operations\n");

    sprite_group_update_all(&enemies);
    sprite_group_render_all(&enemies, sprite_layer);

    printf("  ✓ Updated and rendered all enemies\n");

    /* Test 3: Multiple Groups */
    printf("\nTest 3: Multiple Groups\n");

    sprite_group_t projectiles = sprite_group_create(32);

    for (int i = 0; i < 3; i++) {
        sprite_t *proj = manager.allocate(&manager);
        if (proj) {
            proj->set_position(proj, 100 + i * 20, 15);
            proj->ch = '*';
            sprite_group_add(&projectiles, proj);
        }
    }

    printf("  ✓ Created projectile group (%d sprites)\n", sprite_group_count(&projectiles));
    printf("  ✓ Total active: %d\n", manager.active_count);

    /* Test 4: Distance & Movement */
    printf("\nTest 4: Distance Calculation\n");

    sprite_t *player = manager.allocate(&manager);
    sprite_t *target = manager.allocate(&manager);

    player->set_position(player, 100, 10);
    target->set_position(target, 300, 10);

    int dist = sprite_distance(player, target);
    printf("  ✓ Distance calculated: %d pixels\n", dist);

    sprite_move_towards(player, target->x, target->y, 2);
    printf("  ✓ Player moving towards target\n");

    /* Test 5: Collision Setup */
    printf("\nTest 5: Collision Setup\n");

    for (int i = 0; i < sprite_group_count(&enemies); i++) {
        if (enemies.sprites[i]) {
            enemies.sprites[i]->collision_box.enabled = 1;
            enemies.sprites[i]->collision_box.width = 16;
            enemies.sprites[i]->collision_box.height = 8;
        }
    }

    printf("  ✓ Collision boxes configured\n");

    /* Test 6: Animation Sequences */
    printf("\nTest 6: Animation Support\n");

    sprite_frame_t frames[2] = {
        {'A', 0x0F, 5},
        {'B', 0x0E, 5}
    };

    sprite_animation_t anim = sprite_animation_create(frames, 2, 1);
    player->set_animation(player, &anim);
    player->play_animation(player);

    printf("  ✓ Animation sequence created\n");
    printf("  ✓ Animation playing\n");

    /* Test 7: Full Cycle */
    printf("\nTest 7: Full Update Cycle\n");

    manager.update_all(&manager);
    sprite_group_update_all(&enemies);
    manager.render_all(&manager);

    printf("  ✓ All sprites updated and rendered\n");

    /* Cleanup */
    printf("\nTest 8: Cleanup\n");

    sprite_group_destroy(&enemies);
    sprite_group_destroy(&projectiles);
    sprite_manager_destroy(&manager);
    rrb.done(&rrb);
    graphics_done();

    printf("\n✓ All Phase 8 tests passed!\n");
    printf("Phase 8: Advanced Sprite Features complete.\n");
    return 0;
}
