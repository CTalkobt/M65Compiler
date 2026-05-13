import os

filename = '../../../src/main/CodeGenerator.cpp'
with open(filename, 'r') as f:
    lines = f.readlines()

# Look for the broken section around line 4880
new_lines = []
skip = False
for i in range(len(lines)):
    if 'emitter->emitInstruction("ldx", AddressingMode::BASE_PAGE, 0x04, true);' in lines[i]:
        new_lines.append(lines[i])
        new_lines.append('    }\n')
        skip = True
        continue
    if skip and '    // Restore assembler frame tracking' in lines[i]:
        skip = False
    if not skip:
        new_lines.append(lines[i])

with open(filename, 'w') as f:
    f.writelines(new_lines)
print(f'Fixed {filename}')
