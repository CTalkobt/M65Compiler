# Phase 28: Standard Library Overloads (Parameter-Based Function Selection)

**Status**: Design & Architecture  
**Target Version**: v1.0.18+  
**Priority**: High (API Transparency)  
**Builds On**: Phase 26-27 Far Pointers, Compiler Name Mangling

## Overview

Phase 28 extends the C standard library with **parameter-type-based overloading**, enabling transparent function selection based on argument types at compile time. Users can call `memcpy(far_ptr, local_ptr, size)` and the compiler automatically selects the correct variant.

### Goals

1. **Transparent API**: No special syntax needed (same `memcpy` call for all variants)
2. **Type-Safe**: Compile-time type checking, no runtime dispatch
3. **Performance**: Zero runtime overhead (direct call to selected variant)
4. **Backward Compatible**: Existing code with local pointers unchanged

---

## Architecture: Three-Layer System

```
Layer 1: User Code
    memcpy(far_ptr, local_ptr, size);

Layer 2: Compiler (Name Mangling)
    Analyze parameter types → Generate mangled name
    Emit call to: memcpy__fpvst (far_ptr_t, void*, size_t)

Layer 3: Linker (Symbol Resolution)
    Look up mangled name in symbol table
    Link to correct implementation

Layer 4: Library (Implementations)
    memcpy__pvpvst() → standard C implementation
    memcpy__fpvst() → far_memcpy_to_local() wrapper
    memcpy__fpfpst() → far_memcpy() wrapper
```

---

## Name Mangling Scheme

### Type Code Mapping

```c
/* Fundamental types */
'v'  → void (pointers only)
'b'  → _Bool
'c'  → char / char*
's'  → short
'i'  → int
'l'  → long
'j'  → long long
'f'  → float
'd'  → double

/* Pointer types */
'p'  → base type pointer (void* → 'pv', int* → 'pi')
'r'  → const pointer (const void* → 'rpv')

/* Far pointers */
'f'  → far_ptr_t (prefix: 'fp')
'g'  → const far_ptr_t (prefix: 'gp')

/* Special */
'z'  → size_t
't'  → ptrdiff_t
*/
```

### Mangling Examples

```c
/* memcpy variants */
void memcpy(void*, void*, size_t);
→ memcpy__pvpvst

void memcpy(far_ptr_t, void*, size_t);
→ memcpy__fpvst

void memcpy(void*, far_ptr_t, size_t);
→ memcpy__pvfpst

void memcpy(far_ptr_t, far_ptr_t, size_t);
→ memcpy__fpfpst

/* strlen variants */
size_t strlen(const char*);
→ strlen__rpv

size_t strlen(far_ptr_t);
→ strlen__fp

/* memset variants */
void memset(void*, int, size_t);
→ memset__pvist

void memset(far_ptr_t, int, size_t);
→ memset__fpist
```

---

## Compiler Changes Required

### Phase 1: Parser Recognizes Overloads

```cpp
// In Parser.cpp: Allow multiple declarations of same name with different params
void parse_function_declaration() {
    std::string name = parse_identifier();  // "memcpy"
    std::vector<Type*> params = parse_parameters();  // (void*, void*, size_t)
    
    // NEW: Allow duplicate names if parameters differ
    if (function_table.exists(name)) {
        auto existing = function_table.get(name);
        if (!types_match(existing.params, params)) {
            // Different parameters: OK, this is an overload
            function_table.add_overload(name, params, declaration);
        } else {
            // Same parameters: error (duplicate declaration)
            error("duplicate function declaration");
        }
    } else {
        function_table.add(name, params, declaration);
    }
}
```

### Phase 2: Code Generator Does Overload Resolution

```cpp
// In CodeGenerator.cpp: Resolve overload at call site
void CodeGenerator::visit(FunctionCall& call) {
    std::string base_name = call.name;  // "memcpy"
    
    // Analyze argument types
    std::vector<Type*> arg_types;
    for (auto& arg : call.arguments) {
        arg_types.push_back(arg->infer_type());
    }
    
    // NEW: Resolve which overload matches
    std::string mangled_name = resolve_overload(base_name, arg_types);
    
    if (mangled_name.empty()) {
        error("no overload of " + base_name + " matches argument types");
        return;
    }
    
    // Emit call to mangled name
    emit_call(mangled_name);
}

std::string resolve_overload(const std::string& base_name,
                              const std::vector<Type*>& arg_types) {
    auto overloads = function_table.get_all(base_name);
    
    // Try exact match first
    for (auto& sig : overloads) {
        if (types_match_exactly(arg_types, sig.param_types)) {
            return sig.mangled_name;
        }
    }
    
    // Try implicit conversions (local_ptr → void*, far_ptr → const far_ptr)
    for (auto& sig : overloads) {
        if (can_convert_all(arg_types, sig.param_types)) {
            return sig.mangled_name;
        }
    }
    
    return "";  // No match
}
```

### Phase 3: Mangle Names

