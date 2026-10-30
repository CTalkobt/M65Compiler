// SidFile.hpp — a C64 tune in a PSID/RSID file (.sid): the file read, and
// the tune run on an emulated 6502 so the SID register writes of its init
// and play routines can be recorded (for tune45 import of a .sid).
//
// The PSID header (big-endian words): "PSID"/"RSID", version (1-4), the
// data's offset (0x76 in v1, 0x7C in v2+), the load address (0: the data's
// first two bytes, little-endian, give it), init, play (0: the tune
// installs its own interrupt handler), songs, the start song, the speed
// bits (bit n: song n+1 is timed by a CIA timer, else the 50/60 Hz
// raster; songs past 32 use bit 31), name, author and released (32 bytes
// each, Latin-1), and in v2+ the flags (bit 0 Compute!'s Sidplayer MUS
// data, bit 1 PlaySID-specific / an RSID's C64 BASIC flag, bits 2-3 the
// clock: 1 PAL, 2 NTSC, 3 either; bits 4-5 the SID: 1 6581, 2 8580, 3
// either), a relocation page and length, and (v3/v4) the second and third
// SID's address ($Dxx0 as the byte xx).
//
// The machine runPsid emulates (see SidFile.cpp for the detail):
// - a cycle-counting NMOS 6502: every documented opcode, decimal mode as
//   the NMOS part does it, the stable undocumented opcodes (SLO RLA SRE
//   RRA SAX LAX DCP ISC ANC ALR ARR SBX, SBC #$EB, the NOPs), the unstable
//   ones approximated (XAA and LAX # with the magic constant $EE, LAS, and
//   SHA/SHX/SHY/TAS storing reg & (high byte + 1) without the address
//   quirk); a JAM opcode stops the run with an error. RMW instructions do
//   not repeat the NMOS's dummy write of the old value.
// - 64 KB of RAM, no ROMs: where the tune loaded nothing there are stubs
//   for the KERNAL — an RTS at each jump table entry $FF81-$FFF3, the IRQ
//   entry at $FF48 (pushes A X Y, JMP ($0314)), the IRQ exits $EA31,
//   $EA81 and $FEBC (reaching one ends a play call, like an RTI), the
//   vectors ($0314 = $EA31, $FFFE = $FF48), and $02A6 the PAL/NTSC flag.
//   A jump to any other address where nothing was loaded or written (a
//   BASIC or KERNAL routine) stops the run with an error.
// - the $01 port: I/O ($D000-$DFFF) is visible as on a C64 (CHAREN set
//   and LORAM or HIRAM set), RAM beneath it otherwise; the ROMs are RAM.
//   $01 is set before init as sidplay does: $37, or $36 when init or play
//   is in $A000-$CFFF, $35 when in $E000-$FFFF, $34 in $D000-$DFFF.
// - the SID, $D400-$D7FF mirrored every $20 (and $DE00-$DFFF when a second
//   or third SID is put there): writes recorded; reads give 0 except
//   $D419/$D41A (paddles: $FF), $D41B (osc 3: pseudo-random, an LFSR) and
//   $D41C (env 3: 0).
// - the VIC: $D011 bit 7 / $D012 give a raster line that advances with
//   the cycles (63 a line, 312 lines PAL; 65 and 263 NTSC); $D019 reads
//   $F1 (the raster interrupt pending); the rest read back what was
//   written.
// - CIA 1 and 2: registers kept; a timer's latch ($DC04/5, $DD04/5) is
//   what was written, its counter reads count down from it while the timer
//   runs; $DC0D/$DD0D read $81. No interrupt ever fires on its own: init
//   and play are called, and the stack and PC set up for them.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct PsidFile {
    std::string magic;               // "PSID" or "RSID"
    int version = 0;
    uint16_t dataOffset = 0;
    uint16_t load = 0, init = 0, play = 0;   // load: the real one (from the data when the header's is 0); init 0 → load
    int songs = 1, startSong = 1;
    uint32_t speed = 0;              // bit n: song n+1 timed by a CIA timer (else the 50/60 Hz raster)
    std::string name, author, released;   // UTF-8 (the file's Latin-1 converted)
    uint16_t flags = 0;              // v2+: bit 0 MUS, bit 1 PlaySID/BASIC, bits 2-3 clock, 4-5 SID model
    uint8_t startPage = 0, pageLength = 0;
    uint16_t sid2 = 0, sid3 = 0;     // v3/v4: the second and third SID's address (0: none)
    std::vector<uint8_t> data;       // the C64 data, loaded at `load` (the load address bytes removed)

    bool rsid() const { return magic == "RSID"; }
    bool ntsc() const { return ((flags >> 2) & 3) == 2; }      // NTSC only; PAL, either or unknown → PAL
    bool mos8580() const { return ((flags >> 4) & 3) == 2; }   // 8580 only; else the 6581
    bool mus() const { return flags & 1; }
    bool basic() const { return rsid() && (flags & 2); }
    bool songUsesCia(int song) const {                          // song from 1
        int bit = song < 1 ? 0 : song > 32 ? 31 : song - 1;
        return (speed >> bit) & 1;
    }
};

