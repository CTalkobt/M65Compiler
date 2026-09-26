#include "PreprocessStage.hpp"
#include "Preprocessor.hpp"
#include "Diagnostic.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

Stage::Result PreprocessStage::execute() {
    if (verboseLevel_ >= 1) {
        std::cout << "Preprocessing " << inputFile_ << "..." << std::endl;
    }

    try {
        Preprocessor preprocessor(true);
        preprocessedSource_ = preprocessor.process(sourceRaw_, symbols_, includePaths_, inputFile_);

        // Write output if specified
        if (!outputFile_.empty()) {
            std::ofstream out(outputFile_);
            if (!out.is_open()) {
                return {false, "Failed to open output file: " + outputFile_};
            }
            out << preprocessedSource_;
            out.close();

            if (verboseLevel_ >= 1) {
                std::cout << "Preprocessed output written to " << outputFile_ << std::endl;
            }
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what()};
    }
}
