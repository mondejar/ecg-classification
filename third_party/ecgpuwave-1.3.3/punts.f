 
C	 ======================================================================
C 	 punts.f                                                              =
C	 SUBROUTINES QUE ENS TROBEN TOTS ELS PUNTS SIGNIFICATIUS DEL          =
C      	 ECG A PARTIR DE LES POSICIONS DELS QRS I DEL SENYAL FILTRAT I DERIVAT=
c         Eudald Bogatell    (1-6-91)
c         David Vigo Anglada (  9-92)
C         Last modified: 24-2-2006 (by G. Moody, to avoid compiler warnings)
C	 ======================================================================
	 
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

         subroutine p_significatius(dbuf,jqrs,iqrs,if,is,ns,f,ipbeg, 
     &		ippos1, ipend, iqbeg, iqpos, ispos, isend, itbeg, 
     &		itpos, itend, irpos, ecgpb, derfi, irrpos, iqend,isbeg,
     &		rrmed, ecg, deri, itpos2, morf, idis, nep)
	 
c	 Aquesta subrutina es cridada desde ECGMAIN com una opcio, i ens
c	 troba tots els punts significatius de les ones Q,S,T, i P.

	 
	 dimension dbuf(100000),irpos(8000),ecgpb(100000),iqrs(8000)
	 dimension ipbeg(8000),ippos1(8000),ippos2(8000),ipend(8000),iqbeg(8000)
	 dimension iqpos(8000),ispos(8000),isend(8000), itbeg(8000),itpos(8000)
	 dimension itend(8000), derfi(100000), deri(100000), basel(8000)
	 dimension irrpos(8000), iqend(8000), isbeg(8000), ecg(100000)   
	 dimension itpos2(8000), morf(5,8000), idis(3,20),eauxs(100000)
	 dimension ecg_off(100000)
	 logical comp_qs, eonaq, eonas

c	 isf: mostra final a tractar
c	 dbuf: vector que conti el senyal filtrat i derivat
c	 ecgpb: senyal ECG filtrat a passa banda
c	 ecg: senyal ECG
c	 deri: senyal ECG derivat
c	 derfi: senyal dbuf filtrat
c 	 jqrs: nombre de qrs detectats i confirmats que tractarem
c 	 samp: interval de mostreig en ms
c	 iqrs: posicio dels complexes QRS detectats i confirmats
c	 irpos: vector que conti la posicio de l'ona R
c 	 ipbeg, ippos, ipend: inici, pic i final de l'ona P
c 	 iqbeg, iqpos, iqend: inici, pic i final de l'ona Q
c 	 isbeg, ispos, isend: inici, pic i final de l'ona S
c 	 itbeg, itpos, itpos2, itend: inici, pics i final de l'ona T
c 	 basel: conte les linies de base de cada QRS
c	 idis: conte els posibles episodis de VT i VF
c	 nep: nombre d'episodis de VT i VF
c	 comp_qs: es certa si es detecta un complexe QS
c	 eonaq: es certa si es detecta ona Q
c	 eonas: es certa si es detecta ona S




	 kr=5                        
c	 per definir el principi i final de l'ona R
	 isf=(is+ns)*if
	 samp=1000.0/if
c	 inicialitzem rrmed
	 rrmed=(iqrs(jqrs)-iqrs(1))/(jqrs-1)
	 n=1
         do i=1,300
		iqbeg(i)=0
		iqpos(i)=0
		iqend(i)=0
		irpos(i)=0
		irrpos(i)=0
		isbeg(i)=0
		ispos(i)=0
		isend(i)=0
		ipbeg(i)=0
		ippos1(i)=0
		ipend(i)=0
		itbeg(i)=0
		itpos(i)=0
		itpos2(i)=0
		itend(i)=0
	 end do

c	   Apliquem un filtre passa baix de 2 grau a la derivada per tal de
c 	   eliminar les components de alta frequencia i determinar millor 
c	   les ones P i T.
c	   write(6,*) n, 'antes de los filtros pb'
	   call fpb(isf, if, 40, ecgpb, eauxs, retard)
c           call fpb(isf, if, 40, ecg_off, eauxs, retard)
c	   retard=2*retard
           call normaliz_i(if,10,daux,eauxs)
           call der(isf, if, eauxs,derfi)
           call normaliz_i(if, 2, df_rmax, derfi)
c           write(6,*) if, df_rmax, derfi(1),derfi(100000)

	
	 jep=1	 
c	 QRS que estem tractant
 	  do while(n.le.jqrs.and.isf-iqrs(n).gt.500/samp)
c         afegim una proteccio en el cas de que el iqrs(n) sigui igual a 
c         cero; cas que es pot donar provenint de la subrutina multideriv, 
c         en les derivacions que s'hi ha detectat un fals negatiu.
          if (iqrs(n).ne.0) then
c	 si el qrs cau dins dun episodi de VT o VF no hi busquem cap complexe
c	 QRS
	   if (nep.gt.0.and.iqrs(n).ge.(idis(1,jep)+is)*if-if*0.3.and.
     &		jep.le.nep) then
	     if (iqrs(n).le.(idis(2,jep)+is)*if) then
	     	irpos(n)=iqrs(n)
		go to 100
	      else
		jep=jep+1
	     end if
	   end if	
		
c	   write(6,*) n, 'antes de onar'	
	   call onar(dbuf, ecgpb, iqrs, irpos, irrpos, iqbeg,iqpos, 
     &		ispos, isend, iqend, isbeg, n, samp, ecg,comp_qs,ymaxaux)
c	   write(6,*) n, 'despues de onar'
c	   busquem el pic anterior i posterior a la posicio de la ona R
	   call busca_pic2(irpos(n), dbuf, ima, 'i')
	   ymax=dbuf(ima)
	   call busca_pic2(irpos(n), dbuf, imi, 'd')
	   ymin=dbuf(imi)
c	 protegim per R amples on la derivada ti molts pics pero que la r
c	 sigui significativa
           if (.not.comp_qs) then
	   if (ymax.gt.ymaxaux/4) then
	   inicia=irpos(n)-nint(70/samp)
	   if (inicia.lt.0) inicia=0
	   call buscamaxmin(inicia,irpos(n),
     &	   	dbuf, iaux, yaux, imaa, ymaxa)
	   ilim=irpos(n)+nint(70/samp)
	  call buscamaxmin(irpos(n),ilim,dbuf, imia, ymina, iaux, yaux)
	   if (ymaxa.gt.ymax) then 
		ymax=ymaxa
		ima=imaa
	   end if
	   if (ymina.lt.ymin) then
c	 write(6,*) n, iqrs(n),irpos(n), ima,'primero'
 
		ymin=ymina
		imi=imia
	   end if
	   end if
	   if (abs(ymax).gt.abs(ymin)) then
	       	 dermax=abs(ymax)
	       else
	 	 dermax=abs(ymin)
	   end if

c	 i encara afegim unaltra proteccio si no tenim una Q o S molt marcada
c	 es a dir amb un pic de la derivada gran.
	    ilim=ima-nint(70/samp)
	    ilim2=ima-nint(30/samp)
	   if (ymax.gt.ymaxaux/4) then
	  call buscamaxmin(ilim,ilim2,dbuf, iaux, yaux, imaa, ymaxa)
	   if (ymaxa.gt.dermax/5) then
c	   write(6,*) n, iqrs(n),irpos(n), ima,'ultimo'

		ymax=ymaxa
		ima=imaa
 	   end if
            ilim=imi+nint(40/samp)
	    ilim2=imi+nint(100/samp)
 	   call buscamaxmin(ilim,ilim2,dbuf, imia, ymina, iaux, yaux)
	   if (ymina.lt.-1*dermax/5) then 
		ymin=ymina
		imi=imia
	   end if
	   end if
c	   write(6,*) n, iqrs(n),irpos(n), ima
	   else 
c	 tractem els complexes QS
	   inicia=irpos(n)-nint(150/samp)
	   if (inicia.lt.0) inicia=0
	   call buscamaxmin(inicia,irpos(n),
     &	   	dbuf,imi, ymin ,iaux, yaux)
	   ilim=irpos(n)+nint(180/samp)
	  call buscamaxmin(irpos(n),ilim,dbuf, iaux, yaux, ima, ymax)

c          write(6,2) imin,imax 
c 2 	   format('posicion QS=',i4,'isend=',i4 )
	
	   if (abs(ymax).gt.abs(ymin)) then
	       	 dermax=abs(ymax)
	       else
	 	 dermax=abs(ymin)
	   end if

c	 busquem el principi del QS amb el umbral de la derivada 
	   umbral=ymin/Kr
	   call creuar_umbral (dbuf, umbral, imi, iqbeg(n), 'i')
c          comprobem que el complexe QS no comengi amb un petit pic de pujada.
c          si fos aixi no l'hauriem detectat i l'inici trobat fora dolent
c          perque no el tenia amb compte. L'umbral ser` a Kr=3 i no 5 perque
c          is un petit pic ara.
	   ilim=iqbeg(n)-nint(35/samp)
           call buscamaxmin(ilim,   
     &                       iqbeg(n),dbuf,iaux,yaux,ima2,ymax2)
c	  write(6,*) ymax2, dermax
           if (ymax2.ge.dermax/30) then
               imi=ima2
               umbral=ymax2/2
    	       call creuar_umbral (dbuf, umbral, imi, iumb2, 'i')
	       if (iumb2.gt.iqbeg(n)-30/samp) iqbeg(n)=iumb2
           end if
	   if (-yaux.ge.dermax/30.and.iaux.lt.ima2) then
               imi=iaux
               umbral=yaux/2
    	       call creuar_umbral (dbuf, umbral, imi, iumb2, 'i')
	       if (iumb2.gt.iqbeg(n)-50/samp) iqbeg(n)=iumb2
            end if


c	 Buscamos el final del complejo QS
c	write(6,*) n,  ' QS'

	   umbral=ymax/kr
	   call creuar_umbral (dbuf, umbral, ima, isend(n), 'd')
	    ilim=irpos(n)+nint(180/samp)
 	   if (isend(n)-iqbeg(n).lt.80/samp) then
	    call buscamaxmin(irpos(n),ilim,
     &          dbuf,iaux,yaux,ima2,ymax2)
	    if (ymax2.gt.ymax) then
		umbral=ymax2/kr
		call creuar_umbral (dbuf, umbral, ima2, isend(n), 'd')
	    end if
	   end if

c          Ajuste para picos a la derecha de un QS 
           ilim=isend(n)+nint(20/samp)
           call buscamaxmin(isend(n),ilim,dbuf,imin2,ymin2,iaux,yaux)
c	   write(6,*) ymax2, dermax
           if (-ymin2.ge.dermax/20) then
               umbral=ymin2/2
    	       call creuar_umbral (dbuf, umbral, imin2, iumb2,'d')
	       if (iumb2.lt.isend(n)+30/samp) isend(n)=iumb2
           end if
	   

c           write(6,2) irpos(n),isend(n) 
c 2 	   format('posicion QS=',i4,'isend=',i4 )
	
	   end if


           eonaq=.true.
	   eonas=.true.
          
