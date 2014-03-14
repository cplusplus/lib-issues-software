#ifndef INCLUDE_LWG_REPORT_GENERATOR_H
#define INCLUDE_LWG_REPORT_GENERATOR_H

#include <string>
#include <vector>

#include "issues.h"  // cannot forward declare the 'section_map' alias, nor the 'LwgIssuesXml' alias
#include "file_names.h"

namespace lwg
{
struct issue;
struct mailing_info;


struct report_generator {

   report_generator(mailing_info const & info, section_map & sections, file_names const & names_)
      : config(info)
      , section_db(sections)
      , names(names_)
   {
   }

   // Functions to make the 3 standard published issues list documents
   // A precondition for calling any of these functions is that the list of issues is sorted in numerical order, by issue number.
   // While nothing disasterous will happen if this precondition is violated, the published issues list will list items
   // in the wrong order.
   void make_active(std::vector<issue> const & issues, std::string const & path, std::string const & diff_report);

   void make_defect(std::vector<issue> const & issues, std::string const & path, std::string const & diff_report);

   void make_closed(std::vector<issue> const & issues, std::string const & path, std::string const & diff_report);

   // Additional non-standard documents, useful for running meetings
   void make_tentative(std::vector<issue> const & issues, std::string const & path);
      // publish a document listing all tentative issues that may be acted on during a meeting.


   void make_unresolved(std::vector<issue> const & issues, std::string const & path);
      // publish a document listing all non-tentative, non-ready issues that must be reviewed during a meeting.

   void make_immediate(std::vector<issue> const & issues, std::string const & path);
      // publish a document listing all non-tentative, non-ready issues that must be reviewed during a meeting.


   void make_sort_by_num(std::vector<issue>& issues, std::string const & filename);

   void make_sort_by_priority(std::vector<issue>& issues, std::string const & filename);

   void make_sort_by_status(std::vector<issue>& issues, std::string const & filename);

   void make_sort_by_status_mod_date(std::vector<issue> & issues, std::string const & filename);

   void make_sort_by_section(std::vector<issue>& issues, std::string const & filename, bool active_only = false);

   void make_editors_issues(std::vector<issue> const & issues, std::string const & path);

private:
   mailing_info const & config;
   section_map &        section_db;
   file_names const &   names;
};

} // close namespace lwg

#endif // INCLUDE_LWG_REPORT_GENERATOR_H