// parse a .sid file's bytes; false with `error` set when it is not a PSID/RSID
// or is malformed (data missing, running past $FFFF ...)
bool readPsid(const std::vector<uint8_t>& file, PsidFile& p, std::string& error);

struct SidWrite {
    uint8_t reg, value;              // reg 0-0x1C (the $D400 mirrors folded; $1D-$1F are not recorded)
    uint8_t chip = 0;                // 0 the SID at $D400; 1/2 the header's second/third SID
};
struct SidCall {
    std::vector<SidWrite> writes;    // the writes of one call of the routine, in order
    uint32_t cycles = 0;             // the cycles it took (to its RTS/RTI or KERNAL exit)
};
struct SidRun {
    double callsPerSecond = 50;      // 50 PAL VBI, 60 NTSC VBI, or CIA 1 timer A's rate: clock / (latch + 1)
    bool cia = false;                // timed by CIA 1 timer A (else the raster)
    uint16_t ciaLatch = 0;           // timer A's latch after init (when cia)
    double clock = 985248;           // the CPU clock: 985248 PAL, 1022727 NTSC
    uint16_t playAddress = 0;        // what was called each time: `play`, or the interrupt handler init installed
    bool irq = false;                // called as an interrupt handler (play 0): to its RTI or a KERNAL IRQ exit
    SidCall init;                    // init's writes — NOT in calls
    std::vector<SidCall> calls;      // the play calls, calls[0] the first after init
    std::array<uint8_t, 0x20> initRegs{};   // the SID's registers after init (those never written: 0)
};

// cycle limits for one call (beyond them the run fails: the tune is looping or
// is not a routine that returns)
constexpr uint64_t kSidInitCycleLimit = 1000000;
constexpr uint64_t kSidPlayCycleLimit = 50000;

// run init(song) (A = song - 1, songs counted from 1), then the play routine
// `calls` times, recording the SID writes. play 0: the interrupt handler init
// installed is called as an IRQ — through $FFFE when the KERNAL is banked out
// and init changed it, else through $0314 (as the KERNAL's IRQ entry: A X Y
// pushed, ending at its RTI or a jump to $EA31/$EA81/$FEBC), else $FFFE.
// The vector is looked up again before every call (a tune may chain
// handlers). Timing: a PSID's play routine — its speed bit: CIA 1 timer A's
// latch after init (the KERNAL's $4025 PAL / $4295 NTSC when init leaves
// it), else 50/60 Hz; play 0 — CIA when init left timer A running with its
// interrupt enabled and the VIC's raster interrupt off, the raster when the
// opposite, else the speed bit.
// An RSID is run the same way (it fails if its init never returns, as many
// do: then "needs a real C64"); one flagged as BASIC, and MUS data, are
// refused. false with `error` set: a bad song, no play routine, a JAM, a
// cycle limit hit, a jump into ROM that is not there.
bool runPsid(const PsidFile& p, int song, int calls, SidRun& out, std::string& error);

// The 6502 itself, for tests (Klaus Dormann's functional test runs on it).
class Cpu6502 {
public:
    virtual ~Cpu6502() = default;
    uint8_t a = 0, x = 0, y = 0, s = 0xFF, p = 0x24;
    uint16_t pc = 0;
    uint64_t cycles = 0;
    bool jammed = false;             // a JAM opcode ran: pc at it
    std::array<uint8_t, 0x10000> mem{};

    // run one instruction; its cycles (0 when jammed)
    int step();

protected:
    virtual uint8_t read(uint16_t addr) { return mem[addr]; }
    virtual void write(uint16_t addr, uint8_t v) { mem[addr] = v; }
    void push(uint8_t v) { write(uint16_t(0x100 | s--), v); }
    uint8_t pull() { return read(uint16_t(0x100 | ++s)); }

private:
    int extra_ = 0;                  // page crossing and branch cycles
    uint16_t base_ = 0;              // an indexed mode's base address (for SHA/SHX/SHY/TAS)
    uint8_t fetch() { return read(pc++); }
    uint16_t fetch16() { uint8_t l = fetch(); return uint16_t(l | fetch() << 8); }
    uint16_t ea(int mode, bool penalty);
    void nz(uint8_t v);
    void adc(uint8_t v);
    void sbc(uint8_t v);
    void cmp(uint8_t r, uint8_t v);
    void branch(bool cond);
};