```cpp
std::string mangle_name(const std::string& base_name,
                        const std::vector<Type*>& params) {
    std::string result = base_name + "__";
    
    for (auto param : params) {
        result += mangle_type(param);
    }
    
    return result;
}

std::string mangle_type(Type* t) {
    if (t->is_fundamental()) {
        if (t->name == "void") return "v";
        if (t->name == "char") return "c";
        if (t->name == "int") return "i";
        if (t->name == "long") return "l";
        if (t->name == "float") return "f";
        if (t->name == "double") return "d";
        if (t->name == "size_t") return "z";
        // ... etc
    }
    
    if (t->is_pointer()) {
        Type* base = t->base_type();
        if (base->name == "void") return "pv";
        if (base->name == "char") return "pc";
        if (base->name == "int") return "pi";
        // ... etc
    }
    
    if (t->name == "far_ptr_t") {
        return "fp";
    }
    
    error("unknown type in mangling: " + t->name);
    return "?";
}
```

---

## Linker Changes Required

### Phase 1: Parse Overload Signatures

```cpp
// In Linker (ln45): Read symbol signatures from .o45 files
struct Symbol {
    std::string name;           // Original name: "memcpy"
    std::string mangled_name;   // Mangled: "memcpy__fpvst"
    std::vector<std::string> param_types;  // Parsed from .func_signature
    uint16_t address;
    // ...
};
```

### Phase 2: Resolve References

```cpp
// When linking: resolve call to memcpy__fpvst
Symbol* resolve_symbol(const std::string& mangled_name) {
    // Direct lookup in symbol table
    auto it = symbols.find(mangled_name);
    if (it != symbols.end()) {
        return &it->second;
    }
    
    // If not found, error with helpful message
    std::string base_name = demangle(mangled_name);
    error("undefined reference to " + mangled_name +
          " (no overload of " + base_name + " matches signature)");
    
    // Suggest available overloads
    auto overloads = get_overloads(base_name);
    if (!overloads.empty()) {
        note("available overloads:");
        for (auto& sig : overloads) {
            note("  " + sig.mangled_name);
        }
    }
    
    return nullptr;
}
```

---

## Object File (.o45) Format Extensions

### Symbol Table Extension

```c
.symbol name=memcpy
.symbol_mangled memcpy__fpvst
.symbol_signature (far_ptr_t, void*, size_t)
.symbol_address 0x1234
```

### Function Metadata

```asm
.func memcpy
.func_overload memcpy__fpvst
.param_types (far_ptr_t, void*, size_t)
.return_type void
.func_end
```

---

## Library Implementation Layer

### Key Functions to Overload

| Function | Variants |
|----------|----------|
| `memcpy` | (pvpv), (fpv), (pvfp), (fpfp) — 4 variants |
| `memset` | (pvi), (fpi) — 2 variants |
| `strlen` | (pc), (fp) — 2 variants |
| `strcpy` | (pcpc), (fppc), (pcfp), (fpfp) — 4 variants |
| `strcmp` | (pcpc), (fppc), (pcfp), (fpfp) — 4 variants |
| `strcat` | (pcpc), (fppc) — 2 variants |

### Implementation Pattern

```c
/* In lib/src/stdlib_overloads.c */

/* Original (local to local) */
void* memcpy__pvpvst(void* dst, const void* src, size_t len) {
    return memcpy(dst, src, len);  // Standard C
}

/* Far source, local destination */
void* memcpy__fpvst(void* dst, far_ptr_t src, size_t len) {
    far_memcpy_to_local(dst, src, len);
    return dst;
}

/* Local source, far destination */
void* memcpy__pvfpst(far_ptr_t dst, const void* src, size_t len) {
    far_memcpy_from_local(dst, src, len);
    return dst;
}

/* Far to far */
void* memcpy__fpfpst(far_ptr_t dst, far_ptr_t src, size_t len) {
    far_memcpy(dst, src, len);
    return dst;
}
```

---

## User-Facing API

### Before Phase 28 (Manual)

```c
far_ptr_t audio = far_malloc(512, BANK1);
uint8_t local_buf[256];

// Explicit far operations
far_memcpy_to_local(local_buf, audio, 256);
far_memcpy_from_local(audio, local_buf, 256);
```

### After Phase 28 (Transparent)

```c
far_ptr_t audio = far_malloc(512, BANK1);
uint8_t local_buf[256];

// Same memcpy call, compiler selects correct variant
memcpy(local_buf, audio, 256);         // Calls memcpy__fpvst
memcpy(audio, local_buf, 256);         // Calls memcpy__pvfpst
```

---

## Compilation Flow

### Example: User Calls `memcpy(far_ptr, local_ptr, 256)`

