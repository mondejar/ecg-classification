C  Adaptacion para usar con "wave" y "command line arguments"
C  
C    Pablo Laguna    13 febrero 1994
c
c        ======================================================
C          ECGMAIN (DETECCION QRS)
C          AUTOR: PABLO LAGUNA/ RAIMON JANE/ EUDALD BOGATELL 
C          DATA: 22-MAIG-87
C          Adaptacio: 19-novembre-87 (Ampliacio ona P: 2-2-88)
C                               (Finestres filtres PB:11-3-88)
C                     (Mateixa normalitzacio continua:10-4-89)
C            (ventanas P-QRS-T, normalizacion en todo:15-6-89)
C	   Adaptacio: deteccio punts sig. (Eudald Bogatell 1-6-91)
C	   Adaptacio: David Vigo Anglada. (9-1992)
C                     - tractament multiderivacional
C                     - lmnia de base 
C                     - segment ST
C          Modified: George B. Moody (6-Feb-2002) [1.0]
C                     - now compatible with WFDB library and wrappers
C                     - revised help text
C          Modified: George B. Moody (24-Feb-2006) [1.1]
C                     - fixed warning messages
C          Modified: George B. Moody (19 June 2006) [1.2]
C                     - increased size of 'arg' (command-line argument string)
C          Modified: Joe Ho (23 June 2008) [1.3]
C                     - can now specify signal numbers up to 99 using -s
C
C=====================================================================

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

C        ======================================================
C
C        ECG    Tiene la seqal original digitalizada
C        ECGPA  Tiene la seqal filtrada paso ALTO
C        ECGPB  Tiene la seqal filtrada paso bajo + paso alto
C        ECGDER Tiene la seqal ecgpb derivada
C        ECGQ   Es la seqal ECGDER elevada al cuadrado
C        ECGMW  Tiene la seqal ECGQ convolucionada con un escalon cuadrado
C        EMWDER Tiene la seqal ECGMW derivada
C        ELBA   Tiene la lmnea de base. 
C        ECGLB  Tiene el ecg corregido con la lmnea de base (ecg-elba)
C        DERFI  Tiene la derivada de 
C        DERI   Tiene la derivada de

 	 implicit real (a-h,o-z)

         dimension ecg(100000),ecgpa(100000),ecgpb(100000)
         dimension ecgder(100000)
         dimension ecgq(100000),ecgmw(100000),emwder(100000)
         dimension derfi(100000)
         dimension elba(100000),ecglb(100000),i_base(8000),qqrs(15,50)
         dimension iqrspa(8000),iqrsmw(8000),iqrs(8000),iqrst(8000)
         dimension iqt(8000)
         dimension iondaq(8000),iondat(8000),iqtc(8000),deri(100000)
	 dimension iqpos(8000),ispos(8000),isend(8000),itbeg(8000)
	 dimension itpos(8000)
         dimension ipbeg(8000),ippos1(8000),irrpos(8000)
         dimension ecgaux(100000)
	 dimension itend(8000),ipend(8000),iqbeg(8000),irpos(8000)
	 dimension iqend(8000),isbeg(8000),qrsdef(15,50),cont(15)
	 dimension ilead(4)
         dimension ecg1(100000),ecg2(100000),ecg3(100000),ecg4(100000)

	 logical fin
         character*3 fderiv
         character*1 espera
	 character*12 fd, faux, f1, f2, f3, an, anin, from, too, aux, bt   
         character*32 arg, f, fname, desc, units
         integer initval, group, fmt, spf, bsize, baseline, cksum
         integer putann, getann, wfdbquit, strtim, ij
 	 double precision gain, fm
	 double precision  sampfreq 
	 integer v(32), getvec, info, isigopen, osigfopen1, newheader
	 integer wfdbquiet, wfdbverbose, isqrs


 	 dimension morf(5,8000),buf5(76500),ampt(8000),durp(8000)
 	 dimension durt(8000)
	 dimension amprr(8000),ampr(8000),ampq(8000),amps(8000)
	 dimension ampp(8000)
	 dimension durpic(8000),qrsint(8000),durr(8000),durrr(8000)
	 dimension durq(8000)
	 dimension durs(8000),rrint(8000),print(8000),ppint(8000)
	 dimension qtint(8000)
	 dimension qtpint(8000),basel(8000),pendr(8000),itpos2(8000)
	 dimension idis(3,20), freV(8000), desV(8000)
	 dimension qtpcint(8000), qtcint(8000)
         dimension ipbeg_mean(30),ipend_mean(30),iqbeg_mean(30)
         dimension isend_mean(30),itend_mean(30)
         dimension ipbeg_lead(15,8000),ipend_lead(15,8000)
         dimension iqbeg_lead(15,8000),isend_lead(15,8000)
         dimension itend_lead(15,8000)
         dimension ST_ampl(8000),ST_pend(8000),ST_index(8000)
         dimension ST_area(8000)

	 character*4 tip(8000)
