#include "report_generator.h"

#include "mailing_info.h"
#include "sections.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>  // reference_wrapper
#include <memory>
#include <sstream>
#include <stdexcept>

namespace
{

// Generic utilities that are useful and do not rely on context or types from our domain (issue-list processing)
// =============================================================================================================

auto format_time(std::string const & format, std::tm const & t) -> std::string {
   std::string s;
   std::size_t maxsize{format.size() + 256};
  //for (std::size_t maxsize = format.size() + 64; s.size() == 0 ; maxsize += 64)
  //{
      std::unique_ptr<char[]> buf{new char[maxsize]};
      std::size_t size{std::strftime( buf.get(), maxsize, format.c_str(), &t ) };
      if(size > 0) {
         s += buf.get();
      }
 // }
   return s;
}

auto utc_timestamp() -> std::tm const & {
   static std::time_t t{ std::time(nullptr) };
   static std::tm utc = *std::gmtime(&t);
   return utc;
}

// global data - would like to do something about that.
static std::string const build_timestamp{format_time("<p>Revised %Y-%m-%d at %H:%m:%S UTC</p>\n", utc_timestamp())};


struct order_by_first_tag {
   bool operator()(lwg::issue const & x, lwg::issue const & y) const noexcept {
      assert(!x.tags.empty());
      assert(!y.tags.empty());
      return x.tags.front() < y.tags.front();
   }
};

struct order_by_major_section {
   explicit order_by_major_section(lwg::section_map & sections) 
      : section_db(sections)
      {
      }

   auto operator()(lwg::issue const & x, lwg::issue const & y) const -> bool {
assert(!x.tags.empty());
assert(!y.tags.empty());
      lwg::section_num const & xn = section_db.get()[x.tags[0]];
      lwg::section_num const & yn = section_db.get()[y.tags[0]];
      return  xn.prefix < yn.prefix
          or (xn.prefix > yn.prefix  and  xn.num[0] < yn.num[0]);
   }

private:
   std::reference_wrapper<lwg::section_map> section_db;
};

struct order_by_section {
   explicit order_by_section(lwg::section_map &sections) 
      : section_db(sections)
      {
      }

   auto operator()(lwg::issue const & x, lwg::issue const & y) const -> bool {
      assert(!x.tags.empty());
      assert(!y.tags.empty());
      return section_db.get()[x.tags.front()] < section_db.get()[y.tags.front()];
   }

private:
   std::reference_wrapper<lwg::section_map> section_db;
};

struct order_by_status {
   auto operator()(lwg::issue const & x, lwg::issue const & y) const noexcept -> bool {
      return lwg::get_status_priority(x.stat) < lwg::get_status_priority(y.stat);
   }
};


struct order_by_priority {
   explicit order_by_priority(lwg::section_map &sections) 
      : section_db(sections)
      {
      }

