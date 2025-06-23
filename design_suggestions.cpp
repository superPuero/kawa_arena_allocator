#include "arena_allocator.h"
#include <filesystem>


using namespace kawa;
void process_file(arena_allocator& alloc, const std::filesystem::path& path)
{
	auto salloc = alloc.scope();

	char* buffer = (char*)salloc.push(std::filesystem::file_size(path));
}


int main()
{
	arena_allocator al(1024 * 32, 32);
	process_file(al, "examples.cpp");
}