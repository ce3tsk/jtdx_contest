// This source code file was last time modified by Arvo ES1JA on January 5th, 2019
// All changes are shown in the patch file coming together with the full JTDX source code.

/*
 * Reads cty.dat file
 * Establishes a map between prefixes and their country names
 * VK3ACF July 2013
 */


#ifndef __COUNTRYDAT_H
#define __COUNTRYDAT_H

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QDate>
#include <QByteArray>
#include <QDir>

class CountryDat
{
public:
  /* CE3TSK: the country names follow the UI language only when asked to (Settings > General >
     Translate DXCC names); otherwise the English source text is used in every language. Still
     spelled tr() so lupdate keeps collecting every name into the CountryDat context. */
  inline QString tr(const char *sourceText, const char *disambiguation = Q_NULLPTR, int n = -1) const \
        { return _translated ? QCoreApplication::translate("CountryDat", sourceText, disambiguation, n) : QString::fromUtf8(sourceText); }
  void init(const QString filename,const QString filename2,bool translated = false);
  void load();
  QString find(const QString prefix); // return country name or ""
  QString find2(const QString call); // return lotw date or ""

  /* CE3TSK: the version a cty.dat or LoTW user activity file carries - cty.dat's =VERyyyymmdd
     entry, the newest upload date in the LoTW file - or an invalid date when the content is not
     such a file. Used to refuse a download that is not the file asked for, and to compare a
     downloaded copy with the one bundled with this release. */
  static QDate ctyVersion (QByteArray const& content);
  static QDate lotwVersion (QByteArray const& content);
  /* CE3TSK: the copy of fileName to read. The one in dataDir wins, as it always has, unless both
     copies carry a version and the bundled one (":/fileName") is newer - so an installer shipping
     newer data is never shadowed by an old download. version receives the chosen copy's. */
  static QString fileToUse (QDir const& dataDir, QString const& fileName,
                            QDate (* versionOf) (QByteArray const&), QDate * version = nullptr);
   
private:
  QString _extractName(const QString line);
  QString _extractMasterPrefix(const QString line);
  QString _extractContinent(const QString line);
  QString _extractCQZ(const QString line);
  QString _extractITUZ(const QString line);
  QString _removeBrackets(QString &line, const QString a, const QString b);
  QStringList _extractPrefix(QString &line, bool &more);

  QString _filename,_filename2;
  QHash<QString, QString> _data;
  QHash<QString, QString> _data2;
  QHash<QString, QString> _name;
  bool _translated = false;   /* CE3TSK: see tr() */
};

#endif
