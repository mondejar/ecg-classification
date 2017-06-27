 
C        =======================================================================
C          PROSEN (HACE LOS PROCESADOS DE LA SEQAL: FILTRADO PASO ALTO Y BAJO
C                  DERIVADAS,VALOR CUADRATICO Y INTEGRACION CON VENTANA MOVIL)
C          AUTOR: PABLO LAGUNA/RAIMON JANE, 
C          DATA: 22-MAIG-87
C          MODIFICAT: 19-FEBRER-88
C        =======================================================================
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

c
c        SE LE LLAMA DESDE ECGMAIN PARA REPRESENTAR POR PANTALLA,IMPRESORA 
c        Y PARA DETECTAR QRS (SU PROCESAMIENTO PREVIO)
c
C

C        ---------------------------------------------------------------
C          REALIZA UN FILTRADO PASO BAJO DE LA SEQAL
C        ---------------------------------------------------------------  


         subroutine fpb(n,if,ifc,x,y,iretard)
         dimension x(100000),y(100000)

c        APLICA A LA SEQAL QUE ENTRA EN EL VECTOR x UN FILTRO PASO BAJO. lA 
C        SEQAL QUE ENTRA LA SUPONGO MUESTREADA A if hZ.
C         EL FILTRO TIENE UN POLO DOBLE EN (0,1) Y ifc CEROS DOBLES EN
C        ELA CIRCULO UNIDAD .LA GANANCIA DEL
C        FILTRO SERIA DEl NUMERO de CEROS AL CUADRADO. 
C         EL RETARDO DEL FILTRO ES DE NUMERO de CEROS MENOS UNO
C         LAS CONDICIONES INICIALES SUPONGO QUE LA SEQAL ES CONSTANTE ANTES DEL
C        INSTANTE INICIAL CON VALOR 0
C         LA ECUACION SERIA y(Nt)=2y(Nt-t)-y(Nt-2t)+x(Nt)-2x(Nt-CEROSt)
C                                 +x(Nt-2*CEROSt).
C         LA SEQAL FILTRADA LA DEVUELVE EN EL VECTOR y


C        RM ES EL VALOR MEDIO DE LOS 10 PRIMEROS PUNTOS
c         sum=x(1)
c         do i=2,10
c          sum=sum+x(i)
c         end do
c         rm=sum/10
c          rm=x(1)
         rm=0.

C        l eS EL NUMERO DE CEROS
c        ifc ES LA FRECUENCIA DE CORTE A CERO DEL FILTRO
c        if ES LA FRECUENCIA DE MUESTREO DE LA SEQAL
c         l=nint(1.*if/ifc)
c         y(1)=x(1)+ l*rm
c         do i=2,l
c             y(i)=y(i-1)+x(i)-rm
c         end do
c         do i=l+1,n
c             y(i)=y(i-1)+x(i)-x(i-l)
c         end do
c         do i=n,n+l
c               y(i)=y(i-1)+x(n)-x(i-l)
c         end do
 

