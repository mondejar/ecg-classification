C        ==================================================
C          IMPREGRAF (SUBROUTINAS DE REPRESENTACION  POR IMPRESORA)
C          AUTOR: PABLO LAGUNA
C          DATA: 9-JUNIO-87
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

c        SE LE LLAMA DESDE ECGMAIN PARA CREAR FICHEROS .DAT
c        PARA SACAR POR IMPRESORA
C

C        ----------------------------------------------------------
C            CREACION DE FICEROS PARA IMPRIMIR
C        ----------------------------------------------------------

        
C        --------------------------------------------------
C          MAXMIN (TROVA EL MAXIM I EL MINIM DEL SENYAL)
C        --------------------------------------------------

         subroutine maxmin (sen,nptssr,npts,vmax,vmin)

         real*4 sen(100000)
         

         imax=nptssr+1
         imin=nptssr+1
         if ((npts+nptssr).gt.100000)  npts=100000-nptssr
         do i=2+nptssr,nptssr+npts
             if (sen(i).gt.sen(imax)) then
                imax=i
                        else
                if (sen(i).lt.sen(imin)) then 
                   imin=i
                end if
             end if
         end do
         vmax=sen(imax)
         vmin=sen(imin)
          
         return
         end


C        -----------------------------------------------------
C          REPESCAL (REPRESENTACIO ESCALES)
C        -----------------------------------------------------

         subroutine repescal (nptssr,npts,ndiv,vmax,vmin,vindx,vindy)
     

         real*4 vindx(12),vindy(6)

         vindx(1)=nptssr
         xdiv=npts/ndiv
         do i=2,ndiv+1
             vindx(i)=nptssr+xdiv*(i-1)
         end do
         ydiv=(vmax-vmin)/5
         vindy(1)=vmin
         do i=2,6
             vindy(i)=vmin+ydiv*(i-1)
         end do
         
         return 
         end


C      ----------------------------------------------------------------
C       SACAPANTALLA (PRINTA EL GRAFIC REPRESENTAT EN PANTALLA)
C      ----------------------------------------------------------------


	subroutine sacapantalla (titgen,lizer,sen,nptssr,npts,if,izero,
     &		escy)

        character *17 titgen
        character *6  alfanum
        character *1  lizer,reticula,autoesc
        real *4  vindx(9),vindy(6),sen(100000)
        write(6,1)
 1      format('$ Quieres pintar reticula en la seqal? [n]: ')
        read (5,'(a)') reticula
        write(6,2) 
 2      format('$ Grafico autoescalado [s]: ')
        read (5,'(a)') autoesc

C       CALCULO EL FONDO DE ESCAL
        if (autoesc.eq.'n') then 
	 write(6,50)
 50	 format('$ Tensio maxima [mV]:')
	 read(5,*) vmax
c 60	 format(f4.1)
	 write(6,70)
 70	 format('$ Tensio minima [mV]:')
	 read(5,*) vmin
c 80	 format(f4.1)
                            else
                             call maxmin(sen,nptssr,npts,vmax,vmin)
                             if ( lizer.eq.'s') then
                              if (vmin.gt.0) vmin=0
                              if (vmax.lt.0) vmax=0
                             end if
        end if
        call calcula_paso(npts,640,npescx,npimp)
        npisimp=(700-npimp)/2+75

        call repescal(nptssr,npts,5,vmax,vmin,vindx,vindy)
        if (vmax.ne.vmin) escy=400/(vmax-vmin)
        izero=440-nint(escy*(0.-vmin))
                 
