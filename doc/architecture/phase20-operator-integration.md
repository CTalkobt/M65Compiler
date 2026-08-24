# Phase 20: Advanced Operators Compiler Integration

**Status**: Implementation Guide
**Target Version**: v1.0.11+

## Overview

Phase 20 integrates Phase 19's advanced operators into the MEGA65 C compiler by extending the Lexer, Parser, AST, and CodeGenerator.

## Component Changes

### 1. Lexer Changes (Lexer.cpp)

**Add Token Types:**

```cpp
enum class TokenType {
    // ... existing tokens ...
    
    // Phase 20: Advanced Operators
    PLUS_SAT,        // +|  (saturating add)
    MINUS_SAT,       // -|  (saturating sub)
    MUL_SAT,         // *|  (saturating mul)
    
    ROT_LEFT,        // <<< (bit rotate left)
    ROT_RIGHT,       // >>> (bit rotate right)
    
    MIN_OP,          // <> (minimum)
    MAX_OP,          // >< (maximum)
    
    LSHIFT_SAT,      // <<| (saturating left shift)
    RSHIFT_SAT,      // >>| (saturating right shift)
};
```

**Add to Lexer::scanToken() method:**

```cpp
case '+':
    if (peek() == '|') {
        advance();
        return Token(TokenType::PLUS_SAT, "+|", line);
    }
    return Token(TokenType::PLUS, "+", line);

case '*':
    if (peek() == '|') {
        advance();
        return Token(TokenType::MUL_SAT, "*|", line);
    }
    return Token(TokenType::MULTIPLY, "*", line);

case '<':
    if (peek() == '<') {
        advance();
        if (peek() == '<') {
            advance();
            return Token(TokenType::ROT_LEFT, "<<<", line);
        } else if (peek() == '|') {
            advance();
            return Token(TokenType::LSHIFT_SAT, "<<|", line);
        }
        return Token(TokenType::LSHIFT, "<<", line);
    } else if (peek() == '>') {
        advance();
        return Token(TokenType::MIN_OP, "<>", line);
    }
    return Token(TokenType::LESS_THAN, "<", line);

case '>':
    if (peek() == '>') {
        advance();
        if (peek() == '>') {
            advance();
            return Token(TokenType::ROT_RIGHT, ">>>", line);
        } else if (peek() == '|') {
            advance();
            return Token(TokenType::RSHIFT_SAT, ">>|", line);
        }
        return Token(TokenType::RSHIFT, ">>", line);
    } else if (peek() == '<') {
        advance();
        return Token(TokenType::MAX_OP, "><", line);
    }
    return Token(TokenType::GREATER_THAN, ">", line);

case '-':
    if (peek() == '|') {
        advance();
        return Token(TokenType::MINUS_SAT, "-|", line);
    }
    return Token(TokenType::MINUS, "-", line);
```

### 2. Parser Changes (Parser.cpp)

**Update Operator Precedence Table:**

Operators follow standard C precedence (see Phase 19 spec). Insert new operators at appropriate levels:

```cpp
// In Parser::getOperatorPrecedence()
precedence[TokenType::MUL_SAT] = 12;      // Same as *
precedence[TokenType::PLUS_SAT] = 11;     // Same as +
precedence[TokenType::MINUS_SAT] = 11;    // Same as -
precedence[TokenType::LSHIFT_SAT] = 9;    // Same as <<
precedence[TokenType::RSHIFT_SAT] = 9;    // Same as >>
precedence[TokenType::ROT_LEFT] = 9;      // Same as shift
precedence[TokenType::ROT_RIGHT] = 9;     // Same as shift
precedence[TokenType::MIN_OP] = 8;        // Same as < >
precedence[TokenType::MAX_OP] = 8;        // Same as < >
```

**Update Binary Expression Parser:**