   auto operator()(lwg::issue const & x, lwg::issue const & y) const -> bool {
      assert(!x.tags.empty());
      assert(!y.tags.empty());
      return x.priority == y.priority
           ? section_db.get()[x.tags.front()] < section_db.get()[y.tags.front()]
           : x.priority < y.priority;
   }

private:
   std::reference_wrapper<lwg::section_map> section_db;
};


auto major_section(lwg::section_num const & sn) -> std::string {
   std::string const prefix{sn.prefix};
   std::ostringstream out;
   if (!prefix.empty()) {
      out << prefix << " ";
   }
   if (sn.num[0] < 100) {
      out << sn.num[0];
   }
   else {
      out << char(sn.num[0] - 100 + 'A');
   }
   return out.str();
}

auto remove_square_brackets(lwg::section_tag const & tag) -> lwg::section_tag {
   //assert(tag.size() > 2);
   //assert(tag.front() == '[');
   //assert(tag.back() == ']');
   return tag.substr(1, tag.size()-2);
}


void print_date(std::ostream & out, gregorian::date const & mod_date ) {
   out << mod_date.year() << '-';
   if (mod_date.month() < 10) { out << '0'; }
   out << mod_date.month() << '-';
   if (mod_date.day() < 10) { out << '0'; }
   out << mod_date.day();
}

template<typename Container>
void print_list(std::ostream & out, Container const & source, char const * separator ) {
   char const * sep{""};
   for( auto const & x : source ) {
      out << sep << x;
      sep = separator;
   }
}



void print_file_header(std::ostream& out, std::string const & title) {
   out <<
R"(<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
    "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>)" << title << R"(</title>
<style type="text/css">
  p {text-align:justify}
  li {text-align:justify}
  blockquote.note
  {
    background-color:#E0E0E0;
    padding-left: 15px;
    padding-right: 15px;
    padding-top: 1px;
    padding-bottom: 1px;
  }
  ins {background-color:#A0FFA0}
  del {background-color:#FFA0A0}
</style>
</head>
<body>
)";

}


void print_file_trailer(std::ostream& out) {
    out << "</body>\n";
    out << "</html>\n";
}


void print_table(std::ostream& out, std::vector<lwg::issue>::const_iterator i, std::vector<lwg::issue>::const_iterator e,
  lwg::section_map & section_db, lwg::file_names const & names) {
#if defined (DEBUG_LOGGING)
   std::cout << "\t" << std::distance(i,e) << " items to add to table" << std::endl;
#endif

   out <<
R"(<table border="1" cellpadding="4">
<tr>
  <td align="center"><a href=")" << names.active_name() << R"("><b>Issue</b></a></td>
  <td align="center"><a href=")" << names.status_index_name() << R"("><b>Status</b></a></td>
  <td align="center"><a href=")" << names.section_index_name() << R"("><b>Section</b></a></td>
  <td align="center"><b>Title</b></td>
  <td align="center"><b>Proposed Resolution</b></td>
  <td align="center"><a href=")" << names.unresolved_prioritized_index_name() << R"("><b>Priority</b></a></td>
  <td align="center"><b>Duplicates</b></td>
</tr>
)";

   std::string prev_tag;
   for (; i != e; ++i) {
      out << "<tr>\n";

      // Number
      out << "<td align=\"right\">" << make_html_anchor(*i) << "</td>\n";

      // Status
      out << "<td align=\"left\"><a href=\"" << names.active_name() << "#" << lwg::remove_qualifier(i->stat) << "\">"
          << i->stat << "</a><a name=\"" << i->num << "\"></a></td>\n";

      // Section
      out << "<td align=\"left\">";
assert(!i->tags.empty());
      out << section_db[i->tags[0]] << " " << i->tags[0];
      if (i->tags[0] != prev_tag) {
         prev_tag = i->tags[0];
         out << "<a name=\"" << remove_square_brackets(prev_tag) << "\"</a>";
      }
      out << "</td>\n";

      // Title
      out << "<td align=\"left\">" << i->title << "</td>\n";

      // Has Proposed Resolution
      out << "<td align=\"center\">";
      if (i->has_resolution) {
         out << "Yes";
      }
      else {
         out << "<font color=\"red\">No</font>";
      }
      out << "</td>\n";

      // Priority
      out << "<td align=\"center\">";
      if (i->priority != 99) {
         out << i->priority;
      }
      out << "</td>\n";

      // Duplicates
      out << "<td align=\"left\">";
      print_list(out, i->duplicates, ", ");
      out << "</td>\n"
          << "</tr>\n";
   }
   out << "</table>\n";
}

template <typename Pred>
void print_issues(std::ostream & out, std::vector<lwg::issue> const & issues, lwg::section_map & section_db,
  lwg::file_names const & names, Pred pred) {
   std::multiset<lwg::issue, order_by_first_tag> const  all_issues{ issues.begin(), issues.end()} ;
   std::multiset<lwg::issue, order_by_status>    const  issues_by_status{ issues.begin(), issues.end() };

   std::multiset<lwg::issue, order_by_first_tag> active_issues;
   for (auto const & elem : issues ) {
      if (lwg::is_active(elem.stat)) {
         active_issues.insert(elem);
      }
   }

   for (auto const & iss : issues) {
      if (pred(iss)) {
         out << "<hr>\n";

         // Number and title
         out << "<h3><a name=\"" << iss.num << "\"></a>" << iss.num << ". " << iss.title << "</h3>\n";

         // Section, Status, Submitter, Date
         out << "<p><b>Section:</b> ";
         out << section_db[iss.tags[0]] << " " << iss.tags[0];
         for (unsigned k = 1; k < iss.tags.size(); ++k) {
            out << ", " << section_db[iss.tags[k]] << " " << iss.tags[k];
         }

         out << " <b>Status:</b> <a href=\"" << names.active_name() << "#" << lwg::remove_qualifier(iss.stat) << "\">"
             << iss.stat << "</a>\n";
         out << " <b>Submitter:</b> " << iss.submitter
             << " <b>Opened:</b> ";
         print_date(out, iss.date);
         out << " <b>Last modified:</b> ";
         print_date(out, iss.mod_date);
         out << "</p>\n";

         // view active issues in []
         if (active_issues.count(iss) > 1) {
           out << "<p><b>View other</b> <a href=\"" << names.open_index_name() << "#" << remove_square_brackets(iss.tags[0])
               << "\">active issues</a> in " << iss.tags[0] << ".</p>\n";
         }

         // view all issues in []
         if (all_issues.count(iss) > 1) {
           out << "<p><b>View all other</b> <a href=\"" << names.section_index_name() << "#" << remove_square_brackets(iss.tags[0])
               << "\">issues</a> in " << iss.tags[0] << ".</p>\n";
         }
         // view all issues with same status
         if (issues_by_status.count(iss) > 1) {
           out << "<p><b>View all issues with</b> <a href=\"" << names.status_index_name() << "#" << iss.stat << "\">"
               << iss.stat << "</a> status.</p>\n";
         }

         // duplicates
         if (!iss.duplicates.empty()) {
            out << "<p><b>Duplicate of:</b> ";
            print_list(out, iss.duplicates, ", ");
            out << "</p>\n";
         }

         // text
         out << iss.text << "\n\n";
      }
   }
}

template <typename Pred>
void print_resolutions(std::ostream & out, std::vector<lwg::issue> const & issues, lwg::section_map & section_db, Pred predicate) {
   // This construction calls out for filter-iterators
//   std::multiset<lwg::issue, order_by_first_tag> pending_issues;
   std::vector<lwg::issue> pending_issues;
   for (auto const & elem : issues ) {
      if (predicate(elem)) {
         pending_issues.emplace_back(elem);
      }
   }

   sort(begin(pending_issues), end(pending_issues), order_by_section{section_db});

   for (auto const & iss : pending_issues) {
      if (predicate(iss)) {
         out << "<hr>\n"

             // Number and title
             << "<h3><a name=\"" << iss.num << "\"></a>" << iss.num << ". " << iss.title << "</h3>\n"

             // text
             << iss.resolution << "\n\n";
      }
   }
}

void print_paper_heading(std::ostream& out, std::string const & paper, lwg::mailing_info const & config) {
   out <<
R"(<table>
<tr>
  <td align="left">Doc. no.</td>
  <td align="left">)" << config.get_doc_number(paper) << R"(</td>
</tr>
<tr>
  <td align="left">Date:</td>
  <td align="left">)" << format_time("%Y-%m-%d", utc_timestamp()) << R"(</td>
</tr>
<tr>
  <td align="left">Project:</td>
  <td align="left">Programming Language C++</td>
</tr>
<tr>
  <td align="left">Reply to:</td>
  <td align="left">)" << config.get_maintainer() << R"(</td>
</tr>
</table>
)";

