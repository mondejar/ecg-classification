/* file: init.c		G. Moody	 1 May 1990
			Last revised: 18 November 2013
Initialization functions for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2013 George B. Moody

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

*/

#include "wave.h"
#include "xvwave.h"
#include <xview/notice.h>

static WFDB_Siginfo *df;
static int maxnsig;

void memerr()
{
#ifdef NOTICE
    Xv_notice notice = xv_create((Frame)frame,
				 NOTICE, XV_SHOW, TRUE,
				 NOTICE_MESSAGE_STRINGS,
				  "Insufficient memory", 0,
				 NOTICE_BUTTON_YES, "Continue", NULL);
    xv_destroy_safe(notice);
#else
    (void)notice_prompt((Frame)frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,
			         "Insufficient memory", 0,
		      		NOTICE_BUTTON_YES, "Continue", NULL);
#endif
}

void alloc_sigdata(ns)
int ns;
{
    int i;

    if ((df = realloc(df, ns * sizeof(WFDB_Siginfo))) == NULL ||
	(signame = realloc(signame, ns * sizeof(char *))) == NULL ||
	(sigunits = realloc(sigunits, ns * sizeof(char *))) == NULL ||
	(calibrated = realloc(calibrated, ns * sizeof(char))) == NULL ||
	(scope_v = realloc(scope_v, ns * sizeof(WFDB_Sample))) == NULL ||
	(vref = realloc(vref, ns * sizeof(WFDB_Sample))) == NULL ||
	(level_v = realloc(level_v, ns * sizeof(WFDB_Sample))) == NULL ||
	(v = realloc(v, ns * sizeof(WFDB_Sample))) == NULL ||
	(v0 = realloc(v0, ns * sizeof(WFDB_Sample))) == NULL ||
	(vmax = realloc(vmax, ns * sizeof(WFDB_Sample))) == NULL ||
	(vmin = realloc(vmin, ns * sizeof(WFDB_Sample))) == NULL ||
	(vvalid = realloc(vvalid, ns * sizeof(int))) == NULL ||
	(level_name_string =
		realloc(level_name_string, ns * sizeof(char **))) == NULL ||
	(level_value_string =
		realloc(level_value_string, ns * sizeof(char **))) == NULL ||
	(level_units_string =
		realloc(level_units_string, ns * sizeof(char **))) == NULL ||
	(vscale = realloc(vscale, ns * sizeof(double))) == NULL ||
	(vmag = realloc(vmag, ns * sizeof(double))) == NULL ||
	(dc_coupled = realloc(dc_coupled, ns * sizeof(int))) == NULL ||
	(sigbase = realloc(sigbase, ns * sizeof(int))) == NULL ||
	(blabel = realloc(blabel, ns * sizeof(char *))) == NULL ||
	(level_name =
		realloc(level_name, ns * sizeof(Panel_item))) == NULL ||
	(level_value =
		realloc(level_value, ns * sizeof(Panel_item))) == NULL ||
	(level_units =
		realloc(level_units, ns * sizeof(Panel_item))) == NULL) {
	memerr();
    }
    for (i = maxnsig; i < ns; i++) {
	signame[i] = sigunits[i] = blabel[i] = NULL;
	level_name[i] = level_value[i] = level_units[i] = (Panel_item)NULL;
	dc_coupled[i] = scope_v[i] = vref[i] = level_v[i] = v[i] = v0[i] =
	    vmax[i] = vmin[i] = 0;
	vscale[i] = vmag[i] = 1.0;
	if ((level_name_string[i] = calloc(1, 12)) == NULL ||
	    (level_value_string[i] = calloc(1, 12)) == NULL ||
	    (level_units_string[i] = calloc(1, 12)) == NULL) {
	    memerr();
	}
    }
    maxnsig = ns;
}

