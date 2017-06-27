: file: makeinfo.sh	G. Moody	  24 June 1989
:			Last revised:	6 August 1995
: Generate GNU emacs info files using GNU emacs

: Copyright[C] Massachusetts Institute of Technology 1995. All rights reserved.

emacs -batch $1 >/dev/null -f texinfo-format-buffer -f save-buffer
exit 0

: Use the stand-alone GNU makeinfo utility if it is available, instead of this
: script.  The stand-alone utility is _much_ faster, and it provides better
: diagnostics.

: This version works with GNU emacs versions 18.57 and later, including version
: 19.x., and should work with earlier versions as well.  If it fails, change
: save-buffer to save-buffers-kill-emacs above.  save-buffers-kill-emacs used
: to work in batch mode, but emacs version 18.57 seems to break it, at least on
: SPARCs.

: In at least some cases, texinfo-format-buffer does not work properly in
: interactive mode in an X window.  The problem in these cases is that emacs
: cannot identify and write to the controlling terminal.  Using batch mode, as
: in this script, this problem does not occur.

