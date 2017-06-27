c	 *******************************************************************
c         DADES.FOR 
c	 SUBRUTINES QUE ENS ELABOREN LES DADES IMPORTANTS A PARTIR DELS
c	 PUNTS SIGNIFICATIUS TROBATS EN PUNTS.FOR, I ENS LES MOSTREN
c         Eudald Bogatell (1-6-1991)
c         David Vigo Anglada (9-1992)
c	 *******************************************************************

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

	 subroutine dades_sig (ecg, ipbeg, ippos, ipend, iqbeg,
     &	      iqpos, iqend, irpos, irrpos, isbeg, ispos, isend,
     &	      itbeg, itpos, itend, iqrs, jqrs, ifm, rmax, max, f, ns,
     &	      basel, morf, amprr, ampr, ampq, amps, ampp, ampt, durpic,
     &	      qrsint, durr, durrr, durq, durs, durp, durt, rrint, print, 
     &	      ppint, qtint, qtpint, ritme, rrmed, tip, pendr, itpos2,
     &	      idis, nep, qtcint, qtpcint, k, kder, ST_am, ST_pe,
     &        ST_in, ST_ar, freV, desV) 

c	 Aquesta subrutina es cridada desde ecgmain com una opcio, i ens exposa
c	 les dades de manera que s'en puguin treure conclusions directament

	 dimension irpos(8000),ecg(100000),iqrs(8000),pendr(8000)
	 dimension itpos(8000)
	 dimension ipbeg(8000),ippos(8000),ipend(8000),iqbeg(8000)
	 dimension iqpos(8000),ispos(8000),isend(8000),itbeg(8000)
	 dimension itend(8000), itpos2(8000), irrpos(8000)
	 dimension iqend(8000)
	 dimension isbeg(8000), morf(5,8000),i_base(8000)
 	 dimension ST_am(8000),ST_pe(8000),ST_in(8000),ST_ar(8000)
	 dimension amprr(8000), ampr(8000), ampq(8000), amps(8000)
	 dimension durq(8000)
         dimension ampp(8000), ampt(8000), durp(8000), durt(8000)
	 dimension durpic(8000),qrsint(8000),durr(8000),durrr(8000)
	 dimension durs(8000),rrint(8000),print(8000),ppint(8000)
	 dimension qtint(8000)
	 dimension qtpint(8000),basel(8000),idis(3,20),freV(8000)
	 dimension desV(8000)
	 dimension qtcint(8000), qtpcint(8000)
	 character*12 f
	 character*4 tip(8000)

c	 ecg: senyal ECG
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
c	 ampq, ampr, amps, amprr: amplituds de les ones Q, R, S i S' 
c	 durq, durr, durs, durrr: durades de les ones Q, R, S i R'
c	 rrint, ppint, print: intervals RR, PP i PR
c	 qtint, qtpint: intervals QT i QT de pic
c	 qtcint, qtpcint: intervals QT i QT de pic corretgits.
c	 durpic i pendr: duracio i pendent de la primera pujada de la R
c 	 morf: vector d'enters que ens informa de la morfologia del bateg
c		morf(1,j) -bateg	0: rr normal
c					1: rr llarg
c					2: rr curt
c					3: primer bateg
c
c		morf(2,j) -complex QRS  0: qRs
c					1: qR
c					2: Rs
c					3: R
c					4: rS
c					5: RS
c					6: rsR'
c					7: QS
c					8: notched R ,NR
c                   			9: WQS
c					10:Taquicardia ventricular (VT)
c					11:Fibrilacio ventricular (VF)
c
c		morf(3,j) -ona P	0: existeix, normal
c					1:    "    , invertida
c					2: no existeix
c
c		morf(4,j) -ona T	0: existeix, normal
c					1:    "    , invertida
c					2:    "    , nomes pujada
c					3:    "    , nomes baixada
c					4: bifasica -+
c					5:    "     +-
c					6: no existeix


c	 conv: factor de conversio a volts
	 conv= 1.
	 samp=1000.0/ifm
	 jep=1
c	 ritme: ritme cardiac en pulsacions per min 
	 ritme= 60*ifm/rrmed
	 call baseline(jqrs,ippos,ecg,basel,samp,ipbeg,ipend,iqbeg,
     &                 isend,i_base)     

	 do i=1, jqrs

c	 anem omplint el vector de morfologia, al mateix temps que trobem
c	 algunes dades importants

c	 si el qrs cau dins dun episodi de VT o VF no hi busquem cap complexe
c	 QRS
	   if (nep.gt.0.and.iqrs(i).ge.(idis(1,jep)+is)*ifm-ifm*0.3.and.
     &	jep.le.nep) then
           if (iqrs(i).le.(idis(2,jep)+is)*ifm) then
		if (idis(3,jep).eq.1) then
		    morf(2,i)=10
	  	    tip(i)=' VT '
		else if(idis(3,jep).eq.2) then
		    morf(2,i)=11
	  	    tip(i)=' VF '
                end if
		morf(3,i)=2
		morf(4,i)=6
		go to 500
	      else
		jep=jep+1
	     end if
	   end if

	 if (ippos(i).eq.0) then 
		morf(3,i)=2
	    else
		if (ecg(ippos(i)).lt.ecg(ipbeg(i)).and.
     &			ecg(ippos(i)).lt.ecg(ipend(i))) then
		    morf(3,i)=1
		  else
		    morf(3,i)=0
		end if
	 end if


c 	 definim la morfologia de l'ona T
c	 JA LA TENIM DEFINIDA
c         if(itpos(i).eq.0) then
c		morf(4,i)=4
c	    else
c	      if (itbeg(i).eq.0) then
c		if (ecg(itpos(i)).gt.ecg(itend(i))) then
c		    morf(4,i)=3
c		  else
c		    morf(4,i)=2
c		end if
c	       else
c		if (ecg(itpos(i)).lt.ecg(itbeg(i)).or.
c     &		     ecg(itpos(i)).lt.ecg(itend(i))) then
c			morf(4,i)=1
c		   else 
c			morf(4,i)=0
c		end if
c	      end if
c	 end if

