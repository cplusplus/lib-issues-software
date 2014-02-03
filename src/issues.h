#ifndef INCLUDE_LWG_ISSUES_H
#define INCLUDE_LWG_ISSUES_H

// standard headers
//#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

// solution specific headers
#include "date.h"

namespace lwg
{

using section_tag = std::string;

struct issue {
   int num;
   std::string                stat;
   std::string                title;
   std::vector<section_tag>   tags;
   std::string                submitter;
   gregorian::date            date;
   gregorian::date            mod_date;  // this no longer appears useful
   std::set<std::string>      duplicates;
   std::string                text;
   bool                       has_resolution;
};


// this predicate API should probably switch to 'std::experimental::string_view'
auto filename_for_status(std::string stat) -> std::string;

auto get_status_priority(std::string const & stat) noexcept -> std::ptrdiff_t;

auto is_active(std::string const & stat) -> bool;
auto is_active_not_ready(std::string const & stat) -> bool;
auto is_defect(std::string const & stat) -> bool;
auto is_closed(std::string const & stat) -> bool;
auto is_tentative(std::string const & stat) -> bool;
auto is_not_resolved(std::string const & stat) -> bool;
auto is_votable(std::string stat) -> bool;
auto is_ready(std::string stat) -> bool;

auto make_ref_string(issue const & iss) -> std::string;

struct section_num;

using section_tag = std::string;
using section_map = std::map<section_tag, section_num>;

auto parse_issue_from_file(std::string const & filename, lwg::section_map & section_db) -> issue;
  // Seems appropriate constructor behavior
  // Note that 'section_db' is modifiable as new (unkonwn) sections may be inserted,
  // typically for issues reported against older documents with sections that have
  // since been removed, replaced or merged.

auto read_file_into_string(std::string const & filename) -> std::string;  // Really belongs in a deeper utility

// Functions to "normalize" a status string
// Might profitable switch to 'experimental/string_view'
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
