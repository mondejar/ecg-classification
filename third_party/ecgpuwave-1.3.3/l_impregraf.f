C        ==================================================
C          L_IMPREGRAF (SUBROUTINAS DE REPRESENTACION  POR IMPRESORA)
C          AUTOR: PABLO LAGUNA
C          DATA: 28-OCTUBRE-87
C        ==================================================

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

c        SE LE LLAMA DESDE ECGMAIN PARA CREAR FICHEROS .LSR
c        PARA SACAR POR IMPRESORA
C

C        ----------------------------------------------------------
C            CREACION DE FICEROS PARA IMPRIMIR
C        ----------------------------------------------------------

        
C      ----------------------------------------------------------------
C       PRINLASER (PRINTA EL GRAFIC REPRESENTAT EN PANTALLA
C                  PER LA IMPRESORA LASER)
C      ----------------------------------------------------------------


	subroutine sacalaser (titgen,lizer,sen,nptssr,npts,if,izero,
     &                        escy)


        character *17 titgen
        character *5  alfanum
        character *1  lizer,autoesc
        character *6 aformatimp
        dimension sen(100000)
        real *4  vindx(9),vindy(6)
        integer*4 npts

        call calcula_paso(npts,640,npescx,npimp)
c        call calcula_pasoold(npts,640,npescx,npimp)

C       CALCULO EL FONDO DE ESCALA
	write (6,6) titgen
  6	format('titgen=',a12,'fin')
        write(6,1)
 1      format('$ grafico autoescalado ? [s]:')
        read(5,'(a)') autoesc
        if (autoesc.eq.'n') then
            write(6,2)
 2          format('$ vmax= ')
            read(5,*) vmax
            write(6,3)
 3          format('$ vmin= ')            
            read(5,*) vmin
                            else
            call maxmin(sen,nptssr,npts,vmax,vmin)
        end if
        if ( lizer.eq.'s') then
                           if (vmin.gt.0) vmin=0
                           if (vmax.lt.0) vmax=0
        end if
                           
        call repescal(nptssr,npts,8,vmax,vmin,vindx,vindy)
        if (vmax.ne.vmin) escy=400/(vmax-vmin)
        izero=440-nint(escy*(0.-vmin))
         
