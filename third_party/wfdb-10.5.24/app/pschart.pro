% file: pschart.pro	G. Moody	  27 May 1988
%			Last revised:	 11 August 2005

% -----------------------------------------------------------------------------
% prolog for pschart output
% Copyright (C) 2000 George B. Moody

% This program is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free Software
% Foundation; either version 2 of the License, or (at your option) any later
% version.

% This program is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
% FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
% more details.

% You should have received a copy of the GNU General Public License along with
% this program; if not, write to the Free Software Foundation, Inc., 59 Temple
% Place - Suite 330, Boston, MA 02111-1307, USA.

% You may contact the author by e-mail (george@mit.edu) or postal mail
% (MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
% please visit PhysioNet (http://www.physionet.org/).
% _____________________________________________________________________________

% Note: lines beginning with `%' and empty lines in this file are stripped by
% `pschart' before printing.  Four lines immediately below begin with spaces
% and are not stripped.

 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 % If this message appears in your printout, you may be using a buggy version %
 % of Adobe TranScript. Try using pschart with the -u option as a workaround. %
 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

save 100 dict begin /pschart exch def

% printer resolution (dots per inch; reset by newpage, see below)
/dpi 300 def

% x mm: convert x (in mm) to PostScript points (assuming default scales)
/mm {72 mul 25.4 div}def

% line width in mm for signals, grid lines, marker bars
/lwmm 0.2 def

% line width in PostScript points (reset by newpage)
/lw lwmm mm def

% TM for text
/tm matrix currentmatrix def

% TM for graphics
/gm matrix currentmatrix def

% n I: use italic font, n points
/I {/Times-Italic findfont exch scalefont setfont}def

% n R: use roman font, n points
/R {/Times-Roman findfont exch scalefont setfont}def

% n C: use courier (monospaced) font, n points
/C {/Courier findfont exch scalefont setfont}def

% x0 y0 x1 y1 xtick ytick grid: print a grid with the lower corner at (x0, y0),
%  the upper corner at (x1, y1), and ticks at intervals of xtick and ytick (in
%  mm);  the units of x0, y0, x1, and y1 are printer coordinates.
/grid { newpath 0 setlinecap
 /dy1 exch dpi 25.4 div mul lw sub def /dy2 dy1 lw add 5 mul def
 /dx1 exch dpi 25.4 div mul lw sub def /dx2 dx1 lw add 5 mul def
 /y1 exch def /x1 exch def /y0 exch def /x0 exch def
 dpi 100 idiv setlinewidth x0 y0 moveto x1 y0 lineto x1 y1 lineto x0 y1 lineto
 closepath stroke lw setlinewidth [lw dx1] 0 setdash
 y0 dy2 add dy2 y1 {newpath dup x0 exch moveto x1 exch lineto stroke}for
 [lw dy1] 0 setdash
 x0 dx2 add dx2 x1 {newpath dup y0 moveto y1 lineto stroke }for
 [] 0 setdash
}bind def

% alternate grid
/Grid { newpath 0 setlinecap
 /dy1 exch dpi 25.4 div mul lw sub def /dy2 dy1 lw add 5 mul def
 /dx1 exch dpi 25.4 div mul lw sub def /dx2 dx1 lw add 5 mul def
 /y1 exch def /x1 exch def /y0 exch def /x0 exch def
 dpi 100 idiv setlinewidth x0 y0 moveto x1 y0 lineto x1 y1 lineto x0 y1 lineto
 closepath stroke lw setlinewidth
 y0 dy2 add dy2 y1 {newpath dup x0 exch moveto x1 exch lineto stroke}for
 x0 dx2 add dx2 x1 {newpath dup y0 moveto y1 lineto stroke }for
}bind def

% pn x y prpn: print page number, pn, centered on (x, y) (in mm).
/prpn { mm exch mm exch moveto 10 R /pn exch def /str 10 string def
 pn str cvs stringwidth exch -.5 mul exch rmoveto
 (- ) stringwidth exch neg exch rmoveto
 (- ) show
% `str show' should be sufficient, but LaserWriters print extra spaces at end
 pn str cvs show
 ( -) show } def

% str x y prco: print copyright message, str, beginning at (x, y) (in mm).
/prco { mm exch mm exch moveto
 6 R (Copyright ) show
 /Symbol findfont 6 scalefont setfont (\323) show
 6 R show } def

% dpi newpage: start page, resolution dpi dots per inch
/newpage {/dpi exch def tm setmatrix newpath [] 0 setdash
 1 setlinecap /lw lwmm mm def mark } def

% ss: set scales for plotting
/ss {72 dpi div dup scale /gm matrix currentmatrix def lw setlinewidth} def

% str t: print str starting at current position (label(str))
/t {tm setmatrix show gm setmatrix}def

% str b: print str ending at current position (rlabel(str))
/b {tm setmatrix dup stringwidth exch neg exch rmoveto currentpoint 3 -1 roll
 show moveto gm setmatrix}def

% x y m: set current position to (x,y) (move(x,y))
/m {newpath moveto}def

% x y N: draw line segment from current position to (x,y), set current position
%  to (x,y) (cont(x,y))
/N {rlineto currentpoint stroke moveto}bind def

% str z: decode str and draw connected line segments (see cont() in pschart.c)
/z {{counttomark 2 ge{79 sub rlineto}{126 exch sub}ifelse}forall
 currentpoint stroke moveto}bind def

% default ordinate for annotation plotting
/ay 0 def

% y Ay: set ay
/Ay {/ay exch def}def

% ordinates for marker bars
/ya 0 def
/yb 0 def
/yc 0 def
/yd 0 def

% ya yb yc yd sb: set ordinates for marker bars
/sb {/yd exch def /yc exch def /yb exch def /ya exch def}def

% ya yb Sb: set ordinates for short marker bars
/Sb {/yb exch def /ya exch def /yc yb def /yd yb def}def

% x mb: draw marker bars at x
/mb { dup ya newpath moveto dup yb lineto dup yc moveto yd lineto
stroke}bind def

% str x a: plot str at (x, ay), with marker bars if defined
/a {ya yb ne {dup mb}if ay m t}bind def

% x A: plot a bullet (normal QRS annotation) at (x, ay)
/A {ya yb ne {dup mb}if ay m (\267) t}bind def

/endpschart {cleartomark showpage pschart end restore}def
