 
C        ==================================================
C          ALDETQRS (ALGORISME DETECCIO QRS)
C          AUTOR: PABLO LAGUNA/RAIMON JANE
C          DATA:  26-MAIG-87
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
      
C        SE LE LLAMA DESDE ECGMAIN PARA DETECTAR LOS QRSs. DE LA SENAL DE 
C        DE ECGs.  
C
C        -------------------------------------------------------------
C          REALIZA UN PRIMER APRENDIZAJE
C        -------------------------------------------------------------
 
         subroutine apren1(ifm,sen,der,psen,psor,umb1,umb2)

c        ESTA SUBROUTINA CALCULA LOS PICOS DE SEQAL Y DE RUIDO INICALES
c        ASI COMO LOS UMBRALES PARA EMPEZAR A TRABAJAR.
c        ifm eS LA FRECUENCIA A LA QUE ESTA MUESTREADA LA SEQAL
c        sen eS EL VECTOR EN EL QUE PASO LA SEQAL
c        der eS EL VECTOR EN EL QUE PASO LA DERIVADA DE LA SEQAL
c        EN psen, psor SACO LOS PICOS DE SEQAL Y RUIDO INICIALES
c        EN n PASO EL INDICE QUYE TENGO QUE TOMAR SEGUN sen SEA
c        ecgpb O ecgmw (1 O 0)
c        EN umb1,umb2 SACO LOS UMBRALES INICIALES PARA DETECTAR LOS qrs
c        ipic ES UN VECTOR QUE GUARDA LAS POSICIONES DE LOS MAXIMOS(PICOS)

         dimension sen(100000),der(100000),ipic(8000)

c        BUSCO LOS PICOS ENTRE 1 SEGUNDO Y 3 SEGUNDOS
         i=ifm
         j=0

	 do while (i.lt.(3*ifm))
c	      write(6,*) ' estoy aqui', i , sen(i), sen(i+1), der(i)
	      call detpic(sen,der,i)
              j=j+1
              ipic(j)=i
         end do
    
 
c        TOMO COMO PICO DE SEQAL EL MAS ALTO DE LOS QUE HAY EN DOS SEGUNDOS
 
         psen=sen(ipic(1))
         do l=2,j
             s=sen(ipic(l))
             if (s.gt.psen) then
                 psen=s
             end if
         end do


c        TOMO COMO PICO DE RUIDO EL MAS ALTO QUE ESTA POR DEBAJO DEL 75% DEL 
c        DE SEQAL

         qrs75=0.75*psen
c         modificado a 0.9
         psor=0.
         do l=1,j
             s=sen(ipic(l))
             if (s.lt.qrs75) then
                 if (s.gt.psor) then
                     psor=s
                 end if
             end if
         end do
                    
c        DEFINO LOS UMBRALES SEGUN ESTA RELACION

         umb1=psor+0.25*(psen-psor)
         umb2=umb1/2

 32      format(1x,'umb1=',f11.3,'umb1=',f11.3)
         return
         end


C        -------------------------------------------------------------
C          REALIZA UN SEGUNDO APRENDIZAJE
C        -------------------------------------------------------------

         subroutine apren2(ifm,sen,der,umb1,irrint)
         dimension sen(100000),der(100000),iqrs(2)


c        ESTA SUBROUTINA CALCULA EL INTERVALO rr INICIAL PARA LUEGO SABER 
c        CUANDO VAMOS DEMASIADO LEJOS BUSCANDO, Y PODAMOS VOLVER ATRAS 

c        Se busca fuera del segundo inicial (i=ifm) para evitar transitorios 
c        del filtro

         i=ifm
         j=1
         irrint=0
         do while (irrint.le.(ifm/3))

c         BUSCO LOS DOS PRIMEROS qrs
          do while (j.lt.3)
               i=i+1
               call detpic(sen,der,i)
               do while (sen(i).le.umb1)
                    i=i+1 
                    call detpic(sen,der,i)
               end do
               iqrs(j)=i
               j=j+1
          end do
          irrint=iqrs(2)-iqrs(1)


