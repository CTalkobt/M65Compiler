# Phase 22: Batch Audio Operations with Vector SIMD

**Status**: Design/Implementation  
**Target Version**: v1.0.13+  
**Priority**: High (Real-Time Audio)  
**Builds On**: Phase 21 Vector Operations

## Overview

Phase 22 extends Phase 21's vector types with batch audio operations, enabling real-time processing of entire audio frames, effect chains, and multi-channel mixing using vector SIMD primitives. All operations process multiple samples in parallel with minimal overhead.

## Core Concepts

### AudioBuffer: Specialized Vector for Audio

```c
// AudioBuffer<SampleType, FrameSize> extends Vector<SampleType, FrameSize>
// Optimized for real-time audio with specialized operations

AudioBuffer<int16_t, 256> frame;        // 256-sample 16-bit frame
AudioBuffer<int16_t, 128> hires_frame;  // 128-sample for higher update rate
AudioBuffer<float, 512>    float_frame;  // 512-sample floating-point
```

### Batch Operations on Entire Frames

Instead of sample-by-sample processing:

```c
// Old way (Phase 18): Process one sample at a time
void apply_gain(int16_t* buffer, int length, int gain) {
    for (int i = 0; i < length; ++i) {
        buffer[i] = (buffer[i] * gain) / 256;  // 256 iterations
    }
}

// Phase 22 way: Process entire frame at once
void apply_gain_batch(AudioBuffer<int16_t, 256>& frame, int gain) {
    frame = frame * gain / 256;  // Single operation on all 256 samples
}
```

### Loop Fusion: Multiple Effects in Single Pass

```c
// Phase 22: Chain effects without intermediate buffers
AudioBuffer<int16_t, 256> frame = load_frame();

// Fused operation: mix + limiter + gate in one pass
frame = (frame +| reverb_tail) <> limit_level;  // Mix with saturation + limit
frame = frame >< gate_level;                     // Apply gate (minimum threshold)
frame = frame * master_volume / 256;             // Apply master gain

// No intermediate buffers, no temporary allocations
```

## Batch Audio Primitives

### Frame-Level Operations

#### Load & Store Frames
```c
// Load from hardware/memory
AudioBuffer<int16_t, 256> frame = load_audio_frame(channel);

// Process...

// Store to hardware/memory
store_audio_frame(channel, frame);

// Direct DMA copy (Phase 23)
dma_copy_frame(dst_channel, src_channel);
```

#### Multi-Channel Operations
```c
struct StereoFrame {
    AudioBuffer<int16_t, 256> left;
    AudioBuffer<int16_t, 256> right;
};

struct QuadFrame {
    AudioBuffer<int16_t, 256> front_left;
    AudioBuffer<int16_t, 256> front_right;
    AudioBuffer<int16_t, 256> rear_left;
    AudioBuffer<int16_t, 256> rear_right;
};

// Batch stereo operations
StereoFrame mix_stereo(const StereoFrame& a, const StereoFrame& b) {
    return {
        a.left +| b.left,    // Mix left channels with saturation
        a.right +| b.right   // Mix right channels with saturation
    };
}
```

#### Time-Domain Window Operations
```c
// Apply envelope/window to entire frame
AudioBuffer<uint8_t, 256> envelope = get_attack_envelope();
AudioBuffer<int16_t, 256> audio = load_frame();

// Element-wise multiply (envelope applies to audio)
AudioBuffer<int16_t, 256> enveloped = audio * envelope / 256;
```

### Batch Effect Operations

#### Saturating Mix (Multichannel Blend)
```c
// Mix 4 audio channels safely
AudioBuffer<int16_t, 256> ch1, ch2, ch3, ch4;

// Phase 21: Element-wise saturating addition
AudioBuffer<int16_t, 256> mixed = ch1.saturating_add(ch2);
mixed = mixed.saturating_add(ch3);
mixed = mixed.saturating_add(ch4);

// Phase 22: Fused multi-channel mix
AudioBuffer<int16_t, 256> mixed = batch_saturating_mix({ch1, ch2, ch3, ch4});
```

