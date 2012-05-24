#include <ctype.h>

#ifdef USE_STL
#include <sstream>
#include <iostream>
#endif

#include "xmlparser.h"

FILE* XMLFOpen( const char* filename, const char* mode );

bool XMLBase::condenseWhiteSpace = true;

// Microsoft compiler security
FILE* XMLFOpen( const char* filename, const char* mode )
{
	#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
		FILE* fp = 0;
		errno_t err = fopen_s( &fp, filename, mode );
		if ( !err && fp )
			return fp;
		return 0;
	#else
		return fopen( filename, mode );
	#endif
}

void XMLBase::EncodeString( const STRING& str, STRING* outString )
{
	int i=0;

	while( i<(int)str.length() )
	{
		unsigned char c = (unsigned char) str[i];

		if (    c == '&' 
		     && i < ( (int)str.length() - 2 )
			 && str[i+1] == '#'
			 && str[i+2] == 'x' )
		{
			// Hexadecimal character reference.
			// Pass through unchanged.
			// &#xA9;	-- copyright symbol, for example.
			//
			// The -1 is a bug fix from Rob Laveaux. It keeps
			// an overflow from happening if there is no ';'.
			// There are actually 2 ways to exit this loop -
			// while fails (error case) and break (semicolon found).
			// However, there is no mechanism (currently) for
			// this function to return an error.
			while ( i<(int)str.length()-1 )
			{
				outString->append( str.c_str() + i, 1 );
				++i;
				if ( str[i] == ';' )
					break;
			}
		}
		else if ( c == '&' )
		{
			outString->append( entity[0].str, entity[0].strLength );
			++i;
		}
		else if ( c == '<' )
		{
			outString->append( entity[1].str, entity[1].strLength );
			++i;
		}
		else if ( c == '>' )
		{
			outString->append( entity[2].str, entity[2].strLength );
			++i;
		}
		else if ( c == '\"' )
		{
			outString->append( entity[3].str, entity[3].strLength );
			++i;
		}
		else if ( c == '\'' )
		{
			outString->append( entity[4].str, entity[4].strLength );
			++i;
		}
		else if ( c < 32 )
		{
			// Easy pass at non-alpha/numeric/symbol
			// Below 32 is symbolic.
			char buf[ 32 ];
			
			#if defined(SNPRINTF)		
				SNPRINTF( buf, sizeof(buf), "&#x%02X;", (unsigned) ( c & 0xff ) );
			#else
				sprintf( buf, "&#x%02X;", (unsigned) ( c & 0xff ) );
			#endif		

			//*ME:	warning C4267: convert 'size_t' to 'int'
			//*ME:	Int-Cast to make compiler happy ...
			outString->append( buf, (int)strlen( buf ) );
			++i;
		}
		else
		{
			//char realc = (char) c;
			//outString->append( &realc, 1 );
			*outString += (char) c;	// somewhat more efficient function call.
			++i;
		}
	}
}


XMLNode::XMLNode( NodeType _type ) : XMLBase()
{
	parent = 0;
	type = _type;
	firstChild = 0;
	lastChild = 0;
	prev = 0;
	next = 0;
}


XMLNode::~XMLNode()
{
	XMLNode* node = firstChild;
	XMLNode* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	
}


void XMLNode::CopyTo( XMLNode* target ) const
{
	target->SetValue (value.c_str() );
	target->userData = userData; 
	target->location = location;
}


void XMLNode::Clear()
{
	XMLNode* node = firstChild;
	XMLNode* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	

	firstChild = 0;
	lastChild = 0;
}


XMLNode* XMLNode::LinkEndChild( XMLNode* node )
{
	assert( node->parent == 0 || node->parent == this );
	assert( node->GetDocument() == 0 || node->GetDocument() == this->GetDocument() );

	if ( node->Type() == XMLNode::TINYXML_DOCUMENT )
	{
		delete node;
		if ( GetDocument() ) 
			GetDocument()->SetError( ERROR_DOCUMENT_TOP_ONLY, 0, 0, ENCODING_UNKNOWN );
		return 0;
	}

	node->parent = this;

	node->prev = lastChild;
	node->next = 0;

	if ( lastChild )
		lastChild->next = node;
	else
		firstChild = node;			// it was an empty list.

	lastChild = node;
	return node;
}


XMLNode* XMLNode::InsertEndChild( const XMLNode& addThis )
{
	if ( addThis.Type() == XMLNode::TINYXML_DOCUMENT )
	{
		if ( GetDocument() ) 
			GetDocument()->SetError( ERROR_DOCUMENT_TOP_ONLY, 0, 0, ENCODING_UNKNOWN );
		return 0;
	}
	XMLNode* node = addThis.Clone();
	if ( !node )
		return 0;

	return LinkEndChild( node );
}


XMLNode* XMLNode::InsertBeforeChild( XMLNode* beforeThis, const XMLNode& addThis )
{	
	if ( !beforeThis || beforeThis->parent != this ) {
		return 0;
	}
	if ( addThis.Type() == XMLNode::TINYXML_DOCUMENT )
	{
		if ( GetDocument() ) 
			GetDocument()->SetError( ERROR_DOCUMENT_TOP_ONLY, 0, 0, ENCODING_UNKNOWN );
		return 0;
	}

	XMLNode* node = addThis.Clone();
	if ( !node )
		return 0;
	node->parent = this;

	node->next = beforeThis;
	node->prev = beforeThis->prev;
	if ( beforeThis->prev )
	{
		beforeThis->prev->next = node;
	}
	else
	{
		assert( firstChild == beforeThis );
		firstChild = node;
	}
	beforeThis->prev = node;
	return node;
}


