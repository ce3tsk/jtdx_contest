// -*- Mode: C++ -*-
/* CE3TSK 2026-11-05 (TODO.md 3.-235): File > Export Cabrillo log...: a contest's log as the file its sponsors ask for. The rules are in contestcabrillo.h;
   this is the window over them. It reads the contest's ADIF and writes one file and its own settings, nothing else. Not modal: Halt Tx, Stop and Enable Tx stay
   within reach, as the sequencer runs on behind it. Also with contest mode off, because the logs are due after the contest.

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef EXPORTCABRILLO_H
#define EXPORTCABRILLO_H

#include <QByteArray>
#include <QDialog>
#include <QDir>
#include <QList>
#include <QString>
#include "Configuration.hpp"
#include "contestcabrillo.h"

class CountryDat;
class QComboBox;
class QDialogButtonBox;
class QEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSettings;

class ExportCabrillo final
  : public QDialog
{
  Q_OBJECT

public:
  ExportCabrillo (QSettings *, Configuration const *, CountryDat *, QWidget * parent = nullptr);
  ~ExportCabrillo ();

  QString fileText () const;   // what Save writes
  QString summary () const;    // what is saved, what is left out and why
  QString suggestedName () const;
  bool saveTo (QString const& path);   // whole or not at all; says how it went in the summary

protected:
  bool event (QEvent *) override;

private:
  struct LogFile
  {
    Configuration::SpecialOperatingActivity activity;
    QString slug, path;
    int year;
    QByteArray bytes;
    cabrillo::Log log;
    QString readError;
  };

  void findLogs ();
  void readLog (LogFile&) const;
  void reload ();
  QString logLabel (LogFile const&) const;
  QString defaultLocation (LogFile const&) const;
  void showLog (int index);
  void showContestFields (LogFile const&);
  void storeContestFields () const;
  void storePersonFields () const;
  void updateSelection ();
  void updateFile ();
  void edited ();
  void save ();

  QSettings * settings_;
  Configuration const * configuration_;
  CountryDat * countries_;
  QDir dataDir_;
  QList<LogFile> logs_;
  int shown_ {-1};
  bool loading_ {false};
  bool noClaim_ {false};
  QString previousCall_;
  QString note_;
  cabrillo::Selection selection_;

  QComboBox * logBox_;
  QLineEdit * contest_;
  QComboBox * call_;
  QLineEdit * location_;
  QComboBox * categoryOperator_;
  QComboBox * categoryAssisted_;
  QComboBox * categoryBand_;
  QComboBox * categoryPower_;
  QComboBox * categoryTransmitter_;
  QComboBox * categoryStation_;
  QLineEdit * claimed_;
  QLineEdit * operators_;
  QLineEdit * name_;
  QLineEdit * address1_;
  QLineEdit * address2_;
  QLineEdit * email_;
  QLineEdit * club_;
  QLineEdit * soapbox_;
  QLabel * summary_;
  QPlainTextEdit * preview_;
  QDialogButtonBox * buttons_;
};

#endif
