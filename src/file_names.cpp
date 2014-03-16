#include "file_names.h"

namespace lwg {
  file_names::file_names(const std::string& prefix)
  {
    _prefix = prefix;
    _active = prefix + "active.html";
    _closed = prefix + "closed.html";
    _defects = prefix + "defects.html";
    _toc = prefix + "toc.html";
    _old_toc = prefix + "old-toc.html";
    _status_index = prefix + "status-index.html";  // not "index.html" to avoid name clash
    _status_date_index = prefix + "status-date-index.html";
    _section_index = prefix + "section-index.html";
    _unresolved_toc = prefix + "unresolved-toc.html";
    _unresolved_status_index = prefix + "unresolved-status-index.html";
    _unresolved_status_date_index = prefix + "unresolved-status-date-index.html";
    _unresolved_section_index = prefix + "unresolved-section-index.html";
    _unresolved_prioritized_index = prefix + "unresolved-prioritized-index.html";
    _votable_toc = prefix + "votable-toc.html";
    _votable_status_index = prefix + "votable-status-index.html";
    _votable_status_date_index = prefix + "votable-status-date-index.html";
    _votable_section_index = prefix + "votable-section-index.html";
    _open_index = prefix + "open-index.html";
    _tentative = prefix + "tentative.html";
    _unresolved = prefix + "unresolved.html";
    _immediate = prefix + "immediate.html";
    _issues_for_editor = prefix + "issues-for-editor.html";
  }
}