```cpp
// In Parser::parseBinaryExpression() or similar
std::unique_ptr<Expression> Parser::parseBinaryExpression(
    std::unique_ptr<Expression> left,
    int minPrec) {
    
    while (currentToken.type != TokenType::EOL) {
        int prec = getOperatorPrecedence(currentToken.type);
        if (prec < minPrec) break;
        
        TokenType op = currentToken.type;
        advance();
        
        auto right = parsePrimary();
        
        // Check operator type and create appropriate node
        switch (op) {
            case TokenType::PLUS_SAT:
            case TokenType::MINUS_SAT:
            case TokenType::MUL_SAT:
            case TokenType::LSHIFT_SAT:
            case TokenType::RSHIFT_SAT:
                left = std::make_unique<SaturatingBinaryOp>(
                    op, std::move(left), std::move(right));
                break;
            
            case TokenType::ROT_LEFT:
            case TokenType::ROT_RIGHT:
                left = std::make_unique<BitRotationOp>(
                    op, std::move(left), std::move(right));
                break;
            
            case TokenType::MIN_OP:
            case TokenType::MAX_OP:
                left = std::make_unique<MinMaxOp>(
                    op, std::move(left), std::move(right));
                break;
            
            // ... existing operators ...
            default:
                left = std::make_unique<BinaryOp>(
                    op, std::move(left), std::move(right));
                break;
        }
        
        left = parseBinaryExpression(std::move(left), prec + 1);
    }
    
    return left;
}
```

### 3. AST Changes (ASTNode.h)

**Add New AST Node Types:**

```cpp
// Saturating arithmetic operations
class SaturatingBinaryOp : public Expression {
public:
    TokenType operatorType;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    
    SaturatingBinaryOp(TokenType op, 
                       std::unique_ptr<Expression> l,
                       std::unique_ptr<Expression> r)
        : operatorType(op), left(std::move(l)), right(std::move(r)) {}
    
    void accept(ASTVisitor* visitor) override {
        visitor->visit(*this);
    }
};

// Bit rotation operations
class BitRotationOp : public Expression {
public:
    TokenType rotationType;  // ROT_LEFT or ROT_RIGHT
    std::unique_ptr<Expression> value;
    std::unique_ptr<Expression> amount;
    
    BitRotationOp(TokenType type,
                  std::unique_ptr<Expression> val,
                  std::unique_ptr<Expression> amt)
        : rotationType(type), value(std::move(val)), amount(std::move(amt)) {}
    
    void accept(ASTVisitor* visitor) override {
        visitor->visit(*this);
    }
};

// Min/Max operations
class MinMaxOp : public Expression {
public:
    TokenType operationType;  // MIN_OP or MAX_OP
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    
    MinMaxOp(TokenType op,
             std::unique_ptr<Expression> l,
             std::unique_ptr<Expression> r)
        : operationType(op), left(std::move(l)), right(std::move(r)) {}
    
    void accept(ASTVisitor* visitor) override {
        visitor->visit(*this);
    }
};
```

### 4. Validator Changes (Validator.cpp)

**Add Type Checking:**

```cpp
void Validator::visit(SaturatingBinaryOp& node) {
    node.left->accept(this);
    node.right->accept(this);
    
    // Saturating ops only on integral types
    if (!isIntegralType(node.left->getType())) {
        error("Saturating operators only work on integral types", node.getLine());
    }
    if (!isIntegralType(node.right->getType())) {
        error("Saturating operators only work on integral types", node.getLine());
    }
    
    // Result type same as operands
    node.setType(node.left->getType());
}

void Validator::visit(BitRotationOp& node) {
    node.value->accept(this);
    node.amount->accept(this);
    
    // Rotation only on integral types
    if (!isIntegralType(node.value->getType())) {
        error("Bit rotation only works on integral types", node.getLine());
    }
    
    // Rotation amount should be integral (or constexpr)
    if (!isIntegralType(node.amount->getType())) {
        error("Rotation amount must be integral", node.getLine());
    }
    
    // Result type same as value
    node.setType(node.value->getType());
}

void Validator::visit(MinMaxOp& node) {
    node.left->accept(this);
    node.right->accept(this);
    
    // Min/max work on comparable types
    if (node.left->getType() != node.right->getType()) {
        // Allow type promotion for mixed types
        // ... standard type promotion logic ...
    }
    
    node.setType(node.left->getType());
}
```

### 5. CodeGenerator Changes (CodeGenerator.cpp)

**Generate Code for Saturating Operations:**

