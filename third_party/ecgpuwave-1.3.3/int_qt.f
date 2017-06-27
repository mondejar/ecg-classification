c
c        ----------------------------------------------------------------------
C          INT_QT.FOR BUSCA EL INICIO DE LA ONDA Q Y EL FIN DE LA T
C          PARA MEDIR EL QT
C           AUTOR: PABLO LAGUNA
C           FECHA: 17-9-87
C        ----------------------------------------------------------------------

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


C        Se llama desde ecgmain 

C        ----------------------------------------------------------------------
C           BUSCA EL INTERVALO QT
C        ----------------------------------------------------------------------

         subroutine int_qt(sen,der1,iqrs,iondaq,iondat,ifm,nqrs,
     &                     iqt,iqtc,iask)

         dimension sen(100000),der1(100000),iqrs(8000),iondaq(8000)
         dimension iqt(8000),iqtc(8000),iondat(8000)
                        
         if (iask.eq.2) then
          write(6,*)
          write(6,*) 'que fraccion quieres de la derivada?'
 5        write(6,10)
 10       format('$ der_limite = der_maxima / x    x= ')
          read(6,*,err=5) frac 
         end if
         
         call frec_cardiaca(ifm,iqrs,nqrs,irrint)
         J=0
  
         
          do i=1,nqrs
           call busca_q(sen,der1,iqrs(i),iondaq(i),ifm)
           if (iask.eq.1) then  
             call busca_t1(sen,der1,iqrs(i),iondat(i),ifm,irrint)
                          else 
             call busca_t2(sen,der1,iqrs(i),iondat(i),ifm,irrint,frac) 
           end if
          iqt(i)=(iondat(i)-iondaq(i))*1000/ifm

          rr=(iqrs(i)-iqrs(i-1))/float(ifm)
          if (i.eq.1) rr=(iqrs(2)-iqrs(1))/float(ifm)
          call qt_corregido(rr,irrint,ifm,iqt(i),iqtc(i))    
          if (i.gt.2) call ajustrr(i,iqrs,irrint,no_uso)
         end do
         
         do i=nqrs+1,8000
          iqt(i)=0
          iondaq(i)=0
          iondat(i)=0
          iqtc(i)=0
         end do

         return
         end
C        ----------------------------------------------------------------------
C           BUSCA EL INCIO DE LA ONDA Q
C        ----------------------------------------------------------------------

         subroutine busca_q(sen,der1,iqrs,iqb,ifm)          

         dimension sen(100000),der1(100000)
         dimension der2(8000),der1f(8000)

c        sen ES LA SEQAL EN LA QUE MIRO LA ONA Q
c        der1 ES SU DERIVADA PRIMERA
c        iqrs SON LAS POSICIONES LOS QRS DETECTADOS EN LA SEQAL sen
c        iondaq  SON LAS POSICIONES DEL INICIO DEL QRS
c        der2 ES LA DERIVADA SEGUNDA SE sen EN ZONAS QUE INTERESA 
c        ifm ES LA FRECUENCIA A LA QUE ESTA MUESTREADA LA SEQAL
C        nqrs ES EL NUMERO DE QRS QUE HAY SELECCIONADOS EN LA SEQAL
                    
         in=nint(ifm*0.04)
         n=iqrs
          if (sen(n).gt.0) then
                   call r_q(ifm,der1,iqrs,iq)             
                   ir=iqrs
                           else
                   call posimaxmin(der1,imaxd,imind,iqrs,iqrs+in)
                   call posimaxmin(der1,imaxi,imini,iqrs-in,iqrs)
                   if (der1(imaxd).gt.(-der1(imini))) then
                                   call detectar_cero(der1,n,'d')
                                   ir=n
                                   iq=iqrs
                                                    else
                                   call detectar_cero(der1,n,'i')
                                   ir=n
                                   call r_q(ifm,der1,n,iq)
                   end if 
          end if

c         SI HAY Q BUSCO DONDE EMPIEZA: PRIMERO VEO DONDE ES MAS PENDIENTE
C         EL FLANCO DE BAJADA DE LA ONDA Q (j) Y LUEGO MIRO PARA ATRAS DONDE 
C         CAMBIA MAS RAPIDAMENTE LA DERIVADA-> MAXIMO DE LA DERIVADA 2 

          in=nint(ifm*0.04)
          if (iq.gt.0) then
                         j=iq
                         do while (der1(j-1).le.der1(j))
                          j=j-1
                         end do
                         call derivada2(j-in,j,der1,der2,ifm)
                         l=j                        
                         call detectar_cero(der1,l,'i')
                         k=in
                         call detectar_cero(der2,k,'i')
                         if (l.gt.j-(in-k)) then 
                                       iqb=l
                                            else
                                       iqb=j-(in-k)
                         end if  
                         if (iqb.lt.iq-ifm*0.06) iqb=iq
          
                       else
