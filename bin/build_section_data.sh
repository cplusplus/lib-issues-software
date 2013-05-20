#!/bin/sh
grep -v "^[A-Z]" <bin/annex-f | grep -v "^[0-9]" | grep -v "ISO/IEC" | grep -v '^$' >bin/index
bin/section_data <bin/index >bin/section.data
cat meta-data/tr1_section.data >>bin/section.data
ls -l bin/section.data
