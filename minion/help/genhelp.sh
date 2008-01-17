#!/bin/bash

CODE_ROOT=$1;
TMP_FILE="entries.tmp";
SORTED_TMP="sorted.tmp";
PREFIX_TMP="prefixes.tmp";
SORTED_PREF="sortedprefs.tmp";
touch $TMP_FILE;
rm $TMP_FILE; #ensure empty
touch $TMP_FILE;
touch $PREFIX_TMP;
rm $PREFIX_TMP;
touch $PREFIX_TMP;

dashes() {
    for ((i=1;i<=$1;i+=1)); do echo -n -; done
}

# a piece of code to output a C function based on help code embedded in
# source code

echo "#include <iostream>";
echo "#include <string>";
echo "using namespace std;";
echo "#define NEWLINE '\n'"
echo "void help(string request)";
echo "{";
previous_entry=hjkhasdkjfhsdkbfs76f87sdf; #doesn't match any entry
firsttime=true;
find . \( ! -regex '.*/\..*' \) -iname "*.cpp" -or -iname "*.hpp" -or -iname "*.h" -type f -exec grep -H -n "/\*\* @help" {} \; | while read entry ; do
    match_file=`echo $entry | cut -d: -f1`; #file comment is in
    match_line=`echo $entry | cut -d: -f2`; #line comment begins on
    match_entry=`echo $entry | cut -d' ' -f3`; #entry comment is for
    if [ $firsttime = "false" ]; then
	if [ $previous_entry != $match_entry ]; then
	    echo "} else";
	fi
    fi
    firsttime=false;
    match_heading=`echo $entry | cut -d' ' -f 4`; #heading comment is for
    lines_in_file=`wc -l $match_file | awk '{print $1}' | cut -d' ' -f1`; #lines in match file
    reqd_lines=$(($lines_in_file-$match_line))
    end_of_comment_in_tail=`tail -n$reqd_lines $match_file | grep -m1 -n "\*/" | cut -d':' -f1`;
    end_of_comment=$(($match_line+$end_of_comment_in_tail)); #line comment ends on
    first_l=$(($match_line+1)); #line body starts on
    last_l=$(($end_of_comment-1)); #line body ends on
    body=`sed -n $first_l,"$last_l"p $match_file`; #body of entry
    match_entry_spaces=`echo $match_entry | sed 's/;/ /g'`;
    echo $match_entry_spaces >> $TMP_FILE #record entry for later
    if [ $previous_entry != $match_entry ]; then
	echo "if(\"$match_entry_spaces\" == request) {";
	echo "cout << \"Help entry: \" << \"$match_entry_spaces\" << NEWLINE << NEWLINE;";
    fi
    echo "cout << \"$match_heading\" << \"`dashes $((80-${#match_heading}))`\" << NEWLINE;";
    OLDIFS=$IFS;
    IFS=' ';
    #multiline c string, add << and quotes
    body_for_c=`echo $body | sed 's/^/<< "/g' | sed 's/$/" << NEWLINE/g'`;
    IFS=$OLDIFS;
    echo "cout $body_for_c << NEWLINE << NEWLINE;";
    previous_entry=$match_entry; #remember this entry next time
done
echo "} else";
echo 'cout << "Unknown entry, please try again." << NEWLINE;'; #final else body

cat $TMP_FILE | while read entry; do
    words=`echo $entry | wc -w | awk '{print $1}'`; #number of words in entry
    if [ $words = 1 ]; then
	echo >> $PREFIX_TMP;
    else
	pref_len=$(($words-1)); #words in prefix
	prefix=`echo $entry | cut -d' ' -f-$pref_len`; #all but last word
	echo $prefix >> $PREFIX_TMP; #store for next pass
    fi
done
sort $TMP_FILE | uniq > $SORTED_TMP;
sort $PREFIX_TMP | uniq > $SORTED_PREF; #obtain sorted list of unique prefixes
cat $SORTED_PREF | while read outerentry; do #loop over prefixes
    none_printed=true;
    cat $SORTED_TMP | while read innerentry; do #hunting for things it's a prefix of
	words=`echo $innerentry | wc -w | awk '{print $1}'`;
	pref_len=$(($words-1));
	if [ $words = 1 ]; then
	    prefix="";
	else
	    prefix=`echo $innerentry | cut -d' ' -f-$pref_len`;
	fi
	if [ "$prefix" = "$outerentry" ]; then
	    if [ $none_printed = true ]; then
		none_printed=false;
		echo "if(\"$outerentry\" == request) {";
		echo "cout << \"Available subentries:\" << NEWLINE;";
	    fi
	    echo "cout << \"$innerentry\" << NEWLINE;";
	fi
    done
    echo "} else";
done
echo ";"; #null statement to occupy final else branch
echo '}'; #end of function