c	   write(6,*) n, 'antes de onaq'	
	   if (irrpos(n).ne.0.or.comp_qs) then
		go to 40
	   end if
 20	   call onaq (dbuf, dermax, ima, irpos, n, iqbeg, iqpos,
     &		 	 iqend, samp, ecg, deri, eonaq, itend,iqrs) 
c	   write(6,*) n, 'antes de onas'	
 30	   call onas (dbuf, dermax, imi, irpos, n, isend, ispos,
     &		      isbeg, samp, ecg, iqbeg, deri, eonas,iqrs)
c          write(6,*) n, 'antes de onap'	
 40	   call onap (n, derfi, isf, irpos, iqbeg, ipbeg, ippos1,
     &                ipend, samp, if, dermax, itend, ecgpb, ecg,iqrs)
c          write(6,*)  n, 'despues de onap'	
              
 	   call cal_base(n,ippos,ecg,basel,samp,ipend,iqbeg)     
c	 trobem el final de la Q i el principi de la S en el creuament de
c	 la linea de base.
	   
	   if (iqpos(n).ne.0.and..not.comp_qs.and.irrpos(n).eq.0) then
	      if (ecg(irpos(n)).gt.0) then
		 call creuar_umbral (ecg, basel(n), iqpos(n), iqend(n), 'd')
              else
		 iqend(n)=irpos(n)
	      end if
c	   si hem anat mes enlla de irpos no existeix la ona Q
	      if ((irpos(n)-iqend(n).lt.0).and.
     &            (iqrs(n).gt.iqpos(n)+10/samp) ) then 
		 iqbeg(n)=0
		 iqpos(n)=0
		 iqend(n)=0
		 eonaq=.false.
		 go to 20
	      end if
	   end if
	   if (ispos(n).ne.0.and..not.comp_qs.and.irrpos(n).eq.0) then
	   if (ecg(irpos(n)).gt.0) then
             call creuar_umbral (ecg, basel(n), ispos(n), isbeg(n), 'i')
	     else
              isbeg(n)=irpos(n)
	    end if

c	   si hem anat mes enlla de irpos no existeix la ona S
	   if ( (irpos(n)-isbeg(n).gt.0).and.
     &          (iqrs(n).lt.ispos(n)-10/samp) ) then 
		isbeg(n)=0
		ispos(n)=0
		isend(n)=0
		eonas=.false.
		go to 30
	   end if
	   end if
           call calcula_rmedio(rrmed, n, samp, iqrs)
	  
c            write(6,*) n, 'antes de onat'	
	   call onat (n, derfi, rrmed, isf, irpos, iqbeg, itbeg, itpos,
     &	   itpos2,itend, samp, if, isend, ecg, basel, morf,iqrs) 
c	   call onatold (n, derfi, rrmed, isf, irpos, iqbeg, itbeg, itpos,
c     &				itend, samp, if, isend,iqrs) 
	   if (irpos(n).ne.0) then 
	 	if(comp_qs) then
		   call test_pic (ecg, irpos(n), samp, 'v')
                 else
	           call test_pic (ecg, irpos(n), samp, 'p')
		end if
	   end if
          end if
 100	  n=n+1
	 end do

c	 omplim el reste dels vectors a cero
	 do i=jqrs+1,300
	    irpos(i)=0
	    ipbeg(i)=0
	    ippos1(i)=0
	    ippos2(i)=0
	    ipend(i)=0
	    iqbeg(i)=0
	    iqpos(i)=0
	    iqend(i)=0
	    isbeg(i)=0
	    ispos(i)=0
	    isend(i)=0
	    itbeg(i)=0
	    itpos(i)=0
	    itend(i)=0
	    irrpos(n)=0
	 end do

c	 a continuacio tenim unes ordres que permeten escriure en uns fitxers
c	 les mostres que es desitgin de tots els senyals de cara a treure'n

	 return
	 end


c-----------------------------------------------------------------------------
  
	 subroutine buscamaxmin (ib, ie, date, imi, ymin, ima, ymax)

c	 Ens treu per min i max els minim i maxim valors del senyal date
c	 entre les posicions ib i ie. Les posicions del min i del max ens
c	 les treu per imi i ima respectivament.

	 dimension date(100000)
	
         ima=ib
	 imi=ib
	 ymax=date(ib)
	 ymin=date(ib)
	 i=ib
	 do while (i.lt.ie)
	   if (date(i).ge.ymax) then
		ymax=date(i)
		ima=i
	   end if
	   if (date(i).le.ymin) then
		ymin=date(i)
		imi=i
	   end if
	   i=i+2
	 end do
	 if (date(imi-1).lt.ymin) then
	     ymin=date(imi-1)
	     imi=imi-1
	 end if
	 if (date(imi+1).lt.ymin) then
	     ymin=date(imi+1)
	     imi=imi+1
	 end if
	 if (date(ima-1).gt.ymax) then
	     ymax=date(ima-1)
	     ima=ima-1
	 end if
	 if (date(ima+1).gt.ymax) then
	     ymax=date(ima+1)
	     ima=ima+1
	 end if
	 return
	 end


c-------------------------------------------------------------------------


	 subroutine creuar_umbral (seny, umbral, inici, iumb, sentit)

c	 Ens treu per iumb la posicio del senyal 'seny' quan aquest
c 	 es inferior, en modul, al valor umbral, sempre buscan en un sentit
c	 dreta 'd' o esquerra 'i', a partir d'una posicio inicial inici

	 dimension seny(100000)
	 character*1 sentit

	 iumb=inici
	 if (sentit.eq.'d') then
	     if(seny(inici).gt.umbral) then
 		do while (seny(iumb).gt.umbral)
		  iumb=iumb+1
		end do    
                if (abs(seny(iumb-1)-umbral).lt.abs(seny(iumb)-umbral))
     &          iumb=iumb-1
	     else
 		do while (seny(iumb).lt.umbral)
		  iumb=iumb+1
		end do    
                if (abs(seny(iumb-1)-umbral).lt.abs(seny(iumb)-umbral))
     &          iumb=iumb-1
	     end if
	 else
	     if(seny(inici).gt.umbral) then
       		do while (seny(iumb).gt.umbral)
		  iumb=iumb-1
		end do    
                if (abs(seny(iumb+1)-umbral).lt.abs(seny(iumb)-umbral))
     &          iumb=iumb+1
	     else
       		do while (seny(iumb).lt.umbral)
		  iumb=iumb-1
		end do    
                if (abs(seny(iumb+1)-umbral).lt.abs(seny(iumb)-umbral))
     &          iumb=iumb+1
	     end if
	 end if
	 return
	 end

c------------------------------------------------------------------------------


	 subroutine busca_pic2 (inici, seny, ipic, sentit)

c	 Ens treu per ipic la posicio del primer maxim o minim relatiu 
c	 del senyal, trobat en sentit (d , i ), a partir de la posicio inici
c        Evita picos d euna sola muestra


	 dimension seny(100000)
	 character*1 sentit

	 if (sentit.eq.'d') then             
	        ipic=inici+1
 		do while ((seny(ipic).lt.seny(ipic+1).or.
     &			seny(ipic).lt.seny(ipic+2)).and.
     &			(seny(ipic).gt.seny(ipic-1).or.      
     &			seny(ipic).gt.seny(ipic-2)).or.
     &			(seny(ipic).gt.seny(ipic+1).or.  
     &			seny(ipic).gt.seny(ipic+2)).and.
     &			(seny(ipic).lt.seny(ipic-1).or.  
     &			seny(ipic).lt.seny(ipic-2)))
		  ipic=ipic+1
		end do    
	    else
		ipic=inici-1
 		do while ((seny(ipic).lt.seny(ipic+1).or.
     &			seny(ipic).lt.seny(ipic+2)).and.
     &			(seny(ipic).gt.seny(ipic-1).or.      
     &			seny(ipic).gt.seny(ipic-2)).or.
     &			(seny(ipic).gt.seny(ipic+1).or.  
     &			seny(ipic).gt.seny(ipic+2)).and.
     &			(seny(ipic).lt.seny(ipic-1).or.  
     &			seny(ipic).lt.seny(ipic-2)))
	 	ipic=ipic-1
		end do    
	 end if
	 return
	 end
c------------------------------------------------------------------------------


	 subroutine busca_pic1 (inici, seny, ipic, sentit)

c	 Ens treu per ipic la posicio del primer maxim o minim relatiu 
c	 del senyal, trobat en sentit (d , i ), a partir de la posicio inici


	 dimension seny(100000)
	 character*1 sentit

	 if (sentit.eq.'d') then             
	        ipic=inici+1
 		do while (seny(ipic).lt.seny(ipic+1).and.
     &			seny(ipic).gt.seny(ipic-1).or.      
     &			seny(ipic).gt.seny(ipic+1).and.  
     &			seny(ipic).lt.seny(ipic-1))  
		  ipic=ipic+1
		end do    
	    else
		ipic=inici-1
 		do while (seny(ipic).lt.seny(ipic+1).and.
     &			seny(ipic).gt.seny(ipic-1).or.      
     &			seny(ipic).gt.seny(ipic+1).and.  
     &			seny(ipic).lt.seny(ipic-1))
	 	ipic=ipic-1
		end do    
	 end if
	 return
	 end


c------------------------------------------------------------------------------

         subroutine onar(dbuf, ecgpb, iqrs, irpos, irrpos, iqbeg,iqpos, 
     &		ispos, isend, iqend, isbeg, n, samp, ecg, comp_qs,ymaxaux)

c	 Ens troba la posicio de la ona R en cas de que la deteccio del 
c	 complex QRS correspongui a una ona Q o S per ser aquestes anormalment
c 	 grans.
c	 Tambe ens distingeix els complexos RSR'

         dimension dbuf(100000), irpos(8000), iqrs(8000), ecgpb(100000)
	 dimension irrpos(8000), iqbeg(8000), isend(8000), ecg(100000)
         dimension iqpos(8000), ispos(8000), iqend(8000), isbeg(8000)
         logical comp_qs

c	 iqrs(n): posicio del QRS del qual buscarem la ona R
c	 irpos(n): posicio de la ona R del impuls que estem tractant 

	 comp_qs=.false.                                                   

c 	 Busquem els pics a dreta i esquerra de la posicio iqrs en dbuf
	 call busca_pic2(iqrs(n), dbuf, mpicd, 'd')
         call busca_pic2(iqrs(n), dbuf, mpici, 'i')
c 	 si el senyal en iqrs es menor que cero, o el pic de la derivada 
c	 a l'esquerra es > 0 i el pic de la derivada a la dreta es < 0,
c	 sent al mateix temps l'un molt mes gran que l'altre,
c	 llavors estem en els casos on iqrs no correspon a la R
	 ydbufi=dbuf(mpici)
	 ydbufd=dbuf(mpicd)
	 if(abs(ydbufi).gt.abs(ydbufd)) then
		ymaxaux=abs(ydbufi)
	    else
		ymaxaux=abs(ydbufd)
         end if
	 kpi=2
	 if (ecgpb(iqrs(n)).lt.0.or.ydbufi.gt.0.and.ydbufd.lt.0.and.
     &	     (kpi*ydbufi.lt.-1*ydbufd.or.kpi*(-1)*ydbufd.lt.ydbufi)) then
	 perc=0.25
	 if (ecgpb(iqrs(n)).gt.0.and.ydbufi.gt.0.and.ydbufd.lt.0
     &			.or.(1+perc)*(-1)*ydbufi.gt.ydbufd.and.
     &			(1-perc)*(-1)*ydbufi.lt.ydbufd) then
