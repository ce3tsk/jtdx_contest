#include "exportcabrillo.h"

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <algorithm>

#include "contestpoints.h"
#include "logbook/countrydat.h"
#include "revision_utils.hpp"
#include "usstates.h"

#include "moc_exportcabrillo.cpp"

namespace
{
  QString const GROUP {"CabrilloExport/"};

  // the list, and the saved value when the list has it
  void fill (QComboBox * box, QStringList const& values, QString const& wanted)
  {
    box->clear ();
    box->addItems (values);
    int const at = box->findText (wanted);
    box->setCurrentIndex (at < 0 ? 0 : at);
  }
}

ExportCabrillo::ExportCabrillo (QSettings * settings, Configuration const * configuration, CountryDat * countries, QWidget * parent)
  : QDialog {parent}
  , settings_ {settings}
  , configuration_ {configuration}
  , countries_ {countries}
  , dataDir_ {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}
{
  setWindowTitle (QApplication::applicationName () + " - " + tr ("Export Cabrillo log"));

  auto box = [this] (char const* name) {
    auto b = new QComboBox {this};
    b->setObjectName (name);
    connect (b, static_cast<void (QComboBox::*) (int)> (&QComboBox::currentIndexChanged), this, &ExportCabrillo::edited);
    return b;
  };
  auto line = [this] (char const* name, int longest) {
    auto l = new QLineEdit {this};
    l->setObjectName (name);
    if (longest) l->setMaxLength (longest);
    // the floor that gives the window its width: without it the layout crushes the text boxes to nothing
    l->setMinimumWidth (l->fontMetrics ().horizontalAdvance (QString {28, '0'}));
    connect (l, &QLineEdit::textChanged, this, &ExportCabrillo::edited);
    return l;
  };

  logBox_ = new QComboBox {this};
  logBox_->setObjectName ("log");
  contest_ = line ("CONTEST", 32);
  call_ = new QComboBox {this};
  call_->setObjectName ("CALLSIGN");
  location_ = line ("LOCATION", 0);
  categoryOperator_ = box ("CATEGORY-OPERATOR");
  categoryAssisted_ = box ("CATEGORY-ASSISTED");
  categoryBand_ = box ("CATEGORY-BAND");
  categoryPower_ = box ("CATEGORY-POWER");
  categoryTransmitter_ = box ("CATEGORY-TRANSMITTER");
  categoryStation_ = box ("CATEGORY-STATION");
  categoryOverlay_ = box ("CATEGORY-OVERLAY");   // CE3TSK 2026-11-07: the FT Challenge's Classic overlay; greyed in a contest with none
  claimed_ = line ("CLAIMED-SCORE", 0);
  claimed_->setValidator (new QRegularExpressionValidator {QRegularExpression {"[0-9]*"}, claimed_});
  operators_ = line ("OPERATORS", 0);
  // the spec's limits on a line: NAME 75, ADDRESS 45, SOAPBOX 75 with its tag
  name_ = line ("NAME", 75);
  address1_ = line ("ADDRESS1", 45);
  address2_ = line ("ADDRESS2", 45);
  email_ = line ("EMAIL", 0);
  club_ = line ("CLUB", 0);
  soapbox_ = line ("SOAPBOX", 66);

  auto top = new QFormLayout;
  top->addRow (tr ("Contest log:"), logBox_);

  // the tags as the sponsors spell them, in no language but theirs
  auto fields = new QGridLayout;
  auto place = [this, fields] (int row, int column, char const* tag, QWidget * widget) {
    fields->addWidget (new QLabel {QString {tag} + ':', this}, row, column * 2);
    fields->addWidget (widget, row, column * 2 + 1);
  };
  place (0, 0, "CONTEST", contest_);
  place (1, 0, "CALLSIGN", call_);
  place (2, 0, "LOCATION", location_);
  place (3, 0, "CATEGORY-OPERATOR", categoryOperator_);
  place (4, 0, "CATEGORY-ASSISTED", categoryAssisted_);
  place (5, 0, "CATEGORY-BAND", categoryBand_);
  place (6, 0, "CATEGORY-POWER", categoryPower_);
  place (7, 0, "CATEGORY-TRANSMITTER", categoryTransmitter_);
  place (8, 0, "CATEGORY-STATION", categoryStation_);
  place (9, 0, "CATEGORY-OVERLAY", categoryOverlay_);
  place (0, 1, "CLAIMED-SCORE", claimed_);
  place (1, 1, "OPERATORS", operators_);
  place (2, 1, "NAME", name_);
  place (3, 1, "ADDRESS", address1_);
  place (4, 1, "ADDRESS", address2_);
  place (5, 1, "EMAIL", email_);
  place (6, 1, "CLUB", club_);
  place (7, 1, "SOAPBOX", soapbox_);
  fields->setColumnStretch (1, 1);
  fields->setColumnStretch (3, 2);

  summary_ = new QLabel {this};
  summary_->setObjectName ("summary");
  summary_->setWordWrap (true);
  summary_->setTextInteractionFlags (Qt::TextSelectableByMouse);
  preview_ = new QPlainTextEdit {this};
  preview_->setObjectName ("preview");
  preview_->setReadOnly (true);
  preview_->setLineWrapMode (QPlainTextEdit::NoWrap);
  preview_->setFont (QFontDatabase::systemFont (QFontDatabase::FixedFont));
  preview_->setMinimumHeight (120);
  buttons_ = new QDialogButtonBox {QDialogButtonBox::Save | QDialogButtonBox::Close, this};
  buttons_->setObjectName ("buttons");
  connect (buttons_, &QDialogButtonBox::accepted, this, &ExportCabrillo::save);
  connect (buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);

  auto layout = new QVBoxLayout {this};
  layout->addLayout (top);
  layout->addLayout (fields);
  layout->addWidget (summary_);
  layout->addWidget (preview_, 1);
  layout->addWidget (buttons_);

  loading_ = true;
  name_->setText (settings_->value (GROUP + "Name").toString ());
  address1_->setText (settings_->value (GROUP + "Address1").toString ());
  address2_->setText (settings_->value (GROUP + "Address2").toString ());
  email_->setText (settings_->value (GROUP + "Email").toString ());
  club_->setText (settings_->value (GROUP + "Club").toString ());
  operators_->setText (settings_->value (GROUP + "Operators").toString ());
  loading_ = false;

  findLogs ();
  for (auto const& f : logs_) logBox_->addItem (logLabel (f));
  connect (logBox_, static_cast<void (QComboBox::*) (int)> (&QComboBox::currentIndexChanged), this, &ExportCabrillo::showLog);
  connect (call_, static_cast<void (QComboBox::*) (int)> (&QComboBox::currentIndexChanged), this, [this] {
    if (loading_) return;
    if (operators_->text ().isEmpty () || operators_->text () == previousCall_) operators_->setText (call_->currentText ());
    previousCall_ = call_->currentText ();
    note_.clear ();
    updateSelection ();
  });
  if (logs_.isEmpty ())
    {
      summary_->setText (summary ());
      buttons_->button (QDialogButtonBox::Save)->setEnabled (false);
    }
  else
    {
      restoreGeometry (settings_->value (GROUP + "geometry").toByteArray ());
      showLog (0);
    }
}