   out << "<h1>";
   if (paper == "active") {
      out << config.get_doc_name() + " Active Issues List (Revision ";
   }
   else if (paper == "defect") {
      out << config.get_doc_name() + " Defect Report List (Revision ";
   }
   else if (paper == "closed") {
      out << config.get_doc_name() + " Closed Issues List (Revision ";
   }
   out << config.get_revision() << ")</h1>\n";
   out << "<p>" << build_timestamp << "</p>";
}

} // close unnamed namespace

namespace lwg
{

// Functions to make the 3 standard published issues list documents
// A precondition for calling any of these functions is that the list of issues is sorted in numerical order, by issue number.
// While nothing disasterous will happen if this precondition is violated, the published issues list will list items
// in the wrong order.
void report_generator::make_active(std::vector<issue> const & issues, std::string const & path, std::string const & diff_report) {
   assert(std::is_sorted(issues.begin(), issues.end(), order_by_issue_number{}));

   std::string filename{path + names.active_name()};
   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, config.get_doc_name() + " Active Issues List");
   print_paper_heading(out, "active", config);
   out << config.get_intro("active") << '\n';
   out << "<h2>Revision History</h2>\n" << config.get_revisions(issues, diff_report) << '\n';
   out << "<h2><a name=\"Status\"></a>Issue Status</h2>\n" << config.get_statuses() << '\n';
   out << "<h2>Active Issues</h2>\n";
   print_issues(out, issues, section_db, names, [](issue const & i) {return is_active(i.stat);} );
   print_file_trailer(out);
}


