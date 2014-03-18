#include "file_names.h"

namespace lwg {
  file_names::file_names(lwg::mailing_info const& config)
  {
    _active = config.get_attribute("active_name");
    _closed = config.get_attribute("closed_name");
    _defects = config.get_attribute("defects_name");
    _toc = config.get_attribute("toc_name");
    _old_toc = config.get_attribute("old_toc_name");
    _status_index = config.get_attribute("status_index_name");  // not "index_name") to avoid name clash
    _status_date_index = config.get_attribute("status_date_index_name");
    _section_index = config.get_attribute("section_index_name");
    _unresolved_toc = config.get_attribute("unresolved_toc_name");
    _unresolved_status_index = config.get_attribute("unresolved_status_index_name");
    _unresolved_status_date_index = config.get_attribute("unresolved_status_date_index_name");
    _unresolved_section_index = config.get_attribute("unresolved_section_index_name");
    _unresolved_prioritized_index = config.get_attribute("unresolved_prioritized_index_name");
    _votable_toc = config.get_attribute("votable_toc_name");
    _votable_status_index = config.get_attribute("votable_status_index_name");
    _votable_status_date_index = config.get_attribute("votable_status_date_index_name");
    _votable_section_index = config.get_attribute("votable_section_index_name");
    _open_index = config.get_attribute("open_index_name");
    _tentative = config.get_attribute("tentative_name");
    _unresolved = config.get_attribute("unresolved_name");
    _immediate = config.get_attribute("immediate_name");
    _issues_for_editor = config.get_attribute("issues_for_editor_name");
  }
}