```cpp
void CodeGenerator::visit(SaturatingBinaryOp& node) {
    std::string result_reg = allocateRegister();
    
    // Evaluate left and right operands
    std::string left_val = visit(node.left.get());
    std::string right_val = visit(node.right.get());
    
    Type* type = node.left->getType();
    int type_bits = type->getSizeInBits();
    
    std::string maxVal = getMaxValueForType(type);
    std::string minVal = getMinValueForType(type);
    
    switch (node.operatorType) {
        case TokenType::PLUS_SAT:
            // Check: left > 0 && right > 0 && left > MAX - right
            emitCode("  // Saturating add");
            emitCode("  lda " + left_val);
            emitCode("  clc");
            emitCode("  adc " + right_val);
            emitCode("  bcs @saturate_max_" + result_reg);
            emitCode("  jmp @saturate_done_" + result_reg);
            emitCode("@saturate_max_" + result_reg + ":");
            emitCode("  lda #" + maxVal);
            emitCode("@saturate_done_" + result_reg + ":");
            emitCode("  sta " + result_reg);
            break;
        
        case TokenType::MINUS_SAT:
            // Similar for subtraction with borrow flag
            emitCode("  // Saturating subtract");
            emitCode("  lda " + left_val);
            emitCode("  sec");
            emitCode("  sbc " + right_val);
            emitCode("  bcs @saturate_done_" + result_reg);
            emitCode("  lda #" + minVal);
            emitCode("@saturate_done_" + result_reg + ":");
            emitCode("  sta " + result_reg);
            break;
        
        // ... other saturating ops ...
    }
    
    return result_reg;
}

void CodeGenerator::visit(BitRotationOp& node) {
    std::string value_reg = visit(node.value.get());
    std::string amount_reg = visit(node.amount.get());
    
    // If amount is constant, optimize
    if (auto* lit = dynamic_cast<IntegerLiteral*>(node.amount.get())) {
        int rotation = lit->getValue();
        
        switch (node.rotationType) {
            case TokenType::ROT_LEFT:
                for (int i = 0; i < rotation; ++i) {
                    emitCode("  rol " + value_reg);
                }
                break;
            
            case TokenType::ROT_RIGHT:
                for (int i = 0; i < rotation; ++i) {
                    emitCode("  ror " + value_reg);
                }
                break;
            
            default:
                break;
        }
    } else {
        // Runtime rotation (more complex)
        // Generate loop for runtime amount
        emitCode("  // Runtime bit rotation");
        // ... rotation loop ...
    }
    
    return value_reg;
}

void CodeGenerator::visit(MinMaxOp& node) {
    std::string left_val = visit(node.left.get());
    std::string right_val = visit(node.right.get());
    std::string result_reg = allocateRegister();
    
    emitCode("  // Min/Max operation");
    emitCode("  lda " + left_val);
    
    switch (node.operationType) {
        case TokenType::MIN_OP:
            emitCode("  cmp " + right_val);
            emitCode("  bmi @min_done_" + result_reg);  // left < right, done
            emitCode("  lda " + right_val);
            break;
        
        case TokenType::MAX_OP:
            emitCode("  cmp " + right_val);
            emitCode("  bpl @max_done_" + result_reg);  // left >= right, done
            emitCode("  lda " + right_val);
            break;
        
        default:
            break;
    }
    
    emitCode("@" + 
             (node.operationType == TokenType::MIN_OP ? "min" : "max") + 
             "_done_" + result_reg + ":");
    emitCode("  sta " + result_reg);
    
    return result_reg;
}
```

## Operator Overloading Support

Phase 20 operators fully support operator overloading, allowing custom types to define saturating, rotation, and min/max behaviors.

### Overloadable Operators

