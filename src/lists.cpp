// This program reads all the issues in the issues directory passed as the first command line argument.
// If all documents are successfully parsed, it will generate the standard LWG Issues List documents
// for an ISO SC22 WG21 mailing.

// Based on code originally donated by Howard Hinnant
// Since modified by Alisdair Meredith

// Note that this program requires a reasonably conformant C++0x compiler, supporting at least:
//    auto
//    lambda expressions
//    brace-initialization
//    range-based for loops
//    raw string literals
//    constexpr
//    new function syntax (late specified return type)
//    noexcept
//    new type-alias syntax (using my_name = type)

// Likewise, the following C++11 library facilities are used:
//    to_string

// Its coding style assumes a standard library optimized with move-semantics
// The only known compiler to support all of this today is the experimental gcc trunk (4.6)

// TODO
// .  Better handling of TR "sections", and grouping of issues in "Clause X"
// .  Sort the Revision comments in the same order as the 'Status' reports, rather than alphabetically
// .  Lots of tidy and cleanup after merging the revision-generating tool
// .  Refactor more common text
// .  Split 'format' function and usage to that the issues vector can pass by const-ref in the common cases
// .  Document the purpose amd contract on each function
// Waiting on external fix for preserving file-dates
// .  sort-by-last-modified-date should offer some filter or separation to see only the issues modified since  the last meeting

// Missing standard facilities that we work around
// . Date
// . Filesystem navigation

// Missing standard library facilities that would probably not change this program
// . XML parser

// standard headers
#include <algorithm>
#include <cassert>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

// platform headers - requires a Posix compatible platform
// The hope is to replace all of this with the filesystem TS
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// solution specific headers
#include "date.h"
#include "issues.h"
#include "mailing_info.h"
#include "report_generator.h"


#if 0
// Revisit this after AJM works out linker issues on his Mac
#if 1
// workaround until <experimental/filesystem> is widely available:
##include <boost/filesystem.hpp>
namespace std {
namespace experimental {
namespace filesystem {
using namespace boost::filesystem;
}
}
}
#else
#include <experimental/filesystem>
#endif
#endif

#if 0
using lwg::issue;
using lwg::section_map;
using lwg::section_num;
using lwg::section_tag;
using lwg::sort_by_num;
#else
using namespace lwg;
#endif

// Issue-list specific functionality for the rest of this file
// ===========================================================


struct equal_issue_num {
    bool operator()(issue const & x, int y) const noexcept {return x.num == y;}
    bool operator()(int x, issue const & y) const noexcept {return x == y.num;}
};


#ifdef DEBUG_SUPPORT
void display(std::vector<issue> const & issues) {
    for (auto const & i : issues) { std::cout << i; }
}
#endif


auto read_section_db(std::string const & path) -> section_map {
   auto filename = path + "section.data";
   std::ifstream infile{filename.c_str()};
   if (!infile.is_open()) {
      throw std::runtime_error{"Can't open section.data at " + path};
   }
   std::cout << "Reading section-tag index from: " << filename << std::endl;

   section_map section_db;
   while (infile) {
      ws(infile);
      std::string line;
      getline(infile, line);
      if (!line.empty()) {
         assert(line.back() == ']');
         auto p = line.rfind('[');
         assert(p != std::string::npos);
         section_tag tag = line.substr(p);
         assert(tag.size() > 2);
         assert(tag[0] == '[');
         assert(tag[tag.size()-1] == ']');
         line.erase(p-1);

         section_num num;
         if (tag.find("[trdec.") != std::string::npos) {
            num.prefix = "TRDecimal";
            line.erase(0, 10);
         }
         else if (tag.find("[tr.") != std::string::npos) {
            num.prefix = "TR1";
            line.erase(0, 4);
         }

         std::istringstream temp(line);
         if (!std::isdigit(line[0])) {
            char c;
            temp >> c;
            num.num.push_back(100 + c - 'A');
            temp >> c;
         }

         while (temp) {
            int n;
            temp >> n;
            if (!temp.fail()) {
               num.num.push_back(n);
               char dot;
               temp >> dot;
            }
         }

         section_db[tag] = num;
      }
   }
   return section_db;
}


// Is this simply a debugging aid?
//void check_against_index(section_map const & section_db) {
//   for (auto const & elem : section_db ) {
//      std::string temp = elem.first;
//      temp.erase(temp.end()-1);
//      temp.erase(temp.begin());
//      std::cout << temp << ' ' << elem.second << '\n';
//   }
//}

