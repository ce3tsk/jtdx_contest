// This source code file was last time modified by Arvo ES1JA on November 17th, 2019
// All changes are shown in the patch file coming together with the full JTDX source code.
// Changed since for JTDX_contest by Tihomir Sokcevic CE3TSK; those changes are marked CE3TSK.

#include "adif.h"
#include "../Radio.hpp"
#include "../usstates.h"   /* CE3TSK */
#include <QFile>
#include <QTextStream>
//#include <QDateTime>
#include <QDebug>

namespace
{
  /* CE3TSK 2026-10-04 (review): the lookup every match function shares - worked at all, in a mode, on a band, or on a
     band in a mode - over the four counts each index keeps under key, key+mode, key+band and key+band+mode. It was
     written out eight times. */
  bool workedIn (QHash<QString, int> const& all, QHash<QString, int> const& inMode, QHash<QString, int> const& onBand,
                 QHash<QString, int> const& onBandInMode, QString const& key, QString const& band, QString const& mode)
  {
    if (band.isEmpty () && mode.isEmpty ()) return all.value (key, 0) > 0;
    if (band.isEmpty ()) return inMode.value (key + mode, 0) > 0;
    if (mode.isEmpty ()) return onBand.value (key + band, 0) > 0;
    return onBandInMode.value (key + band + mode, 0) > 0;
  }
}

/*
<CALL:4>W1XT<BAND:3>20m<FREQ:6>14.076<GRIDSQUARE:4>DM33<MODE:4>JT65<RST_RCVD:3>-21<RST_SENT:3>-14<QSO_DATE:8>20110422<TIME_ON:4>0417<TIME_OFF:4>0424<TX_PWR:1>4<COMMENT:34>1st JT65A QSO.   Him: mag loop 20W<STATION_CALLSIGN:6>VK3ACF<MY_GRIDSQUARE:6>qf22lb<eor>
<CALL:6>IK1SOW<BAND:3>20m<FREQ:6>14.076<GRIDSQUARE:4>JN35<MODE:4>JT65<RST_RCVD:3>-19<RST_SENT:3>-11<QSO_DATE:8>20110422<TIME_ON:4>0525<TIME_OFF:4>0533<TX_PWR:1>3<STATION_CALLSIGN:6>VK3ACF<MY_GRIDSQUARE:6>qf22lb<eor>
<CALL:6:S>W4ABC> ...
*/

void ADIF::init(QString filename)
{
    _filename = filename;
    _data.clear();
    _countries.init("","");
}

void ADIF::init(QString filename, CountryDat *countries)
{
    _filename = filename;
    _data.clear();
    _countries = *countries;
}

QString ADIF::_extractField(const QString line, const QString fieldName)
{
    int fieldNameIndex = line.indexOf('<' + fieldName  ,0,Qt::CaseInsensitive);
    if (fieldNameIndex >=0)
    {
        int closingBracketIndex = line.indexOf('>',fieldNameIndex);
        int fieldLengthIndex = line.indexOf(':',fieldNameIndex);  // find the size delimiter
        int dataTypeIndex = -1;
        if (fieldLengthIndex >= 0)
        {
          dataTypeIndex = line.indexOf(':',fieldLengthIndex+1);  // check for a second : indicating there is a data type
          if (dataTypeIndex > closingBracketIndex)
            dataTypeIndex = -1; // second : was found but it was beyond the closing >
        }

        if ((closingBracketIndex > fieldNameIndex) && (fieldLengthIndex > fieldNameIndex) && (fieldLengthIndex< closingBracketIndex))
        {
            int fieldLengthCharCount = closingBracketIndex - fieldLengthIndex -1;
            if (dataTypeIndex >= 0)
              fieldLengthCharCount -= 2; // data type indicator is always a colon followed by a single character
            QString fieldLengthString = line.mid(fieldLengthIndex+1,fieldLengthCharCount);
            int fieldLength = fieldLengthString.toInt();
            if (fieldLength > 0)
            {
              QString field = line.mid(closingBracketIndex+1,fieldLength);
              return field;
            }
       }
    }
    return "";
}