#### Batch Limiter (Soft Clipping)
```c
// Apply soft limiter to entire frame
AudioBuffer<int16_t, 256> audio = load_frame();
int16_t limit_level = 30000;

// Simple limiter: clamp to max
AudioBuffer<int16_t, 256> limited = audio.min(
    AudioBuffer<int16_t, 256>(limit_level)
);

// Soft limiter with saturation (smoother):
AudioBuffer<int16_t, 256> soft_limited = audio;  // Copy
// Apply non-linear saturation for soft clipping
for (int i = 0; i < 256; ++i) {
    if (audio[i] > limit_level) {
        // Soft knee: reduce above threshold
        soft_limited[i] = limit_level + (audio[i] - limit_level) / 4;
    }
}
```

#### Batch Gate (Noise Gate)
```c
// Remove samples below threshold
AudioBuffer<int16_t, 256> audio = load_frame();
int16_t gate_level = 1000;  // Minimum level

// Gate: silence samples below threshold
AudioBuffer<int16_t, 256> gated = audio.max(
    AudioBuffer<int16_t, 256>(gate_level)
);
```

#### Batch Reverb Tail
```c
// Add reverb to entire frame at once
AudioBuffer<int16_t, 256> input = load_frame();
AudioBuffer<int16_t, 256> reverb_tail = get_reverb_tail();

// Saturating mix with reverb
AudioBuffer<int16_t, 256> wet = input.saturating_add(reverb_tail);
```

### Batch Reduction Operations

#### Frame Energy / RMS Calculation
```c
// Calculate frame energy for metering/analysis
AudioBuffer<int16_t, 256> frame = load_frame();

// Energy: sum of squares
int64_t energy = 0;
for (int i = 0; i < 256; ++i) {
    int32_t sample = frame[i];
    energy += sample * sample;
}

int16_t rms = sqrt(energy / 256);  // RMS value for metering
```

#### Frame Peak Detection
```c
// Find peak value in frame
AudioBuffer<int16_t, 256> frame = load_frame();

int16_t peak = 0;
for (int i = 0; i < 256; ++i) {
    int16_t val = abs(frame[i]);
    if (val > peak) peak = val;
}

// Use for automatic gain control, clipping detection
```

#### Frame DC Offset Removal
```c
// Remove DC component from frame
AudioBuffer<int16_t, 256> frame = load_frame();

int32_t dc = frame.sum() / 256;  // Calculate DC
frame = frame - dc;  // Remove DC offset (element-wise subtract)
```

## Effect Chains with Loop Fusion

### Serial Effect Chain (Optimized)

```c
struct EffectChain {
    AudioBuffer<int16_t, 256> input_frame;
    
    // Fused processing: no intermediate buffers between effects
    void process() {
        // Load once
        input_frame = load_audio_frame();
        
        // Effect 1: Gain
        input_frame = input_frame * 2;  // 6dB boost
        
        // Effect 2: Saturating mix with reverb
        AudioBuffer<int16_t, 256> reverb = get_reverb_tail();
        input_frame = input_frame.saturating_add(reverb);
        
        // Effect 3: Limiter
        input_frame = input_frame <> max_level(30000);
        
        // Effect 4: Gate
        input_frame = input_frame >< min_level(1000);
        
        // Effect 5: Master volume
        input_frame = input_frame * master_volume / 256;
        
        // Store once
        store_audio_frame(input_frame);
    }
};
```

### Parallel Effect Mixing