ExportCabrillo::~ExportCabrillo ()
{
  if (shown_ < 0) return;   // no log, nothing shown: a window that never opened has nothing to keep
  storeContestFields ();
  storePersonFields ();
  settings_->setValue (GROUP + "geometry", saveGeometry ());
}

// not modal, so a QSO can be logged while it is open: it reads again when it comes back to the front
bool ExportCabrillo::event (QEvent * e)
{
  if (QEvent::WindowActivate == e->type ()) reload ();
  return QDialog::event (e);
}

void ExportCabrillo::readLog (LogFile& f) const
{
  QFile file {f.path};
  if (file.open (QIODevice::ReadOnly))
    {
      f.readError.clear ();
      f.bytes = file.readAll ();
      f.log = cabrillo::read (QString::fromUtf8 (f.bytes));
    }
  else
    {
      f.readError = file.errorString ();
      f.log = cabrillo::Log {};
    }
}

QString ExportCabrillo::logLabel (LogFile const& f) const
{
  return QString {"%1 %2 (%3 QSO)"}.arg (contest_cabrillo (f.activity).name).arg (f.period).arg (f.log.records.size ());
}

// every contest's logs here, the file of the contest running first, then the newest
void ExportCabrillo::findLogs ()
{
  QString const running = contest_log_filename (configuration_->special_op_id (), QDateTime::currentDateTimeUtc ());
  for (auto const activity : contests_offered ())
    {
      QString const slug = contest_log_slug (activity);
      for (auto const& name : dataDir_.entryList (QStringList {"wsjtx_" + slug + "-*_log.adi"}, QDir::Files))
        {
          QString const period = cabrillo::log_period (name, slug);   // (2026-11-13) "2026-11", or a year-named log's "2026"
          if (period.isEmpty ()) continue;
          LogFile f {activity, slug, dataDir_.absoluteFilePath (name), period, {}, {}, {}};
          readLog (f);
          logs_ << f;
        }
    }
  std::stable_sort (logs_.begin (), logs_.end (), [&running] (LogFile const& a, LogFile const& b) {
    bool const aRuns = QFileInfo {a.path}.fileName () == running;
    bool const bRuns = QFileInfo {b.path}.fileName () == running;
    if (aRuns != bRuns) return aRuns;
    return QFileInfo {a.path}.lastModified () > QFileInfo {b.path}.lastModified ();
  });
}