c        integer*2 aux
         integer*2 buf5,qqrs,qrsdef
         integer nlead,cont,esQRS,i_0,i_1
c 	 nqrs: nombre de qrs detectats i confirmats que tractarem
c 	 samp: interval de mostreig en ms
c	 iqrs: posicio dels complexes QRS detectats i confirmats
c	 irpos: vector que conti la posicio de l'ona R
c 	 ipbeg, ippos1, ipend: inici, pic i final de l'ona P
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
                                   
c           muestras anteriores (imu_an) igual a cero
 
c........................................................................
             imu_an=0
  	     imu_fi=0
	     ic=0
	     flag_qrs=0
	     flag_N=0 
	     nargs = iargc()
	     if (nargs.lt.4) then
 1    write(6,*) 'usage: ecgpuwave -r RECORD -a ANNOTATOR [OPTIONS ...]'
      write(6,*) 'where RECORD specifies the input, ANNOTATOR is the'
      write(6,*) 'output annotator name, and OPTIONS may include:'
      write(6,*) ' -f TIME    start at specified TIME'
      write(6,*) ' -i IANN    read QRS detections from IANN'
      write(6,*) ' -n PFLAG   specify beats to process (requires -i):'
      write(6,*) '             all (PFLAG=0; default)'
      write(6,*) '             normal beats only (PFLAG=1)'
      write(6,*) ' -s SIGNAL  analyze the specified SIGNAL (0-99):'
      write(6,*) '             default: SIGNAL= 0'
      write(6,*) ' -t TIME    stop at specified TIME'
	      return
	     end if
	     from='0'
	     too='0'
	     do 10 i=1, nargs/2
	       call getarg(2*i-1,arg)
	       if (arg.eq.'-r') then
	           call getarg(2*i,arg)
	          f=arg
	          if_r=1
	       else if 	(arg.eq.'-a') then
	           call getarg(2*i,an)
	           if_a=1
 	       else if 	(arg.eq.'-s') then
	          call getarg(2*i,arg)
	          ic=iachar(arg(1:1))-48
                  if (arg(2:2).ne.'') then
                     ic=ic*10+(iachar(arg(2:2))-48)
                  end if
	       else if 	(arg.eq.'-f') then
	           call getarg(2*i,from)
	       else if 	(arg.eq.'-t') then
	          call getarg(2*i,too)
	       else if 	(arg.eq.'-i') then
	          call getarg(2*i,anin)
	          flag_qrs=1
	       else if 	(arg.eq.'-n') then
		 call getarg(2*i,bt)
	         ibt=iachar(bt(1:1))-48
 	         if (ibt.eq.1) then 
                   flag_N=1 
	         end if
	       else   
                  write(6,*) 'ecgpuwave: unrecognized option ',arg
		  call help(2)
		  return
               end if
 10          continue

	  
	      if (if_r.ne.1.or.if_a.ne.1) then
		call help(3)
		return
 	      end if 


c     The '32' below is the maximum number of signals this program will
c     open.  (It will process only one of these signals, but it may be
c     necessary to open more than one signal in order to read the
c     desired signal.)  In order to read a record with a larger number
c     of signals, it may be necessary to increase this value.  If you do
c     so, increase the number of elements in v (above) and in sinfo (in
c     wfdbf.c) to match.
  	      i=isigopen(f,32)
	      i_0=0
	      i_1=1
              ii=setanninfo(i_0,an,i_1)
              ii=annopen(f,1)
	     
	      if (i.le.ic) then
 	       write(6,*) ' Signal',ic,' does not exist in the record'
	       return
	      end if 

                fm=sampfreq(f)
	        ifm=nint(fm) 
         
                i=getsiginfo(ic,fname,desc,units,gain,initval,group,fmt
     &               ,spf,bsize,adcres,ioffset,baseline,nsamp,cksum)
		if (gain.gt.0) then 
			igain=nint(gain)
		  else
		        igain=200
		end if