c         SI EL INTERVALO ES MUY CORTO PUEDE QUE SEAN ONDAS p Y NO q
c         LO TEXTEO CON LA RUTINA pendent QUE ME DA LA PENDIENTE DE 
c         PICO Y TOMO SOLO LA MAYOR

          if (irrint.le.(ifm/3)) then 
              call pendent(ifm,der,iqrs(2),pemax2)
              call pendent(ifm,der,iqrs(1),pemax1)
              if (pemax1.lt.pemax2) then 
                     iqrs(1)=iqrs(2) 
                                    else
c                    TEXTEO QUE NO SEA UNA ONDA r Y LUEGO UNA p
                     if (pemax2.lt.(pemax1/1.5)) then
                                   irrint=0
                     end if
              end if
              j=j-1
          end if
         end do

         if (irrint.gt.ifm*1.1) then 
             irrint=ifm*1.1         
         end if
         return
         end


C        -------------------------------------------------------------
C          REALIZA LA DETECCION DE LOS PICOS
C        -------------------------------------------------------------

         subroutine detect(ifm,isf,sen,der,psen,psor,umb1,umb2,irrint,
     *                     j,iqrs)
       
c        ESTA SUBROUTINA DETECTA LOS QRS CON LOS DATOS INICIALES DE LAS DE 
c        APRENDIZAJE
c        j ES EL NUMERO DE qrs Y iqrs ES EL VECTOR DONDE ESTAN LAS 
c        POSICIONES
c        onat ES UNA VARIABLE LOGICA QUE DICE SI EL PICO DETECTADO PUEDE 
c        SER ONDA t O NO SEGUN A QUE DISTANCIA DEL qrs ANTERIOR ESTE
c        serbak ES UNA VARIUABLE LOGICA QUE INDICA CUANDO NO HEMOS 
c        ENCONTRADO UN qrs Y POR TANTO HEMOS DE VOLVER ATRAS Y MIRAR CON 
c        UN UMBRAL MAS BAJO.
c        ionat eS LA POSICION HASTA LA CUAL ES POSIBLE EN CADA MOMENTO
c        QUE LA ONDA DETECTADA SEA t         

         dimension sen(100000),der(100000),iqrs(8000)
         logical onat,serbak


         
         j=1

          ionat=nint(ifm/5.5)

c         En el QRS primero no comprobara si es onda T ya que no se sabe donde
c          esta puede aparecer
C         ionat=0
         
c        irrlim ES EL LIMITE HASTA DONDE BUSCO UN qrs DESDE EL ANTERIOR  
c        SIN VOLVER ATRAS EN LA BUSQUEDA

         irrlim=nint(1.5*irrint)
         serbak=.false.    

C        BUSCO EL QRS INICIAL ENTRE 1000 MS. Y 3000 MS.
         mf=int(ifm*3)
         penqrs=0
         penmax=0
         i=ifm
         do while (i.lt.mf)
           call detpic(sen,der,i)
           call pendent(ifm,der,i,pen)
           if ((pen.gt.(penqrs)).and.(sen(i).gt.(umb1/2))) then
c               umb1/2 para que coja posibles QRS de menor amplitud que la T
             penqrs=pen
             psen=sen(i)     
           end if
           if (pen.gt.penmax) then penmax=pen
         end do

c        Por si entre 1 y 2.5 ms no hay picos mayores que umb1
         if (penqrs.eq.0) then penqrs=penmax

c        REDEFINO LOS PICOS DE SEQAL Y DE RUIDO COMO EL VALOR DEL QRS Y 
c        EL MAYOR PICO POR DEBAJO DEL 75% DE ESTE ASI COMO LOS UMBRALES  
        
         psor=0.0        
         mf=2.5*ifm
         i=ifm
         do while (i.lt.mf)
           call detpic(sen,der,i)
           if ((sen(i).lt.psen*0.75).and.(sen(i).gt.psor)) then
             psor=sen(i)
           end if            
         end do          

         umb1=psor+0.25*(psen-psor)
         umb2=umb1/2
    
c        BUSCO YA TODOS LOS qrs  EN EL LIMITE SOLICITADO


