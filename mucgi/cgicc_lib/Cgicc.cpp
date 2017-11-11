/* -*-mode:c++; c-file-style: "gnu";-*- */
/*
 *  $Id: Cgicc.cpp,v 1.28 2009/01/03 17:12:07 sebdiaz Exp $
 *
 *  Copyright (C) 1996 - 2004 Stephen F. Booth <sbooth@gnu.org>
 *                       2007 Sebastien DIAZ <sebastien.diaz@gmail.com>
 *  Part of the GNU cgicc library, http://www.gnu.org/software/cgicc
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA 
 */

#ifdef __GNUG__
#  pragma implementation
#endif

#include <new>
#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>

#include "cgicc_lib/CgiUtils.h"
#include "cgicc_lib/Cgicc.h"

namespace cgicc {

  // ============================================================
  // Class FE_nameCompare
  // ============================================================
  class FE_nameCompare : public std::unary_function<FormEntry, bool>
  {
  public:
    
    inline explicit FE_nameCompare(const std::string& name)
      : fName(name) {}
    
    inline bool operator() (const FormEntry& entry) 	const
    { return stringsAreEqual(fName, entry.getName()); }
    
  private:
    std::string fName;
  };
  
  // ============================================================
  // Class FE_valueCompare
  // ============================================================
  class FE_valueCompare : public std::unary_function<FormEntry, bool>
  {
  public:
    
    inline explicit FE_valueCompare(const std::string& value)
      : fValue(value) {}
    
    inline bool operator() (const FormEntry& entry) 	const
    { return stringsAreEqual(fValue, entry.getValue()); }
    
  private:
    std::string fValue;
  };
  
  
  // ============================================================
  // Class FF_compare
  // ============================================================
  class FF_compare : public std::unary_function<FormFile, bool>
  {
  public:
    
    inline explicit FF_compare(const std::string& name)
      : fName(name) {}
    
    inline bool operator() (const FormFile& entry) 	const
    { return stringsAreEqual(fName, entry.getName()); }
    
  private:
    std::string fName;
  };
  
  // ============================================================
  // Function copy_if (handy, missing from STL)
  // ============================================================
  // This code taken directly from 
  // "The C++ Programming Language, Third Edition" by Bjarne Stroustrup
/*  template<class In, class Out, class Pred>
  Out 
  copy_if(In first, 
	  In last, 
	  Out res, 
	  Pred p)
  {
    while(first != last) {
      if(p(*first))
	*res++ = *first;
      ++first;
    }
    return res;
  }
*/
} // namespace cgicc

// ============================================================
// Class MultipartHeader
// ============================================================
class cgicc::MultipartHeader 
{
public:
  
  MultipartHeader(const std::string& disposition,
		  const std::string& name,
		  const std::string& filename,
		  const std::string& cType);
  
  inline
  MultipartHeader(const MultipartHeader& head)
  { operator=(head); }
  ~MultipartHeader();

  MultipartHeader&
  operator= (const MultipartHeader& head);
  
  inline std::string 
  getContentDisposition() 				const
  { return fContentDisposition; }
  
  inline std::string
  getName() 						const
  { return fName; }

  inline std::string 
  getFilename() 					const
  { return fFilename; }

  inline std::string 
  getContentType() 					const
  { return fContentType; }

private:
  std::string fContentDisposition;
  std::string fName;
  std::string fFilename;
  std::string fContentType;
};

cgicc::MultipartHeader::MultipartHeader(const std::string& disposition,
					const std::string& name,
					const std::string& filename,
					const std::string& cType)
  : fContentDisposition(disposition),
    fName(name),
    fFilename(filename),
    fContentType(cType)
{}

cgicc::MultipartHeader::~MultipartHeader()
{}

cgicc::MultipartHeader&
cgicc::MultipartHeader::operator= (const MultipartHeader& head)
{
  fContentDisposition 	= head.fContentDisposition;
  fName 		= head.fName;
  fFilename 		= head.fFilename;
  fContentType 		= head.fContentType;

  return *this;
}

// ============================================================
// Class Cgicc
// ============================================================
cgicc::Cgicc::Cgicc(std::string &query, std::string &cookie, std::string &data, std::string &type)
    :fQueryString(query), fCookie(cookie), fPostData(data), fQueryType(type)
{ 
  // this can be tweaked for performance
  fFormData.reserve(20);
  fFormFiles.reserve(2);
  fCookies.reserve(10);

  parseFormInput(fPostData, fQueryType);
  parseFormInput(fQueryString);
  parseCookies(fCookie);
}

