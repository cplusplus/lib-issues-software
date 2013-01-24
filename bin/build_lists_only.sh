#!/bin/sh
bin/lists && test -f mailing/lwg-active.html && exit
echo ***********************************
echo ********** build failure **********
echo ***********************************
exit 1