void report_generator::make_defect(std::vector<issue> const & issues, std::string const & path, std::string const & diff_report) {
   assert(std::is_sorted(issues.begin(), issues.end(), order_by_issue_number{}));

   std::string filename{path + names.defects_name()};
   std::ofstream out(filename.c_str());
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, config.get_doc_name() + " Defect Report List");
   print_paper_heading(out, "defect", config);
   out << config.get_intro("defect") << '\n';
   out << "<h2>Revision History</h2>\n" << config.get_revisions(issues, diff_report) << '\n';
   out << "<h2>Defect Reports</h2>\n";
   print_issues(out, issues, section_db, names, [](issue const & i) {return is_defect(i.stat);} );
   print_file_trailer(out);
}


void report_generator::make_closed(std::vector<issue> const & issues, std::string const & path, std::string const & diff_report) {
   assert(std::is_sorted(issues.begin(), issues.end(), order_by_issue_number{}));

   std::string filename{path + names.closed_name()};
   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, config.get_doc_name() + " Closed Issues List");
   print_paper_heading(out, "closed", config);
   out << config.get_intro("closed") << '\n';
   out << "<h2>Revision History</h2>\n" << config.get_revisions(issues, diff_report) << '\n';
   out << "<h2>Closed Issues</h2>\n";
   print_issues(out, issues, section_db, names, [](issue const & i) {return is_closed(i.stat);} );
   print_file_trailer(out);
}


// Additional non-standard documents, useful for running meetings
void report_generator::make_tentative(std::vector<issue> const & issues, std::string const & path) {
   // publish a document listing all tentative issues that may be acted on during a meeting.
   assert(std::is_sorted(issues.begin(), issues.end(), order_by_issue_number{}));

   std::string filename{path + names.tentative_name()};
   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, config.get_doc_name() + " Tentative Issues");
//   print_paper_heading(out, "active", config);
//   out << config.get_intro("active") << '\n';
//   out << "<h2>Revision History</h2>\n" << config.get_revisions(issues) << '\n';
//   out << "<h2><a name=\"Status\"></a>Issue Status</h2>\n" << config.get_statuses() << '\n';
   out << "<p>" << build_timestamp << "</p>";
   out << "<h2>Tentative Issues</h2>\n";
   print_issues(out, issues, section_db, names, [](issue const & i) {return is_tentative(i.stat);} );
   print_file_trailer(out);
}