```c
struct ParallelEffects {
    // Dry path
    AudioBuffer<int16_t, 256> dry;
    
    // Wet paths (reverb, delay, chorus)
    AudioBuffer<int16_t, 256> reverb;
    AudioBuffer<int16_t, 256> delay;
    AudioBuffer<int16_t, 256> chorus;
    
    // Mix all together
    AudioBuffer<int16_t, 256> process(const AudioBuffer<int16_t, 256>& input) {
        // Calculate all effects in parallel
        dry = input;
        reverb = apply_reverb(input);
        delay = apply_delay(input);
        chorus = apply_chorus(input);
        
        // Fused mix: dry + all wets with saturation
        AudioBuffer<int16_t, 256> result = dry;
        result = result.saturating_add(reverb);
        result = result.saturating_add(delay);
        result = result.saturating_add(chorus);
        
        return result <> max_level(32767);  // Final limiter
    }
};
```

## Real-Time Performance Optimizations

### Frame-Aligned Processing

```c
// Process audio in fixed frame sizes for predictable latency
const int FRAME_SIZE = 256;  // 256 samples @ 48kHz = ~5.3ms latency

// Real-time audio callback
void on_audio_frame() {
    AudioBuffer<int16_t, FRAME_SIZE> input = load_frame();
    
    // All processing must complete before next frame arrives
    // ~5.3ms budget on 6502 is tight - vector SIMD helps!
    
    AudioBuffer<int16_t, FRAME_SIZE> output = process_effects(input);
    
    store_frame(output);
}
```

### Zero-Copy Processing

```c
// Avoid buffer copies with in-place operations
void apply_effects_inplace(AudioBuffer<int16_t, 256>& frame) {
    // All operations modify frame in-place
    
    frame = frame * gain / 256;              // Modify in place
    frame = frame.saturating_add(reverb);    // Modify in place
    frame = frame <> limit_level;            // Modify in place
    
    // No temporary buffers allocated
}
```

### Operator Precedence & Optimization

```c
// Phase 22: Compiler recognizes patterns for optimization
AudioBuffer<int16_t, 256> audio;

// Pattern 1: Saturating mix + limit
audio = (audio.saturating_add(effect) <> limit);
// Compiler may fuse into single pass with no intermediate storage

// Pattern 2: Multi-effect chain
audio = audio * gain / 256;                   // Gain
audio = audio.saturating_add(reverb) <> limit;  // Mix + Limit
audio = audio >< gate;                        // Gate
// Compiler tracks data flow, eliminates dead temporaries
```

## Compiler Support for Batch Operations

### Pattern Recognition

Compiler recognizes common effect patterns:

```cpp
// In CodeGenerator:
enum EffectPattern {
    SATURATING_MIX,           // a +| b
    SATURATING_MIX_LIMIT,     // (a +| b) <> limit
    GATE_OPERATION,           // a >< threshold
    GAIN_SHIFT,               // a * scale / 256
    MULTI_EFFECT_CHAIN,       // a * g1, then +|, then <>, etc.
};

// Generate optimized code paths for recognized patterns
void generateBatchEffect(EffectPattern pattern, ...) {
    switch (pattern) {
        case SATURATING_MIX:
            // Generate fused saturating addition for entire frame
            // No intermediate samples, single loop
            break;
        case SATURATING_MIX_LIMIT:
            // Generate fused add + clamp in single loop
            // Reduces register pressure, improves cache locality
            break;
        // ... etc
    }
}
```

### Optimization Directives

```c
// Compiler pragmas for batch optimization control
#pragma cc45 batch_optimize(on)      // Enable batch fusion
#pragma cc45 batch_optimize(off)     // Disable for debugging

// Per-function optimization hints
#pragma cc45 batch_process(saturating_mix)
void audio_mixer() {
    // Use fused saturating mix implementation
}
```

## Type Safety in Batch Operations

### Type-Checked Batch Functions

```cpp
// Phase 22: Strong typing for batch operations

template<typename T, size_t N>
AudioBuffer<T, N> batch_saturating_add(
    const AudioBuffer<T, N>& a,
    const AudioBuffer<T, N>& b
) {
    return a.saturating_add(b);  // Type-safe, no implicit conversions
}

template<typename T, size_t N>
AudioBuffer<T, N> batch_apply_gain(
    const AudioBuffer<T, N>& frame,
    int gain_scaled  // Gain as fixed-point (256 = unity)
) {
    AudioBuffer<T, N> result;
    for (size_t i = 0; i < N; ++i) {
        int32_t temp = static_cast<int32_t>(frame[i]) * gain_scaled;
        result[i] = static_cast<T>(temp / 256);
    }
    return result;
}

// Usage: type-safe, no implicit conversions
AudioBuffer<int16_t, 256> output = batch_apply_gain(input, 512);  // 2x gain
```