void ADIF::load(const QString mycall,const QString mygrid,const QString mydate)
{
    _data.clear();
    _cqzWorked.clear();
    _ituzWorked.clear();
    _countriesWorked.clear();
    _gridsWorked.clear();
    _pxsWorked.clear();
    _callsWorked.clear();
    _cqzbandWorked.clear();
    _ituzbandWorked.clear();
    _countriesbandWorked.clear();
    _gridsbandWorked.clear();
    _pxsbandWorked.clear();
    _callsbandWorked.clear();
    _cqzmodeWorked.clear();
    _ituzmodeWorked.clear();
    _countriesmodeWorked.clear();
    _gridsmodeWorked.clear();
    _pxsmodeWorked.clear();
    _callsmodeWorked.clear();
    _cqzbandmodeWorked.clear();
    _ituzbandmodeWorked.clear();
    _countriesbandmodeWorked.clear();
    _gridsbandmodeWorked.clear();
    _pxsbandmodeWorked.clear();
    _callsbandmodeWorked.clear();
    /* CE3TSK: WW Digi contest, 2 character grid fields */
    _fieldsWorked.clear();
    _fieldsbandWorked.clear();
    _fieldsmodeWorked.clear();
    _fieldsbandmodeWorked.clear();
    _statesWorked.clear();       /* CE3TSK */
    _statesbandWorked.clear();
    _statesmodeWorked.clear();
    _statesbandmodeWorked.clear();
    _counts.clear();
    
    QFile inputFile(_filename);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        while ( !in.atEnd() )
        {
            QString record = in.readLine();
			while (record.indexOf("<EOR>", 0, Qt::CaseInsensitive) == -1 && !in.atEnd()) {
				record.append(in.readLine());
			}				
            QString mytime = _extractField(record,"QSO_DATE:")+_extractField(record,"TIME_ON:");
            while (mytime.length() < 14) mytime += "0";
            if ((mycall.isEmpty () || _extractField(record,"STATION_CALLSIGN:") == mycall) && 
                (mygrid.isEmpty () || mygrid.left(4) == _extractField(record,"MY_GRIDSQUARE:").left(4).toUpper()) && 
                (mydate.isEmpty () || mytime.toLongLong() >= mydate.toLongLong())) {
                QSO q;
                q.call = _extractField(record,"CALL:");
                q.band = _extractField(record,"BAND:").toLower();
                q.mode = _extractField(record,"MODE:").toUpper();
                if (q.mode == "MFSK") q.mode = _extractField(record,"SUBMODE:").toUpper();
                if (q.mode.left(3) == "JT9") q.mode = "JT9";
                else if (q.mode.left(4) == "JT65") q.mode = "JT65";
                q.date = _extractField(record,"QSO_DATE:");
                q.gridsquare = _extractField(record,"GRIDSQUARE:");
                q.name = _extractField(record,"NAME:");
                if (!q.call.isEmpty ()) {
                    _data.insert(q.call,q);
                    _callsWorked.insert(q.call,_callsWorked.value(q.call,0)+1);
                    _callsbandWorked.insert(q.call+q.band,_callsbandWorked.value(q.call+q.band,0)+1);
                    _callsmodeWorked.insert(q.call+q.mode,_callsmodeWorked.value(q.call+q.mode,0)+1);
                    _callsbandmodeWorked.insert(q.call+q.band+q.mode,_callsbandmodeWorked.value(q.call+q.band+q.mode,0)+1);
                    _pxsWorked.insert(Radio::striped_prefix(Radio::effective_prefix(q.call)), _pxsWorked.value(Radio::striped_prefix(Radio::effective_prefix(q.call)),0)+1);
                    _pxsbandWorked.insert(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.band, _pxsbandWorked.value(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.band,0)+1);
                    _pxsmodeWorked.insert(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.mode, _pxsmodeWorked.value(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.mode,0)+1);
                    _pxsbandmodeWorked.insert(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.band+q.mode, _pxsbandmodeWorked.value(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.band+q.mode,0)+1);
                    _counts.insert(q.mode,_counts.value(q.mode,0)+1);
                    QString country = _countries.find(q.call);
                    if (!country.isEmpty ()) { //  country was found
                        QStringList items = country.split(',');
                        _countriesWorked.insert(items[0]+','+items[1]+','+items[2], _countriesWorked.value(items[0]+','+items[1]+','+items[2],0)+1);
                        _countriesbandWorked.insert(items[0]+','+items[1]+','+items[2]+q.band, _countriesbandWorked.value(items[0]+','+items[1]+','+items[2]+q.band,0)+1);   /* CE3TSK: its own count (review) */
                        _countriesmodeWorked.insert(items[0]+','+items[1]+','+items[2]+q.mode, _countriesmodeWorked.value(items[0]+','+items[1]+','+items[2]+q.mode,0)+1);   /* CE3TSK: its own count (review) */
                        _countriesbandmodeWorked.insert(items[0]+','+items[1]+','+items[2]+q.band+q.mode, _countriesbandmodeWorked.value(items[0]+','+items[1]+','+items[2]+q.band+q.mode,0)+1);   /* CE3TSK: its own count (review) */
                        _cqzWorked.insert(items[3],_cqzWorked.value(items[3],0)+1);
                        _cqzbandWorked.insert(items[3]+q.band,_cqzbandWorked.value(items[3]+q.band,0)+1);
                        _cqzmodeWorked.insert(items[3]+q.mode,_cqzmodeWorked.value(items[3]+q.mode,0)+1);
                        _cqzbandmodeWorked.insert(items[3]+q.band+q.mode,_cqzbandmodeWorked.value(items[3]+q.band+q.mode,0)+1);
                        _ituzWorked.insert(items[4],_ituzWorked.value(items[4],0)+1);
                        _ituzbandWorked.insert(items[4]+q.band,_ituzbandWorked.value(items[4]+q.band,0)+1);
                        _ituzmodeWorked.insert(items[4]+q.mode,_ituzmodeWorked.value(items[4]+q.mode,0)+1);
                        _ituzbandmodeWorked.insert(items[4]+q.band+q.mode,_ituzbandmodeWorked.value(items[4]+q.band+q.mode,0)+1);
                        addState (items[1], q.call, q.gridsquare, q.band, q.mode);   /* CE3TSK: Worked All States */
                    }
                    if (q.gridsquare.length() > 3) { // grid exists
                        _gridsWorked.insert(q.gridsquare.left(4).toUpper(),_gridsWorked.value(q.gridsquare.left(4).toUpper(),0)+1);
                        _gridsbandWorked.insert(q.gridsquare.left(4).toUpper()+q.band,_gridsbandWorked.value(q.gridsquare.left(4).toUpper()+q.band,0)+1);
                        _gridsmodeWorked.insert(q.gridsquare.left(4).toUpper()+q.mode,_gridsmodeWorked.value(q.gridsquare.left(4).toUpper()+q.mode,0)+1);
                        _gridsbandmodeWorked.insert(q.gridsquare.left(4).toUpper()+q.band+q.mode,_gridsbandmodeWorked.value(q.gridsquare.left(4).toUpper()+q.band+q.mode,0)+1);
                        /* CE3TSK: WW Digi contest - same QSO indexed by its 2 character field */
                        _fieldsWorked.insert(q.gridsquare.left(2).toUpper(),_fieldsWorked.value(q.gridsquare.left(2).toUpper(),0)+1);
                        _fieldsbandWorked.insert(q.gridsquare.left(2).toUpper()+q.band,_fieldsbandWorked.value(q.gridsquare.left(2).toUpper()+q.band,0)+1);
                        _fieldsmodeWorked.insert(q.gridsquare.left(2).toUpper()+q.mode,_fieldsmodeWorked.value(q.gridsquare.left(2).toUpper()+q.mode,0)+1);
                        _fieldsbandmodeWorked.insert(q.gridsquare.left(2).toUpper()+q.band+q.mode,_fieldsbandmodeWorked.value(q.gridsquare.left(2).toUpper()+q.band+q.mode,0)+1);
                    }
                }
            }
        }
        inputFile.close();
/*        printf ("_cqzWorked %d\n",_cqzWorked.size());
        printf ("_cqzbansWorked %d\n",_cqzbandWorked.size());
        printf ("_cqzmodeWorked %d\n",_cqzmodeWorked.size());
        printf ("_cqzbandmodeWorked %d\n",_cqzbandmodeWorked.size());
        printf ("_ituzWorked %d\n",_ituzWorked.size());
        printf ("_ituzbansWorked %d\n",_ituzbandWorked.size());
        printf ("_ituzmodeWorked %d\n",_ituzmodeWorked.size());
        printf ("_ituzbandmodeWorked %d\n",_ituzbandmodeWorked.size());
        printf ("_countriesWorked %d\n",_countriesWorked.size());
        printf ("_countriesbansWorked %d\n",_countriesbandWorked.size());
        printf ("_countriesmodeWorked %d\n",_countriesmodeWorked.size());
        printf ("_countriesbandmodeWorked %d\n",_countriesbandmodeWorked.size());
        printf ("_gridsWorked %d\n",_gridsWorked.size());
        printf ("_gridsbansWorked %d\n",_gridsbandWorked.size());
        printf ("_gridsmodeWorked %d\n",_gridsmodeWorked.size());
        printf ("_gridsbandmodeWorked %d\n",_gridsbandmodeWorked.size());
        printf ("_pxsWorked %d\n",_pxsWorked.size());
        printf ("_pxsbansWorked %d\n",_pxsbandWorked.size());
        printf ("_pxsmodeWorked %d\n",_pxsmodeWorked.size());
        printf ("_pxsbandmodeWorked %d\n",_pxsbandmodeWorked.size());
        printf ("_callsWorked %d\n",_callsWorked.size());
        printf ("_callsbansWorked %d\n",_callsbandWorked.size());
        printf ("_callsmodeWorked %d\n",_callsmodeWorked.size());
        printf ("_callsbandmodeWorked %d\n",_callsbandmodeWorked.size()); */
    }
}