cgicc::Cgicc::~Cgicc()
{}

cgicc::Cgicc& 
cgicc::Cgicc::operator= (const Cgicc& cgi)
{
  fFormData.clear();
  fFormFiles.clear();

  parseFormInput(fPostData, fQueryType);
  parseFormInput(fQueryString);
  
  return *this;
}

const char*
cgicc::Cgicc::getCompileDate() 					const
{ return __DATE__; }

const char*
cgicc::Cgicc::getCompileTime() 					const
{ return __TIME__; }

bool 
cgicc::Cgicc::queryCheckbox(const std::string& elementName) 	const
{
  const_form_iterator iter = getElement(elementName);
  return (iter != fFormData.end() && stringsAreEqual(iter->getValue(), "on"));
}

std::string
cgicc::Cgicc::operator() (const std::string& name) 		const
{
  std::string result;
  const_form_iterator iter = getElement(name);
  if(iter != fFormData.end() && false == iter->isEmpty())
    result = iter->getValue();
  return result;
}

cgicc::form_iterator 
cgicc::Cgicc::getElement(const std::string& name)
{
  return std::find_if(fFormData.begin(), fFormData.end(),FE_nameCompare(name));
}

cgicc::const_form_iterator 
cgicc::Cgicc::getElement(const std::string& name) 		const
{
  return std::find_if(fFormData.begin(), fFormData.end(),FE_nameCompare(name));
}

bool 
cgicc::Cgicc::getElement(const std::string& name, 
			 std::vector<FormEntry>& result) 	const
{ 
  return findEntries(name, true, result); 
}

cgicc::form_iterator 
cgicc::Cgicc::getElementByValue(const std::string& value)
{
  return std::find_if(fFormData.begin(), fFormData.end(),
		      FE_valueCompare(value));
}

cgicc::const_form_iterator 
cgicc::Cgicc::getElementByValue(const std::string& value) 	const
{
  return std::find_if(fFormData.begin(), fFormData.end(), 
		      FE_valueCompare(value));
}

bool 
cgicc::Cgicc::getElementByValue(const std::string& value, 
				std::vector<FormEntry>& result)	const
{ 
  return findEntries(value, false, result); 
}

cgicc::file_iterator 
cgicc::Cgicc::getFile(const std::string& name)
{
  return std::find_if(fFormFiles.begin(), fFormFiles.end(), FF_compare(name));
}

cgicc::const_file_iterator 
cgicc::Cgicc::getFile(const std::string& name) 			const
{
  return std::find_if(fFormFiles.begin(), fFormFiles.end(), FF_compare(name));
}


// implementation method
bool
cgicc::Cgicc::findEntries(const std::string& param, 
			  bool byName,
			  std::vector<FormEntry>& result) 	const
{
  // empty the target vector
  result.clear();

  if(byName) {
    copy_if(fFormData.begin(), fFormData.end(), 
	    std::back_inserter(result),FE_nameCompare(param));
  }
  else {
    copy_if(fFormData.begin(), fFormData.end(), 
	    std::back_inserter(result), FE_valueCompare(param));
  }

  return false == result.empty();
}

