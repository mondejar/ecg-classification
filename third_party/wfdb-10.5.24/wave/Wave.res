! file: Wave.res		G. Moody	   April 1990
!				Last revised:	13 December 2004
! X11 resources for WAVE
!
! You can control many aspects of WAVE's appearance and behavior by setting
! its X11 resources.  This file illustrates how each of the WAVE-specific
! resources can be set.  To make these settings effective, the contents of
! this file should be included in a file named .Xdefaults in your home
! directory.  If you use other X11 applications, this file may exist already,
! and it may contain resources to be shared by some or all X11 applications.
! For this reason, you should be careful not to replace .Xdefaults when you
! add material from this file to it.
!
! This file can also be installed as /usr/lib/X11/app-defaults/Wave (and in
! this location, its contents can be shared by all users).

! If you make changes in .Xdefaults or /usr/lib/X11/app-defaults/Wave while
! WAVE is running, you will need to restart WAVE in order to see the effects
! of your changes.  It may also be necessary to force the resource database
! to be reread, either by logging out and in again, or using a command such as:
!	xrdb -merge ~/.Xdefaults

! -----------------------------------------------------------------------------

! Here are two WAVE resources that most users will want to set (although you
! may prefer a different setting than those shown here).

Wave*background:	white
Wave.SignalWindow.font:	-*-lucida-bold-r-normal-sans-14-*
! If WAVE complains it can't find the font above, try this one instead:
!	Wave.SignalWindow.font: lucidasans-bold-14
! (or use xlsfonts to find an alternative that you like).


! -----------------------------------------------------------------------------

! The remainder of this file contains a listing of all WAVE-specific X11
! resources, illustrating their default settings.  For further information,
! see 'X11 resources for WAVE' in the WAVE User's Guide.  If you wish to change
! any of the defaults, remove the 'comment.' at the beginning of the
! appropriate line, and change the setting as desired.  WAVE should be tolerant
! of most incorrect settings, but it is possible that changing something below
! may make WAVE temporarily unusable.  If this happens, simply undo the last
! change(s) you made and run WAVE again.
!
! Since WAVE is built using the XView toolkit, its appearance and behavior can
! also be modified using many of the generic XView X11 resources; for details,
! see http://www.physionet.org/physiotools/wag/xview-7.htm .
!
! Note that many X11 applications rewrite .Xdefaults if the user changes
! preferences within the application, and that any lines beginning with '!'
! will be removed when this happens.  WAVE itself does this when you use
! the 'Save as new defaults' button in the 'View' panel.  A simple way to
! comment out settings in .Xdefaults without risking their removal is to
! prefix the resource name with 'comment.', as below.

comment.Wave.AllowDottedLines:			true
comment.Wave.Anntab:				/path/to/anntab
comment.Wave.Dpi:				100x100
comment.Wave.GraphicsMode:			8
comment.Wave.Scope.Color.Background:		white
comment.Wave.Scope.Color.Foreground:		blue
comment.Wave.Scope.Grey.Background:		white
comment.Wave.Scope.Grey.Foreground:		black
comment.Wave.Scope.Mono.Background:		white
comment.Wave.SignalWindow.Color.Annotation:	yellow green
comment.Wave.SignalWindow.Color.Background:	white
comment.Wave.SignalWindow.Color.Cursor:		orange red
comment.Wave.SignalWindow.Color.Grid:		grey90
comment.Wave.SignalWindow.Color.Signal:		blue
comment.Wave.SignalWindow.Font:			fixed
comment.Wave.SignalWindow.Grey.Annotation:	grey25
comment.Wave.SignalWindow.Grey.Background:	white
comment.Wave.SignalWindow.Grey.Cursor:		grey50
comment.Wave.SignalWindow.Grey.Grid:		grey75
comment.Wave.SignalWindow.Grey.Signal:		black
comment.Wave.SignalWindow.Height_mm:		120
comment.Wave.SignalWindow.Mono.Background:	white
comment.Wave.SignalWindow.Width_mm:		250
comment.Wave.TextEditor:			textedit
comment.Wave.View.AmplitudeScale:		3
comment.Wave.View.AnnotationMode:		0
comment.Wave.View.AnnotationOverlap:		0
comment.Wave.View.Aux:				false
comment.Wave.View.Baselines:			false
comment.Wave.View.Chan:				false
comment.Wave.View.CoarseGridMode:		5
comment.Wave.View.CoarseTimeScale:		5
comment.Wave.View.GridMode:			3
comment.Wave.View.Level:			false
comment.Wave.View.Markers:			false
comment.Wave.View.Num:				false
comment.Wave.View.SignalNames:			false
comment.Wave.View.Subtype:			false
comment.Wave.View.TimeMode:			0
comment.Wave.View.TimeScale:			12
