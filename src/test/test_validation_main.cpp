// test_validation_main.cpp — bin/test_validation: the validation tests of
// cc45, ca45 and ln45 in one program (#96)
//
//   test_validation                 every category
//   test_validation parser linker   those categories
//   test_validation -l              the categories' names
//
// Each category is a test_validation_<name>.cpp (or test_segment_emission.cpp,
// test_move_fill.cpp) whose main() is compiled as tv_<name>_main; it prints its
// own summary. The exit status is 1 when a category failed. TV_VERBOSE=1 shows
// the PASS lines too (test_harness.hpp).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#define TV_CATEGORIES(X) \
    X(parser) X(struct) X(const) X(semantic) X(preprocessor) X(linker) X(segment_emission) \
    X(assembler) X(simops) X(directives) X(symbols) X(segments) X(proc) X(addressing) \
    X(simops_extended) X(move_fill) X(ca45lib)

#define TV_DECLARE(n) int tv_##n##_main();
TV_CATEGORIES(TV_DECLARE)

struct Category { const char* name; int (*run)(); };
#define TV_ENTRY(n) {#n, tv_##n##_main},
static const Category categories[] = { TV_CATEGORIES(TV_ENTRY) };

int main(int argc, char** argv) {
    std::vector<const Category*> chosen;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "-l")) {
            for (const auto& c : categories) printf("%s\n", c.name);
            return 0;
        }
        const Category* found = nullptr;
        for (const auto& c : categories)
            if (!std::strcmp(argv[i], c.name)) found = &c;
        if (!found) {
            fprintf(stderr, "test_validation: no category '%s' (-l lists them)\n", argv[i]);
            return 2;
        }
        chosen.push_back(found);
    }
    if (chosen.empty())
        for (const auto& c : categories) chosen.push_back(&c);
    std::vector<std::string> failed;
    for (const auto* c : chosen) {
        fflush(stdout);
        if (c->run() != 0) failed.push_back(c->name);
        fflush(stdout);
    }
    if (!failed.empty()) {
        printf("\ntest_validation: %zu of %zu categories failed:", failed.size(), chosen.size());
        for (const auto& f : failed) printf(" %s", f.c_str());
        printf("\n");
        return 1;
    }
    return 0;
}
