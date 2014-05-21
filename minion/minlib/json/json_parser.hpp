#ifndef _GRASPJSON_PARSE_HPP_CAPQLCN
#define _GRASPJSON_PARSE_HPP_CAPQLCN

#include "picojson/picojson.h"
#include "minlib/optional.hpp"
#include "minlib/SimpleMap.hpp"
#include "minlib/optional.hpp"
#include "minlib/immutable_string.hpp"
#include "minlib/string_ops.hpp"

// This allows us to assume missing booleans are false
void inline try_json_fill(bool& ret, const picojson::object& o, const std::string& val)
{
	if(o.count(val))
	{
		const picojson::value& v = mapget(o, val);
		ret = v.get<bool>();
	}
	else
		ret = false;
}

inline std::ostream& json_dump(const bool& b,std::ostream& o)
{ return o << (b ? "true" : "false"); }

inline void json_fill(int& i, const picojson::value& v)
{ i = v.get<double>(); }

inline std::ostream& json_dump(const int& i,std::ostream& o)
{ return o << i; }

inline void json_fill(std::string& s, const picojson::value& v)
{ s = v.get<std::string>(); }

inline std::ostream& json_dump(const std::string& s, std::ostream& o)
{ return o << "\"" << s << "\""; }

inline void json_fill(ImmutableString& s, const picojson::value& v)
{ s = ImmutableString(v.get<std::string>()); }

inline std::ostream& json_dump(const ImmutableString& s, std::ostream& o)
{ return o << "\"" << s.getStdString() << "\""; }

template<typename T>
std::ostream& json_dump(const std::vector<T>& v, std::ostream& o)
{
	o << "[";
	bool first = true;
	for(auto it = v.begin(); it != v.end(); ++it)
	{
		if(first) first=false; else o << ", ";
		json_dump(*it, o);
	}
	return o << "]";
}


template<typename U, typename T>
std::ostream& json_dump(const std::map<U, T>& v, std::ostream& o)
{
	o << "{";
	bool first = true;
	for(auto it = v.begin(); it != v.end(); ++it)
	{
		if(first) first=false; else o << ", ";
		o << "\"" << it->first << "\" : ";
		json_dump(it->second, o);
	}
	return o << "}";
}

template<typename T>
std::ostream& try_json_dump(T& t, std::ostream& o)
{
	return json_dump(t, o);
}


template<typename T>
void json_fill(std::vector<T>& ret, const picojson::value& v)
{
	if(!v.is<picojson::array>())
	{
		std::cerr << "Expected an array, found '" << v << "'\n";
	}
	const picojson::array& a = v.get<picojson::array>();
    for(auto it = a.begin(); it != a.end(); ++it)
 	    {
 	        T val;
 	        json_fill(val, *it);
 	        ret.push_back(val);
 	    }
}

template<typename T, typename U>
void json_fill(std::pair<T,U>& ret, const picojson::value& v)
{
	if(!v.is<picojson::array>())
	{
		std::cerr << "Expected an array, found '" << v << "'\n";
	}
	const picojson::array& a = v.get<picojson::array>();
	if(a.size() != 2)
	{
		std::cerr << "Expected an array of size 2, found '" << v << "'\n";
	}
	
	json_fill(ret.first, a[0]);
	
	json_fill(ret.second, a[1]);
}


template<typename U, typename T>
void json_fill(std::map<U, T>& ret, const picojson::value& v)
{
	if(!v.is<picojson::object>())
	{
		std::cerr << "Need an object to fill a map, found '" << v << "'\n";
	}
	const picojson::object& a = v.get<picojson::object>();
    for(auto it = a.begin(); it != a.end(); ++it)
    {
        T val;
        json_fill(val, it->second);
        ret[static_cast<U>(it->first)] = val;
    }
}

// This allows us to skip a missing vector, and assume it is empty
template<typename T>
void try_json_fill(std::vector<T>& ret, const picojson::object& o, const std::string& val)
{
	if(o.count(val))
	{
		const picojson::value& v = mapget(o, val);
		json_fill(ret, v);
	}
}

template<typename T>
void try_json_fill(T& t, const picojson::object& o, const std::string& val)
{
	json_fill(t, mapget(o, val));
}

// This allows us to skip a missing map, and assume it is empty
template<typename U, typename T>
void try_json_fill(std::map<U, T>& ret, const picojson::object& o, const std::string& val)
{
	if(o.count(val))
	{
		const picojson::value& v = mapget(o, val);
		json_fill(ret, v);
	}
}

// Allows us to have optional members
template<typename T>
void try_json_fill(option<T>& t, const picojson::object& o, const std::string& val)
{
	if(o.count(val))
		json_fill(*t, mapget(o, val));
}


#define FILL_PROP(x) try \
	{ try_json_fill(ret.x, o, #x); o.erase(#x); } \
	catch(const std::runtime_error& er) \
	{ throw std::runtime_error(er.what() + std::string(" while parsing ") + std::string(#x) + " from " + tostring(clone)); }

#define DUMP_PROP(x) \
	if(first) first=false; else o << ","; o << "\"" << #x << "\" :"; try_json_dump(t.x, o); o << "\n";



#define JSON_FILL(TYPE, ...) \
friend void json_fill(TYPE& ret, const picojson::value& v) \
{ \
	if(!v.is<picojson::object>()) throw std::runtime_error("'Not an object'"); \
	picojson::object o = v.get<picojson::object>(); \
	picojson::object clone = o; \
	MINLIB_APPLY(FILL_PROP, MINLIB_EMPTY, __VA_ARGS__); \
	if(!o.empty()) \
		std::cerr << "Warning: Failed to parse:" + tostring(o) << "\n"; \
}\
\
friend std::ostream& json_dump(const TYPE& t, std::ostream& o) \
{ \
	o << "{"; \
	bool first = true; \
	MINLIB_APPLY(DUMP_PROP, MINLIB_EMPTY, __VA_ARGS__); \
	return o << "}\n"; \
} \
friend std::ostream& operator<<(std::ostream& o, const TYPE& t) { return json_dump(t, o); }


inline picojson::value readJSON(std::string s)
{
    s = removeComments(s);
    std::istringstream iss(s);
    picojson::value v;

    iss >> v;

    if(iss.fail()) {
        std::cerr << picojson::get_last_error() << std::endl;
        abort();
    }

    return v;
}

#endif
