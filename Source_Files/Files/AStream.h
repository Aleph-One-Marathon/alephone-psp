/////////////////////////////////////////////////////////////////////////
// $Id: AStream.h,v 1.7 2005/11/12 19:12:45 ghs Exp $
/////////////////////////////////////////////////////////////////////////

/*
 *  AStream.h
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

#ifndef __ASTREAM_H
#define __ASTREAM_H

#include <string>
#include <exception>
#include "cstypes.h"

namespace AStream
{
	enum _Aiostate { _M_aiostate_end = 1L << 16 };
	static const short _S_badbit = 0x01;
	static const short _S_failbit = 0x02;

	typedef _Aiostate iostate;
	static const iostate badbit = iostate(_S_badbit);
	static const iostate failbit = iostate(_S_failbit);
	static const iostate goodbit = iostate(0);

	class failure : public std::exception
	{
		public:
			failure(const std::string& __str) throw();
			failure(const failure &f);
			~failure() throw();
			const char*
			what() const throw();

		private:
			char * _M_name;
	};

	template <typename T>
	class basic_astream
	{
	private:
		T *_M_stream_begin;
		T *_M_stream_end;
		iostate _M_state;
		iostate _M_exception;
	protected:
		T *_M_stream_pos;
		bool
		bound_check(uint32 __delta) throw(AStream::failure);
		
		uint32
		tell_pos() const
		{ return _M_stream_pos - _M_stream_begin; } 

		uint32
		max_pos() const
		{ return _M_stream_end - _M_stream_begin; }

	public:
		iostate
		rdstate() const
		{ return _M_state; }

		void
		setstate(iostate __state)
		{ _M_state= iostate(this->rdstate() | __state); }
		
		bool
		good() const
		{ return this->rdstate() == 0; }
		
		bool 
		fail() const
		{ return (this->rdstate() & (badbit | failbit)) != 0; }

		bool
		bad() const
		{ return (this->rdstate() & badbit) != 0; }
		
		iostate
		exceptions() const
		{ return _M_exception; }
		
		void
		exceptions(iostate except)
		{ _M_exception= except; }
		
		basic_astream(T* __stream, uint32 __length, uint32 __offset) :
			_M_stream_begin(__stream),
			_M_stream_end(__stream + __length),
			_M_state(goodbit),
			_M_exception(failbit),
			_M_stream_pos(__stream + __offset)
		{ if(_M_stream_pos > _M_stream_end) { this->setstate(badbit); } }

		virtual ~basic_astream() {};
	};
}

/* Input Streams, deserializing */

class AIStream : public AStream::basic_astream<const uint8>
{
public:
	AIStream(const uint8* __stream, uint32 __length, uint32 __offset) :
		AStream::basic_astream<const uint8>(__stream, __length, __offset) {}

	uint32
	tellg() const
	{ return this->tell_pos(); }
			
	uint32
	maxg() const
	{ return this->max_pos(); }

	AIStream&
	operator>>(uint8 &__value) throw(AStream::failure);
	
	AIStream&
	operator>>(int8 &__value) throw(AStream::failure);
	
	virtual AIStream&
	  operator>>(bool &__value) throw(AStream::failure);
  
	virtual AIStream&
	operator>>(uint16 &__value) throw(AStream::failure) = 0;
	
	virtual AIStream&
	operator>>(int16 &__value) throw(AStream::failure) = 0;
	
	virtual AIStream&
	operator>>(uint32 &__value) throw(AStream::failure) = 0;
	
	virtual AIStream&
	operator>>(int32 &__value) throw(AStream::failure) = 0;

	AIStream&
	read(char *__ptr, uint32 __count) throw(AStream::failure);
	
	AIStream&
	read(unsigned char * __ptr, uint32 __count) throw(AStream::failure)
	{ return read((char *) __ptr, __count); }
	
	AIStream&
	read(signed char * __ptr, uint32 __count) throw(AStream::failure)
	{ return read((char *) __ptr, __count); }
	
	AIStream&
	ignore(uint32 __count) throw(AStream::failure);

