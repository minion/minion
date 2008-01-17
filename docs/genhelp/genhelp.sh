#!/bin/bash

CODE_ROOT=$1;
TMP_FILE="entries.tmp";
SORTED_TMP="sorted.tmp";
touch $TMP_FILE;
rm $TMP_FILE; #ensure empty
touch $TMP_FILE;
rm docs/htmlhelp/*.html;

dashes() {
    for ((i=1;i<=$1;i+=1)); do echo -n -; done
}

#start an HTML page
makepage() {
    outfile=$1; #page to start
    title=$2; #title of page
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" >> $outfile;
    echo "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" >> $outfile;
    echo "<html xmlns=\"http://www.w3.org/1999/xhtml\">" >> $outfile;
    echo "<head>" >> $outfile;
    echo -n "<title>" >> $outfile; echo -n $2 >> $outfile; echo "</title>" >> $outfile;
    echo "</head>" >> $outfile;
    echo "<body>" >> $outfile;
    echo "<h1>Help on <tt>$title</tt></h1>" >> $outfile;
}

#only start an HTML page if it doesn't already exist
querymakepage() {
    outfile=$1
    title=$2
    if [ ! -e $outfile ]; then #file doesn't exist
	makepage $outfile "$title";
    fi	
}

#end an HTML page
endpage() {
    outfile=$1;
    echo "<p><a href=\"index.html\">Home</a></p>" >> $outfile;
    echo "</body>" >> $outfile;
    echo "</html>" >> $outfile;
}

# a piece of code to output a set of HTML pages based on help code
# embedded in source code

# it creates a file for each entry, and appends each section into the
# relevant file as it encounters them in the source comments

# finally outputs an index page from which the entries can be accessed

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
    html_body=`echo "$body" | sed 's/</\&lt;/g' | sed 's/>/\&gt;/g'`; #replace < by &lt;, etc.
    match_entry_spaces=`echo $match_entry | sed 's/;/ /g'`;
    match_entry_underbars=`echo $match_entry | sed 's/;/_/g'`;
    echo $match_entry_spaces >> $TMP_FILE #record entry for later
    outfile="docs/htmlhelp/$match_entry_underbars.html";
    querymakepage $outfile "$match_entry_spaces"; #make page for entry if necessary
    echo "<h3>$match_heading</h3>" >> $outfile;
    echo "<pre>" >> $outfile;
    echo "$html_body" >> $outfile;
    echo "</pre>" >> $outfile;
done
for i in docs/htmlhelp/*.html; do #end pages
    endpage $i;
done
outfile="docs/htmlhelp/index.html";
makepage $outfile "minion"
# add some information to the top of the index page.
echo "<p>You are viewing documentation for minion. The same " >> $outfile;
echo "documentation is available from a minion executable by " >> $outfile;
echo "typing <tt>minion help</tt> at the command line." >> $outfile;
echo "We intend that the command line help system be the " >> $outfile;
echo "main source of documentation for the system.<p>" >> $outfile;
echo "<p>Each of the entries below concerns a different aspect" >> $outfile;
echo "of the system, and the entries are arranged hierarchically." >> $outfile;
echo "For example to view information about the set of available" >> $outfile;
echo "constraints as a whole view \"constraints\" and to view" >> $outfile;
echo "specific information about the alldiff constraint view " >> $outfile;
echo "\"constraints alldiff\".</p>" >> $outfile;
echo >> $outfile;
echo "<p>A good place to start would be viewing the " >> $outfile;
echo "\"input example\" entry which exhibits a complete" >> $outfile; 
echo "example of a minion input file.<p>" >> $outfile;
echo "<p>Usage: minion [switches] [minion input file]</p>" >> $outfile;
cat $TMP_FILE | sort | uniq > $SORTED_TMP;
cat $SORTED_TMP | while read entry; do
    entry_underbars=`echo $entry | sed 's/ /_/g'`;
    echo "<a href=\"$entry_underbars.html\">$entry</a><br />" >> $outfile;
done
endpage $outfile;
   