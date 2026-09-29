/**
 * Phase 90.5 Benchmark: Callback/Event Handler Patterns
 *
 * Measures code size savings for functions making zero-arg calls.
 * Expected: ~9 bytes saved per zero-arg call (no FP recalculation)
 */

// Zero-arg helper functions
void init_display(void) {}
void clear_screen(void) {}
void refresh_display(void) {}
void update_status(void) {}
void flush_buffer(void) {}
void check_timeout(void) {}
void process_queue(void) {}
void handle_interrupt(void) {}

// Event handlers making zero-arg calls
void on_frame_update(void) {
    update_status();
    refresh_display();
}

void on_input_received(void) {
    process_queue();
    check_timeout();
    refresh_display();
}

void on_timer_tick(void) {
    handle_interrupt();
    update_status();
    flush_buffer();
}

void on_display_ready(void) {
    clear_screen();
    init_display();
    refresh_display();
}

void on_shutdown(void) {
    flush_buffer();
    clear_screen();
}

// Complex event pattern with loops
void poll_events(int count) {
    for (int i = 0; i < count; i++) {
        check_timeout();
        process_queue();
    }
    refresh_display();
}

// Nested event calls
void cascade_updates(void) {
    update_status();
    on_frame_update();
    on_input_received();
    refresh_display();
}

int main(void) {
    init_display();
    on_frame_update();
    on_input_received();
    on_timer_tick();
    on_display_ready();
    on_shutdown();
    poll_events(5);
    cascade_updates();
    return 0;
}
