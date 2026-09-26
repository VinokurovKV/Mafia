#include "mafia/shared_ptr.hpp"

#include <cassert>
#include <utility>

namespace {

struct TrackedValue {
    explicit TrackedValue(int value)
        : value(value) {
        ++aliveCount;
    }

    ~TrackedValue() {
        --aliveCount;
    }

    int value;
    static inline int aliveCount = 0;
};

void testDefaultConstruction() {
    const mafia::SharedPtr<TrackedValue> pointer;

    assert(pointer.get() == nullptr);
    assert(pointer == nullptr);
    assert(nullptr == pointer);
    assert(pointer.useCount() == 0);
}

void testCopying() {
    auto first = mafia::makeShared<TrackedValue>(10);
    assert(first.useCount() == 1);

    {
        mafia::SharedPtr<TrackedValue> second(first);
        assert(first == second);
        assert(first.useCount() == 2);
        assert(second.useCount() == 2);

        mafia::SharedPtr<TrackedValue> third;
        third = second;
        assert(third == first);
        assert(first.useCount() == 3);

        third = third;
        assert(third.useCount() == 3);
    }

    assert(first.useCount() == 1);
}

void testMoving() {
    auto source = mafia::makeShared<TrackedValue>(20);
    auto moved = std::move(source);

    assert(source == nullptr);
    assert(moved != nullptr);
    assert(moved.useCount() == 1);

    mafia::SharedPtr<TrackedValue> assigned;
    assigned = std::move(moved);

    assert(moved == nullptr);
    assert(assigned->value == 20);
    assert(assigned.useCount() == 1);
}

void testDereferenceResetAndSwap() {
    auto first = mafia::makeShared<TrackedValue>(30);
    auto second = mafia::makeShared<TrackedValue>(40);

    assert((*first).value == 30);
    assert(second->value == 40);
    assert(first != second);

    first.swap(second);
    assert(first->value == 40);
    assert(second->value == 30);

    swap(first, second);
    assert(first->value == 30);
    assert(second->value == 40);

    first.reset(new TrackedValue(50));
    assert(first->value == 50);
    assert(first.useCount() == 1);

    first.reset();
    assert(first == nullptr);
}

void testLastOwnerDestroysObject() {
    assert(TrackedValue::aliveCount == 0);

    {
        auto first = mafia::makeShared<TrackedValue>(60);
        assert(TrackedValue::aliveCount == 1);

        {
            auto second = first;
            assert(TrackedValue::aliveCount == 1);
        }

        assert(TrackedValue::aliveCount == 1);
    }

    assert(TrackedValue::aliveCount == 0);
}

}  // namespace

int main() {
    testDefaultConstruction();
    testCopying();
    testMoving();
    testDereferenceResetAndSwap();
    testLastOwnerDestroysObject();
}