C       OBERTURA FITXER
        open(unit=2,file=titgen(1:lnblnk(titgen))//'.ps')
        call l_prinplot(1,1,0,2)
        npisimp=75
        ior_x=150
        ior_y=680
        write(2,4) ior_x,ior_y
  4     format(1x,i3,1x,i3,' translate')
        scale_y=.5
        scale_x=(.5*640)/npimp
        write(2,5) scale_x,scale_y
  5     format(1x,f5.3,1x,f5.3,' scale')

C       DIBUIXA RECTANGLE (npimp * 400)
C       AMB L'ORIGEN DE COORDENADES (npsimp,140)
        call l_printint(npisimp,140,npisimp+npimp,140,12,2)
        call l_printint(npisimp+npimp,140,npisimp+npimp,540,12,2)
        call l_printint(npisimp+npimp,540,npisimp,540,12,2)
        call l_printint(npisimp,540,npisimp,140,12,2)

C       DIBUIXA RATLLES ESCALA
        do i=1,7
            ix=npisimp+i*npimp/8
            call l_printint(ix,140,ix,165,12,2)
            call l_printint(ix,540,ix,515,12,2)
        end do
        do i=1,4
            iy=140+i*80
            call l_printint(npisimp,iy,20+npisimp,iy,12,2)
            call l_printint(npisimp+npimp,iy,npisimp-20+npimp,iy,12,2)
        end do

C       DIBUIXA RETOLS     
        call l_lit(225,130,titgen,17,3,'H',2)
        call l_lit(365,600,'seg',3,2,'H',2)
        call l_lit(npisimp-41,380,'mv',2,2,'V',2)

C       REPRESENTA ESCALA
        do i=1,9
           if (vindx(9).ge.100) then
              write(alfanum,11) vindx(i)/if
 11           format(f5.1)
                                else
              write(alfanum,10) vindx(i)/if
 10           format(f5.2)
           end if
           call l_lit(npisimp-10+(i-1)*npimp/8,580,alfanum,5,2,'H',2)
        end do


C          CALCULA EL FORMATO PARA DIBUJAR EN CADA CASO

              if (vmax.lt.1000..and.vmin.gt.-100) then
                  aformatimp='(f5.1)'
                else if (vmax.lt.10000..and.vmin.gt.-1000) then
                  aformatimp='(f5.0)'
              end if

C        BUSCA Y ESCRIBE EL FORMATO ADECUAD EN EL EJE y
        do i=1,6
            write(alfanum,aformatimp) vindy(i)
            call l_lit(npisimp-36,560-(i-1)*80,alfanum,5,2,'H',2)
        end do

C       DIBUIXA LINIA DE ZERO
C       (Cal emprar IZERO+100, perque l'origen del grafic
C       en l'impresora esta desplacat 100 punts respecte a
C       la pantalla grafica)

        if(lizer.eq.'S'.or.lizer.eq.'s') then
         call l_printint(npisimp,izero+100,npisimp+npimp,izero+100,1,2)
        end if

C       DIBUIXA SENYAL
C       Les escales escx i escy corresponen a les de la pantalla grafica:
C                   escx=npts/740
C                   escy=400(vmax-vmin)      
C       Per a la impresora escy es la mateixa, pero cal emprar una altre
C       per al eix x (npescx), calculada a CALCULA_ESCALA.
        
        if(npts.ge.640) then
            iy1=(izero+100)-nint(escy*sen(nptssr+npescx*1))
            do i=2,npimp
                iy0=iy1
                iy1=(izero+100)-nint(escy*sen(nptssr+npescx*i))
                if (nptssr+npescx*i.gt.100000) iy1=0
                ix0=npisimp+(i-2)
                ix1=ix0+1
                call l_printint(ix0,iy0,ix1,iy1,5,2)
            end do
                        else
            ix1=npisimp+npescx
            iy1=(izero+100)-nint(escy*sen(nptssr+1))
            do i=2,npts
                iy0=iy1
                iy1=(izero+100)-nint(escy*sen(nptssr+i))
                ix0=ix1
                ix1=npisimp+npescx*i
                call l_printint(ix0,iy0,ix1,iy1,5,2)
            end do
        end if

C       TANCAR FITXER

        return
        end
c-------------------------------------------------------------------------

	subroutine l_saca_basel(basel, irpos, npts, nptssr, izero, escy)
c	dibuixem les lineas de base

	dimension basel(8000), irpos(8000)
	j=1
        call calcula_paso(npts,640,npescx,npimp)
        npisimp=75

        do while (irpos(j).lt.npts+nptssr.and.j.le.8000)
         if (irpos(j).gt.nptssr) then
           if (npts.ge.640) then
                          ixpin=npisimp+(irpos(j)-nptssr)/npescx-25
                          ixpfi=npisimp+(irpos(j)-nptssr)/npescx+25
                         else
                          ixpin=npisimp+(irpos(j)-nptssr)*npescx-25
                          ixpfi=npisimp+(irpos(j)-nptssr)*npescx+25
           end if
	   iyba=(izero+100)-nint(escy*basel(j))
           call l_printint(ixpin,iyba,ixpfi,iyba,3,2)
         end if
         j=j+1
        end do
	return
	end
c--------------------------------------------------------------------------
c------------------------------------------------------------------------- 


C       --------------------------------------------------------------
C        DIBUJA MARCAS EN LA SEQAL DEL FICHERO PARA LASER
C       -------------------------------------------------------------
  
        subroutine l_pinta_marca(ipos,npts,nptssr,iyi,iyf)
C   
C       ipos TIENE LAS POSICIONES DE LAS MARCAS EN X
C       iyi ,iyf SON LAS POSICIONES INICIAL Y FINAL EN Y DE LAS MARCAS

        dimension ipos(80)

        call calcula_paso(npts,640,npescx,npimp)
        npisimp=75


        i=1
        do while (ipos(i).lt.npts+nptssr.and.i.le.80)
         if (ipos(i).gt.nptssr) then
c           BUSCO LA POSICION DEL QRS EN LA ESCALA DE LA PANTALLA
           if (npts.ge.640) then
                          ixpos=npisimp+(ipos(i)-nptssr)/npescx
                         else
                          ixpos=npisimp+(ipos(i)-nptssr)*npescx
           end if
           call l_printint(ixpos,iyi,ixpos,iyf,2,2)
         end if
         i=i+1
        end do

        return
        end



C        -----------------------------------------------------------------
C         REPRESENTA LE INTERVALO QT EN FUNCION DEL TIEMPO
C        ----------------------------------------------------------------- 

      
         subroutine l_grafica_qt(titgen,if,is,ns,iqrs,iqt,nqrs)

         dimension iqrs(80),iqt(80)
         character *17 titgen
         character *5  alfanum
         character *6 aformatimp
         real *4  vindx(9),vindy(6)
     
         nptssr=is*if
         npts=ns*if
         call calcula_paso(npts,640,npescx,npimp)

C       CALCULO EL FONDO DE ESCALA
        vmin=0
        vmax=550.
                                   
        call repescal(nptssr,npts,8,vmax,vmin,vindx,vindy)
        escy=400/(vmax-vmin)
        i440=440-nint(escy*440)
         
C       OBERTURA FITXER
        open(unit=2,file=titgen(1:lnblnk(titgen))//'.ps')
        call l_prinplot(1,1,0,2)
        npisimp=75
        ior_x=150
        ior_y=680
        write(2,1) ior_x,ior_y
  1     format(1x,i3,1x,i3,' translate')
        scale_y=.5
        scale_x=(.5*640)/npimp
        write(2,2) scale_x,scale_y
  2     format(1x,2f5.2,' scale')

C       DIBUIXA RECTANGLE (npimp * 400)
C       AMB L'ORIGEN DE COORDENADES (npsimp,140)
        call l_printint(npisimp,140,npisimp+npimp,140,12,2)
        call l_printint(npisimp+npimp,140,npisimp+npimp,540,12,2)
        call l_printint(npisimp+npimp,540,npisimp,540,12,2)
        call l_printint(npisimp,540,npisimp,140,12,2)

C       DIBUIXA RATLLES ESCALA
        do i=1,7
            ix=npisimp+i*npimp/8
            call l_printint(ix,140,ix,165,12,2)
            call l_printint(ix,540,ix,515,12,2)
        end do
        do i=1,4
            iy=140+i*80
            call l_printint(npisimp,iy,20+npisimp,iy,12,2)
            call l_printint(npisimp+npimp,iy,npisimp-20+npimp,iy,12,2)
        end do

C       DIBUIXA RETOLS
        call l_lit(225,130,titgen,17,3,'H',2)
        call l_lit(365,600,'seg',3,2,'H',2)
        call l_lit(npisimp-41,380,'msg',3,2,'V',2)

C       REPRESENTA ESCALA
        do i=1,9
           if (vindx(9).ge.100) then
              write(alfanum,11) vindx(i)/if
 11           format(f5.1)
                                else
              write(alfanum,10) vindx(i)/if
 10           format(f5.2)
           end if
           call l_lit(npisimp-10+(i-1)*npimp/8,580,alfanum,5,2,'H',2)
        end do


C          CALCULA EL FORMATO PARA DIBUJAR EN CADA CASO

              if (vmax.lt.1000..and.vmin.gt.-100) then
                  aformatimp='(f5.1)'
                else if (vmax.lt.10000..and.vmin.gt.-1000) then
                  aformatimp='(f5.0)'
              end if

C        BUSCA Y ESCRIBE EL FORMATO ADECUAD EN EL EJE y
        do i=1,6
            write(alfanum,aformatimp) vindy(i)
            call l_lit(npisimp-36,560-(i-1)*80,alfanum,5,2,'H',2)
        end do

C       DIBUIXA LINIA DE 440 msg
C       (Cal emprar IZERO+100, perque l'origen del grafic
C       en l'impresora esta desplacat 100 punts respecte a
C       la pantalla grafica)

         call l_printint(npisimp,i440+100,npisimp+npimp,i440+100,1,2)

C       DIBUIXA SENYAL
        
        if(npts.ge.640) then
            iy1=540-nint(escy*iqt(1))
            ix1=npisimp
            do i=1,nqrs
                iy0=iy1
                if (iqt(i).ne.0) iy1=540-nint(escy*iqt(i))
                ix0=ix1
                ix1=npisimp+iqrs(i)/npescx
                call l_printint(ix0,iy0,ix1,iy1,5,2)
            end do
                        else
            ix1=npisimp
            iy1=540-nint(escy*iqt(1))
            do i=2,nqrs
                iy0=iy1
                if (iqt(i).ne.0) iy1=540-nint(escy*iqt(i))
                ix0=ix1
                ix1=npisimp+npescx*iqrs(i)
                call l_printint(ix0,iy0,ix1,iy1,5,2)
            end do
        end if

C       TANCAR FITXER
        call l_prinplot(1,1,2,2)
        close(2)

        return
        end


