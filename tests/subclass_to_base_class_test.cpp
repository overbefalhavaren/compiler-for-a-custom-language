#include <memory>
#include <utility>
#include <iostream>

#define TEST_ERROR_INSTANCE false

class Base {
public:
    virtual ~Base() = default;
    virtual void print() const = 0;
};

class Child1 : public Base {
public:
    Child1() = default;
    void print() const {
        std::cout << "Child1\n";
    }
};

class Child2 : public Base {
public:
    Child2() = default;
    void print() const {
        std::cout << "Child2\n";
    }
};

class Other {
public:
    Other() = default;
    ~Other() = default;
    void print() const {
        std::cout << "Other\n";
    }
};

template <typename T>
inline std::unique_ptr<Base> makePtr(T obj) {
    static_assert(std::is_base_of_v<Base, T>, "T must derive from Base");
    return std::make_unique<T>(std::move(obj));
}

int main() {
    Child1 child1 = Child1();
    auto ptr1 = makePtr<Child1>(std::move(child1));
    ptr1->print();

    Child2 child2 = Child2();
    auto ptr2 = makePtr<Child2>(std::move(child2));
    ptr2->print();

#if TEST_ERROR_INSTANCE
    Other other = Other();
    auto ptr3 = makePtr<Other>(std::move(other));
    ptr3->print();
#endif

    return 0;
}
