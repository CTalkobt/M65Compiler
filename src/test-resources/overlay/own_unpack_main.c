/* A program's own unpack(), in a code overlay, linked with
 * --pack-overlays (#346): the overlay loader unpacks with the library's
 * __unpack, not with this one (which ran before any overlay was loaded). */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char own_result;
void unpack(unsigned long src);

int main(void) {
    unpack(0x1234UL);
    OUT[0] = own_result == 0x5A;
    OUT[1] = 0xAA;
    return 0;
}
