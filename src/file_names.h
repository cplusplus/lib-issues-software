#ifndef INCLUDE_FILE_NAMES_H
#define INCLUDE_FILE_NAMES_H

#include <string>
#include "mailing_info.h"

namespace lwg {

  class file_names {
  public:
    file_names(lwg::mailing_info const& config);
    auto active_name() const -> const std::string& {return _active;}
    auto closed_name() const -> const std::string& {return _closed;}
    auto defects_name() const -> const std::string& {return _defects;}
    auto toc_name() const -> const std::string& {return _toc;}
    auto old_toc_name() const -> const std::string& {return _old_toc;}
    auto status_index_name() const -> const std::string& {return _status_index;}
    auto status_date_index_name() const -> const std::string& {return _status_date_index;}
    auto section_index_name() const -> const std::string& {return _section_index;}
    auto unresolved_toc_name() const -> const std::string& {return _unresolved_toc;}
    auto unresolved_status_index_name() const -> const std::string& {return _unresolved_status_index;}
    auto unresolved_status_date_index_name() const -> const std::string& {return _unresolved_status_date_index;}
    auto unresolved_section_index_name() const -> const std::string& {return _unresolved_section_index;}
    auto unresolved_prioritized_index_name() const -> const std::string& {return _unresolved_prioritized_index;}
    auto votable_toc_name() const -> const std::string& {return _votable_toc;}
    auto votable_status_index_name() const -> const std::string& {return _votable_status_index;}
    auto votable_status_date_index_name() const -> const std::string& {return _votable_status_date_index;}
    auto votable_section_index_name() const -> const std::string& {return _votable_section_index;}
    auto open_index_name() const -> const std::string& {return _open_index ;}
    auto tentative_name() const -> const std::string& {return _tentative;}
    auto unresolved_name() const -> const std::string& {return _unresolved;}
    auto immediate_name() const -> const std::string& {return _immediate;}
    auto issues_for_editor_name() const -> const std::string& {return _issues_for_editor;}
  private:
    std::string _active;
    std::string _closed;
    std::string _defects;
    std::string _toc;
    std::string _old_toc;
    std::string _status_index;
    std::string _status_date_index;
    std::string _section_index;
    std::string _unresolved_toc;
    std::string _unresolved_status_index;
    std::string _unresolved_status_date_index;
    std::string _unresolved_section_index;
    std::string _unresolved_prioritized_index;
    std::string _votable_toc;
    std::string _votable_status_index;
    std::string _votable_status_date_index;
    std::string _votable_section_index;
    std::string _open_index;
    std::string _tentative;
    std::string _unresolved;
    std::string _immediate;
    std::string _issues_for_editor;
  };
} // namespace lwg

#endif  // INCLUDE_FILE_NAMES_H