void report_generator::make_unresolved(std::vector<issue> const & issues, std::string const & path) {
   // publish a document listing all non-tentative, non-ready issues that must be reviewed during a meeting.
   assert(std::is_sorted(issues.begin(), issues.end(), order_by_issue_number{}));

   std::string filename{path + names.unresolved_name()};
   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, config.get_doc_name() + " Unresolved Issues");
//   print_paper_heading(out, "active", config);
//   out << config.get_intro("active") << '\n';
//   out << "<h2>Revision History</h2>\n" << config.get_revisions(issues) << '\n';
//   out << "<h2><a name=\"Status\"></a>Issue Status</h2>\n" << config.get_statuses() << '\n';
   out << "<p>" << build_timestamp << "</p>";
   out << "<h2>Unresolved Issues</h2>\n";
   print_issues(out, issues, section_db, names, [](issue const & i) {return is_not_resolved(i.stat);} );
   print_file_trailer(out);
}

void report_generator::make_immediate(std::vector<issue> const & issues, std::string const & path) {
   // publish a document listing all non-tentative, non-ready issues that must be reviewed during a meeting.
   assert(std::is_sorted(issues.begin(), issues.end(), order_by_issue_number{}));

   std::string filename{path + names.unresolved_name()};
   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, config.get_doc_name() + " Issues Resolved Directly In [INSERT CURRENT MEETING HERE]");
   out << R"(<h1>)" << config.get_doc_name() << R"( Issues Resolved Directly In [INSERT CURRENT MEETING HERE]</h1>
<table>
<tr>
<td align="left">Doc. no.</td>
<td align="left">N4???</td>
</tr>
<tr>
<td align="left">Date:</td>
<td align="left">)" << build_timestamp << R"(</td>
</tr>
<tr>
<td align="left">Project:</td>
<td align="left">Programming Language C++</td>
</tr>
<tr>
<td align="left">Reply to:</td>
<td align="left">Alisdair Meredith &lt;<a href="mailto:lwgchair@gmail.com">lwgchair@gmail.com</a>&gt;</td>
</tr>
</table>
)";
   out << "<h2>Immediate Issues</h2>\n";
   print_issues(out, issues, section_db, names, [](issue const & i) {return "Immediate" == i.stat;} );
   print_file_trailer(out);
}

void report_generator::make_editors_issues(std::vector<issue> const & issues, std::string const & path) {
   // publish a single document listing all 'Voting' and 'Immediate' resolutions (only).
   assert(std::is_sorted(issues.begin(), issues.end(), order_by_issue_number{}));

   std::string filename{path + names.issues_for_editor_name()};
   std::ofstream out{filename.c_str()};
   if (!out) {
     throw std::runtime_error{"Failed to open " + filename};
   }
   print_file_header(out, config.get_doc_name() + " Issues Resolved Directly In [INSERT CURRENT MEETING HERE]");
   out << "<h1>" << config.get_doc_name() << " Resolved In [INSERT CURRENT MEETING HERE]</h1>\n";
   print_resolutions(out, issues, section_db, [](issue const & i) {return "Pending WP" == i.stat;} );
   print_file_trailer(out);
}

void report_generator::make_sort_by_num(std::vector<issue>& issues, std::string const & filename) {
   sort(issues.begin(), issues.end(), order_by_issue_number{});

   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, "Table of Contents");

   out << "<h1>" << config.get_doc_name() <<
R"( Issues List (Revision )" << config.get_revision() << R"()</h1>
<h1>Table of Contents</h1>
<p>Reference )" << config.get_doc_reference() << R"(</p>
<p>This document is the Table of Contents for the <a href=")" << names.active_name() << R"(">Active Issues List</a>,
<a href=")" << names.defects_name() << R"(">Defect Reports List</a>,
and <a href=")" << names.closed_name() << R"(">Closed Issues List</a>.</p>
)";
   out << "<p>" << build_timestamp << "</p>";

   print_table(out, issues.begin(), issues.end(), section_db, names);
   print_file_trailer(out);
}


