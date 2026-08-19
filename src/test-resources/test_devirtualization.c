// Test devirtualization detection for virtual methods
// Phase 86: Detects single-implementation virtual methods for direct call optimization

struct Shape {
    int width;
    int height;

    // Virtual method with single known implementation
    virtual int area() {
        return width * height;
    }

    // Virtual method (can be overridden by derived classes)
    virtual int perimeter() {
        return 2 * (width + height);
    }
};

// Derived class with override
struct Circle : Shape {
    // Override the area calculation
    virtual int area() {
        return 314;  // Approximate pi * r^2
    }
};

// Helper to access shape area (normally goes through vtable)
int get_area(struct Shape* s) {
    return s->area();
}

int main() {
    struct Shape rect;
    rect.width = 10;
    rect.height = 20;

    struct Circle circle;
    circle.width = 5;
    circle.height = 5;

    // In a scenario with only rect (no Circle instances),
    // rect.area() could be devirtualized
    int area1 = rect.area();

    // Polymorphic call (goes through vtable)
    struct Shape* shape_ptr = &circle;
    int area2 = shape_ptr->area();

    return area1 + area2;
}