void ADIF::add(const QString call, const QString band, const QString mode, const QString date, const QString gridsquare, const QString name)
{
    QSO q;
    q.call = call;
    q.band = band;
    /* CE3TSK 2026-10-04 (review): the mode as load () reads it from the file - upper case, JT9/JT65 folded - so a QSO
       logged now counts per mode as it will after a restart (the Log QSO box takes a typed "ft8") */
    q.mode = mode.toUpper ();
    if (q.mode.left (3) == "JT9") q.mode = "JT9";
    else if (q.mode.left (4) == "JT65") q.mode = "JT65";
    q.date = date;
    q.gridsquare = gridsquare;
    q.name = name;
    _data.insert(q.call,q);
    _callsWorked.insert(q.call,_callsWorked.value(q.call,0)+1);
    _callsbandWorked.insert(q.call+q.band,_callsbandWorked.value(q.call+q.band,0)+1);
    _callsmodeWorked.insert(q.call+q.mode,_callsmodeWorked.value(q.call+q.mode,0)+1);
    _callsbandmodeWorked.insert(q.call+q.band+q.mode,_callsbandmodeWorked.value(q.call+q.band+q.mode,0)+1);
    _pxsWorked.insert(Radio::striped_prefix(Radio::effective_prefix(q.call)), _pxsWorked.value(Radio::striped_prefix(Radio::effective_prefix(q.call)),0)+1);
    _pxsbandWorked.insert(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.band, _pxsbandWorked.value(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.band,0)+1);
    _pxsmodeWorked.insert(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.mode, _pxsmodeWorked.value(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.mode,0)+1);
    _pxsbandmodeWorked.insert(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.band+q.mode, _pxsbandmodeWorked.value(Radio::striped_prefix(Radio::effective_prefix(q.call))+q.band+q.mode,0)+1);
    _counts.insert(q.mode,_counts.value(q.mode,0)+1);
    QString country = _countries.find(q.call);
    if (!country.isEmpty ()) {
        QStringList items = country.split(',');
        _countriesWorked.insert(items[0]+','+items[1]+','+items[2], _countriesWorked.value(items[0]+','+items[1]+','+items[2],0)+1);
        _countriesbandWorked.insert(items[0]+','+items[1]+','+items[2]+q.band, _countriesbandWorked.value(items[0]+','+items[1]+','+items[2]+q.band,0)+1);   /* CE3TSK: its own count (review) */
        _countriesmodeWorked.insert(items[0]+','+items[1]+','+items[2]+q.mode, _countriesmodeWorked.value(items[0]+','+items[1]+','+items[2]+q.mode,0)+1);   /* CE3TSK: its own count (review) */
        _countriesbandmodeWorked.insert(items[0]+','+items[1]+','+items[2]+q.band+q.mode, _countriesbandmodeWorked.value(items[0]+','+items[1]+','+items[2]+q.band+q.mode,0)+1);   /* CE3TSK: its own count (review) */
        _cqzWorked.insert(items[3],_cqzWorked.value(items[3],0)+1);
        _cqzbandWorked.insert(items[3]+q.band,_cqzbandWorked.value(items[3]+q.band,0)+1);
        _cqzmodeWorked.insert(items[3]+q.mode,_cqzmodeWorked.value(items[3]+q.mode,0)+1);
        _cqzbandmodeWorked.insert(items[3]+q.band+q.mode,_cqzbandmodeWorked.value(items[3]+q.band+q.mode,0)+1);
        _ituzWorked.insert(items[4],_ituzWorked.value(items[4],0)+1);
        _ituzbandWorked.insert(items[4]+q.band,_ituzbandWorked.value(items[4]+q.band,0)+1);
        _ituzmodeWorked.insert(items[4]+q.mode,_ituzmodeWorked.value(items[4]+q.mode,0)+1);
        _ituzbandmodeWorked.insert(items[4]+q.band+q.mode,_ituzbandmodeWorked.value(items[4]+q.band+q.mode,0)+1);
        addState (items[1], q.call, q.gridsquare, q.band, q.mode);   /* CE3TSK: Worked All States */
    }
    if (q.gridsquare.length() > 3) {
        _gridsWorked.insert(q.gridsquare.left(4).toUpper(),_gridsWorked.value(q.gridsquare.left(4).toUpper(),0)+1);
        _gridsbandWorked.insert(q.gridsquare.left(4).toUpper()+q.band,_gridsbandWorked.value(q.gridsquare.left(4).toUpper()+q.band,0)+1);
        _gridsmodeWorked.insert(q.gridsquare.left(4).toUpper()+q.mode,_gridsmodeWorked.value(q.gridsquare.left(4).toUpper()+q.mode,0)+1);
        _gridsbandmodeWorked.insert(q.gridsquare.left(4).toUpper()+q.band+q.mode,_gridsbandmodeWorked.value(q.gridsquare.left(4).toUpper()+q.band+q.mode,0)+1);
        /* CE3TSK: WW Digi contest - same QSO indexed by its 2 character field */
        _fieldsWorked.insert(q.gridsquare.left(2).toUpper(),_fieldsWorked.value(q.gridsquare.left(2).toUpper(),0)+1);
        _fieldsbandWorked.insert(q.gridsquare.left(2).toUpper()+q.band,_fieldsbandWorked.value(q.gridsquare.left(2).toUpper()+q.band,0)+1);
        _fieldsmodeWorked.insert(q.gridsquare.left(2).toUpper()+q.mode,_fieldsmodeWorked.value(q.gridsquare.left(2).toUpper()+q.mode,0)+1);
        _fieldsbandmodeWorked.insert(q.gridsquare.left(2).toUpper()+q.band+q.mode,_fieldsbandmodeWorked.value(q.gridsquare.left(2).toUpper()+q.band+q.mode,0)+1);
    }        
    //qDebug() << "Added as worked:" << call << band << mode << date;
}

