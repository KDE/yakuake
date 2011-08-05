#!/usr/bin/perl

use strict;

my($gotgroup);

while (<>)
{
    if ($_ =~ /^\[Dialogs\]/)
    {
        $gotgroup = 1;
    }
    elsif ($_ =~ /^ShowPopup=false$/ && $gotgroup)
    {
        print "[Event/startup]\n";
        print "Action=\n";
        print "# DELETE [Dialogs]ShowPopup\n";
    }
}
