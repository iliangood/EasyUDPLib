#if !defined MESSAGE_H
#define MESSAGE_H


#include <cstddef>
#include <inttypes.h>
#include <type_traits>
#include <cstring>
#include <stdexcept>

template <size_t N>
class Message
{
	uint8_t array_[N];
	size_t size_;
	size_t readPtr_;
public:
	Message() : size_(0), readPtr_(0) 
	{}

	size_t getSize() const
	{
		return size_;
	}

	size_t size() const
	{
		return getSize();
	}

	size_t getCapacity() const
	{
		return N;
	}

	size_t capacity() const
	{
		return getCapacity;
	}

	size_t getSpace() const
	{
		return N - size_;
	}

	size_t space() const
	{
		return getSpace();
	}

	uint8_t* getData()
	{
		return array_;
	}

	uint8_t* data()
	{
		return getData();
	}

	uint8_t* begin()
	{
		return getData();
	}

	const uint8_t* begin() const
	{
		return getData();
	}

	const uint8_t* getData() const
	{
		return array_;
	}

	const uint8_t* data() const
	{
		return getData();
	}

	uint8_t* getEnd()
	{
		return array_ + size_;
	}

	uint8_t* end()
	{
		return getEnd();
	}

	void addSize(size_t size)
	{
		if(size > space())
			throw std::length_error("Message::addSize(size_t) the size must be less than the remaining space");
		size_ = size_ + size;
	}
	
	size_t push(const uint8_t* data, size_t size)
	{
		if(size > getSpace())
		{
			throw std::length_error("Message::push(const uint8_t*, size_t) the data size must be less than the remaining space");
		}
		memcpy(getEnd(), data, size);
		size_ += size;
		return getSpace();
	}

	size_t push(const char* data)
	{
		return push((const uint8_t*)data, strlen(data) + 1);
	}

	template <typename T>
	size_t push(const T& data) //Возвращает оставшееся место
	{
		static_assert(std::is_trivially_copyable_v<T>(), "T must be trivially copyable (POD-like) for memcpy safety.");
		if(sizeof(T) > getSpace())
		{
			return getSpace();
		}
		memcpy(getEnd(), &data, sizeof(T));
		size_ += sizeof(T);
		return getSpace();
	}

	size_t pop(uint8_t* data, size_t size)
	{
		if(size > size_)
		{
			throw std::length_error("Message::pop(const uint8_t*, size_t) The size of the received data must not be greater than the size of the data in the message.");
		}

		memcpy(data, array_, size);
		memmove(array_, array_ + size, size_ - size);
		size_ -= size;
		return size_;
	}

	size_t pop_back(uint8_t* data, size_t size)
	{
		if(size > size_)
		{
			throw std::length_error("Message::pop(const uint8_t*, size_t) The size of the received data must not be greater than the size of the data in the message.");
		}

		memcpy(data, array_ + size_ - size, size);
		size_ -= size;
		return size_;
	}

	template <typename T>
	T pop() //Возвращает оставшийся размер
	{
		static_assert(std::is_trivially_copyable_v<T>(), "T must be trivially copyable (POD-like) for memcpy safety.");
		T data;
		if(sizeof(T) > size_)
		{
			throw std::length_error("Message::pop<T>() The size of the received data must not be greater than the size of the data in the message.");
		}
		pop((uint8_t*)&data, sizeof(T));
		return data;
	}

	template <typename T>
	T pop_back() //Возвращает оставшийся размер
	{
		static_assert(std::is_trivially_copyable_v<T>(), "T must be trivially copyable (POD-like) for memcpy safety.");
		T data;
		if(sizeof(T) > size_)
		{
			throw std::length_error("Message::pop_back<T>() The size of the received data must not be greater than the size of the data in the message.");
		}
		pop_back((uint8_t*)&data, sizeof(T));
		return data;
	}

	void clear()
	{
		size_ = 0;
	}

	size_t getReadPtr() const
	{
		return readPtr_;
	}

	void setReadPtr(size_t ptr)
	{
		if(ptr > size_)
			readPtr_ = size_;
		else
			readPtr_ = ptr;
	}

	template <typename T>
	T read()
	{
		static_assert(std::is_trivially_copyable_v<T>(), "T must be trivially copyable (POD-like) for memcpy safety.");
		T data;
		if(readPtr_ + sizeof(T) > size_)
		{
			throw std::out_of_range("Message::read<T>() attempt to access memory not owned by Message");
		}
		memcpy(&data, array_ + readPtr_, sizeof(T));
		readPtr_ += sizeof(T);
		return data;
	}
	
	char* readString()
	{
		if(readPtr_ >= size_)
			return nullptr;
		char* start = (char*)(array_ + readPtr_);
		size_t len = strnlen(start, size_ - readPtr_);
		if(readPtr_ + len >= size_) //Нет завершающего нуля
			return nullptr;
		readPtr_ += len + 1;
		return start;
	}
};

#endif