#ifndef INCLUDE_LWG_REPORT_GENERATOR_H
#define INCLUDE_LWG_REPORT_GENERATOR_H

#include <string>
#include <vector>

#include "issues.h"  // cannot forward declare the 'section_map' alias, nor the 'LwgIssuesXml' alias

namespace lwg
{
struct issue;
struct mailing_info;

// Functions to make the 3 standard published issues list documents
// A precondition for calling any of these functions is that the list of issues is sorted in numerical order, by issue number.
// While nothing disasterous will happen if this precondition is violated, the published issues list will list items
// in the wrong order.
void make_active(std::vector<issue> const & issues, std::string const & path, mailing_info const & lwg_issues_xml, section_map & section_db, std::string const & diff_report);

void make_defect(std::vector<issue> const & issues, std::string const & path, mailing_info const & lwg_issues_xml, section_map & section_db, std::string const & diff_report);

void make_closed(std::vector<issue> const & issues, std::string const & path, mailing_info const & lwg_issues_xml, section_map & section_db, std::string const & diff_report);

// Additional non-standard documents, useful for running LWG meetings
void make_tentative(std::vector<issue> const & issues, std::string const & path, mailing_info const & lwg_issues_xml, section_map & section_db);
   // publish a document listing all tentative issues that may be acted on during a meeting.


void make_unresolved(std::vector<issue> const & issues, std::string const & path, mailing_info const & lwg_issues_xml, section_map & section_db);
   // publish a document listing all non-tentative, non-ready issues that must be reviewed during a meeting.

void make_immediate(std::vector<issue> const & issues, std::string const & path, mailing_info const & lwg_issues_xml, section_map & section_db);
   // publish a document listing all non-tentative, non-ready issues that must be reviewed during a meeting.


void make_sort_by_num(std::vector<issue>& issues, std::string const & filename, mailing_info const & lwg_issues_xml, section_map & section_db);

void make_sort_by_status(std::vector<issue>& issues, std::string const & filename, mailing_info const & lwg_issues_xml, section_map & section_db);

void make_sort_by_status_mod_date(std::vector<issue> & issues, std::string const & filename, mailing_info const & lwg_issues_xml, section_map & section_db);

void make_sort_by_section(std::vector<issue>& issues, std::string const & filename, mailing_info const & lwg_issues_xml, section_map & section_db, bool active_only = false);

} // close namespace lwg

#endif // INCLUDE_LWG_REPORT_GENERATOR_H