c        iqrs_ant es la posicion del QRS anterior al que busco
c        inicialmente supongo que esta en 0 para proteger la busqueda del
c        primer QRS
         iqrs_ant=0

         i=1       
         do while (i.lt.isf)
            i=i+1
            call detpic(sen,der,i)
            if (i.gt.isf) then
                go to 30
            end if
                if ((i-iqrs_ant).gt.irrlim) then
c                   write(6,10) i,j,iqrs(j-1),irrlim,umb1
 10                 format(1x,'no hi ha qrs',/,1x,'i=',i5,2X,'j=',i4,
     *                     '  iqrs=',i5,1x,'irrlim=',i5,'umb1=',f8.4)
                    if (serbak.eqv..true.) then
                    umb1=umb2
                    umb2=umb2/2
		    icounter=icounter+1
		    end if

c                   POR SI LLEGO HA CERO Y NO ENCUENTRO PICO ALARGO EL 
C                   INTERVALO DE BUSQUEDA Y PONGO EL UMBRAL ALTO NUEVAM 
                    if (umb1.lt.0.2) then
		      irrlim=irrlim*1.1
 		      umb1=psor+0.25*(psen-psor)
                      umb2=umb1/2
                    end if

		    if ((icounter.gt.3).and.(irrlim.lt.2.5*irrint)) then
                         irrlim=irrlim*1.5
                    end if
                    i=iqrs(j-1)+ifm/5
                    serbak=.true.
                    psor=0
                    call detpic(sen,der,i)
c                  else
c                    serbak=.false. 
                end if

            picact=sen(i)
            if (picact.gt.umb1) then
                onat=.false.
                if (i.lt.ionat) then
                     call tonat(ifm,der,i,penqrs,onat)
                end if
                if (onat.eqv..false.) then
                    call pendent(ifm,der,i,pen)
                    if ((pen.gt.penqrs*0.65.and.pen.lt.1.4*penqrs)
     *                 .or.(serbak.eqv..true.)) then
                      penqrs=0.8*penqrs + 0.2*pen
                     
c           proteccion para picos muy seguidos menos de 200ms.   
c           y posibles ondas P detectadas como QRS               
                     k=i
                    do while ((k-i).lt.ifm/5)
                     call detpic(sen,der,k)
                     if ((k.lt.isf).and.(k-i.lt.ifm/5)
     *                .and.(sen(k).gt.sen(i))) then
                             i=k
                             picact=sen(i)
                     end if               
                    end do          

                     if (serbak.eqv..false.)  then
                           psen=0.125*picact+0.875*psen
                                         else
                           psen=0.25*picact+0.75*psen
                      end if
                      iqrs(j)=i
                      iqrs_ant=i
                      ionat=i+nint(ifm*0.36)
                      i=i+ifm/5
                      j=j+1
                      if (j.gt.2) then
                       call ajustrr(j,iqrs,irrint,irrlim)
                      end if
                      if (psen.le.psor) then psor=psor/2.
                      umb1=psor+0.25*(psen-psor)
                      umb2=umb1/2
c                     psor=0
                      serbak=.false.
              	      icounter=0
                    end if
               end if
                         else
                           if (picact.gt.psor) then
                                psor=picact
                           end if
            end if
         end do
 30      j=j-1

C        LIMPIO EL RESTO DEL VECTOR
         do i=j+1,8000
          iqrs(i)=0
         end do
   
         return
         end




C        -------------------------------------------------------------
C          REALIZA UN TEXT SOBRE LOS QRS
C        -------------------------------------------------------------

         subroutine buenos(ipos,num)
         dimension ipos(8000)

c        ELIMINO LOS QRS QUE PUEDEN SER DEBIDOS A RUIDO 

c        BUSCO EL INTERVALO MAXIMO DE LOS 10 PRIMEROS
         nmax=ipos(2)-ipos(1)
         do i=1,10
          if ((ipos(i+1)-ipos(i)).gt.nmax)  nmax=ipos(i+1)-ipos(i)
         end do
    
