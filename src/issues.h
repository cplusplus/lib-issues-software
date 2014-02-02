#ifndef INCLUDE_LWG_ISSUES_H
#define INCLUDE_LWG_ISSUES_H

// standard headers
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

// solution specific headers
#include "date.h"

namespace lwg
{

struct section_num {
   std::string       prefix;
   std::vector<int>  num;

   // implicitly defined special operations are all that we need
   section_num() = default;
   section_num(section_num const &) = default;
   section_num(section_num      &&) = default;
   ~section_num() = default;

   auto operator=(section_num const &) -> section_num & = default;
   auto operator=(section_num      &&) -> section_num & = default;
};

auto operator <  (section_num const & x, section_num const & y) noexcept -> bool;
auto operator == (section_num const & x, section_num const & y) noexcept -> bool;
auto operator != (section_num const & x, section_num const & y) noexcept -> bool;

auto operator >> (std::istream& is, section_num& sn) -> std::istream &;
auto operator << (std::ostream& os, section_num const & sn) -> std::ostream &;


using section_tag = std::string;
using section_map = std::map<section_tag, section_num>;

struct issue {
   int num;
   std::string                stat;
   std::string                title;
   std::vector<section_tag>   tags;
   std::string                submitter;
   gregorian::date            date;
   gregorian::date            mod_date;
   std::set<std::string>      duplicates;
   std::string                text;
   bool                       has_resolution;

   // implicitly defined special operations are all that we need
   issue() = default;
   issue(issue const &) = default;
   issue(issue      &&) = default;
   ~issue() = default;

   auto operator=(issue const &) -> issue & = default;
   auto operator=(issue      &&) -> issue & = default;
};

// this predicate API should probably switch to 'std::experimental::string_view'
auto filename_for_status(std::string stat) -> std::string;

auto is_active(std::string const & stat) -> bool;
auto is_active_not_ready(std::string const & stat) -> bool;
auto is_defect(std::string const & stat) -> bool;
auto is_closed(std::string const & stat) -> bool;
auto is_tentative(std::string const & stat) -> bool;
auto is_not_resolved(std::string const & stat) -> bool;
auto is_votable(std::string stat) -> bool;
auto is_ready(std::string stat) -> bool;

auto make_ref_string(issue const & iss) -> std::string;

// Functions to "normalize" a status string
auto remove_pending(std::string stat) -> std::string;
auto remove_tentatively(std::string stat) -> std::string;
auto remove_qualifier(std::string const & stat) -> std::string;


struct sort_by_num {
    bool operator()(issue const & x, issue const & y) const noexcept   {  return x.num < y.num;   }
    bool operator()(issue const & x, int y)           const noexcept   {  return x.num < y;       }
    bool operator()(int x,           issue const & y) const noexcept   {  return x     < y.num;   }
};

} // close namespace lwg

#endif // INCLUDE_LWG_ISSUES_H