c                        PARTO HACIA ATRAS DE LA ONDA R POSITIVA  
                  call fpbc(ir-ifm/10,ifm/10,ifm,30,der1,der1f,ret_pb)
                         j=ifm/10
                         do while (der1f(j-1).ge.der1f(j))
                          j=j-1
                         end do
                         idermax=ir-ifm/10+j
                         call derivada2(j-in,j,der1f,der2,ifm)
                         l=idermax    
                         call detectar_cero(der1,l,'i')
                         k=in-1
                         call detectar_cero(der2,k,'i')
                         if (l.gt.idermax-(in-k)) then 
                                       iqb=l
                                     else
                                       iqb=idermax-(in-k)
                         end if   
                          
          end if
          
          return
          end 


c         ------------------------------------------------------------------
C             VA DE LA POSICON DE LA ONDA R A LA DE LA ONDA Q SI LA HAY
C         ------------------------------------------------------------------

          subroutine r_q(ifm,der1,ir,iq)
        
          dimension der1(100000)          

c             BUSCO LA POSICION DELA ONDA Q 'IQ' 40 MS.
C             ANTES DE LA ONDA R SI NO ESTA AQUI IQ=0

          n=ir
          call detectar_cero(der1,n,'i')
          in=nint(ifm*0.035)
          if (n.le.ir-in) then 
                           iq=0
                          else
                           iq=n
          end if
          return
          end

    

C         -------------------------------------------------------------------
C          CALCULA LA DERIVADA DEL TROZO DE SEQAL QUE MANDO
C         -------------------------------------------------------------------

          subroutine derivada2(ni,nf,x,y,ifm)

          dimension x(100000),y(8000)

c         ni,nf SON LOS PTOS. INICIAL Y FINAL DE LOS QUE CALCULO LA DERIVADA
C         EN X PONGO LA SEQAL DE ENTRADA Y EN Y LA DE SALIDA

          k=ifm/8
          y(1)=(ifm/4)*(x(ni+2)+2*x(ni+1)-3*x(ni))
          y(2)=(ifm/7)*(x(ni+3)+2*x(ni+2)-3*x(ni))
          do i=3,nf-ni
           y(i)=k*(x(ni+i+1)+2*x(ni+i)-2*x(ni+i-2)-x(ni+i-3))
          end do
          do i=nf-ni+1,8000
           y(i)=0
          end do
          
          return
          end


C         --------------------------------------------------------------------
C          DETECTA EL CERO SIGUIENTE DE LA SEQAL DE ENTRADA SEN A LA DERECHA
C          O LA IZQUIERDA 'I' 'D' Y SACA LAPOSICION EN N
C         --------------------------------------------------------------------

          subroutine detectar_cero(sen,n,sentido)
          
          dimension sen(100000)
          character*1 sentido

          if (sentido.eq.'i') then 
                               i=n-1
                               do while (sen(i)*sen(i-1).gt.0)
                                i=i-1
                               end do
                              else   
                               i=n+1
                               do while (sen(i)*sen(i-1).gt.0)
                                i=i+1
                               end do
          end if     
          n=i
          return
          end


C         --------------------------------------------------------------------
C          SACA LA POSICION DEL MAXIMO Y DEL MINIMO DE LA SEQAL
C         --------------------------------------------------------------------

          subroutine posimaxmin(sen,imax,imin,ni,nf)
          
          dimension sen(100000)
 
          imax=ni
          imin=ni
          do i=ni,nf
           if (sen(i).gt.sen(imax)) imax=i
           if (sen(i).lt.sen(imin)) imin=i
          end do
  
          return
          end



C        ----------------------------------------------------------------------
C           BUSCA EL FIN DE LA ONDA T SEGUN EL METODO DE LA DERIVADA SEGUNDA
C        ----------------------------------------------------------------------

         subroutine busca_t1(sen,der1,iqrs,ite,ifm,irrint)

         dimension sen(100000),der1(100000),der1f(8000)
         dimension der2(300)

c        sen eS LA SEQAL EN LA QUE MIRO LA ONA Q
c        der1f ES LA DERIVADA PRIMERA FILTRADA
c        der1 eS SU DERIVADA PRIMERA 
c        iqrs SON LAS POSICIONES LOS QRS DETECTADOS EN LA SEQAL sen
c        iondat  SON LAS POSICIONES DEL INICIO DEL QRS
c        der2 ES LA DERIVADA SEGUNDA DE sen EN ZONAS QUE INTERESA 
c        ifm ES LA FRECUENCIA A LA QUE ESTA MUESTREADA LA SEQAL
C        nqrs ES EL NUMERO DE QRS QUE HAY SELECCIONADOS EN LA SEQAL
c        IT ES LA POSICION DE LA ONDA t

         
         it=0
         call busca_it(ifm,irrint,sen,iqrs,der1,it)
c         BUSCO EL PUNTO SE MAYOR DERIVADA NEGATIVA ENTRE IT Y IT+100MS. 
          call fpbc(it,ifm/3,ifm,30,der1,der1f,ret_pb)
          call posimaxmin(der1,imax,j1,it,it+ifm/10)
          call posimaxmin(der1f,imax,j2,1,ifm/10)
          if (j1.gt.it+j2) then 
                         m=j1
                        else
                         m=it+j2
          end if
          