//void synchronize_dupicates(std::vector<issue> & issues) {
//   for( auto & iss : issues ) {
//      if(iss.duplicates.empty()) {
//         continue;
//      }
//
//      // This self-reference looks worth preserving, as can be used to substitute in revision history etc.
//      std::string self_ref = make_ref_string(iss);
//
//      for( auto const & dup : iss.duplicates ) {
//         n->duplicates.insert(self_ref); // This is only reason vector is not const
//         is.duplicates.insert(make_ref_string(*n));
//         r.clear();
//      }
//   }
//}


void format(std::vector<issue> & issues, issue & is, section_map & section_db) {
   std::string & s = is.text;
   int issue_num = is.num;
   std::vector<std::string> tag_stack;
   std::ostringstream er;
   // cannot rewrite as range-based for-loop as the string 's' is modified within the loop
    for (std::string::size_type i{0}; i < s.size(); ++i) {
      if (s[i] == '<') {
         auto j = s.find('>', i);
         if (j == std::string::npos) {
            er.clear();
            er.str("");
            er << "missing '>' in issue " << issue_num;
            throw std::runtime_error{er.str()};
         }

         std::string tag;
         {
            std::istringstream iword{s.substr(i+1, j-i-1)};
            iword >> tag;
         }

         if (tag.empty()) {
             er.clear();
             er.str("");
             er << "unexpected <> in issue " << issue_num;
             throw std::runtime_error{er.str()};
         }

         if (tag[0] == '/') { // closing tag
             tag.erase(tag.begin());
             if (tag == "issue"  or  tag == "revision") {
                s.erase(i, j-i + 1);
                --i;
                return;
             }

             if (tag_stack.empty()  or  tag != tag_stack.back()) {
                er.clear();
                er.str("");
                er << "mismatched tags in issue " << issue_num;
                if (tag_stack.empty()) {
                   er << ".  Had no open tag.";
                }
                else {
                   er << ".  Open tag was " << tag_stack.back() << ".";
                }
                er << "  Closing tag was " << tag;
                throw std::runtime_error{er.str()};
             }

             tag_stack.pop_back();
             if (tag == "discussion") {
                 s.erase(i, j-i + 1);
                 --i;
             }
             else if (tag == "resolution") {
                 s.erase(i, j-i + 1);
                 --i;
             }
             else if (tag == "rationale") {
                 s.erase(i, j-i + 1);
                 --i;
             }
             else if (tag == "duplicate") {
                 s.erase(i, j-i + 1);
                 --i;
             }
             else if (tag == "note") {
                 s.replace(i, j-i + 1, "]</i></p>\n");
                 i += 9;
             }
             else {
                 i = j;
             }

             continue;
         }

         if (s[j-1] == '/') { // self-contained tag: sref, iref
            if (tag == "sref") {
               static const
               auto report_missing_quote = [](std::ostringstream & er, unsigned num) {
                  er.clear();
                  er.str("");
                  er << "missing '\"' in sref in issue " << num;
                  throw std::runtime_error{er.str()};
               };

               std::string r;
               auto k = s.find('\"', i+5);
               if (k >= j) {
                  report_missing_quote(er, issue_num);
               }

               auto l = s.find('\"', k+1);
               if (l >= j) {
                  report_missing_quote(er, issue_num);
               }

               ++k;
               r = s.substr(k, l-k);
               {
                  std::ostringstream t;
                  t << section_db[r] << ' ';
                  r.insert(0, t.str());
               }

               j -= i - 1;
               s.replace(i, j, r);
               i += r.size() - 1;
               continue;
            }
            else if (tag == "iref") {
               static const
               auto report_missing_quote = [](std::ostringstream & er, unsigned num) {
                  er.clear();
                  er.str("");
                  er << "missing '\"' in iref in issue " << num;
                  throw std::runtime_error{er.str()};
               };

               auto k = s.find('\"', i+5);
               if (k >= j) {
                  report_missing_quote(er, issue_num);
               }
               auto l = s.find('\"', k+1);
               if (l >= j) {
                  report_missing_quote(er, issue_num);
               }

               ++k;
               std::string r{s.substr(k, l-k)};
               std::istringstream temp{r};
               int num;
               temp >> num;
               if (temp.fail()) {
                  er.clear();
                  er.str("");
                  er << "bad number in iref in issue " << issue_num;
                  throw std::runtime_error{er.str()};
               }

               auto n = std::lower_bound(issues.begin(), issues.end(), num, sort_by_num{});
               if (n->num != num) {
                  er.clear();
                  er.str("");
                  er << "couldn't find number " << num << " in iref in issue " << issue_num;
                  throw std::runtime_error{er.str()};
               }

               if (!tag_stack.empty()  and  tag_stack.back() == "duplicate") {
                  n->duplicates.insert(make_ref_string(is)); // This is only reason vector is not const
                  is.duplicates.insert(make_ref_string(*n));
                  r.clear();
               }
               else {
                  r = make_ref_string(*n);
               }

               j -= i - 1;
               s.replace(i, j, r);
               i += r.size() - 1;
               continue;
            }
            i = j;
            continue;  // don't worry about this <tag/>
         }

         tag_stack.push_back(tag);
         if (tag == "discussion") {
             s.replace(i, j-i + 1, "<p><b>Discussion:</b></p>");
             i += 24;
         }
         else if (tag == "resolution") {
             s.replace(i, j-i + 1, "<p><b>Proposed resolution:</b></p>");
             i += 33;
         }
         else if (tag == "rationale") {
             s.replace(i, j-i + 1, "<p><b>Rationale:</b></p>");
             i += 23;
         }
         else if (tag == "duplicate") {
             s.erase(i, j-i + 1);
             --i;
         }
         else if (tag == "note") {
             s.replace(i, j-i + 1, "<p><i>[");
             i += 6;
         }
         else if (tag == "!--") {
             tag_stack.pop_back();
             j = s.find("-->", i);
             j += 3;
             s.erase(i, j-i);
             --i;
         }
         else {
             i = j;
         }
      }
   }
}


