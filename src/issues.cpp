#include "issues.h"

#include <istream>
#include <ostream>

#if 1
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

#if 0
// This should be part of <string> in C++11 lib
// Should also be more efficient than using ostringstream!
// Will be available soon when assuming libc++, or gcc 4.8 and later
auto to_string(int i) -> std::string {
   std::ostringstream t;
   t << i;
   return t.str();
}
#else
using std::to_string;
#endif

namespace {
static constexpr char const * LWG_ACTIVE {"lwg-active.html" };
static constexpr char const * LWG_CLOSED {"lwg-closed.html" };
static constexpr char const * LWG_DEFECTS{"lwg-defects.html"};

} // close unnamed namespace

auto lwg::operator < (section_num const & x, section_num const & y) noexcept -> bool {
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

auto lwg::make_ref_string(lwg::issue const & iss) -> std::string {
   auto temp = to_string(iss.num);

   std::string result{"<a href=\""};
   result += filename_for_status(iss.stat);
   result += '#';
   result += temp;
   result += "\">";
   result += temp;
   result += "</a>";
   return result;
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

