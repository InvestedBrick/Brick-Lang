#pragma once
#include <iostream>
class Arena_allocator {
private:
	size_t m_size;
	std::byte* m_buffer;
	std::byte* m_offset;
public:
	inline explicit Arena_allocator(size_t bytes) : m_size(bytes) {
		m_buffer = static_cast<std::byte*>(malloc(m_size));
		m_offset = m_buffer;
	}
	inline Arena_allocator(const Arena_allocator& other) = delete;//delete copy constructor
	inline Arena_allocator operator=(const Arena_allocator& other) = delete;//delete copy assignment operator 
	inline ~Arena_allocator()
	{
		free(m_buffer);
	}
	template<typename T>
	inline T* alloc() {
		void* offset = m_offset;
		m_offset += sizeof(T);
		//std::cout << "Allocated " << sizeof(T) << " bytes" << std::endl;
		return static_cast<T*>(offset);
	}
};