// return true if in the log same band and mode
bool ADIF::match(const QString call, const QString band, const QString mode)
{

    return workedIn (_callsWorked, _callsmodeWorked, _callsbandWorked, _callsbandmodeWorked, call, band, mode);   /* CE3TSK */
}    

// return true if in the log same band and mode
bool ADIF::matchPx(const QString call, const QString band, const QString mode)
{
    
    return workedIn (_pxsWorked, _pxsmodeWorked, _pxsbandWorked, _pxsbandmodeWorked,   /* CE3TSK */
                     Radio::striped_prefix (Radio::effective_prefix (call)), band, mode);
}    

// return true if in the log same band and mode
bool ADIF::getData(const QString call, QString &gridsquare, QString &name)
{
    
    bool setgrid = call.endsWith("/MM") || call.endsWith("/M") || call.endsWith("/AM") || call.endsWith("/P") || call.endsWith("/A");
    QList<QSO> qsos = _data.values(call);
    if (qsos.size()>0)
    {
        QSO q;
        foreach(q,qsos)
        {
            if (     (gridsquare.isEmpty() && !q.gridsquare.isEmpty())
                  || (gridsquare.left(4) == q.gridsquare.left(4) && q.gridsquare.length() > gridsquare.length()))
            {
                if (!setgrid ) gridsquare = q.gridsquare;
                name = q.name;
            }
            if (     (name.isEmpty() && !q.name.isEmpty())
                  || (name.compare(q.name,Qt::CaseInsensitive) == 0))
            {
                name = q.name;
            }
            
        }
        return true;
    }
    return false;
}    

