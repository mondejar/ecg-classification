/* file: sumstats.c	G. Moody	17 August 1989
   			Last revised:	10 August 2010
-------------------------------------------------------------------------------
sumstats: Derive aggregate statistics from bxb, rxr, or epic line-format output
Copyright (C) 1989-2010 George B. Moody

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA.

You may contact the author by e-mail (george@mit.edu) or postal mail
(MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
_______________________________________________________________________________

This program derives the aggregate statistics described in sections 3.5.2 and
3.5.3 of the American National Standard, "Testing and reporting performance
results of cardiac rhythm and ST segment measurement algorithms" (ANSI/AAMI
EC57:1998, based on the earlier AAMI ECAR:1987), and in sections 4.2.14.4.1
and 4.2.14.4.2 of the American National Standard, "Ambulatory electrocardio-
graphs" (ANSI/AAMI EC38:1998).  These standards are available from AAMI,
1110 N Glebe Road, Suite 220, Arlington, VA 22201 USA (http://www.aami.org/).
*/

#include <stdio.h>
#include <wfdb/wfdb.h>

static int nrec, Nrec, Vrec, Frec;
static int NQS, NQP, NVS, NVP, NVF, NSVS, NSVP, NRRE;
static long Nn, Ns, Nv, No, Nx,
	    Sn, Ss, Sv, So, Sx,
	    Vn, Vs, Vv, Vo, Vx,
	    Fn, Fs, Fv, Fo, Fx,
			    Qx,
	    On, Os, Ov;
static long QTP, QFN, QFP, ST;
static long CTS, CFN, CTP, CFP, STS, SFN, STP, SFP, LTS, LFN, LTP, LFP;
static long ETS, EFN, ETP, EFP;
static long NCS, NCP, NSS, NSP, NLS, NLP, NES, NEP, NDS, NDP;
static long NT, VT, FT, QT;
static long detected_episode_length, overlap, total_episode_length;
static double CQS, CQP, CVS, CVP, CVF, CSVS, CSVP, CRRE, CBM, CNM, CVM, CFM;
static double CCS, CCP, CSS, CSP, CLS, CLP, CES, CEP, CDS, CDP, CERR, CMREF;
char *pname;		/* name by which this program was invoked */

/* The strings which follow must match the first lines of the report formats
   read by this program (see `bxb.c', `rxr.c', and `epic.c'). */
static char s1[] =
 "Record Nn' Vn' Fn' On'  Nv   Vv  Fv' Ov' No' Vo' Fo'  Q Se   Q +P   V Se   V +P  V FPR\n";
static char s2[] =
 "Record Nx   Vx   Fx   Qx  % beats  % N    % V    % F   Total Shutdown\n";
static char s3[] =
 "Record CTs CFN CTp CFP STs SFN STp SFP LTs LFN LTp LFP  CSe C+P SSe S+P LSe L+P\n";
static char s4[] =
 "Record Nn' Sn' Vn' Fn' On'  Ns  Ss  Vs  Fs' Os' Nv  Sv   Vv  Fv' Ov' No' So' Vo' Fo'  Q Se   Q +P   V Se   V +P   S Se   S +P RR err\n";
static char s5[] =
 "Record Nx   Sx   Vx   Fx   Qx  % beats  % N    % S    % V    % F   Total Shutdown\n";
static char s6[] = "(SVE run detection)\n";
static char s7a[] = "(AF detection)\n";
static char s7b[] = "(VF detection)\n";
static char s7c[] = "(Ischemic ST detection, both signals)\n";
static char s7d[] = "(Ischemic ST detection, signal 0)\n";
static char s7e[] = "(Ischemic ST detection, signal 1)\n";
static char s8[] = "(Measurement errors)\n";

