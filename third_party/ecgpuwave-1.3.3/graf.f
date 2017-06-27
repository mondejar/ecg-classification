 
C        ==================================================
C          GRAF (SUBROUTINAS DE REPRESENTACION POR PANTALLA 
C             AUTOR: PABLO LAGUNA
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

c        SE LE LLAMA DESDE ECGMAIN PARA REPRESENTAR EN PANTALLA
C



C        ----------------------------------------------------------
C            REPRESENTACION POR PANTALLA
C        ----------------------------------------------------------
  


C        ----------------------------------------------------------
C            PONE LA PANTALLA EN MODO GRAFICO
C        ----------------------------------------------------------

         subroutine inicializa

C        PONE LA PANTALLA EN MODO REGIS, para representar por ella

         write(6,5) char(27)
 5       format(1x,a1,'P1p')

c        pONE LOS COLORES DE LAS PAGINAS
         write(6,10)
 10      format(1X,'s(m0(ah0l25s25))')
         write(6,15)
 15      format(1X,'s(m3(ah240l50s330))')
         write(6,20)
 20      format(1X,'s(m2(ah0l35s60))')
         write(6,25)
 25      format(1X,'s(m1(ah120l50s100))')   

C        lIMPIA LA PANTALLA      
         write(6,30)
 30      format(1X,'(e)')

         return
         end



C        ----------------------------------------------------------
C            DIBUJA LOS EJES
C        ----------------------------------------------------------

         subroutine dibujaejes
 
C        DIBUJA LOS EJES

C        LLEVA EL CURSOR AL ORIGEN
         WRITE(6,4)
 4       format(1X,'w(i2)')
         write(6,5)
 5       format(1X,'p[0,499]')
      

         write(6,10)
 10      format(1X,'v')
         write(6,15)
 15      format(1X,'[,-499]')
         write(6,20)
 20      format(1X,'[+799]')
         write(6,25)
 25      format(1X,'(e)')
         return
         end 


C        ----------------------------------------------------------
C            DIBUJA LA SEQAL
C        ----------------------------------------------------------
     
         subroutine dibuja(ifm,sen,is,ns,iy,tsen,fesc,ipbeg,ippos,ipend,
     &		iqbeg,iqpos,iqend,irpos,isbeg,ispos,irrpos,isend,itbeg,
     &		itpos,itend,nqrs,basel,itpos2)

         dimension ipbeg(8000), ippos(8000), ipend(8000),irpos(8000)
	 dimension iqbeg(8000), basel(8000), itpos(8000)
	 dimension iqpos(8000), ispos(8000), isend(8000), itbeg(8000)
	 dimension itend(8000), irrpos(8000), iqend(8000), isbeg(8000)
         dimension itpos2(8000)

c        DIBUJA LA SEQAL QUE ESTA EN sen DESDE EL SEGUNDO is HASTA is+ns
c        A UNA ALTURA DE PANTALLA iy, CON NOMBRE tsen Y UN FACTOR DE ESCALA
c        fesc
c         
         dimension sen(100000)
         character*5 tsen         
	 character*1 op
	 logical nofi
         
         iss=is*ifm
         ins=ns*ifm
cc         call cal_esc(ins,800, l, ns, 33., fesc)
         call cal_esc(ins,800, l, ns, 33., fesch)

C        IL ESPACIADO DE CADA SEGUNDO    

	 fseg=1./0.4*fesch*33
cc	 fseg=1./0.4*fesc*33
	 iaux=0

C        DIBUJA LA SEQAL

c	 inicialitzem variables
	 j=1
	 do while(irpos(j).le.iss.and.j.lt.nqrs)
	    j=j+1
	 end do
	 ncon=1
c	 if(ins.ge.intt) then
		ja=iss
c	    else
c		ja=nint(1.*iss/l)
c	 end if
	 k=0
	 iseg=0
	 op=' '
	 kau=1

	 do while(k.lt.ins.and.op.eq.' ')
	 call inicializa
	 write(6,100)
 100	 format(1x,'s(e)')

C        DIBUJA EL NIVEL CERO EN Y=IY
         write(6,5)
 5       format(1X,'w(i3)')
         write(6,10) iy
 10      format(1X,'p[799,',i3,']')
         write(6,15)
 15      format(1X,'v')
         write(6,20) 
 20      format(1X,'[-799]')

c        dIBUJA LA SEQAL,DEPENDIENDO DE QUE ESTA TENGA MAS MUESTRAS QUE LA
C        PANTALLA O AL REVES.

         write(6,25) iy-nint(fesc*33*sen(iss+l+k))
 25      format(1X,'w(i3)p[,',i6,']v')

         if (ins.ge.intt) then
		if((ins-k)/l.ge.800) then
                           do i=1,800
                            write(6,30) iy-nint(fesc*33*sen(k+iss+l*i))
 30                         format(1X,'[+1,',i6,']')
                           end do
			   k=k+800*l
                         else
                           do i=1,intt-k/l
                            write(6,132) iy-nint(fesc*33*sen(k+iss+l*i))
 132                        format(1X,'[+1,',i6,']')
                           end do
			   k=intt*l
		end if
	   else
                if((ins-k)*l.ge.800) then
		            do i=1,800/l
                            write(6,34) l,iy-nint(fesc*33*sen(k+iss+i))
 34                         format(1X,'[+',i6,',',i6,']')
                           end do
			   k=k+800/l
                         else
                           do i=1,intt/l-k
                            write(6,36) l,iy-nint(fesc*33*sen(iss+i))
 36                         format(1X,'[+',i6,',',i6,']')
                           end do
	 		   k=intt/l
		end if
         end if  

         write(6,35)
 35      format(1X,'(e)')

C        ESCRIBE LA SEQAL QUE ES AL PIE DE LA GRAFICA

         write(6,31) iy+20
 31      format(1X,'p[740,',i3,']')         
         write(6,33)
 33      format(1X,'w(i1)')
         write(6,32) tsen
 32      format(1X,'t(a0,s1,i0)(d0,s1,d0)',/,1x,'t',1h',a5,1h')         

c	 escriu l'escala definitiva
         write(6,51) 
 51      format(1X,'p[670,450]')         
         write(6,52) fesc
 52      format(1X,'t(a0,s1,i0)(d0,s1,d0)',/,1x,'t',1h','Escala ',1h',/,1x,
     &   't',1h',f3.1,1h',/,1x,'t',1h',':1',1h')

C        ESCRIBE EL ESCALADO DE CADA SEGUNDO

         write(6,40) iaux,iy-5
 40      format(1x,'p[',i3,',',i3,']')                              
		i=0
         	do while (iaux.le.801)
		iaux=nint(iaux+fseg)            
         	write(6,45) is+iseg,iaux
 45      	format(1x,'v[,+10]t',1h',i2,1h','p[',i4,',-10]')        
		iseg=iseg+1
         	end do
		iaux=iaux-800

c	 dibuixa les marques sobre la senyal

	 nofi=.true.
	 do while(nofi.and.j.le.nqrs)
	    if (ins.ge.intt) then
		call dib_punts((ipbeg(j)-ja)/l,(ippos(j)-ja)/l,(ipend(j)-ja)/l,
     &	(iqbeg(j)-ja)/l,(iqpos(j)-ja)/l,(iqend(j)-ja)/l,(irpos(j)-ja)/l,
     &	(isbeg(j)-ja)/l,(ispos(j)-ja)/l,(irrpos(j)-ja)/l,(isend(j)-ja)/l,
     &	(itbeg(j)-ja)/l,(itpos(j)-ja)/l,(itpos2(j)-ja)/l,(itend(j)-ja)/l,
     &  nint(1.*k/l/kau),ncon,nofi,iy-nint(basel(j)*fesc*33),fesc)
		call dib_basel((irpos(j)-ja)/l,iy-nint(basel(j)*fesc*33),fseg)
		if(nofi) then
		   j=j+1
	  	 else
		   ja=ja+800*l
		   kau=kau+1
	 	end if
	     else
		call dib_punts((ipbeg(j)-ja)*l,(ippos(j)-ja)*l,(ipend(j)-ja)*l,
     &	(iqbeg(j)-ja)*l,(iqpos(j)-ja)*l,(iqend(j)-ja)*l,(irpos(j)-ja)*l,
     &	(isbeg(j)-ja)*l,(ispos(j)-ja)*l,(irrpos(j)-ja)*l,(isend(j)-ja)*l,
     &	(itbeg(j)-ja)*l,(itpos(j)-ja)*l,(itpos2(j)-ja)*l,(itend(j)-ja)*l,
     &	nint(1.*k*l/kau),ncon,nofi,iy-nint(basel(j)*fesc*33),fesc)
		call dib_basel((irpos(j)-ja)*l,iy-nint(basel(j)*fesc*33),fseg)
		if(nofi) then
		   j=j+1
	  	 else
		   ja=ja+800/l
		   kau=kau+1
	 	end if
	    end if
	 end do


c	 escriu les opcions
	 call salgraf
	 
 230     write(6,231)
 231     format('$',5X,'Pitja <ret> per continuar <Q> per sortir: ')         
	
c 230     write(6,231)
c 231     format(1x,'p[20,460]')         
c         write(6,233)
c 233     format(1X,'w(i1)')
c         write(6,232)
c 232     format(1X,'t(a0,s1,i0)(d0,s1,d0)',/,1x,'t',1h','Pitja <ret> per       
c     &continuar <Q> per sortir: ',1h')         
	 read(5,310,err=230) op
 310	 format(a1)

	 end do
         return
         end

c------------------------------------------------------------------------------
	 subroutine dib_punts(ipb,ipp,ipe,iqb,iqp,iqe,irp,isb,isp,irrp,
     &		ise,itb,itp,itp2,ite,kk,ncon,nofi,iybas,fesc)
c------------------------------------------------------------------------------

	 logical nofi

	 go to (10,20,30,40,50,60,70,80,90,100,110,120,130,140,150),ncon

 10	 if(ipb.le.kk) then
	 	call dib_marca(ipb,iybas+25,50)
	    else
		ncon=1
		go to 500
	 end if
 20	 if(ipp.le.kk) then
	 	call dib_marca(ipp,nint(iybas-20*fesc),10)
	    else
		ncon=2
		go to 500
	 end if
 30	 if(ipe.le.kk) then
	 	call dib_marca(ipe,iybas+25,50)
	    else
		ncon=3
		go to 500
	 end if
 40	 if(iqb.le.kk) then
	 	call dib_marca(iqb,iybas+25,50)
	    else
		ncon=4
		go to 500
	 end if
 50	 if(iqp.le.kk) then
	 	call dib_marca(iqp,nint(iybas+20*fesc),10)
	    else
		ncon=5
		go to 500
	 end if
 60	 if(iqe.le.kk) then
	 	call dib_marca(iqe,iybas+15,30)
	    else
		ncon=6
		go to 500
	 end if
 70	 if(irp.le.kk) then
	 	call dib_marca(irp,nint(iybas-25*fesc),10)
	    else
		ncon=7
		go to 500
	 end if
 80	 if(isb.le.kk) then
	 	call dib_marca(isb,iybas+15,30)
	    else
		ncon=8
		go to 500
	 end if
 90	 if(isp.le.kk) then
	 	call dib_marca(isp,nint(iybas+20*fesc),10)
	    else
		ncon=9
		go to 500
	 end if
 100	 if(irrp.le.kk) then
	 	call dib_marca(irrp,nint(iybas-25*fesc),10)
	    else
		ncon=10
		go to 500
	 end if
 110	 if(ise.le.kk) then
	 	call dib_marca(ise,iybas+25,50)
	    else
		ncon=11
		go to 500
	 end if
 120	 if(itb.le.kk) then
	 	call dib_marca(itb,iybas+25,50)
	    else
		ncon=12
		go to 500
	 end if
 130	 if(itp.le.kk) then
	 	call dib_marca(itp,nint(iybas+15*fesc),10)
	    else
		ncon=13
		go to 500
	 end if
 140	 if(itp.le.kk) then
		call dib_marca(itp2,nint(iybas-15*fesc),10)
	    else
		ncon=14
		go to 500
	 end if
 150	 if(ite.le.kk) then
	 	call dib_marca(ite,iybas+25,50)
	    else
		ncon=15
		go to 500
	 end if
	 ncon=1
	 return
 500	 nofi=.false.
	 return
	 end
c------------------------------------------------------------------------------
c------------------------------------------------------------------------------

	 subroutine dib_marca(ixpos, iybeg, long)

	 if(ixpos.le.0) return
	 write(6,50) ixpos,iybeg
 50      format(1x,'p[',i3,',',i3,']')
         write(6,55)
 55      format(1x,'w(i3)')
         write(6,60) long
 60      format(1x,'v[,-',i3,']')
         return
	 end


c------------------------------------------------------------------------------
c------------------------------------------------------------------------------
     
         subroutine dibujaold(ifm,sen,is,ns,iy,tsen,gan)

c        DIBUJA LA SEQAL QUE ESTA EN sen DESDE EL SEGUNDO is HASTA is+ns
c        A UNA ALTURA DE PANTALLA iy, CON NOMBRE tsen Y UN FACTOR DE ESCALA
c        gan
c         
         dimension sen(100000)
         character*5 tsen         
         
         iss=is*ifm
         ins=ns*ifm
         call calcula_paso(ins,800,l,npimp)

C        IL ESPACIADO DE CADA SEGUNDO    
         if (ins.ge.800) then
                           il=nint(ifm*1./l)
                         else
                           il=ifm*l
         end if

C        DIBUJA LA SEQAL

C        DIBUJA EL NIVEL CERO EN Y=IY
         write(6,5)
 5       format(1X,'w(i3)')
         write(6,10) iy
 10      format(1X,'p[799,',i3,']')
         write(6,15)
 15      format(1X,'v')
         write(6,20) 
 20      format(1X,'[-799]')

c        dIBUJA LA SEQAL,DEPENDIENDO DE QUE ESTA TENGA MAS MUESTRAS QUE LA
C        PANTALLA O AL REVES.

         write(6,25) iy-nint(gan*sen(iss+l))
 25      format(1X,'w(i3)p[,',i6,']v')

         if (ins.ge.800) then
                           do i=1,npimp
                            write(6,30) iy-nint(gan*sen(iss+l*i))
 30                         format(1X,'[+1,',i6,']')
                           end do
                         else
                           do i=1,npimp/l
                            write(6,34) l,iy-nint(gan*sen(iss+i))
 34                         format(1X,'[+',i6,',',i6,']')
                           end do
         end if  

         write(6,35)
 35      format(1X,'(e)')

C        ESCRIBE LA SEQAL QUE ES AL PIE DE LA GRAFICA

         write(6,31) iy+20
 31      format(1X,'p[740,',i3,']')         
         write(6,33)
 33      format(1X,'w(i1)')
         write(6,32) tsen
 32      format(1X,'t(a0,s1,i0)(d0,s1,d0)',/,1x,'t',1h',a5,1h')         

C        ESCRIBE EL ESCALADO DE CADA SEGUNDO

         write(6,40) iy-5
 40      format(1x,'p[0,',i3,']')
         i=0
         do while ((i*il).le.npimp+1)
         write(6,45) is+i,(i+1)*il
 45      format(1x,'v[,+10]t',1h',i2,1h','p[',i4,',-10]')        
         i=i+1
         end do
         return
         end



C        ----------------------------------------------------------
C            PINTA LOS QRS
C        ----------------------------------------------------------

         subroutine pintqrs(iqrs,ifm,is,ns,iy,il)
                         
c        dIBUJA LOS qrs SOBRE LA SEQAL  

         dimension iqrs(8000)

         iss=is*ifm
         ins=ns*ifm

         call calcula_paso(ins,800,l,npimp)

c        pinta los qrss detectados
         do i=1,8000
          if ((iqrs(i).gt.iss).and.(iqrs(i).lt.(iss+ins))) then
c          busco la posicion del qrs en la escala de la pantalla
           if (ins.ge.800) then
                            ixpos=nint(1.*(iqrs(i)-iss)/l)
                           else
                            ixpos=(iqrs(i)-iss)*l
           end if
           write(6,50) ixpos,iy
 50        format(1x,'p[',i3,',',i3,']')
           write(6,55)
 55        format(1x,'w(i3)')
           write(6,60) il
 60        format(1x,'v[,-',i3,']')
          end if
         end do
         return
         end
         
C        ----------------------------------------------------------
C            SACA LA PANTALLA DEL MODO GARFICO
C        ----------------------------------------------------------         

         subroutine salgraf

c        QUITA LA PANTALLA DEL MODO REGIS

         write(6,10)
 10      format(1x,'(w3)')
         write(6,15) char(27)
 15      format(1X,a1,'q')
         return
         end



C        -----------------------------------------------------------------
C         REPRESENTA LE INTERVALO QT EN FUNCION DEL TIEMPO
C        ----------------------------------------------------------------- 

         subroutine grafica_qt(ifm,is,ns,iqrs,iqt,iqtc)

         dimension iqrs(8000),iqt(8000),iqtc(8000)

         
         iss=is*ifm
         ins=ns*ifm
	 call calcula_paso(ins, 800, l, npimp)

C        IL ESPACIADO DE CADA SEGUNDO    
         if (ins.ge.800) then
                           il=nint(ifm*1./l)
                         else
                           il=ifm*l
         end if

         call inicializa 

C        ESCRIBE LOS EJES

       
         write(6,40) 
 40      format(1x,'p[799,440]w(i3)')
         write(6,41)
 41      format(1x,'v[0',']')
         write(6,42)
 42      format(1x,'v[,','0]')
                    
         write(6,43)
 43      format(1x,'p[799,220]v[-799]t',1h','440 MS',1h')        
                    
C        ESCRIBE EL ESCALADO DE CADA SEGUNDO

         write(6,44)
 44      format(1x,'p[0,440]')

         i=0
         do while ((i*il).le.npimp+1)
          write(6,45) is+i,(i+1)*il
 45       format(1x,'v[,+10]t',1h',i2,1h','p[',i4,',-10]')        
          i=i+1
         end do     

         write(6,46)
 46      format(1x,'p[700,400]t',1h','Seg.',1h')        
                    

         call pintaqt(ifm,iqt,iqrs,iss,ins,l)
         write(6,47)
 47      format(1x,'W(i1)')
         call pintaqt(ifm,iqtc,iqrs,iss,ins,l)
         call salgraf

         return
         end


C        ------------------------------------------------------------------
C         PINTA LOS QT DEL VECTOR IONDAT-IONDAQ
C        ----------------------------------------------------------------

         subroutine  pintaqt(ifm,iqt,iqrs,iss,ins,l)
         
         dimension iqt(8000),iqrs(8000)


         
c        ESCRIBE LA SEQAL
         write(6,46) 440-iqt(1)/2  
 46      format(1x,'p[0,',i4,']')       
         do i=1,8000
          if ((iqrs(i).gt.iss).and.(iqrs(i).lt.(iss+ins))) then
C          BUSCO LA POSICION DEL QRS EN LA ESCALA DE LA PANTALLA
           if (ins.ge.800) then
                            ixpos=nint(1.*(iqrs(i)-iss)/l)
                           else
                            ixpos=(iqrs(i)-iss)*l
           end if
           if (iqt(i).ne.0) then
                             write(6,50) ixpos,440-iqt(i)/2
 50                          format(1x,'v[',i3,',',i4,']')
           end if
          end if
         end do
         write(6,51)
 51      format(1X,'(E)')
         return
         end


C        ----------------------------------------------------------
C         CALCULA  LOS ESCALADOS ADECUADOS
C        ----------------------------------------------------------

         subroutine cal_esc(npts,nptt,npescx,ns,fac,fesc)

c        npts ES EL NUMERO DE PUNTOS QUE MANDO IMPRIMIR DE LA SENAL
c        nptt ES EL NUMERO DE PUNTOS MAXIMO QUE ADMITE EL GRAFICO
c        npescx ES EL PASO DE CADA PUNTO
c	 fac es la relacio punts per centimetre
c	 aquesta subrutina  ens troba el pas i l'escalat mes proper
c	 al que nosaltres voliem

c 	 nptt=nint(ns/0.4*fesc*fac)       (No se para que. Pablo)
         if (npts.ge.nptt) then
                            npescx = nint(npts*1./nptt)
c			    nptt=nint(1.*npts/npescx)
			    npts=nptt*npescx
			    fesc=nptt/ns*0.4/fac
                           else 
                            npescx = nint(nptt*1./npts)
c	 		    nptt=npts*npescx
 	 		    npts=nptt/npescx
 			    fesc=nptt/ns*0.4/fac
         end if
         return
         end
c---------------------------------------------------------------------------

	 subroutine dib_basel(jcen,iybase,fseg)

c	 dibuixa la linea de base agafada en cada QRS

	 if(jcen.gt.800) return
	 write(6,5)
 5       format(1x,'w(p5)')
	 if (jcen-nint(fseg*0.1).lt.0) then
	 	write(6,7) iybase
 7      	format(1x,'p[0,',i3,']')
	    else
	 	write(6,10) jcen-nint(fseg*0.1),iybase
 10      	format(1x,'p[',i3,',',i3,']')
	 end if
	 if (jcen+nint(fseg*0.1).lt.0) then
	 	write(6,15) nint(fseg*0.1)
 15	 	format(1x,'v[+',i3,']')   
	    else
	 	write(6,20) nint(fseg*0.2)
 20	 	format(1x,'v[+',i3,']')   
	 end if
	 write(6,25)
 25       format(1x,'w(p1)')
	 return
 	 end
c-----------------------------------------------------------------------------
                  

