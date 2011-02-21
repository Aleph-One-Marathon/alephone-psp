/////////////////////////////////////////////////////////////////////////
// $Id: AStream.cpp,v 1.11 2005/11/12 19:12:45 ghs Exp $
/////////////////////////////////////////////////////////////////////////

/*
 *  AStream.cpp
 *  AlephModular
 *
 *	Class to handle serialization issues. This is equivalent to Packing[.h/.cpp]
 *	in AlephOne. And is derived from those files.
 *
 *	Why are we doing this instead of just using Packing[.h/.cpp]?
 *	Because of 2 things. First of all, Packing.h was less clear then it should
 *	have been, the choice between Big Endian and Little Endian was made at the
 *	time the file was included. And the actual elements used from the file don't
 *	specify endian explicitly. Second of all, I wanted the stream elements to be
 *	clearly typed and encapsulated.
 *
 *  Created by Br'fin on Wed Nov 27 2002.
 *
 */

#if !defined(DISABLE_NETWORKING)
 
#include "AStream.h"
#include <string.h>
#include <cstdlib>


using namespace std;

AIStream& AIStream::operator>>(uint8 &value)
	throw(AStream::failure)
{
	if(bound_check(1))
	{
		value = *(_M_stream_pos++);
	}
	return *this;
}

AIStream& AIStream::operator>>(int8 &value)
	throw(AStream::failure)
{
	uint8 UValue;
	operator>>(UValue);
	value = int8(UValue);

	return *this;
}

AIStream& AIStream::operator>>(bool &value)
  throw(AStream::failure)
{
  uint8 UValue;
  operator>>(UValue);
  value = (UValue != 0);

  return *this;
}

AIStream& AIStream::read(char *ptr, uint32 count)
	throw(AStream::failure)
{
	if(bound_check(count))
	{
		memcpy(ptr, _M_stream_pos, count);
		_M_stream_pos += count;
	}
	return *this;
}

AIStream& AIStream::ignore(uint32 count)
	throw(AStream::failure)
{
	if(bound_check(count))
	{
		_M_stream_pos += count;
	}
	return *this;
}

AOStream& AOStream::operator<<(uint8 value)
	throw(AStream::failure)
{
	if(bound_check(1))
	{
		*(_M_stream_pos++) = value;
	}
	return *this;
}

AOStream& AOStream::operator<<(int8 value)
	throw(AStream::failure)
{
	return operator<<(uint8(value));
}

AOStream& AOStream::operator<<(bool value)
  throw(AStream::failure)
{
  return operator<<(uint8(value ? 1 : 0));
}

AOStream& AOStream::write(char *ptr, uint32 count)
	throw(AStream::failure)
{
	if(bound_check(count))
	{
		memcpy(_M_stream_pos, ptr, count);
		_M_stream_pos += count;
	}
	return *this;
}

AOStream& AOStream::ignore(uint32 count)
	throw(AStream::failure)
{
	if(bound_check(count))
	{
		_M_stream_pos += count;
	}
	return *this;
}

//big endian

AIStream& AIStreamBE::operator>>(uint16 &value)
	throw(AStream::failure)
{
	if(bound_check(2))
	{
		// Must be unsigned, so they will be zero-extended
		uint16 Byte0 = uint16(*(_M_stream_pos++));
		uint16 Byte1 = uint16(*(_M_stream_pos++));

		value = (Byte0 << 8) | Byte1;
	}
	return *this;
}

AIStream& AIStreamBE::operator>>(int16 &value)
	throw(AStream::failure)
{
	uint16 UValue;
	operator>>(UValue);
	value = int16(UValue);
  
	return *this;
}

AIStream& AIStreamBE::operator>>(uint32 &value)
	throw(AStream::failure)
{
	if(bound_check(4))
	{
		// Must be unsigned, so they will be zero-extended
		uint32 Byte0 = uint32(*(_M_stream_pos++));
		uint32 Byte1 = uint32(*(_M_stream_pos++));
		uint32 Byte2 = uint32(*(_M_stream_pos++));
		uint32 Byte3 = uint32(*(_M_stream_pos++));
	
		value = (Byte0 << 24) | (Byte1 << 16) | (Byte2 << 8) | Byte3;
	}
	return *this;
}

AIStream& AIStreamBE::operator>>(int32 &value)
	throw(AStream::failure)
{
	uint32 UValue;
	operator>>(UValue);
	value = int32(UValue);
  
	return *this;
}

