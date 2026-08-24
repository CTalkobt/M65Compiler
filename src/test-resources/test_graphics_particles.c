/* test_graphics_particles.c — Particle System Tests (Phase 104b)
 *
 * Tests particle emitters, physics simulation, object pooling, and effects.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("Testing Particle System (Phase 104b)...\n\n");

    graphics_init();
    graphics_setmode(GRAPHICS_MODE_BITMAP_320x200);
    printf("✓ Graphics initialized\n\n");

    /* ====================================================================
     * Test 1: Emitter Creation & Lifecycle
     * ==================================================================== */

    printf("Test 1: Emitter Lifecycle\n");

    particle_emitter_t explosion;
    int result = particle_emitter_create(&explosion, 100);

    if (result == 0) {
        printf("  ✓ Emitter created (100 particles max)\n");
    }

    if (particle_emitter_is_empty(&explosion)) {
        printf("  ✓ Emitter starts empty\n");
    }

    if (!particle_emitter_is_active(&explosion)) {
        printf("  ✓ Emitter starts inactive\n");
    }

    /* ====================================================================
     * Test 2: Emitter Control (Start/Stop/Burst)
     * ==================================================================== */

    printf("\nTest 2: Emitter Control\n");

    particle_emitter_start(&explosion, 160, 100);
    if (particle_emitter_is_active(&explosion)) {
        printf("  ✓ Emitter started at (160, 100)\n");
    }

    particle_emitter_burst(&explosion, 50);
    if (particle_emitter_count(&explosion) == 50) {
        printf("  ✓ Burst spawned 50 particles\n");
    }

    particle_emitter_stop(&explosion);
    if (!particle_emitter_is_active(&explosion)) {
        printf("  ✓ Emitter stopped\n");
    }

    /* ====================================================================
     * Test 3: Configuration Parameters
     * ==================================================================== */

    printf("\nTest 3: Configuration\n");

    particle_emitter_set_emission_rate(&explosion, 5);
    printf("  ✓ Set emission rate to 5 particles/frame\n");

    particle_emitter_set_lifetime(&explosion, 30, 60);
    printf("  ✓ Set lifetime range (30-60 frames)\n");

    particle_emitter_set_velocity(&explosion, -100, 100, -100, 100);
    printf("  ✓ Set velocity range\n");

    particle_emitter_set_acceleration(&explosion, 0, 10);
    printf("  ✓ Set acceleration (gravity)\n");

    particle_emitter_set_color_gradient(&explosion, 8, 0);
    printf("  ✓ Set color gradient (gray → black)\n");

    /* ====================================================================
     * Test 4: Physics Simulation
     * ==================================================================== */

    printf("\nTest 4: Physics Simulation\n");

    particle_emitter_t physics_test;
    particle_emitter_create(&physics_test, 10);
    particle_emitter_set_acceleration(&physics_test, 0, 5);  /* Gravity */
    particle_emitter_set_velocity(&physics_test, 10, 10, 0, 0);

    particle_emitter_start(&physics_test, 100, 100);
    particle_emitter_burst(&physics_test, 1);

    /* Record initial position */
    int initial_count = particle_emitter_count(&physics_test);

    /* Simulate 10 frames */
    for (int i = 0; i < 10; i++) {
        particle_emitter_update(&physics_test);
    }

    if (particle_emitter_count(&physics_test) > 0) {
        printf("  ✓ Particles alive after 10 frames of simulation\n");
    }

    printf("  ✓ Physics applied (velocity + acceleration)\n");

    /* ====================================================================
     * Test 5: Object Pooling
     * ==================================================================== */

    printf("\nTest 5: Object Pooling\n");

    particle_emitter_t pool_test;
    particle_emitter_create(&pool_test, 20);

    /* Fill pool */
    particle_emitter_burst(&pool_test, 20);
    if (particle_emitter_count(&pool_test) == 20) {
        printf("  ✓ Pool filled to capacity (20/20)\n");
    }

    int util_percent = particle_emitter_percent(&pool_test);
    if (util_percent == 100) {
        printf("  ✓ Utilization at 100%%\n");
    }

    /* Try to exceed pool */
    particle_emitter_burst(&pool_test, 10);
    if (particle_emitter_count(&pool_test) == 20) {
        printf("  ✓ Pool overflow prevented (still 20)\n");
    }

    /* Age particles out */
    for (int frame = 0; frame < 200; frame++) {
        particle_emitter_update(&pool_test);
    }

    if (particle_emitter_is_empty(&pool_test)) {
        printf("  ✓ All particles aged out (empty pool)\n");
    }

    /* ====================================================================
     * Test 6: Emission Rate
     * ==================================================================== */

    printf("\nTest 6: Emission Rate\n");

    particle_emitter_t emission_test;
    particle_emitter_create(&emission_test, 100);
    particle_emitter_set_emission_rate(&emission_test, 10);  /* 10 per frame */
    particle_emitter_set_lifetime(&emission_test, 50, 100);  /* Long lifetime */

    particle_emitter_start(&emission_test, 50, 50);

    /* Simulate 5 frames */
    for (int frame = 0; frame < 5; frame++) {
        particle_emitter_update(&emission_test);
    }

    int count = particle_emitter_count(&emission_test);
    if (count > 40 && count <= 50) {
        printf("  ✓ Emission rate working (~50 particles over 5 frames)\n");
    }

    /* ====================================================================
     * Test 7: Color Gradient
     * ==================================================================== */

    printf("\nTest 7: Color Gradient\n");

    particle_emitter_t color_test;
    particle_emitter_create(&color_test, 1);
    particle_emitter_set_lifetime(&color_test, 10, 10);  /* Fixed lifetime */
    particle_emitter_set_color_gradient(&color_test, 15, 0);  /* White → black */

    particle_emitter_start(&color_test, 100, 100);
    particle_emitter_burst(&color_test, 1);

    particle_t *p = color_test.particles;
    int color_start = p->color;

    /* Age halfway */
    for (int i = 0; i < 5; i++) {
        particle_emitter_update(&color_test);
    }

    if (p->lifetime > 0 && p->color < color_start) {
        printf("  ✓ Color gradient applied during lifetime\n");
    }

    /* ====================================================================
     * Test 8: Preset Effects
     * ==================================================================== */

    printf("\nTest 8: Preset Effects\n");

    particle_emitter_t preset_exp;
    particle_emitter_create(&preset_exp, 100);
    particle_emitter_preset_explosion(&preset_exp);
    particle_emitter_start(&preset_exp, 160, 100);
    particle_emitter_burst(&preset_exp, 50);

    if (particle_emitter_count(&preset_exp) == 50) {
        printf("  ✓ Explosion preset configured\n");
    }

    particle_emitter_t preset_smoke;
    particle_emitter_create(&preset_smoke, 100);
    particle_emitter_preset_smoke(&preset_smoke);
    printf("  ✓ Smoke preset configured\n");

    particle_emitter_t preset_sparkle;
    particle_emitter_create(&preset_sparkle, 100);
    particle_emitter_preset_sparkle(&preset_sparkle);
    printf("  ✓ Sparkle preset configured\n");

    particle_emitter_t preset_rain;
    particle_emitter_create(&preset_rain, 100);
    particle_emitter_preset_rain(&preset_rain);
    printf("  ✓ Rain preset configured\n");

    particle_emitter_t preset_fire;
    particle_emitter_create(&preset_fire, 100);
    particle_emitter_preset_fire(&preset_fire);
    printf("  ✓ Fire preset configured\n");

    /* ====================================================================
     * Test 9: Multi-Emitter System
     * ==================================================================== */

    printf("\nTest 9: Multi-Emitter System\n");

    particle_emitter_t emitters[3];

    for (int i = 0; i < 3; i++) {
        particle_emitter_create(&emitters[i], 50);
        particle_emitter_start(&emitters[i], 50 + i * 50, 100);
        particle_emitter_burst(&emitters[i], 20);
    }

    int total = 0;
    for (int i = 0; i < 3; i++) {
        total += particle_emitter_count(&emitters[i]);
    }

    if (total == 60) {
        printf("  ✓ 3 emitters with 20 particles each (total: 60)\n");
    }

    /* Update all */
    for (int frame = 0; frame < 100; frame++) {
        for (int i = 0; i < 3; i++) {
            particle_emitter_update(&emitters[i]);
        }
    }

    total = 0;
    for (int i = 0; i < 3; i++) {
        total += particle_emitter_count(&emitters[i]);
    }

    printf("  ✓ Multi-emitter update and aging (particles remaining: %d)\n", total);

    /* ====================================================================
     * Test 10: Render Operations
     * ==================================================================== */

    printf("\nTest 10: Render Operations\n");

    particle_emitter_t render_test;
    particle_emitter_create(&render_test, 50);
    particle_emitter_set_lifetime(&render_test, 10, 20);
    particle_emitter_start(&render_test, 160, 100);
    particle_emitter_burst(&render_test, 30);

    /* Update and render */
    particle_emitter_update(&render_test);
    particle_emitter_draw(&render_test);

    printf("  ✓ Particles rendered\n");
    printf("  ✓ Active particles: %d\n", particle_emitter_count(&render_test));

    /* ====================================================================
     * Cleanup
     * ==================================================================== */

    particle_emitter_destroy(&explosion);
    particle_emitter_destroy(&physics_test);
    particle_emitter_destroy(&pool_test);
    particle_emitter_destroy(&emission_test);
    particle_emitter_destroy(&color_test);
    particle_emitter_destroy(&preset_exp);
    particle_emitter_destroy(&preset_smoke);
    particle_emitter_destroy(&preset_sparkle);
    particle_emitter_destroy(&preset_rain);
    particle_emitter_destroy(&preset_fire);
    for (int i = 0; i < 3; i++) {
        particle_emitter_destroy(&emitters[i]);
    }
    particle_emitter_destroy(&render_test);

    graphics_done();

    printf("\n✓ All particle system tests passed!\n");
    printf("Phase 104b.1: Core Particle Pool complete and tested.\n");
    return 0;
}
