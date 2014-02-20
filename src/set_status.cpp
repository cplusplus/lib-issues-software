// This program resets the status attribute of a single issue 
// It relies entirely on textual search/replace and does not
// use any other associated functionality of the list management
// tools.

// standard headers
#include <algorithm>
#include <fstream>
//#include <functional>
#include <iostream>
//#include <iterator>
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
//#include "issues.h"
//#include "sections.h"


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

struct bad_issue_file : std::runtime_error {
   bad_issue_file(std::string const & filename, std::string const & error_message)
      : runtime_error{"Error parsing issue file " + filename + ": " + error_message}
      {
   }
};


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

// ============================================================================================================

void check_is_directory(std::string const & directory) {
   struct stat sb;
   if (stat(directory.c_str(), &sb) != 0 or !S_ISDIR(sb.st_mode)) {
      throw std::runtime_error(directory + " is not an existing directory");
   }
}

int main(int argc, char const * argv[]) {
   try {
      bool trace_on{false};  // Will pick this up from the command line later

      if (argc != 3) {
         std::cerr << "Must specify exactly one issue, followed by its new status\n";
//         for (auto arg : argv) {
         for (int i{0}; argc != i;  ++i) {
            char const * arg = argv[i];
            std::cerr << "\t" << arg << "\n";
         }
         return -2;
      }

      int const issue_number = atoi(argv[1]);

      std::string new_status{argv[2]};
      std::replace(new_status.begin(), new_status.end(), '_', ' ');  // simplifies unix shell scripting
  
      std::string path;
      char cwd[1024];
      if (getcwd(cwd, sizeof(cwd)) == 0) {
         std::cerr << "unable to getcwd\n";
         return -3;
      }
      path = cwd;
      if (path.back() != '/') { path.push_back('/'); }

      check_is_directory(path);

      std::string issue_file = std::string{"issue"} + argv[1] + ".xml";
      auto const filename = path + "xml/" + issue_file;

      auto issue_data = read_file_into_string(filename);

      // find 'status' tag and replace it
      auto k = issue_data.find("<issue num=\"");
      if (k == std::string::npos) {
         throw bad_issue_file{filename, "Unable to find issue number"};
      }
      k += sizeof("<issue num=\"") - 1;
      auto l = issue_data.find('"', k);
      if (l == std::string::npos) {
         throw bad_issue_file{filename, "Corrupt issue number attribute"};
      }
      if (std::stod(issue_data.substr(k, k-l)) != issue_number) {
         throw bad_issue_file{filename, "Issue number does not match filename"};
      }

      k = issue_data.find("status=\"");
      if (k == std::string::npos) {
         throw bad_issue_file{filename, "Unable to find issue status"};
      }
      k += sizeof("status=\"") - 1;
      l = issue_data.find('"', k);
      if (l == std::string::npos) {
         throw bad_issue_file{filename, "Corrupt status attribute"};
      }
      issue_data.replace(k, l-k, new_status);

      std::ofstream out_file{filename};
      if (!out_file.is_open()) {
         throw std::runtime_error{"Unable to re-open file " + filename};
      }

      out_file << issue_data;
   }
   catch(std::exception const & ex) {
      std::cout << ex.what() << std::endl;
      return -1;
   }
}