c	 estem en el cas RSR'
c	 write(6,*) n,  'RsR'

		perc=0.35
	 	kr=5
		krr=5
	 	if (ecgpb(iqrs(n)).lt.0) then
c	 iqrs correspon a la ona s, llavors la R' estara a la dreta i la R
c	 a la esquerra
		  ncero=mpicd
		  call detectar_cero(dbuf, ncero, 'd')
c	 mirem que el pendent de despres de R' sigui mes gran que un umbral
	       	  call busca_pic2(ncero, dbuf, mpda, 'd')
		  if (-1*dbuf(mpda).lt.ydbufd/2) go to 5
		  irrpos(n)=ncero
		  ispos(n)=iqrs(n)
	          ncero=mpici
		  call detectar_cero(dbuf, ncero, 'i')
c	      mirem que el pendent de antes de R sea mes gran que un umbral
	       	  call busca_pic2(ncero, dbuf, mpda, 'i')
c	 write(6,*) dbuf(mpda), mpda, ydbufi
		  if (dbuf(mpda).lt.-ydbufi/2) go to 5
 		  irpos(n)=ncero
		else if (abs(ydbufi).lt.abs(ydbufd)) then
c	 iqrs correspon a la ona R
		  ncero=mpicd
		  call detectar_cero(dbuf, ncero, 'd')
c 	 afegim una nova condicio verificadora
		  call busca_pic2(ncero, dbuf, mpic, 'd')
		  if (.not.((1+perc)*abs(ydbufd).gt.abs(dbuf(mpic)).and.
     &			(1-perc)*abs(ydbufd).lt.abs(dbuf(mpic)))) goto 10
                  ispos(n)=ncero
		  call detectar_cero(dbuf, ncero, 'd')
                  irrpos(n)=ncero
		  irpos(n)=iqrs(n)
		else if (abs(ydbufi).gt.abs(ydbufd)) then
c	 iqrs correspon a la ona R'
		  ncero=mpici
		  call detectar_cero(dbuf, ncero, 'i')
c 	 afegim una nova condicio verificadora
		  call busca_pic2(ncero, dbuf, mpic, 'i')
		  if (.not.((1+perc)*abs(ydbufi).gt.abs(dbuf(mpic)).and.
     &			(1-perc)*abs(ydbufi).lt.abs(dbuf(mpic)))) goto 10
		  ispos(n)=ncero
		  call detectar_cero(dbuf, ncero, 'i')
                  irpos(n)=ncero
		  irrpos(n)=iqrs(n)
                end if
c	 afegim una verificacio sobre les distancies R-R'
		if (abs(irpos(n)-irrpos(n)).gt.150/samp) then
		  if (ecgpb(iqrs(n)).gt.0) then
			goto 10
	            else
			goto 5
		  end if
		end if
c	 associem l'inici del RSR' a iqbeg i el final a isend, segons el
c	 criteri de la derivada
	        call busca_pic2(irrpos(n), dbuf, mpicd, 'd')
         	call busca_pic2(irpos(n), dbuf, mpici, 'i')
c	 fem un ajust dels pics del complexe QRS
	   if (irrpos(n).ne.0) then 
	   call test_pic (ecg, irrpos(n), samp, 'p')
	   end if
	   if (ispos(n).ne.0) then 
	   call test_pic (ecg, ispos(n), samp, 'v')
	   end if
	   if (irpos(n).ne.0) then 
	   call test_pic (ecg, irpos(n), samp, 'p')
	   end if
		umbral=dbuf(mpicd)/krr
		call creuar_umbral(dbuf, umbral, mpicd, isend(n), 'd')
	 	umbral=ecg(isend(n))
		call creuar_umbral(ecg, umbral, irrpos(n), isbeg(n), 'i')
		umbral=dbuf(mpici)/kr
		call creuar_umbral(dbuf, umbral, mpici, iqbeg(n), 'i')
		umbral=ecg(iqbeg(n))
		call creuar_umbral(ecg, umbral, irpos(n), iqend(n), 'd')
		iqpos(n)=0
  	    else
c 	 estem en el cas de Q o S anormalment grans, llavors
  5	 irrpos(n)=0	
c	 el pendent mes gran es el associat a la ona R, i,que tinguem
c	 presencia aprop de ona R per tant 
c	 busquem les dos posibles posicions de la ona R
		nrted=mpicd
		call detectar_cero(dbuf, nrted, 'd')
		nrtei=mpici
		call detectar_cero(dbuf, nrtei, 'i')
	 prr=1.4
c	write(6,*) n,iqrs(n),nrtei,nrted,dbuf(mpici),dbuf(mpicd)
	 if (abs(dbuf(mpicd)).gt.prr*abs(dbuf(mpici)).or.
     &		iqrs(n)-nrtei.gt.nrted-iqrs(n)) then         
c	 tenim ona Q en la posicio iqrs(n), i la ona R estara en el primer
c	 cero a la dreta
	    	 ncero=mpicd
		call detectar_cero(dbuf, ncero, 'd')
c	 mirem que el pendent de despres de R sigui mes gran que un umbral
	       	  call busca_pic2(ncero, dbuf, mpda, 'd')
c 	 si la distancia entre ones es massa gran potser no existeix ona R
c	 saltem a les sentencies de ona S per verificar-ho.
c	        write(6,*) n, iqrs(n), ncero, mpda,dbuf(mpda),ydbufd

	       if(ncero-iqrs(n).gt.150/samp.or.-1*dbuf(mpda).lt.ydbufd/10)
     &			 then
	 		go to 7
	 	   else
			irpos(n)=ncero
		end if
	   else

c	 tenim ona S en la posicio iqrs(n), i la ona R estara en el primer
c	 cero a l'esquerra
 7	  	ncero=mpici
		call detectar_cero(dbuf, ncero, 'i')
c	 mirem que el pendent de despres de R' sigui mes gran que un umbral
	         ilim=ncero-nint(60/samp)
	     call buscamaxmin(ilim,ncero,dbuf,iau,yau,imax2,ymax2)  
c 	 si la distancia entre ones es massa gran, o be el pic buscat mpdi
c	 no supera l'umbral , no existeix ona R, associem
c	 irpos a iqrs, despres no es detectara ona S, pero sabrem que ho es 
c	 per l'amplitut negativa.
c 	    write(6,*) n,ymax2, ydbufi,ecgpb(ncero),ecgpb(iqrs(n))
	  if ( (iqrs(n)-ncero.gt.140/samp.or.ymax2.lt.-1*ydbufi/10)
     &		.or.(ecgpb(ncero).lt.-abs(ecgpb(iqrs(n))*1/6)) )  then
			irpos(n)=iqrs(n)
			comp_qs =.true.
		   else
			irpos(n)=ncero
		end if
	 end if             
	 end if
	 else 
c	 La posicio iqrs correspon a la R, sent el QRS normal
 10	 irpos(n)=iqrs(n)
	 irrpos(n)=0
	 end if  
	 return
	 end
	 

c------------------------------------------------------------------------------

	 subroutine onaq(dbuf, dermax, ima, irpos, n, iqbeg, iqpos,
     &			iqend, samp, ecg , deri, eonaq, itend,iqrs)

c	 Ens treu per iqpos i iqbeg la posicio i l'inici de la ona Q, en cas
c	 de no existir ens treu la poisicio =0 i inici de la ona R.

	 dimension dbuf(100000), irpos(8000), iqbeg(8000), iqpos(8000), iqend(8000)
	 dimension ecg(100000), deri(100000), itend(100000),iqrs(8000)
	 logical eonaq

c	 eonaq: ens diu si existeix la ona Q
c	 irpos(n): posicio de la ona R del impuls que estem tractant 
c	 kq, kr: constants per definir els umbrals de les derivades per
c		 les ones Q i R respectivament
c	 dermax: derivada maxima del QRS
c	 ima: posicio del maxim anterior a la ona R en el senyal derivat

                 
c	 

	 kq=2
	 kr=5
	 inte=nint(10/samp)
c	 eonaq=.true.
c 	 busquem el cero previ a la posicio de la ona R partint del pic,
c	 que en principi correspondra a la posicio de la ona Q.
	 ncero=ima+1
	 nceau=ima
c	 busquem el cero en la derivada del ecg sense filtrar per tal de 
c	 partir de la posicio correcta per buscar el pic posterior
	 call detectar_cero (deri, ncero, 'i')
	 call detectar_cero (dbuf, nceau, 'i')
c	write(6,*) n, ncero, nceau, 'll'
c 	 si la distancia entre ones es superior a 60 ms , no existeix ona Q
	 if (irpos(n) - ncero.gt.80/samp.and.irpos(n).le.iqrs(n))
     &                eonaq=.false. 	
c	 si tenim ona Q, busquem el pendent maxim anterior

	 if (eonaq) then 
	 call busca_pic2 (nceau, dbuf, mpic, 'i')
c	 posem una proteccio per casos en que la derivada cuasi creua el 0.
	 call busca_pic2 (ima, dbuf, icep, 'i')  	 
c	 si el pic es gran, i no tenim un cuasi creuament de cero,
c	 treballarem amb dbuf
	 if(abs(dbuf(mpic)).gt.dermax/12.and..not.(icep.gt.mpic.and.
     &		abs(dbuf(icep)).lt.dermax/50)) then

c 	 probem de detectar be quan la ona P esta unida a la Q sense cap
c	 inflexio
	 if (irpos(n) - mpic.gt.90/samp.or.nceau-mpic.gt.30/samp
     &               .and.irpos(n).le.iqrs(n)) eonaq=.false. 	
c	 busquem el inici amb el umbral de la derivada o be amb el primer
c	 pic a l'esquerra
c	   umbral=dbuf(mpic)/kq
	   umbral=dbuf(mpic)/1.8
	   call creuar_umbral(dbuf, umbral, mpic, iumb, 'i')
	   call busca_pic1 (mpic, dbuf, ipic, 'i')
c	 write(6,*) n,irpos(n),nceau,mpic,iumb,ipic,dbuf(ipic),dbuf(mpic)


	   if(ipic.gt.iumb) iumb=ipic

c	   if (dbuf(ipic).gt.abs(dbuf(mpic)*0.8)) go to 10
c	   si la distancia es superior a 80 ms, no existeix ona Q
c 	 write(6,*) n, mpic, dbuf(mpic), irpos(n), dermax
 	   if (irpos(n)-iumb.gt.120/samp)  eonaq=.false.
	       
c          comprobem que el complexe Q no comengi amb un petit pic de pujada.
c          si fos aixi no l'hauriem detectat i l'inici trobat fora dolent
c          perque no el tenia amb compte. L'umbral ser` a Kr=3 i no 5 perque
c          is un petit pic ara.
	   ilimp=nint(30/samp)
           call buscamaxmin(iumb-ilimp,   
     &                       iumb,dbuf,imin2,ymin2,imax2,ymax2)