c        COJO LA MEDIA DE LOS INTERVALOS RAZONABLES EN n
c        ls,li sON LOSLIMITES SUPERIOR E INFERIOR PARA CONSIDERAR RAZONABLE   
         ls=nint(nmax*1.1)
         li=nint(nmax*0.8)
         n=0
         j=0
         do i=1,10
          int=ipos(i+1)-ipos(i)
          if ((int.lt.ls).and.(int.gt.li)) then 
           n=n+int
           j=j+1
          end if
         end do
         n=nint(n*1./j)
   
c        ME QUEDO SOLO CON LOS QUE ESTAN SEPARADOS DENTRO DE UNOS LIMITES
         ls=nint(n*1.15)
         li=nint(n*0.85)
         j=2
         do i=1,num-1
          int=ipos(i+1)-ipos(i)
          if ((int.lt.ls).and.(int.gt.li)) then
           ipos(j)=ipos(i+1)
           j=j+1
          end if
         end do
         
C        LIMPIO EL VECTOR RESTANTE DE QRSS
         do i=j,num
          ipos(i)=0
         end do
         
         num=j-1
         return
         end


C        --------------------------------------------------------
C          CALCULA LA AMPLITUD DEL PICO 
C        --------------------------------------------------------

         subroutine amplitud(sen,ipos,num,ampli)

         dimension sen(100000),ipos(8000),ampli(8000)

        do i=1,num        
           ampli(i)=sen(ipos(i))
        end do
 
        do i=num+1,8000
         ampli(i)=0.
        end do

        return
        end


                      
C        --------------------------------------------------------
C          CALCULA LA ANCHURA DEL PICO AL 25% DEL MAXIMO
C        --------------------------------------------------------

         subroutine anchura(sen,ipos,num,anch,ialtura)

         dimension sen(100000),ipos(8000),anch(8000)

        do i=1,num        
         li=ipos(i)
         do while (sen(li).gt.sen(ipos(i))*ialtura/100.)
          li=li-1
         end do     
         
         lf=ipos(i)
         do while (sen(lf).gt.sen(ipos(i))*ialtura/100.)
          lf=lf+1
         end do

         anch(i)=float(lf-li)
        end do

        do i=num+1,8000
         anch(i)=0.
        end do

        return
        end

C        ------------------------------------------------------------
C          REALIZA UN TEXT SOBRE EL VALOR DE 'TEX' DECADA QRS
C        ------------------------------------------------------------


         subroutine buenvalortex(tex,ipos,num,toles,tolei)
         dimension ipos(8000),tex(8000)

C        RECHAZO LOS QRS QUE TENGAN UN VALOR DE TEX ANORMAL POR SI SON 
C        RUIDO
         
         if (num.lt.3) return       

         ampli=0.
         max=tex(1)
         min=tex(1)
         if (num.ge.7) then
              
c                       TOMO LA MEDIA DE LOS 7 PRIMEROS QUE NO SEAN
C                       EL MAS ANCHO NI EL MAS ESTRECHO             
                        do i=1,7
                          ampli=tex(i)+ampli
                          if (tex(i).gt.max) max=tex(i)
                          if (tex(i).lt.min) min=tex(i)
                        end do
                        ampli=(ampli-max-min)/5
C        RECHAZO LOS VALORES QUE ESTAN MUY FUERA DE LA MEDIA 
                        amplit=ampli * 5 + max + min
                        ica=0
                        do i=1,7
                           if (tex(i).gt.ampli*1.5) then
                                                   amplit=amplit-tex(i)
                                                   ica=ica+1
                           end if
                           if (tex(i).lt.ampli*0.5) then
                                                   amplit=amplit-tex(i)    
                                                   ica=ica+1
                           end if
                        end do
                        ampli=amplit/(7-ica)

                       else
                        do i=1,num
                           ampli=tex(i)+ampli
                           if (tex(i).gt.max) max=tex(i)
                           if (tex(i).lt.min) min=tex(i)
                        end do
                        if (num.gt.0)  ampli=(ampli-max-min)/(num-2)           

