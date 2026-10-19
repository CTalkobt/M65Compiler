/* test_bp_cpuport_xemu.c — with the base page moved off page 0, base-page
 * $00/$01 are RAM (the program's __zp_page), not the CPU port: the port
 * answers only at CPU address $0000/$0001
 *
 * Linked with test_bp_cpuport.s45 (bp_cpuport, which never writes the port
 * a value other than its own).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] base-page $00/$01 read the RAM at __zp_page+0/+1
 *   [1] writes to base-page $00/$01 land in that RAM
 *   [2] the port at $0000/$0001 reads the same before and after
 *   [3] $AA sentinel
 */
void bp_cpuport(void);

int main(void) {
    bp_cpuport();
    return 0;
}
