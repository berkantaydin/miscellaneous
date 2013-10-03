#!/usr/bin/perl
use strict;
use warnings;

sub fix_dir
{
	opendir(my $dir, '.');
	while (my $file = readdir($dir)) {
		if ($file =~ /^\./) {
			next;
		}

		if (-d "$file") {
			chdir("$file");
			fix_dir('.');
			chdir('..');
		}

		my $oldname = $file;
		$file =~ s/[\:\;\*\?\"\<\>\|]//g;
		if (not $oldname eq $file) {
			rename("$oldname","$file");
		}
	}
	closedir($dir);
}

fix_dir();
