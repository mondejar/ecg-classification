C       principal.f

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
  
c        -------------------------------------------------------------
c          INICIALIZA VARIABLES A CERO
c        -------------------------------------------------------------

         subroutine inicializa_cero(iondat,iqtc,iqrspa,iqrsmw,iqrs,
     &                              iondaq,iqt)

         dimension iqrspa(8000),iqrsmw(8000),iqrs(8000),iondaq(8000)
         dimension iqt(8000),iondat(8000),iqtc(8000)

         do i=1,8000
          iqrs(i)=0
          iqrspa(i)=0
          iqrsmw(i)=0
          iondat(i)=0
          iondaq(i)=0
          iqt(i)=0
          iqtc(i)=0
         end do

         return
         end                          
 

C        -------------------------------------------------------------
C           OBTIENE TODAS LAS SEQALES INTERMEDIAS PARA DETECTAR QRS
C        -------------------------------------------------------------


         subroutine procesar(index,de_rmax,pa_rmax,pb_rmax,depb_rmax,
     *                             rq_rmax,rmw_rmax,rmd_rmax,
     *            ifm,is,ns,ecg,ecgpa,ecgpb,
     *		ecgder,ecgq,ecgmw,emwder,f,iret_pb,iret_pa, deri)

c        eSTA SUBROUTINA OBTIENE LAS SEQALES INTERMEDIAS NECESARIAS PARA 
c        DETECTAR LOS qrsS. FILTRADAS,...
c       
         real restar, depb_rmax
         integer  index
         character*12  f
         dimension ecg(100000),ecg_off(100000),ecgpa(100000)
         dimension ecgpb(100000)
         dimension ecgq(100000),ecgmw(100000),emwder(100000)
	 dimension deri(100000),ecgder(100000) 
         
 
         isf=(is+ns)*ifm     
	
                  call der(isf,ifm,ecg,deri)
                  if (index.eq.0) then
		      call normaliz_i(ifm,10,de_rmax,deri)
                  else
                   call normaliz_c(ifm,10,de_rmax,deri)
                  end if
			 
 
c             Resto la media de las "if" primeras senales al ecg para 
c             evitar offset grandes que den fuerte respuesta en el filtro
             
c                restar=0
c                do i=1,ifm
c                   restar=restar+ecg(i)
c                enddo
c                restar=restar/ifm
         
c                do i=1,isf 
c                 ecg_off(i)=ecg(i)-restar
c                end do

c                 Aplico dos veces uno de primer orden
                  call fpa(isf,ifm,1,ecg,ECGPA,iret_pa)
c                 call fpa(isf,ifm,1,ecg_off,ECGPA,iret_pa)
c	          iret_pa=2*iret_pa
                  if (index.eq.0) then
                    call normaliz_i(ifm,10,pa_rmax,ecgpa)
c                    write(6,*) 'pa_rmax (norma senyal pa despues i)= ',
c     *                         pa_rmax
                  else
                   call normaliz_c(ifm,10,pa_rmax,ECGPA)
c                  write(6,*) 'pa_rmax (norma senyal pa despues c)= ',
c     *                        pa_rmax
                  end if
                
c                 Aplico dos veces uno de primer orden
                  call fpb(isf,ifm,60,ECGPA,ecgpb,iret_pb)
c                  call fpb(isf,ifm,60,ecg_off,ecgpb,iret_pb)
c	          iret_pb=2*iret_pb
                  if (index.eq.0) then
                    call normaliz_i(ifm,10,pb_rmax,ecgpb)
                  else
                  call normaliz_c(ifm,10,pb_rmax,ECGPB)
                  end if

                   
		  call der(isf,ifm,ecgpb,ecgder)
                  if (index.eq.0) then
		       call normaliz_i(ifm,10,depb_rmax,ecgder)
		  else
		    call normaliz_c(ifm,10,depb_rmax,ECGDER)
                  end if
		  
		  
 
                  call quad(isf,ecgder,ecgq)
                  if (index.eq.0) then
                    call normaliz_i(ifm,10,rq_rmax,ecgq)
                  else
                  call normaliz_c(ifm,10,rq_rmax,ECGQ)
                  end if

                  call mwint(isf,ifm,ecgq,ecgmw)
                  if (index.eq.0) then
                    call normaliz_i(ifm,10,rmw_rmax,ecgmw)
                  else
                  call normaliz_c(ifm,10,rmw_rmax,ECGMW)
                  end if

                  call der(isf,ifm,ecgmw,emwder)
                  if (index.eq.0) then
                    call normaliz_i(ifm,10,rmd_rmax,emwder)
                  else
                  call normaliz_c(ifm,10,rmd_rmax,EMWDER)
                  end if

    
         return
         end


         
C        -------------------------------------------------------------
C           DETECTA LOS QRS
C        -------------------------------------------------------------



         subroutine detectar(index,ifm,is,ns,ecgpb,ecgder,ecgmw,emwder,
     *                  iqrspa,iqrsmw,iqrs,nqrs,f,rmax,pa_rmax,pb_rmax,
     &                mu_an,ecg)

c        eSTA SUBROUTINA LLAMA A LAS QUE SON NECESARIAS PARA DETECTAR LOS
c        qrsS. PREVIO OBTENCION DE LAS SEQALES INTERMEDIAS
c        eSTAS SUBROUTINAS SE ENCUENTRAN EN EL PROGRAMA ALDETQRS.FOR

c        psen eS UN VECTOR QUE TIENE LOS VALORES QUE CONSIDERO COMO 
c        PICOS DE SEQAL psen(1) ES EL PICO DE SEQAL EN ecgpb Y psen(2)
c        EN ecgmw, ASI MISMO psor ES EL VALOR MAS ALTO QUE CONSIDERO 
c        RUIDO EN LAS DOS MISMAS SEQALES.
c        umb1,umb2 SON LOS NIVELES QUE CONSIDERO DEBE DE SOBREPASAR
c        LA SEQAL PARA QUE UN MAXIMO SEA UN qrs. umb1 EN UNA PRIMERA
c        BUSQUEDA y umb2 EN UNA SEGUNDA SI EL PRIMERO ERA DEMASIADO ALTO
c        iqrspa,iqrsmw,iqrs SON RESPECTIVAMENTE LAS POSICIONES DE LOS qrs
c        DETECTADOS EN ecgpb,ECGMW,Y LOS QUE APARECEN EN LOS DOS.

         dimension ecgpb(100000),ecgder(100000),ecgmw(100000)
         dimension emwder(100000)
         dimension iqrs(8000),iqrspa(8000),iqrsmw(8000),ecg(100000)
         character*12 f         
         logical inv

         isf=ifm*(is+ns)
c        
c        index :=0 primera iteracion (hace el test de signo)
c        index :=1 posterior iteracion (usa el resultado del primer test para
c                  invertir o no) variable "inv"
c
         if (index.eq.0) then
          call test(ifm,ecgpb,inv)
         end if
          if (inv.eqv..true.) then
              call invertir(ecgpb,isf)
              call invertir(ecgder,isf)
          end if
         call apren1(ifm,ecgpb,ecgder,psen,psor,umb1,umb2)
         call apren2(ifm,ecgpb,ecgder,umb1,irrint)
         call detect(ifm,isf,ecgpb,ecgder,psen,psor,umb1,umb2,irrint,
     *                nqrspa,iqrspa)
       