void ExportCabrillo::reload ()
{
  if (shown_ < 0) return;
  LogFile& f = logs_[shown_];
  QByteArray const before = f.bytes;
  readLog (f);
  if (f.bytes == before) return;   // what is typed, the claim too, stays until the log has changed
  logBox_->setItemText (shown_, logLabel (f));
  loading_ = true;
  QString const wanted = call_->currentText ();
  call_->clear ();
  call_->addItems (cabrillo::own_calls (f.log.records));
  call_->setCurrentIndex (qMax (0, call_->findText (wanted)));
  loading_ = false;
  updateSelection ();
}

/* the location each contest starts with: YBDXPI's guide asks the grid; the USA and Canada must give their section (WW Digi X.3, and the spec for all ARRL contests), so theirs
   starts empty and Save waits for it; everyone else is DX. */
QString ExportCabrillo::defaultLocation (LogFile const& f) const
{
  if (contest_cabrillo (f.activity).locationIsGrid) return cabrillo::own_grid (f.log.records);
  QString const prefix = countries_->find (call_->currentText ()).section (',', 1, 1).trimmed ();
  return us_states::usEntity (prefix) || "VE" == prefix ? QString {} : QString {"DX"};
}

void ExportCabrillo::showContestFields (LogFile const& f)
{
  auto const c = contest_cabrillo (f.activity);
  auto saved = [this, &f] (char const* key, QString const& otherwise = QString {}) {return settings_->value (GROUP + f.slug + '/' + key, otherwise).toString ();};
  contest_->setText (saved ("Contest", c.name));
  location_->setText (saved ("Location", defaultLocation (f)));
  fill (categoryOperator_, QStringList {"SINGLE-OP", "MULTI-OP", "CHECKLOG"}, saved ("Operator"));
  fill (categoryAssisted_, QStringList {"", "ASSISTED", "NON-ASSISTED"}, saved ("Assisted", c.assisted));
  fill (categoryBand_, c.bands, saved ("Band"));
  fill (categoryPower_, c.powers, saved ("Power"));
  fill (categoryTransmitter_, c.transmitters, saved ("Transmitter"));
  fill (categoryStation_, QStringList {"FIXED", "MOBILE", "PORTABLE", "EXPEDITION"}, saved ("Station"));
  fill (categoryOverlay_, c.overlays.isEmpty () ? QStringList {""} : c.overlays, saved ("Overlay"));
  categoryOverlay_->setEnabled (!c.overlays.isEmpty ());
  soapbox_->setText (saved ("Soapbox"));
}