c	  write(6,*) iumb,imax2,ymax2, imin2,ymin2,dermax
          if (abs(ymin2).ge.dermax/20) then
                  umbral=ymin2/1.8
    	          call creuar_umbral (dbuf, umbral, imin2, iumb2, 'i')
	          if (iumb2.gt.iumb-40/samp) iumb=iumb2
          end if
c	  write(6,*) iumb, iumb2
	  if  ((imax2.lt.imin2.or.abs(ymin2).lt.dermax/20)
     &            .and.abs(ymax2).gt.dermax/20) then
                  umbral=ymax2/1.8
    	          call creuar_umbral (dbuf, umbral, imax2, iumb2, 'i')
	          if (iumb2.gt.iumb-40/samp) iumb=iumb2
          end if
  	 else
  10	 kq=3
c	 per tal de detectar ones Q de component frequencial molt elevat
c 	 fem servir la derivada del ecg sense filtrar.
c	 busquem el pic corresponent a deri.
	 call busca_pic2 (ncero, deri, mpic, 'i')
	 call test_pic (deri, mpic, samp, 'v')
c 	 si el pendent maxim de la ona Q es << que el pendent maxim llavors 
c	 no existeix ona Q
	 if (abs(deri(mpic)).lt.dermax/10
     &       .or.ncero-mpic.gt.30/samp) eonaq=.false.
c	 si existeix la ona Q busquem el seu principi amb el umbral de la
c	 derivada  

	 if (eonaq) then
	   umbral=deri(mpic)/kq
	   umbral=deri(mpic)/2.8
	   call creuar_umbral(deri, umbral, mpic, iumb, 'i')
c	   si la distancia es superior a 80 ms, no existeix ona Q
	   if (irpos(n)-iumb.gt.80/samp) eonaq=.false.
	 end if
	 end if
         end if
c	 si no existeix la ona Q busquem el principi de la ona R amb el
c	 umbral de la derivada 
	 if (.not.eonaq) then
c	   write(6,*) n,ima, dbuf(ima)
	   if (dbuf(ima).ge.1.5) kr=8
	   if (dbuf(ima).ge.3) kr=18
           if (dbuf(ima).ge.4.0) kr=28
	   umbral=dbuf(ima)/kr
           kr=5
	   call creuar_umbral (dbuf, umbral, ima, iumb, 'i')
c          asegurem-nos que la ona R no comenga amb un petit pic :
              ib=iumb-abs(30.0/samp)
              call buscamaxmin(ib,iumb,dbuf,iaux,yaux,ima2,ymax2)
              if (ymax2.ge.dermax/50) then
                  ima=ima2
                  umbral=ymax2/1.5
    	          call creuar_umbral (dbuf, umbral, ima, iumb2, 'i')
		  if (iumb2.gt.iumb-36/samp) iumb=iumb2
              end if
c	      write(6,*) n, ima, dbuf(ima), iumb,iumb2,ima2,ymax2
           end if
c	 omplim els vectors de sortida
	 if (eonaq) then
		iqpos(n)=ncero
            else
		iqpos(n)=0
	 end if
	 iqbeg(n)=iumb
 
c	 write(6,*) n, iqpos(n),iqbeg(n) , 'ippb'	
         if (n.gt.1.and.iqbeg(n).le.itend(n-1)) iqbeg(n)=itend(n-1)+1
	  
	   if (iqpos(n).ne.0) then 
	   call test_pic (ecg, iqpos(n), samp, 'v')
	   end if
c	 busquem el final de la ona Q
c	 if (eonaq) then
c	   umbral=ecg(iumb)
c	   call creuar_umbral(ecg, umbral, iqpos(n), iqend(n), 'd')
c	  else
c	   iqend(n)=0
c	 end if
	 return
	 end

	   
	   	 
c---------------------------------------------------------------------------   



	 subroutine onas(dbuf, dermax, imi, irpos, n, isend, ispos,
     &			isbeg, samp, ecg, iqbeg, deri, eonas,iqrs)

c	 Ens treu per ispos i isend la posicio i el final de la ona S, en cas
c	 de no existir ens treu la poisicio igual a 0 i el final de la ona R.

	 dimension dbuf(100000), irpos(8000), isend(8000), ispos(8000), isbeg(8000)
	 dimension derfi(100000), ecg(100000),iqbeg(8000), deri(100000), iqrs(8000)
	 logical eonas

c	 eonas: ens diu si existeix la ona S
c	 irpos(n): posicio de la ona R del impuls que estem tractant
c	 ks, kr: constants per definir els umbrals de les derivades per
c		 les ones S i R respectivament
c	 dermax: derivada maxima del QRS
c	 imi: posicio del maxim posterior a la ona R en el senyal derivat


c 	 busquem el cero posterior a la posicio de la ona R partint del minim,
c	 que en principi correspondra a la posicio de la ona S.

	 ks=3
	 kr=5
	 inte=nint(10/samp)
c	 eonas=.true.
	 ncero=imi
	 nceau=imi
c	 busquem el cero en la derivada del ecg sense filtrar per tal de 
c	 partir de la posicio correcta per buscar el pic posterior
	 call detectar_cero (deri, ncero, 'd')
	 call detectar_cero (dbuf, nceau, 'd')
	 
c 	 si la distancia entre ones es superior a 130 ms , no existeix ona S
	 if ((ncero-irpos(n).gt.130/samp).and.
     &          irpos(n).ge.iqrs(n))  eonas=.false. 
         if ((nceau.lt.iqrs(n)).and.(ecg(iqrs(n)).lt.0)) then
		ncero=iqrs(n)
		nceau=iqrs(n)
	 end if

c	 si tenim ona S, busquem el pendent maxim posterior
	 if (eonas.eqv..true.) then
c	 en comptes de buscar el primer pic a la dreta busquem el maxim
c	 en un interval pero amb un valor mes restringiu per si tenim 
c	 bloqueig de branca dreta
	  
	  if (ispos(n).eq.iqrs(n)) then
		ilim=nint(nceau+140./samp)
	 	    else 
	       ilim=nint(nceau+80./samp)   
          end if
	 call buscamaxmin (nceau,ilim , dbuf, iau, yau,mpic, ypic)
 
c	 write(6,*) n, nceau, mpic 
	 if (ypic.lt.dermax/10) call busca_pic2 (nceau, dbuf, mpic, 'd')  
c	 posem una proteccio per casos en que la derivada cuasi creua el 0.
	 call busca_pic2 (imi, dbuf, icep, 'd')  	 
c	 si el pic es gran, i no tenim un cuasi creuament de cero,
c	 treballarem amb dbuf
	 if( dbuf(mpic).gt.dermax/30.and.(.not.(icep.lt.mpic.and.
     &	    abs(dbuf(icep)).lt.dermax/50).or.(iqrs(n).eq.ncero)) ) then
c          posem ks en funcis de la pendent
           if (dbuf(mpic).ge.4.) ks=8
           if (dbuf(mpic).ge.4.75) ks=9
           if (dbuf(mpic).ge.6.2) ks=10 
c	   write(6,*) n, dbuf(mpic)
 	   umbral=dbuf(mpic)/ks
           ks=3
	   call creuar_umbral(dbuf, umbral, mpic, iumb, 'd')
	   call busca_pic1 (nint(mpic+10/samp), dbuf, ipic, 'd')
	   if(ipic.lt.iumb.and.dbuf(ipic).lt.dermax/15) iumb=ipic
c		write(6,1) n, ncero, iqrs(n), iumb
c 1	 	format('c=',i4,'ncero=',i4,'iqrs(n)=',i4,'iu=',i4)
 
c	   si la distancia es superior a 200 ms, no existeix ona S
	   if (iumb-irpos(n).gt.200/samp) then
             umbral=dbuf(mpic)/kr
             call creuar_umbral(dbuf, umbral, mpic, iumb, 'd')
             call busca_pic1 (nint(mpic+10/samp), dbuf, ipic, 'd')
             if(ipic.lt.iumb.and.dbuf(ipic).lt.dermax/3) iumb=ipic
           end if 
            
c           write(6,2) iumb-irpos(n) 
c 2 	   format('QRS=',i4 )
	
	 else
           call busca_pic2 (ncero, deri, mpic, 'd')
c 	 si el pendent maxim de la ona S es << que el pendent maxim llavors 
c	 no existeix ona S
c	 per tal de detectar ones S de component frequencial molt elevat
c 	 fem servir la derivada del ecg sense filtrar.
c	 busquem el pic corresponent a deri.
	   ks=3
	   call test_pic (deri, mpic, samp, 'p')
	   if (abs(deri(mpic)).lt.dermax/10.and.irpos(n).ge.iqrs(n))
     &                    eonas=.false.
c	 si existeix la ona S busquem el seu final amb el umbral de la
c	 derivada, o be, amb el primer pic a la dreta  
	   if (eonas) then
	   umbral=deri(mpic)/ks
	   call creuar_umbral(deri, umbral, mpic, iumb, 'd')
	   call busca_pic1 (nint(mpic+10/samp), dbuf, ipic, 'd')
	   if (ipic.lt.iumb) iumb=ipic
c	   si la distancia es superior a 80 ms, no existeix ona S
	   if (iumb-irpos(n).gt.200/samp)  then
             umbral=dbuf(mpic)/kr
             call creuar_umbral(dbuf, umbral, mpic, iumb, 'd')
             call busca_pic1 (nint(mpic+10/samp), dbuf, ipic, 'd')
             if(ipic.lt.iumb.and.dbuf(ipic).lt.dermax/3) iumb=ipic
           end if 
  	 end if
	 end if
         if ((iumb-irpos(n).gt.200/samp).and.
     &            irpos(n).ge.iqrs(n))  eonas=.false.
    
	 end if
c	 si no existeix la ona S busquem el final de la ona R amb el
c	 umbral de la derivada  o be amb el primer pic a la dreta
	 if (.not.eonas) then
	   umbral=dbuf(imi)/kr
	   call creuar_umbral (dbuf, umbral, imi, iumb, 'd')
	   call busca_pic2(nint(imi+10/samp), dbuf, ipic, 'd')
	   if(ipic.lt.iumb) iumb=ipic
	 end if
c	 omplim els vectors de sortida
	 if (eonas) then
		ispos(n)=ncero
            else
		ispos(n)=0
	 end if
	 isend(n)=iumb
	 if (isend(n).lt.iqrs(n)) isend(n)=iqrs(n)+1
	   if (ispos(n).ne.0) then 
	   call test_pic (ecg, ispos(n), samp, 'v')
	   end if
c	 busquem l'inici de la ona S
c	 if (eonas) then
c	   umbral=ecg(iumb) 
c	   call creuar_umbral(ecg, umbral, ispos(n), isbeg(n), 'i')
c	  else
c	   isbeg(n)=0
c	 end if
	 return
	 end
                                         
c--------------------------------------------------------------------------


	 subroutine onat (n, dbuf, rrmed, isf, irpos, iqbeg, itbeg, itpos,
     &   itpos2,itend, samp, if, isend, ecg, basel, morf,iqrs)