XMLNode* XMLNode::InsertAfterChild( XMLNode* afterThis, const XMLNode& addThis )
{
	if ( !afterThis || afterThis->parent != this ) {
		return 0;
	}
	if ( addThis.Type() == XMLNode::TINYXML_DOCUMENT )
	{
		if ( GetDocument() ) 
			GetDocument()->SetError( ERROR_DOCUMENT_TOP_ONLY, 0, 0, ENCODING_UNKNOWN );
		return 0;
	}

	XMLNode* node = addThis.Clone();
	if ( !node )
		return 0;
	node->parent = this;

	node->prev = afterThis;
	node->next = afterThis->next;
	if ( afterThis->next )
	{
		afterThis->next->prev = node;
	}
	else
	{
		assert( lastChild == afterThis );
		lastChild = node;
	}
	afterThis->next = node;
	return node;
}


XMLNode* XMLNode::ReplaceChild( XMLNode* replaceThis, const XMLNode& withThis )
{
	if ( !replaceThis )
		return 0;

	if ( replaceThis->parent != this )
		return 0;

	if ( withThis.ToDocument() ) {
		// A document can never be a child.	Thanks to Noam.
		XMLDocument* document = GetDocument();
		if ( document ) 
			document->SetError( ERROR_DOCUMENT_TOP_ONLY, 0, 0, ENCODING_UNKNOWN );
		return 0;
	}

	XMLNode* node = withThis.Clone();
	if ( !node )
		return 0;

	node->next = replaceThis->next;
	node->prev = replaceThis->prev;

	if ( replaceThis->next )
		replaceThis->next->prev = node;
	else
		lastChild = node;

	if ( replaceThis->prev )
		replaceThis->prev->next = node;
	else
		firstChild = node;

	delete replaceThis;
	node->parent = this;
	return node;
}


bool XMLNode::RemoveChild( XMLNode* removeThis )
{
	if ( !removeThis ) {
		return false;
	}

	if ( removeThis->parent != this )
	{	
		assert( 0 );
		return false;
	}

	if ( removeThis->next )
		removeThis->next->prev = removeThis->prev;
	else
		lastChild = removeThis->prev;

	if ( removeThis->prev )
		removeThis->prev->next = removeThis->next;
	else
		firstChild = removeThis->next;

	delete removeThis;
	return true;
}

const XMLNode* XMLNode::FirstChild( const char * _value ) const
{
	const XMLNode* node;
	for ( node = firstChild; node; node = node->next )
	{
		if ( strcmp( node->Value(), _value ) == 0 )
			return node;
	}
	return 0;
}


const XMLNode* XMLNode::LastChild( const char * _value ) const
{
	const XMLNode* node;
	for ( node = lastChild; node; node = node->prev )
	{
		if ( strcmp( node->Value(), _value ) == 0 )
			return node;
	}
	return 0;
}


const XMLNode* XMLNode::IterateChildren( const XMLNode* previous ) const
{
	if ( !previous )
	{
		return FirstChild();
	}
	else
	{
		assert( previous->parent == this );
		return previous->NextSibling();
	}
}


const XMLNode* XMLNode::IterateChildren( const char * val, const XMLNode* previous ) const
{
	if ( !previous )
	{
		return FirstChild( val );
	}
	else
	{
		assert( previous->parent == this );
		return previous->NextSibling( val );
	}
}


const XMLNode* XMLNode::NextSibling( const char * _value ) const 
{
	const XMLNode* node;
	for ( node = next; node; node = node->next )
	{
		if ( strcmp( node->Value(), _value ) == 0 )
			return node;
	}
	return 0;
}


const XMLNode* XMLNode::PreviousSibling( const char * _value ) const
{
	const XMLNode* node;
	for ( node = prev; node; node = node->prev )
	{
		if ( strcmp( node->Value(), _value ) == 0 )
			return node;
	}
	return 0;
}


void XMLElement::RemoveAttribute( const char * name )
{
    #ifdef USE_STL
	STRING str( name );
	XMLAttribute* node = attributeSet.Find( str );
	#else
	XMLAttribute* node = attributeSet.Find( name );
	#endif
	if ( node )
	{
		attributeSet.Remove( node );
		delete node;
	}
}

