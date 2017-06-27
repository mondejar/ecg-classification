C       lgraf.f

C       -----------------------------------------------------------------------
C       Copyright (C) 2002 Pablo Laguna
C
C       This program is free software; you can redistribute it and/or modify it
C       under the terms of the GNU General Public License as published by the
C       Free Software Foundation; either version 2 of the License, or (at your
C       option) any later version.
C
C       This program is distributed in the hope that it will be useful, but
C       WITHOUT ANY WARRANTY; without even the implied warranty of
C       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
C       General Public License for more details.
C
C       You should have received a copy of the GNU General Public License along
C       with this program; if not, write to the Free Software Foundation, Inc.,
C       59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
C
C       You may contact the author by e-mail (laguna@posta.unizar.es) or postal
C       mail (Dpt. Ingenieria Electrónica y Comunicaciones, Grupo de Tecnología
C       de las Comunicaciones (GTC), Centro Politécnico Superior. Universidad
C       de Zaragoza, María de Luna, 3 (Pol. Actur), Edificio A, Despacho 3.16,
C       50015 Zaragoza. Spain).  For updates to this software, please visit
C       PhysioNet (http://www.physionet.org/).
C       _______________________________________________________________________

C RUTINES ORIGINALS:
C Function "prinplot", "printin" to be used when outputing
C to the laser-writer in postscript mode.		
C Author.: J.Juan					
C Version.: 1.0					
C Date.: 2/25/86					
C Usage.: Link as they were the standar fortran rutines.

C---------------------------------------------------------------------
C            Adaptacio:  1-oct-87   Raimon Jane.
C            Rutines L_PRINPLOT i L_PRINTINT, cridades des de
C            PRINLASER (VTGRAF.EXE)
C            Es troven a la llibreria RGRAF.
C                  Presentan una compatibilitat amb les PRINPLOT i
C                  L_PRINTINT per a la PRINTRONIX. Per tant els eixos
C                  tenen el mateix conveni de signes.
C                  Es per aixo que L_PRINPLOT fa un canvi de signe a Y.
C----------------------------------------------------------------------

C L_PRINPLOT						
C Plot point-by-point in a 11 by 7 in paper		
C Parameters espected:					
C IX == x coor						
C IY == y coor							
C IFUN == mode						
C IOUT == file descriptor.				

C   IFUN can be:					
C        0 == clear raster memory.			
C        1 == go and print the point.			
C        2 == output a page.				
C        3 == 						
C        4 == 						

      subroutine L_PRINPLOT(ix, iy, ifun, iout)
      integer ix, iy, ifun, ifun1, iout
      ifun1 = ifun+1
      go to(100, 200, 300), ifun1

100   write(iout, 1000)
1000  format(1x,'/mov {neg moveto}def')
      write(iout, 1001)
1001  format(1x,'/lin {neg lineto}def')
      write(iout, 1002)
1002  format(1X,'erasepage', /, 1x, 'initgraphics')
      return

200   call L_PRINTINT(ix, iy, ix+1, iy, 1, iout)
      return

300   write(iout, 2000)
2000  format(1X,'stroke',/,1X,'showpage')
      return
      end

C L_PRINTINT							
C Plots a line between two points.				
C Parameters:								
C       IX0 == low x	0, 539 units				
C       IY0 == low y	0, 755 units				
C       IX1 == high x	0, 539 units				
C       IY1 == high y	0, 755 units				
C       K ==  line width	0, 20 units			
C       IOUT == file descriptor			

      subroutine L_PRINTINT(ix0, iy0, ix1, iy1, k, iout)
      integer ix0, iy0, ix1, iy1, k, kl, ix1l, iy1l
      real rk, rix0, riy0, rix1, riy1
      common /laser/ kl, ix1l, iy1l

      if (k.ne.kl) then
          if (k.eq.0) then
              rk = .1*(k+1)
          else
              rk = .1*float(k)
          end if
          write(iout, 3100) rk
3100      format(1X, 'stroke',/, 1X, F6.2, 1X, 'setlinewidth')
          kl = k
      end if

      if (ix0.ne.ix1l .or. iy0.ne.iy1l) then
          rix0 = float(ix0) + 20.
          riy0 = float(iy0) + 20.
          write(iout, 3200) rix0, riy0
3200      format(1X, 'stroke',/, 1X, F6.2, 1X, F6.2, 1X, 'mov')
      end if
      ix1l = ix1
      iy1l = iy1

      rix1 = float(ix1) + 20.
      riy1 = float(iy1) + 20.
      write(iout, 3300) rix1, riy1
3300  format(1X, F6.2, 1X, F6.2, 1X, 'lin')
      return
      end



C    -----------------------------------------------------

      subroutine l_inilit (iesc,iout)
      
      goto (10,20,30), iesc

 10   write (iout,'(A)') ' /Times-Roman findfont 11 scalefont setfont'
      return

 20   write (iout,'(A)') ' /Times-Roman findfont 14 scalefont setfont'
      return

 30   write (iout,'(A)') ' /Times-Roman findfont 24 scalefont setfont'
      return

      end

C    --------------------------------------------------------------

      subroutine l_openlit (x,y,sit,iout)
      character*1 sit

      write (iout,10) x,y
 10   format(1x,f6.2,1x,f6.2,1x,'mov')
      if(sit.eq.'V'.or.sit.eq.'v') then
         write(iout,'(A)') ' 90 rotate'
      end if
      write (iout,20)
 20   format(' (')
      return
      end

C    ------------------------------------------------------
      subroutine l_closelit (sit,iout)
      character*1 sit

      write (iout,10)
 10   format(' ) show')
      if(sit.eq.'V'.or.sit.eq.'v') then
         write(iout,'(A)') ' -90 rotate'
      end if
      return
      end


C    ------------------------------------------------------
      subroutine l_lit (ix,iy,alf,n,iesc,sit,iout)

      character*20 alf
      character*1 sit

      x=float(ix)
      y=float(iy)
      call l_inilit(iesc,iout)
      call l_openlit(x,y,sit,iout)
      do i=1,n
         write(iout,10) alf(i:i)
 10      format(a1)
      end do
      call l_closelit(sit,iout)
      return
      end