auto read_issues(std::string const & issues_path, section_map & section_db) -> std::vector<issue> {
   std::cout << "Reading issues from: " << issues_path << std::endl;

   DIR* dir = opendir(issues_path.c_str());
   if (dir == 0) {
      throw std::runtime_error{"Unable to open issues dir"};
   }

   std::vector<issue> issues{};
   while ( dirent* entry = readdir(dir) ) {
      std::string const issue_file{ entry->d_name };
      if (0 == issue_file.find("issue") ) {
         issues.emplace_back(parse_issue_from_file(issues_path + issue_file, section_db));
      }
   }
   closedir(dir);
   return issues;
}


void prepare_issues(std::vector<issue> & issues, section_map & section_db) {
   // Initially sort the issues by issue number, so each issue can be correctly 'format'ted
   sort(issues.begin(), issues.end(), sort_by_num{});

   // Then we format the issues, which should be the last time we need to touch the issues themselves
   // We may turn this into a two-stage process, analysing duplicates and then applying the links
   // This will allow us to better express constness when the issues are used purely for reference.
   // Currently, the 'format' function takes a reference-to-non-const-vector-of-issues purely to
   // mark up information related to duplicates, so processing duplicates in a separate pass may
   // clarify the code.
   for( auto & i : issues ) { format(issues, i, section_db); }

   // Issues will be routinely re-sorted in later code, but contents should be fixed after formatting.
   // This suggests we may want to be storing some kind of issue handle in the functions that keep
   // re-sorting issues, and so minimize the churn on the larger objects.
}


// ============================================================================================================


auto prepare_issues_for_diff_report(std::vector<issue> const & issues) -> std::vector<std::tuple<int, std::string> > {
   std::vector<std::tuple<int, std::string> > result;
   std::transform( issues.begin(), issues.end(), back_inserter(result),
                   [](issue const & iss) {
                      return std::make_tuple(iss.num, iss.stat);
                   }
                 );
   return result;
}

auto read_issues_from_toc(std::string const & s) -> std::vector<std::tuple<int, std::string> > {
   // parse all issues from the specified stream, 'is'.
   // Throws 'runtime_error' if *any* parse step fails
   //
   // We assume 'is' refers to a "toc" html document, for either the current or a previous issues list.
   // The TOC file consists of a sequence of HTML <tr> elements - each element is one issue/row in the table
   //    First we read the whole stream into a single 'string'
   //    Then we search the string for the first <tr> marker
   //       The first row is the title row and does not contain an issue.
   //       If cannt find the first row, we flag an error and exit
   //    Next we loop through the string, searching for <tr> markers to indicate the start of each issue
   //       We parse the issue number and status from each row, and append a record to the result vector
   //       If any parse fails, throw a runtime_error
   //    If debugging, display the results to 'cout'

   // Skip the title row
   auto i = s.find("<tr>");
   if (std::string::npos == i) {
      throw std::runtime_error{"Unable to find the first (title) row"};
   }

   // Read all issues in table
   std::vector<std::tuple<int, std::string> > issues;
   for(;;) {
      i = s.find("<tr>", i+4);
      if (i == std::string::npos) {
         break;
      }
      i = s.find("</a>", i);
      auto j = s.rfind('>', i);
      if (j == std::string::npos) {
         throw std::runtime_error{"unable to parse issue number: can't find beginning bracket"};
      }
      std::istringstream instr{s.substr(j+1, i-j-1)};
      int num;
      instr >> num;
      if (instr.fail()) {
         throw std::runtime_error{"unable to parse issue number"};
      }
      i = s.find("</a>", i+4);
      if (i == std::string::npos) {
         throw std::runtime_error{"partial issue found"};
      }
      j = s.rfind('>', i);
      if (j == std::string::npos) {
         throw std::runtime_error{"unable to parse issue status: can't find beginning bracket"};
      }
      issues.emplace_back(num, s.substr(j+1, i-j-1));
   }

   //display_issues(issues);
   return issues;
}


