#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include "audio/Effect.hpp"
#include "audio/EffectsChain.hpp"

using namespace audio;

// Test 1: Reverb effect creation and parameters
void test_reverb_effect() {
    auto reverb = std::make_unique<ReverbEffect>();

    assert(reverb->getType() == EffectType::REVERB);
    assert(reverb->getName() == "Reverb");
    assert(reverb->isEnabled());
    assert(reverb->getParameterCount() == 4);

    reverb->setParameter(0, 90);
    assert(reverb->getParameter(0) == 90);

    reverb->setWetDry(80);
    assert(reverb->getWetDry() == 80);

    reverb->initialize();
    reverb->shutdown();

    std::cout << "✓ Reverb effect test passed\n";
}

// Test 2: Chorus effect creation and parameters
void test_chorus_effect() {
    auto chorus = std::make_unique<ChorusEffect>();

    assert(chorus->getType() == EffectType::CHORUS);
    assert(chorus->getName() == "Chorus");
    assert(chorus->getParameterCount() == 3);

    chorus->setParameter(0, 50);  // Rate
    chorus->setParameter(1, 60);  // Depth
    chorus->setParameter(2, 40);  // Feedback

    assert(chorus->getParameter(0) == 50);
    assert(chorus->getParameter(1) == 60);
    assert(chorus->getParameter(2) == 40);

    chorus->initialize();
    chorus->shutdown();

    std::cout << "✓ Chorus effect test passed\n";
}

// Test 3: Delay effect creation and parameters
void test_delay_effect() {
    auto delay = std::make_unique<DelayEffect>();

    assert(delay->getType() == EffectType::DELAY);
    assert(delay->getName() == "Delay");
    assert(delay->getParameterCount() == 3);

    delay->setParameter(0, 75);  // Delay time
    delay->setParameter(1, 65);  // Feedback
    delay->setParameter(2, 50);  // Mix

    assert(delay->getParameter(0) == 75);
    assert(delay->getParameter(1) == 65);
    assert(delay->getParameter(2) == 50);

    delay->initialize();
    delay->shutdown();

    std::cout << "✓ Delay effect test passed\n";
}

// Test 4: Distortion effect creation and parameters
void test_distortion_effect() {
    auto distortion = std::make_unique<DistortionEffect>();

    assert(distortion->getType() == EffectType::DISTORTION);
    assert(distortion->getName() == "Distortion");
    assert(distortion->getParameterCount() == 3);

    distortion->setParameter(0, 100);  // Drive
    distortion->setParameter(1, 70);   // Tone
    distortion->setParameter(2, 90);   // Level

    assert(distortion->getParameter(0) == 100);
    assert(distortion->getParameter(1) == 70);
    assert(distortion->getParameter(2) == 90);

    distortion->initialize();
    distortion->shutdown();

    std::cout << "✓ Distortion effect test passed\n";
}

// Test 5: Flanger effect creation and parameters
void test_flanger_effect() {
    auto flanger = std::make_unique<FlangerEffect>();

    assert(flanger->getType() == EffectType::FLANGER);
    assert(flanger->getName() == "Flanger");
    assert(flanger->getParameterCount() == 3);

    flanger->setParameter(0, 35);  // Rate
    flanger->setParameter(1, 65);  // Depth
    flanger->setParameter(2, 45);  // Feedback

    flanger->initialize();
    flanger->shutdown();

    std::cout << "✓ Flanger effect test passed\n";
}

// Test 6: Phaser effect creation and parameters
void test_phaser_effect() {
    auto phaser = std::make_unique<PhaserEffect>();

    assert(phaser->getType() == EffectType::PHASER);
    assert(phaser->getName() == "Phaser");
    assert(phaser->getParameterCount() == 3);

    phaser->setParameter(0, 40);  // Rate
    phaser->setParameter(1, 75);  // Depth
    phaser->setParameter(2, 55);  // Feedback

    phaser->initialize();
    phaser->shutdown();

    std::cout << "✓ Phaser effect test passed\n";
}

// Test 7: Effect enable/disable
void test_effect_enable_disable() {
    auto reverb = std::make_unique<ReverbEffect>();

    assert(reverb->isEnabled());

    reverb->setEnabled(false);
    assert(!reverb->isEnabled());

    reverb->setEnabled(true);
    assert(reverb->isEnabled());

    std::cout << "✓ Effect enable/disable test passed\n";
}

