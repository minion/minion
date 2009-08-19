#!/bin/bash

CODE_ROOT=$1;
TMP_FILE="entries.tmp";
SORTED_TMP="sorted.tmp";
rm -f $TMP_FILE; #ensure empty
touch $TMP_FILE;
rm -f docs/latexhelp/*;

# a piece of code to output a LaTeX document based on help code
# embedded in source code

# it creates a file for each entry, and appends each section into the
# relevant file as it encounters them in the source comments

# finally outputs a document by compiling the individual fragments in a sorted
# order

# the generated document is meant to be included in another LaTeX document

find . \( ! -regex '.*/\..*' \) \( -iname "*.cpp" -or -iname "*.hpp" -or -iname "*.h" \) -type f -exec grep -H -n "/\*\* @help" {} \; | while read entry ; do
    match_file=`echo $entry | cut -d: -f1`; #file comment is in
    match_line=`echo $entry | cut -d: -f2`; #line comment begins on
    match_entry=`echo $entry | cut -d' ' -f3`; #entry comment is for
    match_heading=`echo $entry | cut -d' ' -f 4`; #heading comment is for
    lines_in_file=`wc -l $match_file | awk '{print $1}' | cut -d' ' -f1`; #lines in match file
    reqd_lines=$(($lines_in_file-$match_line))
    end_of_comment_in_tail=`tail -n$reqd_lines $match_file | grep -m1 -n "\*/" | cut -d':' -f1`;
    end_of_comment=$(($match_line+$end_of_comment_in_tail)); #line comment ends on
    first_l=$(($match_line+1)); #line body starts on
    last_l=$(($end_of_comment-1)); #line body ends on
    body=`sed -n $first_l,"$last_l"p $match_file`; #body of entry
    processed_body=$body;
    match_entry_spaces=`echo $match_entry | sed 's/;/ /g'`;
    match_entry_underbars=`echo $match_entry | sed 's/;/_/g'`;
    echo $match_entry_spaces >> $TMP_FILE #record entry for later
    outfile="docs/latexhelp/$match_entry_underbars.frag";
    echo "\paragraph{$match_heading}" >> $outfile;
    echo "{\footnotesize" >> $outfile;
    echo "\begin{verbatim}" >> $outfile;
    echo "$processed_body" >> $outfile;
    echo "\end{verbatim}" >> $outfile;
    echo "}" >> $outfile;
done
outfile="docs/latexhelp/doc.tex";
cat $TMP_FILE | sort | uniq > $SORTED_TMP;
cat $SORTED_TMP | while read entry; do
    entry_underbars=`echo $entry | sed 's/ /_/g'`;
    entry_latexformatted=`echo $entry | sed 's/_/\\\\textunderscore /g'`;
    echo "\section{$entry_latexformatted}" >> $outfile;
    cat "docs/latexhelp/$entry_underbars.frag" >> $outfile;
done

rm docs/latexhelp/*.frag;
