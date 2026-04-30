; Test .array directive and expr array indexing
.cpu _45gs02
* = $2000

; 1D array of bytes
.array scores, 1, 10

; 2D array: 8 rows x 8 cols of bytes (e.g., chess board)
.array board, 1, 8, 8

; 2D array of words: 4x3
.array grid, 2, 4, 3

; --- Test constant indexing ---
; board[2][5] = board + 2*8 + 5 = board + 21
lda board + 2 * board.__stride0 + 5 * board.__stride1

; scores[3] via manual stride
lda scores + 3 * scores.__stride0

; --- Test expr with constant array index ---
expr .A, scores[3]
expr .A, board[2][5]

; --- Test expr with runtime array index (register) ---
expr .A, scores[.x]
expr .A, board[.y][.x]

; --- Test expr with array in larger expression ---
; scores[3] * 2
expr .A, scores[3] * 2

; --- Verify metadata constants ---
; scores.__elsize should be 1
; scores.__dims should be 1
; scores.__dim0 should be 10
; scores.__stride0 should be 1
; board.__elsize should be 1
; board.__dims should be 2
; board.__dim0 should be 8
; board.__dim1 should be 8
; board.__stride0 should be 8
; board.__stride1 should be 1
; grid.__elsize should be 2
; grid.__stride0 should be 6  (3 * 2)
; grid.__stride1 should be 2

lda #scores.__elsize     ; should be 1
lda #scores.__stride0    ; should be 1
lda #board.__stride0     ; should be 8
lda #board.__stride1     ; should be 1
lda #grid.__stride0      ; should be 6
lda #grid.__stride1      ; should be 2
lda #grid.__elsize       ; should be 2
