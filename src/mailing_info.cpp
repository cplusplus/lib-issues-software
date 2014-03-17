#include "mailing_info.h"

#include "issues.h"

#include <algorithm>
#include <istream>
#include <sstream>
#include <iostream>

namespace {

void replace_all_irefs(std::vector<lwg::issue> const & issues, std::string & s) {
   // Replace all tagged "issues references" in string 's' with an HTML anchor-link to the live issue
   // in its appropriate issue list, as determined by the issue's status.
   // Format of an issue reference: <iref ref="ISS"/>
   // Format of anchor: <a href="INDEX.html#ISS">ISS</a>

   for (auto i = s.find("<iref ref=\""); i != std::string::npos; i = s.find("<iref ref=\"") ) {
      auto j = s.find('>', i);
      if (j == std::string::npos) {
         throw std::runtime_error{"missing '>' after iref"};
      }

      auto k = s.find('\"', i+5);
      if (k >= j) {
         throw std::runtime_error{"missing '\"' in iref"};
      }
      auto l = s.find('\"', k+1);
      if (l >= j) {
         throw std::runtime_error{"missing '\"' in iref"};
      }

      ++k;

      std::istringstream temp{s.substr(k, l-k)};
      int num;
      temp >> num;
      if (temp.fail()) {
         throw std::runtime_error{"bad number in iref"};
      }

      auto n = std::lower_bound(issues.begin(), issues.end(), num, lwg::order_by_issue_number{});
      if (n->num != num) {
         std::ostringstream er;
         er << "couldn't find number " << num << " in iref";
         throw std::runtime_error{er.str()};
      }

      std::string r{make_html_anchor(*n)};
      j -= i - 1;
      s.replace(i, j, r);
      // i += r.size() - 1;  // unused, copy/paste from elsewhere?
   }
}

} // close unnamed namespace

auto lwg::make_html_anchor(lwg::issue const & iss) -> std::string {
   auto temp = std::to_string(iss.num);

   std::string result{"<a href=\""};
   result += filename_for_status(iss.stat);
   result += '#';
   result += temp;
   result += "\">";
   result += temp;
   result += "</a>";
   return result;
}