void
cgicc::Cgicc::parseFormInput(const std::string& data, const std::string &content_type)
{
  
  std::string standard_type	= "application/x-www-form-urlencoded";
  std::string multipart_type 	= "multipart/form-data";

  // Don't waste time on empty input
  if(true == data.empty())
    return;

  // Standard content type = application/x-www-form-urlencoded
  // It may not be explicitly specified
  if(true == content_type.empty() 
     || stringsAreEqual(content_type, standard_type,standard_type.length())) {
    std::string name, value;
    std::string::size_type pos;
    std::string::size_type oldPos = 0;

    // Parse the data in one fell swoop for efficiency
    while(true) {
      // Find the '=' separating the name from its value
      pos = data.find_first_of('=', oldPos);
      
      // If no '=', we're finished
      if(std::string::npos == pos)
	break;
      
      // Decode the name
      name = form_urldecode(data.substr(oldPos, pos - oldPos));
      oldPos = ++pos;
      
      // Find the '&' separating subsequent name/value pairs
      pos = data.find_first_of('&', oldPos);
      
      // Even if an '&' wasn't found the rest of the string is a value
      value = form_urldecode(data.substr(oldPos, pos - oldPos));

      // Store the pair
      fFormData.push_back(FormEntry(name, value));
      
      if(std::string::npos == pos)
	break;

      // Update parse position
      oldPos = ++pos;
    }
  }
  // File upload type = multipart/form-data
  else if(stringsAreEqual(multipart_type, content_type,
			  multipart_type.length())){

    // Find out what the separator is
    std::string 		bType 	= "boundary=";
    std::string::size_type 	pos 	= content_type.find(bType);

    // generate the separators
    std::string sep1 = content_type.substr(pos + bType.length());
    sep1.append("\r\n");
    sep1.insert(0, "--");

    std::string sep2 = content_type.substr(pos + bType.length());
    sep2.append("--\r\n");
    sep2.insert(0, "--");

    // Find the data between the separators
    std::string::size_type start  = data.find(sep1);
    std::string::size_type sepLen = sep1.length();
    std::string::size_type oldPos = start + sepLen;

    while(true) {
      pos = data.find(sep1, oldPos);

      // If sep1 wasn't found, the rest of the data is an item
      if(std::string::npos == pos)
	break;

      // parse the data
      parseMIME(data.substr(oldPos, pos - oldPos));

      // update position
      oldPos = pos + sepLen;
    }

    // The data is terminated by sep2
    pos = data.find(sep2, oldPos);
    // parse the data, if found
    if(std::string::npos != pos) {
      parseMIME(data.substr(oldPos, pos - oldPos));
    }
  }
}

cgicc::MultipartHeader
cgicc::Cgicc::parseHeader(const std::string& data)
{
  std::string disposition;
  disposition = extractBetween(data, "Content-Disposition: ", ";");
  
  std::string name;
  name = extractBetween(data, "name=\"", "\"");
  
  std::string filename;
  filename = extractBetween(data, "filename=\"", "\"");

  std::string cType;
  cType = extractBetween(data, "Content-Type: ", "\r\n\r\n");

  // This is hairy: Netscape and IE don't encode the filenames
  // The RFC says they should be encoded, so I will assume they are.
  filename = form_urldecode(filename);

  return MultipartHeader(disposition, name, filename, cType);
}

void
cgicc::Cgicc::parseMIME(const std::string& data)
{
  // Find the header
  std::string end = "\r\n\r\n";
  std::string::size_type headLimit = data.find(end, 0);
  
  // Detect error
  if(std::string::npos == headLimit)
    throw std::runtime_error("Malformed input");

  // Extract the value - there is still a trailing CR/LF to be subtracted off
  std::string::size_type valueStart = headLimit + end.length();
  std::string value = data.substr(valueStart, data.length() - valueStart - 2);

  // Parse the header - pass trailing CR/LF x 2 to parseHeader
  MultipartHeader head = parseHeader(data.substr(0, valueStart));

  if(head.getFilename().empty())
    fFormData.push_back(FormEntry(head.getName(), value));
  else
    fFormFiles.push_back(FormFile(head.getName(), 
				  head.getFilename(), 
				  head.getContentType(), 
				  value));
}

void
cgicc::Cgicc::parseCookies(std::string &data)
{
  if(false == data.empty()) {
    std::string::size_type pos;
    std::string::size_type oldPos = 0;

    while(true) {
      // find the ';' terminating a name=value pair
      pos = data.find(";", oldPos);

      // if no ';' was found, the rest of the string is a single cookie
      if(std::string::npos == pos) {
	parseCookie(data.substr(oldPos));
	return;
      }

      // otherwise, the string contains multiple cookies
      // extract it and add the cookie to the list
      parseCookie(data.substr(oldPos, pos - oldPos));
      
      // update pos (+1 to skip ';')
      oldPos = pos + 1;
    }
  }
}

void
cgicc::Cgicc::parseCookie(const std::string& data)
{
  // find the '=' separating the name and value
  std::string::size_type pos = data.find("=", 0);

  // if no '=' was found, return
  if(std::string::npos == pos)
    return;

  // skip leading whitespace - " \f\n\r\t\v"
  std::string::size_type wscount = 0;
  std::string::const_iterator data_iter;
  
  for(data_iter = data.begin(); data_iter != data.end(); ++data_iter,++wscount)
    if(0 == std::isspace(*data_iter))
      break;			
  
  // Per RFC 2091, do not unescape the data (thanks to afm@othello.ch)
  std::string name 	= data.substr(wscount, pos - wscount);
  std::string value 	= data.substr(++pos);

  fCookies.push_back(HTTPCookie(name, value));
}