const XMLElement* XMLNode::FirstChildElement() const
{
	const XMLNode* node;

	for (	node = FirstChild();
			node;
			node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


const XMLElement* XMLNode::FirstChildElement( const char * _value ) const
{
	const XMLNode* node;

	for (	node = FirstChild( _value );
			node;
			node = node->NextSibling( _value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


const XMLElement* XMLNode::NextSiblingElement() const
{
	const XMLNode* node;

	for (	node = NextSibling();
			node;
			node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


const XMLElement* XMLNode::NextSiblingElement( const char * _value ) const
{
	const XMLNode* node;

	for (	node = NextSibling( _value );
			node;
			node = node->NextSibling( _value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


const XMLDocument* XMLNode::GetDocument() const
{
	const XMLNode* node;

	for( node = this; node; node = node->parent )
	{
		if ( node->ToDocument() )
			return node->ToDocument();
	}
	return 0;
}


XMLElement::XMLElement (const char * _value)
	: XMLNode( XMLNode::TINYXML_ELEMENT )
{
	firstChild = lastChild = 0;
	value = _value;
}


#ifdef USE_STL
XMLElement::XMLElement( const std::string& _value ) 
	: XMLNode( XMLNode::TINYXML_ELEMENT )
{
	firstChild = lastChild = 0;
	value = _value;
}
#endif


XMLElement::XMLElement( const XMLElement& copy)
	: XMLNode( XMLNode::TINYXML_ELEMENT )
{
	firstChild = lastChild = 0;
	copy.CopyTo( this );	
}


XMLElement& XMLElement::operator=( const XMLElement& base )
{
	ClearThis();
	base.CopyTo( this );
	return *this;
}


XMLElement::~XMLElement()
{
	ClearThis();
}


void XMLElement::ClearThis()
{
	Clear();
	while( attributeSet.First() )
	{
		XMLAttribute* node = attributeSet.First();
		attributeSet.Remove( node );
		delete node;
	}
}


const char* XMLElement::Attribute( const char* name ) const
{
	const XMLAttribute* node = attributeSet.Find( name );
	if ( node )
		return node->Value();
	return 0;
}


#ifdef USE_STL
const std::string* XMLElement::Attribute( const std::string& name ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	if ( attrib )
		return &attrib->ValueStr();
	return 0;
}
#endif


const char* XMLElement::Attribute( const char* name, int* i ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	const char* result = 0;

	if ( attrib ) {
		result = attrib->Value();
		if ( i ) {
			attrib->QueryIntValue( i );
		}
	}
	return result;
}


#ifdef USE_STL
const std::string* XMLElement::Attribute( const std::string& name, int* i ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	const std::string* result = 0;

	if ( attrib ) {
		result = &attrib->ValueStr();
		if ( i ) {
			attrib->QueryIntValue( i );
		}
	}
	return result;
}
#endif


const char* XMLElement::Attribute( const char* name, double* d ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	const char* result = 0;

	if ( attrib ) {
		result = attrib->Value();
		if ( d ) {
			attrib->QueryDoubleValue( d );
		}
	}
	return result;
}


#ifdef USE_STL
const std::string* XMLElement::Attribute( const std::string& name, double* d ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	const std::string* result = 0;

	if ( attrib ) {
		result = &attrib->ValueStr();
		if ( d ) {
			attrib->QueryDoubleValue( d );
		}
	}
	return result;
}
#endif


int XMLElement::QueryIntAttribute( const char* name, int* ival ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	if ( !attrib )
		return NO_ATTRIBUTE;
	return attrib->QueryIntValue( ival );
}


int XMLElement::QueryUnsignedAttribute( const char* name, unsigned* value ) const
{
	const XMLAttribute* node = attributeSet.Find( name );
	if ( !node )
		return NO_ATTRIBUTE;

	int ival = 0;
	int result = node->QueryIntValue( &ival );
	*value = (unsigned)ival;
	return result;
}


int XMLElement::QueryBoolAttribute( const char* name, bool* bval ) const
{
	const XMLAttribute* node = attributeSet.Find( name );
	if ( !node )
		return NO_ATTRIBUTE;
	
	int result = WRONG_TYPE;
	if (    StringEqual( node->Value(), "true", true, ENCODING_UNKNOWN ) 
		 || StringEqual( node->Value(), "yes", true, ENCODING_UNKNOWN ) 
		 || StringEqual( node->Value(), "1", true, ENCODING_UNKNOWN ) ) 
	{
		*bval = true;
		result = SUCCESS;
	}
	else if (    StringEqual( node->Value(), "false", true, ENCODING_UNKNOWN ) 
			  || StringEqual( node->Value(), "no", true, ENCODING_UNKNOWN ) 
			  || StringEqual( node->Value(), "0", true, ENCODING_UNKNOWN ) ) 
	{
		*bval = false;
		result = SUCCESS;
	}
	return result;
}



#ifdef USE_STL
int XMLElement::QueryIntAttribute( const std::string& name, int* ival ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	if ( !attrib )
		return NO_ATTRIBUTE;
	return attrib->QueryIntValue( ival );
}
#endif


int XMLElement::QueryDoubleAttribute( const char* name, double* dval ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	if ( !attrib )
		return NO_ATTRIBUTE;
	return attrib->QueryDoubleValue( dval );
}


#ifdef USE_STL
int XMLElement::QueryDoubleAttribute( const std::string& name, double* dval ) const
{
	const XMLAttribute* attrib = attributeSet.Find( name );
	if ( !attrib )
		return NO_ATTRIBUTE;
	return attrib->QueryDoubleValue( dval );
}
#endif


void XMLElement::SetAttribute( const char * name, int val )
{	
	XMLAttribute* attrib = attributeSet.FindOrCreate( name );
	if ( attrib ) {
		attrib->SetIntValue( val );
	}
}


#ifdef USE_STL
void XMLElement::SetAttribute( const std::string& name, int val )
{	
	XMLAttribute* attrib = attributeSet.FindOrCreate( name );
	if ( attrib ) {
		attrib->SetIntValue( val );
	}
}
#endif


void XMLElement::SetDoubleAttribute( const char * name, double val )
{	
	XMLAttribute* attrib = attributeSet.FindOrCreate( name );
	if ( attrib ) {
		attrib->SetDoubleValue( val );
	}
}


#ifdef USE_STL
void XMLElement::SetDoubleAttribute( const std::string& name, double val )
{	
	XMLAttribute* attrib = attributeSet.FindOrCreate( name );
	if ( attrib ) {
		attrib->SetDoubleValue( val );
	}
}
#endif 


void XMLElement::SetAttribute( const char * cname, const char * cvalue )
{
	XMLAttribute* attrib = attributeSet.FindOrCreate( cname );
	if ( attrib ) {
		attrib->SetValue( cvalue );
	}
}


#ifdef USE_STL
void XMLElement::SetAttribute( const std::string& _name, const std::string& _value )
{
	XMLAttribute* attrib = attributeSet.FindOrCreate( _name );
	if ( attrib ) {
		attrib->SetValue( _value );
	}
}
#endif


void XMLElement::Print( FILE* cfile, int depth ) const
{
	int i;
	assert( cfile );
	for ( i=0; i<depth; i++ ) {
		fprintf( cfile, "    " );
	}

	fprintf( cfile, "<%s", value.c_str() );

	const XMLAttribute* attrib;
	for ( attrib = attributeSet.First(); attrib; attrib = attrib->Next() )
	{
		fprintf( cfile, " " );
		attrib->Print( cfile, depth );
	}

	// There are 3 different formatting approaches:
	// 1) An element without children is printed as a <foo /> node
	// 2) An element with only a text child is printed as <foo> text </foo>
	// 3) An element with children is printed on multiple lines.
	XMLNode* node;
	if ( !firstChild )
	{
		fprintf( cfile, " />" );
	}
	else if ( firstChild == lastChild && firstChild->ToText() )
	{
		fprintf( cfile, ">" );
		firstChild->Print( cfile, depth + 1 );
		fprintf( cfile, "</%s>", value.c_str() );
	}
	else
	{
		fprintf( cfile, ">" );

		for ( node = firstChild; node; node=node->NextSibling() )
		{
			if ( !node->ToText() )
			{
				fprintf( cfile, "\n" );
			}
			node->Print( cfile, depth+1 );
		}
		fprintf( cfile, "\n" );
		for( i=0; i<depth; ++i ) {
			fprintf( cfile, "    " );
		}
		fprintf( cfile, "</%s>", value.c_str() );
	}
}


void XMLElement::CopyTo( XMLElement* target ) const
{
	// superclass:
	XMLNode::CopyTo( target );

	// Element class: 
	// Clone the attributes, then clone the children.
	const XMLAttribute* attribute = 0;
	for(	attribute = attributeSet.First();
	attribute;
	attribute = attribute->Next() )
	{
		target->SetAttribute( attribute->Name(), attribute->Value() );
	}

	XMLNode* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		target->LinkEndChild( node->Clone() );
	}
}

bool XMLElement::Accept( XMLVisitor* visitor ) const
{
	if ( visitor->VisitEnter( *this, attributeSet.First() ) ) 
	{
		for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() )
		{
			if ( !node->Accept( visitor ) )
				break;
		}
	}
	return visitor->VisitExit( *this );
}


XMLNode* XMLElement::Clone() const
{
	XMLElement* clone = new XMLElement( Value() );
	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


const char* XMLElement::GetText() const
{
	const XMLNode* child = this->FirstChild();
	if ( child ) {
		const XMLText* childText = child->ToText();
		if ( childText ) {
			return childText->Value();
		}
	}
	return 0;
}


XMLDocument::XMLDocument() : XMLNode( XMLNode::TINYXML_DOCUMENT )
{
	tabsize = 4;
	useMicrosoftBOM = false;
	ClearError();
}

XMLDocument::XMLDocument( const char * documentName ) : XMLNode( XMLNode::TINYXML_DOCUMENT )
{
	tabsize = 4;
	useMicrosoftBOM = false;
	value = documentName;
	ClearError();
}


#ifdef USE_STL
XMLDocument::XMLDocument( const std::string& documentName ) : XMLNode( XMLNode::TINYXML_DOCUMENT )
{
	tabsize = 4;
	useMicrosoftBOM = false;
    value = documentName;
	ClearError();
}
#endif


XMLDocument::XMLDocument( const XMLDocument& copy ) : XMLNode( XMLNode::TINYXML_DOCUMENT )
{
	copy.CopyTo( this );
}


XMLDocument& XMLDocument::operator=( const XMLDocument& copy )
{
	Clear();
	copy.CopyTo( this );
	return *this;
}


bool XMLDocument::LoadFile( XMLEncoding encoding )
{
	return LoadFile( Value(), encoding );
}


bool XMLDocument::SaveFile() const
{
	return SaveFile( Value() );
}

bool XMLDocument::LoadFile( const char* _filename, XMLEncoding encoding )
{
	STRING filename( _filename );
	value = filename;

	// reading in binary mode so that tinyxml can normalize the EOL
	FILE* file = XMLFOpen( value.c_str (), "rb" );	

	if ( file )
	{
		bool result = LoadFile( file, encoding );
		fclose( file );
		return result;
	}
	else
	{
		SetError( ERROR_OPENING_FILE, 0, 0, ENCODING_UNKNOWN );
		return false;
	}
}

bool XMLDocument::LoadFile( FILE* file, XMLEncoding encoding )
{
	if ( !file ) 
	{
		SetError( ERROR_OPENING_FILE, 0, 0, ENCODING_UNKNOWN );
		return false;
	}

	// Delete the existing data:
	Clear();
	location.Clear();

	// Get the file size, so we can pre-allocate the string. HUGE speed impact.
	long length = 0;
	fseek( file, 0, SEEK_END );
	length = ftell( file );
	fseek( file, 0, SEEK_SET );

	// Strange case, but good to handle up front.
	if ( length <= 0 )
	{
		SetError( ERROR_DOCUMENT_EMPTY, 0, 0, ENCODING_UNKNOWN );
		return false;
	}

	// Subtle bug here. TinyXml did use fgets. But from the XML spec:
	// 2.11 End-of-Line Handling
	// <snip>
	// <quote>
	// ...the XML processor MUST behave as if it normalized all line breaks in external 
	// parsed entities (including the document entity) on input, before parsing, by translating 
	// both the two-character sequence #xD #xA and any #xD that is not followed by #xA to 
	// a single #xA character.
	// </quote>
	//
	// It is not clear fgets does that, and certainly isn't clear it works cross platform. 
	// Generally, you expect fgets to translate from the convention of the OS to the c/unix
	// convention, and not work generally.

	/*
	while( fgets( buf, sizeof(buf), file ) )
	{
		data += buf;
	}
	*/

	char* buf = new char[ length+1 ];
	buf[0] = 0;

	if ( fread( buf, length, 1, file ) != 1 ) {
		delete [] buf;
		SetError( ERROR_OPENING_FILE, 0, 0, ENCODING_UNKNOWN );
		return false;
	}

	// Process the buffer in place to normalize new lines. (See comment above.)
	// Copies from the 'p' to 'q' pointer, where p can advance faster if
	// a newline-carriage return is hit.
	//
	// Wikipedia:
	// Systems based on ASCII or a compatible character set use either LF  (Line feed, '\n', 0x0A, 10 in decimal) or 
	// CR (Carriage return, '\r', 0x0D, 13 in decimal) individually, or CR followed by LF (CR+LF, 0x0D 0x0A)...
	//		* LF:    Multics, Unix and Unix-like systems (GNU/Linux, AIX, Xenix, Mac OS X, FreeBSD, etc.), BeOS, Amiga, RISC OS, and others
    //		* CR+LF: DEC RT-11 and most other early non-Unix, non-IBM OSes, CP/M, MP/M, DOS, OS/2, Microsoft Windows, Symbian OS
    //		* CR:    Commodore 8-bit machines, Apple II family, Mac OS up to version 9 and OS-9

	const char* p = buf;	// the read head
	char* q = buf;			// the write head
	const char CR = 0x0d;
	const char LF = 0x0a;

	buf[length] = 0;
	while( *p ) {
		assert( p < (buf+length) );
		assert( q <= (buf+length) );
		assert( q <= p );

		if ( *p == CR ) {
			*q++ = LF;
			p++;
			if ( *p == LF ) {		// check for CR+LF (and skip LF)
				p++;
			}
		}
		else {
			*q++ = *p++;
		}
	}
	assert( q <= (buf+length) );
	*q = 0;

	Parse( buf, 0, encoding );

	delete [] buf;
	return !Error();
}


bool XMLDocument::SaveFile( const char * filename ) const
{
	// The old c stuff lives on...
	FILE* fp = XMLFOpen( filename, "w" );
	if ( fp )
	{
		bool result = SaveFile( fp );
		fclose( fp );
		return result;
	}
	return false;
}


bool XMLDocument::SaveFile( FILE* fp ) const
{
	if ( useMicrosoftBOM ) 
	{
		const unsigned char UTF_LEAD_0 = 0xefU;
		const unsigned char UTF_LEAD_1 = 0xbbU;
		const unsigned char UTF_LEAD_2 = 0xbfU;

		fputc( UTF_LEAD_0, fp );
		fputc( UTF_LEAD_1, fp );
		fputc( UTF_LEAD_2, fp );
	}
	Print( fp, 0 );
	return (ferror(fp) == 0);
}


void XMLDocument::CopyTo( XMLDocument* target ) const
{
	XMLNode::CopyTo( target );

	target->error = error;
	target->errorId = errorId;
	target->errorDesc = errorDesc;
	target->tabsize = tabsize;
	target->errorLocation = errorLocation;
	target->useMicrosoftBOM = useMicrosoftBOM;

	XMLNode* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		target->LinkEndChild( node->Clone() );
	}	
}


XMLNode* XMLDocument::Clone() const
{
	XMLDocument* clone = new XMLDocument();
	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


void XMLDocument::Print( FILE* cfile, int depth ) const
{
	assert( cfile );
	for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() )
	{
		node->Print( cfile, depth );
		fprintf( cfile, "\n" );
	}
}


bool XMLDocument::Accept( XMLVisitor* visitor ) const
{
	if ( visitor->VisitEnter( *this ) )
	{
		for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() )
		{
			if ( !node->Accept( visitor ) )
				break;
		}
	}
	return visitor->VisitExit( *this );
}


const XMLAttribute* XMLAttribute::Next() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( next->value.empty() && next->name.empty() )
		return 0;
	return next;
}

/*
XMLAttribute* XMLAttribute::Next()
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( next->value.empty() && next->name.empty() )
		return 0;
	return next;
}
*/

const XMLAttribute* XMLAttribute::Previous() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( prev->value.empty() && prev->name.empty() )
		return 0;
	return prev;
}

/*
XMLAttribute* XMLAttribute::Previous()
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( prev->value.empty() && prev->name.empty() )
		return 0;
	return prev;
}
*/

void XMLAttribute::Print( FILE* cfile, int /*depth*/, STRING* str ) const
{
	STRING n, v;

	EncodeString( name, &n );
	EncodeString( value, &v );

	if (value.find ('\"') == STRING::npos) {
		if ( cfile ) {
			fprintf (cfile, "%s=\"%s\"", n.c_str(), v.c_str() );
		}
		if ( str ) {
			(*str) += n; (*str) += "=\""; (*str) += v; (*str) += "\"";
		}
	}
	else {
		if ( cfile ) {
			fprintf (cfile, "%s='%s'", n.c_str(), v.c_str() );
		}
		if ( str ) {
			(*str) += n; (*str) += "='"; (*str) += v; (*str) += "'";
		}
	}
}


int XMLAttribute::QueryIntValue( int* ival ) const
{
	if ( SSCANF( value.c_str(), "%d", ival ) == 1 )
		return SUCCESS;
	return WRONG_TYPE;
}

int XMLAttribute::QueryDoubleValue( double* dval ) const
{
	if ( SSCANF( value.c_str(), "%lf", dval ) == 1 )
		return SUCCESS;
	return WRONG_TYPE;
}

void XMLAttribute::SetIntValue( int _value )
{
	char buf [64];
	#if defined(SNPRINTF)		
		SNPRINTF(buf, sizeof(buf), "%d", _value);
	#else
		sprintf (buf, "%d", _value);
	#endif
	SetValue (buf);
}

void XMLAttribute::SetDoubleValue( double _value )
{
	char buf [256];
	#if defined(SNPRINTF)		
		SNPRINTF( buf, sizeof(buf), "%g", _value);
	#else
		sprintf (buf, "%g", _value);
	#endif
	SetValue (buf);
}

int XMLAttribute::IntValue() const
{
	return atoi (value.c_str ());
}

double  XMLAttribute::DoubleValue() const
{
	return atof (value.c_str ());
}


XMLComment::XMLComment( const XMLComment& copy ) : XMLNode( XMLNode::TINYXML_COMMENT )
{
	copy.CopyTo( this );
}


XMLComment& XMLComment::operator=( const XMLComment& base )
{
	Clear();
	base.CopyTo( this );
	return *this;
}


void XMLComment::Print( FILE* cfile, int depth ) const
{
	assert( cfile );
	for ( int i=0; i<depth; i++ )
	{
		fprintf( cfile,  "    " );
	}
	fprintf( cfile, "<!--%s-->", value.c_str() );
}


void XMLComment::CopyTo( XMLComment* target ) const
{
	XMLNode::CopyTo( target );
}


bool XMLComment::Accept( XMLVisitor* visitor ) const
{
	return visitor->Visit( *this );
}


XMLNode* XMLComment::Clone() const
{
	XMLComment* clone = new XMLComment();

	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


void XMLText::Print( FILE* cfile, int depth ) const
{
	assert( cfile );
	if ( cdata )
	{
		int i;
		fprintf( cfile, "\n" );
		for ( i=0; i<depth; i++ ) {
			fprintf( cfile, "    " );
		}
		fprintf( cfile, "<![CDATA[%s]]>\n", value.c_str() );	// unformatted output
	}
	else
	{
		STRING buffer;
		EncodeString( value, &buffer );
		fprintf( cfile, "%s", buffer.c_str() );
	}
}


void XMLText::CopyTo( XMLText* target ) const
{
	XMLNode::CopyTo( target );
	target->cdata = cdata;
}


bool XMLText::Accept( XMLVisitor* visitor ) const
{
	return visitor->Visit( *this );
}


XMLNode* XMLText::Clone() const
{	
	XMLText* clone = 0;
	clone = new XMLText( "" );

	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


XMLDeclaration::XMLDeclaration( const char * _version,
									const char * _encoding,
									const char * _standalone )
	: XMLNode( XMLNode::TINYXML_DECLARATION )
{
	version = _version;
	encoding = _encoding;
	standalone = _standalone;
}


#ifdef USE_STL
XMLDeclaration::XMLDeclaration(	const std::string& _version,
									const std::string& _encoding,
									const std::string& _standalone )
	: XMLNode( XMLNode::TINYXML_DECLARATION )
{
	version = _version;
	encoding = _encoding;
	standalone = _standalone;
}
#endif


XMLDeclaration::XMLDeclaration( const XMLDeclaration& copy )
	: XMLNode( XMLNode::TINYXML_DECLARATION )
{
	copy.CopyTo( this );	
}


XMLDeclaration& XMLDeclaration::operator=( const XMLDeclaration& copy )
{
	Clear();
	copy.CopyTo( this );
	return *this;
}


void XMLDeclaration::Print( FILE* cfile, int /*depth*/, STRING* str ) const
{
	if ( cfile ) fprintf( cfile, "<?xml " );
	if ( str )	 (*str) += "<?xml ";

	if ( !version.empty() ) {
		if ( cfile ) fprintf (cfile, "version=\"%s\" ", version.c_str ());
		if ( str ) { (*str) += "version=\""; (*str) += version; (*str) += "\" "; }
	}
	if ( !encoding.empty() ) {
		if ( cfile ) fprintf (cfile, "encoding=\"%s\" ", encoding.c_str ());
		if ( str ) { (*str) += "encoding=\""; (*str) += encoding; (*str) += "\" "; }
	}
	if ( !standalone.empty() ) {
		if ( cfile ) fprintf (cfile, "standalone=\"%s\" ", standalone.c_str ());
		if ( str ) { (*str) += "standalone=\""; (*str) += standalone; (*str) += "\" "; }
	}
	if ( cfile ) fprintf( cfile, "?>" );
	if ( str )	 (*str) += "?>";
}


void XMLDeclaration::CopyTo( XMLDeclaration* target ) const
{
	XMLNode::CopyTo( target );

	target->version = version;
	target->encoding = encoding;
	target->standalone = standalone;
}


bool XMLDeclaration::Accept( XMLVisitor* visitor ) const
{
	return visitor->Visit( *this );
}


XMLNode* XMLDeclaration::Clone() const
{	
	XMLDeclaration* clone = new XMLDeclaration();

	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


void XMLUnknown::Print( FILE* cfile, int depth ) const
{
	for ( int i=0; i<depth; i++ )
		fprintf( cfile, "    " );
	fprintf( cfile, "<%s>", value.c_str() );
}


void XMLUnknown::CopyTo( XMLUnknown* target ) const
{
	XMLNode::CopyTo( target );
}


bool XMLUnknown::Accept( XMLVisitor* visitor ) const
{
	return visitor->Visit( *this );
}


XMLNode* XMLUnknown::Clone() const
{
	XMLUnknown* clone = new XMLUnknown();

	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


XMLAttributeSet::XMLAttributeSet()
{
	sentinel.next = &sentinel;
	sentinel.prev = &sentinel;
}


XMLAttributeSet::~XMLAttributeSet()
{
	assert( sentinel.next == &sentinel );
	assert( sentinel.prev == &sentinel );
}


void XMLAttributeSet::Add( XMLAttribute* addMe )
{
    #ifdef USE_STL
	assert( !Find( STRING( addMe->Name() ) ) );	// Shouldn't be multiply adding to the set.
	#else
	assert( !Find( addMe->Name() ) );	// Shouldn't be multiply adding to the set.
	#endif

	addMe->next = &sentinel;
	addMe->prev = sentinel.prev;

	sentinel.prev->next = addMe;
	sentinel.prev      = addMe;
}

void XMLAttributeSet::Remove( XMLAttribute* removeMe )
{
	XMLAttribute* node;

	for( node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node == removeMe )
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
			node->next = 0;
			node->prev = 0;
			return;
		}
	}
	assert( 0 );		// we tried to remove a non-linked attribute.
}


#ifdef USE_STL
XMLAttribute* XMLAttributeSet::Find( const std::string& name ) const
{
	for( XMLAttribute* node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node->name == name )
			return node;
	}
	return 0;
}

XMLAttribute* XMLAttributeSet::FindOrCreate( const std::string& _name )
{
	XMLAttribute* attrib = Find( _name );
	if ( !attrib ) {
		attrib = new XMLAttribute();
		Add( attrib );
		attrib->SetName( _name );
	}
	return attrib;
}
#endif


XMLAttribute* XMLAttributeSet::Find( const char* name ) const
{
	for( XMLAttribute* node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( strcmp( node->name.c_str(), name ) == 0 )
			return node;
	}
	return 0;
}


XMLAttribute* XMLAttributeSet::FindOrCreate( const char* _name )
{
	XMLAttribute* attrib = Find( _name );
	if ( !attrib ) {
		attrib = new XMLAttribute();
		Add( attrib );
		attrib->SetName( _name );
	}
	return attrib;
}


#ifdef USE_STL	
std::istream& operator>> (std::istream & in, XMLNode & base)
{
	STRING tag;
	tag.reserve( 8 * 1000 );
	base.StreamIn( &in, &tag );

	base.Parse( tag.c_str(), 0, DEFAULT_ENCODING );
	return in;
}
#endif


#ifdef USE_STL	
std::ostream& operator<< (std::ostream & out, const XMLNode & base)
{
	XMLPrinter printer;
	printer.SetStreamPrinting();
	base.Accept( &printer );
	out << printer.Str();

	return out;
}


std::string& operator<< (std::string& out, const XMLNode& base )
{
	XMLPrinter printer;
	printer.SetStreamPrinting();
	base.Accept( &printer );
	out.append( printer.Str() );

	return out;
}
#endif


XMLHandle XMLHandle::FirstChild() const
{
	if ( node )
	{
		XMLNode* child = node->FirstChild();
		if ( child )
			return XMLHandle( child );
	}
	return XMLHandle( 0 );
}


XMLHandle XMLHandle::FirstChild( const char * value ) const
{
	if ( node )
	{
		XMLNode* child = node->FirstChild( value );
		if ( child )
			return XMLHandle( child );
	}
	return XMLHandle( 0 );
}


XMLHandle XMLHandle::FirstChildElement() const
{
	if ( node )
	{
		XMLElement* child = node->FirstChildElement();
		if ( child )
			return XMLHandle( child );
	}
	return XMLHandle( 0 );
}


XMLHandle XMLHandle::FirstChildElement( const char * value ) const
{
	if ( node )
	{
		XMLElement* child = node->FirstChildElement( value );
		if ( child )
			return XMLHandle( child );
	}
	return XMLHandle( 0 );
}


XMLHandle XMLHandle::Child( int count ) const
{
	if ( node )
	{
		int i;
		XMLNode* child = node->FirstChild();
		for (	i=0;
				child && i<count;
				child = child->NextSibling(), ++i )
		{
			// nothing
		}
		if ( child )
			return XMLHandle( child );
	}
	return XMLHandle( 0 );
}


XMLHandle XMLHandle::Child( const char* value, int count ) const
{
	if ( node )
	{
		int i;
		XMLNode* child = node->FirstChild( value );
		for (	i=0;
				child && i<count;
				child = child->NextSibling( value ), ++i )
		{
			// nothing
		}
		if ( child )
			return XMLHandle( child );
	}
	return XMLHandle( 0 );
}


XMLHandle XMLHandle::ChildElement( int count ) const
{
	if ( node )
	{
		int i;
		XMLElement* child = node->FirstChildElement();
		for (	i=0;
				child && i<count;
				child = child->NextSiblingElement(), ++i )
		{
			// nothing
		}
		if ( child )
			return XMLHandle( child );
	}
	return XMLHandle( 0 );
}


XMLHandle XMLHandle::ChildElement( const char* value, int count ) const
{
	if ( node )
	{
		int i;
		XMLElement* child = node->FirstChildElement( value );
		for (	i=0;
				child && i<count;
				child = child->NextSiblingElement( value ), ++i )
		{
			// nothing
		}
		if ( child )
			return XMLHandle( child );
	}
	return XMLHandle( 0 );
}


bool XMLPrinter::VisitEnter( const XMLDocument& )
{
	return true;
}

bool XMLPrinter::VisitExit( const XMLDocument& )
{
	return true;
}

bool XMLPrinter::VisitEnter( const XMLElement& element, const XMLAttribute* firstAttribute )
{
	DoIndent();
	buffer += "<";
	buffer += element.Value();

	for( const XMLAttribute* attrib = firstAttribute; attrib; attrib = attrib->Next() )
	{
		buffer += " ";
		attrib->Print( 0, 0, &buffer );
	}

	if ( !element.FirstChild() ) 
	{
		buffer += " />";
		DoLineBreak();
	}
	else 
	{
		buffer += ">";
		if (    element.FirstChild()->ToText()
			  && element.LastChild() == element.FirstChild()
			  && element.FirstChild()->ToText()->CDATA() == false )
		{
			simpleTextPrint = true;
			// no DoLineBreak()!
		}
		else
		{
			DoLineBreak();
		}
	}
	++depth;	
	return true;
}


bool XMLPrinter::VisitExit( const XMLElement& element )
{
	--depth;
	if ( !element.FirstChild() ) 
	{
		// nothing.
	}
	else 
	{
		if ( simpleTextPrint )
		{
			simpleTextPrint = false;
		}
		else
		{
			DoIndent();
		}
		buffer += "</";
		buffer += element.Value();
		buffer += ">";
		DoLineBreak();
	}
	return true;
}


bool XMLPrinter::Visit( const XMLText& text )
{
	if ( text.CDATA() )
	{
		DoIndent();
		buffer += "<![CDATA[";
		buffer += text.Value();
		buffer += "]]>";
		DoLineBreak();
	}
	else if ( simpleTextPrint )
	{
		STRING str;
		XMLBase::EncodeString( text.ValueTStr(), &str );
		buffer += str;
	}
	else
	{
		DoIndent();
		STRING str;
		XMLBase::EncodeString( text.ValueTStr(), &str );
		buffer += str;
		DoLineBreak();
	}
	return true;
}


bool XMLPrinter::Visit( const XMLDeclaration& declaration )
{
	DoIndent();
	declaration.Print( 0, 0, &buffer );
	DoLineBreak();
	return true;
}


bool XMLPrinter::Visit( const XMLComment& comment )
{
	DoIndent();
	buffer += "<!--";
	buffer += comment.Value();
	buffer += "-->";
	DoLineBreak();
	return true;
}


bool XMLPrinter::Visit( const XMLUnknown& unknown )
{
	DoIndent();
	buffer += "<";
	buffer += unknown.Value();
	buffer += ">";
	DoLineBreak();
	return true;
}