struct list_issues {
   std::vector<int> const & issues;
};


auto operator<<( std::ostream & out, list_issues const & x) -> std::ostream & {
   auto list_separator = "";
   for (auto number : x.issues ) {
      out << list_separator << "<iref ref=\"" << number << "\"/>";
      list_separator = ", ";
   }
   return out;
}


struct find_num {
   // Predidate functor useful to find issue 'y' in a mapping of issue-number -> some string.
   constexpr bool operator()(std::tuple<int, std::string> const & x, int y) const noexcept {
      return std::get<0>(x) < y;
   }
};


struct discover_new_issues {
   std::vector<std::tuple<int, std::string> > const & old_issues;
   std::vector<std::tuple<int, std::string> > const & new_issues;
};


auto operator<<( std::ostream & out, discover_new_issues const & x) -> std::ostream & {
   std::vector<std::tuple<int, std::string> > const & old_issues = x.old_issues;
   std::vector<std::tuple<int, std::string> > const & new_issues = x.new_issues;

   struct status_order {
      using status_string = std::string;
      auto operator()(status_string const & x, status_string const & y) const noexcept -> bool {
         return { get_status_priority(x) < get_status_priority(y) };
      }
   };

   std::map<std::string, std::vector<int>, status_order> added_issues;
   for (auto const & i : new_issues ) {
      auto j = std::lower_bound(old_issues.cbegin(), old_issues.cend(), std::get<0>(i), find_num{});
      if(j == old_issues.end()) {
         added_issues[std::get<1>(i)].push_back(std::get<0>(i));
      }
   }

   for (auto const & i : added_issues ) {
      auto const item_count = std::get<1>(i).size();
      if(1 == item_count) {
         out << "<li>Added the following " << std::get<0>(i) << " issue: <iref ref=\"" << std::get<1>(i).front() << "\"/>.</li>\n";
      }
      else {
         out << "<li>Added the following " << item_count << " " << std::get<0>(i) << " issues: " << list_issues{std::get<1>(i)} << ".</li>\n";
      }
   }
   
   if (added_issues.empty()) {
      out << "<li>No issues added.</li>\n";
   }

   return out;
}


struct discover_changed_issues {
   std::vector<std::tuple<int, std::string> > const & old_issues;
   std::vector<std::tuple<int, std::string> > const & new_issues;
};


auto operator<<( std::ostream & out, discover_changed_issues x) -> std::ostream & {
   std::vector<std::tuple<int, std::string> > const & old_issues = x.old_issues;
   std::vector<std::tuple<int, std::string> > const & new_issues = x.new_issues;

   struct status_transition_order {
      using status_string = std::string;
      using from_status_to_status = std::tuple<status_string, status_string>;

      auto operator()(from_status_to_status const & x, from_status_to_status const & y) const noexcept -> bool {
         auto const xp2 = get_status_priority(std::get<1>(x));
         auto const yp2 = get_status_priority(std::get<1>(y));
         return xp2 < yp2  or  (!(yp2 < xp2)  and  get_status_priority(std::get<0>(x)) < get_status_priority(std::get<0>(y)));
      }
   };

   std::map<std::tuple<std::string, std::string>, std::vector<int>, status_transition_order> changed_issues;
   for (auto const & i : new_issues ) {
      auto j = std::lower_bound(old_issues.begin(), old_issues.end(), std::get<0>(i), find_num{});
      if (j != old_issues.end()  and  std::get<0>(i) == std::get<0>(*j)  and  std::get<1>(*j) != std::get<1>(i)) {
         changed_issues[std::tuple<std::string, std::string>{std::get<1>(*j), std::get<1>(i)}].push_back(std::get<0>(i));
      }
   }

   for (auto const & i : changed_issues ) {
      auto const item_count = std::get<1>(i).size();
      if(1 == item_count) {
         out << "<li>Changed the following issue to " << std::get<1>(std::get<0>(i))
             << " (from " << std::get<0>(std::get<0>(i)) << "): <iref ref=\"" << std::get<1>(i).front() << "\"/>.</li>\n";
      }
      else {
         out << "<li>Changed the following " << item_count << " issues to " << std::get<1>(std::get<0>(i))
             << " (from " << std::get<0>(std::get<0>(i)) << "): " << list_issues{std::get<1>(i)} << ".</li>\n";
      }
   }

   if (changed_issues.empty()) {
      out << "<li>No issues changed.</li>\n";
   }

   return out;
}


