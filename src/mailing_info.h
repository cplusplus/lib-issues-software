#ifndef INCLUDE_LWG_MAILING_INFO_H
#define INCLUDE_LWG_MAILING_INFO_H

#include <iosfwd>
#include <string>
#include <vector>

namespace lwg
{

struct issue;

struct mailing_info {
   explicit mailing_info(std::istream & stream);

   auto get_doc_number(std::string doc) const -> std::string;
   auto get_intro(std::string doc) const -> std::string;
   auto get_maintainer() const -> std::string;
   auto get_revision() const -> std::string;
   auto get_revisions(std::vector<issue> const & issues, std::string const & diff_report) const -> std::string;
   auto get_statuses() const -> std::string;
   auto get_doc_name() const -> std::string;  // Examples: "C++ Standard Library" or "File System Technical Specification"
   auto get_doc_reference() const -> std::string;  // Examples: "ISO/IEC IS 14882:2011(E)" or "ISO/IEC TS 18822"
   auto get_file_name_prefix() const->std::string; // Examples: "lwg-", "sg3-", ""

   auto get_attribute(std::string const & attribute_name) const -> std::string;
      // Return the value of the first xml attibute having the specified 'attribute_name'
      // in the stored XML string, 'm_data', without regard to which element holds that
      // attribute.

private:
   std::string m_data;
      // 'm_data' is reparsed too many times in practice, and memory use is not a major concern.
      // Should cache each of the reproducible calls in additional member strings, either at
      // construction, or lazily on each function eval, checking if the cached string is 'empty'.
      // Note that 'm_data' is immutable, we could use 'experimental::string_view' with confidence.
      // However, we do not mark it as 'const', as it would kill the implicit move constructor.
};

// odd place for this to land up, but currently the lowest dependency.
auto make_html_anchor(issue const & iss) -> std::string;

}

#endif // INCLUDE_LWG_MAILING_INFO_H
