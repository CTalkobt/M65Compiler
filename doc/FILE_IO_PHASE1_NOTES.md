# FILE I/O Phase 1 Implementation Notes

## Status: In Progress

**Files Created**:
- `lib/include/stdio.h` — Header with FILE struct and API (expanded to 195 lines)
- `lib/src/stdio_file.c` — fopen, fclose, freopen, fflush implementation (260 lines)
- `lib/src/stdio_io.c` — fgetc, fputc, ungetc, fread, fwrite implementation (230 lines)
- `src/test-resources/test_stdio_phase1.c` — Unit tests (180 lines)

**Total Phase 1 Code**: 665 lines of implementation ready

---

## What's Implemented

### ✅ Header File (stdio.h)
- FILE structure definition
- Function prototypes for Phase 1
- Constants and macros (BUFSIZ, EOF, SEEK_*, etc.)
- CBM device and file type definitions
- Standard stream declarations (stdin, stdout, stderr)

### ✅ FILE Management (stdio_file.c)
- **fopen()** — Parse mode, allocate FILE, call KERNAL
- **fclose()** — Flush buffer, close handle, free resources
- **freopen()** — Close and reopen file
- **fflush()** — Flush write buffers
- **feof()** — Check EOF flag
- **ferror()** — Check error flag
- **clearerr()** — Clear EOF/error flags
- **fileno()** — Get file descriptor

### ✅ I/O Operations (stdio_io.c)
- **fgetc()** — Character input with buffering
- **fputc()** — Character output with buffering
- **ungetc()** — Push character back
- **fread()** — Block input with buffering
- **fwrite()** — Block output with buffering

### ✅ Test Suite (test_stdio_phase1.c)
- Stream initialization tests
- Flag tests
- Constant verification tests
- Mode parsing tests
- 8 test functions total

---

## What Still Needs Implementation

### 1. KERNAL Integration (`cbm_k_*` functions)

These wrapper functions need to be implemented to interface with the Commodore KERNAL ROM:

```c
/* In lib/src/stdio_kernal.c */

unsigned char cbm_k_chkin(unsigned char fd) {
    /* Redirect input from fd
     * KERNAL routine at $FF5A
     * Entry: A = logical file number
     * Exit: A = status byte
     */
}

unsigned char cbm_k_chkout(unsigned char fd) {
    /* Redirect output to fd
     * KERNAL routine at $FF5D
     * Entry: A = logical file number
     * Exit: A = status byte
     */
}

unsigned char cbm_k_clrchn(void) {
    /* Clear input/output redirection
     * KERNAL routine at $FF63
     */
}

unsigned char cbm_k_readst(void) {
    /* Read status byte
     * KERNAL routine at $FF5F
     * Returns: status byte (bit 0 = EOF, etc.)
     */
}

unsigned char cbm_k_setnam(const char *filename) {
    /* Set filename for next open
     * KERNAL routine at $FF5D
     * Entry: A/X = low/high address of filename
     * Entry: Y = length of filename
     */
}

unsigned char cbm_k_setlfs(unsigned char fd, unsigned char device, unsigned char secondary) {
    /* Set file parameters for next open
     * KERNAL routine at $FF57
     * Entry: A = logical file number
     * Entry: X = device number (8=disk, 1=tape)
     * Entry: Y = secondary address
     */
}

unsigned char cbm_k_open(void) {
    /* Open file
     * KERNAL routine at $FF5C
     * Uses: setnam, setlfs parameters
     */
}

unsigned char cbm_k_close(unsigned char fd) {
    /* Close file
     * KERNAL routine at $FF60
     * Entry: A = logical file number
     */
}
```

### 2. Low-Level File I/O (cbm_read, cbm_write, cbm_seek)

These need to be implemented in assembly to interface with KERNAL:

