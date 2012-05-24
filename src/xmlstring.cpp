#ifndef USE_STL

#include "xmlstring.h"

// Error value for find primitive
const XMLString::size_type XMLString::npos = static_cast< XMLString::size_type >(-1);


// Null rep.
XMLString::Rep XMLString::nullrep_ = { 0, 0, { '\0' } };


void XMLString::reserve (size_type cap)
{
	if (cap > capacity())
	{
		XMLString tmp;
		tmp.init(length(), cap);
		memcpy(tmp.start(), data(), length());
		swap(tmp);
	}
}


XMLString& XMLString::assign(const char* str, size_type len)
{
	size_type cap = capacity();
	if (len > cap || cap > 3*(len + 8))
	{
		XMLString tmp;
		tmp.init(len);
		memcpy(tmp.start(), str, len);
		swap(tmp);
	}
	else
	{
		memmove(start(), str, len);
		set_size(len);
	}
	return *this;
}


XMLString& XMLString::append(const char* str, size_type len)
{
	size_type newsize = length() + len;
	if (newsize > capacity())
	{
		reserve (newsize + capacity());
	}
	memmove(finish(), str, len);
	set_size(newsize);
	return *this;
}


XMLString operator + (const XMLString & a, const XMLString & b)
{
	XMLString tmp;
	tmp.reserve(a.length() + b.length());
	tmp += a;
	tmp += b;
	return tmp;
}

XMLString operator + (const XMLString & a, const char* b)
{
	XMLString tmp;
	XMLString::size_type b_len = static_cast<XMLString::size_type>( strlen(b) );
	tmp.reserve(a.length() + b_len);
	tmp += a;
	tmp.append(b, b_len);
	return tmp;
}

XMLString operator + (const char* a, const XMLString & b)
{
	XMLString tmp;
	XMLString::size_type a_len = static_cast<XMLString::size_type>( strlen(a) );
	tmp.reserve(a_len + b.length());
	tmp.append(a, a_len);
	tmp += b;
	return tmp;
}


#endif	// USE_STL
