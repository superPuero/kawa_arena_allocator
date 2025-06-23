#include <iostream>
#include "arena_allocator.h"  

struct Obj1 { char c; };
struct Obj2 { ~Obj2() { std::cout << "Obj2 destroyed\n"; } short s; };
struct Obj4 { int i; };
struct Obj8 { int64_t i64; };

int main()
{
    using namespace kawa;
    constexpr size_t arena_size = 1024;   // 1 KB arena
    constexpr size_t max_entries = 32;    // Max 32 push entries

    // Provide constructor with capacity in bytes, and max amount of simultaneous entries
    arena_allocator arena(arena_size, max_entries);

    std::cout << "Arena capacity: " << arena.capacity() << " bytes\n";

    // Push and construct: allocates and constructs Obj2 using provided arguments (none in this case)
    Obj4* a4 = arena.push<Obj4>();
    std::cout << "Typed push Obj4 at " << a4 << '\n';

    // Push and construct: allocates and constructs Obj2 using provided arguments (12 in this case)
    Obj2* a2 = arena.push<Obj2>(12);
    std::cout << "Push and construct Obj2 at " << a2 << '\n';

    // Raw push: allocate 5 bytes directly
    void* raw_ptr = arena.push(5);
    std::cout << "Raw push 5 bytes at " << raw_ptr << '\n';

    // Scoped usage example - RAII rollback for all pushes inside
    {
        // Get scoped allocator with scope() method
        arena_allocator::scoped scoped_arena = arena.scope();

        Obj8* a8 = scoped_arena.push<Obj8>();
        std::cout << "Scoped typed push Obj8 at " << a8 << '\n';

        scoped_arena.push(10);
        std::cout << "Scoped raw push 10 bytes\n";

        // Be aware that occupied() and scoped_occupied() may reflect alignment padding
        std::cout << "Scoped \"scoped\" occupied bytes: " << scoped_arena.scoped_occupied() << '\n';
        std::cout << "Scoped occupied bytes: " << scoped_arena.occupied() << '\n';
    } // Automatically pops and destroys all scoped allocations

    std::cout << "Arena occupied bytes after scoped block: " << arena.occupied() << '\n';

    // pop() queue pops items in reverse order (LIFO)
    arena.pop(); // raw allocation (5 bytes) — no destructor called
    arena.pop(); // Obj2 - no destructor call for trivially destructable object
    arena.pop(); // Obj4 — destructor called automatically

    std::cout << "Arena occupied bytes after manual pop: " << arena.occupied() << '\n';

    return 0;
}
