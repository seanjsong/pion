// -----------------------------------------------------------------
// libpion: a C++ framework for building lightweight HTTP interfaces
// -----------------------------------------------------------------
// Copyright (C) 2007 Atomic Labs, Inc.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef __PION_HTTPTYPES_HEADER__
#define __PION_HTTPTYPES_HEADER__

#include <libpion/PionConfig.hpp>
#include <boost/functional/hash.hpp>
#include <string>

#if defined(PION_HAVE_UNORDERED_MAP)
	#include <unordered_map>
	#define PION_UNORDERED_MULTIMAP std::tr1::unordered_multimap
#elif defined(PION_HAVE_EXT_HASH_MAP)
	#if __GNUC__ >= 3
		#include <ext/hash_map>
		#define PION_UNORDERED_MULTIMAP __gnu_cxx::hash_multimap
	#else
		#include <ext/hash_map>
		#define PION_UNORDERED_MULTIMAP hash_multimap
	#endif
#elif defined(PION_HAVE_HASH_MAP)
	#include <hash_map>
	#define PION_UNORDERED_MULTIMAP hash_multimap
#endif


namespace pion {	// begin namespace pion

///
/// HTTPTypes: common data types used by HTTP
/// 
struct HTTPTypes
{
	
	// generic strings used by HTTP
	static const std::string	STRING_EMPTY;
	static const std::string	STRING_CRLF;
	static const std::string	STRING_HTTP_VERSION;
	static const std::string	HEADER_NAME_VALUE_DELIMINATOR;
	
	// common HTTP header names
	static const std::string	HEADER_HOST;
	static const std::string	HEADER_COOKIE;
	static const std::string	HEADER_SET_COOKIE;
	static const std::string	HEADER_CONNECTION;
	static const std::string	HEADER_CONTENT_TYPE;
	static const std::string	HEADER_CONTENT_LENGTH;

	// common HTTP content types
	static const std::string	CONTENT_TYPE_HTML;
	static const std::string	CONTENT_TYPE_TEXT;
	static const std::string	CONTENT_TYPE_XML;
	static const std::string	CONTENT_TYPE_URLENCODED;
	
	// common HTTP response messages
	static const std::string	RESPONSE_MESSAGE_OK;
	static const std::string	RESPONSE_MESSAGE_NOT_FOUND;
	static const std::string	RESPONSE_MESSAGE_BAD_REQUEST;
	static const std::string	RESPONSE_MESSAGE_SERVER_ERROR;

	// common HTTP response codes
	static const unsigned int	RESPONSE_CODE_OK;
	static const unsigned int	RESPONSE_CODE_NOT_FOUND;
	static const unsigned int	RESPONSE_CODE_BAD_REQUEST;
	static const unsigned int	RESPONSE_CODE_SERVER_ERROR;
	
	/// data type for a dictionary of strings (used for HTTP headers)
	typedef PION_UNORDERED_MULTIMAP<std::string, std::string, boost::hash<std::string> >	StringDictionary;

	/// data type for HTTP headers
	typedef StringDictionary	Headers;
	
	/// data type for HTTP query parameters
	typedef StringDictionary	QueryParams;
	
	/// data type for HTTP cookie parameters
	typedef StringDictionary	CookieParams;
	
	/// escapes URL-encoded strings (a%20value+with%20spaces)
	static std::string url_decode(const std::string& str);
};

}	// end namespace pion

#endif
