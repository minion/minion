/*
* DoMinion - Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef JSON_OUTPUT_CDSJ
#define JSON_OUTPUT_CDSJ

#include "../macros.hpp"
#include "json_parser.hpp"

#include <vector>

#ifndef NO_SYSTEM
#include <mutex>
#endif

struct JSONStreamer
{
	// This is the stack of currently open terms, telling us if they have had a member printed yet.
	std::vector<bool> element_printed;
	std::vector<std::string> element_type;
	
	std::ostream* stream;
	
	bool initial_open;

private:
	void maybe_print_comma()
	{
		if(!element_printed.empty())
		{
			if(element_printed.back())
				(*stream) << ", ";
			else
				element_printed.back() = true;
		}
	}

	void open_term_internal(std::string s)
	{
		element_printed.push_back(false);
		if(s == "{")
			element_type.push_back("}");
		else if(s == "[")
			element_type.push_back("]");
		else
			abort();
		(*stream) << s;
	}
public:

	bool in_map() const {
		return element_type.size() > 0 && element_type.back() == "}";
	}
	
	void open_vec()
	{
		D_ASSERT(!in_map());
		maybe_print_comma();
		open_term_internal("[");
	}

	void open_map()
	{
		D_ASSERT(!in_map());
		maybe_print_comma();
		open_term_internal("{");
	}
	
	template<typename T>
	void open_vec_with_key(const T& t)
	{
		D_ASSERT(in_map());
		if(element_printed.back() == false)
			element_printed.back() = true;
		else
			(*stream) << ", ";
		(*stream) << "\"" << t << "\" : ";
		open_term_internal("[");
	}
	
	template<typename T>
	void open_map_with_key(const T& t)
	{
		D_ASSERT(in_map());
		if(element_printed.back() == false)
			element_printed.back() = true;
		else
			(*stream) << ", ";
		(*stream) << "\"" << t << "\" : ";
		open_term_internal("{");
	}


	void close_term()
	{
		(*stream) << element_type.back();
		if(element_type.back() == "}")
			(*stream) << "\n";
		element_printed.pop_back();
		element_type.pop_back();
		if(!element_printed.empty())
			element_printed.back() = true;
	}

	int depth() {
		return element_printed.size();
	}

	void newline()
	{ (*stream) << "\n"; }

	JSONStreamer() : initial_open(true)
	{
		open_map();
	}
	
public:
	~JSONStreamer()
	{
        while(depth() > 0)
            close_term();
        (*stream) << std::flush;
  }

	template<typename T, typename U>
	void map_element(const T& t, const U& u)
	{
		D_ASSERT(element_type.back() == "}")
		maybe_print_comma();
		(*stream) << "\"" << t << "\" : " << json_dump(u) << " ";
	}

	template<typename T>
	void vec_element(const T& t)
	{
		D_ASSERT(element_type.back() == "]");
		maybe_print_comma();
		(*stream) << json_dump(t);
	}

	
};


#endif