void count_issues(std::vector<std::tuple<int, std::string> > const & issues, unsigned & n_open, unsigned & n_closed) {
   n_open = 0;
   n_closed = 0;

   for(auto const & elem : issues) {
      if (is_active(std::get<1>(elem))) {
         ++n_open;
      }
      else {
         ++n_closed;
      }
   }
}


struct write_summary {
   std::vector<std::tuple<int, std::string> > const & old_issues;
   std::vector<std::tuple<int, std::string> > const & new_issues;
};


auto operator<<( std::ostream & out, write_summary const & x) -> std::ostream & {
   std::vector<std::tuple<int, std::string> > const & old_issues = x.old_issues;
   std::vector<std::tuple<int, std::string> > const & new_issues = x.new_issues;

   unsigned n_open_new = 0;
   unsigned n_open_old = 0;
   unsigned n_closed_new = 0;
   unsigned n_closed_old = 0;
   count_issues(old_issues, n_open_old, n_closed_old);
   count_issues(new_issues, n_open_new, n_closed_new);

   out << "<li>" << n_open_new << " open issues, ";
   if (n_open_new >= n_open_old) {
      out << "up by " << n_open_new - n_open_old;
   }
   else {
      out << "down by " << n_open_old - n_open_new;
   }
   out << ".</li>\n";

   out << "<li>" << n_closed_new << " closed issues, ";
   if (n_closed_new >= n_closed_old) {
      out << "up by " << n_closed_new - n_closed_old;
   }
   else {
      out << "down by " << n_closed_old - n_closed_new;
   }
   out << ".</li>\n";

   unsigned n_total_new = n_open_new + n_closed_new;
   unsigned n_total_old = n_open_old + n_closed_old;
   out << "<li>" << n_total_new << " issues total, ";
   if (n_total_new >= n_total_old) {
      out << "up by " << n_total_new - n_total_old;
   }
   else {
      out << "down by " << n_total_old - n_total_new;
   }
   out << ".</li>\n";

   return out;
}


void print_current_revisions( std::ostream & out
                            , std::vector<std::tuple<int, std::string> > const & old_issues
                            , std::vector<std::tuple<int, std::string> > const & new_issues
                            ) {
   out << "<ul>\n"
          "<li><b>Summary:</b><ul>\n"
       << write_summary{old_issues, new_issues}
       << "</ul></li>\n"
          "<li><b>Details:</b><ul>\n"
       << discover_new_issues{old_issues, new_issues}
       << discover_changed_issues{old_issues, new_issues}
       << "</ul></li>\n"
          "</ul>\n";
}

void check_directory(std::string const & directory) {
  struct stat sb;
  if (stat(directory.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode))
    throw std::runtime_error(directory + " is no existing directory");
}

// ============================================================================================================


