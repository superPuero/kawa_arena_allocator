#pragma once
#include <memory>
#include <iostream>
#include <new>

#if defined(_DEBUG)

#if defined(_MSC_VER)
#include <intrin.h>
#define KW_ARENA_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define KW_ARENA_DEBUG_BREAK() __builtin_trap()
#else
#define KW_ARENA_DEBUG_BREAK() ((void)0)
#endif

#define KW_ARENA_ASSERT_MSG(expr, msg) \
        do { \
            if (!(expr)) { \
                std::cout << msg << '\n'; \
                KW_ARENA_DEBUG_BREAK(); \
            } \
        } while(0)

#else
#define KW_ARENA_ASSERT_MSG(expr, msg) ((void)0)
#endif

namespace kawa
{
	class arena_allocator
	{
	public:
		class scoped
		{
			public:
				scoped(arena_allocator& aa)	noexcept
					: _source(aa)
				{
					_begin_occupied = _source.occupied();
				};

				scoped(const scoped& other) noexcept = delete;

				scoped(scoped&&) = delete;

				~scoped() noexcept
				{
					while(_scoped_entries)
					{
						_source.pop();
						_scoped_entries--;
					}
				}

			public:
				template<typename T, typename...Args>
				inline T* push(Args&&...args) noexcept
				{
					_scoped_entries++;
					return _source.push<T>(std::forward<Args>(args)...);
				}

				inline void* push(size_t size)	noexcept
				{
					_scoped_entries++;
					return _source.push(size);
				}

				inline void pop() noexcept
				{
					return _source.pop();
				}

				inline size_t entries_occupied() const noexcept
				{
					return _source.entries_occupied();
				}

				inline size_t occupied() const noexcept
				{
					return _source.occupied();
				}

				inline size_t capacity() const noexcept
				{
					return _source.capacity();
				}

				inline size_t scoped_occupied() const noexcept
				{
					return _source.occupied() - _begin_occupied;
				}

				inline size_t scoped_entries_occupied() const noexcept
				{
					return _source.entries_occupied() - _scoped_entries;
				}

			private:
				arena_allocator& _source;
				size_t _begin_occupied = 0;
				size_t _scoped_entries = 0;
		};

	public:
		inline arena_allocator(size_t bytes, size_t entries) noexcept
		{
			_capacity = bytes;
			_data = static_cast<char*>(::operator new(bytes, std::align_val_t{ 8 }));
		
			_current = _data;

			_entries_capacity = entries;
			_sizes = new size_t[entries];
			_strides = new size_t[entries];
			_destructors = new destructor_fn_t[entries]();
		}

		inline arena_allocator(const arena_allocator&) noexcept = delete;

		inline arena_allocator(arena_allocator&& other) noexcept
			: _data(other._data)
			, _current(other._current)
			, _capacity(other._capacity)
			, _sizes(other._sizes)
			, _strides(other._strides)
			, _entries_capacity(other._entries_capacity)
			, _entries_occupied(other._entries_occupied)
			, _destructors(other._destructors)
		{
			other._data = nullptr;
			other._current = nullptr;
			other._capacity = 0;
			other._sizes = nullptr;
			other._strides = nullptr;
			other._destructors = nullptr;

			other._entries_occupied = 0;
			other._entries_capacity = 0;
		}

		inline ~arena_allocator() noexcept
		{
			while (_entries_occupied)
			{
				pop();
			}

			::operator delete(_data, std::align_val_t{ alignof(std::max_align_t) });
			delete[] _sizes;
			delete[] _strides;
			delete[] _destructors;
		}


		template<typename T,  typename...Args>
		inline T* push(Args&&...args) noexcept
		{	
			KW_ARENA_ASSERT_MSG((_current - _data + sizeof(T)) <= _capacity, "Arena allocator: Not enough memory to push object.");
			KW_ARENA_ASSERT_MSG(_entries_occupied < _entries_capacity, "Arena allocator: Maximum entries exceeded.");

			void* aligned = _current;
			size_t space = _capacity - (_current - _data);
			if (std::align(alignof(T), sizeof(T), aligned, space)) 
			{
				_sizes[_entries_occupied] = sizeof(T);
				_strides[_entries_occupied] = ((size_t)aligned - (size_t)_current);

				_current = static_cast<char*>(aligned) + sizeof(T);

				if constexpr (!std::is_trivially_destructible_v<T>)
				{
					_destructors[_entries_occupied] = [](void* ptr) 
						{
							reinterpret_cast<T*>(ptr)->~T();
						};
				}
				else
				{
					_destructors[_entries_occupied] = nullptr;
				}

				_entries_occupied++;

				return new(aligned) T(std::forward<Args>(args)...);
			}

			KW_ARENA_ASSERT_MSG((_current - _data + sizeof(T)) <= _capacity, "Arena allocator: Not enough aligned memory to push object.");
		}		

		inline void* push(size_t size)	noexcept
		{
			KW_ARENA_ASSERT_MSG(size, "Arena allocator: Cannot push zero-sized object.");
			KW_ARENA_ASSERT_MSG((_current - _data + size) <= _capacity, "Arena allocator: Not enough memory to push object.");
			KW_ARENA_ASSERT_MSG(_entries_occupied < _entries_capacity, "Arena allocator: Maximum entries exceeded.");

			void* ptr = _current;
			_current += size;

			_sizes[_entries_occupied] = size;
			_strides[_entries_occupied] = 0;

			_entries_occupied++;
			return ptr;
		}

		inline void pop() noexcept
		{
			KW_ARENA_ASSERT_MSG(_entries_occupied, "Arena allocator: overusing pop()");

			_entries_occupied--;

			_current -= _sizes[_entries_occupied];

			destructor_fn_t dtor = _destructors[_entries_occupied];

			if (dtor)
			{
				dtor(_current);
				dtor = nullptr;
			}

			_current -= _strides[_entries_occupied];
		}

		inline size_t capacity() const noexcept
		{
			return _capacity;
		}

		inline size_t entries_occupied() const noexcept
		{
			return _entries_occupied;
		}

		inline size_t occupied() const noexcept
		{
			return _current - _data;
		}

	public:
		inline arena_allocator::scoped scope() noexcept 
		{
			return arena_allocator::scoped(*this);
		}

	private:
		using destructor_fn_t = void(*)(void*);

		char*	_data					= nullptr;
		char*	_current				= nullptr;

		size_t	_capacity				= 0;

		size_t	_entries_capacity		= 0;
		size_t* _sizes					= nullptr;
		size_t* _strides				= nullptr;
		size_t	_entries_occupied		= 0;

		destructor_fn_t* _destructors	= nullptr;

	};
}

