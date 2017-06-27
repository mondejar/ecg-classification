/* file: ecgcodes.h	T. Baker and G. Moody	  June 1981
			Last revised:  29 April 1999	wfdblib 10.0.0
ECG annotation codes

_______________________________________________________________________________
wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1999 George B. Moody

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Library General Public License for more
details.

You should have received a copy of the GNU Library General Public License along
with this library; if not, write to the Free Software Foundation, Inc., 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

You may contact the author by e-mail (george@mit.edu) or postal mail
(MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
_______________________________________________________________________________

*/

#ifndef wfdb_ECGCODES_H	/* avoid multiple definitions */
#define wfdb_ECGCODES_H

#define	NOTQRS	0	/* not-QRS (not a getann/putann code) */
#define NORMAL	1	/* normal beat */
#define	LBBB	2	/* left bundle branch block beat */
#define	RBBB	3	/* right bundle branch block beat */
#define	ABERR	4	/* aberrated atrial premature beat */
#define	PVC	5	/* premature ventricular contraction */
#define	FUSION	6	/* fusion of ventricular and normal beat */
#define	NPC	7	/* nodal (junctional) premature beat */
#define	APC	8	/* atrial premature contraction */
#define	SVPB	9	/* premature or ectopic supraventricular beat */
#define	VESC	10	/* ventricular escape beat */
#define	NESC	11	/* nodal (junctional) escape beat */
#define	PACE	12	/* paced beat */
#define	UNKNOWN	13	/* unclassifiable beat */
#define	NOISE	14	/* signal quality change */
#define ARFCT	16	/* isolated QRS-like artifact */
#define STCH	18	/* ST change */
#define TCH	19	/* T-wave change */
#define SYSTOLE	20	/* systole */
#define DIASTOLE 21	/* diastole */
#define	NOTE	22	/* comment annotation */
#define MEASURE 23	/* measurement annotation */
#define PWAVE	24	/* P-wave peak */
#define	BBB	25	/* left or right bundle branch block */
#define	PACESP	26	/* non-conducted pacer spike */
#define TWAVE	27	/* T-wave peak */
#define RHYTHM	28	/* rhythm change */
#define UWAVE	29	/* U-wave peak */
#define	LEARN	30	/* learning */
#define	FLWAV	31	/* ventricular flutter wave */
#define	VFON	32	/* start of ventricular flutter/fibrillation */
#define	VFOFF	33	/* end of ventricular flutter/fibrillation */
#define	AESC	34	/* atrial escape beat */
#define SVESC	35	/* supraventricular escape beat */
#define LINK    36	/* link to external data (aux contains URL) */
#define	NAPC	37	/* non-conducted P-wave (blocked APB) */
#define	PFUS	38	/* fusion of paced and normal beat */
#define WFON	39	/* waveform onset */
#define PQ	WFON	/* PQ junction (beginning of QRS) */
#define WFOFF	40	/* waveform end */
#define	JPT	WFOFF	/* J point (end of QRS) */
#define RONT	41	/* R-on-T premature ventricular contraction */

/* ... annotation codes between RONT+1 and ACMAX inclusive are user-defined */

#define	ACMAX	49	/* value of largest valid annot code (must be < 50) */

#endif
