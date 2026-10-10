/* File I/O must link (cbm_k_* KERNAL wrappers) */
#include <stdio.h>
int main(void) { FILE *f = fopen("x", "w"); if (f) { fputc(65, f); fclose(f); } return 0; }