void ExportCabrillo::storeContestFields () const
{
  if (shown_ < 0) return;
  QString const at = GROUP + logs_[shown_].slug + '/';
  settings_->setValue (at + "Contest", contest_->text ());
  settings_->setValue (at + "Location", location_->text ());
  settings_->setValue (at + "Operator", categoryOperator_->currentText ());
  settings_->setValue (at + "Assisted", categoryAssisted_->currentText ());
  settings_->setValue (at + "Band", categoryBand_->currentText ());
  settings_->setValue (at + "Power", categoryPower_->currentText ());
  settings_->setValue (at + "Transmitter", categoryTransmitter_->currentText ());
  settings_->setValue (at + "Station", categoryStation_->currentText ());
  settings_->setValue (at + "Overlay", categoryOverlay_->currentText ());
  settings_->setValue (at + "Soapbox", soapbox_->text ());
}

void ExportCabrillo::storePersonFields () const
{
  // the call itself is what OPERATORS starts as, for whichever call a log has: only other names are worth keeping
  settings_->setValue (GROUP + "Operators", operators_->text () == call_->currentText () ? QString {} : operators_->text ());
  settings_->setValue (GROUP + "Name", name_->text ());
  settings_->setValue (GROUP + "Address1", address1_->text ());
  settings_->setValue (GROUP + "Address2", address2_->text ());
  settings_->setValue (GROUP + "Email", email_->text ());
  settings_->setValue (GROUP + "Club", club_->text ());
}

void ExportCabrillo::showLog (int index)
{
  if (index < 0 || index >= logs_.size ()) return;
  storeContestFields ();
  shown_ = index;
  LogFile const& f = logs_[index];
  bool const operatorsFollow = operators_->text ().isEmpty () || operators_->text () == previousCall_;
  loading_ = true;
  call_->clear ();
  call_->addItems (cabrillo::own_calls (f.log.records));
  int const mine = call_->findText (configuration_->my_callsign ().toUpper ());   // the station's own, when this log has it; else the one with the most QSOs
  if (mine >= 0) call_->setCurrentIndex (mine);
  showContestFields (f);
  previousCall_ = call_->currentText ();
  if (operatorsFollow) operators_->setText (previousCall_);
  loading_ = false;
  note_.clear ();
  updateSelection ();
}

void ExportCabrillo::updateSelection ()
{
  if (loading_ || shown_ < 0) return;
  LogFile const& f = logs_[shown_];
  selection_ = cabrillo::select (f.log.records, call_->currentText ());
  auto const rules = contest_rules_of (f.activity);
  noClaim_ = rules.prefixCountryMultipliers && !countries_->loaded ();   // the points come from the country, and in WSPR the program has not read cty.dat: no claim rather than 0
  auto const score = contest_log_score (rules, f.activity, cabrillo::score_qsos (selection_.qsos), *countries_, call_->currentText (), cabrillo::own_grid (selection_.qsos));
  loading_ = true;
  claimed_->setText (selection_.qsos.isEmpty () || noClaim_ ? QString {} : QString::number (score.total ()));
  loading_ = false;
  updateFile ();
}

void ExportCabrillo::edited ()
{
  if (loading_) return;
  note_.clear ();
  updateFile ();
}

void ExportCabrillo::updateFile ()
{
  if (loading_ || shown_ < 0) return;
  summary_->setText (summary ());
  preview_->setPlainText (fileText ());
  buttons_->button (QDialogButtonBox::Save)->setEnabled (!selection_.qsos.isEmpty () && !location_->text ().trimmed ().isEmpty ());
}

