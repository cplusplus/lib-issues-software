#ifndef INCLUDE_LWG_ISSUES_H
#define INCLUDE_LWG_ISSUES_H

// standard headers
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
   int                        num;            // ID - issue number
   std::string                stat;           // current status of the issue
   std::string                title;          // descriptive title for the issue
   std::vector<section_tag>   tags;           // section(s) of the standard affected by the issue
   std::string                submitter;      // original submitter of the issue
   gregorian::date            date;           // date the issue was filed
   gregorian::date            mod_date;       // this no longer appears useful
   std::set<std::string>      duplicates;     // sorted list of duplicate issues, stored as html anchor references.
   std::string                text;           // text representing the issue
   int                        priority = 99;  // severity, 1 = critical, 4 = minor concern, 0 = trivial to resolve, 99 = not yet prioritised
   std::string                owner;          // person identified as taking ownership of drafting/progressing the issue
   bool                       has_resolution; // 'true' if 'text' contains a proposed resolution
};

struct order_by_issue_number {
    bool operator()(issue const & x, issue const & y) const noexcept   {  return x.num < y.num;   }
    bool operator()(issue const & x, int y)           const noexcept   {  return x.num < y;       }
    bool operator()(int x,           issue const & y) const noexcept   {  return x     < y.num;   }
};


struct section_num;

using section_tag = std::string;
using section_map = std::map<section_tag, section_num>;

auto parse_issue_from_file(std::string file_contents, std::string const & filename, lwg::section_map & section_db) -> issue;
  // Seems appropriate constructor behavior.
  //
  // Note that 'section_db' is modifiable as new (unkonwn) sections may be inserted,
  // typically for issues reported against older documents with sections that have
  // since been removed, replaced or merged.
  //
  // The filename is passed only to improve diagnostics.


// status string utilities - should probably factor into yet another file.

auto filename_for_status(std::string stat) -> std::string;

auto get_status_priority(std::string const & stat) noexcept -> std::ptrdiff_t;

// this predicate API should probably switch to 'std::experimental::string_view'
auto is_active(std::string const & stat) -> bool;
auto is_active_not_ready(std::string const & stat) -> bool;
auto is_defect(std::string const & stat) -> bool;
auto is_closed(std::string const & stat) -> bool;
auto is_tentative(std::string const & stat) -> bool;
auto is_not_resolved(std::string const & stat) -> bool;
auto is_votable(std::string stat) -> bool;
auto is_ready(std::string stat) -> bool;

// Functions to "normalize" a status string
// Might profitable switch to 'experimental/string_view'
auto remove_pending(std::string stat) -> std::string;
auto remove_tentatively(std::string stat) -> std::string;
auto remove_qualifier(std::string const & stat) -> std::string;


} // close namespace lwg

#endif // INCLUDE_LWG_ISSUES_H
