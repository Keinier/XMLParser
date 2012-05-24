#ifndef USE_STL

#ifndef STRING_INCLUDED
#define STRING_INCLUDED

#include <assert.h>
#include <string.h>

/*	The support for explicit isn't that universal, and it isn't really
	required - it is used to check that the XMLString class isn't incorrectly
	used. Be nice to old compilers and macro it here:
*/
#if defined(_MSC_VER) && (_MSC_VER >= 1200 )
	// Microsoft visual studio, version 6 and higher.
	#define EXPLICIT explicit
#elif defined(__GNUC__) && (__GNUC__ >= 3 )
	// GCC version 3 and higher.s
	#define EXPLICIT explicit
#else
	#define EXPLICIT
#endif


/*
   XMLString is an emulation of a subset of the std::string template.
   Its purpose is to allow compiling TinyXML on compilers with no or poor STL support.
   Only the member functions relevant to the TinyXML project have been implemented.
   The buffer allocation is made by a simplistic power of 2 like mechanism : if we increase
   a string and there's no more room, we allocate a buffer twice as big as we need.
*/
class XMLString
{
  public :
	// The size type used
  	typedef size_t size_type;

	// Error value for find primitive
	static const size_type npos; // = -1;


	// XMLString empty constructor
	XMLString () : rep_(&nullrep_)
	{
	}

	// XMLString copy constructor
	XMLString ( const XMLString & copy) : rep_(0)
	{
		init(copy.length());
		memcpy(start(), copy.data(), length());
	}

	// XMLString constructor, based on a string
	EXPLICIT XMLString ( const char * copy) : rep_(0)
	{
		init( static_cast<size_type>( strlen(copy) ));
		memcpy(start(), copy, length());
	}

	// XMLString constructor, based on a string
	EXPLICIT XMLString ( const char * str, size_type len) : rep_(0)
	{
		init(len);
		memcpy(start(), str, len);
	}

	// XMLString destructor
	~XMLString ()
	{
		quit();
	}

	XMLString& operator = (const char * copy)
	{
		return assign( copy, (size_type)strlen(copy));
	}

	XMLString& operator = (const XMLString & copy)
	{
		return assign(copy.start(), copy.length());
	}


	// += operator. Maps to append
	XMLString& operator += (const char * suffix)
	{
		return append(suffix, static_cast<size_type>( strlen(suffix) ));
	}

	// += operator. Maps to append
	XMLString& operator += (char single)
	{
		return append(&single, 1);
	}

	// += operator. Maps to append
	XMLString& operator += (const XMLString & suffix)
	{
		return append(suffix.data(), suffix.length());
	}


	// Convert a XMLString into a null-terminated char *
	const char * c_str () const { return rep_->str; }

	// Convert a XMLString into a char * (need not be null terminated).
	const char * data () const { return rep_->str; }

	// Return the length of a XMLString
	size_type length () const { return rep_->size; }

	// Alias for length()
	size_type size () const { return rep_->size; }

	// Checks if a XMLString is empty
	bool empty () const { return rep_->size == 0; }

	// Return capacity of string
	size_type capacity () const { return rep_->capacity; }


	// single char extraction
	const char& at (size_type index) const
	{
		assert( index < length() );
		return rep_->str[ index ];
	}

	// [] operator
	char& operator [] (size_type index) const
	{
		assert( index < length() );
		return rep_->str[ index ];
	}

	// find a char in a string. Return XMLString::npos if not found
	size_type find (char lookup) const
	{
		return find(lookup, 0);
	}

	// find a char in a string from an offset. Return XMLString::npos if not found
	size_type find (char tofind, size_type offset) const
	{
		if (offset >= length()) return npos;

		for (const char* p = c_str() + offset; *p != '\0'; ++p)
		{
		   if (*p == tofind) return static_cast< size_type >( p - c_str() );
		}
		return npos;
	}

	void clear ()
	{
		//Lee:
		//The original was just too strange, though correct:
		//	XMLString().swap(*this);
		//Instead use the quit & re-init:
		quit();
		init(0,0);
	}

	/*	Function to reserve a big amount of data when we know we'll need it. Be aware that this
		function DOES NOT clear the content of the XMLString if any exists.
	*/
	void reserve (size_type cap);

	XMLString& assign (const char* str, size_type len);

	XMLString& append (const char* str, size_type len);

	void swap (XMLString& other)
	{
		Rep* r = rep_;
		rep_ = other.rep_;
		other.rep_ = r;
	}

  private:

	void init(size_type sz) { init(sz, sz); }
	void set_size(size_type sz) { rep_->str[ rep_->size = sz ] = '\0'; }
	char* start() const { return rep_->str; }
	char* finish() const { return rep_->str + rep_->size; }

	struct Rep
	{
		size_type size, capacity;
		char str[1];
	};

	void init(size_type sz, size_type cap)
	{
		if (cap)
		{
			// Lee: the original form:
			//	rep_ = static_cast<Rep*>(operator new(sizeof(Rep) + cap));
			// doesn't work in some cases of new being overloaded. Switching
			// to the normal allocation, although use an 'int' for systems
			// that are overly picky about structure alignment.
			const size_type bytesNeeded = sizeof(Rep) + cap;
			const size_type intsNeeded = ( bytesNeeded + sizeof(int) - 1 ) / sizeof( int ); 
			rep_ = reinterpret_cast<Rep*>( new int[ intsNeeded ] );

			rep_->str[ rep_->size = sz ] = '\0';
			rep_->capacity = cap;
		}
		else
		{
			rep_ = &nullrep_;
		}
	}

	void quit()
	{
		if (rep_ != &nullrep_)
		{
			// The rep_ is really an array of ints. (see the allocator, above).
			// Cast it back before delete, so the compiler won't incorrectly call destructors.
			delete [] ( reinterpret_cast<int*>( rep_ ) );
		}
	}

	Rep * rep_;
	static Rep nullrep_;

} ;


inline bool operator == (const XMLString & a, const XMLString & b)
{
	return    ( a.length() == b.length() )				// optimization on some platforms
	       && ( strcmp(a.c_str(), b.c_str()) == 0 );	// actual compare
}
inline bool operator < (const XMLString & a, const XMLString & b)
{
	return strcmp(a.c_str(), b.c_str()) < 0;
}

inline bool operator != (const XMLString & a, const XMLString & b) { return !(a == b); }
inline bool operator >  (const XMLString & a, const XMLString & b) { return b < a; }
inline bool operator <= (const XMLString & a, const XMLString & b) { return !(b < a); }
inline bool operator >= (const XMLString & a, const XMLString & b) { return !(a < b); }

inline bool operator == (const XMLString & a, const char* b) { return strcmp(a.c_str(), b) == 0; }
inline bool operator == (const char* a, const XMLString & b) { return b == a; }
inline bool operator != (const XMLString & a, const char* b) { return !(a == b); }
inline bool operator != (const char* a, const XMLString & b) { return !(b == a); }

XMLString operator + (const XMLString & a, const XMLString & b);
XMLString operator + (const XMLString & a, const char* b);
XMLString operator + (const char* a, const XMLString & b);


/*
   XMLOutStream is an emulation of std::ostream. It is based on XMLString.
   Only the operators that we need for TinyXML have been developped.
*/
class XMLOutStream : public XMLString
{
public :

	// XMLOutStream << operator.
	XMLOutStream & operator << (const XMLString & in)
	{
		*this += in;
		return *this;
	}

	// XMLOutStream << operator.
	XMLOutStream & operator << (const char * in)
	{
		*this += in;
		return *this;
	}

} ;

#endif	// STRING_INCLUDED
#endif	// USE_STL