QString ExportCabrillo::summary () const
{
  if (shown_ < 0) return tr ("No contest log in %1.").arg (dataDir_.absolutePath ());
  LogFile const& f = logs_[shown_];
  if (!f.readError.isEmpty ()) return tr ("Cannot read \"%1\": %2").arg (f.path, f.readError);
  QStringList lines;
  if (!note_.isEmpty ()) lines << note_;
  lines << tr ("%1 QSOs to save.").arg (selection_.qsos.size ());
  if (location_->text ().trimmed ().isEmpty ()) lines << tr ("LOCATION: give your ARRL or RAC section.");
  if (noClaim_) lines << tr ("No claimed score: the country data is not loaded.");
  auto add = [&lines] (QString const& text, QList<int> const& where) {
    if (!where.isEmpty ()) lines << text.arg (where.size ()).arg (cabrillo::line_list (where));
  };
  add (tr ("No grid, saved with ZZ00: %1 (log line %2)."), selection_.noGrid);
  if (contest_cabrillo (f.activity).reportInExchange) add (tr ("No report, saved with +99: %1 (log line %2)."), selection_.noReport);   // CE3TSK 2026-11-07
  add (tr ("Left out, damaged: %1 (log line %2)."), f.log.damaged);
  add (tr ("Left out, another own call: %1 (log line %2)."), selection_.otherCall);
  add (tr ("Left out, the same QSO logged twice: %1 (log line %2)."), selection_.twice);
  return lines.join ('\n');
}

QString ExportCabrillo::fileText () const
{
  if (shown_ < 0) return QString {};
  cabrillo::Header h;
  h.contest = contest_->text ();
  h.call = call_->currentText ();
  h.location = location_->text ();
  h.categoryOperator = categoryOperator_->currentText ();
  h.categoryAssisted = categoryAssisted_->currentText ();
  h.categoryBand = categoryBand_->currentText ();
  h.categoryPower = categoryPower_->currentText ();
  h.categoryTransmitter = categoryTransmitter_->currentText ();
  h.categoryStation = categoryStation_->currentText ();
  h.categoryOverlay = categoryOverlay_->currentText ();
  h.claimedScore = claimed_->text ();
  h.gridLocator = cabrillo::own_grid (selection_.qsos);
  h.operators = operators_->text ();
  h.name = name_->text ();
  h.address1 = address1_->text ();
  h.address2 = address2_->text ();
  h.email = email_->text ();
  h.club = club_->text ();
  h.soapbox = soapbox_->text ();
  h.createdBy = "JTDX_contest v" + version ();   // the literal, as the ADIF header: this file leaves the machine
  return cabrillo::document (h, selection_.qsos, contest_cabrillo (logs_[shown_].activity));
}

QString ExportCabrillo::suggestedName () const
{
  if (shown_ < 0) return QString {};
  LogFile const& f = logs_[shown_];
  return cabrillo::file_name (QFileInfo {f.path}.fileName ());
}

bool ExportCabrillo::saveTo (QString const& path)
{
  reload ();
  QSaveFile file {path};
  QByteArray const bytes = fileText ().toUtf8 ();
  bool const ok = file.open (QIODevice::WriteOnly) && file.write (bytes) == bytes.size () && file.commit ();
  note_ = ok ? tr ("Cabrillo log saved: %1").arg (path) : tr ("Cannot write \"%1\": %2").arg (path, file.errorString ());
  if (ok) settings_->setValue (GROUP + "Directory", QFileInfo {path}.absolutePath ());
  summary_->setText (summary ());
  return ok;
}

void ExportCabrillo::save ()
{
  QDir const start {settings_->value (GROUP + "Directory", dataDir_.absolutePath ()).toString ()};
  QFileDialog dialog {this, tr ("Save the Cabrillo log"), start.absoluteFilePath (suggestedName ()), tr ("Cabrillo log (*.cbr *.log)")};
  dialog.setAcceptMode (QFileDialog::AcceptSave);
  dialog.setOption (QFileDialog::DontUseNativeDialog);   // the desktop's own, window modal, keeps the keyboard in its search box: Return would not save
  dialog.setDefaultSuffix (QFileInfo {suggestedName ()}.suffix ());
  dialog.setWindowModality (Qt::WindowModal);   // this window alone: Halt Tx and Stop stay within reach
  if (QDialog::Accepted == dialog.exec () && !dialog.selectedFiles ().isEmpty ()) saveTo (dialog.selectedFiles ().first ());
}