c         BUSCO EL FINAL DE LA ONDA T
          in=nint(ifm*0.2)
          call derivada2(m-it,m-it+in,der1f,der2,ifm)
          l=m
          call detectar_cero(der1,l,'d')
          k=2
          call detectar_cero(der2,k,'d')
          if (l.lt.m+k) then 
                         ite=l
                        else
                         ite=m+k
          end if
          
         return
         end          

C        ----------------------------------------------------------------------
C           BUSCA EL FIN DE LA ONDA T SEGUN EL METODO DEL % DE DERIVADA
C        ----------------------------------------------------------------------

         subroutine busca_t2(sen,der1,iqrs,ite,ifm,irrint,frac)

         dimension sen(100000),der1(100000)
         dimension der1F(8000)

c        sen eS LA SEQAL EN LA QUE MIRO LA ONA Q
c        der1 eS SU DERIVADA PRIMERA 
c        iqrs SON LAS POSICIONES LOS QRS DETECTADOS EN LA SEQAL sen
c        iondat  SON LAS POSICIONES DEL INICIO DEL QRS
c        der1f ES LA DERIVADA PRIMERA FILTRADA PASO BAJO
c        ifm ES LA FRECUENCIA A LA QUE ESTA MUESTREADA LA SEQAL
C        nqrs ES EL NUMERO DE QRS QUE HAY SELECCIONADOS EN LA SEQAL
c        IT ES LA POSICION DE LA ONDA t

         it=0
         call busca_it(ifm,irrint,sen,iqrs,der1,it)

c         BUSCO EL PUNTO SE MAYOR DERIVADA NEGATIVA ENTRE IT Y IT+100MS. 
          call fpbc(it,ifm/5,ifm,30,der1,der1f,ret_pb)
          call posimaxmin(der1f,imax,j2,1,ifm/10)
          derlim=der1f(j2)/frac
          
c         BUSCO EL FINAL DE LA ONDA T
          j=j2
          do while (der1f(j).lt.derlim)
           j=j+1
          end do
          ite=j+it
          if (i.gt.2) call ajustrr(i,iqrs,irrint,no_uso)
          
          
         return
         end          


C        ------------------------------------------------------------
C          BUSCA LA POSICION DE LA ONDA T
C        ------------------------------------------------------------

         subroutine busca_it(ifm,irrint,sen,iqr,der1,it)

         dimension sen(100000),der1(100000)

c        BUSCO LA ONDA t ENTRE 100 Y 400 MS. DESPUES DEL QRS
         ilim1=nint(ifm*0.1)
         if (irrint.lt.ifm*3/5) then 
                                 ilim2=nint(irrint/2.)
                               else
                                 ilim2=nint(ifm*0.4)
         end if  
         rmax=-abs(sen(iqr))
         j=iqr+ilim1
         do while (j.lt.iqr+ilim2)
           call detectar_cero(der1,j,'d')
           if (sen(j).gt.rmax) then
                                rmax=sen(j)
                                if (j.lt.iqr+ilim2) it=j
           end if
         end do
         if (it.lt.iqr) it=j
         return
         end


C        ------------------------------------------------------------
C         CALCULA EL QT CORREGIDO
C        ------------------------------------------------------------

         subroutine qt_corregido(rr,irrint,ifm,iqt,iqtc)

c         CALCULO EL QT CORREGIDO SI EL INTERVALO RR ESTA ACOTADO
      
          if (rr.lt.irrint*1.1/ifm.and.rr.gt.irrint*0.9/ifm) then
                                           iqtc=iqt/sqrt(rr)
          end if
          if (rr.lt.irrint*0.6/ifm) then 
                                  iqtc=0
          endif                               
      
          return
          end

C        -------------------------------------------------------------
C         BUSCA LA FRECUENCIA CARDIACA MEDIA
C        ------------------------------------------------------------

         subroutine frec_cardiaca(ifm,iqrs,nqrs,irrint)

         dimension iqrs(8000)

C        BUSCO LA FRECUENCIA DEL CORAZON MAS COMUN

         isum=0
         max=0
         min=iqrs(2)-iqrs(1)
         nqrsini=10
         if (nqrs.le.10) nqrsini=nqrs-1

         icon=0
         do i=1,nqrsini
          irrint=iqrs(i+1)-iqrs(i)
c         VEO SI LA FRECUENCIA CARDIACA ESTA EN (30,200)PULSACIONES/MIN.
          if (irrint.gt.ifm*3/10.and.irrint.lt.ifm*2) then
                if (irrint.gt.max) max=irrint
                if (irrint.lt.min) min=irrint
                isum=irrint+isum
                icon=icon+1 
          end if
         end do
         if (icon.gt.2) irrint=(isum-max-min)/(icon-2)
         return
         end


C        ----------------------------------------------------------------
C         CREA UN FICHERO CON LOS QT
C        ---------------------------------------------------------------

         subroutine escribe_qt(tit,ifm,iqt,nqrs)

         character*16  tit
         dimension iqt(8000)

c        CREA UN FICHERO NONDE GUARDA EL QT

         open(unit=1,file=tit,status='new')
         do i=1,nqrs
          write(1,*) iqt(i)*1000/ifm
         end do
         close(unit=1)       

         return
         end