c	 ens localitza els punts inicial, pics i final de la ona T. Tambe ens
C	 treu la morfologia de l'ona T pel vector morf segons el codi:
c
c		morf(4,j) -ona T	0: existeix, normal
c					1:    "    , invertida
c					2:    "    , nomes pujada
c					3:    "    , nomes baixada
c					4: bifasica -+
c					5:    "     +-
c					6: no existeix


	 dimension dbuf(100000), irpos(8000), iqbeg(8000), itbeg(8000),itpos(8000)
	 dimension itend(8000), isend(8000), ecg(100000),back(6)
 	 dimension morf(5,8000), basel(8000), itpos2(8000), iqrs(8000)
	 logical flag1, inici_t, fivec
         

c 	 flag1: variable de control de bucle
c	 inici_t: ens diu si podem buscar el inici de la ona (T normal o
c	          invertda)
c	 bwind, ewind: amplada de la finestra
c	 ibw: posicio a partir de la que comengarem a buscar
c	 iew: posicio fins la que buscarem
c 	 ktb, kte: constants per definir els umbrals de la derivada per
c 	 	   localitzar l'inici i final de la ona T respectivament
c	 pco=valor de comparacio entre els pics
	 pco=8.0
	 ktb=2
	 kdis=nint(50/samp)
	 
c	Limite hasta donde se permitira ir al fin de la T 
	 if (rrmed.gt.900/samp) then
	         itqlim=nint(280/samp)
	         ewind=800
 	      else if (rrmed.gt.800/samp) then
	         itqlim=nint(250/samp)
 	         ewind=600
 	      else if (rrmed.gt.600/samp) then
		 itqlim=nint(200/samp)
		 ewind=450
 	      else 
		itqlim=nint(170/samp)
	        ewind=450
          end if
c	  write(6,*) itqlim	
c         factor de aumento de los umbrales si la primera vez va muy lejos
 
         do  i=1,6
          back(i)=1.0
	 end do
	 
         bwind=100
	          ibw=irpos(n)+nint(bwind/samp)
	 iew=irpos(n)+nint(ewind/samp)
	 flag1=.true.
	 inici_t=.true.

c	 inicialitzem finestres
 	
	 if (n.gt.1) then
          call inifinestres(rrmed, isf, irpos(n), bwind, ewind,  
     &				ibw, iew, samp, fivec)
         end if

c	 posem un reajust per QRS molt amples i per intervals rr anormalment
c	 petits.
	 if(ibw.le.isend(n)+kdis) then
		iew=iew+isend(n)-ibw+kdis
		ibw=isend(n)+kdis
	 end if   
 
	 if (iqrs(n+1).gt.0.and.iew.gt.iqrs(n+1)-210/samp) 
     &		iew=iqrs(n+1)-nint(210/samp)
	 if (iqrs(n+1).eq.0) iew=irpos(n)+nint(400/samp)
         if (fivec) return
c          write(6,5) n, (iqrs(n+1)-iew)*samp 
c 5 	   format('beat=',i4,' distancia iwe QRS', f6)
 	
c	 posem una altre condicio per detectar be T de pujada o baixada
	 call buscamaxmin (ibw, iew, dbuf, imi, ymin, ima, ymax) 
	 if (ymin.gt.0.or.ymax.lt.0) then
	  if (iew.eq.iqrs(n+1)-210/samp) then
	   if (ymin.gt.0) ymin=0
	   if (ymax.lt.0) ymax=0
	  else
	   do while (ymin.gt.0.or.ymax.lt.0.and.iqrs(n+1).gt.0.and.
     &		iew.lt.iqrs(n+1)-250/samp)
	     iew=iew+nint(25/samp)
	     call buscamaxmin (ibw, iew, dbuf, imi, ymin, ima, ymax) 
           end do
	  end if
	 end if
c	 inicialitzem morf al valor de no existencia per si no es pot clasificar
c	 dintre de cap dels patrons
	 morf(4,n)=6

	 do while (flag1.and.iew.gt.ibw)
c	 si el maxim i el minim tenen valors semblants busquem els pics
c	 anteriors i posteriors segons cada cas
	 kint1=nint(250/samp)
	 kint2=nint(300/samp)
c	 kend= valor en ms de marge minim a partir de isend on buscarem pics
	 kend=50
c	 ampmi= valor minim de les amplituts de les ones T bifasiques
         ampmi=0.075
c    	 kk es el nivel de comparacion para determinar ondas bifasicas
	 kk=3
	 com=0.3
c		write(6,*) n,imi, ymin, ima, ymax, ibw, iew, iqrs(n+1)
          if (-com*ymin.lt.ymax.and.-ymin.gt.com*ymax) then
c         write(6,2) n 
c 2 	   format('beat=',i4,' with max and min comparables')
	   if (imi.lt.ima) then
	     call buscamaxmin(ibw, imi, dbuf, iau, yau, imaa, ymaxa)
	     call buscamaxmin(ima, iew, dbuf, imip, yminp, iau, yau)
c	 si algun dels dos pics novament trobats es petit, no el considerem 
c	 com part de la ona T, o be si els dos a la vegada son grans
	     if (ymaxa.lt.ymax/kk.and.-yminp.lt.-ymin/kk.or.
     &		ymaxa.ge.ymax/kk.and.-yminp.ge.-ymin/kk) then
		morf(4,n)=1
c	        write(6,*) 'invertida primera opcion'
c		write(6,*) n, imi,ima,ymin,ymax

c	   write(6,*) n, ymaxa, ymin,ymax,yminp
	     else if(ymaxa.ge.ymax/kk) then
		go to 200
c	 mirem si estem en el cas de ona bifasica
c	 	ncea=imaa
c		call detectar_cero(dbuf, ncea, 'd')
c		if (ecg(ncea)-basel(n).lt.ampmi) then
c		   morf(4,n)=1
c		 else 
c	 tenim ona t bifasica +-
c		   imap=ima
c		   ymaxp=ymax
c		   morf(4,n)=5
c		end if
	     else if(-yminp.ge.-ymin/kk) then
		go to 200
c	 mirem si estem en el cas de ona bifasica
c	 	ncep=imip
c		call detectar_cero(dbuf, ncep, 'i')
c		if (ecg(ncep)-basel(n).lt.ampmi) then
c		morf(4,n)=1
c		 else 
c	 tenim ona t bifasica -+
c		   imia=imi
c		   ymina=ymin
c		morf(4,n)=4
c		end if
	     end if	
	    else
c	 estem en el cas de minim posterior al maxim
c	     if (ima-kint1.gt.isend(n)+kend/samp) then
c		inici=ima-kint1
c	      else
c		inici=isend(n)+nint(kend/samp)
c	     end if
c	     if (inici.gt.ima) inici=ima
c             if (imi+kint2.gt.iew) kint2=iew-imi
         call buscamaxmin(imi, iew, dbuf, iau, yau, imap, ymaxp)
	     call buscamaxmin(ibw, ima, dbuf, imia, ymina, iau, yau)
c	 si algun dels dos pics novament trobats es petit, no el considerem 
c	 com part de la ona T, o be si els dos a la vegada son grans
c	     write(6,*) n,n,imia,ymina,ima,ymax,imi,ymin,imap,ymaxp
	     if (ymaxp.lt.ymax/kk.and.-ymina.lt.-ymin/kk.or.
     &		ymaxp.ge.ymax/kk.and.-ymina.ge.-ymin/kk) then
		morf(4,n)=0
	     else if(ymaxp.ge.ymax/kk) then
c	 mirem si estem en el cas de ona bifasica
	 go to 200
c	 	ncep=imap
c		call detectar_cero(dbuf, ncep, 'i')
c		if (basel(n)-ecg(ncep).lt.ampmi) then
c		morf(4,n)=0
c		 else 
c	 tenim ona t bifasica +-
c		   imaa=ima
c		   ymaxa=ymax
c		   morf(4,n)=5
c	   	end if
	     else if(-ymina.ge.-ymin/kk) then
c	 mirem si estem en el cas de ona bifasica
	 go to 200
c	 	ncea=imia
c		call detectar_cero(dbuf, ncea, 'd')
c		if (basel(n)-ecg(ncea).lt.ampmi) then
c		morf(4,n)=0
c		 else 
c	 tenim ona t bifasica -+
c		   imip=imi
c		   yminp=ymin
c		morf(4,n)=4
c		end if
	     end if	
	   end if
 	 else 
c	 estem en el cas de pics diferents
c	 sino, si el maxim es mes gran que el minim busquem els dos minims
c	 anteriors i posteriors
 200	 continue
 	   if (ymax.gt.-1*ymin) then 
c	     if (ima-kint1.gt.isend(n)+kend/samp) then
c		inici=ima-kint1
c	      else
c		inici=isend(n)+nint(kend/samp)
c	     end if
c	     if (inici.gt.ima) inici=ima
	     call buscamaxmin(ibw, ima, dbuf, imia, ymina, iau, yau)
c	     if (ima+kint2.gt.iew) kint2=iew-ima
             call buscamaxmin(ima,iew,dbuf,imip,yminp, iau, yau)
             ncea=imia
	     if (ymina.lt.0) call detectar_cero(dbuf, ncea, 'd')
	     ampa=basel(n)-ecg(ncea)
             ncep=imip
	     if (yminp.lt.0) call detectar_cero(dbuf, ncep, 'i')
	     ampp=ecg(ncep)-basel(n)     
c	 si la primera amplitud es gran podem tenir T de pujada, bifasica -+ o
c	 invertida
c	     write(6,*) n, ampa, ampp, imia,ymina,imip,yminp
	     if (ampa+ampp.gt.ampmi)  then
	     if (-ymina.lt.ymax/pco.and.-yminp.lt.ymax/pco)  then
c	 tenim ona T de nomes pujada
		morf(4,n)=2      
	     else if (-ymina.ge.ymax/pco.and.-yminp.ge.ymax/pco) then
c	 tenim ona T bifasica -+
		morf(4,n)=4
	     else if (-ymina.ge.ymax/pco.and.-yminp.lt.ymax/pco) then
c                    tenim ona T invertida
c 	  write(6,*) 'invertida segunda opcion'
 		     morf(4,n)=1
		     ymin=ymina
	 	     imi=imia
c	             write(6,*) imi,ima,ymin,ymax
            else if (-ymina.lt.ymax/pco.and.-yminp.ge.ymax/pco) then
c	             tenim ona T normal
		     morf(4,n)=0
		     ymin=yminp
	 	     imi=imip
	    end if
	   end if      		

	 else if (ymax.lt.-1*ymin) then
c	     if (imi-kint1.gt.isend(n)+kend/samp) then
c		inici=imi-kint1
c	      else
c		inici=isend(n)+nint(kend/samp)
c	     end if
c	     if (inici.gt.imi) inici=imi
	     call buscamaxmin(ibw, imi, dbuf, iau, yau, imaa, ymaxa)
c	     if (imi+kint2.gt.iew) kint2=iew-imi
             call buscamaxmin(imi, iew,dbuf,iau,yau,imap,ymaxp)
             ncea=imaa
	     if (ymaxa.gt.0) call detectar_cero(dbuf, ncea, 'd')
	     ampa=ecg(ncea)-basel(n)
             ncep=imap
	     if (ymaxp.gt.0) call detectar_cero(dbuf, ncep, 'i')
	     ampp=basel(n)-ecg(ncep)
