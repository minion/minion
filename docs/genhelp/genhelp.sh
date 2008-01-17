#!/bin/bash

CODE_ROOT=$1;
TMP_FILE="entries.tmp";
SORTED_TMP="sorted.tmp";
touch $TMP_FILE;
rm $TMP_FILE; #ensure empty
touch $TMP_FILE;

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
    outfile="/tmp/$match_entry_underbars.html";
    querymakepage $outfile "$match_entry_spaces"; #make page for entry if necessary
    echo "<h3>$match_heading</h3>" >> $outfile;
    echo "<pre>" >> $outfile;
    echo "$html_body" >> $outfile;
    echo "</pre>" >> $outfile;
done
for i in /tmp/*.html; do #end pages
    endpage $i;
done
outfile="/tmp/index.html";
makepage $outfile "minion"
cat $TMP_FILE | sort | uniq > $SORTED_TMP;
cat $SORTED_TMP | while read entry; do
    entry_underbars=`echo $entry | sed 's/ /_/g'`;
    echo "<a href=\"/tmp/$entry_underbars.html\">$entry</a><br />" >> $outfile;
done
endpage $outfile;
    

# cat $TMP_FILE | while read entry; do
#     words=`echo $entry | wc -w | awk '{print $1}'`; #number of words in entry
#     if [ $words = 1 ]; then
# 	echo >> $PREFIX_TMP;
#     else
# 	pref_len=$(($words-1)); #words in prefix
# 	prefix=`echo $entry | cut -d' ' -f-$pref_len`; #all but last word
# 	echo $prefix >> $PREFIX_TMP; #store for next pass
#     fi
# done
# sort $TMP_FILE | uniq > $SORTED_TMP;
# sort $PREFIX_TMP | uniq > $SORTED_PREF; #obtain sorted list of unique prefixes
# cat $SORTED_PREF | while read outerentry; do #loop over prefixes
#     none_printed=true;
#     cat $SORTED_TMP | while read innerentry; do #hunting for things it's a prefix of
# 	words=`echo $innerentry | wc -w | awk '{print $1}'`;
# 	pref_len=$(($words-1));
# 	if [ $words = 1 ]; then
# 	    prefix="";
# 	else
# 	    prefix=`echo $innerentry | cut -d' ' -f-$pref_len`;
# 	fi
# 	if [ "$prefix" = "$outerentry" ]; then
# 	    if [ $none_printed = true ]; then
# 		none_printed=false;
# 		echo "if(\"$outerentry\" == request) {";
# 		echo "cout << \"Available subentries:\" << NEWLINE;";
# 	    fi
# 	    echo "cout << \"help $innerentry\" << NEWLINE;";
# 	fi
#     done
#     echo "} else";
# done
# echo ";"; #null statement to occupy final else branch
# echo '}'; #end of function