namespace lwg
{

mailing_info::mailing_info(std::istream & stream)
   : m_data{std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{}}
{
  //  replace all text in the form:
  //      <replace "attribute-name"/>
  //  with the attribute-value for that attribute-name

  std::string::size_type first, last, pos = 0;
  while ((first = m_data.find("<replace \"", pos)) != std::string::npos)
  {
    last = m_data.find("\"/>", first + 10);
    if (last == std::string::npos)
    {
      std::string msg{std::string{"error in config.xml: failed to find close for: "}  +
        m_data.substr(first, first + 32).c_str() + "... "};
      throw std::runtime_error{msg.c_str()};
      return;
    }
    std::cout << "***attribute-name is " << m_data.substr(first+10, last-(first+10)) << std::endl;
    std::cout << "   attribute-value is " << get_attribute(m_data.substr(first+10, last-(first+10))) << std::endl;
    m_data.replace(first, last+3-first, get_attribute(m_data.substr(first+10, last-(first+10))));
    pos = last;
  }
}

auto mailing_info::get_doc_number(std::string doc) const -> std::string {
    if (doc == "active") {
        doc = "active_docno";
    }
    else if (doc == "defect") {
        doc = "defect_docno";
    }
    else if (doc == "closed") {
        doc = "closed_docno";
    }
    else {
        throw std::runtime_error{"unknown argument to get_doc_number: " + doc};
    }

    return get_attribute(doc);
}

auto mailing_info::get_doc_name() const -> std::string
{
  return get_attribute("doc_name");
}

auto mailing_info::get_doc_reference() const -> std::string
{
  return get_attribute("doc_reference");
}

auto mailing_info::get_file_name_prefix() const -> std::string
{
  return get_attribute("file_name_prefix");
}

auto mailing_info::get_intro(std::string doc) const -> std::string {
    if (doc == "active") {
        doc = "<intro list=\"Active\">";
    }
    else if (doc == "defect") {
        doc = "<intro list=\"Defects\">";
    }
    else if (doc == "closed") {
        doc = "<intro list=\"Closed\">";
    }
    else {
        throw std::runtime_error{"unknown argument to intro: " + doc};
    }

    auto i = m_data.find(doc);
    if (i == std::string::npos) {
        throw std::runtime_error{"Unable to find intro in config.xml"};
    }
    i += doc.size();
    auto j = m_data.find("</intro>", i);
    if (j == std::string::npos) {
        throw std::runtime_error{"Unable to parse intro in config.xml"};
    }
    return m_data.substr(i, j-i);
}


auto mailing_info::get_maintainer() const -> std::string {
   std::string r = get_attribute("maintainer");
   auto m = r.find("&lt;");
   if (m == std::string::npos) {
      throw std::runtime_error{"Unable to parse maintainer email address in config.xml"};
   }
   m += sizeof("&lt;") - 1;
   auto me = r.find("&gt;", m);
   if (me == std::string::npos) {
      throw std::runtime_error{"Unable to parse maintainer email address in config.xml"};
   }
   std::string email = r.substr(m, me-m);
   // &lt;                                    lwgchair@gmail.com    &gt;
   // &lt;<a href="mailto:lwgchair@gmail.com">lwgchair@gmail.com</a>&gt;
   r.replace(m, me-m, "<a href=\"mailto:" + email + "\">" + email + "</a>");
   return r;
}

auto mailing_info::get_revision() const -> std::string {
   return get_attribute("revision");
}


auto mailing_info::get_revisions(std::vector<issue> const & issues, std::string const & diff_report) const -> std::string {
   auto i = m_data.find("<revision_history>");
   if (i == std::string::npos) {
      throw std::runtime_error{"Unable to find <revision_history> in config.xml"};
   }
   i += sizeof("<revision_history>") - 1;

   auto j = m_data.find("</revision_history>", i);
   if (j == std::string::npos) {
      throw std::runtime_error{"Unable to find </revision_history> in config.xml"};
   }
   auto s = m_data.substr(i, j-i);
   j = 0;

   // bulding a potentially large string, would a stringstream be a better solution?
   // Probably not - string will not be *that* big, and stringstream pays the cost of locales
   std::string r = "<ul>\n";

   r += "<li>";
   r += get_revision() + ": " + get_attribute("date") + " " + get_attribute("title");   // We should date and *timestamp* this reference, as we expect to generate several documents per day
   r += diff_report;
   r += "</li>\n";

   while (true) {
      i = s.find("<revision tag=\"", j);
      if (i == std::string::npos) {
         break;
      }
      i += sizeof("<revision tag=\"") - 1;
      j = s.find('\"', i);
      std::string const rv = s.substr(i, j-i);
      i = j+2;
      j = s.find("</revision>", i);

      r += "<li>";
      r += rv + ": ";
      r += s.substr(i, j-i);
      r += "</li>\n";
   }
   r += "</ul>\n";

   replace_all_irefs(issues, r);

   return r;
}


auto mailing_info::get_statuses() const -> std::string {
   auto i = m_data.find("<statuses>");
   if (i == std::string::npos) {
      throw std::runtime_error{"Unable to find statuses in config.xml"};
   }
   i += sizeof("<statuses>") - 1;

   auto j = m_data.find("</statuses>", i);
   if (j == std::string::npos) {
      throw std::runtime_error{"Unable to parse statuses in config.xml"};
   }
   return m_data.substr(i, j-i);
}


auto mailing_info::get_attribute(std::string const & attribute_name) const -> std::string {
    std::string search_string{attribute_name + "=\""};
    auto i = m_data.find(search_string);
    if (i == std::string::npos) {
        throw std::runtime_error{"Unable to find " + attribute_name + " in config.xml"};
    }
    i += search_string.size();
    auto j = m_data.find('\"', i);
    if (j == std::string::npos) {
        throw std::runtime_error{"Unable to parse " + attribute_name + " in config.xml"};
    }
    return m_data.substr(i, j-i);
}

} // close namespace lwg
