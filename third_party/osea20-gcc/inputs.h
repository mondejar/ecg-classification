/* file: inputs.h

This file defines the set of records that 'easytest' processes and that
'bxbep' uses for deriving performance statistics.
*/

// Uncomment only one of the next three lines.
#define PHYSIOBANK	// 25 freely available MIT-BIH records from PhysioBank
// #define MITDB	// 48 MIT-BIH records in 'mitdb'
// #define AHADB	// 69 AHA records in 'ahadb'

// If using MITDB or AHADB, edit the definition of ECG_DB_PATH to include
// the location where you have installed the database files.  Do not remove
// the initial "." or the space following it in the definition of ECG_DB_PATH!

#ifdef PHYSIOBANK
#define ECG_DB_PATH	". http://www.physionet.org/physiobank/database/mitdb"
int Records[] = {100,101,102,103,104,105,106,107,118,119,
		 200,201,202,203,205,207,208,209,210,212,213,214,215,217,219};
#endif

#ifdef MITDB
#define ECG_DB_PATH	". mitdb"
int Records[] = {100,101,102,103,104,105,106,107,108,109,111,112,
		 113,114,115,116,117,118,119,121,122,123,124,200,
		 201,202,203,205,207,208,209,210,212,213,214,215,
		 217,219,220,221,222,223,228,230,231,232,233,234};
#endif

#ifdef AHADB
#define ECG_DB_PATH	". ahadb"
int Records[] = {1201,1202,1203,1204,1205,1206,1207,1208,1209,1210,
		 2201,2203,2204,2205,2206,2207,2208,2209,2210,
		 3201,3202,3203,3204,3205,3206,3207,3208,3209,3210,
		 4201,4202,4203,4204,4205,4206,4207,4208,4209,4210,
		 5201,5202,5203,5204,5205,5206,5207,5208,5209,5210,
		 6201,6202,6203,6204,6205,6206,6207,6208,6209,6210,
		 7201,7202,7203,7204,7205,7206,7207,7208,7209,7210};
#endif

#define REC_COUNT	(sizeof(Records)/sizeof(int))

