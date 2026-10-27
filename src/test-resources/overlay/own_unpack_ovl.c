/* own_unpack_main.c's unpack(), in overlay "one", with a constant large
 * enough that the overlay file is packed (#346) */
#pragma cc45 overlay(one)

extern unsigned char own_result;
static const unsigned char filler[2048] = {1};

void unpack(unsigned long src) {
    own_result = (src == 0x1234UL && filler[0] == 1) ? 0x5A : 0;
}