C        RECHAZO LOS VALORES QUE ESTAN MUY FUERA DE LA MEDIA 
                        amplit=ampli * (num-2) + max + min 
                        ica=0
                        do i=1,num
                           if (tex(i).gt.ampli*1.5) then
                                                   amplit=amplit-tex(i)
                                                     ica=ica+1
                           end if
                           if (tex(i).lt.ampli*0.5) then
                                                  amplit=amplit-tex(i)    
                                                     ica=ica+1
                           end if
                        end do
                        ampli=amplit/(num-ica)
         end if
         

C        RLIMS Y RLIMI SON EL LIMITE INFERIOR Y SUPERIOR PARA CONSIDERAR 
C        VALIDO LA ANCHURA, TOLES,TOLEI SON LAS TOLERANCIAS QUE DOY SUPERIOR
C        E INFERIOR RESPECTIVAMENTE

         alims=ampli*toles
         alimi=ampli*tolei
         
         

         j=1
         do i=1,num
           if ((tex(i).lt.alims).and.(tex(i).gt.alimi)) then
             ipos(j)=ipos(i)
             tex(j)=tex(i)
             if (j.gt.5) then 
              ampli=ampli+(tex(j)-tex(j-5))/5
              alims=ampli*toles
              alimi=ampli*tolei
             end if
             j=j+1
           end if
         end do
         j=j-1   
         
C        LIMPIO EL VECTOR RESTANTE DE QRSS
         do i=j+1,num
          ipos(i)=0
         end do
         
         num=j

         return
         end

         

C        -------------------------------------------------------------
C          REALIZA UNA COFRONTACION DE LOS QRS
C        -------------------------------------------------------------

         subroutine confqrs(ifm,ecgpb,ecgmw,iqrspb,nqrspb,iqrsmw,nqrsmw,
     *                      iqrs,n,ecg)

C        busca que todos los QRS esten en las dos seqales para afirmar 
C        son tales qrs y no son debidos a ruidos.

         dimension iqrspb(8000),iqrsmw(8000),iqrs(8000),ecgpb(100000)
         dimension ecgmw(100000),iqrspb2(8000),iqrsmw2(8000),ampli(8000)
         dimension anch(8000),ecg(100000)

               
	 samp=1000./ifm

C         TOMO iqrspb2,IQRSMW2 IGUALES A LOS QUE ENTRO
          do i=1,8000
           iqrspb2(i)=iqrspb(i)
           iqrsmw2(i)=iqrsmw(i)
          end do
          nqrspb2=nqrspb
          nqrsmw2=nqrsmw

c         call buenos(iqrspb2,nqrspb2)
c         call buenos(iqrsmw2,nqrsmw2)

c          call amplitud(ecgpb,iqrspb2,nqrspb2,ampli)
c          call buenvalortex(ampli,iqrspb2,nqrspb2,1.3,0.7)
c
c          call amplitud(ecgmw,iqrsmw2,nqrsmw2,ampli)
c          call buenvalortex(ampli,iqrsmw2,nqrsmw2,1.3,0.7)

c          call anchura(ecgmw,iqrsmw2,nqrsmw2,anch,20)
c          call buenvalortex(anch,iqrsmw2,nqrsmw2,1.15,0.85)

c        TOMO LOS QRS QUE ESTAN DETECTADOS EN PA Y MW CON EL RETARDO ADECUADO
c	 correccions: agafem com a bons els que o be estiguin en PB o en MW
c	 sempre i quan estiguin a una bona distancia respecte a rrmed
 
 
         l=1
         m=1
         n=1
	 pe1=30./100
	 pe2=50./100
c	 "perc" es un tant per cent del factor de normalitzacio del senyal 
c	 ECGMW
	 perc=1
         do while ((m.le.nqrsmw2).or.(l.le.nqrspb2))
	      call calcula_rmedio ( rrmed, n-1, samp, iqrs)
	      if(m.gt.nqrsmw2) iqrsmw2(m)=100000 
	      if(l.gt.nqrspb2) iqrspb2(l)=100000
              idelay=iqrsmw2(m)-iqrspb2(l)
	      if (abs(idelay).le.pe2*rrmed) then
		  ibe=nint(iqrsmw2(m)-0.1*ifm)
		  ien=nint(iqrsmw2(m)+0.1*ifm)
	         call buscamaxmin(ibe,ien,ecgpb,imi,ymin,ima,ymax)
