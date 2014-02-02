#ifndef INCLUDE_LWG_MAILING_INFO_H
#define INCLUDE_LWG_MAILING_INFO_H

#include <string>
#include <vector>

namespace lwg
{

struct issue;

struct mailing_info {
   explicit mailing_info(std::string const & path);

   auto get_doc_number(std::string doc) const -> std::string;
   auto get_intro(std::string doc) const -> std::string;
   auto get_maintainer() const -> std::string;
   auto get_revision() const -> std::string;
   auto get_revisions(std::vector<issue> const & issues, std::string const & diff_report) const -> std::string;
   auto get_statuses() const -> std::string;

private:
   auto get_attribute(std::string const & attribute) const -> std::string;

   // m_data is reparsed too many times in practice, and memory use is not a major concern.
   // Should cache each of the reproducible calls in additional member strings, either at
   // construction, or lazily on each function eval, checking if the cached string is 'empty'.
   std::string m_data;
};

}

using LwgIssuesXml = lwg::mailing_info;

#endif // INCLUDE_LWG_MAILING_INFO_H