c	 si la primera amplitut es gran podem tenir ona T de baixada, normal
c	 o be bifasica +-
c	 write(6,*) n,imaa,ymaxa,imi,ymin,imap,ymaxp,ampa,ampp
	     if (ampa+ampp.gt.ampmi) then
	     if (ymaxa.lt.-ymin/pco.and.ymaxp.lt.-ymin/pco) then
c	 tenim ona T de nomes baixada
c	        write(6,*) n, 'bajada'
		morf(4,n)=3      
	     else if (ymaxa.ge.-ymin/pco.and.ymaxp.ge.-ymin/pco) then
c	 tenim ona T bifasica +-
c              write(6,*) n, 'bifasica'
 		morf(4,n)=5
	     else if (ymaxa.ge.-ymin/pco.and.ymaxp.lt.-ymin/pco) then
c	 tenim ona T normal
c		write(6,*) n, 'normal'
 		morf(4,n)=0
		ymax=ymaxa
		ima=imaa
	    else if (ymaxa.lt.-ymin/pco.and.ymaxp.ge.-ymin/pco) then 
c	 tenim ona T invertida
		morf(4,n)=1
	        ymax=ymaxp
		ima=imap
c  		write(6,*) imi,ima,ymin,ymax
	     end if 
	    end if
	   end if
	 end if

c	 procedim a buscar pics, inicis i finals segons cada cas
 300	 go to (10, 20, 30, 40, 50, 60, 70), morf(4,n)+1
c 	 tenim ona T normal 
 10 	 umba=ymax/ktb	
	 call creuar_umbral (dbuf, umba, ima, iumba, 'i')
c 	 si hem anat mes enrera de isend, trobem el principi de la T
c	 amb un criteri del primer pic a l'esquerra
	 if (iumba.lt.isend(n)) call busca_pic2(ima, dbuf, iumba, 'e')
c 	 si encara hem anat massa enrera l'associem a isend
	 if (iumba.le.isend(n)) iumba=isend(n)+2 
	 itbeg(n)=iumba

C        kte valdr` 4,5,6,7 en funcis del valor de la derivada max.
         if (abs(ymin).ge.0.41) then
             kte=7
         else if (abs(ymin).ge.0.35) then
             kte=6
         else if (abs(ymin).ge.0.25) then
             kte=5
         else if (abs(ymin).ge.0.10) then   
             kte=4
	 else if (abs(ymin).lt.0.10) then   
             kte=3.5
         end if

         if (kte/back(1).ge.1.1) then
	      umbp=ymin*back(1)/kte
	 else 
	   umbp=ymin/1.1
	 end if
	 call creuar_umbral (dbuf, umbp, imi, iumbp, 'd')
	 itend(n)=iumbp
 
	 icero1=imi
	 call detectar_cero (dbuf, icero1, 'i')
	 icero2=ima
	 call detectar_cero (dbuf, icero2, 'd')
	 if (icero1.ne.icero2) then
		icero=(icero1+icero2)/2
	 else 
            icero=icero1
         end if
	 if (icero.ge.itend(n).or.icero.le.itbeg(n))
     &           icero=itbeg(n)+(itend(n)-itbeg(n))/2

	 itpos(n)=icero
	 itpos2(n)=icero
	 go to 100

c	 tenim ona T invertida
 20 	 umba=ymin/ktb	
	 call creuar_umbral (dbuf, umba, imi, iumba, 'i')
c 	 si hem anat mes enrera de isend, trobem el principi de la T
c	 amb un criteri del primer pic a l'esquerra
	 if (iumba.lt.isend(n)) call busca_pic2(imi, dbuf, iumba, 'e')
c 	 si encara hem anat massa enrera l'associem a isend
	 if (iumba.le.isend(n)) iumba=isend(n)+2 
	 itbeg(n)=iumba

C        kte valdr` 4,5,6,7 en fuincis del valor de la derivada max.
         if (abs(ymax).ge.0.41) then
             kte=7
         else if (abs(ymax).ge.0.35) then
             kte=6
         else if (abs(ymax).ge.0.25) then
             kte=5
         else if (abs(ymax).ge.0.10) then   
             kte=4
	 else if (abs(ymax).lt.0.10) then   
             kte=3.5
         end if

 
         if (kte/back(2).gt.1.1) then
	      umbp=ymax*back(2)/kte
	 else 
	   umbp=ymax/1.1
	 end if
 	 call creuar_umbral (dbuf, umbp, ima, iumbp, 'd')
	 itend(n)=iumbp
c        write(6,*) n, imi, ima, iumbp, itbeg(n), itend(n),ymax,umbp,kte   
	 icero1=ima
	 call detectar_cero (dbuf, icero1, 'i')
	 icero2=imi
	 call detectar_cero (dbuf, icero2, 'd')
	 if (icero1.ne.icero2) then 
              icero=(icero1+icero2)/2
         else 
            icero=icero1
         end if
	  if (icero.ge.itend(n).or.icero.le.itbeg(n))
     &           icero=itbeg(n)+(itend(n)-itbeg(n))/2

c 	 write(6,1) icero1, icero2, icero
c 1	 format('icero1=',i4, ' icero1=',i4,' icero1=',i4)
      

	 itpos(n)=icero
	 itpos2(n)=icero
	 go to 100

c	 tenim ona T de nomes pujada

 30      continue
C        kte valdr` 4,5,6,7 en fuincis del valor de la derivada max.
         if (ymax.ge.0.41) then
             kte=7
         else if (ymax.ge.0.30) then
             kte=6
         else if (ymax.ge.0.20) then
             kte=5
         else if (ymax.gt.0.10) then   
             kte=4
	 else if (ymax.le.0.10) then   
             kte=3.5
         end if
   
         if (kte/back(3).ge.1.1) then
	      umbp=ymax*back(3)/kte
	 else 
	   umbp=ymax/1.1
	 end if
     

         call creuar_umbral (dbuf, umbp, ima, iumbp, 'd')
	 itend(n)=iumbp
	 icero=ima
	 call detectar_cero (dbuf, icero, 'i')
	 call busca_pic2(ima-kdis, dbuf, ipic, 'i')
	 if (ipic.gt.icero) then
	    itpos(n)=ipic
	  else 
	    itpos(n)=icero
	 end if
	 if (itpos(n).le.isend(n)) itpos(n)=isend(n)+1
	 itpos2(n)=0
	 itbeg(n)=0
	 go to 100

c	 tenim ona T de nomes baixada   

 40      continue
C        kte valdr` 4,5,6,7 en funcis del valor de la derivada max.
          if (abs(ymin).ge.0.41) then
             kte=7
         else if (abs(ymin).ge.0.30) then
             kte=6
         else if (abs(ymin).ge.0.20) then
             kte=5
         else if (abs(ymin).ge.0.10) then   
             kte=4
	 else if (abs(ymin).lt.0.10) then   
             kte=3.5
         end if

         if (kte/back(4).ge.1.1) then
	      umbp=ymin*back(4)/kte
	 else 
	   umbp=ymin/1.1
	 end if
     
	 call creuar_umbral (dbuf, umbp, imi, iumbp, 'd')
	 itend(n)=iumbp
	 icero=imi
	 call detectar_cero (dbuf, icero, 'i')
	 call busca_pic2(imi-kdis, dbuf, ipic, 'i')
	 if (ipic.gt.icero) then
	    itpos2(n)=ipic
	  else 
	    itpos2(n)=icero
	 end if
	 if (itpos2(n).le.isend(n)) itpos2(n)=isend(n)+1

	 itpos(n)=0
	 itbeg(n)=0
         go to 100

c	 tenim ona T bifasica -+

 50	 umba=ymina/ktb
	 call creuar_umbral(dbuf, umba, imia, iumba, 'i')
	 if (iumba.lt.isend(n)) call busca_pic2(imia, dbuf, iumba, 'e')
c 	 si encara hem anat massa enrera l'associem a isend
	 if (iumba.le.isend(n)) iumba=isend(n)+2 
	 itbeg(n)=iumba

C        kte valdr` 4,5,6,7 en funcis del valor de la derivada max.
          if (abs(yminp).ge.0.41) then
             kte=7
         else if (abs(yminp).ge.0.30) then
             kte=6
         else if (abs(yminp).ge.0.20) then
             kte=5
         else if (abs(yminp).ge.0.10) then   
             kte=4
	 else if (abs(yminp).lt.0.10) then   
             kte=3.5
         end if
        
         if (kte/back(5).ge.1.1) then
	      umbp=yminp*back(5)/kte
	 else 
	   umbp=yminp/1.1
	 end if

	  
	 call creuar_umbral(dbuf, umbp, imip, iumbp, 'd')
	 itend(n)=iumbp
	 icero=imia
	 call detectar_cero (dbuf, icero, 'd')
	 itpos2(n)=icero
	 icero=imip
	 call detectar_cero (dbuf, icero, 'i')
	 itpos(n)=icero
         if (itpos(n).lt.itpos2(n)) itpos(n)=itpos2(n)
	 go to 100

c	 tenim ona T bifasica +-
 60	 umba=ymaxa/ktb
	 call creuar_umbral(dbuf, umba, imaa, iumba, 'i')
	 if (iumba.lt.isend(n)) call busca_pic2(imaa, dbuf, iumba, 'e')
c 	 si encara hem anat massa enrera l'associem a isend
	 if (iumba.le.isend(n)) iumba=isend(n)+2 
	 itbeg(n)=iumba

C        kte valdr` 4,5,6,7 en funcis del valor de la derivada max.
         if (abs(ymaxp).ge.0.41) then
             kte=7
         else if (abs(ymaxp).ge.0.30) then
             kte=6
         else if (abs(ymaxp).ge.0.20) then
             kte=5
         else if (abs(ymaxp).ge.0.10) then   
             kte=4
	 else if (abs(ymaxp).lt.0.10) then   
             kte=3.5
         end if
         
         if (kte/back(6).ge.1.1) then
	      umbp=ymaxp*back(6)/kte
	 else 
	   umbp=ymaxp/1.1
	 end if
 	
         call creuar_umbral(dbuf, umbp, imap, iumbp, 'd')
	 itend(n)=iumbp
	 icero=imaa
	 call detectar_cero (dbuf, icero, 'd')
	 itpos2(n)=icero
	 icero=imap
	 call detectar_cero (dbuf, icero, 'i')
	 itpos(n)=icero
	 if (itpos(n).lt.itpos2(n)) itpos(n)=itpos2(n)
         go to 100

c	 no tenim ona t
 70	 itbeg(n)=0
	 itpos(n)=0
	 itpos2(n)=0
	 itend(n)=0
	 go to 100

c 	 apliquem una regla de validesa respecte el interval QT
         
c 100 	    write(6,3) n,  itend(n)-iqbeg(n), 750/samp
c 3 	    format('beat=',i2,' QT=', i4, '  QT limite=', f4)
 	  
 100        if (itend(n).ge.isf) go to 70
