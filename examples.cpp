#include <iostream>
#include "arena_allocator.h"  

struct Obj1 { char c; };
struct Obj2 { ~Obj2() {}; short s; };
struct Obj4 { int i; };
struct Obj8 { int64_t i64; };

int main()
{                                         
    constexpr size_t arena_size = 1024;   // 1 KB arena
    constexpr size_t max_entries = 32;    // Max 32 push entries

	// Provide constructor with capacity in bytes, and max ammount of simoltanious entries
    kawa::arena_allocator arena(arena_size, max_entries);

    std::cout << "Arena capacity: " << arena.capacity() << " bytes\n";

    // Typed push: reserves memory for Obj4, returns pointer (no constructor called)
    Obj4* a4 = arena.push<Obj4>();
    std::cout << "Typed push Obj4 at " << a4 << '\n';

    // Push and construct: allocates and constructs Obj2 using provided arguments
    Obj2* a2 = arena.push_and_construct<Obj2>(12);
    std::cout << "Push and construct Obj2 at " << a2 << '\n';

    // Raw push: allocate 5 bytes straight up
    void* raw_ptr = arena.push(5);
    std::cout << "Raw push 5 bytes at " << raw_ptr << '\n';

    // Scoped usage example - RAII pop for all pushes inside
    {
        kawa::arena_allocator::scoped scoped_arena(arena);

        Obj8* a8 = scoped_arena.push<Obj8>();
        std::cout << "Scoped typed push Obj8 at " << a8 << '\n';

        scoped_arena.push(10); 
        std::cout << "Scoped raw push 10 bytes\n";

		// Be aware that occupied() and scoped_occupied() might return values that can look wrong, but they're not
        // It happens because of automatic pointer alignment 
        std::cout << "Scoped \"scoped\" occupied bytes: " << scoped_arena.scoped_occupied() << '\n';

        std::cout << "Scoped occupied bytes: " << scoped_arena.occupied() << '\n';
    } // Pops all scoped allocations automatically here


    std::cout << "Arena occupied bytes after scoped block: " << arena.occupied() << '\n';

	// pop() queue pops items in reverse of them being pushed, so last pushed will be first popped
    arena.pop();

	// Be aware that pop() does not call destructors, it just reclaims the memory
	// So Obj2* a2 = arena.push_and_construct<Obj2>(12), that is next in pop queue will not be destructed on pop()
    // You need to ensure to call destructors of non-trivially destructable object before freeing memory
    a2->~Obj2();
    arena.pop();

    arena.pop();

    std::cout << "Arena occupied bytes after manual pop: " << arena.occupied() << '\n';

    return 0;
}