// return true if in the log same band and mode
bool ADIF::matchCqz(const QString Cqz, const QString band, const QString mode)
{
    return workedIn (_cqzWorked, _cqzmodeWorked, _cqzbandWorked, _cqzbandmodeWorked, Cqz, band, mode);   /* CE3TSK */
}    

// return true if in the log same band and mode
bool ADIF::matchItuz(const QString Ituz, const QString band, const QString mode)
{
    return workedIn (_ituzWorked, _ituzmodeWorked, _ituzbandWorked, _ituzbandmodeWorked, Ituz, band, mode);   /* CE3TSK */
}    

// return true if in the log same band and mode
bool ADIF::matchCountry(const QString countryName, const QString band, const QString mode)
{
    return workedIn (_countriesWorked, _countriesmodeWorked, _countriesbandWorked, _countriesbandmodeWorked,   /* CE3TSK */
                     countryName, band, mode);
}    

// return true if in the log same band and mode
bool ADIF::matchGrid(const QString gridsquare, const QString band, const QString mode)
{
    return workedIn (_gridsWorked, _gridsmodeWorked, _gridsbandWorked, _gridsbandmodeWorked, gridsquare, band, mode);   /* CE3TSK */
}

/* CE3TSK 2026-10-03: Worked All States - see adif.h */
void ADIF::addState (QString const& masterPrefix, QString const& call, QString const& gridsquare, QString const& band, QString const& mode)
{
    QString const state = us_states::clearState (masterPrefix, gridsquare, _countries.licenseState (call));
    if (state.isEmpty ()) return;
    _statesWorked.insert (state, _statesWorked.value (state, 0) + 1);
    _statesbandWorked.insert (state + band, _statesbandWorked.value (state + band, 0) + 1);
    _statesmodeWorked.insert (state + mode, _statesmodeWorked.value (state + mode, 0) + 1);   // CE3TSK 2026-10-04
    _statesbandmodeWorked.insert (state + band + mode, _statesbandmodeWorked.value (state + band + mode, 0) + 1);
}