## Real-World Use Cases

### 1. Real-Time 4-Channel Mixer

```c
struct RealTimeMixer {
    AudioBuffer<int16_t, 256> ch1, ch2, ch3, ch4;
    int16_t channel_gains[4];
    int16_t master_volume;
    int16_t limit_level;
    
    void process_frame() {
        // Apply per-channel gain
        AudioBuffer<int16_t, 256> mix = ch1 * channel_gains[0] / 256;
        mix = mix.saturating_add(ch2 * channel_gains[1] / 256);
        mix = mix.saturating_add(ch3 * channel_gains[2] / 256);
        mix = mix.saturating_add(ch4 * channel_gains[3] / 256);
        
        // Apply master gain + limiter
        mix = (mix * master_volume / 256) <> limit_level;
        
        // Output single master frame
        store_mixed_frame(mix);
    }
};
```

### 2. Guitar Amp Simulator

```c
struct GuitarAmp {
    AudioBuffer<int16_t, 256> input_frame;
    int16_t drive_level;      // 0-255
    int16_t tone_level;       // 0-255
    int16_t master_volume;    // 0-255
    
    void process_frame() {
        // Load input
        input_frame = load_frame();
        
        // Phase 1: Gain staging (boost with saturation)
        input_frame = input_frame.saturating_mul(drive_level / 32);
        
        // Phase 2: Soft clip (non-linear saturation for "warmth")
        for (int i = 0; i < 256; ++i) {
            if (input_frame[i] > 20000) {
                // Soft knee saturation
                input_frame[i] = 20000 + (input_frame[i] - 20000) / 8;
            }
        }
        
        // Phase 3: Tone control (simple high-pass via mixing)
        AudioBuffer<int16_t, 256> filtered = apply_tone_filter(input_frame);
        input_frame = filtered * tone_level / 256;
        
        // Phase 4: Master output
        input_frame = input_frame * master_volume / 256;
        
        // Store output
        store_frame(input_frame);
    }
};
```

### 3. Mastering Chain (Compression + EQ + Limiting)

```c
struct MasteringChain {
    AudioBuffer<int16_t, 256> frame;
    
    void process_frame() {
        frame = load_frame();
        
        // Stage 1: Makeup gain (+6dB)
        frame = frame.saturating_lshift(1);  // *2 with saturation
        
        // Stage 2: Compression (reduce peaks, boost valleys)
        frame = apply_compressor(frame);
        
        // Stage 3: EQ (boost bass via mixing with low-pass)
        AudioBuffer<int16_t, 256> bass = apply_lowpass_filter(frame);
        frame = frame.saturating_add(bass * 50 / 256);  // +3dB bass
        
        // Stage 4: Limiting (hard ceiling)
        frame = frame <> AudioBuffer<int16_t, 256>(28000);
        
        // Stage 5: Output metering
        int16_t peak = get_frame_peak(frame);
        update_level_meter(peak);
        
        store_frame(frame);
    }
};
```

## Testing Strategy

### Test Categories

1. **Basic Batch Operations** (12 tests)
   - Load/store frames
   - Apply gain batch
   - Saturating operations on frames
   - Batch min/max/gating

2. **Multi-Channel Operations** (10 tests)
   - Stereo frame mixing
   - 4-channel blend
   - Per-channel gain
   - Cross-channel operations

3. **Effect Chains** (15 tests)
   - Serial effect chains
   - Parallel effect mixing
   - Gain + saturation + limit
   - Complex multi-effect sequences

4. **Real-Time Performance** (8 tests)
   - Frame alignment verification
   - Zero-copy processing
   - Timing constraints
   - Loop unrolling verification