C       OBERTURA FITXER
        open(2,file=titgen//'.prn',status='new')

C        call prinplot(1,1,0,2)


C       DIBUIXA RECTANGLE (640 * 400)
C       AMB L'ORIGEN DE COORDENADES (75,140)
C        call printint(npisimp,140,npimp+npisimp,140,1,2)
C        call printint(npimp+npisimp,140,npimp+npisimp,540,1,2)
C        call printint(npimp+npisimp,540,npisimp,540,1,2)
C        call printint(npisimp,540,npisimp,140,1,2)

C       DIBUIXA RATLLES ESCALA
        do i=1,4
            ix=npisimp+i*npimp/5
C            call printint(ix,140,ix,165,1,2)
C            call printint(ix,540,ix,515,1,2)
        end do
        do i=1,4
            iy=140+i*80
C            call printint(npisimp,iy,npisimp+20,iy,1,2)
C            call printint(npimp+npisimp,iy,npimp+npisimp-20,iy,1,2)
        end do

C       DIBUIXA RETOLS
C        call liter(225,100,titgen,12,3,'H',2)
C        call liter(365,560,'seg',3,1,'H',2)
C        call liter(npisimp-66,348,'mv',2,1,'V',2)

C       REPRESENTA ESCALA
        do i=1,6
            write(alfanum,10) vindx(i)/IF
 10         format(f6.2)
C            call liter(npisimp-30+(i-1)*npimp/5,545,alfanum,6,1,'H',2)
        end do
        do i=1,6
            write(alfanum,20) vindy(i)
 20         format(f6.2)
C           call liter(npisimp-59,530-(i-1)*80,alfanum,5,1,'H',2)
        end do

C       DIBUJA UNA RETICULA EQUIVALENTE A 25 MM/SEG DE MM. EN MM.
        if (reticula.eq.'s') then
                              ndiv=npts/if*25
                              do i=1,ndiv-1
                               ix=npisimp+i*npimp/ndiv
C                               call printint(ix,140,ix,540,2,2)
                              end do
        
C                             MARCA EN MAS GORDO LOS CM.
                              do i=1,ndiv/10
                               ix=npisimp+i*npimp/ndiv*10
C                               call printint(ix,140,ix,540,1,2)
                              end do
        end if

C       DIBUIXA LINIA DE ZERO
C       (Cal emprar IZERO+100, perque l'origen del grafic
C       en l'impresora esta desplacat 100 punts respecte a
C       la pantalla grafica)

        if(lizer.eq.'S'.or.lizer.eq.'s') then
C           call printint(npimp+npisimp,izero+100,npisimp,izero+100,3,2)
        end if

C       DIBUIXA SENYAL
C       Les escales escx i escy corresponen a les de la pantalla grafica:
C                   escx=npts/740
C                   escy=400(vmax-vmin)      
C       Per a la impresora escy es la mateixa, pero cal emprar un altre
C       per al eix x.
        


        if(npts.ge.640) then
            iy1=(izero+100)-nint(escy*sen(nptssr+npescx*1))
            do i=2,npimp
                iy0=iy1
                iy1=(izero+100)-nint(escy*sen(nptssr+npescx*i))
                if (nptssr+npescx*i.gt.100000) iy1=0
                 ix0=npisimp+(i-2)
                 ix1=ix0+1
C                call printint(ix0,iy0,ix1,iy1,1,2)
            end do
                        else
            ix1=npisimp+npescx
            iy1=(izero+100)-nint(escy*sen(nptssr+1))
            do i=2,npts
                iy0=iy1
                iy1=(izero+100)-nint(escy*sen(nptssr+i))
                ix0=ix1
                ix1=npisimp+npescx*i
C                call printint(ix0,iy0,ix1,iy1,1,2)
            end do
        end if
        return
        end
c-------------------------------------------------------------------------
	subroutine saca_basel(basel, irpos, npts, nptssr, izero, escy)
c	dibuixem les lineas de base

	dimension basel(8000), irpos(8000)
	j=1
        call calcula_paso(npts,640,npescx,npimp)
        npisimp=(700-npimp)/2+75

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
C           call printint(ixpin,iyba,ixpfi,iyba,3,2)
         end if
         j=j+1
        end do
	return
	end
c--------------------------------------------------------------------------
c------------------------------------------------------------------------- 


        subroutine pinta_marca(ipos,npts,nptssr,iyi,iyf)
C   
C       ipos TIENE LAS POSICIONES DE LAS MARCAS EN X
C       iyi ,iyf SON LAS POSICIONES INICIAL Y FINAL EN Y DE LAS MARCAS

        dimension ipos(8000)

        call calcula_paso(npts,640,npescx,npimp)
        npisimp=(700-npimp)/2+75

        i=1
        do while (ipos(i).lt.npts+nptssr.and.i.le.8000)
         if (ipos(i).gt.nptssr) then
c           BUSCO LA POSICION DEL QRS EN LA ESCALA DE LA PANTALLA
           if (npts.ge.640) then
                          ixpos=npisimp+(ipos(i)-nptssr)/npescx
                         else
                          ixpos=npisimp+(ipos(i)-nptssr)*npescx
           end if
C           call printint(ixpos,iyi,ixpos,iyf,2,2)
         end if
         i=i+1
        end do

        return
        end



C        ----------------------------------------------------------
C         CALCULA  LOS ESCALADOS ADECUADOS
C        ----------------------------------------------------------

         subroutine calcula_paso(npts,nptt,npescx,npimp)

c        npts ES EL NUMERO DE PUNTOS QUE MANDO IMPRIMIR DE LA SENAL
c        nptt ES EL NUMERO DE PUNTOS MAXIMO QUE ADMITE EL GRAFICO
c        npimp NUMERO DE PUNTOS QUE SE IMPRIMEN
c        npisimp ES EL NUMERO DE PUNTOS INICIALES SIN IMPRIMIR
c        npescx ES EL PASO DE CADA PUNTO
 
         integer*4 npts,npescx,nptt,npimp

         if (npts.ge.nptt) then
                            npescx = nint((npts*1.)/nptt)
c                         if (npescx*nptt.lt.npts) npescx=npescx+1
		            npts = npescx*nptt
                            npimp = npts/npescx
                       else 
                            npescx = nint(nptt*1./npts)
c                            if (nptt/npescx.lt.npts) npescx=npescx-1
			    npts = nptt/npescx
                            npimp = npts*npescx
         end if
         return
         end

C        ----------------------------------------------------------
C         CALCULA  LOS ESCALADOS ADECUADOS
C        ----------------------------------------------------------

         subroutine calcula_pasoold(npts,nptt,npescx,npimp)

c        npts ES EL NUMERO DE PUNTOS QUE MANDO IMPRIMIR DE LA SENAL
c        nptt ES EL NUMERO DE PUNTOS MAXIMO QUE ADMITE EL GRAFICO
c        npimp NUMERO DE PUNTOS QUE SE IMPRIMEN
c        npisimp ES EL NUMERO DE PUNTOS INICIALES SIN IMPRIMIR
c        npescx ES EL PASO DE CADA PUNTO
 

         if (npts.ge.nptt) then
                            npescx = nint(npts*1./nptt)
                            if (npescx*nptt.lt.npts) npescx=npescx+1
                            npimp=npts/npescx
                           else 
                            npescx = nint(nptt*1./npts)
                            if (nptt/npescx.lt.npts) npescx=npescx-1
                            npimp=npts*npescx
         end if
         return
         end



C        -----------------------------------------------------------------
C         REPRESENTA LE INTERVALO QT EN FUNCION DEL TIEMPO
C        ----------------------------------------------------------------- 

      
         subroutine p_grafica_qt(titgen,if,is,ns,iqrs,iqt,nqrs)

         dimension iqrs(8000),iqt(8000)
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
                                   
        call repescal(nptssr,npts,5,vmax,vmin,vindx,vindy)
        escy=400/(vmax-vmin)
        i440=440-nint(escy*440)
         
C       OBERTURA FITXER
        open(unit=2,file=titgen//'.prn',status='new')
C        call prinplot(1,1,0,2)
        npisimp=(700-npimp)/2+75
        
C       DIBUIXA RECTANGLE (npimp * 400)
C       AMB L'ORIGEN DE COORDENADES (npsimp,140)
C        call printint(npisimp,140,npisimp+npimp,140,1,2)
C        call printint(npisimp+npimp,140,npisimp+npimp,540,1,2)
C        call printint(npisimp+npimp,540,npisimp,540,1,2)
C        call printint(npisimp,540,npisimp,140,1,2)

C       DIBUIXA RATLLES ESCALA
        do i=1,7
            ix=npisimp+i*npimp/8
C            call printint(ix,140,ix,165,1,2)
C            call printint(ix,540,ix,515,1,2)
        end do
        do i=1,4
            iy=140+i*80
c            call printint(npisimp,iy,20+npisimp,iy,1,2)
c            call printint(npisimp+npimp,iy,npisimp-20+npimp,iy,1,2)
        end do

C       DIBUIXA RETOLS
C        call liter(225,100,titgen,12,3,'H',2)
C        call liter(365,560,'seg',3,2,'H',2)
C        call liter(npisimp-41,348,'msg',3,2,'V',2)

C       REPRESENTA ESCALA
        do i=1,9
           if (vindx(9).ge.100) then
              write(alfanum,11) vindx(i)/if
 11           format(f5.1)
                                else
              write(alfanum,10) vindx(i)/if
 10           format(f5.2)
           end if
C           call liter(npisimp-24+(i-1)*npimp/8,545,alfanum,5,2,'H',2)
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
C            call liter(npisimp-59,530-(i-1)*80,alfanum,5,2,'H',2)
        end do

C       DIBUIXA LINIA DE 440 msg
C       (Cal emprar IZERO+100, perque l'origen del grafic
C       en l'impresora esta desplacat 100 punts respecte a
C       la pantalla grafica)

c         call printint(npisimp,i440+100,npisimp+npimp,i440+100,1,2)

C       DIBUIXA SENYAL
        
        if(npts.ge.640) then
            iy1=540-nint(escy*iqt(1))
            ix1=npisimp
            do i=1,nqrs
                iy0=iy1
                if (iqt(i).ne.0) iy1=540-nint(escy*iqt(i))
                ix0=ix1
                ix1=npisimp+iqrs(i)/npescx
C                call printint(ix0,iy0,ix1,iy1,1,2)
            end do
                        else
            ix1=npisimp
            iy1=540-nint(escy*iqt(1))
            do i=2,nqrs
                iy0=iy1
                if (iqt(i).ne.0) iy1=540-nint(escy*iqt(i))
                ix0=ix1
                ix1=npisimp+npescx*iqrs(i)
C               call printint(ix0,iy0,ix1,iy1,1,2)
            end do
        end if

C       TANCAR FITXER
C        call prinplot(1,1,2,2)
        close(2)

        return
        end


