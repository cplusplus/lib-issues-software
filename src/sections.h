#ifndef INCLUDE_LWG_SECTIONS_H
#define INCLUDE_LWG_SECTIONS_H

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace lwg
{

struct section_num;

using section_tag = std::string;
using section_map = std::map<section_tag, section_num>;


struct section_num {
   std::string       prefix;  // initial prefix of section tag, if it denotes a TR/TS, or empty for the primary C++ standard
   std::vector<int>  num;     // sequence of numbers corresponding to section number in relevant doc, e.g,, 17.5.2.1.4.2
};

auto operator <  (section_num const & x, section_num const & y) noexcept -> bool;
   // section 'x' sorts before section 'y' if its 'prefix' field lexicographically precedes that of 'y',
   // and its 'nun' field lexicographically precedes that of 'y' if the prefix fields are equivalent/

auto operator == (section_num const & x, section_num const & y) noexcept -> bool;
auto operator != (section_num const & x, section_num const & y) noexcept -> bool;
   // Two 'section_num' objects compare equal if their 'prefix' and 'num' both compare equal.

auto operator >> (std::istream & is, section_num & sn) -> std::istream &;
auto operator << (std::ostream & os, section_num const & sn) -> std::ostream &;


auto read_section_db(std::istream & stream) -> section_map;
   // Read the current C++ standard tag -> section number index
   // from the specified 'stream', and return it as a new
   // 'section_map' object.

} // close namespace lwg


#endif // INCLUDE_LWG_SECTIONS_H
