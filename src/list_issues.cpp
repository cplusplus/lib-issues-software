// This program reads all the issues in the issues directory passed as the first command line argument.
// If all documents are successfully parsed, it will generate the standard LWG Issues List documents
// for an ISO SC22 WG21 mailing.

// Based on code originally donated by Howard Hinnant
// Since modified by Alisdair Meredith

// Note that this program requires a reasonably conformant C++11 compiler, supporting at least:
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
//    consistent overloading of 'char const *' and 'std::string'

// Following the planned removal of (deprecated) bool post-increment operator, we now
// require the following C++14 library facilities:
//    exchange

// The following C++14 features are being considered for future cleanup/refactoring:
//    polymorphic lambdas
//    string UDLs

// The following TS features are also desirable
//    filesystem
//    string_view

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
// .  sort-by-last-modified-date should offer some filter or separation to see only the issues modified since the last meeting

// Missing standard facilities that we work around
// . Date
// . Filesystem navigation

// Missing standard library facilities that would probably not change this program
// . XML parser

// standard headers
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

// platform headers - requires a Posix compatible platform
// The hope is to replace all of this with the filesystem TS
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// solution specific headers
#include "issues.h"
#include "sections.h"


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


auto read_file_into_string(std::string const & filename) -> std::string {
   // read a text file completely into memory, and return its contents as
   // a 'string' for further manipulation.

   std::ifstream infile{filename.c_str()};
   if (!infile.is_open()) {
      throw std::runtime_error{"Unable to open file " + filename};
   }

   std::istreambuf_iterator<char> first{infile}, last{};
   return std::string {first, last};
}

// Issue-list specific functionality for the rest of this file
// ===========================================================

void filter_issues(std::string const & issues_path, lwg::section_map & section_db, std::function<bool(lwg::issue const &)> predicate) {
   // Open the specified directory, 'issues_path', and iterate all the '.xml' files
   // it contains, parsing each such file as an LWG issue document.  Write to 'out'
   // the number of every issue that satisfies the 'predicate'.
   //
   // The current implementation relies directly on POSIX headers, but the preferred
   // direction for the future is to switch over to the filesystem TS using directory
   // iterators.

   std::unique_ptr<DIR, int(&)(DIR*)> dir{opendir(issues_path.c_str()), closedir};
   if (!dir) {
      throw std::runtime_error{"Unable to open issues dir"};
   }

   while ( dirent* entry = readdir(dir.get()) ) {
      std::string const issue_file{ entry->d_name };
      if (0 == issue_file.find("issue") ) {
         auto const filename = issues_path + issue_file;
         auto const iss = parse_issue_from_file(read_file_into_string(filename), filename, section_db);
         if (predicate(iss)) {
            std::cout << iss.num << '\n';
         }
      }
   }
}

// ============================================================================================================

void check_is_directory(std::string const & directory) {
   struct stat sb;
   if (stat(directory.c_str(), &sb) != 0 or !S_ISDIR(sb.st_mode)) {
      throw std::runtime_error(directory + " is not an existing directory");
   }
}

int main(int argc, char const* argv[]) {
   try {
      bool trace_on{false};  // Will pick this up from the command line later

      if (argc != 2) {
         std::cerr << "Must specify exactly one status\n";
         return 2;
      }
      std::string const status{argv[1]};
  
      std::string path;
      char cwd[1024];
      if (getcwd(cwd, sizeof(cwd)) == 0) {
         std::cerr << "unable to getcwd\n";
         return 1;
      }
      path = cwd;
      if (path.back() != '/') { path.push_back('/'); }

      check_is_directory(path);

      lwg::section_map section_db = [&] {
         // This lambda expression scopes the lifetime of an open 'fstream'
         auto filename = path + "meta-data/section.data";
         std::ifstream infile{filename};
         if (!infile.is_open()) {
            throw std::runtime_error{"Can't open section.data at " + path + "meta-data"};
         }

         if (trace_on) {
            std::cout << "Reading section-tag index from: " << filename << std::endl;
         }

         return lwg::read_section_db(infile);
      }();

      filter_issues(path + "xml/", section_db, [status](lwg::issue const & iss) { return status == iss.stat; });
   }
   catch(std::exception const & ex) {
      std::cout << ex.what() << std::endl;
      return -1;
   }
}

