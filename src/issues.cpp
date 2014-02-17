#include "issues.h"

#include "sections.h"

#include <algorithm>
#include <cassert>
#include <istream>
#include <ostream>
#include <string>
#include <stdexcept>

#include <sstream>

#include <ctime>
#include <sys/stat.h>  // plan to factor this dependency out

namespace {
static constexpr char const * LWG_ACTIVE {"lwg-active.html" };
static constexpr char const * LWG_CLOSED {"lwg-closed.html" };
static constexpr char const * LWG_DEFECTS{"lwg-defects.html"};

// date utilites may factor out again
auto parse_month(std::string const & m) -> gregorian::month {
   // This could be turned into an efficient map lookup with a suitable indexed container
   return (m == "Jan") ? gregorian::jan
        : (m == "Feb") ? gregorian::feb
        : (m == "Mar") ? gregorian::mar
        : (m == "Apr") ? gregorian::apr
        : (m == "May") ? gregorian::may
        : (m == "Jun") ? gregorian::jun
        : (m == "Jul") ? gregorian::jul
        : (m == "Aug") ? gregorian::aug
        : (m == "Sep") ? gregorian::sep
        : (m == "Oct") ? gregorian::oct
        : (m == "Nov") ? gregorian::nov
        : (m == "Dec") ? gregorian::dec
        : throw std::runtime_error{"unknown month " + m};
}

auto parse_date(std::istream & temp) -> gregorian::date {
   int d;
   temp >> d;
   if (temp.fail()) {
      throw std::runtime_error{"date format error"};
   }

   std::string month;
   temp >> month;

   auto m = parse_month(month);
   int y{ 0 };
   temp >> y;
   return m/gregorian::day{d}/y;
}

auto make_date(std::tm const & mod) -> gregorian::date {
   return gregorian::year((unsigned short)(mod.tm_year+1900)) / (mod.tm_mon+1) / mod.tm_mday;
}

auto report_date_file_last_modified(std::string const & filename) -> gregorian::date {
   struct stat buf;
   if (stat(filename.c_str(), &buf) == -1) {
      throw std::runtime_error{"call to stat failed for " + filename};
   }

   return make_date(*std::localtime(&buf.st_mtime));
}

} // close unnamed namespace

// functions to relate the status of an issue to its relevant published list document
auto lwg::filename_for_status(std::string stat) -> std::string {
   // Tentative issues are always active
   if(0 == stat.find("Tentatively")) {
      return LWG_ACTIVE;
   }

   stat = remove_qualifier(stat);
   return (stat == "TC1")           ? LWG_DEFECTS
        : (stat == "CD1")           ? LWG_DEFECTS
        : (stat == "C++11")         ? LWG_DEFECTS
        : (stat == "C++14")         ? LWG_DEFECTS
        : (stat == "WP")            ? LWG_DEFECTS
        : (stat == "Resolved")      ? LWG_DEFECTS
        : (stat == "DR")            ? LWG_DEFECTS
        : (stat == "TRDec")         ? LWG_DEFECTS
        : (stat == "Dup")           ? LWG_CLOSED
        : (stat == "NAD")           ? LWG_CLOSED
        : (stat == "NAD Future")    ? LWG_CLOSED
        : (stat == "NAD Editorial") ? LWG_CLOSED
        : (stat == "NAD Concepts")  ? LWG_CLOSED
        : (stat == "Voting")        ? LWG_ACTIVE
        : (stat == "Immediate")     ? LWG_ACTIVE
        : (stat == "Ready")         ? LWG_ACTIVE
        : (stat == "Review")        ? LWG_ACTIVE
        : (stat == "New")           ? LWG_ACTIVE
        : (stat == "Open")          ? LWG_ACTIVE
        : (stat == "EWG")           ? LWG_ACTIVE
        : (stat == "LEWG")          ? LWG_ACTIVE
        : (stat == "Core")          ? LWG_ACTIVE
        : (stat == "Deferred")      ? LWG_ACTIVE
        : throw std::runtime_error("unknown status " + stat);
}