c        filtro de segundo orden (cuidado se puede inestabilizar

           l=nint(1.*if/ifc)
           y(1)=x(1)+(l**2-1)*rm
           y(2)=(2*y(1)+x(2)-(l**2+1)*rm)
          do i=3,l
               y(i)=(2*y(i-1)-y(i-2)+x(i)-rm)
          end do
           do i=l+1,2*l
               y(i)=(2*y(i-1)-y(i-2)+x(i)-2*x(i-l)+rm)
           end do
           do i=2*l+1,n
               y(i)=(2*y(i-1)-y(i-2)+x(i)-2*x(i-l)+x(i-2*l))
           end do

          do i=n,n+l
               y(i)=(2*y(i-1)-y(i-2)+x(n)-2*x(i-l)+x(i-2*l))
          end do
 
C        QUITO EL RETARDO DE LA SEQAL
         iretard=l-1
         if (n.le.(100000-l+1) ) then
                              do i=1,n
                                y(i)=y(i+l)
                              end do
                             else
                              do i=1,100000-l+1
                                y(i)=y(i+l)
                              end do
                              do i=100000-l+1,n
                                y(i)=0
                              end do
         end if 


C        PONGO EL RESTO A CERO
         do i=n+1,100000
            y(i)=0
         end do
 
         return
         end


C        --------------------------------------------------------------
c          REALIZA UN FILTRADO PASO BAJO DE UN TROZO DE SEQAL
C        --------------------------------------------------------------

         subroutine fpbc(ni,n,if,ifc,x,y,iretard)
         dimension x(100000),y(400)

c        APLICA A LA SEQAL QUE ENTRA EN EL VECTOR x UN FILTRO PASO BAJO. lA 
C        SEQAL QUE ENTRA LA SUPONGO MUESTREADA A if hZ.
C         EL FILTRO TIENE UN POLO DOBLE EN (0,1) Y ifc CEROS DOBLES EN
C        EL CIRCULO UNIDAD .LA GANANCIA DEL
C        FILTRO SERIA DEl NUMERO de CEROS AL CUADRADO. 
C         EL RETARDO DEL FILTRO ES DE NUMERO de CEROS MENOS UNO
C         LAS CONDICIONES INICIALES SUPONGO QUE LA SEQAL ES CONSTANTE ANTES DEL
C        INSTANTE INICIAL CON VALOR 0
C         lA ECUACION SERIA Y(nT)=2Y(nT-T)-Y(nT-2T)+X(nT)-2X(nT-cerosT)
c                                 +X(nT-2*cerosT).
C         lA SEQAL FILTRADA LA DEVUELVE EN EL VECTOR y


C        RM ES EL VALOR MEDIO DE LOS 10 PRIMEROS PUNTOS
c         sum=x(ni)
c         do i=ni+2,ni+10
c          sum=sum+x(i)
c         end do
         rm=x(ni)
       
c        l ES EL NUMERO DE CEROS
C        ifc ES LA FRECUENCIA A LA QUE EL FILTRO CAE A CERO
C        if ES LA FRECUENCIA A LA QUE ESTA MUESTREADA LA SEQAL 
C        ni ES LA POSICION INICIAL A PARTIR DE LA CUAL QUIERO FILTRAR
c        n ES EL NUMERO DE PUNTOS A FILTRAR
C        SE DEBE CUMPLIR (n<1000-l) SI NO SE SALDRA DE LOS LIMITES DEL VECTOR
         l=nint(1.*if/ifc)
         y(1)=x(ni+1)+(l**2-1)*rm
         y(2)=(2*y(1)+x(ni+2)-(l**2+1)*rm)
         do i=3,l
             y(i)=(2*y(i-1)-y(i-2)+x(ni+i)-rm)
         end do
         do i=l+1,2*l
             y(i)=(2*y(i-1)-y(i-2)+x(ni+i)-2*x(ni+i-l)+rm)
         end do
         do i=2*l+1,n+l
             y(i)=(2*y(i-1)-y(i-2)+x(ni+i)-2*x(ni+i-l)+x(ni+i-2*l))
         end do
 
C        QUITO EL RETARDO DE LA SEQAL
         iretard=l
         if (n.le.(400-l+1)) then
                              do i=1,n
                                y(i)=y(i+l)
                              end do
                             else
                              do i=1,400-l+1
                                y(i)=y(i+l)
                              end do
                              do i=400-l+1,n
                                y(i)=0
                              end do
         end if 

C        PONGO EL RESTO A CERO
         do i=n+1,400
            y(i)=0
         end do
 
         return
         end
                     
C        ---------------------------------------------------------------
C          FILTRE PASSA ALT (primer ordre) 
C        ---------------------------------------------------------------  

         subroutine  fpa(n,if,ifc,x,y,iretard)
         dimension x(100000), y(100000)
C                                          


c        APLICA A LA SEQAL QUE ENTRA EN EL VECTOR x UN FILTRO PASO BAJO. lA 
C        SEQAL QUE ENTRA LA SUPONGO MUESTREADA A if hZ.
C         EL FILTRO TIENE UN POLO DOBLE EN (0,1) Y ifc CEROS DOBLES EN
C        ELA CIRCULO UNIDAD .LA GANANCIA DEL
C        FILTRO SERIA DEl NUMERO de CEROS AL CUADRADO. 
C         EL RETARDO DEL FILTRO ES DE NUMERO de CEROS MENOS UNO
C         LAS CONDICIONES INICIALES SUPONGO QUE LA SEQAL ES CONSTANTE ANTES DEL
C        INSTANTE INICIAL CON VALOR 0
C         LA ECUACION SERIA y(Nt)=2y(Nt-t)-y(Nt-2t)+x(Nt)-2x(Nt-CEROSt)
C                                 +x(Nt-2*CEROSt).
C         LA SEQAL FILTRADA LA DEVUELVE EN EL VECTOR y


C        RM ES EL VALOR MEDIO DE LOS 10 PRIMEROS PUNTOS
c         sum=x(1)
c         do i=2,10
c          sum=sum+x(i)
c         end do
c         rm=sum/10
c          rm=x(1)
         rm=0.

C        l eS EL NUMERO DE CEROS
c        ifc ES LA FRECUENCIA DE CORTE A CERO DEL FILTRO
c        if ES LA FRECUENCIA DE MUESTREO DE LA SEQAL
c         l=nint(1.*if/ifc)
c         y(1)=x(1)+ l*rm
c         do i=2,l
c             y(i)=y(i-1)+x(i)-rm
c         end do
c         do i=l+1,n
c             y(i)=y(i-1)+x(i)-x(i-l)
c         end do
c	 do i=n,n+l
c              y(i)=y(i-1)+x(n)-x(i-l)
c         end do

c        filtro de segundo orden (cuidado se puede desestabilizar)

           l=nint(1.*if/ifc)
           y(1)=x(1)+(l**2-1)*rm
           y(2)=(2*y(1)+x(2)-(l**2+1)*rm)
           do i=3,l
               y(i)=(2*y(i-1)-y(i-2)+x(i)-rm)
          end do
           do i=l+1,2*l
               y(i)=(2*y(i-1)-y(i-2)+x(i)-2*x(i-l)+rm)
           end do
          do i=2*l+1,n
               y(i)=(2*y(i-1)-y(i-2)+x(i)-2*x(i-l)+x(i-2*l))
           end do

         do i=n,n+l
               y(i)=(2*y(i-1)-y(i-2)+x(n)-2*x(i-l)+x(i-2*l))
          end do
 

C        PARTE PASO TODO MENOS PASO BAJO

         iretard=l-1
         gain=l*l
         do i=1,n-iretard
            y(i)=gain*x(i)-y(i+iretard)
         end do    
         do i=n-iretard+1,n
           y(i)=0.
         end do

         
         return
         end
                     
C        ---------------------------------------------------------------
C          REALIZA UN FILTRADO PASO ALTO DE LA SEQAL
C        ---------------------------------------------------------------  

C---         subroutine  fpa(n,if,ifc,x,y,iretard)
c---         dimension x(100000),y(100000)
C
c        aPLICA A LA SEQAL QUE ENTRA EN EL VECTOR x UN FILTRO PASO ALTO. lA 
C        SEQAL QUE ENTRA LA SUPONGO MUESTREADA A if hZ . eL FILTRO LO
c        IMPLEMENTO RESTANDO DE UNO PASO TODO OTRO PASO BAJO DE FRECUENCIA
c        DE CORTE MENOR QUE LA ANTERIOR . 
C         cON ESTO OBTENGO UNA  FRECUENCIA DE CORTE DE FC HZ, Y EL
C         lAS CONDICIONES INICIALES SUPONGOQUE LA SEQAL ES CONSTANTE ANTES DEL
C        INSTANTE INICIAL CON VALOR 0.
C         lA ECUACION SERIA y(Nt)=ceros*x(Nt-ceros/2*t)-y(Nt-t)-x(Nt)
c                                 +x(Nt-ceros*t).
C         lA SEQAL FILTRADA LA DEVUELVE EN EL VECTOR y
C        


C        RM ES EL VALOR DEL PRIMER PUNTO
C         rm=x(1)
c--         rm=0.

C        l eS EL NUMERO DE CEROS
c        ifc ES LA FRECUENCIA DE CORTE A CERO DEL FILTRO
c        if ES LA FRECUENCIA DE MUESTREO DE LA SEQAL
c--         l=nint(1.*if/ifc)
c--         y(1)=x(1)+(l**2-1)*rm
c--         y(2)=(2*y(1)+x(2)-(l**2+1)*rm)
c--         do i=3,l
c--             y(i)=(2*y(i-1)-y(i-2)+x(i)-rm)
c--         end do
c--         do i=l+1,2*l
c--             y(i)=(2*y(i-1)-y(i-2)+x(i)-2*x(i-l)+rm)
c--         end do
c--         do i=2*l+1,n
c--             y(i)=(2*y(i-1)-y(i-2)+x(i)-2*x(i-l)+x(i-2*l))
c--         end do
C         do i=n,n+l
C             y(i)=(2*y(i-1)-y(i-2)+x(n)-2*x(i-l)+x(i-2*l))
C         end do


C        PARTE PASO TODO MENOS PASO BAJO
c         do i=1,(l-1)
c         y(i)=l**2*rm-y(i)
c         end do
c         do i=l,n+l
c         y(i)=l**2*x(i-(l-1))-y(i)
c         end do
         
c--         do i=1,l-1
c--         y(i)=l**2*rm-y(i)
c--         end do
c--         do i=l,n
c--         y(i)=l**2*x(i-(l-1))-y(i)
c--         end do

C        QUITO EL RETARDO DE LA SEQAL
c
c         iretard=l-1
c         if (n.le.(100000-l+1)) then
c                              do i=1,n
c                                y(i)=y(i+l-1)
c                              end do
c                             else
c                              do i=1,100000-l+1
c                                y(i)=y(i+l-1)
c                              end do
c                              do i=100000-l+1,n
c                                y(i)=0
c                              end do
c         end if 
         
c-         if (n.le.(100000-l+1)) then
c-                              do i=1,n-L+1
c-                                y(i)=y(i+l-1)
c-                              end do
c-                             else
c-                              do i=1,100000-l+1
c-                                y(i)=y(i+l-1)
c-                              end do
c-                              do i=100000-l,n
c-                                y(i)=0
c-                              end do
c-         end if 

C        PONGO EL RESTO A CERO
c-         do i=n-L,100000
c-          y(i)=0
c-         end do
c-
c---         return
c---         end

C        --------------------------------------------------------------
c          REALIZA UN FILTRADO PASO AlTO DE UN TROZO DE SEQAL
C        --------------------------------------------------------------

         subroutine fpac(ni,n,if,ifc,x,y,iretard)
         dimension x(100000),y(2000)

c        APLICA A LA SEQAL QUE ENTRA EN EL VECTOR x UN FILTRO PASO ALTO. lA 
C        SEQAL QUE ENTRA LA SUPONGO MUESTREADA A if hZ.
C         EL FILTRO TIENE UN POLO DOBLE EN (0,1) Y ifc CEROS DOBLES EN
C        EL CIRCULO UNIDAD .LA GANANCIA DEL
C        FILTRO SERIA DEl NUMERO de CEROS AL CUADRADO. 
C         EL RETARDO DEL FILTRO ES DE NUMERO de CEROS MENOS UNO
C         LAS CONDICIONES INICIALES SUPONGO QUE LA SEQAL ES CONSTANTE ANTES DEL
C        INSTANTE INICIAL CON VALOR X(NI)
C         lA ECUACION SERIA Y(nT)=2Y(nT-T)-Y(nT-2T)+X(nT)-2X(nT-cerosT)
c                                 +X(nT-2*cerosT).
C         lA SEQAL FILTRADA LA DEVUELVE EN EL VECTOR y


C        RM ES EL VALOR MEDIO DE LOS 10 PRIMEROS PUNTOS
c         sum=x(ni)
c         do i=ni+2,ni+10
c          sum=sum+x(i)
c         end do
         rm=x(ni)
       
c        l ES EL NUMERO DE CEROS
C        ifc ES LA FRECUENCIA A LA QUE EL FILTRO CAE A CERO
C        if ES LA FRECUENCIA A LA QUE ESTA MUESTREADA LA SEQAL 
C        ni ES LA POSICION INICIAL A PARTIR DE LA CUAL QUIERO FILTRAR
c        n ES EL NUMERO DE PUNTOS A FILTRAR
C        SE DEBE CUMPLIR (n<2000-l) SI NO SE SALDRA DE LOS LIMITES DEL VECTOR
         l=nint(1.*if/ifc)
         y(1)=x(ni+1)+(l**2-1)*rm
         y(2)=(2*y(1)+x(ni+2)-(l**2+1)*rm)
         do i=3,l
             y(i)=(2*y(i-1)-y(i-2)+x(ni+i)-rm)
         end do
         do i=l+1,2*l
             y(i)=(2*y(i-1)-y(i-2)+x(ni+i)-2*x(ni+i-l)+rm)
         end do
         do i=2*l+1,n+l
             y(i)=(2*y(i-1)-y(i-2)+x(ni+i)-2*x(ni+i-l)+x(ni+i-2*l))
         end do
 
C        QUITO EL RETARDO DE LA SEQAL
         iretard=l
         if (n.le.(2000-l+1)) then
                              do i=1,n
                                y(i)=y(i+l)
                              end do
                             else
                              do i=1,2000-l+1
                                y(i)=y(i+l)
                              end do
                              do i=2000-l+1,n
                                y(i)=0
                              end do
         end if 

C        PARTE PASO TODO MENOS PASO BAJO
         do i=1,n
         y(i)=l**2*x(ni+i)-y(i)
         end do

C        PONGO EL RESTO A CERO
         do i=n+1,2000
            y(i)=0
         end do
 
         return
         end

C        ---------------------------------------------------------------
C          REALIZA UN TEXT SOBRE LA AMPLITUD POSIT. Y NEGAT. DE LA SEQAL
C        ---------------------------------------------------------------  

         subroutine test(if,x,inv)
         dimension x(100000)
         logical inv

C        inv dICE SI HE INVERTIDO LA SEQAL O NO

         senp=0
         senn=0

c        bUSCO EN LOS PRIMEROS SEGUNDOS
         do i=if,if*3
          if (x(i).lt.senn) senn=x(i)
          if (x(i).gt.senp) senp=x(i)
         end do

         inv=.false. 
         if (senp.lt.(-senn)) then
          inv=.true.
         end if
         return
         end
          

C        ---------------------------------------------------------------
C           INVIERTE LA SEQAL 
C        ---------------------------------------------------------------

         subroutine invertir(sen,n)
         dimension sen(100000)
         
         do i=1,n
          sen(i)=-sen(i)
         end do
         return
         end    

C        ---------------------------------------------------------------
C          REALIZA UN FILTRADO DERIVADA DE LA SEQAL
C        ---------------------------------------------------------------  

         subroutine der(n,if,x,y)
         dimension x(100000),y(100000)

c        aPLICA A LA SEQAL QUE ENTRA EN EL VECTOR x UN FILTRO DERIVADA. lA 
C        SEQAL QUE ENTRA LA SUPONGO MUESTREADA A if hZ.
c         ESTA DERIVADA SERA SERA TAL HASTA UNOS 30 hZ. DESPUES CAE LO CUAL
C         ES BUENO PARA NO ENFATIZAR FRECUNCIAS ALTAS
C         eL RETARDO DE LA DERIVADA ES DE l MUESTRAS.
C         lAS CONDICIONES INICIALES SUPONGOQUE LA SEQAL ES CONSTANTE ANTES DEL
C        INSTANTE INICIAL CON VALOR 0.
C         lA ECUACION SERIA y(Nt)=25*(x(Nt)+2x(Nt-l/2*t)-x(Nt-(l/2+l)t)
c                                 -x(Nt-(2*l)t)).
C         lA SEQAL DERIVADA LA DEVUELVE EN EL VECTOR y



          k=if/8
          y(3)=if*(x(2)-x(1))
          y(4)=(if/4)*(2*x(3)-2*x(1))
          do i=5,n+2
            y(i)=k*(x(i)+2*x(i-1)-2*x(i-3)-x(i-4))
          end do

C        QUITO EL RETARDO DE LA SEQAL
         do i=1,n
             y(i)=y(i+2)
         end do

C        PONGO EL RESTO A CERO
         do i=n+1,100000
         y(i)=0
         end do

         return
         end


C        ---------------------------------------------------------------
C          REALIZA LA FUNCION CUADRADA DE LA SEQAL
C        ---------------------------------------------------------------  

         subroutine quad(n,x,y)
         dimension x(100000),y(100000)

C        aPLICA A LA SEQAL QUE ENTRA EN EL VECTOR x EL OPERADOR ELEVER AL 
C        CUADRADO Y LA SACA EN EL VESTOR y


         do i=1,n
             y(i)=x(i)**2
         end do

c        do i=n+1,100000
c            y(i)=0
c        end do

         return
         end


C        ---------------------------------------------------------------
C          REALIZA LA INTEGRACION DE VENTANA MOVIL
C        ---------------------------------------------------------------  

         subroutine mwint(n,if,x,y)
         dimension x(100000),y(100000)

c        aPLICA A LA SEQAL QUE ENTRA EN EL VECTOR x UNA INTEGRACION DE VENTANA MOVI
C        MOVIL DE UNA ANCHURA DE 70 MUESTRAS QUE CORRESPONDE A 70 MS., MAS O 
C        MENOS LA ANCHURA DE UN qrs. 
C        SEQAL QUE ENTRA LA SUPONGO MUESTREADA A if
C         lAS CONDICIONES INICIALES SUPONGOQUE LA SEQAL ES CONSTANTE ANTES DEL
C        INSTANTE INICIAL CON VALOR x(1).
C         lA SEQAL INTEGRADA LA DEVUELVE EN EL VECTOR y


         l=nint(9.5*if/100.)
         do i=1,n
             if (i.lt.l)  then
C                La seguent instruccio tambe hi havia un sum=0  ???
C                 sum= (x(1)*(l-i))/l
                  sum=0        
                 do j=1,i
                       sum=sum+x(j)/l
                 end do 
             end if 
             if (i.ge.l)  then
                 sum=0
                 do j=i-l,i
                       sum=sum+x(j)/l
                 end do 
             end if 
             y(i)=sum         
         end do

c         do i=n+1,100000
c             y(i)=0
c         end do

         return
         end

C        ---------------------------------------------------------------
C          REALIZA LA INTEGRACION DE VENTANA MOVIL
C        ---------------------------------------------------------------  

         subroutine mwint_p(n,if,x,y,z,iqrs)
         dimension x(100000),y(100000),z(100000)

c        aPLICA A LA SEQAL QUE ENTRA EN EL VECTOR x UNA INTEGRACION DE VENTANA MOVI
C        MOVIL DE UNA ANCHURA DE 70 MUESTRAS QUE CORRESPONDE A 70 MS., MAS O 
C        MENOS LA ANCHURA DE UN qrs. 
C        SEQAL QUE ENTRA LA SUPONGO MUESTREADA A if
C         lAS CONDICIONES INICIALES SUPONGOQUE LA SEQAL ES CONSTANTE ANTES DEL
C        INSTANTE INICIAL CON VALOR x(1).
C         lA SEQAL INTEGRADA LA DEVUELVE EN EL VECTOR y


         l=nint(30.5*if/100.)
         l2=nint(30.5*if/100./2)
         do i=1,n
             if (i.lt.l2)  then
C                La seguent instruccio tambe hi havia un sum=0  ???
C                 sum= (x(1)*(l-i))/l
                  sum=0        
                 do j=1,i+l2
                       sum=sum+x(j)*z(iqrs-i+j) /l
                 end do 
             end if 
             if ((i.ge.l2).and.(i.lt.(n-l2)))  then
                 sum=0
                 do j=i-l2,i+l2
                       sum=sum+x(j)*z(iqrs-i+j)/l
                 end do 
             end if 
             if (i.ge.(n-l2))  then
                 sum=0
                 do j=i-l2,n
                       sum=sum+x(j)*z(iqrs-i+j)/l
                 end do 
             end if 
             y(i)=sum         
         end do

c         do i=n+1,100000
c             y(i)=0
c         end do

         return
         end


C        ---------------------------------------------------------------
C          REALIZA UNA NORMALIZACION DE MODULO DE LA SEQAL
C        ---------------------------------------------------------------  

         subroutine normaliz(if,max,y)
         dimension y(100000)
    
C        NORMALIZO AL MAXIMO VALOR max
         rmax=y(1)
         do i=2,2*if
          if  (ABS(y(i)).gt.rmax) then 
           rmax=ABS(y(i))
          end if
         end do
         if (rmax.eq.0) rmax=1

C        NORMALIZO
         do i=1,100000
          y(i)=max*y(i)/rmax
         end do
         return
         end
C        ---------------------------------------------------------------
C          REALIZA LA NORMALIZACION INICIAL
C        ---------------------------------------------------------------  

         subroutine normaliz_i(if,max,rmax,y)
         dimension y(100000)


C        rmax es el factor de normalitzacio (parametre de sortida)   
C        NORMALIZO AL MAXIMO VALOR max
         rmax=y(1)
         do i=2,2*if
          if  (ABS(y(i)).gt.rmax) then 
           rmax=ABS(y(i))
          end if
         end do
         if (rmax.eq.0) rmax=1
 
C        NORMALIZO
         do i=1,100000
          y(i)=max*y(i)/rmax
         end do
         return
         end                                                      

C        ---------------------------------------------------------------
C          REALIZA LA NORMALIZACION "ciclica" con el factor RMAX
C        ---------------------------------------------------------------  

         subroutine normaliz_c(if,max,rmax,y)
         dimension y(100000)
    

C        NORMALIZO
         do i=1,100000
          y(i)=max*y(i)/rmax
         end do
         return
         end


C        ---------------------------------------------------------------
C          REALIZA UN CAMBIO EN LA FRECUENCIA DE MUESTREO DE LA SEQAL
C        ---------------------------------------------------------------  
         
         subroutine nuevafrecuencia(if,sen,senam)
         dimension sen(100000),senam(100000)

C        IFN ES LA NUEVA FRECUENCIA A LA QUE SALE LA SEQAL MUESTREADA
C        IFV ES LA FRECUENCIA A LA QUE ESTA MUESTREADA INICIALMENTE

         ifv=if
 5       write(6,10) ifv
 10      format(1x,/,'$ nueva frecuencia_muestreo ? (<',i5,') if= ')
         read(5,*,err=5) ifn
         icon=ifv/ifn
         if=ifv/icon
         j=1
         do i=1,100000,icon
           sen(j)=sen(i)                  
           senam(j)=senam(i)
           j=j+1
         end do
         do i=j,100000
          sen(i)=0
          senam(j)=0
         end do
         return
         end    



