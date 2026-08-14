# Demonstration of basic45 label support
# Compile with: basic45 labels_demo.bas --labels -o labels_demo.prg

# Simple counter loop with labels
print "Starting counter..."

counter:
print "Count: ";x
x = x + 1

if x < 5 then goto counter

# Subroutine demonstration
print "Calling subroutine..."
gosub show_message

print "Done!"
end

# Subroutine
show_message:
print "Hello from subroutine!"
return