auto lwg::is_active(std::string const & stat) -> bool {
   return filename_for_status(stat) == LWG_ACTIVE;
}

auto lwg::is_active_not_ready(std::string const & stat) -> bool {
   return is_active(stat)  and  stat != "Ready";
}

auto lwg::is_defect(std::string const & stat) -> bool {
   return filename_for_status(stat) == LWG_DEFECTS;
}

auto lwg::is_closed(std::string const & stat) -> bool {
   return filename_for_status(stat) == LWG_CLOSED;
}

auto lwg::is_tentative(std::string const & stat) -> bool {
   // a more efficient implementation will use some variation of strcmp
   return 0 == stat.find("Tentatively");
}

auto lwg::is_not_resolved(std::string const & stat) -> bool {
   for( auto s : {"Core", "Deferred", "EWG", "New", "Open", "Review"}) { if(s == stat) return true; }
   return false;
}

auto lwg::is_votable(std::string stat) -> bool {
   stat = remove_tentatively(stat);
   for( auto s : {"Immediate", "Voting"}) { if(s == stat) return true; }
   return false;
}

auto lwg::is_ready(std::string stat) -> bool {
   return "Ready" == remove_tentatively(stat);
}

auto lwg::parse_issue_from_file(std::string tx, std::string const & filename, lwg::section_map & section_db) -> issue {
   struct bad_issue_file : std::runtime_error {
      bad_issue_file(std::string const & filename, char const * error_message)
         : runtime_error{"Error parsing issue file " + filename + ": " + error_message}
         {
      }
   };

   issue is;

   // Get issue number
   auto k = tx.find("<issue num=\"");
   if (k == std::string::npos) {
      throw bad_issue_file{filename, "Unable to find issue number"};
   }
   k += sizeof("<issue num=\"") - 1;
   auto l = tx.find('\"', k);
   std::istringstream temp{tx.substr(k, l-k)};
   temp >> is.num;

   // Get issue status
   k = tx.find("status=\"", l);
   if (k == std::string::npos) {
      throw bad_issue_file{filename, "Unable to find issue status"};
   }
   k += sizeof("status=\"") - 1;
   l = tx.find('\"', k);
   is.stat = tx.substr(k, l-k);

   // Get issue title
   k = tx.find("<title>", l);
   if (k == std::string::npos) {
      throw bad_issue_file{filename, "Unable to find issue title"};
   }
   k +=  sizeof("<title>") - 1;
   l = tx.find("</title>", k);
   is.title = tx.substr(k, l-k);

   // Get issue sections
   k = tx.find("<section>", l);
   if (k == std::string::npos) {
      throw bad_issue_file{filename, "Unable to find issue section"};
   }
   k += sizeof("<section>") - 1;
   l = tx.find("</section>", k);
   while (k < l) {
      k = tx.find('\"', k);
      if (k >= l) {
          break;
      }
      auto k2 = tx.find('\"', k+1);
      if (k2 >= l) {
         throw bad_issue_file{filename, "Unable to find issue section"};
      }
      ++k;
      is.tags.emplace_back(tx.substr(k, k2-k));
      if (section_db.find(is.tags.back()) == section_db.end()) {
          section_num num{};
          num.num.push_back(100 + 'X' - 'A');
          section_db[is.tags.back()] = num;
      }
      k = k2;
      ++k;
   }

   if (is.tags.empty()) {
      throw bad_issue_file{filename, "Unable to find issue section"};
   }

   // Get submitter
   k = tx.find("<submitter>", l);
   if (k == std::string::npos) {
      throw bad_issue_file{filename, "Unable to find issue submitter"};
   }
   k += sizeof("<submitter>") - 1;
   l = tx.find("</submitter>", k);
   is.submitter = tx.substr(k, l-k);

   // Get date
   k = tx.find("<date>", l);
   if (k == std::string::npos) {
      throw bad_issue_file{filename, "Unable to find issue date"};
   }
   k += sizeof("<date>") - 1;
   l = tx.find("</date>", k);
   temp.clear();
   temp.str(tx.substr(k, l-k));

   try {
      is.date = parse_date(temp);

      // Get modification date
      is.mod_date = report_date_file_last_modified(filename);
   }
   catch(std::exception const & ex) {
      throw bad_issue_file{filename, ex.what()};
   }

   // Get priority - this element is optional
   k = tx.find("<priority>", l);
   if (k != std::string::npos) {
      k += sizeof("<priority>") - 1;
      l = tx.find("</priority>", k);
      if (l == std::string::npos) {
         throw bad_issue_file{filename, "Corrupt 'priority' element: no closing tag"};
      }
      is.priority = std::stoi(tx.substr(k, l-k));
   }

   // Trim text to <discussion>
   k = tx.find("<discussion>", l);
   if (k == std::string::npos) {
      throw bad_issue_file{filename, "Unable to find issue discussion"};
   }
   tx.erase(0, k);

   // Find out if issue has a proposed resolution
   if (is_active(is.stat)) {
      auto k2 = tx.find("<resolution>", 0);
      if (k2 == std::string::npos) {
         is.has_resolution = false;
      }
      else {
         k2 += sizeof("<resolution>") - 1;
         auto l2 = tx.find("</resolution>", k2);
         is.resolution = tx.substr(k2, l2 - k2);
         if (is.resolution.length() < 15) {
            // Filter small ammounts of whitespace between tags, with no actual resolution
            is.resolution.clear();
         }
//         is.has_resolution = l2 - k2 > 15;
         is.has_resolution = !is.resolution.empty();
      }
   }
   else {
      is.has_resolution = true;
   }

   is.text = std::move(tx);
   return is;
}

