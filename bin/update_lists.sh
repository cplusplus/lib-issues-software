#!/bin/sh
# WARNING This batch file assumes a checkout of the gh-pages branch,
# in a directory named issues-gh-pages, at the same level as the master branch checkout
pushd ../issues-gh-pages
git pull
popd
cp -f mailing/lwg-*.html ../issues-gh-pages
cp -f mailing/unresolved-*.html ../issues-gh-pages
cp -f mailing/votable-*.html ../issues-gh-pages
pushd ../issues-gh-pages
git commit -a -m"Update"
git push  "origin" gh-pages:gh-pages
popd
