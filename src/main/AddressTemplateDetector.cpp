#include "AddressTemplateDetector.hpp"

// Stub implementation - address template detection is currently handled
// directly in CodeGenerator::tryEmitAddressTemplate()

void AddressTemplateDetector::analyzeTranslationUnit(TranslationUnit& unit) {}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectPatternInExpr(Expression* expr) {
    return MatchedPattern();
}