// Functions to "normalize" a status string
auto lwg::remove_pending(std::string stat) -> std::string {
   using size_type = std::string::size_type;
   if(0 == stat.find("Pending")) {
      stat.erase(size_type{0}, size_type{8});
   }
   return stat;
}

auto lwg::remove_tentatively(std::string stat) -> std::string {
   using size_type = std::string::size_type;
   if(0 == stat.find("Tentatively")) {
      stat.erase(size_type{0}, size_type{12});
   }
   return stat;
}

auto lwg::remove_qualifier(std::string const & stat) -> std::string {
   return remove_tentatively(remove_pending(stat));
}

auto lwg::get_status_priority(std::string const & stat) noexcept -> std::ptrdiff_t {
   static char const * const status_priority[] {
      "Voting",
      "Tentatively Voting",
      "Immediate",
      "Ready",
      "Tentatively Ready",
      "Tentatively NAD Editorial",
      "Tentatively NAD Future",
      "Tentatively NAD",
      "Review",
      "New",
      "Open",
      "LEWG",
      "EWG",
      "Core",
      "Deferred",
      "Tentatively Resolved",
      "Pending DR",
      "Pending WP",
      "Pending Resolved",
      "Pending NAD Future",
      "Pending NAD Editorial",
      "Pending NAD",
      "NAD Future",
      "DR",
      "WP",
      "C++14",
      "C++11",
      "CD1",
      "TC1",
      "Resolved",
      "TRDec",
      "NAD Editorial",
      "NAD",
      "Dup",
      "NAD Concepts"
   };


#if !defined(DEBUG_SUPPORT)
   static auto const first = std::begin(status_priority);
   static auto const last  = std::end(status_priority);
   return std::find_if( first, last, [&](char const * str){ return str == stat; } ) - first;
#else
   // Diagnose when unknown status strings are passed
   static auto const first = std::begin(status_priority);
   static auto const last  = std::end(status_priority);
   auto const i = std::find_if( first, last, [&](char const * str){ return str == stat; } );
   if(last == i) {
      std::cout << "Unknown status: " << stat << std::endl;
   }
   return i - first;
#endif
}
