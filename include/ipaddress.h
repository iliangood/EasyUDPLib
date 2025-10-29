#if !defined IP_ADDRESS_H
#define IP_ADDRESS_H

#include <inttypes.h>
#include <string>
#include <sstream>
#include <optional>
#include <cassert>
#include <array>
#include <bit>
#include <type_traits>
#include <format>


static_assert(__cplusplus >= 202002L,
"This library does not support c++ older then c++20!"
);

static_assert
(
	std::endian::native == std::endian::little ||
	std::endian::native == std::endian::big,
	"This library does not support mixed-endian (PDP-endian) architectures!"
);


template<typename T>
constexpr T reverseByteOrder(T val)
{
	static_assert(std::is_trivially_copyable<T>(), "template<typename T> static constexpr T reverseByteOrder(T) T shoud be trivially copyable (POD-like)");
	std::array<uint8_t, sizeof(T)> res = std::bit_cast<std::array<uint8_t, sizeof(T)>>(val);
	for(size_t i = 0; i < sizeof(T)/2; ++i)
	{
		std::swap(res[i], res[sizeof(T)-1-i]);
	}
	return std::bit_cast<T>(res);
}

template<typename T>
constexpr T hton(T val)
{
	static_assert(std::is_trivially_copyable<T>(), "template<typename T> static constexpr T hton(T) T shoud be trivially copyable (POD-like)");
	if(std::endian::native == std::endian::big)
		return val;
	return reverseByteOrder(val);
}

template<typename T>
constexpr T ntoh(T val)
{
	static_assert(std::is_trivially_copyable<T>(), "template<typename T> static constexpr T ntoh(T) T shoud be trivially copyable (POD-like)");
	if(std::endian::native == std::endian::big)
		return val;
	return reverseByteOrder(val);
}

class IPAddress
{
	std::array<uint8_t, 4> octets_;

	constexpr IPAddress(uint32_t ip): // from Big-endian
	octets_(std::bit_cast<std::array<uint8_t, 4>>(ip))
	{}

	constexpr IPAddress(std::array<uint8_t, 4> ip): // from Big-endian
	octets_(ip)
	{}

	


public:

	constexpr IPAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth) noexcept :
	octets_{first, second, third, fourth}
	{}

	
	constexpr IPAddress(const IPAddress& other) noexcept = default;
	constexpr IPAddress& operator=(const IPAddress& other) noexcept = default;
	

	static constexpr IPAddress fromHost(uint32_t ip) noexcept // from Host-endian
	{
		return IPAddress(hton(ip));
	}
	static constexpr IPAddress fromNet(uint32_t ip) noexcept // from Big-endian
	{
		return IPAddress(ip);
	}

	static std::optional<IPAddress> fromString(std::string_view str);
	static std::optional<IPAddress> fromString(const char* str);
	std::string toString() const
	{
		return std::format("{}.{}.{}.{}", first(), second(), third(), fourth());
	}

	constexpr uint32_t toHost() const noexcept
	{
		return ntoh(std::bit_cast<uint32_t>(octets_));
	}
	constexpr uint32_t toNet() const noexcept
	{
		return std::bit_cast<uint32_t>(octets_);
	}
	
	constexpr uint8_t& operator[](size_t index) noexcept
	{
		assert(index < 4 && "uint8_t& IPAddress::operator[](size_t) IP address has only 4 octets");
		return octets_[index];
	}
	constexpr const uint8_t& operator[](size_t index) const noexcept
	{
		assert(index < 4 && "const uint8_t& IPAddress::operator[](size_t) IP address has only 4 octets");
		return octets_[index];
	}

	constexpr uint8_t& at(size_t index)
	{
		if(index >= 4)
			throw std::out_of_range("uint8_t& IPAddress::at(size_t) IP address has only 4 octets");
		return octets_[index];
	}
	constexpr const uint8_t& at(size_t index) const
	{
		if(index >= 4)
			throw std::out_of_range("const uint8_t& IPAddress::at(size_t) IP address has only 4 octets");
		return octets_[index];
	}

	constexpr const uint8_t& first() const noexcept  { return octets_[0]; }
	constexpr const uint8_t& second() const noexcept { return octets_[1]; }
	constexpr const uint8_t& third() const noexcept  { return octets_[2]; }
	constexpr const uint8_t& fourth() const noexcept { return octets_[3]; }

	constexpr uint8_t& first() noexcept  { return octets_[0]; }
	constexpr uint8_t& second() noexcept { return octets_[1]; }
	constexpr uint8_t& third() noexcept  { return octets_[2]; }
	constexpr uint8_t& fourth() noexcept { return octets_[3]; }

	constexpr bool operator==(const IPAddress& other) const noexcept { return octets_ == other.octets_; }
	constexpr bool operator!=(const IPAddress& other) const noexcept { return !(*this == other); }

	constexpr IPAddress operator&(const IPAddress& other) const noexcept
	{
		return IPAddress(toNet() & other.toNet());
	}
	constexpr IPAddress operator|(const IPAddress& other) const noexcept
	{
		return IPAddress(toNet() | other.toNet());
	}
	constexpr IPAddress operator^(const IPAddress& other)
	{
		return IPAddress(toNet() ^ other.toNet());
	}
	
	constexpr IPAddress& operator&=(const IPAddress& other)
	{
		*this = IPAddress(*this & other);
		return *this;
	}
	constexpr IPAddress& operator|=(const IPAddress& other)
	{
		*this = IPAddress(*this | other);
		return *this;
	}
	constexpr IPAddress& operator^=(const IPAddress& other)
	{
		*this = IPAddress(*this ^ other);
		return *this;
	}
	
	constexpr IPAddress operator~() const noexcept
	{
		return IPAddress(~this->toNet());
	}
	
	friend std::ostream& operator<<(std::ostream& stream, const IPAddress& ip);

};

inline constexpr IPAddress IP_BROADCAST{255, 255, 255, 255};
inline constexpr IPAddress IP_ANY{0, 0, 0, 0};
inline constexpr IPAddress IP_LOCALHOST{127, 0, 0, 1};


namespace std
{
	template<>
	struct hash<IPAddress>
	{
		size_t operator()(const IPAddress& ip) const noexcept
		{
			return std::hash<uint32_t>{}(ip.toNet());
		}
	};
}

#endif