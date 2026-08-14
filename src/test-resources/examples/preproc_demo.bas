# Demonstration of preprocessor features
# Compile with: basic45 preproc_demo.bas --labels -o preproc_demo.prg

#include "stdlib_basic.bas"

# Define configuration
#define VERSION 100
#define DEBUG 1

print "Program Version: ";VERSION

#ifdef DEBUG
print "Debug mode enabled"
print "Screen size: ";SCREEN_WIDTH;" x ";SCREEN_HEIGHT
#else
print "Release mode"
#endif

start:
print "hello"
end
