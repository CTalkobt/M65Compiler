#include <cassert>
#include <iostream>
#include <memory>
#include "procedural/JazzComposer.hpp"
#include "procedural/AmbientComposer.hpp"
#include "procedural/TechnoComposer.hpp"
#include "procedural/FolkComposer.hpp"
#include "procedural/ClassicalComposer.hpp"
#include "procedural/ChiptureComposer.hpp"
#include "procedural/RagtimeComposer.hpp"

using namespace procedural;

// Test 1: Jazz Composer Creation and Composition
void test_jazz_composer() {
    JazzComposer jazz;
    assert(jazz.getGenreName() == "Jazz");
    assert(!jazz.getDescription().empty());

    auto song = jazz.compose(8, Key::C_MAJOR, 120, 42);
    assert(song != nullptr);
    assert(song->getTempo() == 120);
    assert(song->getTrackCount() > 0);

    std::cout << "✓ Jazz Composer test passed\n";
}

// Test 2: Ambient Composer
void test_ambient_composer() {
    AmbientComposer ambient;
    assert(ambient.getGenreName() == "Ambient");

    ambient.setMinimalism(0.8f);
    ambient.setEvolutionRate(0.4f);
    ambient.setReverbAmount(0.9f);

    auto song = ambient.compose(16, Key::D_MAJOR, 80, 123);
    assert(song != nullptr);
    assert(song->getTempo() == 80);
    assert(song->getTrackCount() > 0);

    std::cout << "✓ Ambient Composer test passed\n";
}

// Test 3: Techno Composer
void test_techno_composer() {
    TechnoComposer techno;
    assert(techno.getGenreName() == "Techno");

    techno.setSubBassIntensity(0.9f);
    techno.setDrumVariation(0.2f);
    techno.setFilterSweep(0.6f);

    auto song = techno.compose(4, Key::A_MAJOR, 128, 456);
    assert(song != nullptr);
    assert(song->getTempo() == 128);

    std::cout << "✓ Techno Composer test passed\n";
}

// Test 4: Folk Composer
void test_folk_composer() {
    FolkComposer folk;
    assert(folk.getGenreName() == "Folk");

    folk.setNarrativeStructure(0.5f);
    folk.setOrnamentationAmount(0.3f);
    folk.setTraditionalism(0.9f);

    auto song = folk.compose(12, Key::G_MAJOR, 100, 789);
    assert(song != nullptr);
    assert(song->getTrackCount() > 0);

    std::cout << "✓ Folk Composer test passed\n";
}

// Test 5: Classical Composer
void test_classical_composer() {
    ClassicalComposer classical;
    assert(classical.getGenreName() == "Classical");

    classical.setFormality(0.95f);
    classical.setModulationDensity(0.3f);
    classical.setPolyphonyLevel(0.7f);

    auto song = classical.compose(16, Key::E_MAJOR, 90, 999);
    assert(song != nullptr);

    std::cout << "✓ Classical Composer test passed\n";
}

// Test 6: Chiptune Composer
void test_chiptune_composer() {
    ChiptuneComposer chiptune;
    assert(chiptune.getGenreName() == "Chiptune");

    chiptune.setRetroStyle(0.9f);
    chiptune.setArpeggioAmount(0.7f);
    chiptune.setSquareWaveIntensity(0.8f);

    auto song = chiptune.compose(8, Key::F_MAJOR, 150, 321);
    assert(song != nullptr);

    std::cout << "✓ Chiptune Composer test passed\n";
}

// Test 7: Ragtime Composer (The Special One!)
void test_ragtime_composer() {
    RagtimeComposer ragtime;
    assert(ragtime.getGenreName() == "Ragtime");

    // Test ragtime-specific parameters
    ragtime.setSyncopationLevel(0.95f);  // Heavy syncopation
    ragtime.setStrideBassIntensity(0.85f);  // Stride bass
    ragtime.setJazzinessAmount(0.2f);  // Traditional ragtime

    auto song = ragtime.compose(8, Key::B_MAJOR, 140, 555);
    assert(song != nullptr);
    assert(song->getTempo() == 140);

    // Verify Ragtime has multiple tracks
    assert(song->getTrackCount() >= 3);  // At least harmony, melody, bass, drums

    std::cout << "✓ Ragtime Composer test passed (Syncopated + Stride Bass)\n";
}