/* Open up a new ECG record. */
int record_init(s)
char *s;
{
    char ts[RNLMAX+30];
    int i, rebuild_list, tl;

    /* Suppress error messages from the WFDB library. */
    wfdbquiet();

    /* Do nothing if the current annotation list has been edited and the
       changes can't be saved. */
    if (post_changes() == 0)
	return (0);

    /* Check to see if the signal list must be rebuilt.  Normally, this is
       done whenever the signal list is empty or if the record name has
       changed, but accept_remote_command (see xvwave.c) can force record_init
       not to rebuild the signal list by setting freeze_siglist. */
    if (freeze_siglist)
	freeze_siglist = rebuild_list = 0;
    else
	rebuild_list = (siglistlen == 0) | strcmp(record, s);
	
    /* Save the name of the new record in local storage. */
    strncpy(record, s, RNLMAX);

    /* Reset the frame title. */
    set_frame_title();

    /* Open as many signals as possible. */
    nsig = isigopen(record, NULL, 0);
    if (nsig > maxnsig)
	alloc_sigdata(nsig);
    nsig = isigopen(record, df, nsig);
    /* Get time resolution for annotations in sample intervals.  Except in
       WFDB_HIGHRES mode (selected using the -H option), the resolution is
       1 sample interval.  In WFDB_HIGHRES mode, when editing a multi-frequency
       record, the resolution for annotation times is 1 frame interval (i.e.,
       getspf() sample intervals). */
    atimeres = getspf();
    /* By convention, a zero or negative sampling frequency is interpreted as
       if the value were WFDB_DEFFREQ (from wfdb.h);  the units are samples per
       second per signal. */
    if (nsig < 0 || (freq = sampfreq(NULL)) <= 0.) freq = WFDB_DEFFREQ;
    setifreq(freq);
    /* Inhibit the output of the 'time resolution' comment annotation unless
       we are operating in high-resolution mode. */
    if ((getgvmode() & WFDB_HIGHRES) == 0) setafreq(0.);

    /* Quit if isigopen failed. */
    if (nsig < 0)
	sprintf(ts, "Record %s is unavailable\n", record);
    if (nsig < 0) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
		      NOTICE_MESSAGE_STRINGS,
		      ts, 0,
		      NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	return (0);
    }

    /* If the record has a low sampling rate, use coarse time scale and grid
       mode. */
    if (freq <= 10.0) {
	tsa_index = coarse_tsa_index;
	grid_mode = coarse_grid_mode;
	mode_undo();
	set_modes();
    }
    else {
	tsa_index = fine_tsa_index;
	grid_mode = fine_grid_mode;
	mode_undo();
	set_modes();
    }

    /* Set signal name pointers.  Shorten the conventional "record x, signal n"
       to "signal n". */
    sprintf(ts, "record %s, ", record);
    tl = strlen(ts);
    for (i = 0; i < nsig; i++) {
	if (strncmp(df[i].desc, ts, tl) == 0)
	    signame[i] = df[i].desc + tl;
	else
	    signame[i] = df[i].desc;
	if (df[i].units == NULL || *df[i].units == '\0')
	    sigunits[i] = "mV";
	else
	    sigunits[i] = df[i].units;
	/* Replace any unspecified signal gains with the default gain from
	   wfdb.h;  the units of gain are ADC units per physical unit. */
	if (df[i].gain == 0) {
	    calibrated[i] = 0;
	    df[i].gain = WFDB_DEFGAIN;
	}
	else
	    calibrated[i] = 1;

    }	

    /* Set range for signal selection on analyze panel. */
    reset_maxsig();

    /* Initialize the signal list unless the new record name matches the
       old one. */
    if (rebuild_list) {
	if (nsig > maxsiglistlen) {
	    siglist = realloc(siglist, nsig * sizeof(int));
	    base = realloc(base, nsig * sizeof(int));
	    level = realloc(level, nsig * sizeof(XSegment));
	    maxsiglistlen = nsig;
	}
	for (i = 0; i < nsig; i++)
	    siglist[i] = i;
	siglistlen = nsig;
	reset_siglist();
    }

    /* Calculate the base levels (in display units) for each signal, and for
       annotation display. */
    set_baselines();
    tmag = 1.0;
    vscale[0] = 0.;	/* force clear_cache() -- see calibrate() */
    calibrate();

    /* Rebuild the level window (see edit.c) */
    recreate_level_popup();
    return (1);
}

/* Set_baselines() determines the ordinates for the signal baselines and for
   annotation display.  Note that the signals are drawn centered about the
   calculated baselines (i.e., the baselines bear no fixed relationship to
   the sample values). */

void set_baselines()
{
    int i;

    if (sig_mode == 0)
	for (i = 0; i < nsig; i++)
	    base[i] = canvas_height*(2*i+1.)/(2.*nsig);
    else
	for (i = 0; i < siglistlen; i++)
	    base[i] = canvas_height*(2*i+1.)/(2.*siglistlen);
    if (i > 1)
	abase = (base[i/2] + base[i/2-1])/2;
    else if (nsig > 0)
	abase = canvas_height*4/5;
    else
	abase = canvas_height/2;
}

/* Calibrate() sets scales for the display.  Ordinate (amplitude) scaling
   is determined for each signal from the gain recorded in the header file,
   and from the display scales in the calibration specification file, but
   calibrate() scales the abscissa (time) for all signals based on the
   sampling frequency for signal 0. */

void calibrate()
{
    int i;
    extern char *getenv();
    struct WFDB_calinfo ci;

    /* vscale is a multiplicative scale factor that converts sample values to
       window ordinates.  Since window ordinates are inverted, vscale includes
       a factor of -1. */
    if (vscale[0] == 0.0) {
	clear_cache();

	/* If specified, read the calibration file to get standard scales. */
	if ((cfname == (char *)NULL) && (cfname = getenv("WFDBCAL")))
	    calopen(cfname);

	for (i = 0; i < nsig; i++) {
	    vscale[i] = - vmag[i] * millivolts(1) / df[i].gain;
	    dc_coupled[i] = 0;
	    if (getcal(df[i].desc, df[i].units, &ci) == 0 && ci.scale != 0.0) {
		vscale[i] /= ci.scale;
		if (ci.caltype & 1) {
		    dc_coupled[i] = 1;
		    sigbase[i] = df[i].baseline;
		    if (blabel[i] = (char *)malloc(strlen(ci.units) +
						   strlen(df[i].desc) + 6))
			sprintf(blabel[i], "0 %s (%s)", ci.units, df[i].desc);
		}
	    }
	}
    }
    /* vscalea is used in the same way as vscale, but only when displaying
       the annotation 'num' fields as a signal. */
    vscalea = - millivolts(1);
    if (af.name && getcal(af.name, "units", &ci) == 0 && ci.scale != 0)
	vscalea /= ci.scale;
    else if (getcal("ann", "units", &ci) == 0 && ci.scale != 0)
	vscalea /= ci.scale;
    else
	vscalea /= WFDB_DEFGAIN;

    /* tscale is a multiplicative scale factor that converts sample intervals
       to window abscissas. */
    if (freq == 0.0) freq = WFDB_DEFFREQ;
    if (tmag <= 0.0) tmag = 1.0;
    nsamp = canvas_width_sec * freq / tmag;
    tscale = tmag * seconds(1) / freq;
}
