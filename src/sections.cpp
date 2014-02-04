#include "sections.h"

#include <cassert>
#include <istream>
#include <ostream>

#include <sstream>

#if !defined(CPP14_LIBRARY_IS_NEEDED_FOR_STD_EXCHANGE)
// This should be part of <utility> in C++14 lib
// Should also be more efficient than using ostringstream!
// Will be available soon when assuming libc++ with Clang 3.4, or gcc 4.9 and later
namespace {

template <typename TYPE, typename V_TYPE>
auto exchange(TYPE & object, V_TYPE && value) -> TYPE {
   auto result = object;
   object = std::forward<V_TYPE>(value);
   return result;
}

} // close unnamed namespace
#else
#include <utility>
using std::exchange;
#endif

auto lwg::operator < (section_num const & x, section_num const & y) noexcept -> bool {
   // prefices are unique, so there should be no need for a tiebreak.
   return (x.prefix < y.prefix) ?  true
        : (y.prefix < x.prefix) ? false
        : x.num < y.num;
}

auto lwg::operator == (section_num const & x, section_num const & y) noexcept -> bool {
   return (x.prefix != y.prefix)
        ? false
        : x.num == y.num;
}

auto lwg::operator != (section_num const & x, section_num const & y) noexcept -> bool {
   return !(x == y);
}

auto lwg::operator >> (std::istream& is, section_num& sn) -> std::istream & {
   sn.prefix.clear();
   sn.num.clear();
   ws(is);
   if (is.peek() == 'T') {
      is.get();
      if (is.peek() == 'R') {
         std::string w;
         is >> w;
         if (w == "R1") {
            sn.prefix = "TR1";
         }
         else if (w == "RDecimal") {
            sn.prefix = "TRDecimal";
         }
         else {
            throw std::runtime_error{"section_num format error"};
         }
         ws(is);
      }
      else {
         sn.num.push_back(100 + 'T' - 'A');
         if (is.peek() != '.') {
            return is;
         }
         is.get();
      }
   }

   while (true) {
      if (std::isdigit(is.peek())) {
         int n;
         is >> n;
         sn.num.push_back(n);
      }
      else {
         char c;
         is >> c;
         sn.num.push_back(100 + c - 'A');
      }
      if (is.peek() != '.') {
         break;
      }
      char dot;
      is >> dot;
   }
   return is;
}

auto lwg::operator << (std::ostream& os, section_num const & sn) -> std::ostream & {
   if (!sn.prefix.empty()) { os << sn.prefix << " "; }

   bool use_period{false};
   for (auto sub : sn.num ) {
      if (exchange(use_period, true)) {
         os << '.';
      }

      if (sub >= 100) {
         os << char(sub - 100 + 'A');
      }
      else {
         os << sub;
      }
   }
   return os;
}

auto lwg::read_section_db(std::istream & infile) -> section_map {
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

