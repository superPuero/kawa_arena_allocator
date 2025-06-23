#include "arena_allocator.h"
#include <filesystem>
#include <fstream>

using namespace kawa;
void process_file(arena_allocator& alloc, const std::filesystem::path& path)
{
	auto salloc = alloc.scope();

	size_t file_size = std::filesystem::file_size(path);

	char* buffer = (char*)salloc.push(file_size + 1);

	buffer[file_size] = '\0';

	std::ifstream file(path);						

	file.read(buffer, file_size);

	std::cout << buffer << '\n';
}

int main()
{
	arena_allocator al(1024 * 32, 32);
	process_file(al, "examples.cpp");
}