void report_generator::make_sort_by_priority(std::vector<issue>& issues, std::string const & filename) {
   sort(issues.begin(), issues.end(), order_by_priority{section_db});

   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, "Table of Contents");

   out << "<h1>" << config.get_doc_name() <<
R"( Issues List (Revision )" << config.get_revision() << R"()</h1>
<h1>Index by Issue Number</h1>
<p>Reference )" << config.get_doc_reference() << R"(</p>
<p>This document is the Table of Contents for the <a href=")" << names.active_name() << R"(">Active Issues List</a>,
<a href=")" << names.defects_name() << R"(">Defect Reports List</a>,
and <a href=")" << names.closed_name() << R"(">Closed Issues List</a>.</p>
)";
   out << "<p>" << build_timestamp << "</p>";

//   print_table(out, issues.begin(), issues.end(), section_db, names);

   for (auto i = issues.cbegin(), e = issues.cend(); i != e;) {
      int px = i->priority;
      auto j = std::find_if(i, e, [&](issue const & iss){ return iss.priority != px; } );
      out << "<h2><a name=\"Priority " << px << "\"</a>";
      if (px == 99) {
         out << "Not Prioritized";
      }
      else {
         out << "Priority " << px;
      }
      out << " (" << (j-i) << " issues)</h2>\n";
      print_table(out, i, j, section_db, names);
      i = j;
   }

   print_file_trailer(out);
}


void report_generator::make_sort_by_status(std::vector<issue>& issues, std::string const & filename) {
   sort(issues.begin(), issues.end(), order_by_issue_number{});
   stable_sort(issues.begin(), issues.end(), [](issue const & x, issue const & y) { return x.mod_date > y.mod_date; } );
   stable_sort(issues.begin(), issues.end(), order_by_section{section_db});
   stable_sort(issues.begin(), issues.end(), order_by_status{});

   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, "Index by Status and Section");

   out << "<h1>" << config.get_doc_name() <<
R"( Issues List (Revision )" << config.get_revision() << R"()</h1>
<h1>Index by Status and Section</h1>
<p>Reference )" << config.get_doc_reference() << R"(</p>
<p>
This document is the Index by Status and Section for the <a href=")" << names.active_name() << R"(">Active Issues List</a>,
<a href=")" << names.defects_name() << R"(">Defect Reports List</a>,
and <a href=")" << names.closed_name() << R"(">Closed Issues List</a>.
</p>

)";
   out << "<p>" << build_timestamp << "</p>";

   for (auto i = issues.cbegin(), e = issues.cend(); i != e;) {
      auto const & current_status = i->stat;
      auto j = std::find_if(i, e, [&](issue const & iss){ return iss.stat != current_status; } );
      out << "<h2><a name=\"" << current_status << "\"</a>" << current_status << " (" << (j-i) << " issues)</h2>\n";
      print_table(out, i, j, section_db, names);
      i = j;
   }

   print_file_trailer(out);
}


void report_generator::make_sort_by_status_mod_date(std::vector<issue> & issues, std::string const & filename) {
   sort(issues.begin(), issues.end(), order_by_issue_number{});
   stable_sort(issues.begin(), issues.end(), order_by_section{section_db});
   stable_sort(issues.begin(), issues.end(), [](issue const & x, issue const & y) { return x.mod_date > y.mod_date; } );
   stable_sort(issues.begin(), issues.end(), order_by_status{});

   std::ofstream out{filename.c_str()};
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, "Index by Status and Date");

   out << "<h1>" << config.get_doc_name() <<
