#!/bin/sh env awk
BEGIN {
    CONVFMT="%2.1f";
    print "| Sample                 |showdown  |commonmark|marked    |markdown-it|"
    print "|------------------------|---------:|---------:|---------:|----------:|"
}
{
        if (/samples\//) {
                sub(/samples\//, "");
                printf "|%-24s|", $7;
        } else if (/^showdown/) {
                sub(/,/, "");
                showdown = $4;
        } else if (/^commonmark/) {
                sub(/,/, "");
                commonmark = $4;
        } else if (/^marked/) {
                sub(/,/, "");
                marked = $4;
        } else if (/^markdown-it/) {
                sub(/,/, "");
                markdownit = $4;
                printf "%10s|%10s|%10s|%11s|\n",
                       (showdown / showdown),
                       (commonmark / showdown),
                       (marked / showdown),
                       (markdownit / showdown);
                markdownit = "";
                showdown = "";
                marked = "";
                commonmark = "";
        } else {
                next;
        }
}
