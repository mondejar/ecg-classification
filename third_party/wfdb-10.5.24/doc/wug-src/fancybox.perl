# fancybox.perl by George Moody (george@mit.edu) 26 May 1999
#
# Extension to LaTeX2HTML V98.1 to support the "fancybox.sty" package

package main;

&process_commands_in_tex (<<_RAW_ARG_CMDS_);
shadowbox # {}
doublebox # {}
ovalbox # {}
Ovalbox # {}
_RAW_ARG_CMDS_

&ignore_commands(<<_IGNORED_CMDS_);
cornersize # {}
_IGNORED_CMDS_

1;	# Must be last line
