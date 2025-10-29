#include <IPAddress.h>

std::optional<IPAddress> IPAddress::fromString(std::string_view str)
{
	std::array<uint8_t, 4> octets;
	size_t pos = 0;
	for(int i = 0; i < 4; ++i)
	{
		size_t end = str.find('.', pos);
		if(i < 3 && end == std::string_view::npos) 
			return std::nullopt;
		if(i == 3 && end != std::string_view::npos)
			return std::nullopt;
		
		std::string_view subStr = i == 3 ? str.substr(pos) : str.substr(pos, end-pos);
		if(subStr.empty() || subStr.size() > 3)
			return std::nullopt;
		uint16_t val = 0;
		for(char c: subStr)
		{
			if( c < '0' || c > '9')
				return std::nullopt;
			val = val * 10 + (c - '0');
			if(val > 255)
				return std::nullopt;
		}
		octets[i] = val;
	}
	return IPAddress(octets);
}

std::optional<IPAddress> IPAddress::fromString(const char* str)
{
	return str ? fromString(std::string_view(str)) : std::nullopt;
}

std::ostream& operator<<(std::ostream& stream, const IPAddress& ip)
{
	stream << static_cast<int>(ip[0]) << '.'
	<< static_cast<int>(ip[1]) << '.'
	<< static_cast<int>(ip[2]) << '.'
	<< static_cast<int>(ip[3]);
	return stream;
}