c	    write(6,*)n, 'isf=', isf, itend(n)
	    if (itend(n)-iqbeg(n).lt.950/samp.and.
     &       (iqrs(n+1)-itend(n).gt.itqlim.or.iqrs(n+1).eq.0.or.
     &        itend(n)-iqbeg(n).lt.400/samp) ) then
		flag1=.false.
	      else
 		if (iew.gt.ibw+100/samp) then
	            iew=iew-nint(50/samp)
		  else
	            iew=iew-nint(25/samp)
	        end if
                call buscamaxmin (ibw, iew, dbuf, imi, ymin, ima, ymax)
	        back(morf(4,n)+1)=back(morf(4,n)+1)*1.8
 	      end if
c	  write(6,*) n,itend(n), iqrs(n+1),itend(n)-iqbeg(n)
  	 end do
        
         if (itend(n).gt.iqrs(n+1)-100/samp.and.iqrs(n+1).ne.0) then
           itend(n)=0
	   itpos(n)=0
	   itbeg(n)=0
	   itpos2(n)=0
	 end if
     
	 return
	 end



c------------------------------------------------------------------------------
	 subroutine onatold (n, dbuf, rrmed, isf, irpos, iqbeg, itbeg, itpos,
     &				itend, samp, if, isend,iqrs)          

c	 subrutina vella on no esta implementat la posiblitat de ones T
c	 bifasiques.
c	 ens localitza els punts inicial, pic i final de la ona T

	 dimension dbuf(100000), irpos(8000), iqbeg(8000), itbeg(8000),itpos(8000)
	 dimension itend(8000), isend(8000),iqrs(8000)
	 logical flag1, inici_t, fivec

c 	 flag1: variable de control de bucle
c	 inici_t: ens diu si podem buscar el inici de la ona (T normal o
c	          invertda)
c	 bwind, ewind: amplada de la finestra
c	 ibw: posicio a partir de la que comengarem a buscar
c	 iew: posicio fins la que buscarem
c 	 ktb, kte: constants per definir els umbrals de la derivada per
c 	 	   localitzar l'inici i final de la ona T respectivament
	 ktb=3
	 kte=2
	 kdis=nint(20/samp)
	 bwind=100
	 ewind=450
	 ibw=irpos(n)+nint(bwind/samp)
	 iew=irpos(n)+nint(ewind/samp)
c	 filtrem passa-baix la derivada del senyal compresa entre les finestres
c	 call filtre_pb (ibw, iew-ibw, if, 30, dbuf,retard)
	 flag1=.true.
	 inici_t=.true.
c	 inicialitzem finestres
	 call inifinestres(rrmed, isf, irpos(n), bwind, ewind, ibw, iew, 
     &				samp, fivec)
c	 posem un reajust per QRS molt amples i per intervals rr anormalment
c	 petits.
	 if(ibw.le.isend(n)+kdis) then
		iew=iew+isend(n)-ibw+kdis
		ibw=isend(n)+kdis
	 end if
	 if (iqrs(n+1).gt.0.and.iew.gt.iqrs(n+1)-100/samp) 
     &		iew=iqrs(n+1)-100/samp
         if (fivec) return
c	 posem una altre condicio per detectar be T de pujada o baixada
	 call buscamaxmin (ibw, iew, dbuf, imi, ymin, ima, ymax) 
	 if (ymin.gt.0.or.ymax.lt.0) then
	   do while (ymin.gt.0.or.ymax.lt.0)
	     iew=iew+25/samp
	     call buscamaxmin (ibw, iew, dbuf, imi, ymin, ima, ymax) 
           end do
	   iew=iew+100/samp
	 end if
	 if (iqrs(n+1).gt.0.and.iew.gt.iqrs(n+1)-100/samp)
     &		iew=iqrs(n+1)-100/samp
	 do while (flag1.and.iew.gt.ibw)
	   ieaux=nint(iew-50/samp)
	   call buscamaxmin (ibw, ieaux, dbuf, imi, ymin, ima, ymax) 
	   imig=imi
	   yming=ymin
	   if (imi.lt.ima) 
     &		call buscamaxmin (ima, iew, dbuf, imi, ymin, iaux1, aux2)
c	 regla de decisio per clasificar la ona T com a normal, invertida, 
c 	 depujada o de baixada
	   if (imi.ne.imig.and.ymin.gt.0.or.
     &		abs(ymin).lt.ymax/3.or.
     &		abs(yming)/4.gt.ymax.and.abs(yming)/4.gt.abs(ymin)) then
	     if (abs(yming).gt.3*ymax.or.ima.gt.iew-50/samp) then
c 	 tenim ona T de nomes baixada
	         imi=imig
	         ymin=yming
		 inici_t=.false.
	       else
c	 tenim ona T invertida o be de pujada
	   if (imi.eq.imig.and.ymax.gt.3*abs(ymin)) then
c	 tenim ona t de nomes pujada
		ymin=ymax
	 	imi=ima
		inici_t=.false.
	     else
c	 tenim ona T invertida
		 imi=ima
 	         ymin=ymax
		 ima=imig
	 	 ymax=yming
	   end if
	     end if
	   end if
c 	 altrament tenim ona T normal
	   icero=imi
c	 busquem el cero anterior al que en cada cas correspongui a la posicio
c	 imi, i que en principi sera el pic de la ona T
	   call detectar_cero (dbuf, icero, 'i')
c	 si hem anat mes enrera de isend associem el pic a la primera vall 
c	 a la esquerra
   	 if (icero.lt.isend(n)) call busca_pic2(imi, dbuf,icero, 'e')
c	 buquem el final de la ona T definint un umbral de la derivada
	   umbral=ymin/kte
	   call creuar_umbral (dbuf, umbral, imi, iumb, 'd')
c 	 apliquem una regla de validesa respecte el interval QT
	   if (iumb-iqbeg(n).lt.620/samp) then
		flag1=.false.
		itpos(n)=icero
		itend(n)=iumb
	     else
		iew=iew-50/samp
	   end if
	 end do
	 itpos(n)=icero
c	 busquem el inici de la ona T mitjangant un umbral
	 if(inici_t) then
		umbral=ymax/ktb
		call creuar_umbral(dbuf, umbral, ima, iumb, 'i')
c 	 si hem anat mes enrera de isend, trobem el principi de la T
c	 amb un criteri del primer pic a l'esquerra
		if (iumb.lt.isend(n)) call busca_pic2(ima, dbuf, iumb, 'e')
c 	 si encara hem anat massa enrera l'associem a isend
		if (iumb.lt.isend(n)) iumb=isend(n)
		itbeg(n)=iumb
	    else
		itbeg(n)=0
	 endif
	 return
	 end



c------------------------------------------------------------------------------


	 subroutine inifinestres (rrmed, isf, irp, bwind, ewind, ibw,
     &					 iew,samp, fivec)

c	 inicialitza les finestres a on es buscara l'ona T.

	 logical fivec
         fivec=.false.
	 if (irp+ewind/samp.lt.isf)then
		if (rrmed.lt.750/samp) then
c			ibw=nint(100/samp)+irp
			ibw=nint(125/samp)+irp
			iew=nint(rrmed*0.6)+irp
		else
			ibw=nint(bwind/samp)+irp
			iew=nint(ewind/samp)+irp
		end if
	    else
		if (irp+bwind/samp.lt.isf) then
			ibw=nint(bwind/samp)+irp
			iew=isf
		else
			fivec=.true.
		endif
	 end if
	 return
	 end				 
c ---------------------------------------------------------------------------

         subroutine filtre_pb (ni,n,if,ifc,x,retard)
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
         retard=l
         if (n.le.(400-l+1)) then
                              do i=1,n
                                x(ni+i)=y(i+l)
                              end do
                             else
                              do i=1,400-l+1
                                x(ni+i)=y(i+l)
                              end do
c                             do i=400-l+1,n
c                               y(i)=0
c                             end do
         end if 

C        PONGO EL RESTO A CERO
         do i=n+1,400
            y(i)=0
         end do
 
         return
         end

C        ---------------------------------------------------------------


	 subroutine onap (n, dbuf, isf, irpos, iqbeg, ipbeg, ippos1,
     &	   ipend, samp, if, dermax, itend, ecgpb,ecg,iqrs)
                         
c	 Aquesta subroutina ens busca inici i final de la ona P. Considera
c	 tant ones P normals com invertides.

	 dimension dbuf(100000), irpos(8000), iqbeg(8000), ipbeg(8000), ipend(8000)
	 dimension ippos1(8000), ippos2(8000), itend(8000) ,ecgpb(100000)
         dimension ecg(100000), iqrs(8000)
	 logical nofi

c	 bwind, ewind: amplada de la finestra
c	 ibw: posicio a partir de la que comengarem a buscar
c	 iew: posicio fins la que buscarem
c 	 kpb, kpe: constants per definir els umbrals de la derivada per
c 	 	   localitzar l'inici i final de la ona P respectivament
	 counter=0
	 nofi=.true.
	 kpb=2
	 kpe=2
	 bwind=200
c	 bwind=380
	 ewind=30
	 ippos1(n)=0
	 iew=iqbeg(n)-nint(ewind/samp)
	 ibw=iqbeg(n)-nint(bwind/samp)
	 if (ibw.lt.0) ibw=0
	 if (iew.lt.0) iew=0

         do while (nofi.and.ippos1(n).eq.0.and.iqbeg(n)-ibw.lt.300/samp)
c 	 no busquem mes enlla del final de T anterior, i a una distancia minima
c	 de qbeg (Vigo : he canviat 330 per 400)
	    if (n.eq.1.or.itend(n-1).eq.0) then
	       nofi=.false.
	    else
	       if (ibw.lt.itend(n-1)) then
		  ibw=itend(n-1)
		  nofi=.false.
	       else
		  nofi=.true.
	       end if
	    end if
	    kdis=nint(20/samp)
c              Estaba cuando los limites se amrcaban desde el QRS
c	 if(iqbeg(n)-iew.lt.kdis) then
c 	   iew=iqbeg(n)-kdis
c	 end if
	    if (iew.lt.0) iew=0

c 	 busquem el maxim i el minim dins la finestra
 10	    call buscamaxmin (ibw, iew, dbuf, imi, ymin, ima, ymax)
C---------------------------------------------------------------------------
C        Aixs is el criteri de l'Audal:
C 	 si les pendents son molt inferiors a les del QRS o be 
c	 si la posicio del minim esmenor que la del maxim, no tenim ona P
c	 if (imi.le.ima.or.ymax.lt.dermax/50.or.abs(ymin).lt.dermax/50) then

c	 tambe detectem ones P invertides, pertant nomes si els pendents
c	 son molt inferiors als del QRS, sent mes exigents en les invertides 
c	 per tal de no detectar-ne de falses.
C---------------------------------------------------------------------------
c   Vigo: canvio 70 per 30 per detectar millor ones p molt prrximes a la q
	    if (imi.le.ima.and.iqbeg(n)-ima.lt.30/samp) then
	       iew=iqbeg(n)-nint(30/samp)
	       ibw=ibw-nint(30/samp)
	       call buscamaxmin (ibw, iew, dbuf, imi, ymin, ima, ymax)
	    end if