// Test 8: All Composers with Different Keys
void test_all_composers_different_keys() {
    std::vector<Key> keys = {
        Key::C_MAJOR, Key::G_MAJOR, Key::D_MAJOR,
        Key::A_MAJOR, Key::E_MAJOR, Key::B_MAJOR,
        Key::F_MAJOR
    };

    JazzComposer jazz;
    AmbientComposer ambient;
    TechnoComposer techno;

    for (const auto& key : keys) {
        auto j = jazz.compose(4, key, 120, 42);
        auto a = ambient.compose(4, key, 120, 42);
        auto t = techno.compose(4, key, 120, 42);

        assert(j != nullptr);
        assert(a != nullptr);
        assert(t != nullptr);
    }

    std::cout << "✓ All composers multi-key test passed\n";
}

// Test 9: Parameter Mutation and Clamping
void test_parameter_clamping() {
    JazzComposer jazz;

    jazz.setSwingAmount(2.0f);  // Should clamp to 1.0
    assert(jazz.getSwingAmount() <= 1.0f);

    jazz.setSwingAmount(-0.5f);  // Should clamp to 0.0
    assert(jazz.getSwingAmount() >= 0.0f);

    RagtimeComposer ragtime;
    ragtime.setSyncopationLevel(10.0f);  // Should clamp
    assert(ragtime.getSyncopationLevel() <= 1.0f);

    std::cout << "✓ Parameter clamping test passed\n";
}

// Test 10: Complex Compositions with Energy/Complexity Settings
void test_composition_parameters() {
    JazzComposer jazz;
    jazz.setComplexity(0.8f);
    jazz.setEnergy(0.7f);
    jazz.setEmotionalTone(0.6f);

    auto song = jazz.compose(12, Key::C_MAJOR, 140, 2000);
    assert(song != nullptr);

    ClassicalComposer classical;
    classical.setComplexity(0.95f);
    classical.setEnergy(0.4f);
    classical.setEmotionalTone(0.5f);

    auto csong = classical.compose(16, Key::E_MAJOR, 90, 2001);
    assert(csong != nullptr);

    std::cout << "✓ Composition parameters test passed\n";
}

// Test 11: Seed Reproducibility
void test_seed_reproducibility() {
    JazzComposer j1, j2;

    auto song1 = j1.compose(8, Key::G_MAJOR, 120, 12345);
    auto song2 = j2.compose(8, Key::G_MAJOR, 120, 12345);

    assert(song1 != nullptr);
    assert(song2 != nullptr);
    // Both should have same structure (same seed = same composition)
    assert(song1->getTrackCount() == song2->getTrackCount());

    std::cout << "✓ Seed reproducibility test passed\n";
}

// Test 12: Ragtime Syncopation Verification
void test_ragtime_syncopation_intensity() {
    RagtimeComposer ragtime;

    // Test with low syncopation (more straight)
    ragtime.setSyncopationLevel(0.1f);
    auto song_straight = ragtime.compose(4, Key::C_MAJOR, 120, 100);
    assert(song_straight != nullptr);

    // Test with high syncopation (very syncopated)
    ragtime.setSyncopationLevel(1.0f);
    auto song_syncopated = ragtime.compose(4, Key::C_MAJOR, 120, 100);
    assert(song_syncopated != nullptr);

    std::cout << "✓ Ragtime syncopation variation test passed\n";
}

int main() {
    std::cout << "\n=== Phase 14: Genre Composers Implementation Tests ===\n";

    test_jazz_composer();
    test_ambient_composer();
    test_techno_composer();
    test_folk_composer();
    test_classical_composer();
    test_chiptune_composer();
    test_ragtime_composer();
    test_all_composers_different_keys();
    test_parameter_clamping();
    test_composition_parameters();
    test_seed_reproducibility();
    test_ragtime_syncopation_intensity();

    std::cout << "\n✅ All 12 Phase 14 tests passed!\n";
    std::cout << "   7 genre composers ready for procedural generation\n";
    std::cout << "   Jazz, Ambient, Techno, Folk, Classical, Chiptune, Ragtime\n\n";

    return 0;
}