```
1. PARSER:
   - Recognize function call: memcpy
   - Parse arguments: (far_ptr_t, void*, size_t)

2. SEMANTIC ANALYZER:
   - Check argument types match some overload
   - Type checking: far_ptr_t ✓, void* ✓, size_t ✓

3. CODE GENERATOR:
   - Resolve overload: memcpy + (far_ptr_t, void*, size_t) → memcpy__fpvst
   - Mangle name: "memcpy__fpvst"
   - Emit code: "call memcpy__fpvst"
   - Store symbol: "memcpy__fpvst" (undefined external)

4. ASSEMBLER:
   - Recognize external symbol: "memcpy__fpvst"
   - Create relocation: R_ADDR16 for "memcpy__fpvst"

5. LINKER:
   - Resolve "memcpy__fpvst" in symbol table
   - Link to implementation from stdlib
   - Patch address in code

6. EXECUTION:
   - CPU calls memcpy__fpvst
   - Executes: far_memcpy_to_local(local_buf, audio, 256)
```

---

## Error Handling

### Compilation Errors

```c
far_ptr_t ptr = ...;
int* local = ...;

memcpy(ptr, local, 256);  // ERROR: no overload matches (far_ptr_t, int*, size_t)

// Compiler message:
// error: no overload of 'memcpy' matches argument types
//   candidate: memcpy(void*, void*, size_t)
//   candidate: memcpy(far_ptr_t, void*, size_t)
//   candidate: memcpy(void*, far_ptr_t, size_t)
//   candidate: memcpy(far_ptr_t, far_ptr_t, size_t)
```

### Link Errors

```
ld: undefined reference to `memcpy__fpfpst'
// This means the far-to-far variant wasn't implemented in stdlib
```

---

## Integration Points

### With Compiler (cc45)

1. **Lexer**: No changes (overloading is not syntactic)
2. **Parser**: Allow duplicate function names
3. **Validator**: Check type compatibility for overloads
4. **CodeGenerator**: 
   - Resolve overload at call site
   - Generate mangled symbol name
5. **Assembler**: Pass mangled name through

### With Linker (ln45)

1. **Symbol table**: Parse mangled names
2. **Relocation**: Resolve mangled symbols
3. **Error messages**: Demangle symbols for readability

### With Standard Library (lib45)

1. **Headers**: Declare all overload variants
2. **Implementations**: Provide variants in archive
3. **Symbol export**: Each variant gets its own mangled symbol

---

## Phase 28 Scope

### What Phase 28 Implements

✅ Name mangling scheme (parameter types → mangled names)  
✅ Compiler parameter resolution (select correct overload)  
✅ Linker symbol matching (resolve mangled names)  
✅ Library variants (memcpy, memset, string functions)  
✅ Error messages (helpful suggestions for mismatches)  

### What Phase 28 Does NOT Do

❌ Virtual function dispatch (requires runtime polymorphism)  
❌ Template instantiation (would require template support)  
❌ Implicit conversions beyond pointer-to-void  
❌ Overload resolution with numeric type coercion  

---

## Performance Model

### Compile-Time Cost

- Overload resolution: O(n) linear search through variants (n ≈ 4-10)
- Name mangling: O(m) where m = number of parameters (m ≈ 2-5)
- **Total**: ~100 cycles per function call to resolve

### Runtime Cost

- **Zero overhead**: Direct call to selected variant
- No dispatch table, no runtime checks
- Same performance as hand-written `far_memcpy_to_local()`

### Code Size

- Library: ~500 additional bytes (4-5 memcpy variants)
- Binary: No overhead (linker only includes used variants)

---

## Testing Strategy

### Compiler Tests

1. **Overload resolution** (10 tests)
   - Exact match, implicit conversion, no match
   - Error messages with available overloads

2. **Name mangling** (8 tests)
   - All type combinations
   - Correct mangling for complex signatures

3. **Code generation** (6 tests)
   - Correct mangled symbol in assembly
   - Proper call instruction

### Linker Tests

1. **Symbol resolution** (8 tests)
   - Find mangled symbols
   - Link error for missing variants

2. **Relocation** (4 tests)
   - Correct address patching
   - Cross-module linking

### Integration Tests

1. **End-to-end** (6 tests)
   - Compile with overloads
   - Link and execute
   - Verify correct variant called

---

## Known Limitations

### Not True C++ Style Overloading

- No SFINAE, no template specialization
- No runtime dispatch
- Limited to compile-time-determinable types

### Conservative Type Matching

- No implicit numeric conversions (int→long, float→double)
- Pointers must match exactly (unless to/from void*)
- User responsible for correct types at call site

### Incremental Adoption

- Phase 28 focuses on critical functions (memcpy, strlen, strcpy)
- Can extend to more functions later without breaking ABI

---

## References

- **Phase 26-27**: Far pointer infrastructure
- **Compiler Architecture**: cc45 CodeGenerator, mangling
- **Linker Architecture**: ln45 symbol resolution
- **Standard Library**: lib45 implementation

---

**Phase 28 brings transparent overloading to standard library functions!** 🎯

With parameter-based function selection, developers can write polymorphic code that works seamlessly with both local and far pointers, dramatically improving API usability while maintaining full type safety and zero runtime overhead.