/* CE3TSK 2026-10-04: at all, on a band, in a mode, or on a band in a mode - as matchCountry asks the DXCC tier */
bool ADIF::matchState (QString const& state, QString const& band, QString const& mode) const
{
    return workedIn (_statesWorked, _statesmodeWorked, _statesbandWorked, _statesbandmodeWorked, state, band, mode);
}

/* CE3TSK: WW Digi contest - worked before test on the 2 character Maidenhead field */
bool ADIF::matchField(const QString field, const QString band, const QString mode)
{
    return workedIn (_fieldsWorked, _fieldsmodeWorked, _fieldsbandWorked, _fieldsbandmodeWorked, field, band, mode);   /* CE3TSK */
}

QList<QString> ADIF::getCallList()
{
    QList<QString> p;
    QMultiHash<QString,QSO>::const_iterator i = _data.constBegin();
     while (i != _data.constEnd())
     {
         p << i.key();
         ++i;
     }
    return p;
}   
    
int ADIF::getCount(const QString mode)
{
    if (mode.isEmpty ()){
        return _data.size();
    } else {
        return _counts.value(mode,0);
    }
}   
    

// open ADIF file and append the QSO details. Return true on success
bool ADIF::addQSOToFile(const QString hisCall, const QString hisGrid, const QString mode, const QString rptSent, const QString rptRcvd, QDateTime const& dateTimeOn, QDateTime const& dateTimeOff, const QString band,
                        const QString comments, const QString name, const QString strDialFreq, const QString m_myCall, const QString m_myGrid, const QString m_txPower,const bool send_to_eqsl)
{
    QFile f2(_filename);
    if (!f2.open(QIODevice::Text | QIODevice::Append))
        return false;
    else
    {
        QTextStream out(&f2);
        if (f2.size()==0)
            out << "JTDX_contest ADIF Export<eoh>" <<   // CE3TSK: the fork names itself in a new log file (the literal, not fork_name(): the log leaves the machine)
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif
  // new file

        QString t;
        t="<call:" + QString::number(hisCall.length()) + ">" + hisCall;
        t+=" <gridsquare:" + QString::number(hisGrid.length()) + ">" + hisGrid;
        if (mode == "FT4" || mode == "FT2") t+=" <mode:4>MFSK <submode:" + QString::number(mode.length()) + ">" + mode;   // CE3TSK: FT2 too
        else t+=" <mode:" + QString::number(mode.length()) + ">" + mode;
        t+=" <rst_sent:" + QString::number(rptSent.length()) + ">" + rptSent;
        t+=" <rst_rcvd:" + QString::number(rptRcvd.length()) + ">" + rptRcvd;
        t+=" <qso_date:8>" + dateTimeOn.date ().toString ("yyyyMMdd");
        t+=" <time_on:6>" + dateTimeOn.time ().toString ("hhmmss");
        t+=" <qso_date_off:8>" + dateTimeOff.date ().toString ("yyyyMMdd");
        t+=" <time_off:6>" + dateTimeOff.time ().toString ("hhmmss");
        t+=" <band:" + QString::number(band.length()) + ">" + band;
        t+=" <freq:" + QString::number(strDialFreq.length()) + ">" + strDialFreq;
        t+=" <station_callsign:" + QString::number(m_myCall.length()) + ">" +
                m_myCall;
        t+=" <my_gridsquare:" + QString::number(m_myGrid.length()) + ">" +
                m_myGrid;
        if(!m_txPower.isEmpty ()) t+= " <tx_pwr:" + QString::number(m_txPower.length()) +
                ">" + m_txPower;
        if(!comments.isEmpty ()) t+=" <comment:" + QString::number(comments.length()) +
                ">" + comments;
        if(!name.isEmpty ()) t+=" <name:" + QString::number(name.length()) +
                ">" + name;
        if (send_to_eqsl) t+=" <eqsl_qsl_sent:1>Y";
        t+=" <eor>";
        out << t <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

        f2.close();
    }
    return true;
}