```cpp
struct Fixed {
    int16_t value;
    
    // Saturating addition
    struct Fixed operator+|(struct Fixed other) {
        int32_t result = (int32_t)this->value + (int32_t)other.value;
        if (result > 32767) result = 32767;
        if (result < -32768) result = -32768;
        return (struct Fixed){(int16_t)result};
    }
    
    // Saturating subtraction
    struct Fixed operator-|(struct Fixed other) {
        int32_t result = (int32_t)this->value - (int32_t)other.value;
        if (result > 32767) result = 32767;
        if (result < -32768) result = -32768;
        return (struct Fixed){(int16_t)result};
    }
    
    // Saturating multiplication
    struct Fixed operator*|(struct Fixed other) {
        int32_t result = (int32_t)this->value * (int32_t)other.value;
        if (result > 32767) result = 32767;
        if (result < -32768) result = -32768;
        return (struct Fixed){(int16_t)result};
    }
    
    // Min operator
    struct Fixed operator<>(struct Fixed other) {
        return this->value < other.value ? *this : other;
    }
    
    // Max operator
    struct Fixed operator><(struct Fixed other) {
        return this->value > other.value ? *this : other;
    }
};
```

### Audio Use Case: Safe Volume Mixing

```cpp
struct AudioSample {
    int16_t level;  // -32768 to 32767
    
    // Saturating add for safe mixing
    struct AudioSample operator+|(struct AudioSample other) {
        int32_t mixed = (int32_t)this->level + (int32_t)other.level;
        // Clamp to valid range
        if (mixed > 32767) mixed = 32767;
        if (mixed < -32768) mixed = -32768;
        return (struct AudioSample){(int16_t)mixed};
    }
    
    // Min for safety limiter
    struct AudioSample operator<>(struct AudioSample max_level) {
        int16_t clamped = this->level < max_level.level 
            ? this->level : max_level.level;
        return (struct AudioSample){clamped};
    }
};

// Usage
struct AudioSample ch1 = {20000};
struct AudioSample ch2 = {15000};
struct AudioSample mixed = ch1 +| ch2;  // Safe: 32767, not overflow
struct AudioSample limited = mixed <> (struct AudioSample){30000};  // Limiter
```

### Graphics Use Case: Sprite Rotation

```cpp
struct Sprite {
    uint8_t bits[8];  // 8-byte sprite pattern
    int width;
    
    struct Sprite rotate_left(int times) {
        struct Sprite rotated = *this;
        for (int i = 0; i < times; ++i) {
            for (int j = 0; j < 8; ++j) {
                rotated.bits[j] = rotated.bits[j] <<< 1;
            }
        }
        return rotated;
    }
};
```

### Color Component Safety

```cpp
struct RGBColor {
    uint8_t r, g, b;
    
    // Saturating addition for safe color mixing
    struct RGBColor operator+|(struct RGBColor other) {
        return (struct RGBColor){
            this->r +| other.r,
            this->g +| other.g,
            this->b +| other.b
        };
    }
    
    // Min/Max for clamping color channels
    uint8_t clamp_channel(uint8_t channel, uint8_t max_val) {
        return channel <> max_val;  // Min: clamp to max_val
    }
};
```

### Validator Changes for Overloading

Update `Validator.cpp` to recognize overloaded operators:

```cpp
void Validator::visit(SaturatingBinaryOp& node) {
    // ... existing integral type checking ...
    
    // Check for operator overloading on struct types
    Type* leftType = node.left->getType();
    if (leftType->isStructType()) {
        std::string opName = getOperatorFunctionName(node.operatorType);
        // operator+|, operator-|, operator*|, operator<|, operator>|
        
        if (!hasOperatorOverload(leftType, opName)) {
            error("Type '" + leftType->getName() + 
                  "' does not overload operator " + opName, 
                  node.getLine());
        }
    }
}

std::string getOperatorFunctionName(TokenType op) {
    switch (op) {
        case TokenType::PLUS_SAT: return "operator+|";
        case TokenType::MINUS_SAT: return "operator-|";
        case TokenType::MUL_SAT: return "operator*|";
        case TokenType::LSHIFT_SAT: return "operator<<|";
        case TokenType::RSHIFT_SAT: return "operator>>|";
        case TokenType::ROT_LEFT: return "operator<<<";
        case TokenType::ROT_RIGHT: return "operator>>>";
        case TokenType::MIN_OP: return "operator<>";
        case TokenType::MAX_OP: return "operator><";
        default: return "unknown";
    }
}

bool hasOperatorOverload(Type* type, const std::string& opName) {
    if (!type->isStructType()) return false;
    
    StructType* structType = static_cast<StructType*>(type);
    for (const auto& method : structType->getMethods()) {
        if (method->getName() == opName) {
            return true;
        }
    }
    return false;
}
```