main(argc, argv)
int argc;
char *argv[];
{
    static char s[256];
    char *prog_name();
    int type;
    FILE *ifile;
    void pstat(), rstat();

    pname = prog_name(argv[0]);
    if (argc < 2) {
	(void)fprintf(stderr, "usage: %s FILE\n", pname);
	(void)fprintf(stderr,
	  " FILE must be the name of an l-format file with column headings\n");
	(void)fprintf(stderr," created by `bxb', `rxr', `mxm', or `epic'\n");
	exit(1);
    }
    if ((ifile = fopen(argv[1], "r")) == NULL) {
	(void)fprintf(stderr, "%s: can't open %s\n", pname, argv[1]);
	exit(2);
    }
    if (fgets(s, 256, ifile) == NULL) {
	(void)fprintf(stderr, "%s: input file %s is empty or unreadable\n",
		      pname,
		argv[1]);
	exit(2);
    }
    /* Identify the input file type. */
    if (strcmp(s, s1) == 0) type = 1;	/* bxb -l beat-by-beat report */
    else if (strcmp(s, s2) == 0) type = 2;	/* bxb -l shutdown report */
    else if (strcmp(s, s3) == 0) type = 3;	/* rxr VE run-by-run report */
    else if (strcmp(s, s4) == 0) type = 4;	/* bxb -L beat-by-beat report*/
    else if (strcmp(s, s5) == 0) type = 5;	/* bxb -L shutdown report */
    else if (strcmp(s, s6) == 0) type = 6;	/* rxr SVE run-by-run report */
    else if (strcmp(s, s7a) == 0) type = 7;	/* epic AF report */
    else if (strcmp(s, s7b) == 0) type = 7;	/* epic VF report */
    else if (strcmp(s, s7c) == 0) type = 7;	/* epic (2-signal) ST report */
    else if (strcmp(s, s7d) == 0) type = 7;	/* epic ST signal 0 report */
    else if (strcmp(s, s7e) == 0) type = 7;	/* epic ST signal 1 report */
    else if (strcmp(s, s8) == 0) type = 8;	/* mxm report */
    else {
	(void)fprintf(stderr,
"%s: input file %s does not appear to be a `bxb', `rxr', `mxm', or `epic'\n",
		pname, argv[1]);
	(void)fprintf(stderr, " -l or -L file with column headings\n");
	exit(2);
    }

    (void)setsampfreq(1000.);	/* arbitrary initialization, required for
				   use of strtim() and mstimstr() later on */

    /* Copy and interpret the input file. */
    (void)printf("%s", s);	/* print column headings */
    if (type == 2 || type == 5 || type == 6 || type == 7 || type == 8) {
	    /* copy and print second header line of report */
	(void)fgets(s, 256, ifile);
	(void)printf("%s", s);
    }
    if (type == 8) {
	    /* copy and print third header line of report */
	(void)fgets(s, 256, ifile);
	(void)printf("%s", s);
    }
    while (fgets(s, 256, ifile)) {
	(void)printf("%s", s);
	if (unpack(type, s) == 0) {
	    (void)fprintf(stderr,
			  "%s: illegal format in %s:\n", pname, argv[1]);
	    (void)fprintf(stderr, "%s", s);
	    exit(5);
	}
    }
    (void)fclose(ifile);

    /* Calculate and print the aggregate statistics. */
    switch (type) {
      case 1:	/* bxb -l beat-by-beat table */
	QTP = Nn+Nv+Vn+Vv+Fn+Fv;
	QFN = No+Vo+Fo;
	QFP = On+Ov;
	(void)printf(
	 "__________________________________________________________________");
	(void)printf("____________________\n");
	(void)printf("Sum  %6ld %3ld %3ld %3ld", Nn, Vn, Fn, On);
	(void)printf(" %3ld %4ld %3ld %3ld", Nv, Vv, Fv, Ov);
	(void)printf(" %3ld %3ld %3ld\n", No, Vo, Fo);
	(void)printf("Gross                                                 ");
        pstat(" %6.2f", (double)QTP, (double)(QTP+QFN));
	pstat(" %6.2f", (double)QTP, (double)(QTP+QFP));
	pstat(" %6.2f", (double)Vv, (double)(Vn+Vv+Vo));
	pstat(" %6.2f", (double)Vv, (double)(Nv+Vv+Ov));
	pstat(" %6.3f", (double)(Nv+Ov), (double)(Nn+Fn+On+Nv+Ov));
	(void)printf("\n");
	(void)printf("Average                                               ");
	pstat(" %6.2f", CQS, (double)NQS);
	pstat(" %6.2f", CQP, (double)NQP);
	pstat(" %6.2f", CVS, (double)NVS);
	pstat(" %6.2f", CVP, (double)NVP);
	pstat(" %6.3f", CVF, (double)NVF);
	(void)printf("\nTotal QRS complexes: %ld  Total VEBs: %ld\n",
		     QTP+QFN, Vn+Vv+Vo);
	break;
      case 2:	/* bxb shutdown report */
	(void)printf("______________________________________________________");
	(void)printf("______________\n");
	(void)printf("Sum   %4ld %4ld %4ld %4ld", Nx, Vx, Fx, Qx);
	(void)printf("                               %4ld seconds\n", ST);
	(void)printf("Gross                    ");
        pstat(" %6.2f", (double)Nx+Vx+Fx+Qx, (double)(NT+VT+FT+QT));
	pstat(" %6.2f", (double)Nx, (double)(NT));
	pstat(" %6.2f", (double)Vx, (double)(VT));
	pstat(" %6.2f", (double)Fx, (double)(FT));
	(void)printf("\n");
	(void)printf("Average                  ");
	pstat(" %6.2f", CBM, (double)nrec);
	pstat(" %6.2f", CNM, (double)Nrec);
	pstat(" %6.2f", CVM, (double)Vrec);
	pstat(" %6.2f", CFM, (double)Frec);
	(void)printf("\n");
	break;
      case 3:	/* rxr VE run-by-run table */
      case 6:	/* rxr SVE run-by-run table */
	(void)printf("______________________________________________________");
	(void)printf("_______________________\n");
	(void)printf(
	 "Sum   %4ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld\n",
	       CTS, CFN, CTP, CFP, STS, SFN, STP, SFP, LTS, LFN, LTP, LFP);
	(void)printf(
		    "Gross                                                  ");
	rstat(" %3.0f", (double)CTS, (double)(CTS+CFN));
	rstat(" %3.0f", (double)CTP, (double)(CTP+CFP));
	rstat(" %3.0f", (double)STS, (double)(STS+SFN));
	rstat(" %3.0f", (double)STP, (double)(STP+SFP));
	rstat(" %3.0f", (double)LTS, (double)(LTS+LFN));
	rstat(" %3.0f", (double)LTP, (double)(LTP+LFP));
	(void)printf(
		  "\nAverage                                                ");
	rstat(" %3.0f", CCS, (double)NCS);
	rstat(" %3.0f", CCP, (double)NCP);
	rstat(" %3.0f", CSS, (double)NSS);
	rstat(" %3.0f", CSP, (double)NSP);
	rstat(" %3.0f", CLS, (double)NLS);
	rstat(" %3.0f", CLP, (double)NLP);
	(void)printf(
	"\nTotal couplets: %ld  Total short runs: %ld  Total long runs: %ld\n",
	       CTS+CFN, STS+SFN, LTS+LFN);
	break;
      case 4:	/* bxb -L beat-by-beat table */
	QTP = Nn+Ns+Nv+Sn+Ss+Sv+Vn+Vs+Vv+Fn+Fs+Fv;
	QFN = No+So+Vo+Fo;
	QFP = On+Os+Ov;
	(void)printf("______________________________________________________");
	(void)printf("______________________________________________________");
	(void)printf("________________________\n");
	(void)printf("Sum %6ld %3ld %3ld %3ld %3ld", Nn, Sn, Vn, Fn, On);
	(void)printf(" %3ld %3ld %3ld %3ld %3ld", Ns, Ss, Vs, Fs, Os);
	(void)printf(" %3ld %3ld %4ld %3ld %3ld", Nv, Sv, Vv, Fv, Ov);
	(void)printf(" %3ld %3ld %3ld %3ld\n", No, So, Vo, Fo);
	(void)printf("Gross                                                 ");
	(void)printf("                             ");
	pstat(" %6.2f", (double)QTP, (double)(QTP+QFN));
	pstat(" %6.2f", (double)QTP, (double)(QTP+QFP));
	pstat(" %6.2f", (double)Vv, (double)(Vn+Vs+Vv+Vo));
	pstat(" %6.2f", (double)Vv, (double)(Nv+Sv+Vv+Ov));
	pstat(" %6.2f", (double)Ss, (double)(Sn+Ss+Sv+So));
	pstat(" %6.2f", (double)Ss, (double)(Ns+Ss+Vs+Os));
	(void)printf("\nAverage                                             ");
	(void)printf("                               ");
	pstat(" %6.2f", CQS, (double)NQS);
	pstat(" %6.2f", CQP, (double)NQP);
	pstat(" %6.2f", CVS, (double)NVS);
	pstat(" %6.2f", CVP, (double)NVP);
	pstat(" %6.2f", CSVS, (double)NSVS);
	pstat(" %6.2f", CSVP, (double)NSVP);
	pstat(" %6.2f", CRRE/100.0, (double)NRRE);
	(void)printf(
	     "\nTotal QRS complexes: %ld  Total VEBs: %ld  Total SVEBs: %ld\n",
		  QTP+QFN, Vn+Vs+Vv+Vo, Sn+Ss+Sv+So);
	break;
      case 5:	/* bxb -L shutdown report */
	(void)printf("______________________________________________________");
	(void)printf("__________________\n");
	(void)printf("Sum  %4ld %4ld %4ld %4ld %4ld", Nx, Sx, Vx, Fx, Qx);
	(void)printf("                                      %4ld seconds\n",
		     ST);
	break;
      case 7:	/* epic report */
	(void)printf("______________________________________________________");
	(void)printf("__________________\n");
	(void)printf("Sum    %4ld %4ld %4ld %4ld                   %s",
		     ETS, EFN, ETP, EFP, mstimstr(total_episode_length));
	(void)printf("   %s\n", mstimstr(detected_episode_length));
	(void)printf("Gross                      ");
	rstat(" %3.0f", (double)ETS, (double)(ETS+EFN));
	rstat(" %3.0f", (double)ETP, (double)(ETP+EFP));
	rstat(" %3.0f", (double)overlap, (double)total_episode_length);
	rstat(" %3.0f", (double)overlap, (double)detected_episode_length);
	(void)printf("\nAverage                    ");
	rstat(" %3.0f", CES, (double)NES);
	rstat(" %3.0f", CEP, (double)NEP);
	rstat(" %3.0f", CDS, (double)NDS);
	rstat(" %3.0f", CDP, (double)NDP);
	(void)printf("\n");
	break;
      case 8:	/* mxm report */
	(void)printf("__________________________________________________\n");
	(void)printf("Average");
	rstat(" %7.4f", CERR, 100.*nrec);
	(void)printf("\t\t");
	rstat("%g", CMREF, 100.*nrec);
	(void)printf("\n");
	break;
    }
    (void)printf("\nSummary of results from %d record%s\n", nrec,
	      nrec == 1 ? "" : "s");
    exit(0);	/*NOTREACHED*/
}

