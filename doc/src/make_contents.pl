#!/usr/bin/perl
# above is magic first line to invoke perl
#
use warnings;
use strict;
# The dox Makefile makes contents and index automatically
# 
# To run it by hand :
#    cd  to src directory (contains this file)
# make_contents.pl 
#   ...  this script generates level_info.txt
# next run doit.pl to generate contents (Organization.dox)
#
# NOTE: this file automatically delete the old Organization.dox and docindex.dox 
#       because it is going to create new ones
#
# To generate alphabetical index by hand, run make_index.pl
#
#
our $debug =0;
# our $idxd;
our %hashpages;
our %hashpagelevel;
our $sortfile;
our  $section_next_column = 6; # Contents page formatting - Section 6 will start the second column
                               # can only go to column 2 between two sections !!
                               # set to -1 for one column only
               
$|=1; # flush output buffers

require "./contents.pl"; # subroutines to make the contents page are in this file
require "./common_subs.pl";     # common subroutines between this file and idx.pl

my $level;
my $done;

print "make_contents is starting\n";

# main dox Makefile does this : 
#
# 1. makes contents list, Organization.dox
#       using make_contents.pl; doit.pl;
# 2. makes alphabetical index,  docindex.dox
#       using make_index.pl
# 3. deletes all temporary files generated by these scripts



print "\n make_contents.pl starting... checking for Organization.dox and docindex.dox ...\n";
create_dummy_files(); # deletes Organization.dox, docindex.dox and makes dummies for mine.pl (will be overwritten later);

print "\n make_contents.pl         ... mining files to make Organization.dox page\n";
mine(); # mines dox files to produce mined_info.txt

fill_page_hash(); # finds pages and mainpage in mined_info.txt
print "\n hashpages....\n";    
for(keys %hashpages) 
{ print  "Page $_ Level $hashpages{$_}\n";}

mainpage(); # fills sorted_info.txt.0 with mainpage
$level=1;
$done=1;
while ($done)
{
    if($level > 10)
    { 
        print "make_contents: stopping at level 10\n";
        die   "      may be something wrong - if not, you need to increase the maximum level\n";
    }

    print "Calling subpage with level=$level\n";
    $done = subpage($level); # replaces subpages with pages
    $level++;
}

for(keys %hashpagelevel) {
    print "Page $_ is at level  $hashpagelevel{$_}\n";}

print "done at level $level\n";
$level--;
copy ($sortfile.".".$level, $sortfile);
print "renamed file $sortfile.$level to  $sortfile\n";
write_levels();
#
print "\n hashpages....\n";    
for(keys %hashpages) 
{ print  "Page $_ Level $hashpages{$_}\n";}
print "NOTE: \n";
print "The only subpages that should be listed above between \"hashpages...\" and \"NOTE:\"\n" ;
print "are the list of Sections (Intro to Organization) at level 0 \n\n";
print "Any extra pages listed above will likely not be listed in the contents or appear in the documentation\n";
print "because they are not listed in any dox file  as a \\subpage (which they should be!!)  \n\n";
print "Continue by running doit.pl to produce Organization.dox\n";


exit;