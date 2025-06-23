#include <chrono>
#include <iostream>
#include <cassert>
#include "arena_allocator.h" 

struct TestData 
{
    int a[16]; // 64 bytes
};

int main() {
    constexpr size_t arenas_size = 1024 * 1024 * 32; // 32 MB
    constexpr size_t iterations = 500000; // 32MB / 64

    kawa::arena_allocator arena(arenas_size, iterations);

    using clock = std::chrono::high_resolution_clock;

    auto start_push = clock::now();

    volatile size_t sanity_sum = 0;
    for (size_t i = 0; i < iterations; ++i) 
    {
        auto* ptr = arena.push<TestData>();
        ptr->a[0] = static_cast<int>(i);
        sanity_sum += ptr->a[0];
    }

    auto end_push = clock::now();

    // Begin timing for pop
    auto start_pop = clock::now();

    for (size_t i = 0; i < iterations; ++i) 
    {
        arena.pop();
    }

    auto end_pop = clock::now();

    auto push_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_push - start_push).count();
    auto pop_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_pop - start_pop).count();

    std::cout << "Pushed " << iterations << " TestData objects\n";
    std::cout << "Sanity check (should be > 0): " << sanity_sum << '\n';
    std::cout << "Memory used after pops: " << arena.occupied() << " bytes\n";
    std::cout << "Push time: " << push_duration << " ns (" << push_duration / 1e6 << " ms)\n";
    std::cout << "Pop time:  " << pop_duration << " ns (" << pop_duration / 1e6 << " ms)\n";

    assert(arena.occupied() == 0);
}