R"( Issues List (Revision )" << config.get_revision() << R"()</h1>
<h1>Index by Status and Date</h1>
<p>Reference )" << config.get_doc_reference() << R"(</p>
<p>
This document is the Index by Status and Date for the <a href=")" << names.active_name() << R"(">Active Issues List</a>,
<a href=")" << names.defects_name() << R"(">Defect Reports List</a>,
and <a href=")" << names.closed_name() << R"(">Closed Issues List</a>.
</p>
)";
   out << "<p>" << build_timestamp << "</p>";

   for (auto i = issues.cbegin(), e = issues.cend(); i != e;) {
      std::string const & current_status = i->stat;
      auto j = find_if(i, e, [&](issue const & iss){ return iss.stat != current_status; } );
      out << "<h2><a name=\"" << current_status << "\"</a>" << current_status << " (" << (j-i) << " issues)</h2>\n";
      print_table(out, i, j, section_db, names);
      i = j;
   }

   print_file_trailer(out);
}


void report_generator::make_sort_by_section(std::vector<issue>& issues, std::string const & filename, bool active_only) {
   sort(issues.begin(), issues.end(), order_by_issue_number{});
   stable_sort(issues.begin(), issues.end(), [](issue const & x, issue const & y) { return x.mod_date > y.mod_date; } );
   stable_sort(issues.begin(), issues.end(), order_by_status{});
   auto b = issues.begin();
   auto e = issues.end();
   if(active_only) {
      auto bReady = find_if(b, e, [](issue const & iss){ return "Ready" == iss.stat; });
      if(bReady != e) {
         b = bReady;
      }
      b = find_if(b, e, [](issue const & iss){ return "Ready" != iss.stat; });
      e = find_if(b, e, [](issue const & iss){ return !is_active(iss.stat); });
   }
   stable_sort(b, e, order_by_section{section_db});
   std::set<issue, order_by_major_section> mjr_section_open{order_by_major_section{section_db}};
   for (auto const & elem : issues ) {
      if (is_active_not_ready(elem.stat)) {
         mjr_section_open.insert(elem);
      }
   }

   std::ofstream out(filename.c_str());
   if (!out)
     throw std::runtime_error{"Failed to open " + filename};
   print_file_header(out, "Index by Section");

   out << "<h1>C++ Standard Library Issues List (Revision " << config.get_revision() << ")</h1>\n";
   out << "<h1>Index by Section</h1>\n";
   out << "<p>Reference " << config.get_doc_reference() << "</p>\n";
   out << "<p>This document is the Index by Section for the <a href=\"" << names.active_name() << "\">Library Active Issues List</a>";
   if(!active_only) {
     out << ", <a href=\"" << names.defects_name() << "\">Library Defect Reports List</a>,"
       " and <a href=\"" << names.closed_name() << "\">Library Closed Issues List</a>";
   }
   out << ".</p>\n";
   out << "<h2>Index by Section";
   if (active_only) {
      out << " (non-Ready active issues only)";
   }
   out << "</h2>\n";
   if (active_only) {
     out << "<p><a href=\"" << names.section_index_name() << "\">(view all issues)</a></p>\n";
   }
   else {
     out << "<p><a href=\"" << names.open_index_name() << "\">(view only non-Ready open issues)</a></p>\n";
   }
   out << "<p>" << build_timestamp << "</p>";

   // Would prefer to use const_iterators from here, but oh well....
   for (auto i = b; i != e;) {
assert(!i->tags.empty());
      int current_num = section_db[i->tags[0]].num[0];
      auto j = i;
      for (; j != e; ++j) {
         if (section_db[j->tags[0]].num[0] != current_num) {
             break;
         }
      }
      std::string const msn{major_section(section_db[i->tags[0]])};
      out << "<h2><a name=\"Section " << msn << "\"></a>" << "Section " << msn << " (" << (j-i) << " issues)</h2>\n";
      if (active_only) {
        out << "<p><a href=\"" << names.section_index_name() << "#Section " << msn << "\">(view all issues)</a></p>\n";
      }
      else if (mjr_section_open.count(*i) > 0) {
        out << "<p><a href=\"" << names.open_index_name() << "#Section " << msn << "\">(view only non-Ready open issues)</a></p>\n";
      }

      print_table(out, i, j, section_db, names);
      i = j;
   }

   print_file_trailer(out);
}

} // close namespace lwg

