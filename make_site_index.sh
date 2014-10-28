#!/bin/sh

SPECVERSION=$1
SITE=_site
VERSIONS=`cd $SITE; ls -d -1 0.* | sort -r -g`

echo "% CommonMark Spec\n"
date=`grep '<div class="version">' $SITE/$SPECVERSION/index.html | perl  -pe 's/^.*(\d\d\d\d-\d\d-\d\d).*$/\1/'`
echo "[**Latest version ($SPECVERSION)** ($date)](/$SPECVERSION/)\n"
echo "Older versions:\n"
for vers in $VERSIONS
  do
    date=`grep '<div class="version">' $SITE/$vers/index.html | perl  -pe 's/^.*(\d\d\d\d-\d\d-\d\d).*$/\1/'`
    if [ "$vers" != "$SPECVERSION" ]; then
	perl -p -i -e 's/<div id="watermark">.*?<\/div>/<div id="watermark" style="background-color:black">This is an older version of the spec. For the most recent version, see <a href="http:\/\/spec.commonmark.org">http:\/\/spec.commonmark.org<\/a>.<\/div>/' $SITE/$vers/index.html
        echo "- [$vers ($date)](/$vers/)"
    fi
  done