### CodeGenerator Changes for Overloaded Operators

When overloaded, emit function call instead of inline operation:

```cpp
void CodeGenerator::visit(SaturatingBinaryOp& node) {
    Type* leftType = node.left->getType();
    
    if (leftType->isStructType()) {
        // Emit struct method call for overloaded operator
        std::string methodName = getOperatorFunctionName(node.operatorType);
        emitStructMethodCall(methodName, node.left, node.right);
    } else {
        // Original inline code for built-in types
        // ... existing implementation ...
    }
}

void CodeGenerator::emitStructMethodCall(
    const std::string& methodName,
    std::unique_ptr<Expression>* lhs,
    std::unique_ptr<Expression>* rhs) {
    
    // Generate: Type result = lhs.methodName(rhs)
    std::string lhs_reg = visit(lhs->get());
    std::string rhs_reg = visit(rhs->get());
    
    emitCode("  // Calling overloaded operator");
    emitCode("  jsr " + methodName);  // Call operator overload
    // ... handle return value and cleanup ...
}
```

## Testing Strategy

### Compiler Test Cases

Create test files demonstrating each operator:

**test_phase20_saturating_add.c:**
```c
#include <stdio.h>

int main() {
    unsigned char a = 200, b = 100;
    unsigned char result = a +| b;  // Should be 255, not overflow
    printf("200 +| 100 = %d\n", result);
    return 0;
}
```

**test_phase20_rotation.c:**
```c
#include <stdio.h>

int main() {
    unsigned char sprite = 0xA5;  // 10100101
    unsigned char rotated = sprite <<< 1;  // 01001011
    printf("0xA5 <<< 1 = 0x%02X\n", rotated);
    return 0;
}
```

**test_phase20_minmax.c:**
```c
#include <stdio.h>

int main() {
    int x = 50, y = 30;
    int min_val = x <> y;
    int max_val = x >< y;
    printf("min(50,30)=%d, max(50,30)=%d\n", min_val, max_val);
    return 0;
}
```

### Integration Tests

Run full compiler test suite to ensure no regressions:
```bash
make test
```

## Integration Checklist

- [ ] Add token types to Lexer.h
- [ ] Implement token recognition in Lexer::scanToken()
- [ ] Update operator precedence table in Parser
- [ ] Update binary expression parser
- [ ] Add AST node types to ASTNode.h
- [ ] Add validator logic for new operations
- [ ] Implement code generation for each operator
- [ ] Add test cases for each operator
- [ ] Update documentation with operator syntax
- [ ] Run full compiler test suite
- [ ] Verify code generation is optimal (minimal instruction count)
- [ ] Create integration test examples

## Performance Targets

- **Saturating Add**: 5-7 bytes assembly (CMP, BCS, LDA, STA)
- **Bit Rotation (const)**: ROL/ROR per rotation (1 byte each)
- **Bit Rotation (dynamic)**: Loop with counter (15-20 bytes)
- **Min/Max**: 4-5 bytes assembly (CMP, BPL/BMI, LDA, STA)
- **Saturating Shift (const)**: Inline with overflow check (8-10 bytes)

## Known Considerations

1. **Operator Precedence**: Must match C standards to avoid surprise results
2. **Type Compatibility**: Saturating ops only on integral types
3. **Constant Folding**: Optimize constant expressions at compile time
4. **Overflow Detection**: Use 6502 flags (carry, overflow) efficiently
5. **Code Size**: Balance safety with generated code size

## Success Criteria

✅ All 20 Phase 19 operator tests pass when compiled  
✅ No regressions in existing compiler test suite  
✅ Code generation produces optimal 6502 instruction sequences  
✅ Operator precedence matches C standards  
✅ Type checking prevents unsafe operations  

## Future Enhancements

- SIMD-like batch operations using these operators
- Vector support with saturating arithmetic
- Hardware intrinsics for advanced operations
- Profile-guided optimization of saturation checks