5. **Practical Scenarios** (10 tests)
   - Real-time mixer
   - Guitar amp simulator
   - Mastering chain
   - Reverb processor with multichannel

**Total: 55 tests**

## Compiler Implementation

### Pattern Recognition & Fusion

```cpp
// In IR Optimizer:

// Recognize pattern: a +| b <> limit
if (is_saturating_add(expr1) && is_min_op(expr2)) {
    // Generate fused operation
    emit_fused_saturating_add_limit(expr1, expr2);
}

// Recognize pattern: chain of operations on same vector
if (is_chain_of_vector_ops(statements)) {
    // Mark for in-place optimization
    mark_for_inplace_fusion(statements);
}
```

### Code Generation for Batch

```asm
; Batch saturating add for 256 samples
; AudioBuffer<int16_t, 256> result = a +| b
; A = start of array a, B = start of array b, C = result

loop:
    lda (A)         ; Load a[i]
    clc
    adc (B)         ; Add b[i]
    bvc @no_sat     ; Check overflow
    lda #127        ; Saturate positive
    bra @store
@no_sat:
    bmi @neg        ; Check if negative
    bra @store
    lda #-128       ; Saturate negative
@store:
    sta (C)         ; Store result
    
    ; Advance pointers (unrolled 4x for efficiency)
    ; ...repeat 3 more times...
    
    cpy #64         ; Check if done (256 samples / 4 per iteration)
    bne loop
```

### SIMD Hints & Metadata

Compiler emits hints for linker-level vectorization:

```
; Batch operation metadata
.batch_hint operation=saturating_add, type=int16_t, count=256
.batch_hint operation=element_limit, type=int16_t, count=256
.batch_hint chain=[gain, saturating_add, min_op], fusible=yes
```

## Integration with Phase 21

- **Phase 21 Vectors**: Fundamental building blocks
- **Phase 22 Batch Ops**: High-level convenience operations
- **Synergy**: Compiler recognizes patterns from both layers and fuses optimally

## Performance Goals

### Target Metrics

| Operation | Frame Size | Target Time | Samples/µs |
|-----------|-----------|-------------|-----------|
| Saturating Add | 256 | 3.2µs | ~80 |
| Multi-effect chain | 256 | 8µs | ~32 |
| 4-channel mix | 256 | 5µs | ~51 |
| Mastering chain | 256 | 15µs | ~17 |

### On 6502 @ 3.5MHz

- 1µs per instruction (approximate)
- 256 samples @ 48kHz = 5.3ms per frame
- Budget: ~18,550 instructions per frame

### Achieved (Estimated)

- Saturating add: ~4 instructions/sample = 1024 instructions total ✅
- Effect chain (5 effects): ~20 instructions/sample = 5120 instructions total ✅
- Headroom for multiple chains or higher sample rates

## Known Limitations

1. **Frame-Aligned Only**: Block-based processing, not sample-accurate scheduling
2. **Fixed Frame Size**: Dynamic frame sizes deferred to Phase 23
3. **No Lookahead**: Analysis-based effects (compressor with look-ahead) need buffering
4. **Limited Internal Precision**: Fixed-point math, no arbitrary precision

## Future Enhancements (Phase 23+)

- **Phase 23**: DMA-accelerated batch operations, hardware audio buffer management
- **Phase 24**: Variable frame sizes, adaptive latency modes
- **Phase 25**: Lookahead buffering, predictive effects scheduling
- **Phase 26**: Neural network inference on audio buffers (lightweight models)

## References

- Phase 21: Vector Operations (`doc/architecture/phase21-vector-operations.md`)
- Phase 19: Advanced Operators (`doc/architecture/phase19-advanced-operators.md`)
- Phase 18: Audio Effects (`doc/architecture/phase18-audio-effects.md`)
- MEGA65 Hardware: Audio subsystem registers and DMA

---

**Phase 22 enables production-grade real-time audio processing on MEGA65!** 🎵