c            write(6,*) n,imi,ima,iqrsmw2(m),iqrspb2(l),ymax,ymin,idelay
          	   if (abs(ymin).gt.abs(ymax)) then
			iqrs(n)=imi
		     else
			iqrs(n)=ima
                   end if
		 n=n+1
		 l=l+1
		 m=m+1
              else if (idelay.lt.-pe2*rrmed) then
    		 if (iqrsmw2(m)-iqrs(n-1).gt.(1-pe2)*rrmed.or.n.eq.1) then
	           ibe=nint(iqrsmw2(m)-0.1*ifm)
		   ien=nint(iqrsmw2(m)+0.1*ifm)
		   call buscamaxmin(ibe,ien,ecgpb,imi,ymin,ima,ymax)
c	    write(6,*) n,imi,ima,iqrsmw2(m),ymax,ymin
	           if (abs(ymin).gt.abs(ymax)) then
			iqrs(n)=imi
		     else
			iqrs(n)=ima
                   end if
	 	   n=n+1
                   m=m+1
                  else
		   m=m+1
		 end if
	      else if (idelay.gt.pe2*rrmed) then	
		 if (iqrspb2(l)-iqrs(n-1).gt.(1-pe2)*rrmed.or.n.eq.1) then
c 	 fem un test per no detectar ones T molt grans com falsos QRS, en
c	 aquest cas el qrs estara davant coincidint amb un pic de ecgmw
		   if (ecgmw(iqrspb2(l)+nint(95*samp/2)).gt.perc) then 
		     iqrs(n)=iqrspb2(l)
	 	     n=n+1
                     l=l+1
                    else
		     ibe=nint(iqrspb2(l)-250./samp)
		     ien=nint(iqrspb2(l)-50./samp)
		     call buscamaxmin (ibe,ien,ecgmw,iaux,yaux,imax,ymax)
		     if ((ymax.gt.perc).and.(n.gt.1).and.
     &                   (imax.gt.iqrs(n-1)+250/samp)) then 
c	 concretem la posicio al voltant de ima
			ibe=nint(imax-0.05*ifm)
		        ien=nint(imax+0.05*ifm)
		       call buscamaxmin (ibe,ien,ecgpb,imi,ymin,ima,ymax)
		       if (abs(ymin).gt.abs(ymax)) then
			iqrs(n)=imi
		       else
			iqrs(n)=ima
                       end if
	 	       n=n+1
                       l=l+1
                      else
		       l=l+1
		     end if
		   end if
		  else
		   l=l+1
		 end if
	      end if
         end do
         n=n-1
    
c	 do i=1,n
c          write(6,*) i,iqrs(i)
c         end do


         
C        LIMPIO EL RESTO DEL VECTOR
         do i=n+1,8000
          iqrs(i)=0
         end do

         write(6,5) nqrspb,nqrsmw,n 
 5       format(1x,'QRS  en  el ECG:',i4,/,' QRS en la conv.:',i4,/,
     &          ' QRS conformados:',i4)
  
         return
         end



C        -------------------------------------------------------------
C          BUSCA LOS PICOS DE LA SEQAL
C        -------------------------------------------------------------
         

         subroutine detpic(sen,der,i)

C        detecta un pico en la seqal SEN con ayuda de su derivada DER 
C        a partir de una posicion I y lo saca en la variable I
C        DETMAX es una var. logica que dice si hemos detectado un maximo o no
C        DERPOS es una var. logica que dice si la derivada es positiva o no

         dimension sen(100000),der(100000)
         logical detmax,derpos

         detmax=.false.
         do while (der(i).eq.0.)
              i=i+1 
              if (i.gt.39999) then
                  return
              end if
         end do
         if (der(i).gt.0.) then
                        derpos=.true.
                           else
                        derpos=.false.
         end if
         do while (detmax.eqv..false.)
              if (derpos.eqv..true.) then
                  do while (der(i).ge.0.)
                      i=i+1
                      if (i.gt.39999) then
                          return
                      end if
                  end do
                      t=der(i-1)/(-der(i))
                      if (t.lt.1.) then i=i-1
                      detmax=.true.
                                  else
                  do while (der(i).le.0)
                      i=i+1
                      if (i.gt.39999) then
                          return
                      end if
                  end do
                  derpos=.true.
              end if
         end do
         return       
         end