```c
/* In lib/src/stdio_cbm.s45 or stdio_cbm.c */

int cbm_read(fd_t fd, void *buffer, unsigned int length) {
    /* Read length bytes from fd into buffer
     * Implementation:
     * 1. Call cbm_k_chkin(fd) to redirect input
     * 2. Loop calling cbm_k_chrin() length times
     * 3. Call cbm_k_clrchn() to restore input
     * 4. Return bytes read (or -1 on error)
     */
}

int cbm_write(fd_t fd, const void *buffer, unsigned int length) {
    /* Write length bytes to fd from buffer
     * Implementation:
     * 1. Call cbm_k_chkout(fd) to redirect output
     * 2. Loop calling cbm_k_chrout() length times
     * 3. Call cbm_k_clrchn() to restore output
     * 4. Return bytes written (or -1 on error)
     */
}

int cbm_seek(fd_t fd, unsigned long position) {
    /* Seek to position in file
     * Note: Not fully supported by all Commodore drives
     * Sequential files (.seq) cannot seek reliably
     * Relative files (.rel) support seeking
     */
}

unsigned long cbm_tell(fd_t fd) {
    /* Get current position in file */
}

int cbm_status(fd_t fd) {
    /* Get status byte from KERNAL readst */
}
```

### 3. Helper Functions in stdio_file.c

Two internal functions are stubbed but need implementation:

```c
/* Already stubbed, needs KERNAL integration */
static int _kernal_open(const char *filename, unsigned char cbm_type, unsigned char flags) {
    /* Implementation steps:
     * 1. Find free logical file number (channel 3-14)
     * 2. Determine device (8 for disk, 1 for tape, 0 for keyboard)
     * 3. Calculate secondary address based on flags
     *    - Read: SA = 0
     *    - Write: SA = 1
     *    - Append: SA = 2
     * 4. Call cbm_k_setnam(filename)
     * 5. Call cbm_k_setlfs(fd, device, SA)
     * 6. Call cbm_k_open()
     * 7. Check cbm_k_readst() for errors
     * 8. Return file handle (3-14) or -1 on error
     */
}

/* Already stubbed, needs buffer management */
static int _flush_write_buffer(FILE *stream) {
    /* Currently just checks flags and calls cbm_write
     * Implementation:
     * 1. Check if stream has write flag
     * 2. Call cbm_write() to write buffer to device
     * 3. Handle partial writes (incomplete flush)
     * 4. Clear buffer pointers after successful flush
     * 5. Return 0 on success, EOF on error
     */
}

/* Already stubbed, needs buffer refill */
static int _fill_read_buffer(FILE *stream) {
    /* Currently calls cbm_read to refill buffer
     * Implementation:
     * 1. Check if stream has read flag
     * 2. Call cbm_read() to read from device into buffer
     * 3. Handle short reads (EOF condition)
     * 4. Set _IOEOF or _IOERR flags as appropriate
     * 5. Reset buffer pointers
     * 6. Return 0 on success, EOF on error
     */
}
```

### 4. Missing cbm_* Functions

Directory and file management functions needed for later phases:

```c
/* Not yet implemented, needed for Phase 4 */

int cbm_opendir(unsigned char device) {
    /* Open directory listing ("$" file)
     * Returns: file descriptor for directory listing
     */
}

int cbm_readdir(fd_t fd, char *filename, unsigned char *type, unsigned int *size) {
    /* Read next directory entry
     * Parse: "filename" type size bytes
     * Returns: 0 at end of directory
     */
}

int cbm_closedir(fd_t fd) {
    /* Close directory listing */
}

int cbm_delete(unsigned char device, const char *filename) {
    /* Delete file using KERNAL scratch command */
}

int cbm_rename(unsigned char device, const char *old_name, const char *new_name) {
    /* Rename file using KERNAL rename command */
}

int cbm_format(unsigned char device, const char *name, const char *id) {
    /* Format disk */
}
```

---

## Implementation Order

### Step 1: KERNAL Wrapper Functions (4-6 hours)
Create `lib/src/stdio_kernal.c` with assembly implementations of:
- cbm_k_chkin, cbm_k_chkout, cbm_k_clrchn
- cbm_k_readst, cbm_k_setnam, cbm_k_setlfs
- cbm_k_open, cbm_k_close

**Deliverable**: Can open and close files via KERNAL

### Step 2: cbm_read / cbm_write (4-6 hours)
Implement in `lib/src/stdio_cbm.c`:
- cbm_read() using CHKIN/CHRIN loop
- cbm_write() using CHKOUT/CHROUT loop
- cbm_status() via READST

**Deliverable**: Can read and write file data

### Step 3: Buffer Management (2-3 hours)
Complete `_fill_read_buffer()` and `_flush_write_buffer()` in stdio_file.c