int main(int argc, char* argv[]) {
   try {
      std::string path;
      std::cout << "Preparing new LWG issues lists..." << std::endl;
      if (argc == 2) {
         path = argv[1];
      }
      else {
         char cwd[1024];
         if (getcwd(cwd, sizeof(cwd)) == 0) {
            std::cout << "unable to getcwd\n";
            return 1;
         }
         path = cwd;
      }

      if (path.back() != '/') { path += '/'; }
      check_directory(path);
	  
      const std::string target_path{path + "mailing/"};
      check_directory(target_path);
	  

      section_map section_db = read_section_db(path + "meta-data/");
      //    check_against_index(section_db);

      auto const old_issues = read_issues_from_toc(read_file_into_string(path + "meta-data/lwg-toc.old.html"));

      auto const issues_path = path + "xml/";
      LwgIssuesXml lwg_issues_xml(issues_path);

      auto issues = read_issues(issues_path, section_db);
      prepare_issues(issues, section_db);

      // issues must be sorted by number before making the mailing list documents
      //sort(issues.begin(), issues.end(), sort_by_num{});

      // Collect a report on all issues that have changed status
      // This will be added to the revision history of the 3 standard documents
      auto const new_issues = prepare_issues_for_diff_report(issues);

      std::ostringstream os_diff_report;
      print_current_revisions(os_diff_report, old_issues, new_issues );
      auto const diff_report = os_diff_report.str();

      std::vector<issue> unresolved_issues;
      std::vector<issue> votable_issues;

      std::copy_if(issues.begin(), issues.end(), std::back_inserter(unresolved_issues), [](issue const & iss){ return is_not_resolved(iss.stat); } );
      std::copy_if(issues.begin(), issues.end(), std::back_inserter(votable_issues),    [](issue const & iss){ return is_votable(iss.stat); } );

      // If votable list is empty, we are between meetings and should list Ready issues instead
      // Otherwise, issues moved to Ready during a meeting will remain 'unresolved' by that meeting
      auto ready_inserter = votable_issues.empty()
                          ? std::back_inserter(votable_issues)
                          : std::back_inserter(unresolved_issues);
      std::copy_if(issues.begin(), issues.end(), ready_inserter, [](issue const & iss){ return is_ready(iss.stat); } );

      // First generate the primary 3 standard issues lists
      make_active(issues, target_path, lwg_issues_xml, section_db, diff_report);
      make_defect(issues, target_path, lwg_issues_xml, section_db, diff_report);
      make_closed(issues, target_path, lwg_issues_xml, section_db, diff_report);

      // unofficial documents
      make_tentative (issues, target_path, lwg_issues_xml, section_db);
      make_unresolved(issues, target_path, lwg_issues_xml, section_db);
      make_immediate (issues, target_path, lwg_issues_xml, section_db);


      // Now we have a parsed and formatted set of issues, we can write the standard set of HTML documents
      // Note that each of these functions is going to re-sort the 'issues' vector for its own purposes
      make_sort_by_num            (issues, {target_path + "lwg-toc.html"},         lwg_issues_xml, section_db);
      make_sort_by_status         (issues, {target_path + "lwg-status.html"},      lwg_issues_xml, section_db);
      make_sort_by_status_mod_date(issues, {target_path + "lwg-status-date.html"}, lwg_issues_xml, section_db);
      make_sort_by_section        (issues, {target_path + "lwg-index.html"},       lwg_issues_xml, section_db);

      // Note that this additional document is very similar to unresolved-index.html below
      make_sort_by_section        (issues, {target_path + "lwg-index-open.html"},  lwg_issues_xml, section_db, true);

      // Make a similar set of index documents for the issues that are 'live' during a meeting
      // Note that these documents want to reference each other, rather than lwg- equivalents,
      // although it may not be worth attempting fix-ups as the per-issue level
      // During meetings, it would be good to list newly-Ready issues here
      make_sort_by_num            (unresolved_issues, {target_path + "unresolved-toc.html"},         lwg_issues_xml, section_db);
      make_sort_by_status         (unresolved_issues, {target_path + "unresolved-status.html"},      lwg_issues_xml, section_db);
      make_sort_by_status_mod_date(unresolved_issues, {target_path + "unresolved-status-date.html"}, lwg_issues_xml, section_db);
      make_sort_by_section        (unresolved_issues, {target_path + "unresolved-index.html"},       lwg_issues_xml, section_db);

      // Make another set of index documents for the issues that are up for a vote during a meeting
      // Note that these documents want to reference each other, rather than lwg- equivalents,
      // although it may not be worth attempting fix-ups as the per-issue level
      // Between meetings, it would be good to list Ready issues here
      make_sort_by_num            (votable_issues, {target_path + "votable-toc.html"},         lwg_issues_xml, section_db);
      make_sort_by_status         (votable_issues, {target_path + "votable-status.html"},      lwg_issues_xml, section_db);
      make_sort_by_status_mod_date(votable_issues, {target_path + "votable-status-date.html"}, lwg_issues_xml, section_db);
      make_sort_by_section        (votable_issues, {target_path + "votable-index.html"},       lwg_issues_xml, section_db);

      std::cout << "Made all documents\n";
   }
   catch(std::exception const & ex) {
      std::cout << ex.what() << std::endl;
      return 1;
   }
}