C---------------------------------------------------------------------------
C     Aixs is el criteri de l'Audal:
C	 if (ima.le.imi.and.(ymax.lt.dermax/50.or.abs(ymin).lt.dermax/50)
c     &.or.ima.ge.imi.and.(ymax.lt.dermax/25.or.abs(ymin).lt.dermax/25))
c     & then
C-------------------------------------------------------------------------
C     Ara se'n adopta un altre:
C     si la diferencia d'amplituds entre la posicio de R i la del punt de 
C     maxima amplitud a la finestra on busquem P, es superior a un cert valor
C     considerarem que no existeix ona P. Les amplituds ser`n buscades en 
C     la senyal ecgpb. Primer calculem el punt de m`xima amplitud absoluta en
C     la finestra de bzsqueda de P, i despres podrem comparar-la. De fet con-
C     siderarem bones aquelles ones P l'amplitud de les quals sigui superior a
C     un 7% de la de la ona R, o que tinguin una pendent superior al 1.4% la
C     del complexe QRS. 
C
	    base=0
	    itimes=0
	    do ii=1, nint(15/samp)
	       base=base+(ecgpb(iqbeg(n)-ii))
	       itimes=itimes+1
	    end do
	    base=base/itimes
	    ecgpbmax=0.
	    do ii=ibw,iew
	       if (abs(ecgpb(ii)-base).ge.ecgpbmax) then
		  ecgpbmax=abs(ecgpb(ii)-base)
		  posmax=ii
	       end if
	    end do
c	write(6,*) n, ecgpbmax, abs(ecgpb(iqrs(n))), ymax, ymin,dermax
	    if (ecgpbmax.le.(abs(ecgpb(iqrs(n))-base)/30).or.      
     &          ((ymax.lt.dermax/100.and.abs(ymin).lt.dermax/100).and.
     &          (ymax.lt.abs(ymin)/1.5.or.ymax.gt.abs(ymin)*1.5)) )then
	       ippos1(n)=0
	       ippos2(n)=0
	       ipbeg(n)=0
	       ipend(n)=0
	    else
c	 definim si es normal o invertida
	       if (imi.le.ima) then
c	 tenim ona P invertida
		  iaux=imi
		  yaux=ymin
		  imi=ima
		  ymin=ymax
		  ima=iaux
		  ymax=yaux
	       end if	 

c 	 busquem els dos ceros a dreta i esquerra de la posicio del maxim i
c	 minim respectivament, que correspondran als posibles pics de la ona P
	       icero1=ima
	       call detectar_cero (dbuf, icero1, 'd')
	       icero2=imi
	       call detectar_cero (dbuf, icero2, 'i')
c	 si els dos pics disten mes de un cert valor
c	 if (icero2-icero1.gt.20/samp) then
c	 considerem que la ona p te dos pics 
c	   	ippos1(n)=icero1
c	  	ippos2(n)=icero2
c	   else
c	 considerem que el pic esta en el cente dels dos pics
	       ippos1(n)=nint((icero1+icero2)/2.)
	       ippos2(n)=0
c	 end if
c	 busquem l'inici i final de la ona P segons el criteri del umbral de
c	 la derivada
c        Vigo: fem Kpb=1.65 i ara 1.5 i ara 1.35
	       umbral=ymax/1.35
c        umbral=ymax/kpb
	       ima2=ima
 20	       call creuar_umbral (dbuf, umbral, ima2, iumb, 'i')
	       if ((iqbeg(n)-iumb).ge.240.or.
     &         (iumb.le.itend(n-1).and.n.gt.1.and.itend(n-1).ne.0)) then 
		  ibw=ibw+20
		  if (ibw.gt.iew-20/samp) go to 100
		  call buscamaxmin (ibw,iew,dbuf,imi2,ymin2,ima2,ymax2)
		  goto 20
	       end if

	       call busca_soroll(iumb-40,iumb-5,ecg,soroll)
c        D. Vigo: confirmem que l'inici trobat es bo; fem un test comparant
c              l'algada del'ona p trobada amb l'algada de soroll, i comprobant
c              que l'inici de p no estgui massa aprop del pic de p.
	       if (abs(ecgpb(iumb)-ecgpb(ippos1(n))).lt.(1.5*soroll).and.
     &             (ippos1(n)-iumb).lt.40/samp) then
		  iew2=ima2-nint(15./samp)
		  if (iew2.gt.itend(n-1).and.iew2.ge.ibw) then
		     call buscamaxmin (ibw,iew2,dbuf,imi2,ymin2,ima2,ymax2)
		     if (ymax2.ge.ymax/2) then
			go to 20
		     end if
		  end if
	       end if
	       ipbeg(n)=iumb
	       umbral=ymin/kpe
	 
	       call creuar_umbral (dbuf, umbral, imi, iumb, 'd')
c	 si el umbral es rebassa mes enlla de iqbeg definim el final amb un
c	 criteri de la derivada minima
	       if (iumb.ge.iqbeg(n)) 
     &             call buscamaxmin(imi,iqbeg(n),dbuf,iumb,ymin,iau,yau)
	       ipend(n)=iumb
c	 si encara hem anat massa enll`, fem ipend=iqbeg
	       if (ipend(n).ge.iqbeg(n)) then 
		  if (counter.eq.0) then
		     iew=iew-nint(10/samp) 
		     counter=1
c         write(6,1) ipend(n), iqbeg(n)
c 1 	 format('Pend=',i4,' Qbeg=',i4)
		     go to 10
		  else
		     ipend(n)=iqbeg(n)-1
		  end if
	       end if 

C        comprobem que la diferencia d'algades entre pic i final de P sigui
C        significativa respecte el soroll calculat
	       call busca_soroll(ibw,iew,ecg,soroll)
	       if(abs(ecgpb(ippos1(n))-ecgpb(ipend(n))).le.(1.5*soroll))then
		  goto 100
	       end if

c	 fem una verificacio dels resultats obtinguts per validar la ona P
	       if (ipbeg(n).ge.ipend(n).or.ippos1(n).le.ipbeg(n)
     &             .or.ippos1(n).ge.ipend(n).or.ipbeg(n).lt.itend(n-1)
     &             .or.ipend(n)-ipbeg(n).gt.180/samp) then
c    &             .or.ipend(n)-ipbeg(n).gt.150/samp) then
		  go to 100
	       else
		  go to 110
	       end if
	    end if
 100	    ippos1(n)=0
	    ippos2(n)=0
	    ipbeg(n)=0
	    ipend(n)=0
 110	    iew=iew-50./samp
	    ibw=ibw-50./samp
c       eliminem el bucle de buscar la ona P mes enrera si no la trobem a la
c       primera finestra
c       nofi=.false.
	 end do
	 return
	 end


c----------------------------------------------------------------------------
	 subroutine calcula_rmedio (rrmed, n, samp, irpos)

c	 aquesta subrutina calcula el la distancia mitja entre les ones R
c	 consequtives. Aquells intervals que es desviin molt de la mitja son
c	 rebutjats

	 dimension irpos(8000)

	 perc=0.5
	 if (n.le.1) then
	    if (rrmed.eq.0) then
		rrmed=irpos(2)-irpos(1) 
	        if (rrmed.gt.1000/samp.or.rrmed.lt.500/samp)
     &                   rrmed=nint(650/samp)
	      else
		return
	    end if
	  else
	    if (n.lt.7) then	
	       if (irpos(n)-irpos(n-1).gt.rrmed*(1-perc).and.
     &		irpos(n)-irpos(n-1).lt.rrmed*(1+perc)) then
     	       rrmed=rrmed*(n-2)/(n-1)+(irpos(n)-irpos(n-1))/(n-1)
	       end if
	     else
	       if (irpos(n)-irpos(n-1).gt.rrmed*(1-perc).and.
     &		irpos(n)-irpos(n-1).lt.rrmed*(1+perc)) then
                  rrmed=rrmed*4/5+(irpos(n)-irpos(n-1))/5
	       end if
	    end if
	 end if
         	
c        write(6,1) rrmed
c 1 	 format('rrmed=',f8)
	 
	 return
	 end

c---------------------------------------------------------------------------


	 subroutine test_pic (seny, lpic, samp, t)

c	 aquesta subrutina ens fa un test per comprobar si el pic trobat 
c	 amb el criteri de la derivada nula correspon exactament amb el
c	 pic del senyal 'seny'.
c	 't' ens diu si tenim un pic 'p' o una vall 'v'
	
	 dimension seny(100000)
	 character*1 t

	 kpos=nint(10/samp)
c 	 busquem kpos posicions a dreta i esquerra de lpic
	 laux=lpic
	 if (t.eq.'p') then	 
	   do i=lpic-kpos, lpic+kpos
	   if(seny(i).gt.seny(laux)) laux=i
	   end do
	 else
	   do i=lpic-kpos, lpic+kpos
	   if(seny(i).lt.seny(laux)) laux=i
	   end do
	 end if
	 lpic= laux
	 return 
	 end
	 
c----------------------------------------------------------------------------


 	 subroutine cal_base(n,ippos,ecg,basel,samp,ipend,iqbeg)     
  
 	 dimension basel(8000),ipend(8000), iqbeg(8000),ippos(8000)
	 dimension ecg(100000)

c	 busquem la linea de base com la mitjana dels punts del segment PR

	 nqui=nint(15./samp)
	 ntre=nint(30./samp)
         ntre_q = nint(10./samp)
         nqui_q = nint(5./samp)

	 if (iqbeg(n).ne.0) then
	   if (ippos(n).ne.0) then   
	 	sum=0.
		if (iqbeg(n)-ipend(n).gt.33/samp) then
			do k=ipend(n)+nqui, iqbeg(n)-nqui
			   sum=sum+ecg(k)
			end do
			baselin=sum/(iqbeg(n)-nqui-nqui-ipend(n)+1)
		else
			if(iqbeg(n).eq.ipend(n)) then
			  baselin=ecg(iqbeg(n))
			else
			  do k=ipend(n), iqbeg(n)
			   sum =sum+ecg(k)
			  end do
			  baselin= sum/(iqbeg(n)-ipend(n)+1)
			end if
		end if
	   else
		sum=0.
	 	if(iqbeg(n)-nqui_q.gt.1) then
		  k=iqbeg(n)-nqui_q
		  do while(k.ge.iqbeg(n)-ntre_q-nqui_q.and.k.gt.0)
			sum=sum+ecg(k)
			k=k-1
		  end do
		  if(k.eq.0)then
			baselin=sum/(iqbeg(n)-nqui_q)
		    else
			baselin=sum/(ntre_q+1)
		  end if
	        else
		  do k=1,iqbeg(n)
			sum=sum+ecg(k)
		  end do
		  baselin=sum/iqbeg(n)
		end if
	 end if
	 basel(n)=baselin
	 end if
 	 return 
	 end

c        --------------------------------------------------------------
         subroutine busca_soroll(ibw,iew,ecg,soroll)

         dimension ecg(100000)

         if (ibw.lt.0) ibw=0
         if (iebw.lt.0) iew=0

         soroll=0.
         inici=ibw

         ncont=0
         do while (inici.lt.iew)
            ncont=ncont+1
            ifinal=inici+5
            if (ifinal.gt.iew) then 
                ifinal=iew
            end if

            call buscamaxmin(inici,ifinal,ecg,imi2,ymin2,ima2,ymax2)
            soroll=(ymax2-ymin2)+soroll
            inici=ifinal
         end do 

         if (ncont.gt.0) then
            soroll=abs(soroll/ncont)
         end if

        return 
       end


