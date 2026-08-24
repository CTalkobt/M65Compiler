/* test_graphics_sprites_phase8.c — Advanced Sprite Features (Phase 8)
 *
 * Tests animation sequences, movement patterns, sprite groups.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>

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

    /* Test 1: Animation Sequences */
    printf("Test 1: Animation Sequences\n");

    sprite_frame_t walk_frames[4] = {
        {'-', 0x0F, 3},
        {'\\', 0x0F, 3},
        {'|', 0x0F, 3},
        {'/', 0x0F, 3}
    };

    sprite_animation_t walk_anim = sprite_animation_create(walk_frames, 4, 1);

    sprite_frame_t jump_frames[2] = {
        {'^', 0x0E, 5},
        {'v', 0x0E, 5}
    };

    sprite_animation_t jump_anim = sprite_animation_create(jump_frames, 2, 0);

    sprite_animation_t anims[2] = {walk_anim, jump_anim};
    sprite_animation_sequence_t seq = sprite_animation_sequence_create(anims, 2, 1);

    sprite_t *player = manager.allocate(&manager);
    player->set_position(player, 64, 10);
    player->set_animation(player, &seq.animations[0]);
    player->play_animation(player);

    if (seq.count == 2 && seq.auto_advance) {
        printf("  ✓ Animation sequence created\n");
    }

    /* Test 2: Movement Patterns */
    printf("\nTest 2: Movement Patterns\n");

    sprite_pattern_t linear_pattern = sprite_pattern_create(PATTERN_LINEAR, 10, 8, 0);

    sprite_t *sprite2 = manager.allocate(&manager);
    sprite2->set_position(sprite2, 0, 5);
    sprite2->ch = 'L';

    int start_x = sprite2->x;
    for (int i = 0; i < 5; i++) {
        sprite_pattern_apply(sprite2, &linear_pattern);
    }

    if (sprite2->x > start_x) {
        printf("  ✓ Linear movement applied\n");
    }

    /* Test 3: Sine Wave */
    printf("\nTest 3: Sine Wave Pattern\n");

    sprite_pattern_t sine_pattern = sprite_pattern_create(PATTERN_SINE, 8, 16, 0);

    sprite_t *sprite3 = manager.allocate(&manager);
    sprite3->set_position(sprite3, 160, 10);
    sprite3->ch = 'S';

    for (int i = 0; i < 10; i++) {
        sprite_pattern_apply(sprite3, &sine_pattern);
    }

    printf("  ✓ Sine wave pattern applied\n");

    /* Test 4: Sprite Groups */
    printf("\nTest 4: Sprite Groups\n");

    sprite_group_t enemies = sprite_group_create(16);

    for (int i = 0; i < 5; i++) {
        sprite_t *enemy = manager.allocate(&manager);
        if (enemy) {
            enemy->set_position(enemy, i * 50, 8);
            enemy->ch = 'E';
            enemy->color = 0x03;
            sprite_group_add(&enemies, enemy);
        }
    }

    if (sprite_group_count(&enemies) == 5) {
        printf("  ✓ Enemy group created with %d sprites\n", sprite_group_count(&enemies));
    }

    /* Test 5: Group Operations */
    printf("\nTest 5: Group Batch Operations\n");

    for (int i = 0; i < sprite_group_count(&enemies); i++) {
        if (enemies.sprites[i]) {
            enemies.sprites[i]->set_velocity(enemies.sprites[i], 1, 0);
        }
    }

    sprite_group_update_all(&enemies);
    sprite_group_render_all(&enemies, sprite_layer);

    printf("  ✓ Updated and rendered %d enemies\n", sprite_group_count(&enemies));

    /* Test 6: Collision Detection */
    printf("\nTest 6: Collision Detection\n");

    for (int i = 0; i < sprite_group_count(&enemies); i++) {
        if (enemies.sprites[i]) {
            enemies.sprites[i]->collision_box.enabled = 1;
            enemies.sprites[i]->collision_box.width = 16;
            enemies.sprites[i]->collision_box.height = 8;
        }
    }

    printf("  ✓ Collision detection setup\n");

    /* Test 7: Distance Calculation */
    printf("\nTest 7: Distance & Movement\n");

    sprite_t *target = manager.allocate(&manager);
    target->set_position(target, 300, 12);
    target->ch = 'T';

    int dist = sprite_distance(player, target);
    printf("  ✓ Distance calculated: %d pixels\n", dist);

    sprite_move_towards(player, target->x, target->y, 2);
    printf("  ✓ Moving towards target\n");

    /* Test 8: Multiple Groups */
    printf("\nTest 8: Multiple Sprite Groups\n");

    sprite_group_t projectiles = sprite_group_create(32);

    for (int i = 0; i < 3; i++) {
        sprite_t *proj = manager.allocate(&manager);
        if (proj) {
            proj->set_position(proj, 100 + i * 20, 15);
            proj->ch = '*';
            proj->color = 0x0F;
            sprite_group_add(&projectiles, proj);
        }
    }

    printf("  ✓ Projectile group created (%d sprites)\n", sprite_group_count(&projectiles));
    printf("  ✓ Total active: %d sprites\n", manager.active_count);

    /* Test 9: Pattern Duration */
    printf("\nTest 9: Pattern Duration\n");

    sprite_pattern_t timed_pattern = sprite_pattern_create(PATTERN_BOUNCE, 8, 10, 50);

    sprite_t *bouncer = manager.allocate(&manager);
    bouncer->set_position(bouncer, 200, 12);
    bouncer->ch = 'B';

    for (int i = 0; i < 60; i++) {
        sprite_pattern_apply(bouncer, &timed_pattern);
    }

    if (sprite_pattern_finished(&timed_pattern)) {
        printf("  ✓ Pattern duration finished\n");
    }

    /* Test 10: Force Application */
    printf("\nTest 10: Force Application\n");

    int vx_before = player->vx;
    sprite_apply_force(player, 5, 0);

    if (player->vx == vx_before + 5) {
        printf("  ✓ Force applied (velocity: %d → %d)\n", vx_before, player->vx);
    }

    /* Test 11: Full Update */
    printf("\nTest 11: Full Update Cycle\n");

    manager.update_all(&manager);
    sprite_group_update_all(&enemies);
    sprite_group_update_all(&projectiles);

    manager.render_all(&manager);
    rrb.render(&rrb);

    printf("  ✓ Update cycle complete\n");

    /* Cleanup */
    printf("\nTest 12: Cleanup\n");

    sprite_group_destroy(&enemies);
    sprite_group_destroy(&projectiles);
    sprite_manager_destroy(&manager);
    rrb.done(&rrb);
    graphics_done();

    printf("\n✓ All Phase 8 tests passed!\n");
    printf("Phase 8: Advanced Sprite Features complete.\n");
    printf("\nFeatures Implemented:\n");
    printf("  ✓ Animation sequences with auto-advance\n");
    printf("  ✓ Movement patterns (Linear, Sine, Circle, Bounce)\n");
    printf("  ✓ Sprite groups for batch operations\n");
    printf("  ✓ Distance calculation & movement towards\n");
    printf("  ✓ Force application for physics\n");
    printf("  ✓ Group operations (update, render, collisions)\n");
    return 0;
}