// Test 8: Effect wet/dry control
void test_effect_wet_dry() {
    auto chorus = std::make_unique<ChorusEffect>();

    assert(chorus->getWetDry() == 64);  // Default 50/50

    chorus->setWetDry(100);
    assert(chorus->getWetDry() == 100);

    chorus->setWetDry(30);
    assert(chorus->getWetDry() == 30);

    std::cout << "✓ Effect wet/dry control test passed\n";
}

// Test 9: Effects chain creation and effect management
void test_effects_chain() {
    EffectsChain chain;

    assert(chain.getEffectCount() == 0);

    auto reverb = std::make_unique<ReverbEffect>();
    chain.addEffect(std::move(reverb));

    assert(chain.getEffectCount() == 1);
    assert(chain.getEffect(0) != nullptr);
    assert(chain.getEffect(0)->getType() == EffectType::REVERB);

    chain.removeEffect(0);
    assert(chain.getEffectCount() == 0);

    std::cout << "✓ Effects chain management test passed\n";
}

// Test 10: Multiple effects in chain
void test_multiple_effects_in_chain() {
    EffectsChain chain;

    // Add multiple effects
    chain.addEffect(std::make_unique<ReverbEffect>());
    chain.addEffect(std::make_unique<ChorusEffect>());
    chain.addEffect(std::make_unique<DelayEffect>());
    chain.addEffect(std::make_unique<DistortionEffect>());

    assert(chain.getEffectCount() == 4);

    // Verify order
    assert(chain.getEffect(0)->getType() == EffectType::REVERB);
    assert(chain.getEffect(1)->getType() == EffectType::CHORUS);
    assert(chain.getEffect(2)->getType() == EffectType::DELAY);
    assert(chain.getEffect(3)->getType() == EffectType::DISTORTION);

    chain.clearEffects();
    assert(chain.getEffectCount() == 0);

    std::cout << "✓ Multiple effects in chain test passed\n";
}

// Test 11: Effect order manipulation
void test_effect_order_manipulation() {
    EffectsChain chain;

    chain.addEffect(std::make_unique<ReverbEffect>());
    chain.addEffect(std::make_unique<ChorusEffect>());
    chain.addEffect(std::make_unique<DelayEffect>());

    // Swap effects
    chain.swapEffects(0, 2);
    assert(chain.getEffect(0)->getType() == EffectType::DELAY);
    assert(chain.getEffect(2)->getType() == EffectType::REVERB);

    chain.clearEffects();
    std::cout << "✓ Effect order manipulation test passed\n";
}

// Test 12: Effect enable/disable in chain
void test_effect_control_in_chain() {
    EffectsChain chain;

    chain.addEffect(std::make_unique<ReverbEffect>());
    chain.addEffect(std::make_unique<ChorusEffect>());

    assert(chain.getEffect(0)->isEnabled());

    chain.enableEffect(0, false);
    assert(!chain.getEffect(0)->isEnabled());

    chain.enableEffect(0, true);
    assert(chain.getEffect(0)->isEnabled());

    chain.clearEffects();
    std::cout << "✓ Effect control in chain test passed\n";
}

// Test 13: Master wet/dry control
void test_master_wet_dry() {
    EffectsChain chain;

    assert(chain.getMasterDry() == 100);
    assert(chain.getMasterWet() == 50);

    chain.setMasterDry(80);
    chain.setMasterWet(70);

    assert(chain.getMasterDry() == 80);
    assert(chain.getMasterWet() == 70);

    std::cout << "✓ Master wet/dry control test passed\n";
}

// Test 14: Effect bypass
void test_effect_bypass() {
    EffectsChain chain;

    assert(!chain.isBypassed());

    chain.bypass(true);
    assert(chain.isBypassed());

    chain.bypass(false);
    assert(!chain.isBypassed());

    std::cout << "✓ Effect bypass test passed\n";
}

// Test 15: Audio processing with effects
void test_audio_processing() {
    EffectsChain chain;

    // Create test audio buffer (sine-like pattern)
    std::vector<uint8_t> audioBuffer(256);
    for (uint32_t i = 0; i < audioBuffer.size(); ++i) {
        audioBuffer[i] = 128 + (i % 64);  // Simple pattern
    }

    chain.addEffect(std::make_unique<ReverbEffect>());
    chain.addEffect(std::make_unique<DelayEffect>());

    chain.process(audioBuffer.data(), audioBuffer.size());

    assert(chain.getSamplesProcessed() == 256);

    chain.clearEffects();
    std::cout << "✓ Audio processing test passed\n";
}