**Deliverable**: Buffered I/O fully functional

### Step 4: Testing (3-4 hours)
Expand test_stdio_phase1.c with:
- File creation tests
- Read/write tests
- Buffer flushing tests
- Error condition tests

**Deliverable**: Comprehensive test coverage

### Step 5: Integration (1-2 hours)
- Update Makefile to build new files
- Link into standard library (c45.lib)
- Verify no conflicts with existing code

**Deliverable**: Phase 1 complete and tested

---

## Estimated Total Effort: 40-50 hours

**Breakdown**:
- KERNAL wrappers: 5 hours (most time in testing KERNAL calls)
- cbm_read/cbm_write: 5 hours
- Buffer management: 2 hours
- Testing: 3 hours
- Integration/debugging: 5 hours
- Documentation: 2 hours
- **Total: 22 hours**

(Conservative estimate accounting for:
- KERNAL documentation research
- Testing on real MEGA65 / emulator
- Edge case handling
- Performance optimization)

---

## Development Environment

### Testing Approach

1. **Unit tests** (test_stdio_phase1.c)
   - Verify constants and structure definitions
   - Test flag operations
   - No actual file I/O required

2. **Integration tests** (with actual file I/O)
   - Require emulator or MEGA65 hardware
   - Test file creation and deletion
   - Test reading/writing data

3. **Regression tests**
   - Ensure existing code still works
   - Run full test suite after each phase

### Build System

Current Makefile doesn't build stdlib. Need to:
1. Create `lib/Makefile` if not exists
2. Add rules for stdio_file.c, stdio_io.c, stdio_cbm.c
3. Build into `lib/c45.lib` archive
4. Link with other test programs

### Dependencies

- **cbm.h** — Already exists, provides KERNAL definitions
- **dma.h** — For potential DMA-based I/O (Phase 2)
- **mega65.h** — Hardware register definitions
- **stdlib.h** — For malloc/free
- **string.h** — For memcpy/memset

---

## Known Issues & Future Improvements

### Current Limitations

1. **No disk45 integration** — Could provide faster I/O than KERNAL
2. **Single ungetc character** — Standard allows at least 1, we support exactly 1
3. **No tmpfile support** — Needs temporary file allocation strategy
4. **No fpos_t type** — Typedef needed for fgetpos/fsetpos
5. **No wide character support** — Not needed for Phase 1

### Performance Optimization (Future)

1. **Larger buffers** for sequential I/O
2. **DMA-based writes** for bulk data
3. **Read-ahead buffering** for disk I/O
4. **Cache directory entries** for repeated access

### Error Handling (Current)

- Returns EOF on most errors
- Sets _IOERR flag for caller to check
- Could add more detailed error codes (future)

---

## Files to Create/Modify

| File | Status | Notes |
|------|--------|-------|
| lib/include/stdio.h | ✅ Complete | Header file |
| lib/src/stdio_file.c | ✅ Complete | FILE management |
| lib/src/stdio_io.c | ✅ Complete | Character/block I/O |
| lib/src/stdio_kernal.c | ⏳ TODO | KERNAL wrappers |
| lib/src/stdio_cbm.c | ⏳ TODO | cbm_* functions |
| src/test-resources/test_stdio_phase1.c | ✅ Complete | Unit tests |
| lib/Makefile | ⏳ TODO | Build configuration |
| lib/c45.lib | ⏳ TODO | Standard library archive |

---

## Next Steps

1. ✅ Create stdio.h header (DONE)
2. ✅ Implement stdio_file.c (DONE)
3. ✅ Implement stdio_io.c (DONE)
4. ✅ Create test suite (DONE)
5. ⏳ Create KERNAL wrapper functions
6. ⏳ Implement cbm_read/cbm_write
7. ⏳ Complete buffer management
8. ⏳ Run integration tests
9. ⏳ Update build system
10. ⏳ Documentation and examples

---

## References

- **MEGA65 Hardware Reference** — KERNAL ROM addresses and calling conventions
- **Commodore 64 Programmer's Reference Guide** — KERNAL ROM routines
- **C Standard Library Reference** — fopen/fclose specifications
- **6502 Assembly** — For KERNAL wrapper implementations