c	 write(6,*) ' estoy aqui', ecgmw(255), ecgmw(279)
         
         call apren1(ifm,ecgmw,emwder,psen,psor,umb1,umb2)
         call detect(ifm,isf,ecgmw,emwder,psen,psor,umb1,umb2,irrint,
     *                nqrsmw,iqrsmw)
         call confqrs(ifm,ecgpb,ecgmw,iqrspa,nqrspa,iqrsmw,nqrsmw,iqrs
     *                ,nqrs, ecg)
 	  if (inv.eqv..true.) then
              call invertir(ecgpb,isf)
              call invertir(ecgder,isf)
         end if
  
        
         call escribir(ifm,ecgpb,iqrspa,nqrspa,ecgmw,iqrsmw,nqrsmw,
     *                 iqrs,f,rmax,pa_rmax,pb_rmax)
 
         call escribir_qrs(ifm,nqrs,iqrs,f,rmax,pa_rmax,pb_rmax,mu_an)
	
         return 
         end         


C        -------------------------------------------------------------
C           CREA FICHEROS .QRS DONDE ESTAN LAS POSICIONES DE ESTOS
C        -------------------------------------------------------------

         subroutine escribir_qrs(ifm,nqrs,iqrs,f,rmax,pa_rmax,pb_rmax,
     *                            mu_an)
         
c        cREA UN FICHERO *.QRS EN EL QUE ESTAN LAS POSICIOPNES DE LOS 
c        qrs DESPUES DE SELECCIONAR LOS QUE CONSIDERO VALIDOS COMO TAL

         dimension iqrs(8000)
         character*12 f
                                                             
         write(21,10) f
10       format(3x,'n',2x,'posicion',2x,a7)
         write(21,*)
         write(21,11) ifm
11       format(3x,'frecuencia de muestreo =',i4)
         write(21,*) '    posicion en milisegundos'
         write(21,*)   
         do i=1,nqrs
          anum=(iqrs(i)+mu_an)*1000/ifm
          write(21,15) i,anum