c            Becouse ST-T database has offset=o but is not true
		if (ioffset.eq.0) then 
		   do i=1,2*ifm
		     ig=getvec(v)
		     ioffset=ioffset+v(ic+1)
		    end do
                    ioffset=nint(ioffset/(2.*ifm)) 
	  	end if

	        imu_an=strtim(from)
                imu_fi=strtim(too)
	        if (imu_fi.eq.0) imu_fi=nsamp
	        if (imu_fi.le.imu_an) return

	      
	   
              if (flag_qrs.eq.1 ) then
                ii=setanninfo(0,anin,0)
                ii=annopen('+'//f,1)
c               ii=annopen1('+'//f,anin,0)
	       ij=getann(0,itime,i,ii,iii,iiii,aux)
	       if (itime.gt.imu_an+ifm) imu_an=itime-ifm
	      end if	            
  

	        rmax=1
	      	index=0
	        ilastqrs=0      
c  		write(6,*)  'ioffset', ioffset
c
c
c               used to write signals from the processing
c	       i=osigfopen1('pb',ic)
	     
               fin=.false.
               do while (fin.eqv..false.)

c                                             READ SIGNALS		 
	          i=isigsettime(imu_an+1)
	          is=0
	          if (imu_fi-imu_an.gt.100000-2*ifm) then
		      ns = 100000/ifm
	          else
	             ns = (imu_fi-imu_an+2*ifm)/ifm
	          end if
                  do i=1,100000
		    ig=getvec(v)
   		    if (ig.eq.-1.or.imu_an+i.ge.imu_fi+2*ifm) then
		     fin=.true.
	             ecg(i)=0. 
		    else 
		      ecg(i)=1.0*(v(ic+1)-ioffset)/igain
		    end if
	          end do

c				         INITIALICE AND SIGNAL PROCESSING                                                                            
                call inicializa_cero(iondat,iqtc,iqrspa,iqrsmw,iqrs,
     &                          iondaq,iqt)
               	call procesar(index,de_rmax,pa_rmax,pb_rmax,depb_rmax,
     &                             rq_rmax,rmw_rmax,rmd_rmax,
     &                   ifm,is,ns,ecg,ecgpa,ecgpb,ecgder,
     &                   ecgq,ecgmw,emwder,f,iret_pb,iret_pa,deri)
	
	  
 	        
c	         write(6,*) 'pa_rmax',pa_rmax,'pb_rmax',pb_rmax,'index',index

   		 	        

C					QRS DETECTION OR READING
            	if (flag_qrs.eq.0) then
       	         call detectar(index,ifm,is,ns,ecgpb,ecgder,ecgmw,emwder,iqrspa,
     *                   iqrsmw,iqrs,nqrs,f,rmax,pa_rmax,pb_rmax,
     *                   imu_an,ecg)
                 do j=1,nqrs
		   iqrst(j)=1        
c			los llamo a todos los latidos detectados normales
                 end do 
                else 
	           itime=imu_an
	           ii=iannsettime(imu_an)
	           n=1
	           ij=0
                   do while (itime.lt.imu_an+100000-2*ifm
     &                       .and.itime.lt.imu_fi)
  	            ij=getann(0,itime,i,ii,iii,iiii,aux)
                    esQRS=isqrs(i)
                    
	            if (ij.ne.0) then 
		     nqrs=n-1
	             fin=.true.
		     go to 20
	            end if
		    if (((flag_N.eq.0).and.(esQRS.eq.1)).or.((flag_N.eq.1).and.(i.eq.1))) then 
	             ibe=itime-imu_an-nint(0.2*ifm)
		     ien=itime-imu_an+nint(0.17*ifm)
	             call buscamaxmin(ibe,ien,ecgpa,imi,ymin,ima,ymax)

c                 write(6,*) 'flag_N ', flag_N, 'itime ', itime, ' i ', i
c	         write(6,*) 'itime', itime, 'imu_an', imu_an, ' imi',imi,' ymin',ymin,' ima',ima,' yma',ymax


                     if (abs(ymin).gt.abs(ymax)) then
			iqrs(n)=imi
		      else
			iqrs(n)=ima
                     end if
                     iqrst(n)=i
		     n=n+1
	            end if
 		   end do
	           nqrs=n-1
	        end if
 20             continue
	        index=1
        
c					WAVEFORM ONSET AND END DETERMINATION
		call p_significatius (ecgder,nqrs,iqrs,ifm,is,ns,f,ipbeg, 
     &		ippos1, ipend, iqbeg, iqpos, ispos, isend, itbeg, 
     &		itpos, itend, irpos, ecgpb, derfi, irrpos, iqend, isbeg,
     &		rrmed, ecg, deri, itpos2, morf, idis, nep) 

 	             	   
c	         do i=1, 100000
c	           v(1)=nint(200*ecgpb(i)/10)
c	           ig=putvec(v)
c                end do
        
  
c	   write(6,*) ilastqrs, iqrs(1)+imu_an,iqrs(2)+imu_an,iqrs(3)+imu_an  

	 
                if (iqrs(1)+imu_an.gt.ilastqrs+0.4*ifm) then
	            iprimerqrs=1
		else if (iqrs(2)+imu_an.gt.ilastqrs+0.4*ifm) then
		    iprimerqrs=2
	        else if (iqrs(2)+imu_an.le.ilastqrs+0.4*ifm) then
		    iprimerqrs=3
                end if

		ii=wfdbquiet(1)

               do ie=iprimerqrs,nqrs-1
               if (ipbeg(ie).gt.0) 
     &              ii=putann(0,ipbeg(ie)+imu_an,39,0,ic,0,' ')    
               if (ippos1(ie).gt.0) 
     &             ii=putann(0,ippos1(ie)+imu_an,24,0,ic,0,' ')    
               if (ipend(ie).gt.0) 
     &             ii=putann(0,ipend(ie)+imu_an,40,0,ic,0,' ')    
               if (iqbeg(ie).gt.0) 
     &                ii=putann(0,iqbeg(ie)+imu_an,39,0,ic,1,' ')    
               if (iqrs(ie).gt.0) 
     &              ii=putann(0,iqrs(ie)+imu_an,iqrst(ie),0,ic,0,' ')    
               if (isend(ie).gt.0) 
     &              ii=putann(0,isend(ie)+imu_an,40,0,ic,1,' ')    
               if (itbeg(ie).gt.0) 
     &              ii=putann(0,itbeg(ie)+imu_an,39,0,ic,2,' ')    
               if (itpos2(ie).gt.0) 
     &          ii=putann(0,itpos2(ie)+imu_an,27,0,ic,morf(4,ie),' ')    
               if (itpos(ie).gt.0.and.itpos(ie).ne.itpos2(ie)) 
     &          ii=putann(0,itpos(ie)+imu_an,27,0,ic,morf(4,ie),' ')    
               if (itend(ie).gt.0) 
     &              ii=putann(0,itend(ie)+imu_an,40,0,ic,2,' ')    
               end do
	       ii=wfdbverbose(1)
 	       ilastqrs=iqrs(nqrs-1)+imu_an
               imu_an=imu_an+100000-2*ifm 
	       write(6,*) '.. ',nqrs-iprimerqrs,' beats'
	       if (flag_qrs.eq.1.and.ij.eq.-1) go to 30
            end do

            close(unit=2) 
            close(unit=3) 
            close(unit=4) 
            close(unit=7) 
            close(unit=8) 
            close(unit=9)
c 	         i=newheader('pb')
	
 30 	    ii=wfdbquit(i)
         stop 
        end

                             
 
      subroutine help(i)

      write(6,*) 'usage: ecgpuwave -r RECORD -a ANNOTATOR [OPTIONS ...]'
      write(6,*) 'where RECORD specifies the input, ANNOTATOR is the'
      write(6,*) 'output annotator name, and OPTIONS may include:'
      write(6,*) ' -f TIME    start at specified TIME'
      write(6,*) ' -i IANN    read QRS detections from IANN'
      write(6,*) ' -n PFLAG   specify beats to process (requires -i):'
      write(6,*) '             all (PFLAG=0; default)'
      write(6,*) '             normal beats only (PFLAG=1)'
      write(6,*) ' -s SIGNAL  analyze the specified SIGNAL (default: 0)'
      write(6,*) ' -t TIME    stop at specified TIME'
      return
      end
