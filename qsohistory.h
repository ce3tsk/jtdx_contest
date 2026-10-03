/*
 * maintains QSO Histories and autoselect
 * Created by Arvo ES1JA 
 */

#ifndef __QSOHISTORY_H
#define __QSOHISTORY_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QHash>
#include "Radio.hpp"
#include <QRegularExpression>
#include <QtMath>
#include "JTDXDateTime.h"
class QsoHistory
{
 public:
//                  0     1    2    3      4      5        6        7         8         9   10   11     12     13   14   15   16
	enum Status {NONE, RFIN, RCQ, SCQ, RCALL, SCALL, RREPORT, SREPORT, RRREPORT, SRREPORT, RRR, SRR, RRR73, SRR73, R73, S73, FIN};
	/* CE3TSK 2026-09-30: a likely false decode (falsedecodes.h) the autoselect must not pick on its
	   own. The doubt belongs to the latest message from the station, not to the station: each one
	   received sets or clears it, so a false decode that borrowed a real call holds him back only
	   until his next clean message; within one period a clean message wins, so a doubtful decode
	   printed after it (a later pass, the TX background) cannot hold him back either. Our own
	   transmissions leave it as it is (DOUBT_KEEP). */
	enum Doubt {DOUBT_KEEP, DOUBT_SET, DOUBT_CLEAR};
	void init();
	void message(QString const& callsign, Status status, int priority, QString const& param, QString const& tyyp, QString const& continent, QString const& mpx, unsigned time, QString const& rep, int freq,  QString const& mode, Doubt doubt = DOUBT_KEEP);
	void rx(QString const& callsign, int freq);
	void time(unsigned time);
	void owndata (QString const& mycontinent, QString const& myprefix, QString const& mygrid, bool strictdirCQ);
	void wwdigi (bool state); /* CE3TSK: WW Digi contest mode */
	Status status(QString const& callsign, QString &grid);
	/* CE3TSK 2026-10-02: the grid this very call sent - "" when the one kept came from another form of it.
	   An entry is keyed by the BASE call, so VE3ABC's FN03 is also VE3ABC/W1's and KH6/K1ABC's BL11 K1ABC's.
	   The sender changes only with the grid: the window hands the grid it looked up back as the message's
	   parameter, and "CQ KH6/K1ABC" storing K1ABC's FN42 again must not make it KH6/K1ABC's. */
	QString gridSentBy (QString const& callsign) const;
	Status autoseq(QString &callsign, QString &grid, QString &rep, int &rx, int &tx, unsigned &time, int &count, int &prio, QString &mode);
	Status log_data(QString const& callsign, unsigned &time, QString &rrep, QString &srep);
	int remove(QString const& callsign);
	int forget(QString const& callsign);   /* CE3TSK: as remove(), but keeps the blacklist entry */		
	int blacklist(QString const& callsign);
	void calllist(QString const& callsign,int level, unsigned time);
	int reset_count(QString const& callsign,Status status = NONE);
	JTDXDateTime * jtdxtime;
 private:
 	QRegularExpression _gridRe = QRegularExpression("^[A-R]{2,2}[0-9]{2,2}[A-R]{0,2}[0-9]{0,2}[A-R]{0,2}");

 	struct latlng {
 	  double lat;
 	  double lng;
 	};

 	struct QSO
 	{
	  QString	call,grid,r_rep,s_rep,tyyp,continent,mpx,mode;
	  QString	gridCall;   /* CE3TSK: the call, in full, that sent grid - see gridSentBy () */
	  Status	status,srx_c,srx_p,stx_c,stx_p;
	  unsigned	b_time,time;
	  int		distance,rx,tx,count,priority;
	  bool		doubtful = false;   /* CE3TSK: see Doubt - rx () stores entries it fills only in part */
	  unsigned	clean_time = ~0u;   /* CE3TSK: the period of his last clean message - it wins over a doubtful
	                                   one of the same period, whichever the decoder printed first. ~0u: none
	                                   yet - 0 is a real period, 00:00:00 UTC */
	   	
 	};

 	struct CALLED
 	{
	  int rep;
	  unsigned time;
 	};

	QHash<QString, QSO> _data;
	QHash<QString, int> _blackdata;
	QHash<QString, CALLED> _calldata;
	bool _working = false;
	bool as_active = false;
	bool _strictdirCQ = false;
	bool _wwDigi = false; /* CE3TSK: WW Digi contest mode */
	QSO _CQ;
	latlng _mylatlng;
	int a_init = 0;
	int b_init = 0;
	int dist = 0;
	unsigned max_r_time = 0, algo = 0;
	QString myprefix_="" ,mycontinent_="" ,mygrid_="" ,Rrep = "-60" ; 

        double rad_3 = M_PI/180.0; 
        double rad_0 = 20.0 * rad_3;
        double rad_1 = 10.0 * rad_3;
        double rad_2 = 2.0 * rad_3;
        double rad_4 = rad_3 / 12.0;
        double rad_5 = rad_3 / 24.0;
        double rad_6 = rad_3 / 120.0;
        double rad_7 = rad_3 / 240.0;
        double rad_8 = rad_3 / 2880.0;
        double rad_9 = rad_3 / 5760.0;
         
 	latlng fromQth(QString const& qth);
 	int Distance(latlng latlng1,latlng latlng2);
};

#endif