/* `unpack' interprets a line from the input file. */
unpack(type, s)
int type;
char *s;
{
    static char rec[10], mb[8], mn[8], ms[8], mv[8], mf[8], mds[8], mdp[8];
    static char qse[8], qpp[8], vse[8], vpp[8], sse[8], spp[8], srre[8];
    static char rts[20], tts[20];
    static double rre, ds, dp, err, mref;
    static int cts, cfn, ctp, cfp, sts, sfn, stp, sfp, lts, lfn, ltp, lfp;
    static int dummy, nt, vt, ft, qt;
    static long ets, efn, etp, efp;
    static long nn, sn, vn, fn, on, ns, ss, vs, fs, os;
    static long nv, sv, vv, fv, ov, no, so, vo, fo;
    static long nx, sx, vx, fx, qx, st;
    static long rt, tt;

    switch (type) {
      case 1:	/* bxb beat-by-beat report */
	fo = -1L;
	(void)sscanf(s, "%s%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld", rec,
	       &nn, &vn, &fn, &on, &nv, &vv, &fv, &ov, &no, &vo, &fo);
	if (fo < 0L) return (0);
	Nn += nn; Vn += vn; Fn += fn; On += on;
	Nv += nv; Vv += vv; Fv += fv; Ov += ov;
	No += no; Vo += vo; Fo += fo;
	if (nn+nv+no + vn+vv+vo + fn+fv+fo) {
	    CQS += (nn+nv+vn+vv+fn+fv)/(double)(nn+nv+no+vn+vv+vo+fn+fv+fo);
	    NQS++;
	}
	if (nn+vn+fn+on + nv+vv+fv+ov) {
	    CQP += (nn+vn+fn+nv+vv+fv)/(double)(nn+vn+fn+on+nv+vv+fv+ov);
	    NQP++;
	}
	if (vn+vv+vo) {
	    CVS += vv/(double)(vn+vv+vo);
	    NVS++;
	}
	if (nv+vv+ov) {
	    CVP += vv/(double)(nv+vv+ov);
	    NVP++;
	}
	if (nn+fn+on + nv+ov) {
	    CVF += (nv+ov)/(double)(nn+fn+on + nv+ov);
	    NVF++;
	}
	nrec++;
	return (1);
      case 2:	/* bxb -l shutdown report */
	st = -1L;
	(void)sscanf(s, "%s%ld%ld%ld%ld%s%s%s%s%ld seconds %ld%ld%ld%ld%ld",
		     rec, &nx, &vx, &fx, &qx, mb, mn, mv, mf, &st,
		     &nt, &dummy, &vt, &ft, &qt);
	if (st < 0L) return (0);
	Nx += nx; Vx += vx; Fx += fx; Qx += qx; ST += st;
	NT += nt; VT += vt; FT += ft; QT += qt;
	if (nt + vt + ft + qt) {
	    CBM += (nx + vx + fx + qx)/(double)(nt + vt + ft + qt); nrec++;
	    if (nt) { CNM += nx/(double)nt; Nrec++; NT += nt; }
	    if (vt) { CVM += vx/(double)vt; Vrec++; VT += vt; }
	    if (ft) { CFM += fx/(double)ft; Frec++; FT += ft; }
	    QT += qt;
	}
	return (1);
      case 3:	/* rxr run-by-run report */
      case 6:
	lfp = -1;
	(void)sscanf(s, "%s%d%d%d%d%d%d%d%d%d%d%d%d", rec,
	       &cts, &cfn, &ctp, &cfp, &sts, &sfn, &stp, &sfp,
	       &lts, &lfn, &ltp, &lfp);
	if (lfp < 0) return (0);
	CTS += cts; CFN += cfn; CTP += ctp; CFP += cfp;
	STS += sts; SFN += sfn; STP += stp; SFP += sfp;
	LTS += lts; LFN += lfn; LTP += ltp; LFP += lfp;
	if (cts+cfn) {
	    CCS += cts/(double)(cts+cfn);
	    NCS++;
	}
	if (ctp+cfp) {
	    CCP += ctp/(double)(ctp+cfp);
	    NCP++;
	}
	if (sts+sfn) {
	    CSS += sts/(double)(sts+sfn);
	    NSS++;
	}
	if (stp+sfp) {
	    CSP += stp/(double)(stp+sfp);
	    NSP++;
	}
	if (lts+lfn) {
	    CLS += lts/(double)(lts+lfn);
	    NLS++;
	}
	if (ltp+lfp) {
	    CLP += ltp/(double)(ltp+lfp);
	    NLP++;
	}
	nrec++;
	return (1);
      case 4:	/* bxb -L beat-by-beat report */
	fo = -1L;
	(void)sscanf(s,
 "%s%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%ld%s%s%s%s%s%s%s",
	       rec, &nn, &sn, &vn, &fn, &on, &ns, &ss, &vs, &fs, &os,
			  &nv, &sv, &vv, &fv, &ov, &no, &so, &vo, &fo,
				  qse, qpp, vse, vpp, sse, spp, srre);
	if (fo < 0L) return (0);
	Nn += nn; Sn += sn; Vn += vn; Fn += fn; On += on;
	Ns += ns; Ss += ss; Vs += vs; Fs += fs; Os += os;
	Nv += nv; Sv += sv; Vv += vv; Fv += fv; Ov += ov;
	No += no; So += so; Vo += vo; Fo += fo;
	if (nn+ns+nv+no + sn+ss+sv+so + vn+vs+vv+vo + fn+fs+fv+fo) {
	    CQS += (nn+ns+nv+sn+ss+sv+vn+vs+vv+fn+fs+fv) /
		    (double)(nn+ns+nv+no+sn+ss+sv+so+vn+vs+vv+vo+fn+fs+fv+fo);
	    NQS++;
	}
	if (nn+sn+vn+fn+on + ns+ss+vs+fs+os + nv+sv+vv+fv+ov) {
	    CQP += (nn+sn+vn+fn+ns+ss+vs+fs+nv+sv+vv+fv) /
		    (double)(nn+sn+vn+fn+on+ns+ss+vs+fs+os+nv+sv+vv+fv+ov);
	    NQP++;
	}
	if (vn+vs+vv+vo) {
	    CVS += vv/(double)(vn+vs+vv+vo);
	    NVS++;
	}
	if (nv+sv+vv+ov) {
	    CVP += vv/(double)(nv+sv+vv+ov);
	    NVP++;
	}
	if (sn+ss+sv+so) {
	    CSVS += ss/(double)(sn+ss+sv+so);
	    NSVS++;
	}
	if (ns+ss+vs+os) {
	    CSVP += ss/(double)(ns+ss+vs+os);
	    NSVP++;
	}
	rre = -1.0;
	(void)sscanf(srre, "%lf", &rre);
	if (rre >= 0.0) {
	    CRRE += rre;
	    NRRE++;
	}
	nrec++;
	return (1);
      case 5:	/* bxb -L shutdown report */
	st = -1L;
	(void)sscanf(s, "%s%ld%ld%ld%ld%ld%s%s%s%s%s%ld", rec,
	       &nx, &sx, &vx, &fx, &qx, mb, mn, ms, mv, mf, &st);
	if (st < 0L) return (0);
	Nx += nx; Sx += sx; Vx += vx; Fx += fx; Qx += qx; ST += st;
	nrec++;
	return (1);
      case 7:	/* epic report */
	tts[0] = '\0';
	(void)sscanf(s, "%s%ld%ld%ld%ld%s%s%s%s%s%s", rec,
		     &ets, &efn, &etp, &efp, mb, mn, mds, mdp, rts, tts);
	if (tts[0] == '\0') return (0);
	ETS += ets; EFN += efn; ETP += etp; EFP += efp;
	if (ets+efn) {
	    CES += ets/(double)(ets+efn);
	    NES++;
	}
	if (etp+efp) {
	    CEP += etp/(double)(etp+efp);
	    NEP++;
	}
	total_episode_length += rt = strtim(rts);
	detected_episode_length += tt = strtim(tts);

	if (rt > 0L) {
	    CDS += ds = atof(mds)/100.;
	    NDS++;
	}

	if (tt > 0L) {
	    CDP += dp = atof(mdp)/100.;
	    NDP++;
	}

	if (rt > tt)
	    overlap += rt * ds;
	else if (tt > 0L)
	    overlap += tt * dp;

	nrec++;
	return (1);
      case 8:	/* mxm report */
	err = -1.0;
	(void)sscanf(s, "%s%lf%lf\n", rec, &err, &mref);
	if (err < 0.0) return (0);
	CERR += err;
	CMREF += mref;
	nrec++;
	return (1);
    }
    return (0);
}

/* `pstat' prints a/b in percentage units, or a hyphen if a/b is undefined. */
void pstat(s, a, b)
char *s;
double a, b;
{
    if (b <= 0.) (void)printf("      -");
    else (void)printf(s, 100.*a/b);
}

/* `rstat' prints a/b in percentage units, or a hyphen if a/b is undefined. */
void rstat(s, a, b)
char *s;
double a, b;
{
    if (b <= 0.) (void)printf("   -");
    else (void)printf(s, 100.*a/b);
}

char *prog_name(s)
char *s;
{
    char *p = s + strlen(s);

#ifdef MSDOS
    while (p >= s && *p != '\\' && *p != ':') {
	if (*p == '.')
	    *p = '\0';		/* strip off extension */
	if ('A' <= *p && *p <= 'Z')
	    *p += 'a' - 'A';	/* convert to lower case */
	p--;
    }
#else
    while (p >= s && *p != '/')
	p--;
#endif
    return (p+1);
}