C        -------------------------------------------------------------
C          CALCULA LA PENDIENTE EN UN LUGAR
C        -------------------------------------------------------------


         subroutine pendent(ifm,der,i,pmax)

c        DADO UN VALOR DE LA DERIVADA DE UNA SEQAL der A PARTIR DE UN PUNTO
c        i Y MUESTREADA A UNA FRECUENCIA ifm EN pmax SACO LA PENDIENTE 
c        MAXIMA DE LA SEQAL 80 MS. ANTES DEL PUNTO SOLICITADO QUE NORMAL-
c        MENTE SERA UN MAXIMO SI BIEN NO TIENE POR QUE

         dimension der(100000)

         if (i.gt.100000-80) then 
                              l=100000
                            else 
                              l=i+80
         end if                                   
         pmax=der(l)
         do while ((l.gt.(i-ifm*0.08)).and.(l.gt.0))
            if (abs(der(l)).gt.pmax) then  
              pmax=abs(der(l))
            end if
            l=l-1
         end do
         return
         end


C        -------------------------------------------------------------
C          COMPRUEBA SI ES O NO ONDA T
C        -------------------------------------------------------------

         subroutine tonat(ifm,der,i,penqrs,onat)

c        CON ESTA SUBROUTINA SE SI UN MAXIMO EN UNA POSICION i Y CON UNA 
c        PENDIENTE DEL qrs ANTERIOR DE penqrs ESTE MAXIMO PUEDE SER UNA 
c        ONDA t SI SU PENDIENTE ES MENOR QUE 0.75*penqrs
   
         dimension der(100000)
         logical onat

         call pendent(ifm,der,i,pmax)
         if (pmax.lt.(penqrs*0.75)) then
            onat=.true.
                                 else
            onat=.false.
         end if
         return
         end


C        -------------------------------------------------------------
C          RECALCULA EL INTERVALO RR ACTUAL
C        -------------------------------------------------------------

         subroutine ajustrr(j,iqrs,irrint,irrlim)

c        AJUSTA EL INTERVALO MEDIO DEL VALOR rr SEGUN LOS 5 ULTIMOS VALORES 
c        DE LOS qrs irrint.Y CALCULA EL INTERVALO MEDIO DE LOS MAS USUALES
c        PARA ESTABLECER EL LIMITE DE BUSQUEDA irrlim.
c        j ES EL NUMERO DE qrs QUE HAY HASTA EL MOMENTO
c        iqrs SON LAS POSICIONES QUE OCCUPOAN ESTOS.

         dimension iqrs(8000),irr(8000),irrsel(8000)

         if(j.eq.3) then
             irr(1)=irrint
             irrsel(1)=irrint
             n=1
             nsel=1
             irrbaix=nint(0.92*irrint)
             irralt=nint(1.16*irrint)
         end if
         irr(j-1)=iqrs(j-1)-iqrs(j-2)
         n=n+1
         if ((irr(j-1).gt.irrbaix).and.(irr(j-1).lt.irralt)) then
             nsel=nsel+1
             irrsel(nsel)=irr(j-1)
         end if
         sum=0
         if (n.le.8) then
             do l=1,n
                 sum=irr(l)+sum
             end do
             irrp8u=nint((1./n)*sum)
                     else
             do l=0,7
                 sum=irr(n-l)+sum
             end do
             irrp8u=nint(0.125*sum)
         end if
         sum=0
         if (nsel.le.8) then
             do l=1,nsel
                 sum=irrsel(l)+sum
              end do
             irrpsel=nint(sum/nsel)
                        else
             do l=0,7
                 sum=irrsel(nsel-l)+sum
             end do
             irrpsel=nint(0.125*sum)
         end if
         irrint=irrpsel
         irrbaix=nint(0.92*irrp8u)
         irralt=nint(1.16*irrp8u)                    
         irrlim=nint(1.5*irrpsel)
         return
         end

                                                      