c	 busquem les amplituts de les ones del QRS que mes tard ens serviran 
c	 per definir la morfologia del QRS

	 if (irrpos(i).ne.0) then
	    amprr(i)=abs((ecg(irrpos(i))-basel(i))*conv)
	  else
	    amprr(i)=0.
	 end if
	 ampr(i)=(ecg(irpos(i))-basel(i))*conv
	 if (iqpos(i).eq.0) then
	    ampq(i)=0.
	  else
	    ampq(i)=abs((ecg(iqpos(i))-basel(i))*conv)	
	 end if
	 if (ispos(i).eq.0) then
	    amps(i)=0.
	  else
	    amps(i)=abs((ecg(ispos(i))-basel(i))*conv)
	 end if

c        busquem les amplituds de les ones P :
	 if (ippos(i).eq.0) then
	    amps(i)=0.
	  else
	    ampp(i)=abs((ecg(ippos(i))-basel(i))*conv)
	 end if
c        i per l'ona T:
	 if (morf(4,i).eq.0.or.morf(4,i).eq.1) then 
	    ampt(i)=abs((ecg(itpos(i))-ecg(itend(i)))*conv)
	  else if (morf(4,i).eq.2.or.morf(4,i).eq.3) then 
            if (ispos(i).ne.0) then
               iaux=isend(i)
            else 
               iaux=basel(i)
            end if
	    ampt(i)=abs((ecg(itend(i))-ecg(iaux))*conv)
	  else if (morf(4,i).eq.4.or.morf(4,i).eq.5) then 
	    ampt(i)=abs((ecg(itpos(i))-ecg(itend(i)))*conv)
            iaux=abs((ecg(itpos2(i))-ecg(itend(i)))*conv)
            if (iaux.gt.ampt(i)) ampt(i)=iaux
          else
            ampt(i)=0.
	 end if

c	 definim la morfologia del QRS

	 tip(i)='????'
	 if (amprr(i).gt.0) then
	    if (amps(i).le.0.) then
		morf(2,i)=8
		tip(i)='NR'
	      else
		morf(2,i)=6
		tip(i)='rsRp'
	    end if
	  else
	    if (ampr(i).le.0.) then
		if (ampq(i).gt.0..and.amps(i).gt.0.0) then
			morf(2,i)=9
			tip(i)=' WQS'
		end if
		if(ampq(i).le.0..and.amps(i).gt.0..or.ampr(i).lt.0) then
			morf(2,i)=7
			tip(i)='  QS'
		end if
	      else
		if(ampq(i).gt.0..and.amps(i).gt.0.) then
			morf(2,i)=0
			tip(i)=' qRs'
		end if
		if(amps(i).le.0..and.ampq(i).le.0.) then
			morf(2,i)=3
			tip(i)='   R'
		end if
		if(amps(i).le.0..and.ampq(i).gt.0.) then
			morf(2,i)=1
			tip(i)='  qR'
		end if
		if(amps(i).gt.0..and.ampq(i).le.0.) then
		  if(amps(i)*2.lt.ampr(i)) then
			morf(2,i)=2
			tip(i)='  Rs'
		     else 
			if(ampr(i)*2.lt.amps(i)) then
			   morf(2,i)=4
			   tip(i)='  rS'
			 else
			   morf(2,i)=5
			   tip(i)='  RS'
			end if
		  end if
		end if
	    end if
	  end if
            

c	 busquem altres intervals de interes
	 
	 qrsint(i)=(isend(i)-iqbeg(i))*samp		
	 durpic(i)=(irpos(i)-iqbeg(i))*samp

c	 durada de les ones del qrs

	 if (morf(2,i).eq.6) then
		durr(i)=(iqend(i)-iqbeg(i))*samp
		durs(i)=(isbeg(i)-iqend(i))*samp
		durrr(i)=(isend(i)-isbeg(i))*samp
		durq(i)=0.
	 else 
	 if(iqend(i).ne.0.and.isbeg(i).ne.0) then
		durr(i)=(isbeg(i)-iqend(i))*samp		
		durq(i)=(iqend(i)-iqbeg(i))*samp
		durs(i)=(isend(i)-isbeg(i))*samp
	 else if(iqend(i).eq.0.and.isbeg(i).ne.0) then
		durr(i)=(isbeg(i)-iqbeg(i))*samp		
		durq(i)=0.
		durs(i)=(isend(i)-isbeg(i))*samp
	 else if(iqend(i).ne.0.and.isbeg(i).eq.0) then
		durr(i)=(isend(i)-iqend(i))*samp		
		durq(i)=(iqend(i)-iqbeg(i))*samp
		durs(i)=0.
	 else if(iqend(i).eq.0.and.isbeg(i).eq.0) then
		durr(i)=(isend(i)-iqbeg(i))*samp		
		durq(i)=0.
		durs(i)=0.
	 end if
	 end if

c        durada de les ones P i T
	 if (ippos(i).ne.0) then
             durp(i)=(ipend(i)-ipbeg(i))*samp		
         else
             durp(i)=0.
         end if
	 if (itpos(i).ne.0) then
             durt(i)=(itend(i)-itbeg(i))*samp		
         else
             durt(i)=0.
         end if

c	 pendent de la ona R
	 if (ampr(i).gt.0) then
            if ( (irpos(i)-iqbeg(i)-nint(10/samp)).ne.0) then
	 pendr(i)=(ecg(irpos(i))-ecg(iqbeg(i)+nint(10/samp)))/
     &            (irpos(i)-iqbeg(i)-nint(10/samp))*conv/samp*1000.
            else
	 pendr(i)=(ecg(irpos(i))-ecg(iqbeg(i)+nint(10/samp)))/
     &            (irpos(i)-iqbeg(i))*conv/samp*1000.
            end if
	 end if
c	 morfologia del ritme

	 if (i.eq.1) then 
		morf(1,1)=3
		rrint(1)=0
	    else 
	 	morf(1,i)=0
		rrint(i)=(irpos(i)-irpos(i-1))*samp
	 end if
c	 fem les correccions necesaries pel cas complexe QS
	 if (ampr(i).lt.0.and.morf(2,i).eq.7) then
	 	durs(i)=durr(i)
		durr(i)=0
		pendr(i)=0
		ispos(i)=irpos(i)
		amps(i)=abs(ampr(i))
		ampr(i)=0
		durpic(i)=0
	 end if

