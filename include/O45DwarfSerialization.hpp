#pragma once

#include "DebugInfoBuilder.hpp"
#include "LineNumberProgram.hpp"
#include "O45Writer.hpp"

// Phase 113: DWARF serialization helper for O45Writer integration

// Prepare O45Writer with serialized DWARF sections from DebugInfoBuilder and LineNumberProgramBuilder
void prepareO45WriterWithDWARF(O45Writer& writer,
                               const DebugInfoBuilder& debugBuilder,
                               const dwarf::LineNumberProgramBuilder& lineBuilder);