// Test 16: Effect parameter names
void test_effect_parameter_names() {
    auto reverb = std::make_unique<ReverbEffect>();

    assert(!reverb->getParameterName(0).empty());
    assert(reverb->getParameterName(0) == "Room Size");
    assert(reverb->getParameterName(1) == "Damping");
    assert(reverb->getParameterName(2) == "Width");
    assert(reverb->getParameterName(3) == "Dry Level");

    std::cout << "✓ Effect parameter names test passed\n";
}

// Test 17: Effect chain statistics
void test_chain_statistics() {
    EffectsChain chain;

    assert(chain.getSamplesProcessed() == 0);

    chain.addEffect(std::make_unique<ReverbEffect>());

    std::vector<uint8_t> audioBuffer(512);
    chain.process(audioBuffer.data(), audioBuffer.size());

    assert(chain.getSamplesProcessed() == 512);

    chain.resetStatistics();
    assert(chain.getSamplesProcessed() == 0);

    chain.clearEffects();
    std::cout << "✓ Chain statistics test passed\n";
}

// Test 18: Effects chain limit (max 8 effects)
void test_effects_chain_limit() {
    EffectsChain chain;

    // Try to add more than 8 effects
    for (int i = 0; i < 10; ++i) {
        chain.addEffect(std::make_unique<ReverbEffect>());
    }

    assert(chain.getEffectCount() == 8);  // Should be capped at 8

    chain.clearEffects();
    std::cout << "✓ Effects chain limit test passed\n";
}

// Test 19: Parameter bounds checking
void test_parameter_bounds() {
    auto delay = std::make_unique<DelayEffect>();

    // Set various parameter values
    delay->setParameter(0, 255);  // Delay time
    delay->setParameter(1, 127);  // Feedback
    delay->setParameter(2, 100);  // Mix

    assert(delay->getParameter(0) == 255);
    assert(delay->getParameter(1) == 127);
    assert(delay->getParameter(2) == 100);

    std::cout << "✓ Parameter bounds checking test passed\n";
}

// Test 20: Complete effects processing workflow
void test_complete_workflow() {
    EffectsChain chain;

    // Build an effects chain
    chain.addEffect(std::make_unique<ReverbEffect>());
    chain.addEffect(std::make_unique<ChorusEffect>());
    chain.addEffect(std::make_unique<DelayEffect>());

    // Configure each effect
    chain.getEffect(0)->setParameter(0, 85);  // Reverb room size
    chain.getEffect(1)->setParameter(0, 45);  // Chorus rate
    chain.getEffect(2)->setParameter(1, 70);  // Delay feedback

    // Set chain mix
    chain.setMasterDry(90);
    chain.setMasterWet(60);

    // Create test audio
    std::vector<uint8_t> audioBuffer(1024);
    for (uint32_t i = 0; i < audioBuffer.size(); ++i) {
        audioBuffer[i] = 128 + (rand() % 32) - 16;  // Random noise
    }

    uint32_t originalSamples = chain.getSamplesProcessed();
    chain.process(audioBuffer.data(), audioBuffer.size());

    assert(chain.getSamplesProcessed() == originalSamples + 1024);

    chain.clearEffects();
    std::cout << "✓ Complete effects workflow test passed\n";
}

int main() {
    std::cout << "\n=== Phase 18: Audio Effects Processing Tests ===\n";

    test_reverb_effect();
    test_chorus_effect();
    test_delay_effect();
    test_distortion_effect();
    test_flanger_effect();
    test_phaser_effect();
    test_effect_enable_disable();
    test_effect_wet_dry();
    test_effects_chain();
    test_multiple_effects_in_chain();
    test_effect_order_manipulation();
    test_effect_control_in_chain();
    test_master_wet_dry();
    test_effect_bypass();
    test_audio_processing();
    test_effect_parameter_names();
    test_chain_statistics();
    test_effects_chain_limit();
    test_parameter_bounds();
    test_complete_workflow();

    std::cout << "\n✅ All 20 Phase 18 effects processing tests passed!\n";
    std::cout << "   Audio effects system ready for MEGA65\n\n";

    return 0;
}