c	 intervals PR i PP

	 if (morf(3,i).eq.2) then
	  	print(i)=0.
		ppint(i)=0.
	    else
		print(i)=(iqbeg(i)-ipbeg(i))*samp
		if (i.eq.1.or.morf(3,i-1).eq.2) then 
			ppint(i)=0.
		    else
			ppint(i)=(ippos(i)-ippos(i-1))*samp
		end if
	 end if

c	 intervals QT

	 if (morf(4,i).eq.6.or.iqbeg(i).eq.0.or.itend(i).eq.0) then
		qtint(i)=0.
	 	qtpint(i)=0.
		qtcint(i)=0.
	 	qtpcint(i)=0.
	    else
	      if (morf(4,i).eq.3) then
		qtpint(i)=(itpos2(i)-iqbeg(i))*samp
	       else
		qtpint(i)=(itpos(i)-iqbeg(i))*samp
	      end if
       	      qtint(i)=(itend(i)-iqbeg(i))*samp
c	 si el interval rr esta acotat busquem el qt corretgit
	      if(rrint(i).gt.0.8*rrmed*samp.and.
     &		rrint(i).lt.1.2*rrmed*samp) then
		qtcint(i)=qtint(i)/sqrt(rrint(i)/1000)
		qtpcint(i)=qtpint(i)/sqrt(rrint(i)/1000)
	       else
		qtcint(i)=0.
	 	qtpcint(i)=0.
	      end if
	 end if
 500	 continue	 
	 end do
	 call impr_dat(f,ifm,ns,jqrs,morf,amprr,ampr,ampq,amps,ampp,ampt,
     &	       durpic,qrsint,durr,durrr,durq,durs,durp,durt,rrint,print, 
     &	       ppint, qtint, qtpint, samp, ritme, rrmed*samp, tip,
     &	       irpos,pendr,k,kder,ST_am,ST_pe,ST_in,ST_ar,freV,desV) 
         call impr_qt(f(1:lnblnk(f))//'.qtc',jqrs,qtcint, qtpcint)
	 return
	 end

c---------------------------------------------------------------------------
c---------------------------------------------------------------------------

	 subroutine  impr_qt( tit,jqrs,qtcint, qtpcint)

c	 ens escriu en un fitxer els intervals QT i QTP corretgits
	 dimension qtcint(8000), qtpcint(8000)
	 character*16 tit

	 open(unit=1, file=tit)
	 write(1,*) ' INTERVALS QT I QT PIC CORREGITS '
	 do i=1,jqrs
	    write(1,*) i, qtcint(i), qtpcint(i)
	 end do
	 close (unit=1)
	 return
	 end

	    
c---------------------------------------------------------------------------

	 subroutine mos_dat(f,ifm,ns,jqrs,morf,amprr,ampr,ampq,amps,ampp,
     &		ampt,durpic,qrsint,durr,durrr,durq,durs,durp,durt,rrint, 
     &		print,ppint,qtint,qtpint,ritme,rrmed, tip, irpos, pendr,
     &		qtcint, qtpcint, ST_am, ST_pe, ST_in, ST_ar)                
 
c 	 aquesta subrutina ens ofereix les opcions de mostrar les dades


 	 dimension morf(5,8000),ST_am(8000),ST_pe(8000),ST_in(8000)
 	 dimension ST_ar(8000)
	 dimension amprr(8000),ampr(8000),ampq(8000),amps(8000)
	 dimension pendr(8000)
	 dimension durpic(8000),qrsint(8000),durr(8000),durrr(8000)
	 dimension durq(8000)
	 dimension durs(8000),rrint(8000),print(8000),ppint(8000)
	 dimension qtint(8000)
	 dimension qtpint(8000), irpos(8000),ampp(8000),ampt(8000)
	 dimension qtpcint(8000), qtcint(8000),durp(8000),durt(8000)
	 character*12 f
	 character*4 tip(8000)
	 character*1 op
	 logical nofi

	 samp=1000./ifm
	 write (6,30)
 30 	 format (///,t20,'OPCIONS MOSTRAR DADES',///)        
	 write(6,35)
 35 	 format (t20,'1 Pantalla',//,t20,'2 Impresora',//,
     &           t20,'3 Extret per pantalla',///,t20,'0 sortir')
 39	 write(6,40)
 40      format(////,'$',t20,'OPCIO: ')
         read(5,41,err=39) k
 41      format(I2)         
	 if (k.eq.0) then
	   return
	 else if(k.eq.1) then
 	 write(6,50)
 50      format(//,'$',t20,'Segon inicial [0.0]: ')
         read(5,*) segi
c 51      format(f4.1)         
	 call visualitzar(f,ifm,ns,jqrs,morf,amprr,ampr,ampq,amps,ampp,
     &        ampt,durpic,qrsint,durr,durrr,durq,durs,durp,durt,rrint, 
     &        print,ppint,qtint,qtpint,samp, ritme, rrmed, tip, irpos,
     &        segi, pendr, qtcint, qtpcint) 
	 else if(k.eq.2) then
	  open(unit=31, file=f(1:lnblnk(f))//'.inf')
 90	  write(6,100)
 100	  format(/,'$',t10,'Seleccio de bategs [s]: ')
	  read(5,110) op
 110	  format(a)
          call imp_capc(f, ifm, ns, jqrs, ritme, rrmed*samp)
	  if(op.eq.'n'.or.op.eq.'N') then
	   do i=1,jqrs	 
	    call imprimir(i,morf,amprr,ampr,ampq,amps,ampp,ampt,irpos,
     &		ifm,durpic,qrsint,durr,durrr,durq,durs,durp,durt,rrint, 
     &		print,ppint,qtint, qtpint,tip, pendr, qtcint, qtpcint, 
     &          ST_am, ST_pe, ST_in, ST_ar)
	   end do
	  else
	   nofi=.true.
	 do while(nofi)
 	  write(6,120)
 120	  format(/,'$',t10,'Instant anterior al bateg en seg. ',
     &           '[<qq> per sortir]:')
	  read(5,*,err=140) segi
c 130	  format(f4.1)
	  go to 150
 140	  nofi=.false.
c	  trobem a partir del segon anterior el bateg a mostrar
 150	  j=1
	  if (nofi) then
	   do while(irpos(j).lt.nint(segi*ifm).and.j.le.jqrs)
	     j=j+1
	   end do
	   call imprimir(j,morf,amprr,ampr,ampq,amps,ampp,ampt,irpos,ifm,
     &		durpic,qrsint,durr,durrr,durq,durs,durp,durt,rrint, 
     &		print,ppint, qtint, qtpint,tip, pendr, qtcint, qtpcint, 
     &          ST_am, ST_pe, ST_in, ST_ar)
	  end if		
	 end do
	 end if
	 close(unit=31)
	 else if (k.eq.3) then
	 call vis_dat(f, ifm, ns, jqrs, morf, amprr, ampr, ampq, amps,
     &		durpic, qrsint, durr, durrr, durq, durs, rrint, print, 
     &		ppint, qtint, qtpint, samp, ritme, rrmed*samp, tip,
     &		irpos, pendr) 
	 
	 end if
	 return 
	 end

c--------------------------------------------------------------------------

	 subroutine imprimir(i,morf,amprr,ampr,ampq,amps,ampp,ampt,
     &		irpos,ifm,durpic,qrsint,durr,durrr,durq,durs,durp,durt, 
     &		rrint,print,ppint,qtint,qtpint,tip,pendr,qtcint,qtpcint, 
     &          ST_am, ST_pe, ST_in, ST_ar) 

c	 aquesta subroutina ens mostra per impresora les dades obtingudes 
c 	 aixi com algunes clasificacions respecte les morfologies

 	 dimension morf(5,8000), irpos(8000), pendr(8000)
	 dimension amprr(8000),ampr(8000),ampq(8000),amps(8000)
	 dimension durpic(8000),qrsint(8000),durr(8000),durrr(8000)
	 dimension durq(8000)
	 dimension durs(8000),rrint(8000),print(8000),ppint(8000)
	 dimension qtint(8000)
	 dimension durt(8000)
	 dimension qtpint(8000),ampp(8000),ampt(8000),durp(8000)
	 dimension qtcint(8000), qtpcint(8000)
         dimension ST_am(8000),ST_pe(8000),ST_in(8000),ST_ar(8000)
	 character*4 tip(8000)
	 character*12 noe

	 noe='no existeix'
	 write(31,50) i, tip(i)
 50	 format(/,15x,'BATEG: ',i3,t47,'Tipus de complexe qrs: ',a4)
c 	 instant del bateg en segons
	 seg=1.*irpos(i)/ifm
	 if (morf(3,i).eq.0) then
	 write(31,63) seg
 63	 format(15x,'Instant:',f6.2,' seg',t47,'La ona P es normal')
	 else if (morf(3,i).eq.1) then
	 write(31,64) seg
 64	 format(15x,'Instant:',f6.2,' seg',t47,'La ona P es invertida')
	 else if (morf(3,i).eq.2) then
	 write(31,65) seg
 65	 format(15x,'Instant:',f5.1,t47,'La ona P no existeix')
	 end if

	 go to (500, 501, 502, 503, 504, 505, 506), morf(4,i)+1
 500	 write(31,66)
 66	 format(t47,'La ona T es normal')
	 go to 510
 501	 write(31,67)
 67	 format(t47,'La ona T es invertida')
	 go to 510
 502	 write(31,68)
 68	 format(t47,'Ona T de pujada')
	 go to 510
 503	 write(31,69)
 69	 format(t47,'Ona T de baixada')
	 go to 510
 504	 write(31,70)
 70	 format(t47,'Ona T bifasica -+')
	 go to 510
 505	 write(31,71)
 71	 format(t47,'Ona T bifasica +-')
	 go to 510
 506	 write(31,72)
 72	 format(t47,'La ona T no existeix')
 
 510     continue

	 if (morf(1,i).eq.3) then
		write(31,74) 'primer bateg'
 74		format(5x,'Amplada interval RR = ',a12)
	    else    
       		write(31,75) nint(rrint(i))
 75	 	format(5x,'Amplada interval RR =',i5,' ms')
 	 end if    
	 write(31,80) nint(qrsint(i))
 80	 format(5x,'Amplada interval QRS=',i5,' ms')
	 if (morf(4,i).eq.6) then
		write(31,85) noe, noe
 85		format(5x,'Amplada interval QT =',a12,
     &  t47,'Interval QTC =',a12)
		write(31,90) noe, noe
 90		format(5x,'Amplada interval QTP=',a12,
     &  t47,'Interval QTPC =',a12)
	    else    
		if (qtcint(i).eq.0) then
       		write(31,95) nint(qtint(i))
 95	 	format(5x,'Amplada interval QT =',i5,' ms',
     &  t47,'QTC = interval rr fora de limits')
		write(31,96) nint(qtpint(i))
 96		format(5x,'Amplada interval QTP=',i5,' ms',
     &  t47,'QTPC = interval rr fora de limits')
		else
       		write(31,98) nint(qtint(i)),nint(qtcint(i))          
 98	 	format(5x,'Amplada interval QT =',i5,' ms',
     &  t47,'Interval QTC =',i5)
		write(31,99) nint(qtpint(i)),nint(qtpcint(i))     
 99		format(5x,'Amplada interval QTP=',i5,' ms',
     &  t47,'Interval QTPC =',i5)
		end if
 	 end if                       
	 if (morf(3,i).eq.2) then
		write(31,105) noe
 105		format(5x,'Amplada interval PR =',a12)
		write(31,110) noe
 110		format(5x,'Amplada interval PP =',a12)
	    else    
       		write(31,115) nint(print(i))
 115	 	format(5x,'Amplada interval PR =',i5,' ms')
		if (morf(1,i).eq.3) then
		  write(31,116) 'primer bateg'
 116		  format(5x,'Amplada interval PP = ',a12)
	         else
		  if (morf(3,i-1).eq.2) then
		     write(31,117) noe
 117		     format(5x,'Amplada interval PP =',a12)
                    else
		     write(31,120) nint(ppint(i))
 120		     format(5x,'Amplada interval PP =',i5,' ms')
		  end if
		end if
 	 end if    
	 write(31,*)
	 write(31,123) ampp(i), nint(durp(i))
 123	 format(5x,'Amplitut de la ona P =',f8.3,' mV',
     &          t47,'Duracio de la ona P =',i4,' ms')
	 write(31,125) ampr(i), nint(durr(i))
 125	 format(5x,'Amplitut de la ona R =',f8.3,' mV',
     &          t47,'Duracio de la ona R =',i4,' ms')
	 write(31,130) ampq(i), nint(durq(i))
 130	 format(5x,'Amplitut de la ona Q =',f8.3,' mV',
     &          t47,'Duracio de la ona Q =',i4,' ms')
	 write(31,135) amps(i),nint(durs(i))
 135	 format(5x,'Amplitut de la ona S =',f8.3,' mV',
     &          t47,'Duracio de la ona S =',i4,' ms')
	 write(31,137) ampt(i), nint(durt(i))
 137	 format(5x,'Amplitut de la ona T =',f8.3,' mV',
     &          t47,'Duracio de la ona T =',i4,' ms')
	 if (morf(2,i).eq.6) then
	 write(31,140) amprr(i), nint(durrr(i))
 140	 format(5x,'Amplitut de la ona R',1h','=',f8.3,' mV',
     &          t47,'Duracio de la ona R',1h','=',i4,' ms')
	 end if
	 write(31,148) nint(pendr(i)),nint(durpic(i))
 148	 format(5x,'Pendent de la ona R  =',i8,' mV',1h/,'seg',
     &          t47,'Duracio pic de ona R=',i4,' ms',/)
	 write(31,150) ST_pe(i),ST_am(i),ST_ar(i),ST_in(i) 
 150	 format(5x,'Pendent segment ST =',f7.3,' mV/seg',
     &          t47,'Amplitud segment ST =',f7.3,' mV',/
     &          5x,'Area segment ST    =',f7.2,' uV*seg', 
     &          t47,'Index segment ST    =',f7.3,/)
	 write (31,160) 
 160	 format (5x,'--------------------------------------------------
     &-------------------------')
	 return
	 end

c--------------------------------------------------------------------------

	 subroutine visualitzar(f,ifm,ns,jqrs,morf,amprr,ampr,ampq,amps,
     &        ampp,ampt,durpic,qrsint,durr,durrr,durq,durs,durp,durt, 
     &	      rrint,print,ppint,qtint,qtpint,samp,ritme,rrmed,tip,irpos,
     &        segi, pendr, qtcint, qtpcint)

c	 aquesta subroutina ens mostra per pantalla les dades obtingudes 
c 	 aixi com algunes clasificacions respecte les morfologies

 	 dimension morf(5,8000),ampp(8000),ampt(8000),durp(8000)
 	 dimension durt(8000)
	 dimension amprr(8000),ampr(8000),ampq(8000),amps(8000)
	 dimension pendr(8000)
	 dimension durpic(8000),qrsint(8000),durr(8000),durrr(8000)
	 dimension durq(8000)
	 dimension durs(8000),rrint(8000),print(8000),ppint(8000)
	 dimension qtint(8000)
	 dimension qtpint(8000), irpos(8000)
	 dimension qtpcint(8000), qtcint(8000)
	 character*12 f, noe
	 character*4 tip(8000)
	 character*1 op

c	 trobem a partir del segon inicial el primer bateg a mostrar
	 j=1
	 do while(irpos(j).lt.nint(segi*ifm).and.j.le.jqrs)
	     j=j+1
	 end do
	 noe=' no existeix'
	 do i=j, jqrs
c	 write(6,45)
c 45	 format('1')
	 write(6,*)
	 call capcalera (f, ifm, ns, jqrs, ritme, rrmed*samp)
	 write(6,701) i, tip(i)
 701	 format(15x,'BATEG: ',i3,t47,'Tipus de complexe qrs: ',a4)
c 	 instant del bateg en segons
	 seg=1.*irpos(i)/ifm
	 if (morf(3,i).eq.0) then
	 write(6,631) seg
 631	 format(15x,'Instant:',f6.2,' seg',t47,'La ona P es normal')
	 else if (morf(3,i).eq.1) then
	 write(6,641) seg
 641	 format(15x,'Instant:',f6.2,' seg',t47,'La ona P es invertida')
	 else if (morf(3,i).eq.2) then
	 write(6,651) seg
 651	 format(15x,'Instant:',f5.1,t47,'La ona P no existeix')
	 end if
	 go to (800, 801, 802, 803, 804, 805, 806), morf(4,i)+1
 800	 write(6,661)
 661	 format(t47,'La ona T es normal')
	 go to 810
 801	 write(6,67)
 67	 format(t47,'La ona T es invertida')
	 go to 810
 802	 write(6,68)
 68	 format(t47,'Ona T de pujada')
	 go to 810
 803	 write(6,69)
 69	 format(t47,'Ona T de baixada')
	 go to 810
 804	 write(6,70)
 70	 format(t47,'Ona T bifasica -+')
	 go to 810
 805	 write(6,71)
 71	 format(t47,'Ona T bifasica +-')
	 go to 810
 806	 write(6,72)
 72	 format(t47,'La ona T no existeix')
 
 810     continue



	 if (morf(1,i).eq.3) then
		write(6,740) 'primer bateg'
 740		format(5x,'Amplada interval RR = ',a12)
	    else    
       		write(6,750) nint(rrint(i))
 750	 	format(5x,'Amplada interval RR =',i5,' ms')
 	 end if    
	 write(6,840) nint(qrsint(i))
 840	 format(5x,'Amplada interval QRS=',i5,' ms')
	 if (morf(4,i).eq.6) then
		write(6,850) noe, noe
 850		format(5x,'Amplada interval QT =',a12,
     &  t47,'Interval QTC =',a12)
		write(6,950) noe, noe
 950		format(5x,'Amplada interval QTP=',a12,
     &  t47,'Interval QTPC =',a12)
	    else    
		if (qtcint(i).eq.0) then
       		write(6,951) nint(qtint(i))
 951	 	format(5x,'Amplada interval QT =',i5,' ms',
     &  t47,'QTC = interval rr fora de limits')
		write(6,961) nint(qtpint(i))
 961		format(5x,'Amplada interval QTP=',i5,' ms',
     &  t47,'QTPC = interval rr fora de limits')
		else
       		write(6,981) nint(qtint(i)),nint(qtcint(i))          
 981	 	format(5x,'Amplada interval QT =',i5,' ms',
     &  t47,'Interval QTC =',i5)
		write(6,991) nint(qtpint(i)),nint(qtpcint(i))     
 991		format(5x,'Amplada interval QTP=',i5,' ms',
     &  t47,'Interval QTPC =',i5)
		end if
 	 end if                       
	 if (morf(3,i).eq.2) then
		write(6,105) noe
 105		format(5x,'Amplada interval PR =',a12)
		write(6,110) noe
 110		format(5x,'Amplada interval PP =',a12)
	    else    
       		write(6,115) nint(print(i))
 115	 	format(5x,'Amplada interval PR =',i5,' ms')
		if (morf(1,i).eq.3) then
		  write(6,116) 'primer bateg'
 116		  format(5x,'Amplada interval PP = ',a12)
	         else    
		  if (morf(3,i-1).eq.2) then
			write(6,117) noe
 117			format(5x,'Amplada interval PP =',a12)
	  	    else
           		write(6,120) nint(ppint(i))
 120		        format(5x,'Amplada interval PP =',i5,' ms')
		  end if
		end if
 	 end if    
	 write(6,*)
	 write(6,123) ampp(i), nint(durp(i))
 123	 format(5x,'Amplitut de la ona P =',f8.3,' mV',
     &          t47,'Duracio de la ona P =',i4,' ms')
	 write(6,125) ampr(i), nint(durr(i))
 125	 format(5x,'Amplitut de la ona R =',f8.3,' mV',
     &          t47,'Duracio de la ona R =',i4,' ms')
	 write(6,130) ampq(i), nint(durq(i))
 130	 format(5x,'Amplitut de la ona Q =',f8.3,' mV',
     &          t47,'Duracio de la ona Q =',i4,' ms')
	 write(6,135) amps(i),nint(durs(i))
 135	 format(5x,'Amplitut de la ona S =',f8.3,' mV',
     &          t47,'Duracio de la ona S =',i4,' ms')
	 write(6,137) ampt(i), nint(durt(i))
 137	 format(5x,'Amplitut de la ona T =',f8.3,' mV',
     &          t47,'Duracio de la ona T =',i4,' ms')
	 if (morf(2,i).eq.6) then
	 write(6,140) amprr(i), nint(durrr(i))
 140	 format(5x,'Amplitut de la ona R',1h','=',f8.3,' mV',
     &          t47,'Duracio de la ona R',1h','=',i4,' ms')
	 end if
	 write(6,148) nint(pendr(i)),nint(durpic(i))
 148	 format(5x,'Pendent de la ona R  =',i8,' mV',1h/,'seg',
     &          t47,'Duracio pic de ona R=',i4,' ms',/)
 290	 write(6,300)
 300	 format(10x,/,'$Pitja <ret>per continuar  <Q> per sortir: ')
	 read(5,310,err=290) op
 310	 format(a)
	 if(op.eq.'q'.or.op.eq.'Q') return
	 end do
	 return
	 end

c-------------------------------------------------------------------------------


	 subroutine capcalera (f, ifm, ns, jqrs, ritme, rrmit)

c	 ens mostra per pantalla la capgalera a cada bateg
	 
	 character*12 f

	 write (6,10) 
 10	 format (5x,'--------------------------------------------------
     &--------------------')
	 write (6,20) f, ifm
	 write (6,30) ns, jqrs
	 write (6,40) nint(ritme), nint(rrmit)
 20	 format (10x,'nom del pacient:  ',a12,
     &           t47,'frequ. de mostreig:',i5,' Hz')
 30	 format (10x,'long. de registre:',i5,' seg',
     &           t47,'bategs detectats:  ',i5)
 40 	 format (10x,'ritme cardiac:    ',i5,' puls min',
     &           t47,'interval RR mitg:  ',i5,' ms')
	 write (6,10)
	 return 
	 end
c-------------------------------------------------------------------------
	 subroutine imp_capc(f, ifm, ns, jqrs, ritme, rrmit)

c	 ens imprimeig la capgalera al inici del llistat
	 
	 character*12 f

	 write (31,10) 
 10	 format (5x,'--------------------------------------------------
     &-------------------------')
	 write (31,20) f, ifm
	 write (31,30) ns, jqrs
	 write (31,40) nint(ritme), nint(rrmit)
 20	 format (10x,'nom del pacient:  ',a12,
     &           t47,'frequ. de mostreig:',i5,' Hz')
 30	 format (10x,'long. de registre:',i5,' seg',
     &           t47,'bategs detectats:  ',i5)
 40 	 format (10x,'ritme cardiac:    ',i5,' puls min',
     &           t47,'interval RR mitg:  ',i5,' ms')
	 write (31,10)
	 return 
	 end

c-------------------------------------------------------------------------
 	 subroutine baseline(jqrs,ippos,ecg,basel,samp,ipbeg,ipend,     
     &                       iqbeg,isend,i_base)
c-------------------------------------------------------------------------

 	 dimension basel(8000),ipend(8000),iqbeg(8000),ippos(8000)
	 dimension ecg(100000),i_base(8000),isend(8000),ipbeg(8000)

c	 busquem la linea de base com la mitjana dels punts del segment PR

	 nqui=nint(15./samp)
	 ntre=nint(30./samp)
         ntre_q = nint(10./samp)
         nqui_q = nint(5./samp)


	 if (iqbeg(1).eq.0) then
		inic=2
	    else
		inic=1
	 end if

	 do i=inic,jqrs
	 if (ippos(i).ne.0) then   
	 	sum=0.
		if (iqbeg(i)-ipend(i).gt.33/samp) then 
			do k=ipend(i)+nqui, iqbeg(i)-nqui
			   sum=sum+ecg(k)
			end do
			baselin=sum/(iqbeg(i)-nqui-nqui-ipend(i)+1)
                        pos_base=(iqbeg(i)+ipend(i))/2.

                else if (iqbeg(i)-ipend(i).le.33/samp) then 
c               else if (iqbeg(i).eq.ipend(i).gt.25) then

		       if(iqbeg(i).eq.ipend(i)) then
			  baselin=ecg(iqbeg(i))
                          pos_base=iqbeg(i)
		       else
			  do k=ipend(i), iqbeg(i)
			   sum =sum+ecg(k)
			  end do
			  baselin= sum/(iqbeg(i)-ipend(i)+1)
                          pos_base=(iqbeg(i)+ipend(i))/2.
		       end if
c                else 
c                       continue
c			do k=ipbeg(i)-33/samp-nqui,ipbeg(i)-nqui
c			   sum=sum+ecg(k)
c			end do
c			baselin=sum/(33/samp)
c                        pos_base=(ipbeg(i)+(17/samp))
c
		end if
	 else
	     if (iqbeg(i).gt.0) then
		sum=0.
	 	if(iqbeg(i)-nqui_q.gt.1) then
		  k=iqbeg(i)-nqui_q
		  do while(k.ge.iqbeg(i)-ntre_q-nqui_q.and.k.gt.0)
			sum=sum+ecg(k)
			k=k-1
		  end do
		  if(k.eq.0)then
			baselin=sum/(iqbeg(i)-nqui_q)
                        pos_base=(iqbeg(i)-nqui_q)/2.
		    else
			baselin=sum/(ntre_q+1)
                        pos_base=iqbeg(i)-nqui_q-ntre_q/2.
		  end if
		else
		  do k=1,iqbeg(i)
			sum=sum+ecg(k)
		  end do
		  baselin=sum/iqbeg(i)
                  pos_base=iqbeg(i)/2.
		end if
	     else
		baselin=0
	     end if
	 end if
	 basel(i)=baselin
         i_base(i)=int(pos_base+0.5)
	 end do

c        Si iqbeg(1) est` a la primera mostra, senyal de que no s'ha pogut
c        detectar bi; llavors fem basel(1)= al nivell de isend(1), per evitar
c        que la pimera mostra sigui alterada
	 if (iqbeg(1).le.1) basel(1)=ecg(isend(1))  
	 if (iqbeg(1).le.1) i_base(1)=isend(1)
 	 return 
	 end

c--------------------------------------------------------------------------

	 subroutine impr_dat(f,ifm,ns,jqrs,morf,amprr,ampr,ampq,amps,
     &	       ampp,ampt,durpic,qrsint,durr,durrr,durq,durs,durp,durt, 
     &	       rrint,print,ppint,qtint,qtpint,samp,ritme,rrmit,tip, 
     &         irpos,pendr,k,kder,ST_am,ST_pe,ST_in,ST_ar,freV,desV)
c 	 Aquesta subrutina ens treu totes les dades en un fitxer .DAT per a 
c	 ser impres per la printronix, o be per a ser llegit desde un altre 
c	 programa

 	 dimension morf(5,8000),pendr(8000),durp(8000),durt(8000)
	 dimension amprr(8000),ampr(8000),ampq(8000),amps(8000)
	 dimension durpic(8000),qrsint(8000),durr(8000),durrr(8000)
	 dimension durq(8000)
	 dimension durs(8000),rrint(8000),print(8000),ppint(8000)
	 dimension qtint(8000)
	 dimension qtpint(8000),irpos(8000),ampp(8000),ampt(8000)
         dimension ST_am(8000),ST_pe(8000),ST_in(8000),ST_ar(8000)
         dimension freV(8000),desV(8000)

	 character*12 f, noe, fd
	 character*4 tip(8000)
	 character*3 fderiv
	 character*1 op

c	 Obrim fitxer de sortida. Si hem arribat aqui per l'opcis 18 de
c        tractament multiderivacional nomis s'ha d'obrir el primer cop per la
c        primera derivacis.
         if (k.eq.18) then
            fd=f
            fderiv=fd(9:11)
            f=fd(1:7)
         end if
         if (k.ne.18) then
	    open(unit=30, file=f(1:lnblnk(f))//'.dad')

	 write(30,10) f, ns, nint(ritme), nint(rrmit), ifm, jqrs
 10    format(//,1x,'Pacient:',a11,2x,'longitut:',i4,' seg',12x,'ritme:'
     &   ,i3,' pul',1h/,'min',4x,'RR mitg:',i4,' ms',3x,'freq mos:',i4,
     &   'Hz',3x,'batecs:',i4,/)

	 write(30,12)
 12	 format(1x,'CODI DE MORFOLOGIES: ona P: 0=normal, 1=invertida,
     & 2=no existeix;     batec: 0=normal, 3=primer batec',/,
     &1x,'ona T: 0=normal, 1=invertida, 2=nomes pujada, 3=nomes baixada,,
     & 4=bifasica -+, 5=bifasica +-, 6=no existeix',/)

         else
          if (kder.eq.1) then
	    open(unit=30, file=f(1:lnblnk(f))//'.dad')

	 write(30,15) f, ns, ifm, jqrs
 15 	 format(//,1x,'Pacient:',a11,2x,'longitut:',i4,' seg',2x,
     &          3x,'freq mos:',i4,'Hz',
     &          3x,'batecs:',i4,/)

	 write(30,16)
 16	 format(1x,'CODI DE MORFOLOGIES: ona P: 0=normal, 1=invertida,
     & 2=no existeix;     batec: 0=normal, 3=primer batec',/,
     &1x,'ona T: 0=normal, 1=invertida, 2=nomes pujada, 3=nomes baixada,,
     & 4=bifasica -+, 5=bifasica +-, 6=no existeix',/)
          end if
         end if

         if (k.eq.18) then
            write(30,17) fderiv,nint(ritme),nint(rrmit)
 17         format(//1x,'Derivacio: ',a3,10x,'ritme:',i3,' pul',1h/,
     &             'min',10x,'RR mitg:',i4,' ms'/)
         end if

	 write(30,20)
 20	 format(/18x,'I N T E R V A L S  (ms)',15x,'A M P L I T U T S  (mv)',
     &          17x,'D U R A C I O N S    (ms)')
	 write(30,25)
 25      format(15x,28('-'),4x,40('-'),5x,32('-'))
	 write(30,30)
 30	 format(' Batec  seg.    RR  QRS   QT  QTP   PR   PP      P     ',
     &   'Q      R      S      T      R',1H','       P    Q    R    S', 
     &   '    T    R',1H',' PicR' /)
	 iau=0
	 do i=1,jqrs
         if ((tip(i-1).eq.' VT '.and.tip(i+1).eq.' VT ').or.(tip(i-1)
     &        .eq.' VF '.and.tip(i+1).eq.' VF ')) go to 45
         iau=iau+1
	 seg=1.*irpos(i)/ifm
	 write(30,40) i,seg,nint(rrint(i)),nint(qrsint(i)),nint(qtint(i)),
     &   nint(qtpint(i)),nint(print(i)),nint(ppint(i)),ampp(i),ampq(i),
     &   ampr(i),amps(i),ampt(i),amprr(i),nint(durp(i)),nint(durq(i)),
     &   nint(durr(i)),nint(durs(i)),nint(durt(i)),nint(durrr(i)),
     &	 nint(durpic(i))

 40	 format(1x,i3,3x,f5.2,1x,6(1x,i4),
     &          4x,6(f6.3,1x),1x,6(i4,1x),1x,i3)

         if(iau.eq.5) then
	    iau=0
	    write(30,*)
	 end if
 45      continue
         end do

   	 write(30,50)
 50      format (///,14x,'PEND-R    MORFOLOGIES         S E G M E N T  '
     &           ,' ST            ARRITMIES')
	 write(30,55)
 55      format(14x,6('-'),2x,13('-'),3x,32('-'),2x,15('-'))
	 write(30,60)
 60      format(' Batec  seg.  mV/s      QRS P T Bat   ampl.    pend.',
     &        '    index     area     ventricular'/37x,
     &        ' (mV)   (mV/seg)          (uV*seg)  freq(Hz) sd(mV)')
   	 iau=0
         do i=1,jqrs
          if ((tip(i-1).eq.' VT '.and.tip(i+1).eq.' VT ').or.(tip(i-1)
     &        .eq.' VF '.and.tip(i+1).eq.' VF ')) go to 75
          iau=iau+1
   	    seg=1.*irpos(i)/ifm
	    write(30,70) i,seg,nint(pendr(i)),tip(i),morf(2,i),morf(3,i),
     &          morf(4,i),morf(1,i),ST_am(i),ST_pe(i),ST_in(i),ST_ar(i),    
     &          freV(i),desV(i)
 70         format (1x,i3,3x,f5.2,2x,i3,4x,a4,i2,1x,i1,1x,i1,1x,i1,3x,
     &              f7.3,2x,f7.3,2x,f7.3,2x,f7.2,3x,f5.2,2x,f7.4)
            if(iau.eq.5) then
      	      iau=0
	      write(30,*)
	    end if
 75      continue
	 end do

         if (k.ne.18.or.kder.eq.15) then
      	    close(unit=30)
         end if
	 return
	 end

c---------------------------------------------------------------------------

	 subroutine vis_dat(f, ifm, ns, jqrs, morf, amprr, ampr, ampq, amps,
     &		durpic, qrsint, durr, durrr, durq, durs, rrint, print, 
     &		ppint, qtint, qtpint, samp, ritme, rrmit, tip,irpos,pendr) 

c 	 Aquesta subrutina ens mostra totes les dades per pantalla 

 	 dimension morf(5,8000),pendr(8000)
	 dimension amprr(8000),ampr(8000),ampq(8000),amps(8000)
	 dimension durpic(8000),qrsint(8000),durr(8000),durrr(8000)
	 dimension durq(8000)
	 dimension durs(8000),rrint(8000),print(8000),ppint(8000)
	 dimension qtint(8000)
	 dimension qtpint(8000), irpos(8000)
	 character*12 f, noe
	 character*4 tip(8000)
	 character*1 op
      character*2 FRMT

	 write(6,5)
 5	 format(5x,'Canviar el format a 132 columnes i pitjar <ret>')
      FRMT = 'i1'
      read(5,FRMT,err=6) ret

 6	 write(6,10) f, ns, nint(ritme), nint(rrmit), ifm, jqrs
 10 	 format(//,1x,'Pacient:',a8,2x,'longitut:',i4,' seg',2x,'ritme:',i3,
     &   ' pul',1h/,'min',2x,'RR mitg:',i4,' ms',3x,'freq mos:',i4,'Hz',
     &   3x,'bategs:',i4,/)
	 write(6,15)
 15	 format(1x,'CODI DE MORFOLOGIES: ona P: 0=normal, 1=invertida,
     & 2=no existeix;     bateg: 0=normal, 3=primer bateg',/,
     &1x,'ona T: 0=normal, 1=invertida, 2=nomes pujada, 3=nomes baixada,,
     & 4=bifasica -+, 5=bifasica +-, 6=no existeix',/)
	 write(6,20)
 20	 format(19x,'I N T E R V A L S  (ms)',13x,'A M P L I T U T S  (mV)',
     &   7x,'D U R A C I O N S  (ms)',2x,'PEND-R',2x,'MORFOLOGIES')
	 write(6,30)
 30	 format(' bateg seg.    RR    QRS   QT    QTP   PR    PP        Q',
     &   '      R      S      R',1H','       Q    R    S    R',1H',
     &   '  PicR  mV',1h/,'seg    QRS P T Bat')
	 iau=0
	 do i=1,jqrs
         iau=iau+1
	 seg=1.*irpos(i)/ifm
	 write(6,40) i,seg,nint(rrint(i)),nint(qrsint(i)),nint(qtint(i)),
     &   nint(qtpint(i)),nint(print(i)),nint(ppint(i)),ampq(i),ampr(i),
     &   amps(i),amprr(i),nint(durq(i)),nint(durr(i)),nint(durs(i)),
     &   nint(durrr(i)),nint(durpic(i)),nint(pendr(i)),tip(i),
     &	 morf(2,i),morf(3,i),morf(4,i),morf(1,i)

 40	 format(1x,i3,1x,f6.2,2x,i5,1x,i5,1x,i5,1x,i5,1x,i5,1x,i5,
     &   4x,f6.3,1x,f6.3,1x,f6.3,1x,f6.3,4x,i4,1x,i4,1x,i4,1x,i4,1x,i4,
     &   3x,i4,3x,a4,i2,1x,i1,1x,i1,1x,i1)
         if(iau.eq.5) then
	 iau=0
	 write(6,*)
	 end if
	 end do

	 write(6,50)
 50	 format(5x,'Canviar el format a 80 columnes i pitjar <ret>')
      read(5,FRMT,err=60) ret
 60	 return
	 end
c-----------------------------------------------------------------------