AOStream& AOStreamBE::operator<<(uint16 value)
	throw(AStream::failure)
{
	if(bound_check(2))
	{
		*(_M_stream_pos++) = uint8(value >> 8);
		*(_M_stream_pos++) = uint8(value);
	}
	return *this;
}

AOStream& AOStreamBE::operator<<(int16 value)
	throw(AStream::failure)
{
	return operator<<(uint16(value));
}

AOStream& AOStreamBE::operator<<(uint32 value)
	throw(AStream::failure)
{
	if(bound_check(4))
	{
		*(_M_stream_pos++) = uint8(value >> 24);
		*(_M_stream_pos++) = uint8(value >> 16);
		*(_M_stream_pos++) = uint8(value >> 8);
		*(_M_stream_pos++) = uint8(value);
	}
	return *this;
}

AOStream& AOStreamBE::operator<<(int32 value)
	throw(AStream::failure)
{
	return operator<<(uint32(value));
}


// little endian

AIStream& AIStreamLE::operator>>(uint16 &value)
	throw(AStream::failure)
{
	if(bound_check(2))
	{
		// Must be unsigned, so they will be zero-extended
		uint16 Byte0 = uint16(*(_M_stream_pos++));
		uint16 Byte1 = uint16(*(_M_stream_pos++));
	
		value = (Byte1 << 8) | Byte0;
	}
	return *this;
}

AIStream& AIStreamLE::operator>>(int16 &value)
	throw(AStream::failure)
{
	uint16 UValue;
	operator>>(UValue);
	value = int16(UValue);
  
	return *this;
}

AIStream& AIStreamLE::operator>>(uint32 &value)
	throw(AStream::failure)
{
	if(bound_check(4))
	{
		// Must be unsigned, so they will be zero-extended
		uint32 Byte0 = uint32(*(_M_stream_pos++));
		uint32 Byte1 = uint32(*(_M_stream_pos++));
		uint32 Byte2 = uint32(*(_M_stream_pos++));
		uint32 Byte3 = uint32(*(_M_stream_pos++));
	
		value = (Byte3 << 24) | (Byte2 << 16) | (Byte1 << 8) | Byte0;
	}
	return *this;
}

AIStream& AIStreamLE::operator>>(int32 &value)
	throw(AStream::failure)
{
	uint32 UValue;
	operator>>(UValue);
	value = int32(UValue);
    
	return *this;
}

AOStream& AOStreamLE::operator<<(uint16 value)
	throw(AStream::failure)
{
	if(bound_check(2))
	{
		*(_M_stream_pos++) = uint8(value);
		*(_M_stream_pos++) = uint8(value >> 8);
	}
	return *this;
}

AOStream& AOStreamLE::operator<<(int16 value)
	throw(AStream::failure)
{
	return operator<<(uint16(value));
}

AOStream& AOStreamLE::operator<<(uint32 value)
	throw(AStream::failure)
{
	if(bound_check(4))
	{
		*(_M_stream_pos++) = uint8(value);
		*(_M_stream_pos++) = uint8(value >> 8);
		*(_M_stream_pos++) = uint8(value >> 16);
		*(_M_stream_pos++) = uint8(value >> 24);
	}
	return *this;
}

AOStream& AOStreamLE::operator<<(int32 value)
	throw(AStream::failure)
{
	return operator<<(uint32(value));
}

template<typename T>
bool AStream::basic_astream<T>::bound_check(uint32 delta)
	throw(AStream::failure)
{
	if(_M_stream_pos + delta > _M_stream_end)
	{
		this->setstate(AStream::failbit);
		if ((this->exceptions() & AStream::failbit) != 0)
		{
			throw AStream::failure("serialization bound check failed");
		}
	}
	return !(this->fail());
}

AStream::failure::failure(const std::string& str) throw()
{
	_M_name = strdup(str.c_str());
}

AStream::failure::failure(const failure &f) {
	if (f._M_name) {
		_M_name = strdup(f._M_name);
	}
}

AStream::failure::~failure() throw() {
	if (_M_name) {
		free(_M_name);
		_M_name = NULL;
	}
}

 const char*	AStream::failure::what() const throw()
 {
	 return _M_name;
 }

#endif // !defined(DISABLE_NETWORKING)

