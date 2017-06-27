% file: psfd.pro	G. Moody	10 August 1988
%			Last revised:	11 August 2005

% -----------------------------------------------------------------------------
% prolog for psfd output
% Copyright (C) 1988-2005 George B. Moody

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
% `psfd' before printing.  Four lines immediately below begin with spaces and
% are not stripped.

 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 % If this message appears in your printout, you may be using a buggy version %
 % of Adobe TranScript.  Try using psfd with the -u option as a workaround.   %
 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

save 100 dict begin /psfd exch def

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

% n S: use sans-serif font, n points
/S {/Helvetica findfont exch scalefont setfont}def

% x0 y0 x1 y1 nt grid: print a grid with the lower corner at (x0, y0),
%  the upper corner at (x1, y1), and nt+1 ticks at equal intervals between x0
%  and y0;  the units of x0, y0, x1, and y1 are printer coordinates.
/grid { /nt exch def /y1 exch def /x1 exch def /y0 exch def /x0 exch def
%  calculate tick spacing, dx, in printer units
 /dx x1 x0 sub nt div def
%  calculate tick lengths, dy1 = 2 mm (major ticks), dy2 = 1 mm (minor ticks)
 /dy1 dpi 12.7 div def /dy2 dpi 25.4 div def
%  draw axes
 x0 y0 moveto x1 y0 lineto x0 y1 moveto x1 y1 lineto stroke
%  draw ticks.  `cvi' below is needed for NeWSprint but not Apple LaserWriter.
 0 1 nt cvi { dup 5 mod 0 eq {/dy dy1 def} {/dy dy2 def} ifelse
  dx mul x0 add /xx exch def newpath
  xx y0 moveto xx y0 dy sub lineto
  xx y1 moveto xx y1 dy add lineto stroke }for
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
/newpage {/dpi exch def tm setmatrix newpath [] 0 setdash 0 setgray
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

% str z: decode str and draw connected line segments (see cont() in psfd.c)
/z {{33 sub dup 1 and exch 2 idiv 23 sub rlineto}forall
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
/ye 0 def

% ya yb yc yd sb: set ordinates for marker bars
/sb {/yd exch def /yc exch def /yb exch def /ya exch def
 /ye dpi 50.8 div lw sub def}def

% ya yb Sb: set ordinates for short marker bars
/Sb {/yb exch def /ya exch def /yc yb def /yd yb def
 /ye dpi 50.8 div lw sub def}def

% x mb: draw marker bars at x
/mb { dup ya newpath moveto dup yb lineto dup yc moveto yd lineto
 [lw ye] 0 setdash stroke [] 0 setdash}bind def

% str x a: plot str at (x, ay), with marker bars if defined
/a {ya yb ne {dup mb}if ay m t}bind def

% x A: plot a bullet (normal QRS annotation) at (x, ay)
/A {ya yb ne {dup mb}if ay m (\267) t}bind def

/endpsfd {cleartomark showpage psfd end restore}def