15        format(2x,i4,4x,F12.0)     
         end do             
         write(21,1) rmax
 1       format(1x,'RMAX =',F12.3,'   Factor d''escala senyal')
         write(21,2) pa_rmax
 2       format(1x,'PA_RMAX =',F12.3,'   Factor d''escala senyal
     *   passa_alt')
         write(21,3) pb_rmax
 3       format(1x,'PB_RMAX =',F12.3,'   Factor d''escala senyal
     *   passa_baix')

         return
         end



C        -------------------------------------------------------------
C           CREA FICHEROS .QPM DONDE ESTAN LOS QRS DE Pb Y MW
C        -------------------------------------------------------------

         subroutine escribir(ifm,ecgpb,iqrspa,nqrspa,ecgmw,iqrsmw,
     *                      nqrsmw,iqrs,f,rmax,pa_rmax,pb_rmax)

c        eSCRIBO EN UN FICHERO *.qpm LAS POSICIONES DEL qrs. DETECTADOS 
c        EN LAS DOS SEQALES EN LAS QUE MIRO.

         dimension ecgpb(100000),ecgmw(100000),iqrspa(8000)
         dimension iqrsmw(8000)
         dimension iqrs(8000)
         character*12 f

          
         nmax=nqrspa
         if (nqrsmw.gt.nmax) nmax=nqrsmw

           
        
         write(20,10) f
10       format(3x,'n',2x,'posicion pa',2x,'intervalo',2x,'valor',
     *           4x,'posicion mw',2x,'intervalo',3x,'valor',7x,'retar',
     *           '  n',4x,'posi-qrs','  paciente = ',a7)          
         write(20,*)
         write(20,11) ifm
11       format(3x,'frecuencia de muestreo =',i4)
         write(20,*)
       
         do i=1,nmax
         write(20,15) i,iqrspa(i),iqrspa(i+1)-iqrspa(i),
     *                ecgpb(iqrspa(i)),iqrsmw(i),iqrsmw(i+1)-iqrsmw(i),
     *                ecgmw(iqrsmw(i)),iqrsmw(i)-iqrspa(i),i,iqrs(i)
15       format(2x,i4,4x,i9,6x,i9,4x,f9.3,5x,i9,6x,i9,4x,f9.3,4x,i9,5x,
     *          i4,5x,i9)     
         end do
     
         write(20,1) rmax
 1       format(1x,'RMAX =',F12.3,'   Factor d''escala senyal')
         write(20,2) pa_rmax
 2       format(1x,'PA_RMAX =',F12.3,'   Factor d''escala senyal 
     *   passa_alt')
         write(20,3) pb_rmax
 3       format(1x,'PB_RMAX =',F12.3,'   Factor d''escala senyal 
     *   passa_baix')
         return
         end
                     

C        -------------------------------------------------------------
C           DEFINE EL COMIENZO Y FIN DE LA ONDA Q Y T RESPECTIVAMENTE
C        -------------------------------------------------------------

         subroutine calcula_qt(ecgpb,ecgder,iqrs,iondaq,iondat,
     &                         ifm,nqrs,f,iqt,iqtc)

C        BUSCA Y DEFINE EL PRINCIPIO Y FIN DE Q Y T SEGUN DISTINTOS 
C        ALGORITMOS

         dimension ecgpb(100000),ecgder(100000),iqrs(8000)
         dimension iondaq(8000)
         dimension iondat(8000),iqt(8000),iqtc(8000)
         character*12 f

         write(6,*) ' que algoritmo quieres usar?'
         write(6,*) '   1-fuerte inflexion'
         write(6,*) '   2-umbral de la derivada'
 5       write(6,10)
 10      format('$         OPCION:')
         read(5,*,err=5) iask
         
         call int_qt(ecgpb,ecgder,iqrs,iondaq,iondat,ifm,nqrs,
     &               iqt,iqtc,iask)
         call escribe_qt(f//'.qt',ifm,iqt,nqrs)
         call escribe_qt(f//'.qtc',ifm,iqtc,nqrs)
         return
         end        
                       
C        -------------------------------------------------------------
C           CREA FICHEROS .prn PARA IMPRIMIR POR LA PRINTRONIX
C        -------------------------------------------------------------

         subroutine impregraf(f,ifm,ecg,ecgpa,ecgpb,ecgder,ecgq,ecgmw,
     &		emwder,elba,ecglb,iqrs,iqrspa,iqrsmw,ipbeg,ippos1,
     &		ippos2,ipend, iqbeg, iqpos, ispos, isend, itbeg, itpos,
     &		itend, derfi, irrpos, iqend, isbeg, basel, itpos2)

c        eSTA SUBROUTINA CREA UN FICHERO IMPRIMIBLE POR UNA IMPRESORA 
c        pRINTRONIX. QUE REPRESENTA LAS SEQALES SELECCIONADAS, HACIENDO 
c        DE LA LIBRERIA DE RUTINAS GRAFICAS graflpt.Y DE RUTINAS DEL 
c        PROGRAMA GRAF.FOR. eL FICHERO QUE CREA SE LLAMA *.prn
c        
         dimension ecg(100000),ecgpa(100000),ecgpb(100000)
         dimension ecgder(100000)
         dimension ecgq(100000),ecgmw(100000),emwder(100000)
         dimension der1f(100000)
         dimension elba(100000),ecglb(100000),ecg_off(100000)
         dimension iqrs(8000),iondat(8000),iondaq(8000)
         dimension iqrspa(8000)
         dimension der2(100000),derfi(100000),iqrsmw(8000)
         dimension ipbeg(8000),ippos1(8000),ippos2(8000)
         dimension ipend(8000)
	 dimension iqbeg(8000),itpos(8000)  
	 dimension iqpos(8000),ispos(8000),isend(8000),itbeg(8000)
	 dimension itend(8000),irrpos(8000),iqend(8000),isbeg(8000)
         dimension basel(8000),itpos2(8000)
         character*12 f 
	 character*1 siono, siono2     

         iask=2 
         call pedir(is,ns)
         nptssr=is*ifm
         npts=ns*ifm

         do while (iask.ne.0)
           write(6,*)
           write(6,*)'se crea un fichero  (sen.prn) de cada seqal'
           write(6,*)'2 ecg      3 ecgpa       4 ecgpb        5 ecgder'
           write(6,*)'6 ecgq     7 ecgmw       8 emwder       9 derfil'
           write(6,*)'1 der2    10 lba        11 ecglb        0 ninguna'
 15        write(6,20)
 20        format('$ OPCION:')
           read(5,*,err=15) iask
         
          if (iask.ne.0) then
 30         write (6,40)
 40         format(/'$ Hi vols les marques dels punts', 
     &             ' significatius ? [s/n]')      
            read (5,'(A)',err=30) siono

 42         write (6,45)
 45         format(/'$ Hi vols les marques de la lmnia de base? [s/n]') 
            read (5,'(A)',err=42) siono2
          end if
         
        if (iask.eq.2) then
         call sacagraf(f(1:lnblnk(f))//'_ecg','s',ecg,nptssr,npts,
     &			ifm,izero,iaskt,escy)
          if (siono2.eq.'s') then 
	   call l_saca_basel(basel, iqrs, npts, nptssr, izero, escy)
          end if
         else if (iask.eq.3) then
          call sacagraf(f(1:lnblnk(f))//'_pa','s',ecgpa,nptssr,npts,
     &			ifm,izero,iaskt,aux)
               else if (iask.eq.4) then
         call sacagraf(f(1:lnblnk(f))//'_pb','s',ecgpb,nptssr,npts,
     &			ifm,izero,iaskt,aux)
               else if (iask.eq.5) then
         call sacagraf(f(1:lnblnk(f))//'_der','s',ecgder,nptssr,npts,
     &			ifm,izero,iaskt,aux)
               else if (iask.eq.6) then
         call sacagraf(f(1:lnblnk(f))//'_q','s',ecgq,nptssr,npts,
     &			ifm,izero,iaskt,aux)
               else if (iask.eq.7) then
         call sacagraf(f(1:lnblnk(f))//'_mw','s',ecgmw,nptssr,npts,
     &			ifm,izero,iaskt,aux)
         call marca(iqrsmw,npts,nptssr,150,175,iaskt)
               else if (iask.eq.8) then
         call sacagraf(f(1:lnblnk(f))//'_mder','s',emwder,nptssr,npts,
     &			ifm,izero,iaskt,aux)
               else if (iask.eq.9) then
c           call fpb(ifm*(is+ns),ifm,30,ecgder,der1f,iret_pb)
c           call normaliz(ifm,10,der1f)
         call sacagraf(f(1:lnblnk(f))//'_dfi','s',derfi,nptssr,npts,
     &			ifm,izero,iaskt,aux)
               else if (iask.eq.10) then
         call sacagraf(f(1:lnblnk(f))//'_lba','s',elba,nptssr,npts,
     &		       ifm,izero,iaskt,aux)
               else if(iask.eq.1) then
           call fpb(ifm*(is+ns),ifm,30,ecgder,der1f,iret_pb)
c           call fpb(ifm*(is+ns),ifm,30,ecg_off,der1f,iret_pb)
c	   iret_pb=2*iret_pb
           call der(ifm*(is+ns),ifm,der1f,der2)
           call normaliz(ifm,10,der2)
         call sacagraf(f(1:lnblnk(f))//'_der2','s',der2,nptssr,npts,
     &			ifm,izero,iaskt,aux)
               else if (iask.eq.11) then
         call sacagraf(f(1:lnblnk(f))//'_ecglb','s',ecglb,nptssr,npts,
     &		       ifm,izero,iaskt, aux)

               end if                                                    

c        Les marques dels punts significatius no cal que es representin si no es 
c        vol
        if (siono.eq.'s') then
         call marca(iqrs,npts,nptssr,150,175,iaskt)
         call marca(irrpos,npts,nptssr,150,175,iaskt)
         call marca(ipbeg,npts,nptssr,izero+150,izero+50,iaskt)
         call marca(ippos1,npts,nptssr,izero+50,izero+25,iaskt)
         call marca(ippos2,npts,nptssr,izero+50,izero+25,iaskt)
	 call marca(ipend,npts,nptssr,izero+150,izero+50,iaskt)                
         call marca(iqbeg,npts,nptssr,izero+150,izero+50,iaskt)
         call marca(iqpos,npts,nptssr,izero+175,izero+150,iaskt)
c         call marca(iqend,npts,nptssr,izero+175,izero+150,iaskt)
c         call marca(isbeg,npts,nptssr,izero+175,izero+150,iaskt)
         call marca(ispos,npts,nptssr,izero+175,izero+150,iaskt)
         call marca(isend,npts,nptssr,izero+150,izero+50,iaskt)
         call marca(itbeg,npts,nptssr,izero+150,izero+50,iaskt)     
         call marca(itpos,npts,nptssr,izero+175,izero+150,iaskt)     
         call marca(itpos2,npts,nptssr,izero+50,izero+25,iaskt)     
         call marca(itend,npts,nptssr,izero+150,izero+50,iaskt)
        end if
c 
c         CERRAR EL FICHERO
          if (iask.eq.0) return
         call l_prinplot(1,1,2,2)

          CLOSE(2)
         end do    

         return
         end


C        -------------------------------------------------------------
C           REPRESENTA POR PANTALLA VT240
C        -------------------------------------------------------------

         subroutine pantagrafold(ifm,ecg,ecgpa,ecgpb,ecgder,ecgq,ecgmw,
     &   emwder,elba,iqrspa,iqrsmw,iqrs,ipbeg, ippos1, ippos2, ipend,
     &		     iqbeg, iqpos, ispos, isend, itbeg, itpos, itend,
     &		derfi, irrpos, iqend, isbeg,  is, ns,kon)  
                     
c        eSTA SUBROUTINA REPRESENTA POR PANTALLA vt240 LAS SEQALES ELECTRO-
c        CARDIOGRAFICAS, CON SU ESCALADO Y LOS qrs DETECTADOS EN ecgpb Y 
c        ecgmw ASI COMO AQUELLOS QUE SELECCIONO COMO VALIDOS, PONIENDOLES 
c        UNA DOBLE MARCA. uSO TAMBIEN RUTINAS DEL PROGRAMA GRAF.FOR
c        
         dimension ecg(100000),ecgpa(100000),ecgpb(100000)
         dimension ecgder(100000)
         dimension ecgq(100000),ecgmw(100000),emwder(100000),iqrs(8000)
         dimension elba(100000)
         dimension iqrspa(8000),iqrsmw(8000),iondaq(8000),iondat(8000)
         dimension ecgder2(100000), derfi(100000)
         dimension ipbeg(8000),ippos1(8000),ippos2(8000),ipend(8000)
	 dimension iqbeg(8000)
	 dimension iqpos(8000),ispos(8000),isend(8000),itbeg(8000)
	 dimension itpos(8000)
	 dimension itend(8000), irrpos(8000), iqend(8000), isbeg(8000)
  	 character*1 espera
         
         if (kon.gt.0) then
	 call salgraf
	 call pedir(is,ns)
         call inicializa
	 end if
c        representacion por pantalla
                  call inicializa
                  call dibujaold(ifm,ecg,is,ns,100,'  ecg',80.)
                  call pintqrs(iqrs,ifm,is,ns,30,15) 
c                  call pintqrs(irrpos,ifm,is,ns,50,10) 
c                  call pintqrs(ipbeg,ifm,is,ns,125,50)
c                  call pintqrs(ippos1,ifm,is,ns,75,10)
c                  call pintqrs(ippos2,ifm,is,ns,75,10)
c                  call pintqrs(ipend,ifm,is,ns,125,50)
c                  call pintqrs(iqbeg,ifm,is,ns,125,50)
c                  call pintqrs(iqpos,ifm,is,ns,150,15)
c                  call pintqrs(iqend,ifm,is,ns,135,15)
c                  call pintqrs(isbeg,ifm,is,ns,135,15)
c                  call pintqrs(ispos,ifm,is,ns,150,15)
c                  call pintqrs(isend,ifm,is,ns,125,50)
c                  call pintqrs(itbeg,ifm,is,ns,125,50)
c                  call pintqrs(itpos,ifm,is,ns,75,10)
c                  call pintqrs(itpos,ifm,is,ns,135,10)
c                  call pintqrs(itend,ifm,is,ns,125,50)
c                  call dibujaold(ifm,ecgpa,is,ns,190,'ecgpa',3.)
                  call dibujaold(ifm,ECGPB,is,ns,275,'ECGPB',6.0)
                  call pintqrs(iqrspa,ifm,is,ns,225,15) 
c                  call pintqrs(irrpos,ifm,is,ns,225,15) 
c                  call pintqrs(ipbeg,ifm,is,ns,300,50)
c                  call pintqrs(ippos1,ifm,is,ns,250,10)
c                  call pintqrs(ippos2,ifm,is,ns,250,10)
c                  call pintqrs(ipend,ifm,is,ns,300,50)
c                  call pintqrs(iqbeg,ifm,is,ns,300,50)
c                  call pintqrs(iqpos,ifm,is,ns,325,15)
c                  call pintqrs(iqend,ifm,is,ns,310,15)
c                  call pintqrs(isbeg,ifm,is,ns,310,15)
c                  call pintqrs(ispos,ifm,is,ns,325,15)
c                  call pintqrs(isend,ifm,is,ns,300,50)
c                  call pintqrs(itbeg,ifm,is,ns,300,50) 
c                  call pintqrs(itpos,ifm,is,ns,250,10)
c                  call pintqrs(itpos,ifm,is,ns,310,10)  
c                  call pintqrs(itend,ifm,is,ns,300,50)
                  call dibujaold(ifm,ecgmw,is,ns,450,'ecgmw',6.0)
                  call pintqrs(iqrsmw,ifm,is,ns,380,15) 
c                  call dibujaold(ifm,derfi,is,ns,450,'ecgder',6.0)
c                  call pintqrs(iqrs,ifm,is,ns,400,15) 
c                  call pintqrs(irrpos,ifm,is,ns,400,15) 
c                  call pintqrs(ipbeg,ifm,is,ns,475,50)
c                  call pintqrs(ippos1,ifm,is,ns,425,10)
c                  call pintqrs(ippos2,ifm,is,ns,425,10)
c                  call pintqrs(ipend,ifm,is,ns,475,50)
c                  call pintqrs(iqbeg,ifm,is,ns,475,50)
c                  call pintqrs(iqpos,ifm,is,ns,500,15)
c                  call pintqrs(iqend,ifm,is,ns,485,15)
c                  call pintqrs(isbeg,ifm,is,ns,485,15)
c                  call pintqrs(ispos,ifm,is,ns,500,15)
c                  call pintqrs(isend,ifm,is,ns,475,50)
c                  call pintqrs(itbeg,ifm,is,ns,475,50)
c		  call pintqrs(itpos,ifm,is,ns,425,10)
c		  call pintqrs(itpos,ifm,is,ns,485,10)
c                  call pintqrs(itend,ifm,is,ns,475,50)
 
	  		   
c                  call der(if*(is+ns),ifm,ecgder,ecgder2)
c                  call normaliz(ifm,10,ecgder2)
c                  call dibujaold(ifm,ecgder2,is,ns,360,'ecgd2',30.)

                  call salgraf         
 5	 write(6,10)
 10	 format('$ [CR]:')
	 read(5,'(a)',err=5) espera                  


                                    
         return
         end

C        -------------------------------------------------------------
C           REPRESENTA POR PANTALLA VT240
C        -------------------------------------------------------------

          subroutine pantagraf(ifm,ecg,ecgpa,ecgpb,ecgder,ecgq,ecgmw,
     &	  emwder,elba,ecglb,iqrspa,iqrsmw,iqrs,ipbeg, ippos1, ippos2, 
     &	       ipend,iqbeg, iqpos, ispos, isend, itbeg, itpos, itend,
     &		derfi,irrpos,iqend,isbeg,nqrs,basel,deri,itpos2,i_base)  
                    
c        eSTA SUBROUTINA REPRESENTA POR PANTALLA vt240 LAS SEQALES ELECTRO-
c        CARDIOGRAFICAS, CON SU ESCALADO Y LOS qrs DETECTADOS EN ecgpb Y 
c        ecgmw ASI COMO AQUELLOS QUE SELECCIONO COMO VALIDOS, PONIENDOLES 
c        UNA DOBLE MARCA. uSO TAMBIEN RUTINAS DEL PROGRAMA GRAF.FOR
c        
         dimension ecg(100000),ecgpa(100000),ecgpb(100000)
         dimension ecgder(100000)
         dimension ecgq(100000),ecgmw(100000),emwder(100000),iqrs(8000)
         dimension elba(100000),ecglb(100000),i_base(8000)
         dimension iqrspa(8000),iqrsmw(8000),iondaq(8000),iondat(8000)
         dimension ecgder2(100000),derfi(100000),deri(100000)
         dimension ipbeg(8000),ippos1(8000),ippos2(8000),ipend(8000)
	 dimension iqbeg(8000),basel(8000)
	 dimension iqpos(8000),ispos(8000),isend(8000),itbeg(8000)
	 dimension itpos(8000)
	 dimension itend(8000),irrpos(8000), iqend(8000), isbeg(8000)
         dimension auxba(8000),itpos2(8000)
         

         call pedir(is,ns)
	 
c	 iy=3*fesc*33
	 jgain=200
	 iy=240          
	 k=1
	 kon=0
	 samp=1000./ifm
	 call baseline(nqrs,ippos1,ecg,basel,samp,ipbeg,ipend,iqbeg,
     &                 isend,i_base)     
	 do i=1,nqrs
	   auxba(i)=0
	 end do
	 do while(k.ne.0)      
	 write(6,30)     
 30 	 format (///,t28,'OPCIONS GRAFIQUES',//)        
	 write(6,35)
 35 	 format (
     & t10,'1 ecg          5 derfi         9 ecg, ecgpb, derfi'
     &/t10,'2 ecgpa        6 lba          10 ecg, ecgpb, lba'  
     &/t10,'3 ecgpb        7 deri         11 ecg, ecgpb, ecgmw'
     &/t10,'4 ecgder       8 ecglb'//       
     & t10,'               0 sortir')
 39	 write(6,40)
 40      format(//,'$',t20,'OPCIO: ')
         read(5,41,err=39) k
 41      format(I2)         
	 if (k.eq.0) return
	 if ((k.ne.9).and.(k.ne.10).and.(k.ne.11)) then
            write(6,10)
 10         format(2x,/,'$ Introdueix factor de escala [1.]: ')
            read(5,*,err=15) fesc
c 11        format(f3.1)  
 15         if (fesc.eq.0.0) fesc=1.       
	 end if
c        representacion por pantalla

         call inicializa
	 if(k.eq.1) then
         call dibuja(ifm,ecg,is,ns,iy,' ecg',fesc,ipbeg,ippos1,ipend,
     &		iqbeg,iqpos,iqend,iqrs,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend, nqrs,basel,itpos2)
	 else if(k.eq.2)then
         call dibuja(ifm,ecgpa,is,ns,iy,'ecgpa',fesc,ipbeg,ippos1,ipend,
     &		iqbeg,iqpos,iqend,iqrs,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend, nqrs,auxba,itpos2)
	 else if(k.eq.3)then
         call dibuja(ifm,ecgpb,is,ns,iy,'ecgpb',fesc,ipbeg,ippos1,ipend,
     &		iqbeg,iqpos,iqend,iqrs,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend, nqrs,auxba,itpos2)
	 else if(k.eq.4)then
        call dibuja(ifm,ecgder,is,ns,iy,'ecgde',fesc,ipbeg,ippos1,ipend,
     &		iqbeg,iqpos,iqend,iqrs,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend, nqrs,auxba,itpos2)
	 else if(k.eq.5)then
         call dibuja(ifm,derfi,is,ns,iy,'derfi',fesc,ipbeg,ippos1,ipend,
     &		iqbeg,iqpos,iqend,iqrs,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend, nqrs,auxba,itpos2)
	 else if(k.eq.6)then
       call dibuja(ifm,elba,is,ns,iy,'lin_base',fesc,ipbeg,ippos1,ipend,
     &		iqbeg,iqpos,iqend,iqrs,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend, nqrs,auxba,itpos2)
	 else if(k.eq.7)then
         call dibuja(ifm,deri,is,ns,iy,'deri',fesc,ipbeg,ippos1,ipend,
     &		iqbeg,iqpos,iqend,iqrs,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend, nqrs,auxba,itpos2)
	 else if(k.eq.8)then
        call dibuja(ifm,ecglb,is,ns,iy,'ecg-lb',fesc,ipbeg,ippos1,ipend,
     &		iqbeg,iqpos,iqend,iqrs,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend, nqrs,auxba,itpos2)

 
	 else if(k.eq.9) then
         if (kon.gt.0) then
	 call salgraf
	 call pedir(is,ns)
         call inicializa
	 end if
c        representacion por pantalla
                  call inicializa
                  call dibujaold(ifm,ecg,is,ns,100,'ecg',45.0)
                  call pintqrs(iqrs,ifm,is,ns,30,15) 
                  call dibujaold(ifm,ECGpb,is,ns,250,'ECGpb',7.0)
                  call dibujaold(ifm,derfi,is,ns,400,'derfi',30.0)   
                  call pintqrs(irrpos,ifm,is,ns,50,10) 
                  call pintqrs(ipbeg,ifm,is,ns,125,50)
                  call pintqrs(ippos1,ifm,is,ns,75,10)
                  call pintqrs(ipend,ifm,is,ns,125,50)
                  call pintqrs(iqbeg,ifm,is,ns,125,50)
                  call pintqrs(iqpos,ifm,is,ns,150,15)
                  call pintqrs(iqend,ifm,is,ns,135,15)
                  call pintqrs(isbeg,ifm,is,ns,135,15)
                  call pintqrs(ispos,ifm,is,ns,150,15)
                  call pintqrs(isend,ifm,is,ns,125,50)
                  call pintqrs(itbeg,ifm,is,ns,125,50)
                  call pintqrs(itpos,ifm,is,ns,75,10)
                  call pintqrs(itpos,ifm,is,ns,135,10)
                  call pintqrs(itend,ifm,is,ns,125,50)
                                                          
                  call salgraf         
 97	 write(6,98)
 98	 format('$ [CR]:')
	 read(5,'(a)',err=97) espera                  


	 else if(k.eq.10) then
         if (kon.gt.0) then
	 call salgraf
	 call pedir(is,ns)
         call inicializa
	 end if
c        representacion por pantalla
                  call inicializa
                  call dibujaold(ifm,ecg,is,ns,100,'ecg',45.0)
                  call pintqrs(iqrs,ifm,is,ns,30,15) 
                  call dibujaold(ifm,elba,is,ns,250,'lin_b',45.0)   
                  call dibujaold(ifm,ECGpb,is,ns,400,'ECGpb',7.0)
                  call salgraf         
 99	 write(6,100)
 100	 format('$ [CR]:')
	 read(5,'(a)',err=99) espera                  

	 else if(k.eq.11) then
         if (kon.gt.0) then
	 call salgraf
	 call pedir(is,ns)
         call inicializa
	 end if
c        representacion por pantalla
                  call inicializa
                  call dibujaold(ifm,ecg,is,ns,100,'  ecg',45.)
                  call pintqrs(iqrs,ifm,is,ns,30,15) 
                  call dibujaold(ifm,ECGPB,is,ns,275,'ECGPB',6.0)
                  call pintqrs(iqrspa,ifm,is,ns,225,15) 
                  call dibujaold(ifm,ecgmw,is,ns,450,'ecgmw',6.0)
                  call pintqrs(iqrsmw,ifm,is,ns,380,15) 
                  call salgraf         
 110	 write(6,120)
 120	 format('$ [CR]:')
	 read(5,'(a)',err=110) espera                 
         end if

         non=kon+1
	 end do
	 return
	 end

C        -------------------------------------------------------------
C           TOMO EL SEGUNDO INCIAL Y EL NUMERO DE SEG. A PROCESAR
C        -------------------------------------------------------------

         subroutine pedir(is,ns)

c        SOLICITA EN EL PUNTO QUE SE LLAMA EL SEGUNDO INICIAL Y EL NUMERO
c        DE SEGUNDOS CON LOS QUE QIERO TRABAJAR.

          write(6,*)
 5        write(6,10) 
 10       format('$ segundo inicial is= ')
          read(5,*,err=5) is
          write(6,*)
 15       write(6,20)
 20       format('$ numero de segundos ns= ')
          read(5,*,err=15) ns

          return
          end


C        -------------------------------------------------------------
C           REPRESENTA LA FUNCION DE TRANFERENCIA DE LOS FILTROS
C        -------------------------------------------------------------

         subroutine funtrans(ifm,fcb,fca)

         dimension fpb(100000),fpa(100000),der2(100000),fpt(100000)
         character*9 blancos
         character*1 ask 

C        rEPRESENTA Y CREA UN FICHERO SI SE QUIERE DE LAS FUNCIONES DE 
C        TRANSFERENCIA DEL FILTRO PASO BANDA Y DE LA DERIVADA        
c        fcb eS LA FREC. DE CORTE DEL fpb, fca DEL fpa, flr ES LA MAYOR 
c        FRECUENCIA REPRESENTADA
c        lb eS EL NUMERO DE CEROS DEL fpb. Y la DEL fpa. 
         
         lb=nint(ifm/fcb)
         la=nint(ifm/fca)
         
         DO I=1,1500
c         DO I=1,1000
         FPB(I)=PB(I,lb,ifm)
c         FPA(I)=la-PB1(I,la,ifm)
         FPA(I)=(la*la)-PB(I,la,ifm)
         FPT(I)=FPB(I)*FPA(I)
       DER2(I)=(ifm/4)*(SIN(4*I*3.1416/10/ifm)+2*SIN(2*3.1416*I/10/ifm))
         END DO
         CALL NORMALIZ(1000,50,FPB)
         CALL NORMALIZ(1000,50,FPA)
c         CALL NORMALIZ(1000,50,DER2)
         CALL NORMALIZ(1500,50,DER2)
         call normaliz(1000,50,fpt)


                  call inicializa
                  call dibujaold(100,fpt,0,10,220,'hz/10',3.)
                  call dibujaold(100,der2,0,10,440,'der  ',3.)
                  call salgraf

         write(6,5)
 5       format('$ Quieres copia por impresora? [n]: ')
         read(5,'(A)') ask
         if (ask.eq.'s'.or.ask.eq.'S') then  
           write(6,*)
           write(6,*) '1 printronix    2 laser_writer '
 10        write(6,15)
 15        format('$ OPCION:')
           read(5,*,err=10) iaskt
   
            iask=1
            do while (iask.ne.0)
             write(6,*)
             write(6,*) 'quieres crear un fichero para impresora ?'
             write(6,*) '1 fil-pb      2 fil-pa     3 deriv'
             write(6,*) '4 fil-pt      0 salir'
 20          write(6,25)
 25          format('$ OPCION:')
             read(5,*,err=20) iask
          
            BLANCOS='      '
             if (iask.eq.1) then                     
c          call sacagraf('           fil_pb','s',fpb,0,1000,10,
c     &                  izero,iaskt, aux)
          call sacagraf('           fil_pb','s',fpb,0,150,10,
     &                  izero,iaskt, aux)
            else if (iask.eq.2) then 
c          call sacagraf('           fil_pa','s',fpa,0,1000,10,
c     &                  izero,iaskt, aux)
          call sacagraf('           fil_pa','s',fpa,0,150,10,
     &                  izero,iaskt, aux)
            else if (iask.eq.3) then  
c          call sacagraf('            deriv','s',der2,0,1000,10,
c     &                  izero,iaskt, aux)
          call sacagraf('            deriv','s',der2,0,1500,10,
     &                  izero,iaskt, aux)
            else if (iask.eq.4) then 
          call sacagraf('           fil_pt','s',fpt,0,1000,10,
     &                  izero,iaskt, aux)
            end if            
            if (iask.eq.0) return
            if (iaskt.eq.1) then
C                             call prinplot(1,1,2,2)
                            else
                             call l_prinplot(1,1,2,2)
            end if
            close(2) 
            end do
         end if
         return
         end
                                                                        
c        ---------------------------------------------------------------

         real function pb(l,n,ifm)

c        ES UNA FUNCION QUE CALCULA EL VALOR DE LA FUNCION DE TRANSFERENCIA 
c        DEL FILTRO PASO BAJO PARA FRECUENCIA DE MUESTREO ifm, NUMERO DE CEROS
c        n, y l	FRECUENCIA DE LA SEQAL, de segundo orden
  
         r=sin(3.1416*l/10/ifm)
         if (r.Eq.0.) then
            pb=n**2
         else
         pb=(sin(n*3.1416*l/10/ifm))**2/(sin(3.1416*l/10/ifm))**2
         end if
         return 
         end
                                                                        
c        ---------------------------------------------------------------

         real function pb1(l,n,ifm)

c        eS UNA FUNCION QUE CALCULA EL VALOR DE LA FUNCION DE TRANSFERENCIA 
c        DEL FILTRO PASO BAJO PARA FRECUENCIA DE MUESTREO ifm, NUMERO DE CEROS
c        n, y l	FRECUENCIA DE LA SEQAL, de primer orden 
  
         r=sin(3.1416*l/10/ifm)
         if (r.Eq.0.) then
            pb1=n
         else
         pb1=abs( sin(n*3.1416*l/10/ifm) / sin(3.1416*l/10/ifm) )
         end if
         return 
         end
                                                                        
C        -----------------------------------------------------------------
C         RUTINA PARA REPRESENTAR QT
C        ----------------------------------------------------------------

         subroutine  representar_qt(ifm,is,ns,iqrs,iqt,iqtc,nqrs,f)            
 
         dimension iqrs(8000),iqt(8000),iqtc(8000)
         character*1 ask
         character*12 f
                 
         call grafica_qt(ifm,is,ns,iqrs,iqt,iqtc)     
         write(6,5)   
 5       format('$ [CR]:')     
         read(5,'(A)') espera
         write(6,*)
         write(6,10)
 10       format('$ Quieres copia para impresora? [n]: ')
         read(5,'(A)') ask
         if (ask.eq.'s'.or.ask.eq.'S') then
             write(6,*) 
             write(6,*) '1 impresora    2 laser_writer'
 15          write(6,20)
 20          format('$ OPCION:')
             read(5,*,err=15) iask
             if (iask.eq.2) then    
                 call l_grafica_qt(f//'_qt',ifm,is,ns,iqrs,iqt,nqrs)
                 call l_grafica_qt(f//'_qtc',ifm,is,ns,iqrs,iqtc,nqrs)
                            else
                 call p_grafica_qt(f//'_qt',ifm,is,ns,iqrs,iqt,nqrs)
                 call p_grafica_qt(f//'_qtc',ifm,is,ns,iqrs,iqtc,nqrs)
             end if
         end if

         return
         end


C        ----------------------------------------------------------------
C         SACAGRAF BIEN POR LA PRINTRONIX BIEN POR LA LASER
C        ---------------------------------------------------------------- 

         subroutine sacagraf(titgen,lizer,sen,nptssr,npts,ifm,
     &                          izero,iaskt,escy)

         character*17 titgen
         character*1 lizer
         dimension sen(100000) 
         integer*4 npts
C       
C        iaskt VALE 1 PARA PRINTRONIX Y 2 PARA LASER

          call sacalaser(titgen,lizer,sen,nptssr,npts,ifm,izero,
     &                  escy)

         return
         end

C        ---------------------------------------------------------------
C          MARCA SOBRE LOS GRAFICOS
C        --------------------------------------------------------------

         subroutine marca(ipos,npts,nptssr,iyi,iyf,iaskt)  
         dimension ipos(8000)

         if (iaskt.eq.1) then
                        call pinta_marca(ipos,npts,nptssr,iyi,iyf)
                         else
                        call l_pinta_marca(ipos,npts,nptssr,iyi,iyf)
         end if
   
         return
         end                             
c      -------------------------------------------------------------
c          Inicializa ventana en el QRS
c      -------------------------------------------------------------
    
           subroutine inicializa_ven_qrs(f,f1,f2,f3,npos_r,ior,
     &                                 ianchs_r,ianchp_r)

            character*12 f,f1,f2,f3

            write(6,*)
 1          write(6,2)
 2          format('$ Especifica modificacio  punt QRS: ')
            read(5,*,err=1) npos_r                                      
            ior=3     
            write(6,*)
            do while (ior.ne.0.and.ior.ne.1.and.ior.ne.2)
              write(6,*) 'Selecciona el filtrat realitzat al senyal: '
              write(6,*) '     0   Filtre passa alt  (fo =  1 Hz)'
              write(6,*) '     1   Filtre passa banda  (fo = 60 Hz)'
              write(6,*) '     2   ECG original '
 5            write(6,6)
 6            format('$                OPCION:')
              read(5,*,err=5) ior
            end do                                                              
           if (ior.eq.0) then
             open(unit=7,file=f(1:lnblnk(f))//'fin_pa.qrs')
            else if (ior.eq.1) then
             open(unit=7,file=f(1:lnblnk(f))//'fin_pb.qrs')
            else if (ior.eq.2) then
             open(unit=7,file=f(1:lnblnk(f))//'fin.qrs')
           end if
c           open(unit=17,file=f1(1:lnblnk(f1))//'fin.qrs')
C            open(unit=27,file=f2(1:lnblnk(f2))//'fin.qrs')
C            open(unit=37,file=f3(1:lnblnk(f3))//'fin.qrs')
                            
            write(6,*)
 10         write(6,11)
 11          format('$ anchura ventana sincronismo en R (muestras): ')
            read(5,*,err=10) ianchs_r
 12          write(6,13)
 13          format('$ anchura ventana promediado en R (muestras): ')  
            read(5,*,err=12) ianchp_r
          return
          end
                              
           
c      -------------------------------------------------------------
c          Inicializa ventana en la onda P
c      -------------------------------------------------------------
    
         subroutine inicializa_ven_p(f,f1,f2,f3,npos_p,iop,ianchs_p,
     &                                 ianchp_p)
                    
         character*12 f,f1,f2,f3
            write(6,*)
 2          write(6,3)
 3          format('$ selecciona distancia P-R: ')
            read(5,*,err=2) npos_p
            iop=3     
            write(6,*)
            do while (iop.ne.0.and.iop.ne.1.and.iop.ne.2)
              write(6,*) 'Selecciona el filtrat realitzat al senyal: '
              write(6,*) '     0   Filtre passa alt  (fo =  1 Hz)'
              write(6,*) '     1   Filtre passa banda  (fo = 60 Hz)'
              write(6,*) '     2   ECG original '
 5            write(6,6)
 6            format('$                OPCION:')
              read(5,*,err=5) iop
            end do                                                              
            if (iop.eq.0) then
              open(unit=8,file=f(1:lnblnk(f))//'fin_pa.p')
             else if (iop.eq.1) then
              open(unit=8,file=f(1:lnblnk(f))//'fin_pb.p')
             else if (iop.eq.2) then
              open(unit=8,file=f(1:lnblnk(f))//'fin.p')
            end if
c            open(unit=18,file=f1//'fin.p')   
C            open(unit=28,file=f2(1:lnblnk(f2))//'fin.p')   
C            open(unit=38,file=f3(1:lnblnk(f3))//'fin.p')   
                            
            write(6,*)
 9          write(6,10)
 10         format('$ anchura ventana sincronismo en P (muestras): ')
            read(5,*,err=9) ianchs_p
 14         write(6,15)
 15         format('$ anchura ventana promediado en P (muestras): ')  
            read(5,*,err=14) ianchp_p
          return
          end
                              
c      -------------------------------------------------------------
c          Inicializa ventana en la onda T
c      -------------------------------------------------------------
    
         subroutine inicializa_ven_t(f,f1,f2,f3,npos_t,iot,ianchs_t,
     &                               ianchp_t)
                    
         character*12 f,f1,f2,f3
            write(6,*)
 2          write(6,3)
 3          format('$ selecciona distancia T-R: ')
            read(5,*,err=2) npos_t
            iot=3     
            write(6,*)
            do while (iot.ne.0.and.iot.ne.1.and.iot.ne.2)
              write(6,*) 'Selecciona el filtrat realitzat al senyal: '
              write(6,*) '     0   Filtre passa alt  (fo =  1 Hz)'
              write(6,*) '     1   Filtre passa banda  (fo = 60 Hz)'
              write(6,*) '     2   ECG original '
 5            write(6,6)
 6            format('$                OPCION:')
              read(5,*,err=5) iot
            end do                                                              
            if (iot.eq.0) then    
              open(unit=9,file=f(1:lnblnk(f))//'fin_pa.t')
            else if (iot.eq.1) then    
              open(unit=9,file=f(1:lnblnk(f))//'fin_pb.t')
            else if (iot.eq.2) then    
              open(unit=9,file=f(1:lnblnk(f))//'fin.t')
            end if
c            open(unit=19,file=f1(1:lnblnk(f1))//'fin.t')   
c            open(unit=29,file=f2(1:lnblnk(f2))//'fin.t')   
c            open(unit=39,file=f3(1:lnblnk(f3))//'fin.t')   
                            
            write(6,*)
 9          write(6,10)
 10         format('$ anchura ventana sincronismo en T (muestras): ')
            read(5,*,err=9) ianchs_t
 14         write(6,15)
 15         format('$ anchura ventana promediado en T (muestras): ')  
            read(5,*,err=14) ianchp_t
          return
          end
                              


c       -------------------------------------------------------------
c           Inizialixa ventanas
c      --------------------------------------------------------------

         subroutine inizializa_ven(f,f1,f2,f3,ipqrs,ior,iop,iot,
     &   npos_r,npos_p,npos_t,ianchs_r,ianchp_r,ianchs_p,
     &                 ianchp_p,ianchs_t,ianchp_t)        
                                                       
c        ipqrs: selecciona ventana entornoa P o a QRS
c        iop: si ipqrs es en P iop dice que seqal guardo; filtrada paso
c             alto o paso banda
c        npos: distancia del QRS al centro de la ventana de sincronismo
c        ianchs: anchura de la ventana de sincronismo
c        ianchp: anchura de la ventana de promediado

         character*12 f,f1,f2,f3
         ipqrs=0                                            
         do while ((ipqrs.lt.1).or.(ipqrs.gt.7))
          write(6,*)
 5        write(6,*) ' ventana en: QRS     [1]      P [2]    QRS-P [3]'
          write(6,*) '             QRS-P-T [4]      T [5]    P-T   [6]'
          write(6,*) '             QRS-T   [7]                        :'
          write(6,10)
 10       format('$ OPCION:')
          read(5,*,err=5) ipqrs                           
          write(6,*)
         end do
         if (ipqrs.eq.1.or.ipqrs.eq.3
     &       .or.ipqrs.eq.4.or.ipqrs.eq.7) then
         call inicializa_ven_qrs(f,f1,f2,f3,npos_r,ior,
     &                           ianchs_r,ianchp_r)
         end if
         if (ipqrs.eq.2.or.ipqrs.eq.3
     &       .or.ipqrs.eq.4.or.ipqrs.eq.6) then
         call inicializa_ven_p(f,f1,f2,f3,npos_p,iop,ianchs_p,ianchp_p)
         end if
               
         if (ipqrs.eq.4.or.ipqrs.eq.5
     &       .or.ipqrs.eq.6.or.ipqrs.eq.7) then
         call inicializa_ven_t(f,f1,f2,f3,npos_t,iot,ianchs_t,ianchp_t)
         end if
                            
                                
          return
          end                        


c       ---------------------------------------------------------------
c         CREA VENTANAS PARA PROMEDIAR
C       ---------------------------------------------------------------

c        Crea ventanas segun especificaciones que pide

          subroutine crea_ventanas(ecg,ecgpa,ecgpb,ecgam1,ecgam2,
     &               ecgam3,iqrs,nqrs,ifm,
     &              is,ns,ipqrs,npos_r,npos_p,npos_t,ior,iop,iot,
     &                     ianchs_r,ianchp_r,ianchs_p,ianchp_p,
     &                     ianchs_t,ianchp_t,iret_pb,iret_pa) 
                                                                     
          dimension ecg(100000),ecgpa(100000),ecgpb(100000)
          dimension ecgam1(100000), ecgam2(100000),ecgam3(100000)  
          dimension iqrs(8000)

     
          if (ipqrs.eq.1.or.ipqrs.eq.3
     &        .or.ipqrs.eq.4.or.ipqrs.eq.7) then 
            ianchi=ianchs_r/2
            ianchd=ianchp_r - ianchi                                           
            if (ior.eq.0) then
              call crefinqrs(7,1,ecgpa,iqrs,nqrs,ifm,is,ns,ior,
     &             ianchi,ianchi,ianchp_r,npos_r,iret_pa,iret_pb)
            else if (ior.eq.1) then
              call crefinqrs(7,1,ecgpb,iqrs,nqrs,ifm,is,ns,ior,
     &              ianchi,ianchi,ianchp_r,npos_r,iret_pa,iret_pb)
            else if (ior.eq.2) then
              call crefinqrs(7,1,ecg,iqrs,nqrs,ifm,is,ns,ior,
     &               ianchi,ianchi,ianchp_r,npos_r,iret_pa,iret_pb)
            end if
c          call crefinqrs(17,2,ecgam1,iqrs,nqrs,ifm,is,ns,ior,
c    &                ianchd,ianchi,ianchp_r,npos_r,iret_pa,iret_pb)
c            call crefinqrs(27,2,ecgam2,iqrs,nqrs,ifm,is,ns,ior,
c     &                ianchd,ianchi,ianchp_r,npos_r,iret_pa,iret_pb)
c            call crefinqrs(37,2,ecgam3,iqrs,nqrs,ifm,is,ns,ior,
c     &                 ianchd,ianchi,ianchp_r,npos_r,iret_pa,iret_pb)
           end if

           if (ipqrs.eq.2.or.ipqrs.eq.3
     &         .or.ipqrs.eq.4.or.ipqrs.eq.6) then
            ianchi=ianchs_p/2
            ianchd=ianchp_p - ianchi                                           
            if (iop.eq.0) then
              call crefinp(8,1,ecgpa,iqrs,nqrs,ifm,is,ns,npos_p,iop,
     &                          ianchi,ianchi,ianchp_p,iret_pb,iret_pa)     
             else    if (iop.eq.1) then
              call crefinp(8,1,ecgpb,iqrs,nqrs,ifm,is,ns,npos_p,iop,
     &                          ianchi,ianchi,ianchp_p,iret_pb,iret_pa)     
             else    if (iop.eq.2) then
              call crefinp(8,1,ecg,iqrs,nqrs,ifm,is,ns,npos_p,iop,
     &                          ianchi,ianchi,ianchp_p,iret_pb,iret_pa)     
            end if
c           call crefinp(18,2,ecgam1,iqrs,nqrs,ifm,is,ns,npos_p,iop,
c    &                          ianchd,ianchi,ianchp_p,iret_pb,iret_pa)     
C            call crefinp(28,2,ecgam2,iqrs,nqrs,ifm,is,ns,npos_p,iop,
C     &                          ianchd,ianchi,ianchp_p,iret_pb,iret_pa)     
C            call crefinp(38,2,ecgam3,iqrs,nqrs,ifm,is,ns,npos_p,iop,
C     &                          ianchd,ianchi,ianchp_p,iret_pb,iret_pa)     
          end if

          if (ipqrs.eq.4.or.ipqrs.eq.5
     &        .or.ipqrs.eq.6.or.ipqrs.eq.7) then
            ianchi=ianchs_t/2
            ianchd=ianchp_t - ianchi                                           
            if (iot.eq.0) then
              call crefinp(9,1,ecgpa,iqrs,nqrs,ifm,is,ns,npos_t,iot,
     &                        ianchi,ianchi,ianchp_t,iret_pb,iret_pa)     
            else if (iot.eq.1) then
              call crefinp(9,1,ecgpb,iqrs,nqrs,ifm,is,ns,npos_t,iot,
     &                        ianchi,ianchi,ianchp_t,iret_pb,iret_pa)     
            else if (iot.eq.2) then
              call crefinp(9,1,ecg,iqrs,nqrs,ifm,is,ns,npos_t,iot,
     &                        ianchi,ianchi,ianchp_t,iret_pb,iret_pa)     
            end if
c            call crefinp(19,2,ecgam1,iqrs,nqrs,ifm,is,ns,npos_t,iot,
c     &                      ianchd,ianchi,ianchp_t,iret_pb,iret_pa)     
c            call crefinp(29,2,ecgam2,iqrs,nqrs,ifm,is,ns,npos_t,iot,
c     &                      ianchd,ianchi,ianchp_t,iret_pb,iret_pa)     
c            call crefinp(39,2,ecgam3,iqrs,nqrs,ifm,is,ns,npos_t,iot,
c     &                      ianchd,ianchi,ianchp_t,iret_pb,iret_pa)     
          end if
                      
         return
         end
C        ---------------------------------------------------------------
C          CREACIO FITXER DE FINESTRES QRS (200 punts)
C        ---------------------------------------------------------------

C          Creacio d'una finestra de 200 punts [1-200], amb el valor
C          del QRS detectat posicionat a I=100.

           subroutine crefinqrs(iu,ifor,sen,iqrs,nqrs,ifm,is,ns,ior,
     &                    ianchd,ianchi,ianchp,npos,iret_pa,iret_pb) 
                                   
c          iu: canal de escritura
c          ianchd: anchura de la ventana a la derecha de QRS
c          ianchi: anchura de de la ventana a la izquierda del QRS

           dimension sen(100000),iqrs(8000)

                                                               
           l=1
           if(iqrs(1)-npos.lt.ianchi) then
              l=2       
           end if
           do i=l,nqrs-1
               inifin=iqrs(i)-npos-ianchi
               if (ifor.eq.1) then
                               do j=inifin,inifin+ianchi+ianchd-1
                                write(iu,1) sen(j)
                               end do
                              else if (ifor.eq.2) then
                               do j=inifin,inifin+ianchi+ianchd-1
                                write(iu,2) int(sen(j))
                               end do
               end if
           end do        
           if (ior.eq.0) then    
             iretard=iret_pa
            else  if (ior.eq.1) then    
             iretard=iret_pb + iret_pa
            else  if (ior.eq.2) then    
             iretard=0 
           end if        
  
           if ((iqrs(nqrs)-npos+ianchp-ianchi+iretard).lt.((is+ns)*ifm)
     &          .and.(nqrs.gt.0)) then
               inifin=iqrs(nqrs)-npos-ianchi
               if (ifor.eq.1) then
                               do j=inifin,inifin+ianchi+ianchd-1
                                write(iu,1) sen(j)
                               end do
                              else if (ifor.eq.2) then
                               do j=inifin,inifin+ianchi+ianchd-1
                                write(iu,2) int(sen(j))
                               end do
               end if
           end if
  1                format(1x,f8.4)
  2                format(1x,I6)

           return
           end


C        ---------------------------------------------------------------
C          ESCRIU FINESTRES P (200 punts) AMB EL FILTRAT SELECCIONAT
C        ---------------------------------------------------------------

C          Creacio d'una finestra de 200 punts [1-200], centrat a la
C          posicio del QRS menys NPOS (seleccionat en l'execucio).
C          Els valors recomenats soc: 120ms (PAC8) i 150ms (PAC1).
C          Considera el senyal filtrat passa alt o passa banda.

           subroutine crefinp(iu,ifor,sen,iqrs,nqrs,ifm,is,ns,npos,iop,
     &                          ianchd,ianchi,ianchp,iret_pb,iret_pa)     
           dimension sen(100000),iqrs(8000)


           l=1
           if(iqrs(1)-npos.lt.ianchi) then
              l=2
           end if
           do i=l,nqrs-1
               inifin=iqrs(i)-npos-ianchi
               if (ifor.eq.1) then
                               do j=inifin,inifin+ianchd+ianchi-1
                                write(iu,1) sen(j)
                               end do
                  else if (ifor.eq.2) then 
                               do j=inifin,inifin+ianchd+ianchi-1
                                write(iu,2) int(sen(j))
                               end do
               end if
           end do
           if (iop.eq.0) then    
             iretard=iret_pa
            else  if (iop.eq.1) then    
             iretard=iret_pb + iret_pa
            else  if (iop.eq.2) then    
             iretard=0 
           end if
           if ((iqrs(nqrs)-npos+ianchp-ianchi+iretard).lt.((is+ns)*ifm)
     &          .and.(nqrs.gt.0)) then
               inifin=iqrs(nqrs)-npos-ianchi
               if (ifor.eq.1) then
                               do j=inifin,inifin+ianchd+ianchi-1
                                write(iu,1) sen(j)
                               end do
                  else if (ifor.eq.2) then 
                               do j=inifin,inifin+ianchd+ianchi-1
                                write(iu,2) int(sen(j))
                               end do
               end if
           end if

  1                format(1x,f8.4)
  2                format(1x,I6)

           return
           end       
                     

