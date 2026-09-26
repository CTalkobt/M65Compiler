// Test array initialization with braced initializer lists
// Results transmitted via Ethernet instead of to memory
// Sends a raw Ethernet frame with test results as payload
__asm__(".no_zp_save");

#include <stdio.h>
#include <string.h>
#include <mega65.h>

// Ethernet frame buffer at FFDE800 (RX buffer) / FFDE800+ (TX buffer)
// Note: actual address may differ per hardware revision
#define ETH_TX_BUFFER ((volatile unsigned char *)0xFFDE800)

// Global arrays with initializer lists
char bytes[4] = {0x10, 0x20, 0x30, 0x40};
int words[3] = {100, 200, 300};
int partial[5] = {11, 22};
char zeros[3] = {};

void send_ethernet_frame(const unsigned char *data, int len) {
    // Copy test results to ethernet TX buffer
    volatile unsigned char *txbuf = ETH_TX_BUFFER;

    for (int i = 0; i < len && i < 1500; i++) {
        txbuf[i] = data[i];
    }

    // Set TX frame size (low byte, high byte)
    eth->txszlo = len & 0xFF;
    eth->txszhi = (len >> 8) & 0xFF;

    // Trigger transmission (would need proper implementation per hardware)
    // eth->ctrl |= ETH_TXEN;
}

void main() {
    // Local array with initializer list
    char local_bytes[3] = {0xAA, 0xBB, 0xCC};
    int local_words[2] = {1000, 2000};

    // Build ethernet frame payload: [test_id, expected, actual, ...]
    unsigned char frame[128] = {0};
    int frame_idx = 0;

    // Frame header (simple format: magic, test count, results)
    frame[frame_idx++] = 0xA5;  // Magic: array init test
    frame[frame_idx++] = 15;    // Number of test values

    // Test 1: global char array
    frame[frame_idx++] = bytes[0];   // 0x10
    frame[frame_idx++] = bytes[3];   // 0x40

    // Test 2: global int array (low bytes)
    frame[frame_idx++] = words[0];   // 100 = 0x64
    frame[frame_idx++] = words[2];   // 300 = 0x2C (low byte of 0x012C)

    // Test 3: partial init - initialized elements
    frame[frame_idx++] = partial[0]; // 11 = 0x0B
    frame[frame_idx++] = partial[1]; // 22 = 0x16

    // Test 4: partial init - zero-filled elements
    frame[frame_idx++] = partial[2]; // 0
    frame[frame_idx++] = partial[4]; // 0

    // Test 5: empty init - all zeros
    frame[frame_idx++] = zeros[0];   // 0
    frame[frame_idx++] = zeros[2];   // 0

    // Test 6: local char array
    frame[frame_idx++] = local_bytes[0]; // 0xAA
    frame[frame_idx++] = local_bytes[2]; // 0xCC

    // Test 7: local int array (low bytes)
    frame[frame_idx++] = local_words[0]; // 1000 = 0xE8 (low byte of 0x03E8)
    frame[frame_idx++] = local_words[1]; // 2000 = 0xD0 (low byte of 0x07D0)

    // Terminator
    frame[frame_idx++] = 0xFF;

    // Send via ethernet
    send_ethernet_frame(frame, frame_idx);

    // Also write to memory for compatibility with mmemu testing
    volatile char *result = (char *)0x4000;
    for (int i = 0; i < frame_idx && i < 64; i++) {
        result[i] = frame[i];
    }
}