/* CE3TSK: the gridsquare of every QSO in this log, for contest points. Empty entries are
   kept - a QSO without a grid scores nothing, and grid_distance_km() returns -1 for it. */
QList<QString> ADIF::gridList () const
{
    QList<QString> grids;
    grids.reserve (_data.size ());
    for (auto it = _data.constBegin (); it != _data.constEnd (); ++it) grids << it.value ().gridsquare;
    return grids;
}

QString ADIF::bandFromFrequency(double dialFreq)
{
    QString band="";
    if(dialFreq>0.135 and dialFreq<0.139) band="2200m";
    else if(dialFreq>0.45 and dialFreq<0.55) band="630m";
    else if(dialFreq>1.8 and dialFreq<2.0) band="160m";
    else if(dialFreq>3.5 and dialFreq<4.0) band="80m";
    else if(dialFreq>5.1 and dialFreq<5.45) band="60m";
    else if(dialFreq>7.0 and dialFreq<7.3) band="40m";
    else if(dialFreq>10.0 and dialFreq<10.15) band="30m";
    else if(dialFreq>14.0 and dialFreq<14.35) band="20m";
    else if(dialFreq>18.068 and dialFreq<18.168) band="17m";
    else if(dialFreq>21.0 and dialFreq<21.45) band="15m";
    else if(dialFreq>24.890 and dialFreq<24.990) band="12m";
    else if(dialFreq>28.0 and dialFreq<29.7) band="10m";
    else if(dialFreq>50.0 and dialFreq<54.0) band="6m";
    else if(dialFreq>70.0 and dialFreq<71.0) band="4m";
    else if(dialFreq>144.0 and dialFreq<148.0) band="2m";
    else if(dialFreq>222.0 and dialFreq<225.0) band="1.25m";
    else if(dialFreq>420.0 and dialFreq<450.0) band="70cm";
    else if(dialFreq>902.0 and dialFreq<928.0) band="33cm";
    else if(dialFreq>1240.0 and dialFreq<1300.0) band="23cm";
    else if(dialFreq>2300.0 and dialFreq<2450.0) band="13cm";
    else if(dialFreq>3300.0 and dialFreq<3500.0) band="9cm";
    else if(dialFreq>5650.0 and dialFreq<5925.0) band="6cm";
    else if(dialFreq>10000.0 and dialFreq<10500.0) band="3cm";
    else if(dialFreq>24000.0 and dialFreq<24250.0) band="1.25cm";
    else if(dialFreq>47000.0 and dialFreq<47200.0) band="6mm";
    else if(dialFreq>75500.0 and dialFreq<81000.0) band="4mm";
    return band;
}