	// Uses >> instead of operator>> so as to pick up friendly operator>>
	template<class T>
	inline AIStream&
	read(T* __list, uint32 __count) throw(AStream::failure)
	{
		T* ValuePtr = __list;
		for (unsigned int k=0; k<__count; k++)
			*this >> *(ValuePtr++);
  
		return *this;
	};
  
};

class AIStreamBE : public AIStream
{
public:
	AIStreamBE(const uint8* __stream, uint32 __length, uint32 __offset = 0) :
		AIStream(__stream, __length, __offset) {};
  
	AIStream&
	operator>>(uint16 &__value) throw(AStream::failure);
	
	AIStream&
	operator>>(int16 &__value) throw(AStream::failure);
	
	AIStream&
	operator>>(uint32 &__value) throw(AStream::failure);
	
	AIStream&
	operator>>(int32 &__value) throw(AStream::failure);
};

class AIStreamLE : public AIStream
{
public:
	AIStreamLE(const uint8* __stream, uint32 __length, uint32 __offset = 0) :
		AIStream(__stream, __length, __offset) {};
  
	AIStream&
	operator>>(uint16 &__value) throw(AStream::failure);
	
	AIStream&
	operator>>(int16 &__value) throw(AStream::failure);
	
	AIStream&
	operator>>(uint32 &__value) throw(AStream::failure);
	
	AIStream&
	operator>>(int32 &__value) throw(AStream::failure);
};

/* Output Streams, serializing */

class AOStream : public AStream::basic_astream<uint8>
{
public:
	AOStream(uint8* __stream, uint32 __length, uint32 __offset) :
		AStream::basic_astream<uint8>(__stream, __length, __offset) {}

	uint32
	tellp() const
	{ return this->tell_pos(); }
			
	uint32
	maxp() const
	{ return this->max_pos(); }
		
	AOStream&
	operator<<(uint8 __value) throw(AStream::failure);
	
	AOStream&
	operator<<(int8 __value) throw(AStream::failure);

	virtual AOStream&
        operator<<(bool __value) throw (AStream::failure);  

	virtual AOStream&
	operator<<(uint16 __value) throw(AStream::failure) = 0;
	
	virtual AOStream&
	operator<<(int16 __value) throw(AStream::failure) = 0;
	
	virtual AOStream&
	operator<<(uint32 __value) throw(AStream::failure) = 0;
	
	virtual AOStream&
	operator<<(int32 __value) throw(AStream::failure) = 0;


	AOStream&
	write(char *__ptr, uint32 __count) throw(AStream::failure);
	
	AOStream&
	write(unsigned char * __ptr, uint32 __count) throw(AStream::failure)
	{ return write((char *) __ptr, __count); }
	
	AOStream&
	write(signed char * __ptr, uint32 __count) throw(AStream::failure)
	{ return write((char *) __ptr, __count); }
	
	AOStream& ignore(uint32 __count) throw(AStream::failure);

	// Uses << instead of operator>> so as to pick up friendly operator<<
	template<class T>
	inline AOStream&
	write(T* __list, uint32 __count) throw(AStream::failure)
	{
		T* ValuePtr = __list;
		for (unsigned int k=0; k<__count; k++)
			*this << *(ValuePtr++);
    
		return *this;
	}
};

class AOStreamBE: public AOStream
{
public:
	AOStreamBE(uint8* __stream, uint32 __length, uint32 __offset = 0) :
		AOStream(__stream, __length, __offset) {};
  
	AOStream&
	operator<<(uint16 __value) throw(AStream::failure);
	
	AOStream&
	operator<<(int16 __value) throw(AStream::failure);
	
	AOStream&
	operator<<(uint32 __value) throw(AStream::failure);
	
	AOStream&
	operator<<(int32 __value) throw(AStream::failure);
};

class AOStreamLE: public AOStream
{
public:
	AOStreamLE(uint8* __stream, uint32 __length, uint32 __offset = 0) :
		AOStream(__stream, __length, __offset) {}
  
	AOStream&
	operator<<(uint16 __value) throw(AStream::failure);
	
	AOStream&
	operator<<(int16 __value) throw(AStream::failure);
	
	AOStream&
	operator<<(uint32 __value) throw(AStream::failure);
	
	AOStream&
	operator<<(int32 __value) throw(AStream::failure);
};

#endif
