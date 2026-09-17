//-------------------------------------------------------- MainWindow

#include "mainwindow.h"
#include <cinttypes>
#include <limits>
#include <fftw3.h>
#include <thread>

#include <QProcessEnvironment>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QDesktopServices>
#include <QToolButton>   // CE3TSK: the Ko-fi button in the menu bar corner
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QtConcurrent/QtConcurrentRun>
#include <QProgressDialog>
#include <QHostInfo>
#include <QVector>
#include <QCursor>
#include <QToolTip>
#include <QTextDocument>   /* CE3TSK: measuring the header line */
#include <QButtonGroup>
#include <QUdpSocket>
#include <QtMath>
#if QT_VERSION >= QT_VERSION_CHECK (5, 15, 0)
#include <QRandomGenerator>
#endif

#include "revision_utils.hpp"
#include "qt_helpers.hpp"
#include "tooltip_wrap.hpp"   /* CE3TSK */
#include "soundout.h"
#include "soundin.h"
#include "Modulator.hpp"
#include "Detector.hpp"
#include "plotter.h"
#include "about.h"
#include "widegraph.h"
#include "sleep.h"
#include "logqso.h"
#include "wav16to32.h"
#include "decoderange.h"
#include "contestreport.h"
#include "contestreply.h"
#include "logfields.h"   // CE3TSK: what a log entry takes when the QSO skipped a step   /* CE3TSK: contest-mode rejection of signal-report messages */
#include "decodelabel.h"   // CE3TSK
#include "modetiming.h"   // CE3TSK: the per-mode timing constants
#include <QPainter>
#include <functional>   // CE3TSK P13: the recursive menu hook
#include <QThread>                  /* CE3TSK: 16 bit wav expansion */
#include "decodedtext.h"
#include "Radio.hpp"
#include "Bands.hpp"
#include "TransceiverFactory.hpp"
#include "FrequencyList.hpp"
#include "StationList.hpp"
#include "LiveFrequencyValidator.hpp"
#include "MessageClient.hpp"
#include "wsprnet.h"
#include "eqsl.h"
#include "signalmeter.h"
#include "HelpTextWindow.hpp"
#include "Audio/BWFFile.hpp"

#include "ui_mainwindow.h"
#include "moc_mainwindow.cpp"



extern "C" {
  //----------------------------------------------------- C and Fortran routines
  void symspec_(struct dec_data *, int* k, int* ntrperiod, int* nsps,
                float* px, float s[], float* df3, int* nhsym, int* npts8);

  void four2a_(_Complex float *, int * nfft, int * ndim, int * isign, int * iform, fortran_charlen_t);
				
  void genft8_(char* msg, int* i3, int* n3, int* ntxhash, char* msgsent, char ft8msgbits[], int itone[], fortran_charlen_t, fortran_charlen_t);

  void genft4_(char* msg, int* ichk, int* ntxhash, char* msgsent, char ft4msgbits[], int itone[], fortran_charlen_t, fortran_charlen_t);

  void gen_ft8wave_(int itone[], int* nsym, int* nsps, float* bt, float* fsample, float* f0, float xjunk[], float wave[], int* icmplx, int* nwave);

  void gen_ft4wave_(int itone[], int* nsym, int* nsps, float* fsample, float* f0, float xjunk[], float wave[], int* icmplx, int* nwave);

  void gen9_(char* msg, int* ichk, char* msgsent, int itone[],
               int* itext, fortran_charlen_t, fortran_charlen_t);

  void gen10_(char* msg, int* ichk, char* msgsent, int itone[],
               int* itext, fortran_charlen_t, fortran_charlen_t);

  void gen65_(char* msg, int* ichk, char* msgsent, int itone[],
              int* itext, fortran_charlen_t, fortran_charlen_t);


  void genwspr_(char* msg, char* msgsent, int itone[], fortran_charlen_t, fortran_charlen_t);

  void azdist_(char* MyGrid, char* HisGrid, double* utch, int* nAz, int* nEl,
               int* nDmiles, int* nDkm, int* nHotAz, int* nHotABetter,
               fortran_charlen_t, fortran_charlen_t);

  void morse_(char* msg, int* icw, int* ncw, fortran_charlen_t);

  int ptt_(int nport, int ntx, int* iptt, int* nopen);

  void wspr_downsample_(short int d2[], int* k);
  int savec2_(char* fname, int* TR_seconds, double* dial_freq, fortran_charlen_t);

  void wav12_(short d2[], short d1[], int* nbytes, short* nbitsam2);

  void foxgen_();
}

int volatile itone[NUM_WSPR_SYMBOLS];	//Audio tones for all Tx symbols
int volatile icw[NUM_CW_SYMBOLS];	    //Dits for CW ID
dec_data_t dec_data;             // for sharing with Fortran

int rc;
qint32  g_iptt {0};
wchar_t buffer[256];
int   old_row {-1};
QVector<QColor> g_ColorTbl;

namespace
{
  Radio::Frequency constexpr default_frequency {14076000};
  QRegularExpression message_alphabet {"[- @A-Za-z0-9+./?#<>]*"};
  QRegularExpression messagespec_alphabet {"[- @A-Za-z0-9+./?#<>;]*"};
  QRegularExpression wcall_alphabet {"[A-Za-z0-9/,]*"};
  QRegularExpression wcountry_alphabet {"[A-Za-z0-9/,*]*"};
  QRegularExpression wgrid_alphabet {"([a-r]{2,2}[0-9]{2,2}[,]{1,1})*",QRegularExpression::CaseInsensitiveOption};
  QRegularExpression cqdir_alphabet {"[a-z]{0,2}",QRegularExpression::CaseInsensitiveOption};
  QRegularExpression dxCall_alphabet {"[A-Za-z0-9/]*"};
  QRegularExpression dxGrid_alphabet {"[A-Ra-r]{2,2}[0-9]{2,2}[A-Xa-x]{2,2}[0-9]{2,2}[A-Xa-x]{2,2}"};
  QRegularExpression words_re {R"(^(?:(?<word1>(?:CQ|DE|QRZ)(?:\s?DX|\s(?:[A-Z]{2}|\d{3}))|[A-Z0-9/]+)\s)(?:(?<word2>[A-Z0-9/]+)(?:\s(?<word3>[-+A-Z0-9]+)(?:\s(?<word4>(?:OOO|(?!RR73)[A-R]{2}[0-9]{2})))?)?)?)"};
  constexpr int default_rx_audio_buffer_frames {-1}; // lets Qt decide
  constexpr int default_tx_audio_buffer_frames {-1}; // lets Qt decide

  bool message_is_73 (int type, QStringList const& msg_parts)
  {
    return type >= 0
      && (((type < 6 || 7 == type)
           && (msg_parts.contains ("73") || msg_parts.contains ("RR73")))
          || (type == 6 && !msg_parts.filter ("73").isEmpty ()));
  }

  int ms_minute_error (JTDXDateTime * jtdxtime)
  {
    auto const& now = jtdxtime->currentDateTime2 ();
    auto const& time = now.time ();
    auto second = time.second ();
    return now.msecsTo (now.addSecs (second > 30 ? 60 - second : -second)) - time.msec ();
  }
}

//--------------------------------------------------- MainWindow constructor
MainWindow::MainWindow(bool multiple, QSettings * settings, QSharedMemory *shdmem,
                       unsigned downSampleFactor, QNetworkAccessManager * network_manager,
                       QProcessEnvironment const& env, QWidget *parent) :
  QMainWindow(parent),
  m_jtdxtime {new JTDXDateTime()},

  m_env {env},
  m_dataDir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)},
  m_valid {true},
  m_revision {revision ()},
  m_multiple {multiple},
  m_settings {settings},
  ui(new Ui::MainWindow),
//  m_olek {false},
//  m_olek2 {false},
  m_config {network_manager, settings, this},

  m_WSPR_band_hopping {settings, &m_config, this},
  m_WSPR_tx_next {false},
  m_wideGraph (new WideGraph(settings, m_jtdxtime)),
  m_logDlg (new LogQSO (settings, &m_config, m_jtdxtime, this)),
  m_lastDialFreq {145000000},
  //m_dialFreq {std::numeric_limits<Radio::Frequency>::max ()},
  m_dialFreqRxWSPR {0},
  m_detector {new Detector {RX_SAMPLE_RATE, double(NTMAX), m_jtdxtime , downSampleFactor}},
  m_FFTSize {6912 / 2},         // conservative value to avoid buffer overruns
  m_soundInput {new SoundInput},
  m_modulator {new Modulator {TX_SAMPLE_RATE, NTMAX, m_jtdxtime}},
  m_soundOutput {new SoundOutput},
  m_rx_audio_buffer_frames {0},
  m_tx_audio_buffer_frames {0},
  m_TRperiod {60.0},
  m_msErase {0},
  m_secBandChanged {0},
  m_secTxStopped {0},
  m_msDecStarted {0},
  m_freqNominal {0},
  m_freqTxNominal {0},
  m_lastDisplayFreq {0},
  m_mslastTX {0},
  m_mslastMon {0},
  m_msDecoderStarted {0},
  m_waterfallAvg {1},
  m_ntx {1},
  m_addtx {0},
  m_nlasttx {0},
  m_lapmyc {0},
  m_delay {0},
  m_XIT {0},
  m_ndepth {3},
  m_ncandthin {100},
  m_nFT8Cycles {3},
  m_nFT8SWLCycles {3},
  m_nFT8RXfSens {1},
  m_nFT4depth {3},
  m_sec0 {-1},
  m_RxLog {1},			//Write Date and Time to RxLog
  m_nutc0 {999999},
  m_ntr {0},
  m_tx {0},
  m_secID {0},
  m_pctx {0},
  m_nseq {0},
  m_nWSPRdecodes {0},
  m_used_freq {0},
  m_nguardfreq {51},
  m_idleMinutes {0},
  m_oldTx5Index {0},
  m_oldFreeMsgIndex {0},
  m_ft8threads {0},
  m_acceptUDP {1},
  m_lastCallingFreq {1500},
  m_saveWav {0},
  m_callMode {2},
  m_ft8Sensitivity {0},
  m_position {0},
  m_nsecBandChanged {0},
  m_nDecodes {0},
  m_ncand {0},
  m_btxok {false},
  m_diskData {false},
  m_loopall {false},
  m_txFirst {false},
  m_txGenerated {false},
  m_enableTx {false},
  m_restart {false},
  m_startAnother {false},
  m_showHarmonics {false},
  m_showMyCallMsgRxWindow {true},
  m_showWantedCallRxWindow {true},
  m_bypassRxfFilters {false},
  m_bypassAllFilters {false},
  m_windowPopup {false},
  m_autoErase {false},
  m_autoEraseBC {false},
  m_rprtPriority {false},
  m_dataAvailable {false},
  m_blankLine {false},
  m_notified {false}, 
  m_start {true},
  m_start2 {true},
  m_decodedText2 {false},
  m_freeText {false},
  m_sentFirst73 {false},
  m_reply_me {false},
  m_reply_other {false},
  m_reply_CQ73 {false},
  m_tci_mod_active {false},
  m_tci {false},
  m_counter {0},
  m_currentMessageType {-1},
  m_currentMessage {""},
  m_curMsgTx {""},
  m_lastMessageType {-1},
  m_lockTxFreq {false},
  m_skipTx1 {false},
  m_swl {false},
  m_filter {false},
  m_agcc {false},
  m_hint {true},
  m_disable_TX_on_73 {false},
  m_showTooltips {true},
  m_autoTx {false},
  m_autoseq {false},
  m_wasAutoSeq {false},
  m_Tx5setAutoSeqOff {false},
  m_FTsetAutoSeqOff {false},
  m_uploadSpots {false},
  m_uploading {false},
  m_txNext {false},
  m_grid6 {false},
  m_tuneup {false},
  m_bTxTime {false},
  m_rxDone {false},
  m_bSimplex {false},
  m_logqso73 {false},
  m_processAuto_done {false},
  m_haltTrans {false},
  m_crossbandOptionEnabled {true},
  m_crossbandHLOptionEnabled {true},
  m_repliedCQ {""},
  m_dxbcallTxHalted {""},
  m_currentQSOcallsign {""},
  m_callPrioCQ {false},
  m_callFirst73 {false},
  m_maxDistance {false},
  m_answerWorkedB4 {false},
  m_singleshot {false},
  m_wwDigi {false}, /* CE3TSK: WW Digi contest mode */
  m_autofilter {false},
  m_houndMode {false},
  m_commonFT8b {true},
  m_houndTXfreqJumps {false},
  m_spotDXsummit {false},
  m_FilterState {0},
  m_manualDecode {false},
  m_haltTxWritten {false},
  m_strictdirCQ {false},
  m_colorTxMsgButtons {false},
  m_txbColorSet {false},
  m_bMyCallStd {true},
  m_bHisCallStd {true},
  m_callNotif {false},
  m_gridNotif {false},
  m_countryNameTranslated {false},
  m_qsoLogged {false},
  m_logInitNeeded {false},
  m_dataFilesChanged {false},
  m_dxCallHidden {false},
  m_wantedchkd {false},
  m_menus {true},
  m_wasSkipTx1 {false},
  m_modeChanged {false},
  m_FT8WideDxCallSearch {false},
  m_multInst {false},
  m_myCallCompound {false},
  m_hisCallCompound {false},
  m_callToClipboard {true},
  m_rigOk {false},
  m_bandChanged {false},
  m_useDarkStyle {false},
  m_lostaudio {false},
  m_lasthint {false},
  m_monitoroff {false},
  m_savedRRR {false},
  m_lang {"en_US"},
  m_lastloggedcall {""},
  m_finishedCall {""},
  m_finishedTime {0},
  m_cqdir {""},
  m_lastMode {""},
  m_callsign {""},
  m_grid {""},
  m_name {""},
  m_timeFrom {""},
  m_m_prefix {""},
  m_m_continent {""},
  m_spotText {""},
  m_QSOProgress {CALLING},
  m_transmittedQSOProgress {CALLING},
  tx_status_label {new QLabel {"Receiving"}},
  mode_label {new QLabel {""}},
  last_tx_label {new QLabel {""}},
  txwatchdog_label {new QLabel {""}},
  progressBar {new QProgressBar},
  date_label {new QLabel {""}},
  lastlogged_label {new QLabel {""}},
  qso_count_label {new QLabel {""}},
  contest_score_label {new QLabel {""}},   /* CE3TSK */
  wsprNet {new WSPRNet {network_manager, this}},
  Eqsl {new EQSL {network_manager, this}},
  m_hisCall {""},
  m_hisGrid {""},
  m_wantedCall {""}, m_wantedCountry {""}, m_wantedPrefix {""}, m_wantedGrid {""},
  m_wantedCallList {}, m_wantedCountryList {}, m_wantedPrefixList {}, m_wantedGridList {},
  m_appDir {QApplication::applicationDirPath ()},
  m_palette {"Linrad"},
  m_mode {"FT8"},
  m_oldmode {""},
  m_modeTx {"FT8"},
  m_rpt {"-15"},
  m_rptSent {"-15"},
  m_rptRcvd {"-15"},
  m_pfx {
      "1A", "1S",
      "3A", "3B6", "3B8", "3B9", "3C", "3C0", "3D2", "3D2C",
      "3D2R", "3DA", "3V", "3W", "3X", "3Y", "3YB", "3YP",
      "4J", "4L", "4S", "4U1I", "4U1U", "4W", "4X",
      "5A", "5B", "5H", "5N", "5R", "5T", "5U", "5V", "5W", "5X", "5Z",
      "6W", "6Y",
      "7O", "7P", "7Q", "7X",
      "8P", "8Q", "8R",
      "9A", "9G", "9H", "9J", "9K", "9L", "9M2", "9M6", "9N",
      "9Q", "9U", "9V", "9X", "9Y",
      "A2", "A3", "A4", "A5", "A6", "A7", "A9", "AP",
      "BS7", "BV", "BV9", "BY",
      "C2", "C3", "C5", "C6", "C9", "CE", "CE0X", "CE0Y",
      "CE0Z", "CE9", "CM", "CN", "CP", "CT", "CT3", "CU",
      "CX", "CY0", "CY9",
      "D2", "D4", "D6", "DL", "DU",
      "E3", "E4", "E5", "EA", "EA6", "EA8", "EA9", "EI", "EK",
      "EL", "EP", "ER", "ES", "ET", "EU", "EX", "EY", "EZ",
      "F", "FG", "FH", "FJ", "FK", "FKC", "FM", "FO", "FOA",
      "FOC", "FOM", "FP", "FR", "FRG", "FRJ", "FRT", "FT5W",
      "FT5X", "FT5Z", "FW", "FY",
      "M", "MD", "MI", "MJ", "MM", "MU", "MW",
      "H4", "H40", "HA", "HB", "HB0", "HC", "HC8", "HH",
      "HI", "HK", "HK0", "HK0M", "HL", "HM", "HP", "HR",
      "HS", "HV", "HZ",
      "I", "IS", "IS0",
      "J2", "J3", "J5", "J6", "J7", "J8", "JA", "JDM",
      "JDO", "JT", "JW", "JX", "JY",
      "K", "KC4", "KG4", "KH0", "KH1", "KH2", "KH3", "KH4", "KH5",
      "KH5K", "KH6", "KH7", "KH8", "KH9", "KL", "KP1", "KP2",
      "KP4", "KP5",
      "LA", "LU", "LX", "LY", "LZ",
      "OA", "OD", "OE", "OH", "OH0", "OJ0", "OK", "OM", "ON",
      "OX", "OY", "OZ",
      "P2", "P4", "PA", "PJ2", "PJ7", "PY", "PY0F", "PT0S", "PY0T", "PZ",
      "R1F", "R1M",
      "S0", "S2", "S5", "S7", "S9", "SM", "SP", "ST", "SU",
      "SV", "SVA", "SV5", "SV9",
      "T2", "T30", "T31", "T32", "T33", "T5", "T7", "T8", "T9", "TA",
      "TF", "TG", "TI", "TI9", "TJ", "TK", "TL", "TN", "TR", "TT",
      "TU", "TY", "TZ",
      "UA", "UA2", "UA9", "UK", "UN", "UR",
      "V2", "V3", "V4", "V5", "V6", "V7", "V8", "VE", "VK", "VK0H",
      "VK0M", "VK9C", "VK9L", "VK9M", "VK9N", "VK9W", "VK9X", "VP2E",
      "VP2M", "VP2V", "VP5", "VP6", "VP6D", "VP8", "VP8G", "VP8H",
      "VP8O", "VP8S", "VP9", "VQ9", "VR", "VU", "VU4", "VU7",
      "XE", "XF4", "XT", "XU", "XW", "XX9", "XZ",
      "YA", "YB", "YI", "YJ", "YK", "YL", "YN", "YO", "YS", "YU", "YV", "YV0",
      "Z2", "Z3", "ZA", "ZB", "ZC4", "ZD7", "ZD8", "ZD9", "ZF", "ZK1N",
      "ZK1S", "ZK2", "ZK3", "ZL", "ZL7", "ZL8", "ZL9", "ZP", "ZS", "ZS8"
      },
  m_sfx {"P",  "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "A"},
  m_dateTimeQSOOn {m_jtdxtime->currentDateTimeUtc2()},
  m_status {QsoHistory::NONE},
  mem_jtdxjt9 {shdmem},
  m_downSampleFactor (downSampleFactor),
  m_audioThreadPriority (QThread::HighPriority),
//  m_audioThreadPriority (QThread::HighestPriority),
//  m_audioThreadPriority (QThread::TimeCriticalPriority),
  m_bandEdited {false},
  m_splitMode {false},
  m_monitoring {false},
  m_tx_when_ready {false},
  m_transmitting {false},
  m_tune {false},
  m_txwatchdog {false},
  m_block_pwr_tooltip {false},
  m_PwrBandSetOK {true},
  m_okToPost {false},
  m_lastMonitoredFrequency {default_frequency},
  m_toneSpacing {0.},
  m_geometry_restored {2},
  m_firstDecode {0},
  m_optimizingProgress {"Optimizing decoder FFTs for your CPU.\n"
      "Please be patient,\n"
      "this may take a few minutes", QString {}, 0, 1, this},
  m_messageClient {new MessageClient {QApplication::applicationName (), QCoreApplication::applicationVersion (),
                   m_config.udp_server_name (), m_config.udp_server_port (),
                   this}},
  psk_Reporter {new PSK_Reporter {m_messageClient, this}},
  m_manual {network_manager}
{
  ui->setupUi(this);
  m_bandButtonsTimer.setSingleShot (true);   // CE3TSK: View > Band buttons, see scheduleBandButtons ()
  m_bandButtonsTimer.setInterval (0);
  connect (&m_bandButtonsTimer, &QTimer::timeout, this, &MainWindow::rebuildBandButtons);
  m_dialWheelTimer.setSingleShot (true);   // CE3TSK: dial wheel tuning, see dialFrequencyWheel ()
  m_dialWheelTimer.setInterval (200);
  connect (&m_dialWheelTimer, &QTimer::timeout, this, &MainWindow::applyDialWheel);
  wrap_tooltips (this);   /* CE3TSK: Qt does not word-wrap a plain tooltip, see tooltip_wrap.hpp */
  m_config.set_jtdxtime (m_jtdxtime);
  ui->decodedTextBrowser->setConfiguration (&m_config);
  ui->decodedTextBrowser2->setConfiguration (&m_config);
  refreshSpecialOp (true); /* CE3TSK: initial, must not clear DX or halt TX */
  m_qsoHistory.jtdxtime = m_jtdxtime;
  m_qsoHistory2.jtdxtime = m_jtdxtime; 
  m_baseCall = Radio::base_callsign (m_config.my_callsign ());
 
  m_optimizingProgress.setWindowModality (Qt::WindowModal);
  m_optimizingProgress.setAutoReset (false);
  m_optimizingProgress.setMinimumDuration (15000); // only show after 15s delay

  m_tci = m_config.tci_audio() && m_config.is_tci();
  // Closedown.
  connect (ui->actionExit, &QAction::triggered, this, &QMainWindow::close);

  // parts of the rig error message box that are fixed

  m_rigErrorMessageBox.setInformativeText (tr ("Do you want to reconfigure the radio interface?"));
  m_rigErrorMessageBox.setStandardButtons (JTDXMessageBox::Cancel | JTDXMessageBox::Ok | JTDXMessageBox::Retry);
  m_rigErrorMessageBox.setDefaultButton (JTDXMessageBox::Ok);
  m_rigErrorMessageBox.translate_buttons();
  m_rigErrorMessageBox.setIcon (JTDXMessageBox::Critical);

  // start audio thread and hook up slots & signals for shutdown management
  // these objects need to be in the audio thread so that invoking
  // their slots is done in a thread safe way
  m_soundOutput->moveToThread (&m_audioThread);
  m_modulator->moveToThread (&m_audioThread);
  m_soundInput->moveToThread (&m_audioThread);
  m_detector->moveToThread (&m_audioThread);
  bool ok;
  auto buffer_size = env.value ("JTDX_RX_AUDIO_BUFFER_FRAMES", "0").toInt (&ok);
  m_rx_audio_buffer_frames = ok && buffer_size ? buffer_size : default_rx_audio_buffer_frames;
  buffer_size = env.value ("JTDX_TX_AUDIO_BUFFER_FRAMES", "0").toInt (&ok);
  m_tx_audio_buffer_frames = ok && buffer_size ? buffer_size : default_tx_audio_buffer_frames;
  // hook up sound output stream slots & signals and disposal
  connect (this, &MainWindow::initializeAudioOutputStream, m_soundOutput, &SoundOutput::setFormat);
  connect (m_soundOutput, &SoundOutput::error, this, &MainWindow::showSoundOutError);
  // connect (m_soundOutput, &SoundOutput::status, this, &MainWindow::showStatusMessage);
  connect (this, &MainWindow::outAttenuationChanged, m_soundOutput, &SoundOutput::setAttenuation);
  connect (&m_audioThread, &QThread::finished, m_soundOutput, &QObject::deleteLater);

  // hook up Modulator slots and disposal
  connect (this, &MainWindow::transmitFrequency, m_modulator, &Modulator::setFrequency);
  connect (this, &MainWindow::endTransmitMessage, m_modulator, &Modulator::stop);
  connect (this, &MainWindow::tune, m_modulator, &Modulator::tune);
  connect (this, &MainWindow::sendMessage, m_modulator, &Modulator::start);
  connect (&m_audioThread, &QThread::finished, m_modulator, &QObject::deleteLater);

  // hook up the audio input stream signals, slots and disposal
  connect (this, &MainWindow::startAudioInputStream, m_soundInput, &SoundInput::start);
  connect (this, &MainWindow::suspendAudioInputStream, m_soundInput, &SoundInput::suspend);
  connect (this, &MainWindow::resumeAudioInputStream, m_soundInput, &SoundInput::resume);
  connect (this, &MainWindow::finished, m_soundInput, &SoundInput::stop);
  connect(m_soundInput, &SoundInput::error, this, &MainWindow::showSoundInError);
  // connect(m_soundInput, &SoundInput::status, this, &MainWindow::showStatusMessage);
  connect (&m_audioThread, &QThread::finished, m_soundInput, &QObject::deleteLater);

  connect (this, &MainWindow::finished, this, &MainWindow::close);

  // hook up the detector signals, slots and disposal
  connect (this, &MainWindow::FFTSize, m_detector, &Detector::setBlockSize);
  
  connect(m_detector, &Detector::framesWritten, this, &MainWindow::dataSink,Qt::QueuedConnection);
  connect (&m_audioThread, &QThread::finished, m_detector, &QObject::deleteLater);

  // setup the waterfall
  connect(m_wideGraph.data (), SIGNAL(freezeDecode2(int)),this,SLOT(freezeDecode(int)));
  connect(m_wideGraph.data (), SIGNAL(f11f12(int)),this,SLOT(bumpFqso(int)));
  connect(m_wideGraph.data (), SIGNAL(setXIT2(int)),this,SLOT(setXIT(int)));

  connect (this, &MainWindow::finished, m_wideGraph.data (), &WideGraph::close);

  // setup the log QSO dialog
  connect (m_logDlg.data (), &LogQSO::acceptQSO, this, &MainWindow::acceptQSO2);
  connect (this, &MainWindow::finished, m_logDlg.data (), &LogQSO::close);

  // Network message handlers
  connect (m_messageClient, &MessageClient::reply, this, &MainWindow::replyToUDP);
  connect (m_messageClient, &MessageClient::replay, this, &MainWindow::replayDecodes);
  connect (m_messageClient, &MessageClient::halt_tx, [this] (bool enableTx_only) {
      if (m_config.accept_udp_requests ()) {
        if (enableTx_only) { if (ui->enableTxButton->isChecked ()) ui->enableTxButton->click(); }
//		else { ui->stopTxButton->click(); }
		else { haltTx("UDP request "); }
      }
    });
  connect (m_messageClient, &MessageClient::error, this, &MainWindow::networkError);
  connect (m_messageClient, &MessageClient::free_text, [this] (QString const& text, bool send) {
      if (m_config.accept_udp_requests ()) {
        // send + non-empty text means set and send the free text
        // message, !send + non-empty text means set the current free
        // text message, send + empty text means send the current free
        // text message without change, !send + empty text means clear
        // the current free text messageski
        txwatchdog (false);
        qDebug () << "Free text UDP message - text:" << text << "send:" << send << "text empty:" << text.isEmpty ();
        if (0 == ui->tabWidget->currentIndex ()) {
          if (!text.isEmpty ()) {
            ui->tx5->setCurrentText (text);
          }
          if (send) {
            ui->txb5->click ();
          } else if (text.isEmpty ()) {
            ui->tx5->setCurrentText (text);
          }
        } else if (1 == ui->tabWidget->currentIndex ()) {
          if (!text.isEmpty ()) {
            ui->freeTextMsg->setCurrentText (text);
          }
          if (send) {
            ui->rbFreeText->click ();
          } else if (text.isEmpty ()) {
            ui->freeTextMsg->setCurrentText (text);
          }
        }
      }
    });

  connect (m_messageClient, &MessageClient::set_tx_deltafreq, [this] (quint32 tx_delta_frequency) {
    if (m_config.accept_udp_requests ()) {
        txwatchdog (false);
        quint32 start_freq=m_wideGraph->nStartFreq();
        quint32 max_freq=m_wideGraph->Fmax();
        if(tx_delta_frequency > start_freq && tx_delta_frequency < max_freq) ui->TxFreqSpinBox->setValue(tx_delta_frequency);
    }
  });

  connect (m_messageClient, &MessageClient::trigger_CQ, [this] (QString const& direction, bool tx_period, bool send) {
    if (m_config.accept_udp_requests ()) {
        txwatchdog (false);
        if (0 == ui->tabWidget->currentIndex ()) {
          if (direction != ui->direction1LineEdit->text()) ui->direction1LineEdit->setText(direction);
          if (send) ui->txb6->click ();
          if (tx_period != m_txFirst) ui->TxMinuteButton->click ();
          if (send && !m_enableTx) ui->enableTxButton->click ();
        }
        if (1 == ui->tabWidget->currentIndex ()) {
          if (direction != ui->directionLineEdit->text()) ui->directionLineEdit->setText(direction);
          if (send) ui->pbCallCQ->click ();
          if (tx_period != m_txFirst) ui->TxMinuteButton->click ();
          if (send && !m_enableTx) ui->enableTxButton->click ();
        }
    }
  });

  // Hook up WSPR band hopping
  connect (ui->band_hopping_schedule_push_button, &QPushButton::clicked
           , &m_WSPR_band_hopping, &WSPRBandHopping::show_dialog);
  connect (ui->sbTxPercent, static_cast<void (QSpinBox::*) (int)> (&QSpinBox::valueChanged)
           , &m_WSPR_band_hopping, &WSPRBandHopping::set_tx_percent);

  on_EraseButton_clicked ();
  clearDX ("");

  QActionGroup* modeGroup = new QActionGroup(this);
  ui->actionFT4->setActionGroup(modeGroup);
  ui->actionFT2->setActionGroup(modeGroup);   // CE3TSK
  ui->actionFT8->setActionGroup(modeGroup);
  ui->actionJT65->setActionGroup(modeGroup);
  ui->actionJT9_JT65->setActionGroup(modeGroup);
  ui->actionJT9->setActionGroup(modeGroup);
  ui->actionT10->setActionGroup(modeGroup);
  ui->actionWSPR_2->setActionGroup(modeGroup);

  QActionGroup* languageGroup = new QActionGroup(this);
  ui->actionEnglish->setActionGroup(languageGroup);
  ui->actionEstonian->setActionGroup(languageGroup);
  ui->actionRussian->setActionGroup(languageGroup);
  ui->actionCatalan->setActionGroup(languageGroup);
  ui->actionCroatian->setActionGroup(languageGroup);
  ui->actionDanish->setActionGroup(languageGroup);
  ui->actionDutch->setActionGroup(languageGroup);
  ui->actionHungarian->setActionGroup(languageGroup);
  ui->actionSpanish->setActionGroup(languageGroup);
  ui->actionSwedish->setActionGroup(languageGroup);
  ui->actionFrench->setActionGroup(languageGroup);
  ui->actionItalian->setActionGroup(languageGroup);
  ui->actionGerman->setActionGroup(languageGroup);   // CE3TSK
  ui->actionLatvian->setActionGroup(languageGroup);
  ui->actionPolish->setActionGroup(languageGroup);
  ui->actionPortuguese->setActionGroup(languageGroup);
  ui->actionPortuguese_BR->setActionGroup(languageGroup);
  ui->actionChinese_simplified->setActionGroup(languageGroup);
  ui->actionChinese_traditional->setActionGroup(languageGroup);
  ui->actionJapanese->setActionGroup(languageGroup);
  ui->actionKorean->setActionGroup(languageGroup);   // CE3TSK

  QActionGroup* saveGroup = new QActionGroup(this);
  ui->actionNone->setActionGroup(saveGroup);
  ui->actionSave_decoded->setActionGroup(saveGroup);
  ui->actionSave_all->setActionGroup(saveGroup);

  QActionGroup* DepthGroup = new QActionGroup(this);
  ui->actionQuickDecode->setActionGroup(DepthGroup);
  ui->actionMediumDecode->setActionGroup(DepthGroup);
  ui->actionDeepestDecode->setActionGroup(DepthGroup);

  QActionGroup* FT8CyclesGroup = new QActionGroup(this);
  ui->actionDecFT8cycles3->setActionGroup(FT8CyclesGroup);
  ui->actionDecFT8cycles4->setActionGroup(FT8CyclesGroup);
  ui->actionDecFT8cycles5->setActionGroup(FT8CyclesGroup);
  ui->actionDecFT8cycles6->setActionGroup(FT8CyclesGroup);
  ui->actionDecFT8cycles7->setActionGroup(FT8CyclesGroup);
  ui->actionDecFT8cycles8->setActionGroup(FT8CyclesGroup);
  ui->actionDecFT8cycles9->setActionGroup(FT8CyclesGroup);

  QActionGroup* FT8SWLCyclesGroup = new QActionGroup(this);
  ui->actionDecFT8SWLcycles3->setActionGroup(FT8SWLCyclesGroup);
  ui->actionDecFT8SWLcycles4->setActionGroup(FT8SWLCyclesGroup);
  ui->actionDecFT8SWLcycles5->setActionGroup(FT8SWLCyclesGroup);
  ui->actionDecFT8SWLcycles6->setActionGroup(FT8SWLCyclesGroup);
  ui->actionDecFT8SWLcycles7->setActionGroup(FT8SWLCyclesGroup);
  ui->actionDecFT8SWLcycles8->setActionGroup(FT8SWLCyclesGroup);
  ui->actionDecFT8SWLcycles9->setActionGroup(FT8SWLCyclesGroup);


  QActionGroup* FT8RXfreqSensitivityGroup = new QActionGroup(this);
  ui->actionRXfLow->setActionGroup(FT8RXfreqSensitivityGroup);
  ui->actionRXfMedium->setActionGroup(FT8RXfreqSensitivityGroup);
  ui->actionRXfHigh->setActionGroup(FT8RXfreqSensitivityGroup);
  
  // CE3TSK: decoding presets - radio actions, none checked while the controls match no preset
  QActionGroup* FT8PresetGroup = new QActionGroup(this);
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
  FT8PresetGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
#else
  // older Qt: a plain group; refreshDecodePreset() sets all three states explicitly whenever
  // the menu opens, and after a preset is applied, so the radio behaviour is kept by hand
  FT8PresetGroup->setExclusive(false);
#endif
  ui->actionFT8PresetDefault->setActionGroup(FT8PresetGroup);
  ui->actionFT8PresetMaxEfficiency->setActionGroup(FT8PresetGroup);
  ui->actionFT8PresetMaxDecodes->setActionGroup(FT8PresetGroup);
  ui->actionFT8PresetPipelineLight->setActionGroup(FT8PresetGroup);
  markRecommendedPresets();   // CE3TSK
  markFT4Presets();   // CE3TSK item 67
  markFT2Presets();   // CE3TSK step 5: the same marks on FT2's tiers
  ui->actionFT8PresetPipeline->setActionGroup(FT8PresetGroup);
  ui->actionFT8PresetPipelineFull->setActionGroup(FT8PresetGroup);
  ui->actionFT8PresetPipelineRun->setActionGroup(FT8PresetGroup);
  // CE3TSK: decode bandwidth radio entries (decoderange.h), the index kept in each action's data
  QActionGroup* BWGroup = new QActionGroup(this);
  {
    QAction* bw[] = {ui->actionBW0, ui->actionBW1, ui->actionBW2, ui->actionBW3, ui->actionBW4, ui->actionBW5,
                     ui->actionBW6, ui->actionBW7, ui->actionBW8, ui->actionBW9, ui->actionBW10, ui->actionBW11, ui->actionBW12, ui->actionBW13};
    for (int i = 0; i < DECODE_BW_COUNT; ++i) { bw[i]->setData(i); bw[i]->setActionGroup(BWGroup); }
    connect(BWGroup, &QActionGroup::triggered, this, [this](QAction* a) { m_decodeBandwidth=a->data().toInt(); });
  }
  // CE3TSK: the TX background's radio entries
  QActionGroup* FT8BgGroup = new QActionGroup(this);
  for (auto a : {ui->actionFT8BgOff, ui->actionFT8BgAuto, ui->actionFT8Bg1, ui->actionFT8Bg2, ui->actionFT8Bg3, ui->actionFT8Bg4, ui->actionFT8Bg5}) a->setActionGroup(FT8BgGroup);
  QActionGroup* BgCyclesGroup = new QActionGroup(this);
  for (auto a : {ui->actionBgCycles3, ui->actionBgCycles4, ui->actionBgCycles5, ui->actionBgCycles6, ui->actionBgCycles7, ui->actionBgCycles8, ui->actionBgCycles9}) a->setActionGroup(BgCyclesGroup);
  QActionGroup* BgSWLCyclesGroup = new QActionGroup(this);
  for (auto a : {ui->actionBgSWLcycles3, ui->actionBgSWLcycles4, ui->actionBgSWLcycles5, ui->actionBgSWLcycles6, ui->actionBgSWLcycles7, ui->actionBgSWLcycles8, ui->actionBgSWLcycles9}) a->setActionGroup(BgSWLCyclesGroup);
  QActionGroup* BgRXfGroup = new QActionGroup(this);
  for (auto a : {ui->actionBgRXfLow, ui->actionBgRXfMedium, ui->actionBgRXfHigh}) a->setActionGroup(BgRXfGroup);
  QActionGroup* BgSensGroup = new QActionGroup(this);
  for (auto a : {ui->actionBgSensMin, ui->actionBgSensLow, ui->actionBgSensSubpass}) a->setActionGroup(BgSensGroup);
  QActionGroup* FT4EnsembleGroup = new QActionGroup(this);   // CE3TSK: FT4 ensemble members, same shape
  for (auto a : {ui->actionFT4EnsembleOff, ui->actionFT4EnsembleAuto, ui->actionFT4EnsembleBudget, ui->actionFT4Ensemble1, ui->actionFT4Ensemble2, ui->actionFT4Ensemble3,
                 ui->actionFT4Ensemble4, ui->actionFT4Ensemble5, ui->actionFT4Ensemble6}) a->setActionGroup(FT4EnsembleGroup);
  QActionGroup* FT4BgEnsembleGroup = new QActionGroup(this);   // CE3TSK item 59: the TX background's ladder
  for (auto a : {ui->actionFT4BgEnsembleOff, ui->actionFT4BgEnsembleAuto, ui->actionFT4BgEnsemble1, ui->actionFT4BgEnsemble2, ui->actionFT4BgEnsemble3,
                 ui->actionFT4BgEnsemble4, ui->actionFT4BgEnsemble5, ui->actionFT4BgEnsemble6}) a->setActionGroup(FT4BgEnsembleGroup);
  QActionGroup* FT4BgEffortGroup = new QActionGroup(this);   // CE3TSK item 69
  for (auto a : {ui->actionFT4BgFast, ui->actionFT4BgMedium, ui->actionFT4BgDeep}) a->setActionGroup(FT4BgEffortGroup);
  QActionGroup* FT4SensGroup = new QActionGroup(this);   // CE3TSK item 72
  for (auto a : {ui->actionFT4SensStd, ui->actionFT4SensLow}) a->setActionGroup(FT4SensGroup);
  QActionGroup* FT4BgSensGroup = new QActionGroup(this);   // CE3TSK item 73
  for (auto a : {ui->actionFT4BgSensStd, ui->actionFT4BgSensLow}) a->setActionGroup(FT4BgSensGroup);
  QActionGroup* FT4RXfGroup = new QActionGroup(this);   // CE3TSK item 75
  for (auto a : {ui->actionFT4RXfLow, ui->actionFT4RXfMedium, ui->actionFT4RXfHigh}) a->setActionGroup(FT4RXfGroup);
  QActionGroup* FT4BgRXfGroup = new QActionGroup(this);
  for (auto a : {ui->actionFT4BgRXfLow, ui->actionFT4BgRXfMedium, ui->actionFT4BgRXfHigh}) a->setActionGroup(FT4BgRXfGroup);
  QActionGroup* FT8EnsembleGroup = new QActionGroup(this);   // CE3TSK: ensemble effort radio entries
  for (auto a : {ui->actionFT8EnsembleOff, ui->actionFT8EnsembleAuto, ui->actionFT8EnsembleBudget, ui->actionFT8Ensemble1, ui->actionFT8Ensemble2,
                 ui->actionFT8Ensemble3, ui->actionFT8Ensemble4, ui->actionFT8Ensemble5}) a->setActionGroup(FT8EnsembleGroup);
  connect(ui->menuFT8_decoding, &QMenu::aboutToShow, this, &MainWindow::refreshDecodePreset);
  /* CE3TSK: the Ko-fi button, in the menu bar's right corner. The title bar itself belongs to
     the window manager, so this is the closest a Qt application can put it - and it sits on the
     line directly below, which is what the slides do with their fixed top-left button. Same
     cup as the About dialog uses, on its own and large - a wordmark that small would not be
     readable (contrib/support_cup_*.png, drawn by tools/make_support_button.py). Flat: no
     frame until hovered, and the tooltip carries the words. */
  {
    auto * kofi = new QToolButton {this};
    kofi->setAutoRaise (true);
    kofi->setCursor (Qt::PointingHandCursor);
    kofi->setObjectName ("kofiButton");
    kofi->setIcon (QIcon {m_useDarkStyle ? ":/support_cup_dark.png" : ":/support_cup_light.png"});
    kofi->setIconSize (QSize {28, 28});   // the cup alone - the wordmark would be unreadable here
    kofi->setToolTip (tr ("Support JTDX_contest on Ko-fi"));
    connect (kofi, &QToolButton::clicked, this,
             [] { QDesktopServices::openUrl (QUrl {"https://ko-fi.com/ce3tsk"}); });
    ui->menuBar->setCornerWidget (kofi, Qt::TopRightCorner);
  }
  connect(ui->menuFT4_decoding, &QMenu::aboutToShow, this, &MainWindow::refreshFT4Preset);   // CE3TSK: same for FT4
  connect(ui->menuFT2_decoding, &QMenu::aboutToShow, this, &MainWindow::refreshFT2Preset);   // CE3TSK step 5: and for FT2
  QActionGroup* FT2PresetGroup = new QActionGroup(this);
  for (auto a : {ui->actionFT2PresetFast, ui->actionFT2PresetDefault,
                 ui->actionFT2PresetRecommended, ui->actionFT2PresetMaxEffort}) a->setActionGroup(FT2PresetGroup);
  QActionGroup* FT2EnsembleGroup = new QActionGroup(this);
  for (auto a : {ui->actionFT2EnsembleOff, ui->actionFT2EnsembleAuto, ui->actionFT2EnsembleBudget}) a->setActionGroup(FT2EnsembleGroup);
  QActionGroup* FT2BgEnsembleGroup = new QActionGroup(this);
  for (auto a : {ui->actionFT2BgEnsembleOff, ui->actionFT2BgEnsemble3, ui->actionFT2BgEnsemble6,
                 ui->actionFT2BgEnsembleAuto}) a->setActionGroup(FT2BgEnsembleGroup);
  QActionGroup* FT4PresetGroup = new QActionGroup(this);
  for (auto a : {ui->actionFT4PresetFast, ui->actionFT4PresetDefault, ui->actionFT4PresetBestPower,
                 ui->actionFT4PresetRecommended, ui->actionFT4PresetMaxDecodes, ui->actionFT4PresetMaxEffort}) a->setActionGroup(FT4PresetGroup);
  // P13: the preset lamp under the Contest lamp follows every recipe control (the RX and TX
  // background submenus, the SWL button) and the thread count; the auto-connected slots of
  // those actions run first (setupUi connected them), so the members are current here
  {
    std::function<void(QMenu*)> hook = [this, &hook](QMenu* m) {
      for (auto a : m->actions()) { if (a->menu()) hook(a->menu()); else if (!a->isSeparator()) connect(a, &QAction::triggered, this, &MainWindow::refreshDecodePreset); }
    };
    hook(ui->menuFT8_RX); hook(ui->menuFT8_TX); hook(ui->menuFT8_threads);
    hook(ui->menuFT4_RX); hook(ui->menuFT4_TX);   // item 73: the lamp reads the FT4 preset in FT4 mode
    /* CE3TSK step 5: and FT2's, or the lamp never notices a control changing under it - the tier
       would still read "Preset R" after the operator moved an RX member off it, and would not come
       back when they moved it back. Every action in these two submenus feeds the same refresh. */
    hook(ui->menuFT2_RX); hook(ui->menuFT2_TX);
    connect(ui->swlButton, &QPushButton::clicked, this, &MainWindow::refreshDecodePreset);
  // CE3TSK: dragging the bar is the one thing that changes the share the panes keep
  /* CE3TSK: the drag is read AFTER Qt has applied it. Reading sizes() inside splitterMoved gives the
     positions from before the move, so the share was set back to the old one and the next layout
     snapped the bar home - the handle could not be dragged at all, which is how this was found. */
  connect(ui->splitter, &QSplitter::splitterMoved, this,
          [this] (int, int) { if (!m_splitApplying) QTimer::singleShot (0, this, [this] { rememberSplitRatio (); }); });
  }
  QActionGroup* FT8DecoderSensitivityGroup = new QActionGroup(this);
  ui->actionFT8SensMin->setActionGroup(FT8DecoderSensitivityGroup);
  ui->actionlowFT8thresholds->setActionGroup(FT8DecoderSensitivityGroup);
  ui->actionFT8subpass->setActionGroup(FT8DecoderSensitivityGroup);

  QActionGroup* FT4DepthGroup = new QActionGroup(this);
  ui->actionFT4fast->setActionGroup(FT4DepthGroup);
  ui->actionFT4medium->setActionGroup(FT4DepthGroup);
  ui->actionFT4deep->setActionGroup(FT4DepthGroup);

  QActionGroup* AutoseqGroup = new QActionGroup(this);
  ui->actionCallNone->setActionGroup(AutoseqGroup);
  ui->actionCallFirst->setActionGroup(AutoseqGroup);
  ui->actionCallMid->setActionGroup(AutoseqGroup);
  ui->actionCallEnd->setActionGroup(AutoseqGroup);

  QActionGroup* FT8threadsGroup = new QActionGroup(this);
  ui->actionMTAuto->setActionGroup(FT8threadsGroup);
  ui->actionMT1->setActionGroup(FT8threadsGroup);
  ui->actionMT2->setActionGroup(FT8threadsGroup);
  ui->actionMT3->setActionGroup(FT8threadsGroup);
  ui->actionMT4->setActionGroup(FT8threadsGroup);
  ui->actionMT5->setActionGroup(FT8threadsGroup);
  ui->actionMT6->setActionGroup(FT8threadsGroup);
  ui->actionMT7->setActionGroup(FT8threadsGroup);
  ui->actionMT8->setActionGroup(FT8threadsGroup);
  ui->actionMT9->setActionGroup(FT8threadsGroup);
  ui->actionMT10->setActionGroup(FT8threadsGroup);
  ui->actionMT11->setActionGroup(FT8threadsGroup);
  ui->actionMT12->setActionGroup(FT8threadsGroup);
  ui->actionMT13->setActionGroup(FT8threadsGroup);
  ui->actionMT14->setActionGroup(FT8threadsGroup);
  ui->actionMT15->setActionGroup(FT8threadsGroup);
  ui->actionMT16->setActionGroup(FT8threadsGroup);
  ui->actionMT17->setActionGroup(FT8threadsGroup);
  ui->actionMT18->setActionGroup(FT8threadsGroup);
  ui->actionMT19->setActionGroup(FT8threadsGroup);
  ui->actionMT20->setActionGroup(FT8threadsGroup);
  ui->actionMT21->setActionGroup(FT8threadsGroup);
  ui->actionMT22->setActionGroup(FT8threadsGroup);
  ui->actionMT23->setActionGroup(FT8threadsGroup);
  ui->actionMT24->setActionGroup(FT8threadsGroup);

  QActionGroup* AcceptUDPGroup = new QActionGroup(this);
  ui->actionAcceptUDPCQ->setActionGroup(AcceptUDPGroup);
  ui->actionAcceptUDPCQ73->setActionGroup(AcceptUDPGroup);
  ui->actionAcceptUDPAny->setActionGroup(AcceptUDPGroup);

  QButtonGroup* txMsgButtonGroup = new QButtonGroup {this};
  txMsgButtonGroup->addButton(ui->txrb1,1);
  txMsgButtonGroup->addButton(ui->txrb2,2);
  txMsgButtonGroup->addButton(ui->txrb3,3);
  txMsgButtonGroup->addButton(ui->txrb4,4);
  txMsgButtonGroup->addButton(ui->txrb5,5);
  txMsgButtonGroup->addButton(ui->txrb6,6);
  connect(txMsgButtonGroup,SIGNAL(buttonClicked(int)),SLOT(set_ntx(int)));
  connect(ui->decodedTextBrowser2,SIGNAL(selectCallsign(bool,bool)),this,SLOT(doubleClickOnCall(bool,bool)));
  connect(ui->decodedTextBrowser,SIGNAL(selectCallsign(bool,bool)),this,SLOT(doubleClickOnCall2(bool,bool)));
  connect(ui->decodedTextBrowser->horizontalScrollBar(),SIGNAL(sliderMoved(int)),SLOT(ScrollBarPosition(int)));

  // initialise decoded text font and hook up change signal
  setDecodedTextFont (m_config.decoded_text_font ());
  connect (&m_config, &Configuration::decoded_text_font_changed, [this] (QFont const& font) {
      setDecodedTextFont (font);
    });

//  setWindowTitle (program_title ());
  setWindowTitle (program_title ()); /* CE3TSK */

  createStatusBar();

  connect(&proc_jtdxjt9, &QProcess::readyReadStandardOutput,this, &MainWindow::readFromStdout);
#if QT_VERSION < QT_VERSION_CHECK (5, 6, 0)
  connect(&proc_jtdxjt9, static_cast<void (QProcess::*) (QProcess::ProcessError)> (&QProcess::error),
          [this] (QProcess::ProcessError error) {
            subProcessError (&proc_jtdxjt9, error);
          });
#else
  connect(&proc_jtdxjt9, &QProcess::errorOccurred, [this] (QProcess::ProcessError error) {
                                                 subProcessError (&proc_jtdxjt9, error);
                                               });
#endif
  connect(&proc_jtdxjt9, static_cast<void (QProcess::*) (int, QProcess::ExitStatus)> (&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus status) {
            if (subProcessFailed (&proc_jtdxjt9, exitCode, status))
              {
                m_valid = false;          // ensures exit if still
                                          // constructing
                QTimer::singleShot (0, this, SLOT (close ()));
              }
          });

  connect(&p1, &QProcess::readyReadStandardOutput,this, &MainWindow::p1ReadFromStdout);
#if QT_VERSION < QT_VERSION_CHECK (5, 6, 0)
  connect(&p1, static_cast<void (QProcess::*) (QProcess::ProcessError)> (&QProcess::error),
          [this] (QProcess::ProcessError error) {
            subProcessError (&p1, error);
          });
#else
  connect(&p1, &QProcess::errorOccurred, [this] (QProcess::ProcessError error) {
                                           subProcessError (&p1, error);
                                         });
#endif
  connect(&p1, static_cast<void (QProcess::*) (int, QProcess::ExitStatus)> (&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus status) {
            if (subProcessFailed (&p1, exitCode, status))
              {
                m_valid = false;          // ensures exit if still
                                          // constructing
                QTimer::singleShot (0, this, SLOT (close ()));
              }
          });

#if QT_VERSION < QT_VERSION_CHECK (5, 6, 0)
  connect(&p3, static_cast<void (QProcess::*) (QProcess::ProcessError)> (&QProcess::error),
          [this] (QProcess::ProcessError error) {
#else
  connect(&p3, &QProcess::errorOccurred, [this] (QProcess::ProcessError error) {
#endif
#if !defined(Q_OS_WIN)
                                           if (QProcess::FailedToStart != error)
#else
                                           if (QProcess::Crashed != error)
#endif
                                             {
                                               subProcessError (&p3, error);
                                             }
                                         });
  connect(&p3, &QProcess::started, [this] () {
                                     showStatusMessage (QString {"Started: %1 \"%2\""}.arg (p3.program ()).arg (p3.arguments ().join ("\" \"")));
                                   });
  connect(&p3, static_cast<void (QProcess::*) (int, QProcess::ExitStatus)> (&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus status) {
#if defined(Q_OS_WIN)
            // We forgo detecting user_hardware failures with exit
            // code 1 on Windows. This is because we use CMD.EXE to
            // run the executable. CMD.EXE returns exit code 1 when it
            // can't find the target executable.
            if (exitCode != 1)  // CMD.EXE couldn't find file to execute
#else
            // We forgo detecting user_hardware failures with exit
            // code 127 non-Windows. This is because we use /bin/sh to
            // run the executable. /bin/sh returns exit code 127 when it
            // can't find the target executable.
            if (exitCode != 127)  // /bin/sh couldn't find file to execute
#endif
              {
                subProcessFailed (&p3, exitCode, status);
              }
          });

  // hook up save WAV file exit handling
  connect (&m_saveWAVWatcher, &QFutureWatcher<QString>::finished, [this] {
      // extract the promise from the future
      auto const& result = m_saveWAVWatcher.future ().result ();
      if (!result.isEmpty ())   // error
        {
          JTDXMessageBox::critical_message (this, "", tr("Error Writing WAV File"), result);
        }
    });

  bindBandCombo ();   /* CE3TSK: also called on a contest transition, see below */
  connect (ui->bandComboBox->lineEdit (), &QLineEdit::textEdited, [this] (QString const&) {m_bandEdited = true; if(m_config.write_decoded_debug()) writeToALLTXT("bandComboBox line edited to " + ui->bandComboBox->lineEdit()->text());});

  // hook up configuration signals
  connect (&m_config, &Configuration::transceiver_update, this, &MainWindow::handle_transceiver_update);
  connect (&m_config, &Configuration::transceiver_TCIframesWritten, this, &MainWindow::dataSink);
  connect (&m_config, &Configuration::transceiver_TCImodActive, this, &MainWindow::tci_mod_active);
  connect (&m_config, &Configuration::transceiver_failure, this, &MainWindow::handle_transceiver_failure);
  connect (&m_config, &Configuration::udp_server_changed, m_messageClient, &MessageClient::set_server);
  connect (&m_config, &Configuration::udp_server_port_changed, m_messageClient, &MessageClient::set_server_port);
  connect (&m_config, &Configuration::data_files_updated, this, &MainWindow::dataFilesUpdated);   // CE3TSK


  // set up message text validators
  ui->tx1->setValidator (new QRegularExpressionValidator {message_alphabet, this});
  ui->tx2->setValidator (new QRegularExpressionValidator {messagespec_alphabet, this});
  ui->tx3->setValidator (new QRegularExpressionValidator {message_alphabet, this});
  ui->tx4->setValidator (new QRegularExpressionValidator {message_alphabet, this});
  ui->tx5->setValidator (new QRegularExpressionValidator {message_alphabet, this});
  ui->tx6->setValidator (new QRegularExpressionValidator {message_alphabet, this});
  ui->freeTextMsg->setValidator (new QRegularExpressionValidator {message_alphabet, this});
  ui->wantedCall->setValidator (new QRegularExpressionValidator {wcall_alphabet, this});
  ui->wantedCountry->setValidator (new QRegularExpressionValidator {wcountry_alphabet, this});
  ui->wantedPrefix->setValidator (new QRegularExpressionValidator {wcall_alphabet, this});
  ui->wantedGrid->setValidator (new QRegularExpressionValidator {wgrid_alphabet, this});
  ui->dxCallEntry->setValidator (new QRegularExpressionValidator {dxCall_alphabet, this});
  ui->dxGridEntry->setValidator (new QRegularExpressionValidator {dxGrid_alphabet, this});
  ui->directionLineEdit->setValidator (new QRegularExpressionValidator {cqdir_alphabet, this});
  ui->direction1LineEdit->setValidator (new QRegularExpressionValidator {cqdir_alphabet, this});

  // Free text macros model to widget hook up.
  ui->tx5->setModel (m_config.macros ());
  connect (ui->tx5->lineEdit ()
           , &QLineEdit::editingFinished
           , [this] () {on_tx5_currentTextChanged (ui->tx5->lineEdit ()->text ());});
  ui->freeTextMsg->setModel (m_config.macros ());
  connect (ui->freeTextMsg->lineEdit ()
           , &QLineEdit::editingFinished
           , [this] () {on_freeTextMsg_currentTextChanged (ui->freeTextMsg->lineEdit ()->text ());});

  connect(&m_guiTimer, &QTimer::timeout, this, &MainWindow::guiUpdate);
  m_guiTimer.start(100);   //### Don't change the 100 ms! ###

  ptt0Timer.setSingleShot(true); connect(&ptt0Timer, &QTimer::timeout, this, &MainWindow::stopTx2);
  ptt1Timer.setSingleShot(true); connect(&ptt1Timer, &QTimer::timeout, this, &MainWindow::startTx2);
  logQSOTimer.setSingleShot(true); connect(&logQSOTimer, &QTimer::timeout, this, &MainWindow::on_logQSOButton_clicked);
  tuneButtonTimer.setSingleShot(true); connect(&tuneButtonTimer, &QTimer::timeout, this, &MainWindow::haltTxTuneTimer);
  cqButtonTimer.setSingleShot(true); connect(&cqButtonTimer, &QTimer::timeout, this, &MainWindow::on_pbCallCQ_clicked);
  enableTxButtonTimer.setSingleShot(true); connect(&enableTxButtonTimer, &QTimer::timeout, this, &MainWindow::enableTxButton_off);
  tx73ButtonTimer.setSingleShot(true); connect(&tx73ButtonTimer, &QTimer::timeout, this, &MainWindow::on_pbSend73_clicked);
  logClearDXTimer.setSingleShot(true); connect(&logClearDXTimer, &QTimer::timeout, this, &MainWindow::logClearDX);
  dxbcallTxHaltedClearTimer.setSingleShot(true); connect(&dxbcallTxHaltedClearTimer, &QTimer::timeout, this, &MainWindow::dxbcallTxHaltedClear);
  tuneATU_Timer.setSingleShot(true); connect(&tuneATU_Timer, &QTimer::timeout, this, &MainWindow::stopTuneATU);
  killFileTimer.setSingleShot(true); connect(&killFileTimer, &QTimer::timeout, this, &MainWindow::killFile);
  uploadTimer.setSingleShot(true); connect(&uploadTimer, SIGNAL(timeout()), this, SLOT(uploadSpots()));
  TxAgainTimer.setSingleShot(true); connect(&TxAgainTimer, SIGNAL(timeout()), this, SLOT(TxAgain()));
  StopTuneTimer.setSingleShot(true); connect(&StopTuneTimer, SIGNAL(timeout()), this, SLOT(stop_tuning()));
  RxQSYTimer.setSingleShot(true); connect(&RxQSYTimer, SIGNAL(timeout()), this, SLOT(RxQSY()));
  minuteTimer.setSingleShot(true); connect (&minuteTimer, &QTimer::timeout, this, &MainWindow::on_the_minute);

  connect(m_wideGraph.data (), SIGNAL(setFreq3(int,int)), this, SLOT(setFreq4(int,int)));
  connect(m_wideGraph.data (), SIGNAL(setRxFreq3(int)), this, SLOT(setRxFreq4(int)));
  connect(m_wideGraph.data (), SIGNAL(filter_on3()), this, SLOT(filter_on()));
  connect(m_wideGraph.data (), SIGNAL(toggle_filter3()), this, SLOT(toggle_filter()));
  connect(m_wideGraph.data (), SIGNAL(esc_key()), this, SLOT(escapeHalt()));

  fsWatcher = new QFileSystemWatcher(this);
  fsWatcher->addPath(m_dataDir.absoluteFilePath ("wsjtx_log.adi"));
  connect(fsWatcher, SIGNAL(fileChanged(QString)), this, SLOT(logChanged()));
  
  m_rrr=false;
  m_killAll=false;
  m_sentFirst73=false;
  m_skipTx1=false;
  m_QSOText.clear();
  decodeBusy(false);
  m_callingFrequency=0;
  QString t1[28]={"1 uW","2 uW","5 uW","10 uW","20 uW","50 uW","100 uW","200 uW","500 uW",
                  "1 mW","2 mW","5 mW","10 mW","20 mW","50 mW","100 mW","200 mW","500 mW",
                  "1 W","2 W","5 W","10 W","20 W","50 W","100 W","200 W","500 W","1 kW"};

  for(int i=0; i<28; i++)  {                      //Initialize dBm values
    float dbm=(10.0*i)/3.0 - 30.0;
    int ndbm=0;
    if(dbm<0) ndbm=int(dbm-0.5);
    if(dbm>=0) ndbm=int(dbm+0.5);
    QString t;
    t = QString::asprintf("%d dBm  ",ndbm);
    t+=t1[i];
    ui->TxPowerComboBox->addItem(t);
  }


  ui->labAz->setStyleSheet("border: 0px;");
  ui->labDist->setStyleSheet("border: 0px;");

  m_useDarkStyle = m_config.useDarkStyle(); setDecodeMenuColours();
  ui->actionUse_dark_style->setChecked (m_useDarkStyle);   // CE3TSK
  readSettings();		         //Restore user's setup params
  refreshDecodePreset();   // P13: the preset lamp from the restored controls

  QString t;
  if (m_mode.startsWith("FT")) t = "UTC     dB   DT "+tr("Freq   Message");
  else t = "UTC   dB   DT "+tr("Freq   Message");
  ui->decodedTextLabel->setTextFormat(Qt::PlainText); ui->decodedTextLabel->setText(t);
  ui->decodedTextLabel2->setText(t);

  /* CE3TSK: these six used to be pinned to 80 px wide here, which is narrower than the
     lamps above them and, in a language whose word does not fit, narrower than the
     button's own text - "Одиноч. QSO" burst out of its cap and stood proud of the
     column while its neighbours stayed short. The .ui now caps only the height, so all
     six take the column's width, as the lamps do. The width is still bounded: the
     column is only ever as wide as its widest item asks to be. */
  dynamicButtonsInit();

  m_audioThread.start (m_audioThreadPriority);

#ifdef WIN32
  if (!m_multiple)
    {
      while(true)
        {
          int iret=killbyname("jtdxjt9.exe");
          if(iret == 603) break;
            JTDXMessageBox::warning_message (this, "", tr ("Error Killing jtdxjt9.exe Process")
                                         , tr ("KillByName return code: %1")
                                         .arg (iret));
        }
    }
#endif

//  auto_tx_label->setText (m_autoTx ? "AutoTx Armed" : "AutoTx Disarmed");

  {
    //delete any .quit file that might have been left lying around
    //since its presence will cause jtdxjt9 to exit a soon as we start it
    //and decodes will hang
    QFile quitFile {m_config.temp_dir ().absoluteFilePath (".quit")};
    while (quitFile.exists ())
      {
        if (!quitFile.remove ())
          {
            JTDXMessageBox::query_message (this, "", tr ("Error removing \"%1\"").arg (quitFile.fileName ())
                                       , tr ("Click OK to retry"));
          }
      }
  }

  //Create .lock so jtdxjt9 will wait
  QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.open(QIODevice::ReadWrite);

  QStringList jt9_args {
    "-s", QApplication::applicationName () // shared memory key,
                                           // includes rig
#ifdef NDEBUG
      , "-w", "1"               //FFTW patience - release
#else
      , "-w", "1"               //FFTW patience - debug builds for speed
#endif
      // The number  of threads for  FFTW specified here is  chosen as
      // three because  that gives  the best  throughput of  the large
      // FFTs used  in jt9.  The count  is the minimum of  (the number
      // available CPU threads less one) and 12 (was three).  This ensures that
      // there is always at least one free CPU thread to run the other
      // mode decoder in parallel.
      , "-m", QString::number (qMin (qMax (QThread::idealThreadCount () - 1, 1), 24)) //FFTW threads
//      , "-m", QString::number (qMin (qMax (QThread::idealThreadCount (), 1), 24)) //FFTW threads
      , "-e", QDir::toNativeSeparators (m_appDir)
      , "-a", QDir::toNativeSeparators (m_dataDir.absolutePath ())
      , "-t", QDir::toNativeSeparators (m_config.temp_dir ().absolutePath ())
      , "-r", QDir::toNativeSeparators (m_config.data_dir ().absolutePath ())
      };
  QProcessEnvironment new_env {m_env};
  /* CE3TSK: 24M, not the 10M this carried while only FT8 threaded. The FT4 slice loop gives every
     thread its own copy of the period's work space - the downsampler's FFT buffer and subtractft4's
     /heap8/ scratch above all - and measured on the 240-period hour the threaded FT4 decoder
     segfaults at 8M and only just survives at 10M. Sitting on the edge of a hard crash is no place
     to be, and the stacks are reserved rather than committed, so the margin costs nothing real.
     To be plain about what 24M rests on: the measurements are the two above, 8M fatal and 10M
     surviving on this machine at twelve threads. 24M is not a third measurement - it is a margin
     over the observed edge, chosen because more threads than twelve, or an FT4 preset adding
     per-thread work space, move that edge and the cost of being wrong is a mid-period crash.
     params_layout.sh pins TLS separately, under 5 MB, which is a different budget: TLS shares the
     thread's mapping with the stack but does not size it. */
  new_env.insert  ("OMP_STACKSIZE", "24M");
  proc_jtdxjt9.setProcessEnvironment (new_env);
  proc_jtdxjt9.start(QDir::toNativeSeparators (m_appDir) + QDir::separator () +
          "jtdxjt9", jt9_args, QIODevice::ReadWrite | QIODevice::Unbuffered);

  QString fname {QDir::toNativeSeparators(m_dataDir.absoluteFilePath ("wsjtx_wisdom.dat"))};
  QByteArray cfname=fname.toLocal8Bit();
  fftwf_import_wisdom_from_filename(cfname);

//  QThread::currentThread()->setPriority(QThread::HighPriority);

  connect (&m_wav_future_watcher, &QFutureWatcher<void>::finished, this, &MainWindow::diskDat);

#if JTDX_DEBUG_TO_FILE
  FILE * pFile = fopen (QDir(QStandardPaths::writableLocation (QStandardPaths::DataLocation)).absoluteFilePath ("jtdx_debug.txt").toStdString().c_str(),"a");  
  fprintf (pFile,"%s(%0.1f) JTDX v%s start, performance %d threads\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),m_jtdxtime->GetOffset(),
      (version() + (m_tci ? " tci " : " ") + revision()).toStdString().c_str(),QThread::idealThreadCount ());
  fclose (pFile);
#endif
//  printf ("is_tci: %d downsampleFactor %d\n",m_tci,m_downSampleFactor);
  if (m_tci) {
    Q_EMIT m_config.transceiver_period(double(NTMAX));
  } else {
    if (!m_config.audio_input_device ().isNull ()) {
      Q_EMIT startAudioInputStream (m_config.audio_input_device (), m_rx_audio_buffer_frames, m_detector, m_downSampleFactor, m_config.audio_input_channel ());
    }
    if (!m_config.audio_output_device ().isNull ()) {
      Q_EMIT initializeAudioOutputStream (m_config.audio_output_device (), AudioDevice::Mono == m_config.audio_output_channel () ? 1 : 2, m_tx_audio_buffer_frames);
    }
  }
//  if (m_tci) Q_EMIT m_config.transceiver_trfrequency(ui->TxFreqSpinBox->value () - m_XIT); 
//  else Q_EMIT transmitFrequency (ui->TxFreqSpinBox->value () - m_XIT);

  enable_DXCC_entity ();  // sets text window proportions and (re)inits the logbook
  if(m_config.monitor_off_at_startup()) m_monitoroff=true;

  // this must be done before initializing the mode as some modes need
  // to turn off split on the rig e.g. WSPR
  m_config.transceiver_online ();

  ui->TxMinuteButton->setChecked(m_txFirst);
  setMinButton();
  morse_(const_cast<char *> (m_config.my_callsign ().toLatin1().constData()),
         const_cast<int *> (icw), &m_ncw, m_config.my_callsign ().length());
  on_actionWide_Waterfall_triggered();
  m_wideGraph->setTol(500);
  m_wideGraph->setLockTxFreq(m_lockTxFreq);
  m_wideGraph->setMode(m_mode);
  m_wideGraph->setTopJT65(m_config.ntopfreq65());
  m_wideGraph->setModeTx(m_modeTx);
  
  minuteTimer.start (ms_minute_error (m_jtdxtime) + 60 * 1000);
  
  if(m_mode=="FT8") on_actionFT8_triggered();
  else if(m_mode=="FT4") on_actionFT4_triggered();
  else if(m_mode=="FT2") on_actionFT2_triggered();   // CE3TSK
  else if(m_mode=="JT9+JT65") on_actionJT9_JT65_triggered();
  else if(m_mode=="JT9") on_actionJT9_triggered();
  else if(m_mode=="JT65") on_actionJT65_triggered();
  else if(m_mode=="T10") on_actionT10_triggered();
  else if(m_mode=="WSPR-2") on_actionWSPR_2_triggered();

  if(m_mode!="FT8") { ui->actionEnable_hound_mode->setChecked(false); ui->actionEnable_hound_mode->setEnabled(false); }

  if (m_tci) Q_EMIT m_config.transceiver_trfrequency(ui->TxFreqSpinBox->value () - m_XIT);
  else Q_EMIT transmitFrequency (ui->TxFreqSpinBox->value () - m_XIT);
  ui->actionCallPriorityAndSearchCQ->setEnabled(m_callMode==2 || m_callMode==3);
  m_callPrioCQ=ui->actionCallPriorityAndSearchCQ->isChecked() && (m_callMode==2 || m_callMode==3);
  m_maxDistance=ui->actionMaxDistance->isChecked();
  m_answerWorkedB4=ui->actionAnswerWorkedB4->isChecked();
  m_callWorkedB4=ui->actionCallWorkedB4->isChecked();
  m_callHigherNewCall=ui->actionCallHigherNewCall->isChecked();
  m_singleshot=ui->actionSingleShot->isChecked();
  m_autofilter=ui->actionAutoFilter->isChecked();
  if(ui->actionEnable_hound_mode->isChecked()) on_actionEnable_hound_mode_toggled(true);
  m_wideGraph->setFilter(m_filter);
  ui->actionUse_TX_frequency_jumps->setEnabled(false);
  m_showHarmonics=ui->actionShow_messages_decoded_from_harmonics->isChecked();
  m_showMyCallMsgRxWindow=ui->actionMyCallRXFwindow->isChecked();
  m_showWantedCallRxWindow=ui->actionWantedCallRXFwindow->isChecked();
  m_FT8EarlyStart=ui->actionFT8EarlyStart->isChecked();
  m_bypassRxfFilters=ui->actionBypass_text_filters_on_RX_frequency->isChecked();
  m_bypassAllFilters=ui->actionBypass_all_text_filters->isChecked();
  m_windowPopup=ui->actionEnable_main_window_popup->isChecked();
  m_autoErase=ui->actionAutoErase->isChecked();
  m_autoEraseBC=ui->actionEraseWindowsAtBandChange->isChecked();

  m_rprtPriority=ui->actionReport_message_priority->isChecked();
  if(m_rprtPriority && m_maxDistance) { ui->actionMaxDistance->setChecked(false); m_maxDistance=false; }

  ui->sbTxPercent->setValue(m_pctx);
  ui->TxPowerComboBox->setCurrentIndex(int(0.3*(m_dBm + 30.0)+0.2));
  ui->cbUploadWSPR_Spots->setChecked(m_uploadSpots);
  
//  ui->pbTxLock->setChecked(m_lockTxFreq);
//  on_pbTxLock_clicked(m_lockTxFreq);

  if(m_mode.left(4)=="WSPR" and m_pctx>0)  {
    QPalette palette {ui->sbTxPercent->palette ()};
    palette.setColor(QPalette::Base,Qt::yellow);
    ui->sbTxPercent->setPalette(palette);
  }

  setStopHSym();
  
  progressBar->setMaximum(int(m_TRperiod));
  if (m_tci) Q_EMIT m_config.transceiver_period(m_TRperiod);
  else { m_modulator->setPeriod(m_TRperiod); m_detector->setPeriod(m_TRperiod); }// TODO - not thread safe
  connect( wsprNet, SIGNAL(uploadStatus(QString)), this, SLOT(uploadResponse(QString)));

//  setting up CQ message on init
  m_myCallCompound=(!m_config.my_callsign().isEmpty() && m_config.my_callsign().contains("/")); // && !m_config.my_callsign().endsWith("/P") && !m_config.my_callsign().endsWith("/R"));
  genStdMsgs(m_rpt);
  ui->txrb6->setChecked(true); ui->genMsg->setText(ui->tx6->text());
  if (0 == ui->tabWidget->currentIndex ()) { m_ntx=6; }
  else if (1 == ui->tabWidget->currentIndex ()) { m_ntx=7; }
  m_nlasttx = 6; m_QSOProgress = CALLING;
  ui->rbGenMsg->setChecked(true);
//
  if (!m_mode.startsWith ("WSPR")) countQSOs ();
  if(!ui->cbMenus->isChecked()) { ui->cbMenus->setChecked(true); ui->cbMenus->setChecked(false); }
  if(!ui->cbShowWanted->isChecked()) { ui->cbShowWanted->setChecked(true); ui->cbShowWanted->setChecked(false); }
  m_oldmode=m_mode;
  mode_label->setText(m_mode);
  m_lastloggedtime=m_jtdxtime->currentDateTimeUtc2().addSecs(-7*int(m_TRperiod));
  QFile f0 {m_dataDir.absoluteFilePath ("CALL3.TXT")};
  if(!f0.exists()) { 
  QFile f1 {m_config.data_dir ().absoluteFilePath ("CALL3.TXT")};
  f1.copy(m_dataDir.absoluteFilePath ("CALL3.TXT"));
  }
  m_lastDisplayFreq=m_lastMonitoredFrequency;
  m_bMyCallStd=stdCall(m_config.my_callsign ());
  if(!m_config.my_callsign().isEmpty()) toggle_skipTx1();
  ui->spotMsgLabel->setVisible(false); ui->spotEditLabel->setVisible(false);  ui->spotLineEdit->setVisible(false); ui->propEditLabel->setVisible(false); ui->propLineEdit->setVisible(false);
  ui->genStdMsgsPushButton->click ();
  ui->spotMsgLabel->setTextFormat(Qt::PlainText);
  m_mslastTX = m_jtdxtime->currentMSecsSinceEpoch2();
  m_multInst=QApplication::applicationName ().length()>4;
  foxgen_();

// make sure TX hash tables are filled in at first message transmission
  char message[38]="CQ QQ1QQQ KO85                       ";char msgsent[38]; char ft8msgbits[77];
  int i3=0; int n3=0; int ntxhash=1;
  genft8_(message,&i3,&n3,&ntxhash,msgsent,const_cast<char *> (ft8msgbits),const_cast<int *> (itone),37,37);

  m_bHisCallStd=stdCall(m_hisCall); styleChanged();
  m_config.fit_widget_size_limits ();   // CE3TSK: the pass the font setting ran before these widgets existed
  QTimer::singleShot (0, this, &MainWindow::offerRecommendedColors);   // CE3TSK: the one-time colour offer
  // this must be the last statement of constructor
  if (!m_valid) throw std::runtime_error {"Fatal initialization exception"};
}

//--------------------------------------------------- MainWindow destructor
MainWindow::~MainWindow()
{
  QString fname {QDir::toNativeSeparators(m_dataDir.absoluteFilePath ("wsjtx_wisdom.dat"))};
  QByteArray cfname=fname.toLocal8Bit();
  fftwf_export_wisdom_to_filename(cfname);
  {
    int nfft {-1};
    int ndim {1};
    int isign {1};
    int iform {1};
    // free FFT plan resources
    four2a_ (nullptr, &nfft, &ndim, &isign, &iform, 0);
  }
  fftwf_forget_wisdom ();
  fftwf_cleanup ();
  m_audioThread.quit ();
  m_audioThread.wait ();
  remove_child_from_event_filter (this);
}

//-------------------------------------------------------- writeSettings()
void MainWindow::writeSettings()
{
  m_settings->beginGroup("MainWindow");
  m_settings->setValue("geometry",saveGeometry ());
  m_settings->setValue("geometryMinHint",minimumSizeHint ());   // CE3TSK: see restoreMainGeometry ()
  m_settings->setValue("state",saveState ());
  m_settings->setValue("vertSplitter",ui->splitter->saveState());
  m_settings->setValue("MRUdir",m_path);
  m_settings->setValue("ConvertMRUdir",m_convertPath);   /* CE3TSK */
  m_settings->setValue("ConvertOutMRUdir",m_convertOutPath);   /* CE3TSK */
  m_settings->setValue("TxFirst",m_txFirst);
  m_settings->setValue("RRR/RR73",m_rrr);
  m_settings->setValue("CQdirection",m_cqdir);
  m_settings->setValue("DXcall",ui->dxCallEntry->text());
  m_settings->setValue("DXgrid",ui->dxGridEntry->text());
  m_settings->setValue("WantedCallCommaList",ui->wantedCall->text());
  m_settings->setValue("WantedCountryCommaList",ui->wantedCountry->text());
  m_settings->setValue("WantedPrefixCommaList",ui->wantedPrefix->text());
  m_settings->setValue("WantedGridCommaList",ui->wantedGrid->text());
  m_settings->setValue("FreeText",ui->freeTextMsg->currentText ());
  m_settings->setValue("ShowMenus",ui->cbMenus->isChecked());
  m_settings->setValue("ShowWanted",ui->cbShowWanted->isChecked());
  m_settings->endGroup();

  m_settings->beginGroup("Common");
  m_settings->setValue("FT8threads",m_ft8threads);
  m_settings->setValue("AcceptUDPReplyMessages",m_acceptUDP);
  m_settings->setValue("Mode",m_mode);
  m_settings->setValue("ModeTx",m_modeTx);
  m_settings->setValue("SaveWav",m_saveWav);
  m_settings->setValue("Language",m_lang);
  m_settings->setValue("CallMode",m_callMode);
  m_settings->setValue("CallPriorityCQ",ui->actionCallPriorityAndSearchCQ->isChecked());
  m_settings->setValue("MaxDistance",ui->actionMaxDistance->isChecked());
  m_settings->setValue("AnswerWorkedB4",ui->actionAnswerWorkedB4->isChecked());
  m_settings->setValue("CallWorkedB4",ui->actionCallWorkedB4->isChecked());
  m_settings->setValue("CallHigherNewCall",ui->actionCallHigherNewCall->isChecked());
  m_settings->setValue("SingleShotQSO",ui->actionSingleShot->isChecked());
  /* CE3TSK: the parked main window controls, so a restart mid contest still knows what to
     hand back. Written unconditionally; m_uiParkedValid says whether they mean anything. */
  m_settings->setValue("ContestUiParked",m_uiParkedValid);
  m_settings->setValue("ContestUiAutoTx",m_uiParked.autoTx);
  m_settings->setValue("ContestUiSkipTx1",m_uiParked.skipTx1);
  m_settings->setValue("ContestUiRRR",m_uiParked.rrr);
  m_settings->setValue("ContestUiMaxDistance",m_uiParked.maxDistance);
  m_settings->setValue("ContestUiRprtPriority",m_uiParked.rprtPriority);
  m_settings->setValue("ContestUiMode",m_uiParked.mode);
  m_settings->setValue("AutoFilter",ui->actionAutoFilter->isChecked());
  m_settings->setValue("EnableHoundMode",ui->actionEnable_hound_mode->isChecked());  
  m_settings->setValue("ShowHarmonics",ui->actionShow_messages_decoded_from_harmonics->isChecked());
  m_settings->setValue("HideFTContestMessages",ui->actionHide_FT_contest_messages->isChecked());
  m_settings->setValue("HideTelemetryMessages",ui->actionHide_telemetry_messages->isChecked());
  m_settings->setValue("HideFT8Dupes",ui->actionHide_FT8_dupe_messages->isChecked());
  m_settings->setValue("ShowMyCallMessagesRxWindow",ui->actionMyCallRXFwindow->isChecked());
  m_settings->setValue("ShowWantedCallMessagesRxWindow",ui->actionWantedCallRXFwindow->isChecked());
  m_settings->setValue("FT8Sensitivity",m_ft8Sensitivity);
  m_settings->setValue("FT8DecodeBandwidth",m_decodeBandwidth);   // CE3TSK
  m_settings->setValue("FT8TwoSlicings",m_ft8TwoSlicings);
  m_settings->setValue("FT8AltPass",m_ft8AltPass);
  m_settings->setValue("FT4AltPass",m_ft4AltPass);   // CE3TSK
  m_settings->setValue("FT4DeepOSD",m_ft4DeepOSD);   // CE3TSK item 58
  m_settings->setValue("FT4BgEnsemble",m_ft4BgEnsemble);   // CE3TSK item 59
  m_settings->setValue("FT4BgDepth",m_ft4BgDepth);   // CE3TSK item 69
  m_settings->setValue("FT4BgDeepOSD",m_ft4BgDeepOSD);
  m_settings->setValue("FT4BgAltPass",m_ft4BgAltPass);
  m_settings->setValue("FT4BgTwoSlicings",m_ft4BgTwoSlicings);
  m_settings->setValue("FT4BgResidual",m_ft4BgResidual);   // CE3TSK item 72
  m_settings->setValue("FT4Sensitivity",m_ft4Sens);
  m_settings->setValue("FT4BgSensitivity",m_ft4BgSens);   // CE3TSK item 73
  m_settings->setValue("FT4BgEnabled",m_ft4BgEnabled);   // CE3TSK item 74
  m_settings->setValue("FT4QSORXfreqSensitivity",m_ft4RXfSens);   // CE3TSK item 75
  m_settings->setValue("FT4BgQSORXfreqSensitivity",m_ft4BgRXfSens);
  m_settings->setValue("FT4EnsembleEffort",m_ft4Ensemble);
  m_settings->setValue("FT4RXBudget",m_ft4RXBudget);   // CE3TSK item 80: tenths
  /* CE3TSK step 5: FT2's own values, one key per field of the recipe. They are written under
     their own names rather than shared with FT4 because the two modes want different answers:
     FT2 decodes a 3.75 s period against a 0.58 s deadline. */
  m_settings->setValue("FT2Depth",m_ft2Recipe.depth);
  m_settings->setValue("FT2AltPass",m_ft2Recipe.alt);
  m_settings->setValue("FT2Ensemble",m_ft2Recipe.members);
  m_settings->setValue("FT2DeepOSD",m_ft2Recipe.deeposd);
  m_settings->setValue("FT2BgEnsemble",m_ft2Recipe.bg);
  m_settings->setValue("FT2BgDepth",m_ft2Recipe.bgdepth);
  m_settings->setValue("FT2BgDeepOSD",m_ft2Recipe.bgdeeposd);
  m_settings->setValue("FT2BgAltPass",m_ft2Recipe.bgalt);
  m_settings->setValue("FT2BgTwoSlicings",m_ft2Recipe.bgtwopass);
  m_settings->setValue("FT2Sensitivity",m_ft2Recipe.sens);
  m_settings->setValue("FT2BgResidual",m_ft2Recipe.bgresidual);
  m_settings->setValue("FT2BgSensitivity",m_ft2Recipe.bgsens);
  m_settings->setValue("FT2TwoSlicings",m_ft2Recipe.twopass);
  m_settings->setValue("FT2BgEnabled",m_ft2Recipe.bgon);
  m_settings->setValue("FT2RXfSens",m_ft2Recipe.rxf);
  m_settings->setValue("FT2BgRXfSens",m_ft2Recipe.bgrxf);
  m_settings->setValue("FT2RXBudget",m_ft2RXBudget);
  m_settings->setValue("FT2BgMargin",m_ft2BgMargin);
  m_settings->setValue("FT4BgMargin",m_ft4BgMargin);
  m_settings->setValue("FT4TwoSlicings",m_ft4TwoSlicings);
  m_settings->setValue("FT8EnsembleEffort",m_ft8EnsembleEffort);
  m_settings->setValue("FT8BackgroundEnabled",m_bgEnabled);   // CE3TSK: the TX background recipe
  m_settings->setValue("FT8BgSWLMode",m_bgSWL);
  m_settings->setValue("NFT8BgCycles",m_nBgCycles);
  m_settings->setValue("NFT8BgSWLCycles",m_nBgSWLCycles);
  m_settings->setValue("FT8BgSensitivity",m_bgSensitivity);
  m_settings->setValue("NFT8BgQSORXfreqSensitivity",m_bgRXfSens);
  m_settings->setValue("FT8BgDeepOSD",m_bgDeepOSD);
  m_settings->setValue("FT8BgTwoSlicings",m_bgTwoSlicings);
  m_settings->setValue("FT8BgAltPass",m_bgAltPass);
  m_settings->setValue("FT8BgClassicCycles",m_bgClassic);
  m_settings->setValue("FT8BgEnsembleEffort",m_bgEnsembleEffort);
  m_settings->setValue("FT8BackgroundMargin",m_ft8BackgroundMargin);
  m_settings->setValue("FT8RXBudget",m_ft8RXBudget);   // CE3TSK P8
  m_settings->setValue("FT8DeepOSD",m_ft8DeepOSD);
  m_settings->setValue("FT8DecoderEarlyStart",ui->actionFT8EarlyStart->isChecked());
  m_settings->setValue("FT8WideDXCallSearch",m_FT8WideDxCallSearch);
  m_settings->setValue("BypassRXFreqTextFilters",ui->actionBypass_text_filters_on_RX_frequency->isChecked());
  m_settings->setValue("BypassAllTextFilters",ui->actionBypass_all_text_filters->isChecked());
  m_settings->setValue("EnableMainwindowPopup",ui->actionEnable_main_window_popup->isChecked());
  m_settings->setValue("AutoErase",ui->actionAutoErase->isChecked());
  m_settings->setValue("EraseWindowsAtBandChange",ui->actionEraseWindowsAtBandChange->isChecked());
  m_settings->setValue("ReportMessagePriority",ui->actionReport_message_priority->isChecked());
  m_settings->setValue("NDepth",m_ndepth);
  m_settings->setValue("NCandidateListThinning",m_ncandthin);
  qint32 nDTcenter=100 * ui->DTCenterSpinBox->value();
  m_settings->setValue("DTCenterInt",nDTcenter);
  m_settings->setValue("NFT8Cycles",m_nFT8Cycles);
  m_settings->setValue("NFT8SWLCycles",m_nFT8SWLCycles);
  m_settings->setValue("NFT8QSORXfreqSensitivity",m_nFT8RXfSens);
  m_settings->setValue("NFT4Depth",m_nFT4depth);
  m_settings->setValue("SwitchFilterOff",m_FilterState);
  m_settings->setValue("RxFreq",ui->RxFreqSpinBox->value());
  m_settings->setValue("TxFreq",ui->TxFreqSpinBox->value());
  m_settings->setValue("WSPRfreq",ui->WSPRfreqSpinBox->value());
  m_settings->setValue("DialFreq",QVariant::fromValue(m_lastMonitoredFrequency));
  m_settings->setValue("OutAttenuation",m_outAttenuationRestored ? ui->outAttenuation->value ()
                                                                 : m_outAttenuation);   // CE3TSK
  m_settings->setValue("GUItab",ui->tabWidget->currentIndex());
  m_settings->setValue("LockTxFreq",m_lockTxFreq);
  m_settings->setValue("SkipTx1", m_skipTx1);
  m_settings->setValue("PctTx",m_pctx);
  m_settings->setValue("dBm",m_dBm);
  m_settings->setValue("UploadSpots",m_uploadSpots);
  m_settings->setValue("BandHopping",ui->band_hopping_group_box->isChecked ());
  m_settings->setValue("pwrBandTxMemory",m_pwrBandTxMemory);
  m_settings->setValue("pwrBandTuneMemory",m_pwrBandTuneMemory);
  m_settings->setValue("SWLMode",m_swl);
  m_settings->setValue("Filter",m_filter);
  m_settings->setValue("AGCcompensation",m_agcc);
  m_settings->setValue("Hint",m_hint);
  m_settings->setValue("73TxDisable",m_disable_TX_on_73);
  m_settings->setValue("ShowMainWindowTooltips",m_showTooltips);
  m_settings->setValue("ColorTxMessageButtons",m_colorTxMsgButtons);
  m_settings->setValue("BandButtons",ui->actionBand_buttons->isChecked ());   // CE3TSK
  m_settings->setValue("CallsignToClipboard",m_callToClipboard);
  m_settings->setValue("Crossband160mJA",m_crossbandOptionEnabled);
  m_settings->setValue("Crossband160mHL",m_crossbandHLOptionEnabled);
  m_settings->setValue("QuickCall",m_autoTx);
  m_settings->setValue("AutoSequence",m_autoseq);
  m_settings->setValue("SpotText",m_spotText);
  m_settings->setValue("ClearWCallAtLog",ui->cbClearCallsign->isChecked ());
  m_settings->setValue("ClearWGridAtLog",ui->cbClearGrid->isChecked ());
  m_settings->setValue("NoOwnCallWSPR",ui->cbNoOwnCall->isChecked());
  m_settings->setValue("SMeterUnits",ui->S_meter_button->isChecked());
  m_settings->endGroup();
}

/* CE3TSK: restore the saved main window geometry. A saved size can be below what the layout
   needs for one of two reasons. The operator narrowed the window on purpose: Qt lets it be
   dragged down to the .ui's 733x422, although the controls start to clip below 829 px at 11 pt.
   That is the operator's choice, so it reopens exactly as saved. Or it was saved under a smaller
   font, and then Qt would crush the children further than the operator ever saw: only this grows
   the window, by as much as the layout's minimum has risen since the save and never past that
   minimum, so a large window is left alone. Clamping to sizeHint () (971 px wide), and then to
   minimumSizeHint (), both threw a narrowed width away. See UI_DARK_STYLE.md. */
void MainWindow::restoreMainGeometry ()
{
  restoreGeometry (m_geometry);
  if (!m_geometryMinHint.isValid ()) return;   // saved before the minimum was recorded: as saved
  auto const needed = minimumSizeHint ();
  auto const growth = (needed - m_geometryMinHint).expandedTo (QSize {0, 0});
  resize (size ().expandedTo ((size () + growth).boundedTo (needed)));
}

//---------------------------------------------------------- readSettings()
void MainWindow::readSettings()
{
  m_settings->beginGroup("MainWindow");
  
  m_geometry = m_settings->value ("geometry",saveGeometry()).toByteArray();
  m_geometryMinHint = m_settings->value ("geometryMinHint").toSize ();   // CE3TSK
  restoreMainGeometry ();   // CE3TSK
  /* CE3TSK: mainwindow.ui pins ~30 widgets with hard pixel maximumSize caps chosen for the
     original font, so a larger application font cannot grow past them and the text is clipped
     ("Rx 305 Hz" loses the Hz, "GenMsgs" the s). Raise each cap to the widget's own sizeHint,
     which already accounts for font and content - at the design font every sizeHint is inside
     its cap, so the familiar layout is left untouched. Configuration::set_application_font does
     the same for a font changed at run time; this covers start-up, when the font is applied
     before this window exists. */
  for (auto* child : findChildren<QWidget *> ())
    {
      /* remember what the .ui asked for the first time we see the widget, and always work
         from that - otherwise a font increase ratchets the limits up and a later decrease
         cannot bring them back down, leaving the layout inflated until the next restart. */
      if (!child->property ("jtdxLimits").isValid ())
        {
          child->setProperty ("jtdxLimits", QRect {child->minimumWidth (), child->minimumHeight (),
                                                   child->maximumWidth (), child->maximumHeight ()});
        }
      auto const from_ui = child->property ("jtdxLimits").toRect ();
      auto const hint = child->sizeHint ();
      child->setMaximumWidth (from_ui.width () < QWIDGETSIZE_MAX
                              ? qMax (from_ui.width (), hint.width ()) : from_ui.width ());
      child->setMaximumHeight (from_ui.height () < QWIDGETSIZE_MAX
                               ? qMax (from_ui.height (), hint.height ()) : from_ui.height ());
      /* a button's label is the whole point of the button, so it must not be squeezed:
         GenMsgs is pinned at a 60px minimum and S meter is sized oddly by its own
         stylesheet, and both lost characters at a larger font. */
      if (qobject_cast<QAbstractButton *> (child))
        {
          child->setMinimumWidth (qMax (from_ui.x (), hint.width ()));
          child->setMinimumHeight (qMax (from_ui.y (), hint.height ()));
        }
    }
  restoreState (m_settings->value ("state",saveState ()).toByteArray ());
  ui->splitter->restoreState(m_settings->value("vertSplitter").toByteArray());
  m_path = m_settings->value("MRUdir",m_config.save_directory ().absolutePath ()).toString ();
  m_convertPath = m_settings->value("ConvertMRUdir",m_config.save_directory ().absolutePath ()).toString ();   /* CE3TSK */
  /* CE3TSK: empty until the operator saves a converted copy somewhere; while it is empty the
     copy is offered beside its source, which is where it belonged before this was remembered */
  m_convertOutPath = m_settings->value("ConvertOutMRUdir","").toString ();   /* CE3TSK */

  m_txFirst = m_settings->value("TxFirst",true).toBool();

  m_rrr = m_settings->value("RRR/RR73",false).toBool();
  ui->rrrCheckBox->setChecked(m_rrr);
  ui->rrr1CheckBox->setChecked(m_rrr);
  if(m_rrr) { ui->pbSendRRR->setText("RRR"); } 
  else { ui->pbSendRRR->setText("RR73"); }
  
  m_cqdir = m_settings->value("CQdirection","").toString();
  if(!m_cqdir.isEmpty()) {
    QRegularExpression cqdir_re("^[A-Za-z]{2,2}$"); QRegularExpressionMatch match = cqdir_re.match(m_cqdir);
    bool hasMatch = match.hasMatch(); if(!hasMatch) m_cqdir="";
  }
  ui->directionLineEdit->setText(m_cqdir);
  if(!m_cqdir.isEmpty ()) { ui->pbCallCQ->setText("CQ " + m_cqdir); }
  else { ui->pbCallCQ->setText("CQ"); }

  ui->dxCallEntry->setText(m_settings->value("DXcall","").toString());
  ui->dxGridEntry->setText(m_settings->value("DXgrid","").toString());
  ui->wantedCall->setText(m_settings->value("WantedCallCommaList","").toString());
  ui->wantedCountry->setText(m_settings->value("WantedCountryCommaList","").toString());
  ui->wantedPrefix->setText(m_settings->value("WantedPrefixCommaList","").toString());
  ui->wantedGrid->setText(m_settings->value("WantedGridCommaList","").toString());

  if(m_settings->contains ("FreeText")) ui->freeTextMsg->setCurrentText (m_settings->value ("FreeText").toString ());

  /* CE3TSK: the menus show unless the key says otherwise - the default is now written down here
     rather than implied by cbMenus being checked in the .ui, and a stored 0/1 counts as well as
     the literal "false" the old string comparison demanded. */
  if(!m_settings->value("ShowMenus",true).toBool()) { ui->cbMenus->setChecked(false); on_cbMenus_toggled(false); }
  else { ui->cbMenus->setChecked(true); on_cbMenus_toggled(true); }

  bool wanted=m_settings->value("ShowWanted",false).toBool(); m_wantedchkd=wanted; ui->cbShowWanted->setChecked(wanted);

  m_settings->endGroup();

  // do this outside of settings group because it uses groups internally
  m_settings->beginGroup("Common");

  m_ft8threads=m_settings->value("FT8threads",0).toInt();
  if(!(m_ft8threads>=0 && m_ft8threads<25)) m_ft8threads=0;
  if(m_ft8threads==0) ui->actionMTAuto->setChecked(true);
  else if(m_ft8threads==1) ui->actionMT1->setChecked(true);
  else if(m_ft8threads==2) ui->actionMT2->setChecked(true);
  else if(m_ft8threads==3) ui->actionMT3->setChecked(true);
  else if(m_ft8threads==4) ui->actionMT4->setChecked(true);
  else if(m_ft8threads==5) ui->actionMT5->setChecked(true);
  else if(m_ft8threads==6) ui->actionMT6->setChecked(true);
  else if(m_ft8threads==7) ui->actionMT7->setChecked(true);
  else if(m_ft8threads==8) ui->actionMT8->setChecked(true);
  else if(m_ft8threads==9) ui->actionMT9->setChecked(true);
  else if(m_ft8threads==10) ui->actionMT10->setChecked(true);
  else if(m_ft8threads==11) ui->actionMT11->setChecked(true);
  else if(m_ft8threads==12) ui->actionMT12->setChecked(true);
  else if(m_ft8threads==13) ui->actionMT13->setChecked(true);
  else if(m_ft8threads==14) ui->actionMT14->setChecked(true);
  else if(m_ft8threads==15) ui->actionMT15->setChecked(true);
  else if(m_ft8threads==16) ui->actionMT16->setChecked(true);
  else if(m_ft8threads==17) ui->actionMT17->setChecked(true);
  else if(m_ft8threads==18) ui->actionMT18->setChecked(true);
  else if(m_ft8threads==19) ui->actionMT19->setChecked(true);
  else if(m_ft8threads==20) ui->actionMT20->setChecked(true);
  else if(m_ft8threads==21) ui->actionMT21->setChecked(true);
  else if(m_ft8threads==22) ui->actionMT22->setChecked(true);
  else if(m_ft8threads==23) ui->actionMT23->setChecked(true);
  else if(m_ft8threads==24) ui->actionMT24->setChecked(true);

  m_acceptUDP=m_settings->value("AcceptUDPReplyMessages",3).toInt(); if(!(m_acceptUDP>=1 && m_acceptUDP<=3)) m_acceptUDP=3;
  if(m_acceptUDP==1) ui->actionAcceptUDPCQ->setChecked(true);
  else if(m_acceptUDP==2) ui->actionAcceptUDPCQ73->setChecked(true);
  else if(m_acceptUDP==3) ui->actionAcceptUDPAny->setChecked(true);

  m_mode=m_settings->value("Mode","FT8").toString();
  m_modeTx=m_settings->value("ModeTx","FT8").toString();

  if(!m_mode.startsWith("FT") && !m_mode.startsWith("JT") && m_mode!="T10" && !m_mode.startsWith ("WSPR")) {
     m_mode="FT8"; m_modeTx="FT8";
  }

  if(!m_modeTx.startsWith("FT") && !m_modeTx.startsWith("JT") && m_modeTx!="T10" && !m_modeTx.startsWith ("WSPR")) {
    if(m_mode=="FT8") m_modeTx="FT8";
	else if(m_mode=="FT4") m_modeTx="FT4";
    else if(m_mode=="FT2") m_modeTx="FT2";   // CE3TSK
    else if(m_mode=="JT9+JT65") m_modeTx="JT65";
    else if(m_mode=="JT65") m_modeTx="JT65";
    else if(m_mode=="JT9") m_modeTx="JT9";
    else if(m_mode=="T10") m_modeTx="T10";
    else if(m_modeTx.startsWith ("WSPR")) m_modeTx="WSPR-2";
  }
  if(m_modeTx.startsWith("JT9")) ui->pbTxMode->setText("Tx JT9  @");
  if(m_modeTx=="JT65") ui->pbTxMode->setText("Tx JT65  #");

  m_saveWav=m_settings->value("SaveWav",0).toInt();
  if(!(m_saveWav>=0 && m_saveWav<=2)) m_saveWav=0;  
  if(m_saveWav==0) ui->actionNone->setChecked(true);
  else if(m_saveWav==1) ui->actionSave_decoded->setChecked(true);
  else if(m_saveWav==2) ui->actionSave_all->setChecked(true);

  m_lang=m_settings->value("Language","en_US").toString();
  ui->actionEnglish->setText("English");
  ui->actionEstonian->setText("Eesti");
  ui->actionRussian->setText("Русский");
  ui->actionCatalan->setText("Català");
  ui->actionCroatian->setText("Hrvatski");
  ui->actionDanish->setText("Dansk");
  ui->actionDutch->setText("Nederlands");
  ui->actionSpanish->setText("Español");
  ui->actionSwedish->setText("Svenska");
  ui->actionFrench->setText("Français");
  ui->actionItalian->setText("Italiano");
  ui->actionGerman->setText("Deutsch");   // CE3TSK: each language names itself
  ui->actionLatvian->setText("Latviski");
  ui->actionHungarian->setText("Magyar");
  ui->actionPolish->setText("Polski");
  ui->actionPortuguese->setText("Português");
  ui->actionPortuguese_BR->setText("Português BR");
  ui->actionChinese_simplified->setText("简体中文");
  ui->actionChinese_traditional->setText("繁體中文");
  ui->actionJapanese->setText("日本語");
  ui->actionKorean->setText("한국어");   // CE3TSK: each language names itself
  set_language (m_lang);
  
  m_callMode=m_settings->value("CallMode",3).toInt();
  if(!(m_callMode>=0 && m_callMode<=3)) m_callMode=2; 
  if(m_callMode==0) ui->actionCallNone->setChecked(true);
  else if(m_callMode==1) ui->actionCallFirst->setChecked(true);
  else if(m_callMode==2) ui->actionCallMid->setChecked(true);
  else if(m_callMode==3) ui->actionCallEnd->setChecked(true);

  if(m_settings->value("CallPriorityCQ").toString()!="false" && m_settings->value("CallPriorityCQ").toString()!="true") 
    ui->actionCallPriorityAndSearchCQ->setChecked(false);
  else ui->actionCallPriorityAndSearchCQ->setChecked(m_settings->value("CallPriorityCQ",false).toBool());

  ui->actionMaxDistance->setChecked(m_settings->value("MaxDistance",false).toBool());
  ui->actionAnswerWorkedB4->setChecked(m_settings->value("AnswerWorkedB4",true).toBool());
  ui->actionCallWorkedB4->setChecked(m_settings->value("CallWorkedB4",true).toBool());
  ui->actionCallHigherNewCall->setChecked(m_settings->value("CallHigherNewCall",false).toBool());
  ui->actionSingleShot->setChecked(m_settings->value("SingleShotQSO",false).toBool());
  /* CE3TSK */
  m_uiParkedValid = m_settings->value("ContestUiParked",false).toBool();
  m_settings->remove("ContestUiHound");   /* CE3TSK: no longer parked */
  m_uiParked.autoTx = m_settings->value("ContestUiAutoTx",true).toBool();
  m_uiParked.skipTx1 = m_settings->value("ContestUiSkipTx1",true).toBool();
  m_uiParked.rrr = m_settings->value("ContestUiRRR",false).toBool();
  m_uiParked.maxDistance = m_settings->value("ContestUiMaxDistance",false).toBool();
  m_uiParked.rprtPriority = m_settings->value("ContestUiRprtPriority",false).toBool();
  /* CE3TSK: AutoSeq, call mode and priority CQ are no longer parked; drop the keys an
     earlier build may have written rather than leave them in the file for good. */
  m_settings->remove("ContestUiAutoSeq");
  m_settings->remove("ContestUiCallMode");
  m_settings->remove("ContestUiCallPrioCQ");
  m_settings->remove("ContestUiSingleShot");   /* CE3TSK: no longer parked */
  m_uiParked.mode = m_settings->value("ContestUiMode","").toString();
  ui->actionAutoFilter->setChecked(m_settings->value("AutoFilter",false).toBool());
  ui->actionEnable_hound_mode->setChecked(m_settings->value("EnableHoundMode",false).toBool());

  ui->actionShow_messages_decoded_from_harmonics->setChecked(m_settings->value("ShowHarmonics",false).toBool());

  if(m_settings->value("HideFTContestMessages").toString()!="false" && m_settings->value("HideFTContestMessages").toString()!="true") 
    ui->actionHide_FT_contest_messages->setChecked(false);
  else ui->actionHide_FT_contest_messages->setChecked(m_settings->value("HideFTContestMessages").toBool());

  if(m_settings->value("HideTelemetryMessages").toString()!="false" && m_settings->value("HideTelemetryMessages").toString()!="true") 
    ui->actionHide_telemetry_messages->setChecked(false);
  else ui->actionHide_telemetry_messages->setChecked(m_settings->value("HideTelemetryMessages").toBool());

  ui->actionHide_FT8_dupe_messages->setChecked(m_settings->value("HideFT8Dupes",true).toBool());
  ui->actionMyCallRXFwindow->setChecked(m_settings->value("ShowMyCallMessagesRxWindow",true).toBool());
  ui->actionWantedCallRXFwindow->setChecked(m_settings->value("ShowWantedCallMessagesRxWindow",true).toBool());

  m_ft8Sensitivity=m_settings->value("FT8Sensitivity",1).toInt();
  m_ft8DeepOSD=m_settings->value("FT8DeepOSD",false).toBool();   // CE3TSK: weak-signal OSD depth, off by default
  // CE3TSK: the decode bandwidth choice; the earlier boolean FT8FullBandDecode migrates once
  // a fresh configuration starts at 100-3100 Hz; a stored choice stays; the earlier boolean migrates to what it meant
  m_decodeBandwidth=m_settings->value("FT8DecodeBandwidth",
      m_settings->contains("FT8FullBandDecode") ? decode_bandwidth_from_full_band(m_settings->value("FT8FullBandDecode",true).toBool())
                                                : DECODE_BW_DEFAULT).toInt();
  m_decodeBandwidth=decode_bandwidth_stored(m_decodeBandwidth);   // CE3TSK: the rule lives in decoderange.h, pinned by test_range
  setDecodeBandwidthAction();
  ui->actionFT8DeepOSD->setChecked(m_ft8DeepOSD);   // CE3TSK
  m_ft8TwoSlicings=m_settings->value("FT8TwoSlicings",false).toBool();   // CE3TSK
  m_ft8AltPass=m_settings->value("FT8AltPass",false).toBool();
  m_ft4AltPass=m_settings->value("FT4AltPass",false).toBool(); ui->actionFT4AltPass->setChecked(m_ft4AltPass);   // CE3TSK
  m_ft4DeepOSD=m_settings->value("FT4DeepOSD",false).toBool(); ui->actionFT4DeepOSD->setChecked(m_ft4DeepOSD);   // CE3TSK item 58
  // CE3TSK items 59/73: -1 = auto. The bound is decodepreset.h's predicate, not a hand-written
  // 6: with the maximum in one place, raising it for a seventh member kind cannot leave the
  // receive setting accepting a value that this line silently resets to off at startup.
  m_ft4BgEnsemble=m_settings->value("FT4BgEnsemble",0).toInt(); if(m_ft4BgEnsemble!=FT4_ENSEMBLE_AUTO && !valid_ft4_ensemble(m_ft4BgEnsemble)) m_ft4BgEnsemble=0;
  // CE3TSK item 69: the TX background's own settings, defaulting to the RX phase's defaults
  m_ft4BgDepth=m_settings->value("FT4BgDepth",3).toInt(); if(m_ft4BgDepth<1||m_ft4BgDepth>3) m_ft4BgDepth=3;
  m_ft4BgDeepOSD=m_settings->value("FT4BgDeepOSD",false).toBool(); ui->actionFT4BgDeepOSD->setChecked(m_ft4BgDeepOSD);
  m_ft4BgAltPass=m_settings->value("FT4BgAltPass",false).toBool(); ui->actionFT4BgAltPass->setChecked(m_ft4BgAltPass);
  m_ft4BgTwoSlicings=m_settings->value("FT4BgTwoSlicings",true).toBool(); ui->actionFT4BgTwoSlicings->setChecked(m_ft4BgTwoSlicings);
  m_ft4BgResidual=m_settings->value("FT4BgResidual",false).toBool(); ui->actionFT4BgResidual->setChecked(m_ft4BgResidual);   // CE3TSK item 72
  m_ft4Sens=m_settings->value("FT4Sensitivity",0).toInt(); if(m_ft4Sens<0||m_ft4Sens>1) m_ft4Sens=0; setFT4SensAction();
  m_ft4BgSens=m_settings->value("FT4BgSensitivity",0).toInt(); if(m_ft4BgSens<0||m_ft4BgSens>1) m_ft4BgSens=0; setFT4BgSensAction();   // CE3TSK item 73
  // CE3TSK item 74: the explicit switch, as FT8BackgroundEnabled; no key yet = on when a member target was stored (item 59's rule)
  m_ft4BgEnabled=m_settings->value("FT4BgEnabled",m_ft4BgEnsemble!=0).toBool(); ui->actionFT4BgEnabled->setChecked(m_ft4BgEnabled);
  m_ft4RXfSens=m_settings->value("FT4QSORXfreqSensitivity",RXF_MEDIUM).toInt(); if(m_ft4RXfSens<0||m_ft4RXfSens>3) m_ft4RXfSens=RXF_MEDIUM;   // CE3TSK item 75
  m_ft4BgRXfSens=m_settings->value("FT4BgQSORXfreqSensitivity",RXF_MEDIUM).toInt(); if(m_ft4BgRXfSens<0||m_ft4BgRXfSens>3) m_ft4BgRXfSens=RXF_MEDIUM;
  setFT4RXfActions();
  setFT4BgEffortAction();
  /* CE3TSK: FT4 ensemble members, migrating the FT4Dither boolean of the single-member days
     the way FT8EnsembleEffort migrates FT8Ensemble (decodepreset.h) */
  m_ft4Ensemble=ft4_ensemble_from_settings(m_settings->value("FT4EnsembleEffort",FT4_ENSEMBLE_NO_KEY).toInt(),   // item 73: -1 is auto now
                                           m_settings->value("FT4Dither",false).toBool());
  setFT4EnsembleAction();
  setFT4BgEnsembleAction();
  // CE3TSK item 80: FT8's P8 budget and P7 margin, FT4's own values - the RX budget against the 1.36 s reply
  // deadline, the margin the background leaves before the next decode (the max effort preset sets 0.5 s)
  m_ft4RXBudget=m_settings->value("FT4RXBudget",13).toInt(); if(!(m_ft4RXBudget>=5 && m_ft4RXBudget<=600)) m_ft4RXBudget=13;
  {   // CE3TSK step 5: FT2's own settings, defaulting to its recommended preset on a profile without them
    FT4Recipe const d = ft2_preset_recipe (FT2Preset::Recommended);
    m_ft2Recipe.depth=m_settings->value("FT2Depth",d.depth).toInt();
    if(!(m_ft2Recipe.depth>=1 && m_ft2Recipe.depth<=3)) m_ft2Recipe.depth=d.depth;
    m_ft2Recipe.alt=m_settings->value("FT2AltPass",d.alt).toBool();
    m_ft2Recipe.members=m_settings->value("FT2Ensemble",d.members).toInt();
    if(!valid_ft4_ensemble(m_ft2Recipe.members) && m_ft2Recipe.members!=FT4_ENSEMBLE_AUTO
       && m_ft2Recipe.members!=FT4_ENSEMBLE_BUDGET) m_ft2Recipe.members=d.members;
    m_ft2Recipe.deeposd=m_settings->value("FT2DeepOSD",d.deeposd).toBool();
    m_ft2Recipe.bg=m_settings->value("FT2BgEnsemble",d.bg).toInt();
    if(!valid_ft4_ensemble(m_ft2Recipe.bg) && m_ft2Recipe.bg!=FT4_ENSEMBLE_AUTO) m_ft2Recipe.bg=d.bg;
    m_ft2Recipe.bgdepth=m_settings->value("FT2BgDepth",d.bgdepth).toInt();
    if(!(m_ft2Recipe.bgdepth>=0 && m_ft2Recipe.bgdepth<=3)) m_ft2Recipe.bgdepth=d.bgdepth;
    m_ft2Recipe.bgdeeposd=m_settings->value("FT2BgDeepOSD",d.bgdeeposd).toBool();
    m_ft2Recipe.bgalt=m_settings->value("FT2BgAltPass",d.bgalt).toBool();
    m_ft2Recipe.bgtwopass=m_settings->value("FT2BgTwoSlicings",d.bgtwopass).toBool();
    m_ft2Recipe.sens=m_settings->value("FT2Sensitivity",d.sens).toInt();
    if(!(m_ft2Recipe.sens>=0 && m_ft2Recipe.sens<=1)) m_ft2Recipe.sens=d.sens;
    m_ft2Recipe.bgresidual=m_settings->value("FT2BgResidual",d.bgresidual).toBool();
    m_ft2Recipe.bgsens=m_settings->value("FT2BgSensitivity",d.bgsens).toInt();
    if(!(m_ft2Recipe.bgsens>=0 && m_ft2Recipe.bgsens<=1)) m_ft2Recipe.bgsens=d.bgsens;
    m_ft2Recipe.twopass=m_settings->value("FT2TwoSlicings",d.twopass).toBool();
    m_ft2Recipe.bgon=m_settings->value("FT2BgEnabled",d.bgon).toBool();
    m_ft2Recipe.rxf=m_settings->value("FT2RXfSens",d.rxf).toInt();
    if(!(m_ft2Recipe.rxf>=0 && m_ft2Recipe.rxf<=3)) m_ft2Recipe.rxf=d.rxf;
    m_ft2Recipe.bgrxf=m_settings->value("FT2BgRXfSens",d.bgrxf).toInt();
    if(!(m_ft2Recipe.bgrxf>=0 && m_ft2Recipe.bgrxf<=3)) m_ft2Recipe.bgrxf=d.bgrxf;
    m_ft2RXBudget=m_settings->value("FT2RXBudget",FT2_RX_BUDGET_DEFAULT).toInt();
    if(!(m_ft2RXBudget>=3 && m_ft2RXBudget<=600)) m_ft2RXBudget=FT2_RX_BUDGET_DEFAULT;
    m_ft2BgMargin=m_settings->value("FT2BgMargin",FT2_BG_MARGIN_DEFAULT).toInt();
    if(!(m_ft2BgMargin>=1 && m_ft2BgMargin<=100)) m_ft2BgMargin=FT2_BG_MARGIN_DEFAULT;
  }
  m_ft4BgMargin=m_settings->value("FT4BgMargin",10).toInt(); if(m_ft4BgMargin<0 || m_ft4BgMargin>100) m_ft4BgMargin=10;
  /* CE3TSK: FT4's second slicing pass defaults ON, where FT8's FT8TwoSlicings defaults off.
     FT4's signals are wide against a slice, so without the offset pass a threaded run loses
     75-118 messages of 1716; with it, 21-24 (FT4_THREADING_PLAN.md). */
  m_ft4TwoSlicings=m_settings->value("FT4TwoSlicings",true).toBool();
  ui->actionFT4TwoSlicings->setChecked(m_ft4TwoSlicings);
  ui->actionFT8TwoSlicings->setChecked(m_ft8TwoSlicings);
  ui->actionFT8AltPass->setChecked(m_ft8AltPass);
  // CE3TSK: ensemble effort: -1 auto (by thread count), 0 off, 1-5 members; the earlier boolean
  // FT8Ensemble key, if present, becomes auto
  m_ft8EnsembleEffort=m_settings->value("FT8EnsembleEffort",m_settings->value("FT8Ensemble",false).toBool() ? -1 : 0).toInt();
  if(!valid_ensemble_effort(m_ft8EnsembleEffort)) m_ft8EnsembleEffort=0;   // CE3TSK P8: the rule (with budget auto) lives in decodepreset.h, pinned by test_preset
  setEnsembleEffortAction();
  // CE3TSK: pipeline ensemble - the TX background phase and its own recipe (defaults: the
  // pipeline presets' background); an earlier FT8BackgroundEffort key other than 0 means enabled
  m_bgEnabled=m_settings->value("FT8BackgroundEnabled",m_settings->value("FT8BackgroundEffort",0).toInt()!=0).toBool();
  m_bgSWL=m_settings->value("FT8BgSWLMode",true).toBool();
  m_nBgCycles=m_settings->value("NFT8BgCycles",3).toInt(); if(!(m_nBgCycles>=3 && m_nBgCycles<=9)) m_nBgCycles=3;
  m_nBgSWLCycles=m_settings->value("NFT8BgSWLCycles",5).toInt(); if(!(m_nBgSWLCycles>=3 && m_nBgSWLCycles<=9)) m_nBgSWLCycles=5;
  m_bgSensitivity=m_settings->value("FT8BgSensitivity",1).toInt(); if(!(m_bgSensitivity>=0 && m_bgSensitivity<=2)) m_bgSensitivity=1;
  m_bgRXfSens=m_settings->value("NFT8BgQSORXfreqSensitivity",2).toInt(); if(!(m_bgRXfSens>=1 && m_bgRXfSens<=3)) m_bgRXfSens=2;
  m_bgDeepOSD=m_settings->value("FT8BgDeepOSD",false).toBool();
  m_bgTwoSlicings=m_settings->value("FT8BgTwoSlicings",false).toBool();
  m_bgAltPass=m_settings->value("FT8BgAltPass",true).toBool();
  m_bgClassic=m_settings->value("FT8BgClassicCycles",CLASSIC_CYCLES).toInt();   // P9: 0 off, else 3-9 cycles
  if(!valid_classic_cycles(m_bgClassic)) m_bgClassic=CLASSIC_CYCLES;
  m_bgEnsembleEffort=m_settings->value("FT8BgEnsembleEffort",ENSEMBLE_AUTO).toInt();
  if(m_bgEnsembleEffort<-1 || m_bgEnsembleEffort>ENSEMBLE_MAX_MEMBERS) m_bgEnsembleEffort=ENSEMBLE_AUTO;
  m_ft8BackgroundMargin=m_settings->value("FT8BackgroundMargin",10).toInt();
  m_ft8RXBudget=m_settings->value("FT8RXBudget",27).toInt(); if(!(m_ft8RXBudget>=5 && m_ft8RXBudget<=600)) m_ft8RXBudget=27;   // CE3TSK P8: tenths
  if(m_ft8BackgroundMargin<0 || m_ft8BackgroundMargin>100) m_ft8BackgroundMargin=10;
  m_rxLag=0.0; m_rxLagKnown=false; m_bgLastCut=false; m_bgLastKnown=false;   // CE3TSK: nothing decoded yet
  m_rxPhase=false; m_bgPhase=false; m_bgRan=false; m_bgCut=false; m_bgAbortAsked=false; m_nDecodesRx=0; m_txPeriod=-1;
  setBackgroundActions();
  setDecodeMenuColours();
  if(!(m_ft8Sensitivity>=0 && m_ft8Sensitivity<=2)) m_ft8Sensitivity=0;
  if(m_ft8Sensitivity==0) ui->actionFT8SensMin->setChecked(true);
  else if(m_ft8Sensitivity==1) ui->actionlowFT8thresholds->setChecked(true);
  else if(m_ft8Sensitivity==2) ui->actionFT8subpass->setChecked(true);

  if(m_settings->value("FT8DecoderEarlyStart").toString()!="false" && m_settings->value("FT8DecoderEarlyStart").toString()!="true") 
    ui->actionFT8EarlyStart->setChecked(true);
  else ui->actionFT8EarlyStart->setChecked(m_settings->value("FT8DecoderEarlyStart",false).toBool());

  m_FT8WideDxCallSearch=m_settings->value("FT8WideDXCallSearch",true).toBool();
  ui->actionFT8WidebandDXCallSearch->setChecked(m_FT8WideDxCallSearch);

  ui->actionBypass_text_filters_on_RX_frequency->setChecked(m_settings->value("BypassRXFreqTextFilters",true).toBool());
  ui->actionBypass_all_text_filters->setChecked(m_settings->value("BypassAllTextFilters",true).toBool());
  ui->actionEnable_main_window_popup->setChecked(m_settings->value("EnableMainwindowPopup",false).toBool());
  ui->actionAutoErase->setChecked(m_settings->value("AutoErase",false).toBool());
  ui->actionEraseWindowsAtBandChange->setChecked(m_settings->value("EraseWindowsAtBandChange",true).toBool());
  ui->actionReport_message_priority->setChecked(m_settings->value("ReportMessagePriority",false).toBool());
  /* CE3TSK: a restart already in contest mode forces nothing (the values were forced before the
     restart and persisted) - except a control that a newer build forces than the one that parked:
     Max distance / report priority came later, so they are forced off here, after both are read. */
  if (m_wwDigi && m_uiParkedValid) {
    if (ui->actionMaxDistance->isChecked()) ui->actionMaxDistance->setChecked(false);
    if (ui->actionReport_message_priority->isChecked()) ui->actionReport_message_priority->setChecked(false);
  }

  m_ndepth=m_settings->value("NDepth",3).toInt(); if(!(m_ndepth>=1 && m_ndepth<=3)) m_ndepth=3;
  if(m_ndepth==3) ui->actionDeepestDecode->setChecked(true);
  else if(m_ndepth==2) ui->actionMediumDecode->setChecked(true);
  else if(m_ndepth==1) ui->actionQuickDecode->setChecked(true);

  ui->candListSpinBox->setValue(5); // ensure a change is signaled
  if(m_settings->value("NCandidateListThinning").toInt()>=5 && m_settings->value("NCandidateListThinning").toInt()<=100)
    ui->candListSpinBox->setValue(m_settings->value("NCandidateListThinning",100).toInt());
  else ui->candListSpinBox->setValue(100);

  ui->DTCenterSpinBox->setValue(0.1); // ensure a change is signaled
  qint32 nDTcenter=m_settings->value("DTCenterInt",0).toInt();
  if(nDTcenter>-201 && nDTcenter<251) ui->DTCenterSpinBox->setValue(static_cast<double>(nDTcenter/100.));
  else ui->DTCenterSpinBox->setValue(0.0);

  m_nFT8Cycles=m_settings->value("NFT8Cycles",3).toInt(); if(!(m_nFT8Cycles>=3 && m_nFT8Cycles<=9)) m_nFT8Cycles=3;
  switch (m_nFT8Cycles) {
    case 3:
        ui->actionDecFT8cycles3->setChecked(true);
        break;
    case 4:
        ui->actionDecFT8cycles4->setChecked(true);
        break;
    case 5:
        ui->actionDecFT8cycles5->setChecked(true);
        break;
    case 6:
        ui->actionDecFT8cycles6->setChecked(true);
        break;
    case 7:
        ui->actionDecFT8cycles7->setChecked(true);
        break;
    case 8:
        ui->actionDecFT8cycles8->setChecked(true);
        break;
    case 9:
        ui->actionDecFT8cycles9->setChecked(true);
        break;
    default:
       ui->actionDecFT8cycles3->setChecked(true);
  }

  m_nFT8SWLCycles=m_settings->value("NFT8SWLCycles",3).toInt(); if(!(m_nFT8SWLCycles>=3 && m_nFT8SWLCycles<=9)) m_nFT8SWLCycles=3;
  switch (m_nFT8SWLCycles) {
    case 3:
        ui->actionDecFT8SWLcycles3->setChecked(true);
        break;
    case 4:
        ui->actionDecFT8SWLcycles4->setChecked(true);
        break;
    case 5:
        ui->actionDecFT8SWLcycles5->setChecked(true);
        break;
    case 6:
        ui->actionDecFT8SWLcycles6->setChecked(true);
        break;
    case 7:
        ui->actionDecFT8SWLcycles7->setChecked(true);
        break;
    case 8:
        ui->actionDecFT8SWLcycles8->setChecked(true);
        break;
    case 9:
        ui->actionDecFT8SWLcycles9->setChecked(true);
        break;
    default:
       ui->actionDecFT8SWLcycles3->setChecked(true);
  }

  m_nFT8RXfSens=m_settings->value("NFT8QSORXfreqSensitivity",2).toInt(); if(!(m_nFT8RXfSens>=1 && m_nFT8RXfSens<=3)) m_nFT8RXfSens=1;
  if(m_nFT8RXfSens==1) ui->actionRXfLow->setChecked(true);
  else if(m_nFT8RXfSens==2) ui->actionRXfMedium->setChecked(true);
  else if(m_nFT8RXfSens==3) ui->actionRXfHigh->setChecked(true);
  
  m_nFT4depth=m_settings->value("NFT4Depth",3).toInt(); if(!(m_nFT4depth>=1 && m_nFT4depth<=3)) m_nFT4depth=3;
  if(m_nFT4depth==3) ui->actionFT4deep->setChecked(true);
  else if(m_nFT4depth==2) ui->actionFT4medium->setChecked(true);
  else if(m_nFT4depth==1) ui->actionFT4fast->setChecked(true);
  
  m_FilterState=m_settings->value("SwitchFilterOff",0).toInt(); if(!(m_FilterState>=0 && m_FilterState<=2)) m_FilterState=0;
  if(m_FilterState==0) on_actionAutoFilter_toggled(false);
  else if(m_FilterState==1) { ui->actionSwitch_Filter_OFF_at_sending_73->setChecked(true); on_actionSwitch_Filter_OFF_at_sending_73_triggered(true); }
  else if(m_FilterState==2) { ui->actionSwitch_Filter_OFF_at_getting_73->setChecked(true); on_actionSwitch_Filter_OFF_at_getting_73_triggered(true); }

  ui->RxFreqSpinBox->setValue(100); // ensure a change is signaled
  if(m_settings->value("RxFreq").toInt()>=0 && m_settings->value("RxFreq").toInt()<=5000)
    ui->RxFreqSpinBox->setValue(m_settings->value("RxFreq",1500).toInt());
  else ui->RxFreqSpinBox->setValue(1500);

  ui->WSPRfreqSpinBox->setValue(1400); // ensure a change is signaled
  if(m_settings->value("WSPRfreq").toInt()>=1400 && m_settings->value("WSPRfreq").toInt()<=1600)
    ui->WSPRfreqSpinBox->setValue(m_settings->value("WSPRfreq",1500).toInt());
  else ui->WSPRfreqSpinBox->setValue(1500);

  ui->TxFreqSpinBox->setValue(100); // ensure a change is signaled
  if(m_settings->value("TxFreq").toInt()>=100 && m_settings->value("TxFreq").toInt()<=5000)
    ui->TxFreqSpinBox->setValue(m_settings->value("TxFreq",1500).toInt());
  else ui->TxFreqSpinBox->setValue(1500);

  m_lastMonitoredFrequency = m_settings->value ("DialFreq",
     QVariant::fromValue<Frequency> (default_frequency)).value<Frequency> ();
  // setup initial value of tx attenuator, range 0...450 (0...45dB attenuation)
  if(m_settings->value("OutAttenuation").toInt()>=0 && m_settings->value("OutAttenuation").toInt()<=450)
    m_outAttenuation = m_settings->value ("OutAttenuation", 225).toInt ();

  int n=m_settings->value("GUItab",0).toInt(); if(!(n>=0 && n<=1)) n=0; ui->tabWidget->setCurrentIndex(n);

  m_lockTxFreq=m_settings->value("LockTxFreq",false).toBool();

  m_skipTx1=m_settings->value("SkipTx1",true).toBool();
  ui->skipTx1->setChecked(m_skipTx1);
  ui->skipGrid->setChecked(m_skipTx1);

  m_pctx=m_settings->value("PctTx",20).toInt(); if(!(m_pctx>=0 && m_pctx<=100)) m_pctx=20;
  m_dBm=m_settings->value("dBm",37).toInt(); if(!(m_dBm>=-30 && m_dBm<=60)) m_dBm=37;

  m_uploadSpots=m_settings->value("UploadSpots",false).toBool();
  
  ui->band_hopping_group_box->setChecked (m_settings->value ("BandHopping", false).toBool());
  m_pwrBandTxMemory=m_settings->value("pwrBandTxMemory").toHash();
  m_pwrBandTuneMemory=m_settings->value("pwrBandTuneMemory").toHash();

  m_swl=m_settings->value("SWLMode",false).toBool();
  ui->swlButton->setChecked(m_swl);
  
  m_filter=m_settings->value("Filter",false).toBool();
  ui->filterButton->setChecked(m_filter);

  m_agcc=m_settings->value("AGCcompensation",false).toBool();
  ui->AGCcButton->setChecked(m_agcc);
  
  m_hint=m_settings->value("Hint",true).toBool();
  ui->hintButton->setChecked(m_hint);
  
  m_disable_TX_on_73=m_settings->value("73TxDisable",false).toBool();
  ui->actionDisableTx73->setChecked(m_disable_TX_on_73);

  readSequencerSettings ();   /* CE3TSK: whatever the sequencer policy keeps in the ini */


  /* CE3TSK: tooltips on by default - a fresh install shows them, an existing profile keeps
     whatever it saved because the key is then present in the ini. Seeded on the first run so
     the state is written down rather than implied by a default no one can see. */
  if (!m_settings->contains ("ShowMainWindowTooltips")) m_settings->setValue ("ShowMainWindowTooltips", true);
  m_showTooltips=m_settings->value("ShowMainWindowTooltips",true).toBool();
  ui->actionShow_tooltips_main_window->setChecked(m_showTooltips);

  m_colorTxMsgButtons=m_settings->value("ColorTxMessageButtons",true).toBool();
  ui->actionColor_Tx_message_buttons->setChecked(m_colorTxMsgButtons);

  ui->actionBand_buttons->setChecked (m_settings->value ("BandButtons", false).toBool ());   // CE3TSK
  ui->bandButtonsWidget->setVisible (ui->actionBand_buttons->isChecked ());

  m_callToClipboard=m_settings->value("CallsignToClipboard",true).toBool();
  ui->actionCallsign_to_clipboard->setChecked(m_callToClipboard);

  m_crossbandOptionEnabled=m_settings->value("Crossband160mJA",false).toBool();
  ui->actionCrossband_160m_JA->setChecked(m_crossbandOptionEnabled);

  m_crossbandHLOptionEnabled=m_settings->value("Crossband160mHL",true).toBool();
  ui->actionCrossband_160m_HL->setChecked(m_crossbandHLOptionEnabled);

  m_autoTx=m_settings->value("QuickCall",true).toBool();
  ui->AutoTxButton->setChecked(m_autoTx);

  m_autoseq=m_settings->value("AutoSequence",true).toBool();
  if (m_autoseq) { clearDXfields(""); enableTab1TXRB(false); }
  ui->AutoSeqButton->setChecked(m_autoseq);
  setAutoSeqButtonStyle(m_autoseq);

  m_spotText=m_settings->value("SpotText","").toString();
  ui->spotLineEdit->setText(m_spotText);

  ui->cbClearCallsign->setChecked (m_settings->value ("ClearWCallAtLog",true).toBool());
  ui->cbClearGrid->setChecked (m_settings->value ("ClearWGridAtLog",true).toBool());
  ui->cbNoOwnCall->setChecked(m_settings->value("NoOwnCallWSPR",false).toBool());

  ui->S_meter_button->setChecked (m_settings->value ("SMeterUnits",true).toBool());

  m_settings->endGroup();
  
  if(m_config.do_snr()) {
    if(ui->S_meter_button->isChecked()) ui->S_meter_button->setText("S0");
    else ui->S_meter_button->setText("dBm");
  }
  else ui->S_meter_button->setText(tr("S meter"));
  ui->S_meter_button->setEnabled(m_config.do_snr());
  dec_data.params.nstophint=1;
  m_nlasttx=0;
  m_delay=0;
}

void MainWindow::setDecodedTextFont (QFont const& font)
{
  ui->decodedTextBrowser->setContentFont (font);
  ui->decodedTextBrowser2->setContentFont (font);
  QFontMetrics fm(font);
  auto style_sheet = "QLabel {" + font_as_stylesheet (font) + '}';
  
  ui->decodedTextLabel->setStyleSheet (ui->decodedTextLabel->styleSheet () + style_sheet);
  ui->decodedTextLabel->setMinimumHeight (fm.height());
  ui->decodedTextLabel->setMaximumHeight (fm.height());
  ui->decodedTextLabel2->setStyleSheet (ui->decodedTextLabel2->styleSheet () + style_sheet);
  ui->decodedTextLabel2->setMinimumHeight (fm.height());
  ui->decodedTextLabel2->setMaximumHeight (fm.height());
  fitDecodeLabels ();   // CE3TSK: rich text in a CJK language needs more than the metric
  updateGeometry ();
}

void MainWindow::setStopHSym()
{
  m_hsymStop=179;
  if(m_mode=="FT8") {
    if(m_swl) m_hsymStop = 51;
    else if(m_FT8EarlyStart) m_hsymStop = 48;
    else m_hsymStop=49;
  }
  else if(m_mode=="FT4") m_hsymStop=21;
  else if(m_mode=="FT2") m_hsymStop=22;   // CE3TSK: 22*1728 = 38016 samples of a 45000-sample period
  else if(m_mode.startsWith("JT") or m_mode=="T10") { m_hsymStop=173; if(m_config.decode_at_52s()) m_hsymStop=179; }
  else if(m_mode.startsWith ("WSPR")) m_hsymStop=396;
}

// init labUTC clock stylesheet at SW start and operation
void MainWindow::setClockStyle(bool reset)
{
  QDateTime t = m_jtdxtime->currentDateTimeUtc2();
  QString minute = t.time().toString("mm");
  QString second = t.time().toString("ss");
  QString secms = t.time().toString("ss.zzz");
  secms.remove(2,1); int ft4int = secms.toInt()/7500;
  int ft2int = secms.toInt()/3750;   // CE3TSK: FT2's period index inside the minute, 0..15

  if(m_start || reset) {
    if(m_mode.startsWith("FT")) {
      if(m_mode=="FT8") {
		int isecond = second.toInt();
		if((isecond >= 0 &&  isecond < 15) || (isecond >= 30 &&  isecond < 45)) ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color : %2").arg(Radio::convert_dark("#96ffff",m_useDarkStyle),Radio::convert_dark("#1400b1",m_useDarkStyle)));
        else ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color: %2").arg(Radio::convert_dark("#ffff96",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
      }
      else if(m_mode=="FT2") {   // CE3TSK: the same alternation, at FT2's rate
        if(ft2int%2==0) ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color : %2").arg(Radio::convert_dark("#96ffff",m_useDarkStyle),Radio::convert_dark("#1400b1",m_useDarkStyle)));
        else ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color: %2").arg(Radio::convert_dark("#ffff96",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
      }
      else if(m_mode=="FT4") {
        if(ft4int==0 || ft4int==2 || ft4int==4 || ft4int==6) ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color : %2").arg(Radio::convert_dark("#96ffff",m_useDarkStyle),Radio::convert_dark("#1400b1",m_useDarkStyle)));
        else ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color: %2").arg(Radio::convert_dark("#ffff96",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
      }
    }
    else if(!m_mode.startsWith ("WSPR")) {
      if((minute.toInt())%2==0)	ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color : %2").arg(Radio::convert_dark("#96ffff",m_useDarkStyle),Radio::convert_dark("#1400b1",m_useDarkStyle)));
      else ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color: %2").arg(Radio::convert_dark("#ffff96",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
    }

    if(m_mode.startsWith ("WSPR")) {
      ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color: %2").arg(Radio::convert_dark("#ffff96",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
      ui->TxMinuteButton->setText("N/A");
      ui->TxMinuteButton->setStyleSheet(QString("background: %1").arg(Radio::convert_dark("#d2d2d2",m_useDarkStyle))); 
    }
    m_start=false;
  }
  else {
    if(m_mode=="FT8") {
      if(second=="00" || second=="30") ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color : %2").arg(Radio::convert_dark("#96ffff",m_useDarkStyle),Radio::convert_dark("#1400b1",m_useDarkStyle)));
	  else if(second=="15" || second=="45") ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color: %2").arg(Radio::convert_dark("#ffff96",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
	}
    else if(m_mode=="FT4") {
        if(ft4int==0 || ft4int==2 || ft4int==4 || ft4int==6) ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color : %2").arg(Radio::convert_dark("#96ffff",m_useDarkStyle),Radio::convert_dark("#1400b1",m_useDarkStyle)));
        else ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color: %2").arg(Radio::convert_dark("#ffff96",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
    }
	else if((m_mode.startsWith("JT") || m_mode=="T10") && second=="00") {
		if((minute.toInt())%2==0) {
			ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color : %2").arg(Radio::convert_dark("#96ffff",m_useDarkStyle),Radio::convert_dark("#1400b1",m_useDarkStyle)));
        } else {
			ui->labUTC->setStyleSheet(QString("font-size: 18pt;background: %1;color: %2").arg(Radio::convert_dark("#ffff96",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
        }
    }
  }
}

void MainWindow::setAutoSeqButtonStyle(bool checked) {
  if(checked) {
    if(m_singleshot) { ui->AutoSeqButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-radius: 5px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#0000ff",m_useDarkStyle),Radio::convert_dark("#00ff00",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle))); }
    else { ui->AutoSeqButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-radius: 5px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#00ff00",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle))); }
  } else {
    if(m_singleshot) {
      if(m_mode.startsWith("FT")) { ui->AutoSeqButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#0000ff",m_useDarkStyle),Radio::convert_dark("#ffbbbb",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
      else { ui->AutoSeqButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#0000ff",m_useDarkStyle),Radio::convert_dark("#e0e0e0",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }}
    else { 
      if(m_mode.startsWith("FT")) { ui->AutoSeqButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#ffbbbb",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
	  else { ui->AutoSeqButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#e0e0e0",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
    }
  }
}

// set text on TX even/odd minute button
void MainWindow::setMinButton()
{
  if(!m_mode.startsWith ("WSPR")) {
	if(m_txFirst) {
	  if(m_mode.startsWith("FT")) {
		if(m_mode=="FT8") ui->TxMinuteButton->setText("TX 00/30");
		else if(m_mode=="FT2") ui->TxMinuteButton->setText("TX 0.0");   // CE3TSK: FT2's periods start every 3.75 s
		else ui->TxMinuteButton->setText("TX 00");
      }
      else ui->TxMinuteButton->setText(tr("TX Even"));
      ui->TxMinuteButton->setStyleSheet(QString("QPushButton {background: %1}").arg(Radio::convert_dark("#96ffff",m_useDarkStyle)));
    } else {
	  if(m_mode.startsWith("FT")) {
		if(m_mode=="FT8") ui->TxMinuteButton->setText("TX 15/45");
        else if(m_mode=="FT2") ui->TxMinuteButton->setText("TX 3.75");   // CE3TSK
        else ui->TxMinuteButton->setText("TX 7.5");
      } 
      else ui->TxMinuteButton->setText(tr("TX Odd"));
      ui->TxMinuteButton->setStyleSheet(QString("QPushButton {background: %1}").arg(Radio::convert_dark("#ffff96",m_useDarkStyle)));
    }
  } 
  else {
	ui->TxMinuteButton->setText("N/A");
	ui->TxMinuteButton->setStyleSheet(QString("QPushButton {background: %1}").arg(Radio::convert_dark("#bebebe",m_useDarkStyle)));
  }
}

void MainWindow::autoStopTx(QString reason)
{
//prevent AF RX frequency jumps since QSO is finished and prevent unexpected Halt Tx in autologging mode
//  if(m_config.clear_DX () || m_config.autolog()) clearDX ();
  if(m_enableTx || m_transmitting || m_btxok || g_iptt==1) haltTx(reason);
//prevent any possible sequence breaking with this callsign at the next QSO attempt
  if(m_skipTx1 && reason.endsWith ("counter triggered ") && (m_ntx==2 || m_QSOProgress==REPORT)) m_qsoHistory.remove(m_hisCall);
//prevent AF RX frequency jumps since QSO is finished and prevent unexpected Halt Tx in autologging mode
  if((m_config.clear_DX () || m_config.autolog()) && !m_hisCall.isEmpty() && !m_houndMode) clearDX (" cleared from autoStopTx()");
}

void MainWindow::writeHaltTxEvent(QString reason)
{
  bool haltTrans=false;
  if(!m_transmitting && g_iptt==1) haltTrans=true;
  if(m_config.write_decoded_debug()) {
    QFile f {m_dataDir.absoluteFilePath (m_jtdxtime->currentDateTimeUtc2().toString("yyyyMM_")+"ALL.TXT")};
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
       QTextStream out(&f);
       if(m_transmitting) {
          out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")"
              << "  Halt Tx triggered at TX: " << reason << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6)
              << " MHz  " << m_modeTx
              << ":  " << m_currentMessage <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

       } else {
          if(!haltTrans) {
             out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")"
                 << "  Halt Tx triggered at RX: " << reason <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

          } else {			  
             out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")"
                 << "  Halt Tx triggered at transition from RX to TX: " << reason <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

          }
       }
       f.close();
    } else {
       JTDXMessageBox::warning_message (this, "", tr ("File Open Error")
                                    , tr ("Cannot open \"%1\" for append: %2")
                                    .arg (f.fileName ()).arg (f.errorString ()));
    }
  }
}

//-------------------------------------------------------------- dataSink()
void MainWindow::dataSink(qint64 frames)
{
  static float s[NSMAX];
  static int ihsym=0;
  static int ihsymlast=52;
  static int nlostaudio=0;
  static int trmin;
  static int npts8;
  static float px=0.0;
  static float df3;
  static QDateTime last {m_jtdxtime->currentDateTimeUtc2().addSecs(-300)};
  static bool lastdelayed {false};

  if(m_diskData) dec_data.params.ndiskdat=1; else dec_data.params.ndiskdat=0;

// Get power, spectrum, and ihsym
  trmin=m_TRperiod/60.0;
  int k (frames);
  decode_range (m_mode.startsWith ("FT"), m_decodeBandwidth, m_wideGraph->nStartFreq (), m_wideGraph->Fmax (),   // CE3TSK
               dec_data.params.nfa, dec_data.params.nfb);
  int nsps=m_nsps;
  symspec_(&dec_data,&k,&trmin,&nsps,&px,s,&df3,&ihsym,&npts8);
  if(m_mode=="WSPR-2") wspr_downsample_(dec_data.d2,&k);
  if(ihsym <=0) return;
//  printf("%s(%0.1f) dataSink %s %d %d\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),m_jtdxtime->GetOffset(),last.toString("hh:mm:ss.zzz").toStdString().c_str(),ihsym,k);
  QString t;
  t = QString::asprintf(" Rx noise: %5.1f ",px);
  ui->signal_meter_widget->setValue(px); // Update thermometer
  if(m_monitoring || m_diskData) {
    m_wideGraph->dataSink2(s,df3,ihsym,m_diskData);
  }
  setStopHSym();
  if(ihsym==3*m_hsymStop/4) m_dialFreqRxWSPR=m_freqNominal;
  int nhsymEStopFT8 = m_hsymStop;
  if(m_diskData || m_mode.startsWith("WSPR")) m_delay=0;

// let Win users to decode some intervals if some frames were dropped in system audio
//#if defined(Q_OS_WIN)
  if(!m_diskData && m_mode=="FT8") {
    if(ihsym==1) nlostaudio=52-ihsymlast;
    ihsymlast=ihsym;
    if(nlostaudio > 0) {
//printf("%s lost audio blocks %d \n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss").toStdString().c_str(),nlostaudio);
      quint64 timedelta = m_jtdxtime->currentMSecsSinceEpoch2() - m_mslastMon;
      if(timedelta > 14990) {
        if(nlostaudio < 3) setBandLabelColour ("#ffff00");
        else setBandLabelColour ("#ff8000");
        ui->label_6->setText(tr("lost audio ")+QString::number(nlostaudio));
        if(m_config.write_decoded_debug()) writeToALLTXT("Lost audio blocks: " + QString::number(nlostaudio));
      }
      else setBandLabelColour ("#fdedc5");
      nlostaudio=0; m_lostaudio=true;
    }
    if(!m_diskData && m_mode=="FT8" && ihsym>45 && ihsym<nhsymEStopFT8 && m_delay==0) {
      QDateTime now1 {m_jtdxtime->currentDateTimeUtc2 ()};
      quint64 n1=now1.toMSecsSinceEpoch()%15000;
      if(n1>14735) {
        if(m_swl) ihsym=51; 
        else if(m_FT8EarlyStart) ihsym=48;
        else ihsym=49;
      } 
    }
  }
//#endif

  int ihsymdelay=0;
  if(m_delay > 0) ihsymdelay=delay_blocks(m_delay,m_nsps)+ihsym;   // CE3TSK: modetiming.h
//cycling approximately once per 269..301 milliseconds
  if((m_mode=="FT8" && m_delay==0 && ihsym == nhsymEStopFT8)
     || (m_mode=="FT8" && m_delay > 0 && ihsymdelay >= nhsymEStopFT8)
     || (m_mode=="FT4" && m_delay==0 && ihsym == m_hsymStop)
     || (m_mode=="FT4" && m_delay > 0 && ihsymdelay >= m_hsymStop)
     || (m_mode=="FT2" && m_delay==0 && ihsym == m_hsymStop)          // CE3TSK
     || (m_mode=="FT2" && m_delay > 0 && ihsymdelay >= m_hsymStop)
     || ((m_mode.startsWith("JT") || m_mode=="T10") && m_delay==0 && ihsym == m_hsymStop)
     || ((m_mode.startsWith("JT") || m_mode=="T10") && m_delay > 0 && ihsymdelay >= m_hsymStop)
     || (m_mode.startsWith("WSPR") && ihsym == m_hsymStop)) {
    QDateTime now = m_jtdxtime->currentDateTimeUtc2 ();
//prevent dupe decoding
    if(lastdelayed && !m_modeChanged) {
      if(m_mode=="FT8" && last.secsTo(now)<12) { lastdelayed=false; return; }
      else if(m_mode=="FT4" && last.secsTo(now)<6) { lastdelayed=false; return; }
      /* CE3TSK: FT2's guard is half a period, 1.9 s, and it has to be counted in milliseconds -
         secsTo () truncates, so a 3.75 s period cannot be guarded with whole seconds. */
      else if(m_mode=="FT2" && last.msecsTo(now)<1900) { lastdelayed=false; return; }
      else if(!m_mode.startsWith("FT") && !m_mode.startsWith("WSPR") && last.secsTo(now)<46) { lastdelayed=false; return; }
      lastdelayed=false;
    }
    if(m_delay>0) lastdelayed=true;

    if( m_dialFreqRxWSPR==0) m_dialFreqRxWSPR=m_freqNominal;
    m_dataAvailable=true;
    dec_data.params.npts8=(ihsym*m_nsps)/16;
    dec_data.params.newdat=1;
    dec_data.params.nagain=0;
    dec_data.params.nagainfil=0;	
    dec_data.params.nzhsym=m_hsymStop;
//    m_dateTime = now.toString ("yyyy-MMM-dd hh:mm");

    if(!m_decoderBusy && m_mode.left(4)!="WSPR") { //Start decoder
      // CE3TSK P7: the decode of a period in which we transmitted is skipped while the TX
      // background is enabled, so the background runs on through our TX period (decodebudget.h)
      int const thisPeriod = period_index_of_trigger (0.001 * (m_jtdxtime->currentMSecsSinceEpoch2 () % 86400000), m_TRperiod);
      if(skip_own_tx_decode (m_diskData, (m_mode=="FT8" && m_bgEnabled) || (m_mode=="FT4" && m_ft4BgEnabled)
                             || (m_mode=="FT2" && m_ft2Recipe.bgon), m_txPeriod, thisPeriod)) {   // item 80: FT4 too; step 5: FT2 too
        last=now;
        if(m_config.write_decoded_debug()) writeToALLTXT("Decode skipped: own TX period, TX background running");
        // what the empty decode's <DecodeFinished> used to do for this period: the sequencer's
        // pass (it runs after every period, transmitted or not) and the wav-kill timer (in
        // "save decoded" mode the period's file goes, nothing was decoded)
        m_bDecoded=false; killFileTimer.start (750.0*m_TRperiod);
        if (m_autoseq && !m_manualDecode) process_Auto();
      } else {
      last=now; decode(); 
      if(!m_lostaudio) { setBandLabelColour ("#fdedc5"); ui->label_6->setText(tr("Band")); }
      }
    }
    m_delay=0;

    if(!m_diskData && (m_saveWav!=0 || m_mode.mid (0,4) == "WSPR")) { //Save unless "Save None"
      if(m_mode.startsWith("FT")) {
        int n=fmod(double(now.time().second()),m_TRperiod);
        if(n<(m_TRperiod/2)) n=n+m_TRperiod;
        auto const& period_start=now.addSecs(-n);
        m_fnameWE=m_config.save_directory().absoluteFilePath (period_start.toString("yyMMdd_hhmmss"));
      } else {
        auto const& period_start = now.addSecs (-(now.time ().minute () % (int(m_TRperiod) / 60)) * 60);
        m_fnameWE=m_config.save_directory ().absoluteFilePath (period_start.toString ("yyMMdd_hhmm"));
      }
      m_fileToSave.clear ();
      int samples=m_TRperiod*12000;
      if(m_mode=="FT4") samples=21*3456;
      if(m_mode=="FT2") samples=22*1728;   // CE3TSK: what the decoder is given, as FT4's line does
      // the following is potential a threading hazard - not a good
      // idea to pass pointer to be processed in another thread
      if ((m_saveWav==2 || m_saveWav==1 || m_mode.mid (0,4) == "WSPR") && !m_fnameWE.isEmpty ())
         m_saveWAVWatcher.setFuture (QtConcurrent::run (std::bind (&MainWindow::save_wave_file,
             this, m_fnameWE, &dec_data.d2[0], samples, m_config.my_callsign(),
             m_config.my_grid(), m_mode, m_freqNominal, m_hisCall, m_hisGrid,m_jtdxtime)));

      if (m_mode.mid (0,4) == "WSPR") {
        QString c2name_string {m_fnameWE + ".c2"};
        int len1=c2name_string.length();
        char c2name[80];
        strcpy(c2name,c2name_string.toLatin1 ().constData ());
        int nsec=120;
        int nbfo=1500;
        double f0m1500=m_freqNominal/1000000.0 + nbfo - 1500;
        int err = savec2_(c2name,&nsec,&f0m1500,len1);
        if (err!=0) JTDXMessageBox::warning_message (this, "", tr ("Error saving c2 file"), c2name);
      }
    }

    if(m_mode.left(4)=="WSPR" && !m_decoderBusy) {
      m_decoderBusy = true;
      QString t2,cmnd,depth_string;
      double f0m1500=m_dialFreqRxWSPR/1000000.0;   // + 0.000001*(m_BFO - 1500);
      t2 = QString::asprintf(" -f %.6f ",f0m1500);
      if(m_ndepth==1) depth_string=" -qB "; //2 pass w subtract, no Block detection, no shift jittering
      if(m_ndepth==2) depth_string=" -B ";  //2 pass w subtract, no Block detection
      if(m_ndepth==3) depth_string=" -C 5000 -o 4";   //2 pass w subtract, Block detection and OSD.

      if(m_diskData) {
        cmnd='"' + m_appDir + '"' + "/wsprd_jtdx " + depth_string + " -a \"" +
            QDir::toNativeSeparators(m_dataDir.absolutePath()) + "\" \"" + m_path + "\"";
      } else {
        cmnd='"' + m_appDir + '"' + "/wsprd_jtdx " + depth_string + " -a \"" +
            QDir::toNativeSeparators(m_dataDir.absolutePath()) + "\" " +
            t2 + '"' + m_fnameWE + ".wav\"";
      }
      QString t3=cmnd;
      int i1=cmnd.indexOf("/wsprd_jtdx ");
//      cmnd=t3.left(i1+7) + t3.mid(i1+7);
      cmnd=t3.mid(0,i1+7) + t3.mid(i1+7);
      if(ui) ui->DecodeButton->setChecked (true);
      p1.start(QDir::toNativeSeparators(cmnd));
      statusUpdate ();
    }
    m_rxDone=true;
  }
}

QString MainWindow::save_wave_file (QString const& name, short const * data, int samples,
        QString const& my_callsign, QString const& my_grid, QString const& mode,
        Frequency frequency, QString const& his_call, QString const& his_grid,JTDXDateTime * jtdxtime) const
{
  //
  // This member function runs in a thread and should not access
  // members that may be changed in the GUI thread or any other thread
  // without suitable synchronization.
  //
  QAudioFormat format;
  format.setCodec ("audio/pcm");
  format.setSampleRate (12000);
  format.setChannelCount (1);
  format.setSampleSize (16);   /* the native depth again since 2026-09-03, AUDIO_DEPTH_PLAN.md */
  format.setSampleType (QAudioFormat::SignedInt);
  auto source = QString {"%1, %2"}.arg (my_callsign).arg (my_grid);
  auto comment = QString {"Mode=%1%2, Freq=%3%4"}
     .arg (mode)
     .arg (QString {mode.contains ('J') && !mode.contains ('+')
           ? QString {", Sub Mode="} + QChar {'A' + 0}
         : QString {}})
        .arg (Radio::frequency_MHz_string (frequency))
     .arg (QString {!mode.startsWith ("WSPR") ? QString {", DXCall=%1, DXGrid=%2"}
         .arg (his_call)
         .arg (his_grid).toLocal8Bit () : ""});

  BWFFile::InfoDictionary list_info {
      {{{'I','S','R','C'}}, source.toLocal8Bit ()},
      {{{'I','S','F','T'}}, program_title (revision ()).simplified ().toLocal8Bit ()},
      {{{'I','C','R','D'}}, jtdxtime->currentDateTime2 ()
                          .toString ("yyyy-MM-ddTHH:mm:ss.zzzZ").toLocal8Bit ()},
      {{{'I','C','M','T'}}, comment.toLocal8Bit ()},
  };
  BWFFile wav {format, name + ".wav", list_info};
  if (!wav.open (BWFFile::WriteOnly)
      || 0 > wav.write (reinterpret_cast<char const *> (data)
                        , sizeof (short) * samples))
    {
      return wav.errorString ();
    }
  return QString {};
}

void MainWindow::showSoundInError(const QString& errorMsg) { JTDXMessageBox::critical_message(this, "", tr("Error in SoundInput"), errorMsg); }
void MainWindow::showSoundOutError(const QString& errorMsg) { JTDXMessageBox::critical_message(this, "", tr("Error in SoundOutput"), errorMsg); }
void MainWindow::showStatusMessage(const QString& statusMsg) { statusBar()->showMessage(statusMsg); }


/* CE3TSK: asked once, on the first run of JTDX_contest with a profile whose notification
   colours are not the recommended ones - i.e. a user arriving from stock JTDX with colours
   that were never checked for contrast. Fired from the constructor through a zero timer so
   the main window is up and the dialog has a parent to centre on. */
void MainWindow::offerRecommendedColors ()
{
  if (!m_config.recommended_colors_offer_pending ()) return;

  QMessageBox mb {this};
  mb.setIcon (QMessageBox::Question);
  mb.setWindowTitle (QApplication::applicationName () + " - " + tr ("Recommended colours"));
  mb.setTextFormat (Qt::PlainText);
  mb.setText (tr ("Use the recommended notification colours?"));
  mb.setInformativeText (
      tr ("This profile carries notification colours from an earlier setup. JTDX_contest ships a "
          "set that was checked against every background each colour can appear on, and meets the "
          "AA contrast level of the WCAG accessibility guidelines, so the decodes stay legible.\n\n"
          "Choosing Yes also switches on the new dark style, which those colours are made for.\n\n"
          "You can go back to the light style at any time in Settings, General; and the recommended "
          "colours can be set again later in Settings, Notifications.\n\n"
          "This is asked only once."));
  mb.setStandardButtons (QMessageBox::Yes | QMessageBox::No);
  mb.setDefaultButton (QMessageBox::Yes);
  mb.button (QMessageBox::Yes)->setText (tr ("&Yes, use them"));
  mb.button (QMessageBox::No)->setText (tr ("&No, keep mine"));

  if (QMessageBox::Yes == mb.exec ())
    {
      m_config.accept_recommended_colors ();
      darkStyleChanged ();   // the same follow-up as a style change from Settings
      if (m_config.write_decoded_debug ()) writeToALLTXT ("Recommended colours and dark style applied on first run");
    }
  else
    {
      m_config.decline_recommended_colors ();
    }
}

void MainWindow::on_actionSettings_triggered()               //Setup Dialog
{
  // things that might change that we need know about
  m_strictdirCQ = m_config.strictdirCQ ();
  m_callsign = m_config.my_callsign ();
  m_useDarkStyle = m_config.useDarkStyle(); setDecodeMenuColours();
  m_grid = m_config.my_grid();
  m_callNotif = m_config.callNotif();
  m_gridNotif = m_config.gridNotif();
  m_countryNameTranslated = m_config.countryNameTranslated();
  m_timeFrom = m_config.timeFrom();
  bool spot_to_dxsummit = m_config.spot_to_dxsummit();

  if (QDialog::Accepted == m_config.exec ()) {
      if(m_config.write_decoded_debug()) writeToALLTXT("Configuration settings change accepted");
      ui->decodedTextBrowser->setConfiguration (&m_config);
      ui->decodedTextBrowser2->setConfiguration (&m_config);
      refreshSpecialOp (); /* CE3TSK: before anything below reads m_wwDigi */
      if (m_config.useDarkStyle() != m_useDarkStyle) {
        darkStyleChanged ();
        if(m_config.write_decoded_debug()) writeToALLTXT("Both windows cleared, triggered by dark style change");
      }
      if(m_config.my_callsign () != m_callsign) {
        m_bMyCallStd=stdCall(m_config.my_callsign ());
        if(!m_config.my_callsign().isEmpty()) toggle_skipTx1();
        m_myCallCompound=(!m_config.my_callsign().isEmpty() && m_config.my_callsign().contains("/")); // && !m_config.my_callsign().endsWith("/P") && !m_config.my_callsign().endsWith("/R"));
        if(!m_config.my_callsign().isEmpty()) {
          if(m_bMyCallStd) {
            if(!ui->skipTx1->isEnabled() && !m_wwDigi) { ui->skipTx1->setEnabled(true); ui->skipGrid->setEnabled(true); }   /* CE3TSK */
          } else {
            if(m_skipTx1) { m_skipTx1=false; ui->skipTx1->setChecked(false); ui->skipGrid->setChecked(false); }
            ui->skipTx1->setEnabled(false); ui->skipGrid->setEnabled(false);
          }
        }
        m_baseCall = Radio::base_callsign (m_config.my_callsign ());
        ui->genStdMsgsPushButton->click ();
        m_lastloggedtime=m_jtdxtime->currentDateTimeUtc2().addSecs(-7*int(m_TRperiod));
        m_lastloggedcall.clear(); setLastLogdLabel();
        morse_(const_cast<char *> (m_config.my_callsign ().toLatin1().constData())
               , const_cast<int *> (icw)
               , &m_ncw
               , m_config.my_callsign ().length());
      }

      /* CE3TSK: WW Digi contest - the station grid is the exchange, so a grid change must
         rebuild the messages. The callsign branch above is the only other place that does
         it, so without this, blanking the grid would leave valid looking messages in place
         until something else happened to regenerate them. */
      if(m_wwDigi && m_config.my_grid () != m_grid) ui->genStdMsgsPushButton->click ();

      on_dxGridEntry_textChanged (m_hisGrid); // recalculate distances in case of units change
      enable_DXCC_entity ();  // sets text window proportions and (re)inits the logbook

      if(m_config.spot_to_psk_reporter ()) {
        pskSetLocal ();
      }
      bool was_monitoring = m_monitoring;
      if (m_monitoring && (m_config.restart_tci () || m_tci != (m_config.tci_audio() && m_config.is_tci()))) on_monitorButton_clicked (false);
      if (!(m_config.tci_audio() && m_config.is_tci())) {
        //here computer soundcard
        if((m_config.restart_audio_input () || m_tci) && !m_config.audio_input_device ().isNull ()) {
          Q_EMIT startAudioInputStream (m_config.audio_input_device (), m_rx_audio_buffer_frames, m_detector,
                                        m_downSampleFactor, m_config.audio_input_channel ());
        }

        if((m_config.restart_audio_output () || m_tci) && !m_config.audio_output_device ().isNull ()) {
          Q_EMIT initializeAudioOutputStream (m_config.audio_output_device (),
             AudioDevice::Mono == m_config.audio_output_channel () ? 1 : 2, m_tx_audio_buffer_frames);
          if(m_transmitting || g_iptt==1) {
  //           ui->stopTxButton->click (); // halt any transmission
             haltTx("settings change is accepted ");
             enableTx_mode (false);       // switch off EnableTx button
             ui->enableTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-radius: 5px;border-color: %3;min-width: 63px;padding: 0px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#ffff76",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));
             TxAgainTimer.start(2000);
           }
        }
        if (m_tci) {
          Q_EMIT transmitFrequency (ui->TxFreqSpinBox->value () - m_XIT);
          m_modulator->setPeriod(m_TRperiod);
          m_detector->setPeriod(m_TRperiod);
        }
      } else {
        if(m_transmitting || g_iptt==1) {
//           ui->stopTxButton->click (); // halt any transmission
           haltTx("settings change is accepted ");
           enableTx_mode (false);       // switch off EnableTx button
           ui->enableTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-radius: 5px;border-color: %3;min-width: 63px;padding: 0px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#ffff76",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));
           TxAgainTimer.start(2000);
         }
        
        Q_EMIT m_config.transceiver_period(double(NTMAX));
        Q_EMIT m_config.transceiver_trfrequency(ui->TxFreqSpinBox->value () - m_XIT);
        Q_EMIT m_config.transceiver_period(m_TRperiod);
      }

      if (was_monitoring && (m_config.restart_tci () || m_tci != (m_config.tci_audio() && m_config.is_tci())) && !m_monitoring && !m_transmitting && g_iptt!=1 ) {
        m_tci = m_config.tci_audio() && m_config.is_tci();
        on_monitorButton_clicked (true);
      } else m_tci = m_config.tci_audio() && m_config.is_tci();
 
      displayDialFrequency ();

      if(m_mode=="FT8") on_actionFT8_triggered();
      else if(m_mode=="FT4") on_actionFT4_triggered();
      else if(m_mode=="FT2") on_actionFT2_triggered();   // CE3TSK
      else if(m_mode=="JT9+JT65") on_actionJT9_JT65_triggered();
      else if(m_mode=="JT9") on_actionJT9_triggered();
      else if(m_mode=="JT65") on_actionJT65_triggered();
      else if(m_mode=="T10") on_actionT10_triggered();
      else if(m_mode=="WSPR-2") on_actionWSPR_2_triggered();

	  m_config.transceiver_online ();
	  m_wideGraph->setTopJT65(m_config.ntopfreq65());
	  setXIT (ui->TxFreqSpinBox->value ());
      update_watchdog_label ();
      if(m_mode != "WSPR-2" && spot_to_dxsummit != m_config.spot_to_dxsummit()) {
         m_dxCallHidden=false;   // CE3TSK
         if(m_config.spot_to_dxsummit() ) { ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#c4c4ff",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
         else { ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#aabec8",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
      }
      if(!m_config.do_snr()) ui->S_meter_button->setText(tr("S meter"));
      ui->S_meter_button->setEnabled(m_config.do_snr());
      if(!m_config.do_pwr()) {ui->PWRlabel->setText(tr("Pwr")); ui->SWRlabel->setText("");}
      on_spotLineEdit_textChanged(ui->spotLineEdit->text());
      ui->bandComboBox->setCurrentText (m_config.bands ()->find (m_freqNominal));
  }
}

void MainWindow::on_filterButton_clicked (bool checked)
{
  if(checked) { m_filter=true; m_wideGraph->setFilter(m_filter); }
  else { dec_data.params.nagainfil=0; m_filter=false; m_wideGraph->setFilter(m_filter); }
  if(m_config.write_decoded_debug()) {
    QString filter = m_filter ? "  Filter button is switched on" : "  Filter button is switched off";
    writeToALLTXT(filter);
  }
}

void MainWindow::toggle_filter() { ui->filterButton->click(); }
void MainWindow::escapeHalt() { haltTx("TX halted via Escape button from widegraph "); }
void MainWindow::filter_on() { if(!m_filter) ui->filterButton->click(); }
void MainWindow::on_swlButton_clicked (bool checked) { if(checked) m_swl=true; else m_swl=false; }
void MainWindow::on_AGCcButton_clicked(bool checked) { if(checked) m_agcc=true; else m_agcc=false; }
void MainWindow::on_hintButton_clicked (bool checked) { m_hint=checked; }
void MainWindow::on_HoundButton_clicked (bool checked) { ui->actionEnable_hound_mode->setChecked(checked);}

void MainWindow::on_AutoTxButton_clicked (bool checked)
{
  m_autoTx = checked;
  if(checked) ui->AutoTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-radius: 5px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#00ff00",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));
  else if(m_autoseq) ui->AutoTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#ffbbbb",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle)));
  else ui->AutoTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#e0e0e0",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle)));
}

void MainWindow::on_AutoSeqButton_clicked (bool checked)
{
  m_autoseq = checked;
  if (checked) {
    m_wasAutoSeq=false; //in case of toggling AutoSeq button by user
//txrb button selection by user can brake AutoSeq, disable all txrb buttons
    enableTab1TXRB(false);
    if(ui->tabWidget->currentIndex()==1) {
      on_rbGenMsg_clicked(true);
      ui->rbGenMsg->setChecked(true);
    }
    if(!m_autoTx) ui->AutoTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#ffbbbb",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle)));
  } else {
    enableTab1TXRB(true);
	if(!m_autoTx) ui->AutoTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#e0e0e0",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle)));
  }
  setAutoSeqButtonStyle(m_autoseq);
}

void MainWindow::on_monitorButton_clicked (bool checked)
{
  if (!m_transmitting)
    {
      auto prior = m_monitoring;
      m_monitoroff = !checked;
      monitor (checked);
      if (!checked) initLogIfNeeded ();   // CE3TSK

      if (checked && !prior)
        {
          if (m_config.monitor_last_used ())
            {
              // put rig back where it was when last in control
              m_freqNominal = m_lastMonitoredFrequency;
              m_freqTxNominal = m_freqNominal;
              highlightBandButton ();   // CE3TSK
              setRig ();
              setXIT (ui->TxFreqSpinBox->value ());
            }
        }

      //Get Configuration in/out of strict split and mode checking
      Q_EMIT m_config.sync_transceiver (true, checked);
    }
  else
    {
      ui->monitorButton->setChecked (false); // disallow
    }
}

void MainWindow::monitor (bool state)
{
  ui->monitorButton->setChecked (state);
  if (state) {
    m_diskData = false;	// no longer reading WAV files
    if (!m_monitoring) {
      m_mslastMon=m_jtdxtime->currentMSecsSinceEpoch2();
      if (m_tci) Q_EMIT m_config.transceiver_audio(true);
      else Q_EMIT resumeAudioInputStream ();
      QDateTime  currentTime = m_jtdxtime->currentDateTimeUtc2 (); // decode part of interval
      QString curtime=currentTime.toString("ss.zzz");
      curtime.remove(2,1); curtime.remove(3,2);
      int curdsec = curtime.toInt();
      if(m_addtx==-1) m_addtx=2; else if(m_addtx==-2) m_addtx=4; else m_addtx=0; // no delay for manual triggering Monitor button
      /* CE3TSK: one implementation for every mode (modetiming.h). The remainder has to be taken
         in milliseconds: FT2's period is 37.5 tenths, so tenths cannot express it. */
      qint64 const msmin=currentTime.time().second()*1000 + currentTime.time().msec();
      if(m_mode == "FT8") {
         curdsec=period_position_tenths(msmin,tr_period_of(m_mode)); if(curdsec > 0 && curdsec < 90) m_delay=curdsec+m_addtx; else m_delay = 0;
      }
      else if(m_mode == "FT4") {
         curdsec=period_position_tenths(msmin,tr_period_of(m_mode)); if(curdsec > 0 && curdsec < 40) m_delay=curdsec+m_addtx; else m_delay = 0;
      }
      else if(m_mode == "FT2") {
         curdsec=period_position_tenths(msmin,tr_period_of(m_mode)); if(curdsec > 0 && curdsec < 20) m_delay=curdsec+m_addtx; else m_delay = 0;
      }
	  else if(!m_mode.startsWith("WSPR")) {
         if(curdsec > 0 && curdsec < 350) m_delay=curdsec+m_addtx; else m_delay = 0; // 2 second processing delay
      }
      m_addtx=0;
    }
  } else {
    if (m_tci) Q_EMIT m_config.transceiver_audio(false);
    else Q_EMIT suspendAudioInputStream ();
  }
  m_monitoring = state;
  if(m_monitoring && m_txbColorSet) { resetTxMsgBtnColor(); m_txbColorSet=false; }
}

void MainWindow::on_actionAbout_triggered() { CAboutDlg {this, m_useDarkStyle}.exec (); } //Display "About"

void MainWindow::on_enableTxButton_clicked (bool checked)
{
  if(m_enableTx && !checked && m_curMsgTx.startsWith(m_hisCall+" ")) m_lasthint=true;
  if(checked && m_lasthint) m_lasthint=false;
  m_enableTx = checked;
  statusUpdate ();
  if(m_mode.left(4)=="WSPR")  {
    QPalette palette {ui->sbTxPercent->palette ()};
    if(m_enableTx or m_pctx==0) { palette.setColor(QPalette::Base,Qt::white); }
    else { palette.setColor(QPalette::Base,Qt::yellow); }
    ui->sbTxPercent->setPalette(palette);
  }
  if(m_enableTx) {
	 setEnableTxButtonStyle ();
  } else {
// sync TX variables 
     if(!m_transmitting) { m_bTxTime=false; m_tx_when_ready=false; m_restart=false; m_txNext=false; }
	 setEnableTxButtonStyle ();
  }
}

void MainWindow::enableTx_mode (bool state) { ui->enableTxButton->setChecked (state); on_enableTxButton_clicked (state); }
void MainWindow::enableTxButton_off () { enableTx_mode (false); }

void MainWindow::keyPressEvent( QKeyEvent *e )                //keyPressEvent
{
  int n;
  switch(e->key())
    {
    case Qt::Key_D:
      if(!m_decoderBusy && m_mode != "WSPR-2") {
         if(e->modifiers() & Qt::ShiftModifier) {
            dec_data.params.newdat=0;
            dec_data.params.nagain=0;
            decode();
            return;
         }
         if(e->modifiers() & Qt::AltModifier) {
            dec_data.params.newdat=0;
            dec_data.params.nagainfil=1;
            decode();
            return;
         }
      }
      break;
    case Qt::Key_A:
      if((e->modifiers() & Qt::AltModifier) && (e->modifiers() & Qt::ControlModifier)) {
        ui->wantedCall->clear();
        return;
      }
      break;
    case Qt::Key_F2:	  
      if(!m_menus) QTimer::singleShot (0, this, SLOT (on_actionSettings_triggered ()));
      return;
    case Qt::Key_F4:
      clearDX (" F4 key pressed");
      ui->dxCallEntry->setFocus();
      return;
    case Qt::Key_F6:
      if(e->modifiers() & Qt::ShiftModifier) { on_actionDecode_remaining_files_in_directory_triggered(); return; }
      else { if(!m_menus) on_actionOpen_next_in_directory_triggered(); }
      return;
    case Qt::Key_F7:	  
      if(!m_menus) on_actionOpen_wsjtx_log_adi_triggered();
      return;
      break;
    case Qt::Key_F11:
      n=11;
      if(e->modifiers() & Qt::ControlModifier) n+=100;
      bumpFqso(n);
      return;
    case Qt::Key_F12:
      n=12;
      if(e->modifiers() & Qt::ControlModifier) n+=100;
      bumpFqso(n);
      return;
    case Qt::Key_E:
      if(e->modifiers() & Qt::ShiftModifier) {
        m_txFirst=false;
        ui->TxMinuteButton->setChecked(m_txFirst);
        setMinButton();
        return;
      }
      if(e->modifiers() & Qt::ControlModifier) {
        m_txFirst=true;
        ui->TxMinuteButton->setChecked(m_txFirst);
        setMinButton();
        return;
      }
      if(e->modifiers() & Qt::AltModifier && m_lang=="ru_RU") {
        ui->EraseButton->click();
        return;
      }
      break;
    case Qt::Key_F:
      if(e->modifiers() & Qt::ControlModifier) {
        if(ui->tabWidget->currentIndex()==0) { ui->tx5->clearEditText(); ui->tx5->setFocus(); }
        else { ui->freeTextMsg->clearEditText(); ui->freeTextMsg->setFocus(); }
        return;
      }
      if(e->modifiers() & Qt::AltModifier) {
        if(m_bypassAllFilters) { ui->actionBypass_all_text_filters->setChecked(false); m_bypassAllFilters=false; } 
        else { ui->actionBypass_all_text_filters->setChecked(true); m_bypassAllFilters=true; }
      }
      break;
    case Qt::Key_G:
      if(e->modifiers() & Qt::AltModifier) {
        genStdMsgs(m_rpt);
        return;
      }
      break;
    case Qt::Key_H:
      if(e->modifiers() & Qt::AltModifier) {
        on_stopTxButton_clicked();
        return;
      }
      break;
    case Qt::Key_L:
      if(e->modifiers() & Qt::ControlModifier) {
        lookup();
        genStdMsgs(m_rpt);
        return;
      }
      if(e->modifiers() & Qt::AltModifier && m_lang=="ru_RU") {
        lookup();
        return;
      }
      break;
    case Qt::Key_O:
      if(!m_menus && e->modifiers() & Qt::ControlModifier) {
        QTimer::singleShot (0, this, SLOT (on_actionOpen_triggered()));
        return;
      }
      if(e->modifiers() & Qt::AltModifier && m_lang=="ru_RU") {
        ui->DecodeButton->click();
        return;
      }
      break;
    case Qt::Key_V:
      if(e->modifiers() & Qt::AltModifier) {
        m_fileToSave=m_fnameWE;
        return;
      }
      break;
    case Qt::Key_Z:
      if(e->modifiers() & Qt::AltModifier) {
        ui->filterButton->click();
        return;
      }
      break;
    case Qt::Key_Escape:
      haltTx("TX halted via Escape button ");
      break;
    case Qt::Key_B:
      if(e->modifiers() & Qt::AltModifier) {
        on_actionFT8_triggered();
        return;
      }
      break;
    case Qt::Key_C:
      if(e->modifiers() & Qt::AltModifier) {
        on_actionFT4_triggered();
        return;
      }
      break;
    case Qt::Key_M:
      if(e->modifiers() & Qt::AltModifier && m_lang=="ru_RU") {
        ui->monitorButton->click();
        return;
      }
      break;
    case Qt::Key_N:
      if(e->modifiers() & Qt::AltModifier && m_lang=="ru_RU") {
        ui->enableTxButton->click();
        return;
      }
      break;
    case Qt::Key_S:
      if(e->modifiers() & Qt::AltModifier && m_lang=="ru_RU") {
        ui->stopButton->click();
        return;
      }
      break;
    }

  QMainWindow::keyPressEvent (e);
}

void MainWindow::bumpFqso(int n)                                 //bumpFqso()
{
  int i;
  bool ctrl = (n>=100);
  n=n%100;
  i=ui->RxFreqSpinBox->value ();
  if(n==11) i--;
  if(n==12) i++;
  if (ui->RxFreqSpinBox->isEnabled ())
    {
      ui->RxFreqSpinBox->setValue (i);
    }
  if(ctrl and m_mode.left(4)=="WSPR") {
    ui->WSPRfreqSpinBox->setValue(i);
  } else {
    if(ctrl && ui->TxFreqSpinBox->isEnabled ()) {
      ui->TxFreqSpinBox->setValue (i);
    }
  }
}

void MainWindow::displayDialFrequency ()
{
  static bool startup=true;
  Frequency dial_frequency {m_rigState.ptt () && m_rigState.split () ?
      m_rigState.tx_frequency () : m_rigState.frequency ()};
  if(m_monitoroff && m_config.rig_name()=="None") dial_frequency=m_freqNominal;
  if (dialWheelHolding ()) dial_frequency = m_dialWheelTarget;   // CE3TSK: the wheel's target, not the rig's older report
  // lookup band
  auto const& band_name = m_config.bands ()->find (dial_frequency);
//  printf("last band %s curband %s band %s freq %lld\n",m_lastBand.toStdString().c_str(),ui->bandComboBox->currentText().toStdString().c_str(),band_name.toStdString().c_str(),dial_frequency);
  if(m_lastBand != band_name && ui->bandComboBox->currentText() != band_name) {
    // only change this when necessary as we get called a lot and it
    // would trash any user input to the band combo box line edit
    if(!m_lastBand.isEmpty() && !band_name.isEmpty()) { // handle loss of the poll response from transceiver where dial_frequency==0
      if(m_autoEraseBC && !startup) { // option: erase both windows if band is changed
        ui->decodedTextBrowser->clear(); ui->decodedTextBrowser2->clear();
        clearDX (" cleared, triggered by erase both windows option upon band change from transceiver");
      }
      m_qsoHistory.init();
      if(m_config.write_decoded_debug()) {
        QString text = startup ? "program startup, m_lastBand: " : "QSO history initialized by band change from transceiver, m_lastBand: ";
        writeToALLTXT(text + m_lastBand + ", current band: " + band_name + ", dial_frequency: " + QString::number(dial_frequency) + ", TX VFO frequency: " + 
        QString::number(m_rigState.tx_frequency ()) + ", RX VFO frequency: " + QString::number(m_rigState.frequency ()));
      }
      // Set the attenuation value if options are checked
      QString curBand;
      if (m_config.pwrBandTxMemory() && !m_tune) {
          if (m_mode == "JT9+JT65" && m_modeTx == "JT65") { curBand = band_name+m_modeTx; }
          else { curBand = band_name+m_mode; }
          if (m_pwrBandTxMemory.contains(curBand)) { m_PwrBandSetOK = false; ui->outAttenuation->setValue(m_pwrBandTxMemory[curBand].toInt()); m_PwrBandSetOK = true;/* printf("set power from freq %s %s %d\n",m_lastBand.toStdString().c_str(),curBand.toStdString().c_str(),m_pwrBandTxMemory[curBand].toInt());*/}
          else if (m_outAttenuationRestored) { m_pwrBandTxMemory[curBand] = ui->outAttenuation->value(); }   /* CE3TSK: only record a band once the slider holds a real value, never the .ui default */
      }
      startup=false;
    }
    ui->bandComboBox->setCurrentText (band_name);
    m_wideGraph->setRxBand (band_name);
  }
  m_lastBand = band_name;
  // search working frequencies for one we are within 10kHz of (1 Mhz of on VHF and up)
  bool valid {false};
  quint64 min_offset {99999999};
  for (auto const& item : *m_config.frequencies ())
    {
      // we need to do specific checks for above and below here to
      // ensure that we can use unsigned Radio::Frequency since we
      // potentially use the full 64-bit unsigned range.
      auto const& working_frequency = item.frequency_;
      auto const& offset = dial_frequency > working_frequency ?
        dial_frequency - working_frequency :
        working_frequency - dial_frequency;
      if (offset < min_offset) {
        min_offset = offset;
      }
    }
  if (min_offset < 10000u) {
    valid = true;
  }
  if (valid) ui->labDialFreq->setStyleSheet(QString("QLabel {font-family: MS Shell Dlg 2;font-size: 18pt;background: %1;color: %2;}").arg(Radio::convert_dark("#e1e1e1",m_useDarkStyle),Radio::convert_dark("#0000ff",m_useDarkStyle)));
  else ui->labDialFreq->setStyleSheet(QString("QLabel {font-family: MS Shell Dlg 2;font-size: 18pt;background: %1;color: %2;}").arg(Radio::convert_dark("#ff0000",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));   // CE3TSK: blue on the red alarm ground measured 2.15:1, black is 5.25:1
  ui->labDialFreq->setText (Radio::pretty_frequency_MHz_string (dial_frequency));

  static bool first_freq {true};
  static Frequency first_value {145000000};
  if(((first_freq && dial_frequency!=0 && dial_frequency!=145000000) || (first_value != m_lastDialFreq)) && m_mode=="FT8") {
    first_value = m_lastDialFreq;
    if(first_freq) dial_frequency = m_freqNominal;
    bool commonFT8b=false;
    for(long unsigned int i=0; i < sizeof (m_ft8Freq) / sizeof (m_ft8Freq[0]); i++) {
      int kHzdiff=dial_frequency/1000 - m_ft8Freq[i];
      if(kHzdiff > -2 && kHzdiff < 3) { commonFT8b=true; break; }
    }
    m_commonFT8b=commonFT8b; first_freq=false;
    if (m_houndMode) {
    // Don't allow Hound frequency control in common FT8 bands if VFO Split mode is switched off
      QString message = "";
      if(!m_config.split_mode() && !m_commonFT8b && m_config.rig_name() != "None") {
        message =  tr ("Hound mode TX frequency control requires"
                                                            " *Split* rig control (either *Rig* or *Fake It* set"
                                                            " in the *Settings | Radio* tab.)");
        JTDXMessageBox::warning_message (this, "", tr ("Hound TX frequency control warning"), message);
        ui->actionEnable_hound_mode->setChecked(false);
      } else {
        m_houndTXfreqJumps=!m_commonFT8b && m_config.split_mode() && m_config.rig_name() != "None";
        ui->actionUse_TX_frequency_jumps->setChecked(m_houndTXfreqJumps);
        if(m_commonFT8b || m_config.rig_name() == "None") ui->actionUse_TX_frequency_jumps->setEnabled(false);
        else ui->actionUse_TX_frequency_jumps->setEnabled(true);
      }
    }
  }
}

/* CE3TSK: widgets coloured by what they show as well as by the style - the rig lamp by the last rig
   event, the "Band" label by lost audio or clock drift, the mode label by the mode, the DX call
   field after a logged QSO, the Enable Tx, Hound and DX Call buttons by their state. Their colour is
   set through these, which remember it, so styleChanged () can paint the same state again after a
   style switch; before, they kept the old style's colours until their state next changed. */
void MainWindow::setRigLamp (QString const& colour)
{
  m_rigLampColour = colour;
  ui->readFreq->setStyleSheet(ui->readFreq->styleSheet().left(230)+QString("background: %1;\n color: %2;\n}").arg(Radio::convert_dark(colour,m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));
}

void MainWindow::setBandLabelColour (QString const& colour)
{
  m_bandLabelColour = colour;
  ui->label_6->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark(colour,m_useDarkStyle)));
}

void MainWindow::setModeLabelStyle (QString const& mode)
{
  QString colour {"#6699ff"};   // FT8
  if (mode == "JT9") colour = "#ff99cc";
  else if (mode == "T10") colour = "#aaffff";
  else if (mode == "FT4") colour = "#a99ee2";
  else if (mode == "FT2") colour = "#ffacda";   // CE3TSK
  else if (mode == "JT65") colour = "#66ff66";
  else if (mode == "JT9+JT65") colour = "#ffff66";
  else if (mode == "WSPR-2") colour = "#ff66ff";
  mode_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark(colour,m_useDarkStyle)));
}

void MainWindow::setDxCallEntryColour (QString const& background)
{
  m_dxCallEntryColour = background;
  ui->dxCallEntry->setStyleSheet(QString("QLineEdit {color: %1; background: %2}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark(background,m_useDarkStyle)));
}

void MainWindow::setEnableTxButtonStyle ()
{
  if(m_enableTx) ui->enableTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-radius: 5px;border-color: %3;min-width: 63px;padding: 0px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#ff3c3c",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));
  else ui->enableTxButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 63px;padding: 0px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#dcdcdc",m_useDarkStyle),Radio::convert_dark("#adadad",m_useDarkStyle)));
}

void MainWindow::setHoundButtonStyle ()
{
  if(m_houndMode) ui->HoundButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-radius: 5px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#00ff00",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));
  else ui->HoundButton->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#e1e1e1",m_useDarkStyle),Radio::convert_dark("#adadad",m_useDarkStyle)));
}

void MainWindow::setSpotButtonStyle ()
{
  QString const background {m_dxCallHidden ? "#00ff00" : m_spotDXsummit ? "#c4ffc4" : (m_config.spot_to_dxsummit() ? "#c4c4ff" : "#aabec8")};
  ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark(background,m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle)));
}

void MainWindow::setTxStatusColour (QString const& colour)
{
  m_txStatusColour = colour;
  if (colour.isEmpty ()) tx_status_label->setStyleSheet ("");
  else tx_status_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark(colour,m_useDarkStyle)));
}

// the progress bar is restyled every second only while monitoring, transmitting or idle - not in file mode
void MainWindow::setProgressBarStyle ()
{
  QString cssSafe = QString("QProgressBar { border: 2px solid %1; border-radius: 5px; background: %2; text-align: center; } QProgressBar::chunk { background: %3; width: 1px; }").arg(Radio::convert_dark("#808080",m_useDarkStyle),Radio::convert_dark("#ffffff",m_useDarkStyle),Radio::convert_dark("#00ff00",m_useDarkStyle));
  QString cssTransmit = QString("QProgressBar { border: 2px solid %1; border-radius: 5px; background: %2; text-align: center; } QProgressBar::chunk { background: %3; width: 1px; }").arg(Radio::convert_dark("#808080",m_useDarkStyle),Radio::convert_dark("#ffffff",m_useDarkStyle),Radio::convert_dark("#ff0000",m_useDarkStyle));
  progressBar->setStyleSheet (m_transmitting ? cssTransmit : cssSafe);
}

/* CE3TSK: View > Band buttons - one button per frequency the band selector offers for the mode:
   the default working frequency of each band for the current mode and IARU region, or, while a
   contest is selected, every entry of that contest's own set. The list is re-filtered on each
   mode change and swapped on each contest change, so the buttons are rebuilt from its change
   signals, coalesced to one rebuild per pass of the event loop. */
void MainWindow::on_actionBand_buttons_toggled (bool checked)
{
  ui->bandButtonsWidget->setVisible (checked);
  rebuildBandButtons ();
}

void MainWindow::scheduleBandButtons ()
{
  auto * const frequencies = m_config.frequencies ();
  if (m_bandButtonsModel != frequencies)
    {
      for (auto const& connection : m_bandButtonsConnections) disconnect (connection);
      m_bandButtonsConnections.clear ();
      m_bandButtonsModel = frequencies;
      auto const later = [this] { m_bandButtonsTimer.start (); };
      m_bandButtonsConnections
        << connect (frequencies, &QAbstractItemModel::modelReset, this, later)
        << connect (frequencies, &QAbstractItemModel::layoutChanged, this, later)
        << connect (frequencies, &QAbstractItemModel::rowsInserted, this, later)
        << connect (frequencies, &QAbstractItemModel::rowsRemoved, this, later)
        << connect (frequencies, &QAbstractItemModel::dataChanged, this, later);
    }
  m_bandButtonsTimer.start ();
}

void MainWindow::rebuildBandButtons ()
{
  qDeleteAll (m_bandButtons);
  m_bandButtons.clear ();
  if (!ui->actionBand_buttons->isChecked ()) return;   // built when shown
  auto const * const frequencies = m_config.frequencies ();
  bool const contest {m_config.special_op_id () != Configuration::SpecialOperatingActivity::NONE};
  QList<Radio::Frequency> wanted;
  for (int row = 0; row < frequencies->rowCount (); ++row)
    {
      auto const source = frequencies->mapToSource (frequencies->index (row, FrequencyList_v2::frequency_column));
      if (!source.isValid ()) continue;
      auto const& item = frequencies->frequency_list ()[source.row ()];
      if ((contest || item.default_) && !wanted.contains (item.frequency_)) wanted << item.frequency_;
    }
  std::sort (wanted.begin (), wanted.end ());
  QHash<QString, int> per_band;
  for (auto const f : wanted) ++per_band[m_config.bands ()->find (f)];
  static QRegularExpression const metre_band {R"(^\d+m$)"};
  for (auto const f : wanted)
    {
      auto const band = m_config.bands ()->find (f);
      bool const by_frequency {band.isEmpty () || per_band.value (band) > 1};
      QString label {band};
      if (by_frequency)
        {
          // two buttons on one band: name them by frequency, 50.313 and 50.323
          label = QString::number (f / 1e6, 'f', 6);
          while (label.endsWith ('0')) label.chop (1);
          if (label.endsWith ('.')) label.chop (1);
        }
      else if (metre_band.match (band).hasMatch ()) label.chop (1);   // 20m -> 20; 70cm stays as it is
      auto * const button = new QPushButton {label, ui->bandButtonsWidget};
      // the tooltip says what the label does not: the frequency of a band button, the band of a
      // frequency button
      if (by_frequency && !band.isEmpty ()) button->setToolTip (band);
      else
        {
          QLocale const locale;   // the dial's decimal point, without its trailing zeros: 14,074 and 7,0475
          QString mhz {locale.toString (f / 1e6, 'f', 6)};
          while (mhz.endsWith (locale.zeroDigit ())) mhz.chop (1);
          if (mhz.endsWith (locale.decimalPoint ())) mhz.chop (1);
          button->setToolTip (mhz + " MHz");
        }
      button->setCheckable (true);
      button->setFocusPolicy (Qt::NoFocus);
      button->setProperty ("frequency", QVariant::fromValue<qulonglong> (f));
      connect (button, &QPushButton::clicked, this, [this, f] { selectBandButton (f); });
      ui->bandButtonsLayout->addWidget (button);
      m_bandButtons << button;
    }
  highlightBandButton ();
}

void MainWindow::selectBandButton (Radio::Frequency frequency)
{
  auto const * const frequencies = m_config.frequencies ();
  for (int row = 0; row < frequencies->rowCount (); ++row)
    {
      auto const source = frequencies->mapToSource (frequencies->index (row, FrequencyList_v2::frequency_column));
      if (source.isValid () && frequencies->frequency_list ()[source.row ()].frequency_ == frequency)
        {
          // exactly what picking the row in the band selector does, as switch_mode () does it
          ui->bandComboBox->setCurrentIndex (row);
          on_bandComboBox_activated (row);
          break;
        }
    }
  highlightBandButton ();
}

void MainWindow::highlightBandButton ()
{
  if (m_bandButtons.isEmpty ()) return;
  QPushButton * exact {nullptr};
  QPushButton * same_band {nullptr};
  auto const band = m_config.bands ()->find (m_freqNominal);
  for (auto * const button : m_bandButtons)
    {
      auto const f = button->property ("frequency").toULongLong ();
      if (f == m_freqNominal) exact = button;
      else if (!same_band && !band.isEmpty () && m_config.bands ()->find (f) == band) same_band = button;   // out of band matches nothing
    }
  auto * const lit = exact ? exact : same_band;
  for (auto * const button : m_bandButtons) button->setChecked (button == lit);
}

/* CE3TSK: tuning with the mouse wheel over the dial frequency. Only the three kHz digits after the
   decimal point respond - in "7.074 000" the 0, the 7 and the 4 - so a notch moves the dial by
   100, 10 or 1 kHz, carrying into the next digit the way a sum does (7.079 + 1 kHz = 7.080,
   7.000 - 1 kHz = 6.999). The MHz digits would change the band and the Hz digits are finer than
   any use, so both are left alone, and so is a step that would take the dial out of the band it
   is in. Notches in a burst show on the display at once and reach the rig as one QSY 200 ms after
   the last, through band_changed () like the band selector; until the rig reports the new
   frequency the display holds the target. Nothing happens while transmitting or tuning. */
bool MainWindow::dialFrequencyWheel (QWheelEvent * event)
{
  if (m_transmitting || m_tune) return false;
  auto const * const label = ui->labDialFreq;
  auto const text = label->text ();   // "7.074 000": the kHz digits are 7, 6 and 5 from the end in any locale
  if (text.size () < 8) return false;
  QFontMetrics const metrics {label->font ()};
  int const margin {label->margin ()};
  auto const area = label->contentsRect ().adjusted (margin, margin, -margin, -margin);
  int const left {area.left () + (area.width () - metrics.horizontalAdvance (text)) / 2};   // the .ui centres the text
  int const x {event->position ().toPoint ().x ()};
  int digit {-1};
  for (int i = 0; i < 3; ++i)
    {
      int const at {text.size () - 7 + i};
      int const from {left + metrics.horizontalAdvance (text.left (at))};
      if (text.at (at).isDigit () && x >= from && x < from + metrics.horizontalAdvance (text.at (at))) digit = i;
    }
  if (digit < 0) return false;
  m_dialWheelDelta += event->angleDelta ().y ();
  int const notches {m_dialWheelDelta / 120};
  if (!notches) return true;   // part of a notch, from a touchpad
  m_dialWheelDelta -= notches * 120;
  Frequency const from {dialWheelHolding () ? m_dialWheelTarget : m_freqNominal};
  qint64 const to {static_cast<qint64> (from) + notches * (digit == 0 ? 100000 : digit == 1 ? 10000 : 1000)};
  if (to <= 0 || m_config.bands ()->find (static_cast<Frequency> (to)) != m_config.bands ()->find (from)) return true;   // stays in its band
  m_dialWheelTarget = static_cast<Frequency> (to);
  m_dialWheelClock.start ();
  m_dialWheelTimer.start ();
  displayDialFrequency ();
  return true;
}

void MainWindow::applyDialWheel ()
{
  auto const target = m_dialWheelTarget;
  if (m_transmitting || m_tune || target == m_freqNominal) return;
  m_bandEdited = true;
  band_changed (target); if(m_config.write_decoded_debug()) writeToALLTXT("Band changed from dial wheel, frequency: " + QString::number(target));
  m_dialWheelClock.start ();   // hold the target on the display while the rig catches up
  displayDialFrequency ();
}

/* CE3TSK: a double click on the DX Call button, or on the callsign box beside it, opens the call's
   qrz.com page - https://www.qrz.com/db/CE3TSK. A compound call keeps its slash exactly as it is:
   qrz.com answers /db/VP2E/CE3TSK but returns 404 for the percent-encoded %2F, and the box's
   validator allows only letters, digits and '/', so nothing here needs encoding. An empty box does
   nothing. The button's other actions are untouched: a double click delivers a single clicked (),
   which spots to dxsummit as before when that is enabled, and m_spotDXsummit stops a second spot. */
void MainWindow::lookupDxCallOnQrz ()
{
  auto const call = ui->dxCallEntry->text ().trimmed ().toUpper ();
  if (call.isEmpty ()) return;
  QDesktopServices::openUrl (QUrl {"https://www.qrz.com/db/" + call});
}

bool MainWindow::dialWheelHolding () const
{
  return m_dialWheelClock.isValid () && m_dialWheelClock.elapsed () < 1500 && !m_transmitting;
}

/* CE3TSK: the dark style was switched - from Settings, the first-run colour offer or View > Use
   dark style. The decoded lines already on screen carry the colors of the style they were
   written under, baked into their HTML - after the switch they are the wrong ones (a dark
   country column on a white background, and worse the other way). DisplayText keeps no source
   rows, and Radio::convert_dark clamps at 0 so it cannot be inverted, so the only honest option
   is to start both windows again. */
void MainWindow::darkStyleChanged ()
{
  m_useDarkStyle = m_config.useDarkStyle(); setDecodeMenuColours();
  // the windows colour each new line from the style flag they copied in setConfiguration ()
  ui->decodedTextBrowser->setConfiguration (&m_config);
  ui->decodedTextBrowser2->setConfiguration (&m_config);
  ui->actionUse_dark_style->setChecked (m_useDarkStyle);
  ui->decodedTextBrowser->clear(); ui->decodedTextBrowser2->clear();
  styleChanged();
}

/* CE3TSK: View > Use dark style, the same switch as the check box in Settings, General */
void MainWindow::on_actionUse_dark_style_triggered (bool checked)
{
  m_config.set_dark_style (checked);
  if (m_config.useDarkStyle () != m_useDarkStyle)
    {
      darkStyleChanged ();
      if(m_config.write_decoded_debug()) writeToALLTXT("Both windows cleared, triggered by dark style change");
    }
  else ui->actionUse_dark_style->setChecked (m_useDarkStyle);   // the style could not be switched
}

void MainWindow::styleChanged()
{
  updateTimingLamps();   // CE3TSK: the lamps carry their own palette, light and dark

  ui->pbTxLock->setChecked(m_lockTxFreq);
  on_pbTxLock_clicked(m_lockTxFreq);
  ui->tx5->setStyleSheet(QString("QComboBox {selection-background-color: %1;background: %1}").arg(Radio::convert_dark("#ffffff",m_useDarkStyle)));
  on_directionLineEdit_textChanged(m_cqdir);
  on_direction1LineEdit_textChanged(m_cqdir);
  if(!m_uploadSpots) ui->cbUploadWSPR_Spots->setStyleSheet(QString("QCheckBox{background: %1}").arg(Radio::convert_dark("#ffff00",m_useDarkStyle)));
  ui->label_4->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#aabec8",m_useDarkStyle)));
  ui->label_9->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#aabec8",m_useDarkStyle)));
  ui->label_10->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#aabec8",m_useDarkStyle)));
  if (!m_mode.startsWith ("WSPR")) {
	if (m_config.prompt_to_log ()) { qso_count_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#99ff99",m_useDarkStyle))); }
	else if (m_config.autolog ()) { qso_count_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#9999ff",m_useDarkStyle))); }
	else { qso_count_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#ffffff",m_useDarkStyle))); }
  }
setDxCallEntryColour (m_dxCallEntryColour.isEmpty () ? QString {"#ffffff"} : m_dxCallEntryColour);
setEnableTxButtonStyle ();
  /* CE3TSK: the Ko-fi artwork has a light and a dark variant, swap with the style */
  if (auto * kofi = qobject_cast<QToolButton *> (ui->menuBar->cornerWidget (Qt::TopRightCorner)))
    kofi->setIcon (QIcon {m_useDarkStyle ? ":/support_cup_dark.png" : ":/support_cup_light.png"});
  setLastLogdLabel();
  setAutoSeqButtonStyle(m_autoseq);
  setSpotButtonStyle ();
//  ui->txrb1->setStyleSheet(QString("QRadioButton::indicator:checked:disabled{background: %1;width: 6px;height: 6px;border-radius: 3px;margin-left: 3px}").arg(Radio::convert_dark("#222222",m_useDarkStyle)));
//  ui->txrb2->setStyleSheet(QString("QRadioButton::indicator:checked:disabled{background: %1;width: 6px;height: 6px;border-radius: 3px;margin-left: 3px}").arg(Radio::convert_dark("#222222",m_useDarkStyle)));
//  ui->txrb3->setStyleSheet(QString("QRadioButton::indicator:checked:disabled{background: %1;width: 6px;height: 6px;border-radius: 3px;margin-left: 3px}").arg(Radio::convert_dark("#222222",m_useDarkStyle)));
//  ui->txrb4->setStyleSheet(QString("QRadioButton::indicator:checked:disabled{background: %1;width: 6px;height: 6px;border-radius: 3px;margin-left: 3px}").arg(Radio::convert_dark("#222222",m_useDarkStyle)));
//  ui->txrb5->setStyleSheet(QString("QRadioButton::indicator:checked:disabled{background: %1;width: 6px;height: 6px;border-radius: 3px;margin-left: 3px}").arg(Radio::convert_dark("#222222",m_useDarkStyle)));
//  ui->txrb6->setStyleSheet(QString("QRadioButton::indicator:checked:disabled{background: %1;width: 6px;height: 6px;border-radius: 3px;margin-left: 3px}").arg(Radio::convert_dark("#222222",m_useDarkStyle)));
  if(m_autoseq && !m_autoTx) ui->AutoTxButton->setStyleSheet(QString("QPushButton{color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#ffbbbb",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle)));
  else ui->AutoTxButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->tuneButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#ff0000",m_useDarkStyle)));
  ui->monitorButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->bypassButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->singleQSOButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->AnsB4Button->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->swlButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->filterButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->AGCcButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->hintButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->syncButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ff00",m_useDarkStyle)));
  ui->DecodeButton->setStyleSheet(QString("QPushButton:checked{background: %1}").arg(Radio::convert_dark("#00ffff",m_useDarkStyle)));
  /* CE3TSK: the colours that follow a state rather than the style alone, painted again so a switch
     looks the way a fresh start in the new style would (see setRigLamp) */
  if (!m_rigLampColour.isEmpty ()) setRigLamp (m_rigLampColour);
  if (!m_bandLabelColour.isEmpty ()) setBandLabelColour (m_bandLabelColour);
  if (!m_mode.isEmpty ()) setModeLabelStyle (m_mode);
  setClockStyle (true);   // the clock, and in WSPR the TX minute button
  if (!m_mode.startsWith ("WSPR")) setMinButton ();
  setProgressBarStyle ();
  setTxStatusColour (m_txStatusColour);
  if (!ui->HoundButton->styleSheet ().isEmpty ()) setHoundButtonStyle ();
  if (m_txbColorSet) setTxMsgBtnColor ();
  on_tx5_currentTextChanged (ui->tx5->currentText ());
  on_freeTextMsg_currentTextChanged (ui->freeTextMsg->currentText ());
  if (!ui->bandComboBox->lineEdit ()->styleSheet ().isEmpty ())   // out of band
    ui->bandComboBox->lineEdit ()->setStyleSheet (QString("QLineEdit {color: %1; background-color : %2}").arg(Radio::convert_dark("#ffff00",m_useDarkStyle),Radio::convert_dark("#ff0000",m_useDarkStyle)));
  if (Qt::RichText == ui->decodedTextLabel->textFormat ()) updateDecodeLabel ();   // Avg= and Lag= carry colours
  m_wideGraph->setDarkStyle(m_useDarkStyle);
  statusUpdate ();
}

void MainWindow::statusChanged()
{
  statusUpdate ();
  QFile f {m_config.temp_dir ().absoluteFilePath ("wsjtx_status.txt")};
  if(f.open(QFile::WriteOnly | QIODevice::Text)) {
    QTextStream out(&f);
    out << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6)
        << ";" << m_mode << ";" << m_hisCall << ";"
        << ui->rptSpinBox->value() << ";" << m_modeTx <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

    f.close();
  } else {
    JTDXMessageBox::warning_message (this, "", tr ("File Open Error")
                                 , tr ("Cannot open \"%1\" for append: %2")
                                 .arg (f.fileName ()).arg (f.errorString ()));
  }
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)  //eventFilter()
{
  switch (event->type())
    {
    case QEvent::WindowActivate:
      txwatchdog (false);
      break;
    case QEvent::KeyPress:
      // fall through
    case QEvent::MouseButtonPress:
      txwatchdog (false);
      break;

    case QEvent::ChildAdded:
      // ensure our child widgets get added to our event filter
      add_child_to_event_filter (static_cast<QChildEvent *> (event)->child ());
      break;

    case QEvent::ChildRemoved:
      // ensure our child widgets get d=removed from our event filter
      remove_child_from_event_filter (static_cast<QChildEvent *> (event)->child ());
      break;

    case QEvent::ToolTip:
      if(!m_showTooltips) return true;
      break;

    case QEvent::Wheel:
      // CE3TSK: the wheel over a kHz digit of the dial frequency tunes it
      if (object == ui->labDialFreq && dialFrequencyWheel (static_cast<QWheelEvent *> (event))) return true;
      break;

    case QEvent::MouseButtonDblClick:
      // CE3TSK: the DX Call button or the callsign box looks the call up on qrz.com
      if (object == ui->pbSpotDXCall || object == ui->dxCallEntry)
        {
          lookupDxCallOnQrz ();
          if (object == ui->pbSpotDXCall) return true;   // the box keeps its own word selection
        }
      break;

    default: break;
    }

  return QObject::eventFilter(object, event);
}

void MainWindow::createStatusBar()                           //createStatusBar
{
  statusBar()->setMinimumHeight (30);
  statusBar()->setContentsMargins(2,1,2,2);

  tx_status_label->setAlignment(Qt::AlignHCenter);
  tx_status_label->setAlignment(Qt::AlignVCenter);
  tx_status_label->setContentsMargins(1,1,1,1); //(int left, int top, int right, int bottom)
  tx_status_label->setMinimumSize(QSize(150,20));
  setTxStatusColour ("#00ff00");
  tx_status_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(tx_status_label);

  mode_label->setAlignment(Qt::AlignHCenter);
  mode_label->setAlignment(Qt::AlignVCenter);
  mode_label->setContentsMargins(1,1,1,1);
  mode_label->setMinimumSize(QSize(63,20));
  mode_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(mode_label);

  last_tx_label->setAlignment(Qt::AlignHCenter);
  last_tx_label->setAlignment(Qt::AlignVCenter);
  last_tx_label->setContentsMargins(1,1,1,1);
  last_tx_label->setMinimumSize(QSize(140,20));
  last_tx_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(last_tx_label);

  txwatchdog_label->setAlignment(Qt::AlignHCenter);
  txwatchdog_label->setAlignment(Qt::AlignVCenter);
  txwatchdog_label->setContentsMargins(1,1,1,1);
  txwatchdog_label->setMinimumSize(QSize(52,20));
  txwatchdog_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(txwatchdog_label);
  update_watchdog_label ();



  statusBar()->addWidget(progressBar,1);
  progressBar->setMinimumWidth(40);
  progressBar->setFormat("%v/"+QString::number(m_TRperiod));

  lastlogged_label->setAlignment(Qt::AlignHCenter);
  lastlogged_label->setAlignment(Qt::AlignVCenter);
  lastlogged_label->setContentsMargins(1,1,1,1);
  lastlogged_label->setMinimumSize(QSize(76,20));
  lastlogged_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(lastlogged_label);

  date_label->setAlignment(Qt::AlignHCenter);
  date_label->setAlignment(Qt::AlignVCenter);
  date_label->setContentsMargins(1,1,1,1);
  date_label->setMinimumSize(QSize(76,20));
  date_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(date_label);


  qso_count_label->setAlignment(Qt::AlignHCenter);
  qso_count_label->setAlignment(Qt::AlignVCenter);
  qso_count_label->setContentsMargins(1,1,1,1);
  qso_count_label->setMinimumSize(QSize(100,20));
  qso_count_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(qso_count_label);

  /* CE3TSK: contest score, rightmost and hidden unless a contest is running */
  contest_score_label->setAlignment(Qt::AlignHCenter);
  contest_score_label->setAlignment(Qt::AlignVCenter);
  contest_score_label->setContentsMargins(1,1,1,1);
  contest_score_label->setMinimumSize(QSize(150,20));
  contest_score_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  contest_score_label->setToolTip(tr("Contest multipliers x QSO points = score"));
  contest_score_label->setVisible(false);
  statusBar()->addWidget(contest_score_label);
 }

bool MainWindow::subProcessFailed (QProcess * process, int exit_code, QProcess::ExitStatus status)
{
  if (m_valid && (exit_code || QProcess::NormalExit != status))
    {
      QStringList arguments;
      for (auto argument: process->arguments ())
        {
          if (argument.contains (' ')) argument = '"' + argument + '"';
          arguments << argument;
        }
      JTDXMessageBox::critical_message (this, "", tr ("Subprocess Error")
                                    , tr ("Subprocess failed with exit code %1")
                                    .arg (exit_code)
                                    , tr ("Running: %1\n%2")
                                    .arg (process->program () + ' ' + arguments.join (' '))
                                    .arg (QString {process->readAllStandardError()}));
      return true;          // ensures exit if still constructing
    }
  return false;  
}

void MainWindow::subProcessError (QProcess * process, QProcess::ProcessError)
{
  if (m_valid)
    {
      QStringList arguments;
      for (auto argument: process->arguments ())
        {
          if (argument.contains (' ')) argument = '"' + argument + '"';
          arguments << argument;
        }
      JTDXMessageBox::critical_message (this, "", tr ("Subprocess error")
                                    , tr ("Running: %1\n%2")
                                    .arg (process->program () + ' ' + arguments.join (' '))
                                    .arg (process->errorString ()));
      QTimer::singleShot (0, this, SLOT (close ()));
      m_valid = false;              // ensures exit if still constructing
    }
}

void MainWindow::closeEvent(QCloseEvent * e)
{
  m_valid = false;              // suppresses subprocess errors
  if(m_config.clear_DX_exit())
    {
      clearDX ("");
    }
  m_config.transceiver_offline ();
  writeSettings ();
  m_guiTimer.stop ();
  m_prefixes.reset ();
  m_shortcuts.reset ();
  m_mouseCmnds.reset ();

  killFile ();
  m_killAll=true;
  mem_jtdxjt9->detach();
  QFile quitFile {m_config.temp_dir ().absoluteFilePath (".quit")};
  quitFile.open(QIODevice::ReadWrite);
  QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.remove(); // Allow jtdxjt9 to terminate
  bool b=proc_jtdxjt9.waitForFinished(1000);
  if(!b) proc_jtdxjt9.close();
  quitFile.remove();
  Q_EMIT finished ();
  QMainWindow::closeEvent (e);
}

void MainWindow::on_stopButton_clicked() { monitor (false); m_loopall=false; if (!m_transmitting) initLogIfNeeded (); }
void MainWindow::on_AnsB4Button_clicked (bool checked) { ui->actionAnswerWorkedB4->setChecked(checked); }
void MainWindow::on_singleQSOButton_clicked (bool checked) { ui->actionSingleShot->setChecked(checked); }
void MainWindow::on_bypassButton_clicked (bool checked) { ui->actionBypass_all_text_filters->setChecked(checked); }

void MainWindow::on_pbSpotDXCall_clicked ()
{
  if(m_config.spot_to_dxsummit() && !m_spotDXsummit && !m_hisCall.isEmpty() && !m_config.my_callsign().isEmpty ()) {
//http://www.dxsummit.fi/SendSpot.aspx?callSign=CE3TSK&dxCallSign=8P6PD&frequency=28074&info=FT8
    QUrl url("http://www.dxsummit.fi/SendSpot.aspx");
    double frequency=(m_freqNominal + ui->RxFreqSpinBox->value())/1000.0;
    QUrlQuery query;
    query.addQueryItem("callSign", m_config.my_callsign());
    query.addQueryItem("dxCallSign", m_hisCall);
    query.addQueryItem("frequency", QString::number(frequency,'f',1));
    QString spotInfoText=ui->spotMsgLabel->text(); spotInfoText.remove(0,6);
    query.addQueryItem("info", spotInfoText);
    url.setQuery(query.query());
    QEventLoop eventLoop;
    QTimer timer;
    timer.setSingleShot(true);
    QNetworkAccessManager mgr;
    QNetworkRequest req( url );
    QNetworkReply *reply = mgr.get(req);
    QObject::connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    QObject::connect( reply, SIGNAL(finished()), &eventLoop, SLOT(quit()) );
    timer.start(10*1000);
    eventLoop.exec( QEventLoop::ExcludeUserInputEvents );
    if(timer.isActive()) {
      timer.stop();
//    if (reply->error() == QNetworkReply::NoError) {
//      printf("Success : %s\n",reply->readAll().toStdString().c_str());
//    } else {
//      printf("Failure : %s\n",reply->errorString().toStdString().c_str());
//    }
      m_dxCallHidden=false;   // CE3TSK
      ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#c4ffc4",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle)));
      ui->pbSpotDXCall->setText(tr("Spotted"));
      m_spotDXsummit=true;
    } else {
   // timeout
     QObject::disconnect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));

     reply->abort();
     JTDXMessageBox::critical_message(0, "", "Critical", tr("Can not establish/complete connection to dxsummit server"));
    }
    delete reply;
  }
}

void MainWindow::msgBox(QString t) { msgBox0.setText(t); msgBox0.translate_buttons(); msgBox0.exec(); }
void MainWindow::on_actionJTDX_Web_Site_triggered() { m_manual.display_html_url (QUrl {"https://ce3tsk.com/"}, ""); }   /* CE3TSK: the fork's site (F1) - the only user of DisplayManual; JTDX's forum,
                                                              sample-download and manual items are gone */

void MainWindow::on_actionWide_Waterfall_triggered() { m_wideGraph->show(); } //Display Waterfalls

void MainWindow::on_actionCopyright_Notice_triggered()
{
  JTDXMessageBox::information_message(this, "", tr("The algorithms, source code, look-and-feel of WSJT-X and related "
                           "programs, and protocol specifications for the modes FSK441, FT8, JT4, "
                           "JT6M, JT9, JT65, JTMS, QRA64, ISCAT, MSK144 are Copyright (C) "
                           "2001-2018 by one or more of the following authors: Joseph Taylor, "
                           "K1JT; Bill Somerville, G4WJS; Steven Franke, K9AN; Nico Palermo, "
                           "IV3NWV; Greg Beam, KI7MT; Michael Black, W9MDB; Edson Pereira, PY2SDR; "
                           "Philip Karn, KA9Q; and other members of the WSJT Development Group."));

}

void MainWindow::hideMenus(bool checked)
{
  ui->menuBar->setVisible(!checked);
  ui->label_6->setVisible(!checked);
  if(!m_mode.startsWith ("WSPR")) {
     ui->label_7->setVisible(!checked);
     ui->decodedTextLabel2->setVisible(!checked);
  }
//  ui->decodedTextLabel->setVisible(!checked);
  ui->gridLayout_14->layout()->setSpacing(0);
  ui->gridLayout_14->layout()->setContentsMargins(0,0,0,0);
  ui->horizontalLayout_10->layout()->setSpacing(0);
  ui->horizontalLayout_10->layout()->setContentsMargins(0,0,0,0);
  ui->verticalLayout->layout()->setSpacing(1);
  ui->verticalLayout->layout()->setContentsMargins(0,0,0,0);
  ui->verticalLayout_7->layout()->setSpacing(1);
  ui->verticalLayout_7->layout()->setContentsMargins(0,0,0,0);
}

void MainWindow::on_actionOpen_triggered()                     //Open File
{
  monitor (false);

  QString fname;
  fname=QFileDialog::getOpenFileName(this, "Open File", m_path,
                                     "WSJT Files (*.wav)");
  if(!fname.isEmpty ()) {
    m_path=fname;
    int i1=fname.lastIndexOf("/");
    QString baseName=fname.mid(i1+1);
    setTxStatusColour ("#99ffff");
    tx_status_label->setText(" " + baseName + " ");
    on_stopButton_clicked();
    m_diskData=true;
    read_wav_file (fname);
  }
}

/* CE3TSK: File -> Convert bit depth (32/16). Detects the bit depth of a chosen wav file and
   offers a converted copy under the original name with a _16 or _32 suffix. Both directions
   go through BWFFile, which parses the bext chunk and the LIST-INFO dictionary on read and
   writes them back on write, so everything JTDX stores in a saved file - callsign and grid
   in ISRC, program title in ISFT, creation time in ICRD, mode/frequency/DX in ICMT - is
   carried over. The one thing the suffix costs is the filename timestamp: replaying the
   converted copy parses no time from the name, which only matters for QsoHistory ordering. */
/* CE3TSK: one file's conversion, lifted out of the menu handler so a batch can use it too.
   `oname` empty means "name it yourself": beside the source, with the depth appended, and
   refuse rather than overwrite. Returns false with `err` set on any failure; err == EXISTS is
   the one case a batch reports as skipped rather than failed. Nothing is written on failure. */
/* the sentinel that separates "already converted" from a real failure; not a message, so it is
   never shown to the operator */
static QString const EXISTS {QStringLiteral ("__exists__")};

bool MainWindow::convert_wav_depth (QString const& fname, QString& oname, int& out_bits, QString& err)
{
  BWFFile in {QAudioFormat {}, fname};
  if (!in.open (BWFFile::ReadOnly))
    {
      err = tr ("cannot be opened");
      return false;
    }
  auto const& fmt = in.format ();
  int const bits = fmt.sampleSize ();
  if ((16 != bits && 32 != bits) || QAudioFormat::SignedInt != fmt.sampleType ()
      || 1 != fmt.channelCount ())
    {
      err = tr ("is not a mono 16 or 32 bit signed integer wav (%1 bit, %2 channel(s))")
              .arg (bits).arg (fmt.channelCount ());
      return false;
    }
  out_bits = 32 == bits ? 16 : 32;

  QFileInfo fi {fname};
  /* the suffix is appended, never substituted, so a converted file's name keeps its full
     conversion history: A_16.wav becomes A_16_32.wav, and so on. Deliberate - replacing a
     trailing depth suffix was tried and rejected, since the chain shows provenance. */
  if (oname.isEmpty ())
    {
      /* a batch names its own output: beside the source, with the depth appended */
      oname = fi.path () + '/' + fi.completeBaseName ()
              + (16 == out_bits ? "_16" : "_32") + ".wav";
      if (QFile::exists (oname)) { err = EXISTS; return false; }
    }

  QByteArray data;
  data.resize (int (in.size ()));
  auto const n = in.read (data.data (), data.size ());
  data.truncate (int (std::max (qint64 {0}, n)));

  QByteArray converted;
  if (16 == out_bits)
    {
      auto const* src = reinterpret_cast<qint32 const*> (data.constData ());
      int const frames = data.size () / 4;
      converted.resize (frames * 2);
      wav32to16 (src, reinterpret_cast<qint16*> (converted.data ()), frames);
    }
  else
    {
      auto const* src = reinterpret_cast<qint16 const*> (data.constData ());
      int const frames = data.size () / 2;
      converted.resize (frames * 4);
      wav16to32 (src, reinterpret_cast<qint32*> (converted.data ()), frames);
    }

  QAudioFormat ofmt = fmt;
  ofmt.setSampleSize (out_bits);
  /* CE3TSK: BWFFile's read keeps each LIST-INFO value's terminating NUL and its write
     appends another - a pre-existing round trip asymmetry that grows every value by one NUL
     per pass. Nothing in JTDX read-then-wrote metadata before this converter, so it never
     showed. Trimming here and letting the writer add its single terminator makes the
     conversion byte-identical on the metadata. */
  auto info = in.list_info ();
  for (auto& v : info) while (v.endsWith ('\0')) v.chop (1);
  BWFFile out {ofmt, oname, info};
  /* bext, if the source carried one. The public API exposes it field by field; copying only
     when the source description or originator is non-empty avoids stamping an empty bext
     chunk onto files that never had one - JTDX itself writes only LIST-INFO. */
  /* the const view matters: bext_version's setter has a defaulted parameter, so on a
     non const object the no-argument call resolves to the setter and returns void */
  BWFFile const& cin = in;
  if (!cin.bext_description ().isEmpty () || !cin.bext_originator ().isEmpty ())
    {
      out.bext_version (cin.bext_version ());
      out.bext_description (cin.bext_description ());
      out.bext_originator (cin.bext_originator ());
      out.bext_originator_reference (cin.bext_originator_reference ());
      out.bext_origination_date_time (cin.bext_origination_date_time ());
      out.bext_time_reference (cin.bext_time_reference ());
      out.bext_coding_history (cin.bext_coding_history ());
    }
  if (!out.open (BWFFile::WriteOnly) || out.write (converted) != converted.size ())
    {
      err = tr ("could not be written to %1").arg (QFileInfo {oname}.fileName ());
      return false;
    }
  if(m_config.write_decoded() || m_config.write_decoded_debug())
    writeToALLTXT (QString {"Converted %1 to %2 bit: %3"}.arg (fname.mid (fname.lastIndexOf ('/') + 1)).arg (out_bits).arg (oname.mid (oname.lastIndexOf ('/') + 1)));
  return true;
}

void MainWindow::on_actionConvert_bit_depth_triggered()
{
  /* the dialog reopens where the last conversion was picked, the same MRU pattern File ->
     Open uses with m_path; falls back to the save directory on first use. Several files may
     be chosen at once: one is asked where to save, a batch names its own outputs beside the
     sources so the operator is not asked once per file. */
  QStringList fnames = QFileDialog::getOpenFileNames (this, tr ("Convert bit depth"),
                                                      m_convertPath,
                                                      tr ("WSJT Files (*.wav)"));
  if (fnames.isEmpty ()) return;
  m_convertPath = fnames.first ();
  /* written immediately, not only in writeSettings(): that runs on a clean close, and a
     killed or crashed session would otherwise forget the directory */
  m_settings->setValue ("ConvertMRUdir", m_convertPath);

  if (1 == fnames.size ())
    {
      QString const& fname = fnames.first ();
      QFileInfo fi {fname};
      /* the suffix is appended, never substituted, so a converted file's name keeps its full
         conversion history: A_16.wav becomes A_16_32.wav, and so on. Deliberate - replacing a
         trailing depth suffix was tried and rejected, since the chain shows provenance. The
         depth is not known before the file is opened, so the suggestion offers both and the
         helper corrects it. */
      BWFFile probe {QAudioFormat {}, fname};
      int probe_bits = 0;
      if (probe.open (BWFFile::ReadOnly)) { probe_bits = probe.format ().sampleSize (); probe.close (); }
      /* the name is always derived from the source; only the directory follows the operator -
         the last place a converted copy was saved, or beside the source until there is one */
      QString const outdir = m_convertOutPath.isEmpty () ? fi.path () : m_convertOutPath;
      QString const suggested = outdir + '/' + fi.completeBaseName ()
                                + (32 == probe_bits ? "_16" : "_32") + ".wav";
      QString oname = QFileDialog::getSaveFileName (this, tr ("Save converted file"), suggested,
                                                    tr ("WSJT Files (*.wav)"));
      if (oname.isEmpty ()) return;
      if (!oname.endsWith (".wav", Qt::CaseInsensitive)) oname += ".wav";
      int out_bits = 0; QString err;
      if (!convert_wav_depth (fname, oname, out_bits, err))
        {
          JTDXMessageBox::warning_message (this, "", tr ("Cannot convert"),
                                           QFileInfo {fname}.fileName () + ' ' + err);
          return;
        }
      m_convertOutPath = QFileInfo {oname}.path ();
      /* written at once, as the input directory is: a killed session would otherwise forget it */
      m_settings->setValue ("ConvertOutMRUdir", m_convertOutPath);
      JTDXMessageBox::information_message (this, "",
        tr ("Saved %1 bit copy as %2").arg (out_bits).arg (oname.mid (oname.lastIndexOf ('/') + 1)));
      return;
    }

  /* the batch: every file converted beside itself, an existing target skipped rather than
     overwritten, and one summary at the end instead of a dialog per file */
  int done = 0, skipped = 0;
  QStringList failures;
  for (auto const& fname : fnames)
    {
      QString oname; int out_bits = 0; QString err;
      if (convert_wav_depth (fname, oname, out_bits, err)) ++done;
      else if (EXISTS == err) ++skipped;
      else failures << QFileInfo {fname}.fileName () + ' ' + err;
    }
  QString msg = tr ("%1 of %2 files converted").arg (done).arg (fnames.size ());
  if (skipped) msg += '\n' + tr ("%1 skipped, the converted file already exists").arg (skipped);
  if (!failures.isEmpty ()) msg += '\n' + tr ("%1 failed:").arg (failures.size ()) + '\n' + failures.join ('\n');
  if (failures.isEmpty ()) JTDXMessageBox::information_message (this, "", msg);
  else JTDXMessageBox::warning_message (this, "", tr ("Convert bit depth"), msg);
}

void MainWindow::read_wav_file (QString const& fname)
{
  // call diskDat() when done
  m_wav_future_watcher.setFuture (QtConcurrent::run ([this, fname] {
    if(m_config.write_decoded() || m_config.write_decoded_debug()) { QString basename = fname.mid (fname.lastIndexOf ('/') + 1); writeToALLTXT("Reading wav file " + basename); }
    auto pos = fname.indexOf (".wav", 0, Qt::CaseInsensitive);
    // global variables and threads do not mix well, this needs changing
    dec_data.params.nutc = 0;
    if (pos > 0) {
      if (pos == fname.indexOf ('_', -11) + 7) dec_data.params.nutc = fname.mid (pos - 6, 6).toInt ();
      else dec_data.params.nutc = 100 * fname.mid (pos - 4, 4).toInt ();
    }
    BWFFile file {QAudioFormat {}, fname};
    file.open (BWFFile::ReadOnly);
    auto const& format = file.format ();
    auto bytes_per_frame = format.bytesPerFrame ();
    int nsamples=m_TRperiod * RX_SAMPLE_RATE;
    std::size_t const capacity = std::min (std::size_t (nsamples), sizeof (dec_data.d2) / sizeof (dec_data.d2[0]));
    int frames_read = 0;
    if (11025 == format.sampleRate ()) {
      /* the legacy resampling path, deliberately untouched: wav12_ receives the sample size
         and has always dealt with these files itself */
      qint64 max_bytes = qint64 (capacity) * bytes_per_frame;
      auto n = file.read (reinterpret_cast<char *> (dec_data.d2), std::min (max_bytes, file.size ()));
      frames_read = n / bytes_per_frame;
      short sample_size = format.sampleSize ();
      wav12_ (dec_data.d2, dec_data.d2, &frames_read, &sample_size);
    } else if (16 == format.sampleSize () && QAudioFormat::SignedInt == format.sampleType ()
               && 1 == format.channelCount ()) {   /* stereo would interleave-mangle */
      /* the native depth again since 2026-09-03 (AUDIO_DEPTH_PLAN.md): WSJT-X, JTDX and
         this fork all write these - a plain read into d2. BWFFile has parsed the real chunk
         layout to find the data, so a LIST-INFO chunk ahead of it is no problem. */
      qint64 max_bytes = qint64 (capacity) * bytes_per_frame;
      auto n = file.read (reinterpret_cast<char *> (dec_data.d2), std::min (max_bytes, file.size ()));
      frames_read = n / bytes_per_frame;
    } else if (32 == format.sampleSize () && QAudioFormat::SignedInt == format.sampleType ()
               && 1 == format.channelCount ()) {
      /* CE3TSK: a 32 bit file - every recording this fork saved before 2026-09-03 and all of
         its archived benchmark sets - contracted to 16 bit on the fly, the top 16 bits kept
         (wav16to32.h). The file on disk is never touched. Chunked through a scratch buffer
         rather than parked in place: d2 cannot hold four bytes of source plus two of result
         per sample for the 60 s modes, and a plain loop has nothing to get wrong. */
      qint32 scratch[16384];
      std::size_t const chunk = sizeof (scratch) / sizeof (scratch[0]);
      while (std::size_t (frames_read) < capacity) {
        std::size_t const want = std::min (chunk, capacity - std::size_t (frames_read));
        auto n = file.read (reinterpret_cast<char *> (scratch), qint64 (want) * sizeof (scratch[0]));
        if (n <= 0) break;
        int const got = int (n / sizeof (scratch[0]));
        wav32to16 (scratch, dec_data.d2 + frames_read, got);
        frames_read += got;
        if (std::size_t (got) < want) break;
      }
      if(m_config.write_decoded() || m_config.write_decoded_debug()) writeToALLTXT("32-bit wav file contracted to 16-bit on the fly");
    } else {
      /* float or 8 bit: reading it raw would decode as noise and look like a JTDX fault */
      writeToALLTXT (QString {"Unsupported wav format: %1 bit, sample type %2, %3 channel(s) - file not read"}
                     .arg (format.sampleSize ()).arg (int (format.sampleType ())).arg (format.channelCount ()));
    }
    /* CE3TSK: zero the unfilled remainder, in elements. The old code added the BYTE count n
       to the sample pointer d2, advancing twice (four times, while d2 was int) too far -
       harmless on a full length file where nothing remained, but a short file zeroed far
       past the intended region into the globals behind d2. */
    if (std::size_t (frames_read) < capacity)
      std::memset (dec_data.d2 + frames_read, 0, (capacity - std::size_t (frames_read)) * sizeof (dec_data.d2[0]));
    dec_data.params.kin = frames_read;
    dec_data.params.newdat = 1;
  }));
}

void MainWindow::on_actionOpen_next_in_directory_triggered()   //Open Next
{
  monitor (false);

  int i,len;
  QFileInfo fi(m_path);
  QStringList list;
  list= fi.dir().entryList().filter(".wav",Qt::CaseInsensitive);
  for (i = 0; i < list.size()-1; ++i) {
    if(i==list.size()-2) m_loopall=false;
    len=list.at(i).length();
    if(list.at(i)==m_path.right(len)) {
      int n=m_path.length();
      QString fname=m_path.replace(n-len,len,list.at(i+1));
      m_path=fname;
      int i1=fname.lastIndexOf("/");
      QString baseName=fname.mid(i1+1);
      setTxStatusColour ("#99ffff");
      tx_status_label->setText(" " + baseName + " ");
      m_diskData=true;
      read_wav_file (fname);
      return;
    }
  }
}
//Open all remaining files
void MainWindow::on_actionDecode_remaining_files_in_directory_triggered() { m_loopall=true; on_actionOpen_next_in_directory_triggered(); }

void MainWindow::diskDat()                                   //diskDat()
{
  if(dec_data.params.kin>0) {
    int k;
    int kstep=m_FFTSize;
    m_diskData=true;
    for(int n=1; n<=m_hsymStop; n++) {                      // Do the waterfall spectra
      k=(n+1)*kstep;
//      if(k > dec_data.params.kin) break;
      dec_data.params.npts8=k/8;
      dataSink(k);
      qApp->processEvents();                                //Update the waterfall
    }
  } else {
    JTDXMessageBox::information_message(this, "", tr("No data read from disk. Wrong file format?"));
  }
}

//Delete ../save/*.wav
void MainWindow::on_actionDelete_all_wav_files_in_SaveDir_triggered()
{
  if (JTDXMessageBox::Yes == JTDXMessageBox::warning_message(this, "", tr("Confirm Delete"),
                                              tr("Are you sure you want to delete all *.wav and *.c2 files in\n") +
                                              QDir::toNativeSeparators(m_config.save_directory ().absolutePath ()) + " ?",
                                              "", JTDXMessageBox::Yes | JTDXMessageBox::No, JTDXMessageBox::Yes)) {
    Q_FOREACH (auto const& file
               , m_config.save_directory ().entryList ({"*.wav", "*.c2"}, QDir::Files | QDir::Writable)) {
      m_config.save_directory ().remove (file);
    }
  }
}

void MainWindow::on_actionNone_triggered() { m_saveWav=0; ui->actionNone->setChecked(true); }
void MainWindow::on_actionSave_decoded_triggered() { m_saveWav=1; ui->actionSave_decoded->setChecked(true); }
void MainWindow::on_actionSave_all_triggered() { m_saveWav=2; ui->actionSave_all->setChecked(true); }

void MainWindow::on_actionEnglish_triggered() { ui->actionEnglish->setChecked(true); set_language("en_US"); }
void MainWindow::on_actionEstonian_triggered() { ui->actionEstonian->setChecked(true); set_language("et_EE"); }
void MainWindow::on_actionRussian_triggered() { ui->actionRussian->setChecked(true); set_language("ru_RU"); }
void MainWindow::on_actionCatalan_triggered() { ui->actionCatalan->setChecked(true); set_language("ca_ES"); }
void MainWindow::on_actionCroatian_triggered() { ui->actionCroatian->setChecked(true); set_language("hr_HR"); }
void MainWindow::on_actionDanish_triggered() { ui->actionDanish->setChecked(true); set_language("da_DK"); }
void MainWindow::on_actionDutch_triggered() { ui->actionDutch->setChecked(true); set_language("nl_NL"); }
void MainWindow::on_actionHungarian_triggered() { ui->actionHungarian->setChecked(true); set_language("hu_HU"); }
void MainWindow::on_actionSpanish_triggered() { ui->actionSpanish->setChecked(true); set_language("es_ES"); }
void MainWindow::on_actionSwedish_triggered() { ui->actionSwedish->setChecked(true); set_language("sv_SE"); }
void MainWindow::on_actionFrench_triggered() { ui->actionFrench->setChecked(true); set_language("fr_FR"); }
void MainWindow::on_actionItalian_triggered() { ui->actionItalian->setChecked(true); set_language("it_IT"); }
void MainWindow::on_actionGerman_triggered() { ui->actionGerman->setChecked(true); set_language("de_DE"); }   // CE3TSK
void MainWindow::on_actionLatvian_triggered() { ui->actionLatvian->setChecked(true); set_language("lv_LV"); }
void MainWindow::on_actionPolish_triggered() { ui->actionPolish->setChecked(true); set_language("pl_PL"); }
void MainWindow::on_actionPortuguese_triggered() { ui->actionPortuguese->setChecked(true); set_language("pt_PT"); }
void MainWindow::on_actionPortuguese_BR_triggered() { ui->actionPortuguese_BR->setChecked(true); set_language("pt_BR"); }
void MainWindow::on_actionChinese_simplified_triggered() { ui->actionChinese_simplified->setChecked(true); set_language("zh_CN"); }
void MainWindow::on_actionChinese_traditional_triggered() { ui->actionChinese_traditional->setChecked(true); set_language("zh_HK"); }
void MainWindow::on_actionJapanese_triggered() { ui->actionJapanese->setChecked(true); set_language("ja_JP"); }
void MainWindow::on_actionKorean_triggered() { ui->actionKorean->setChecked(true); set_language("ko_KR"); }   // CE3TSK

void MainWindow::on_actionCallNone_toggled(bool checked)
{
  m_callMode=0;
  if (checked) {
    if (m_callPrioCQ) {
      ui->actionCallPriorityAndSearchCQ->setChecked(false);
      m_callPrioCQ=false;
    }
    ui->actionCallPriorityAndSearchCQ->setEnabled(false);
    ui->AutoSeqButton->setText(tr("AutoSeq0"));
  }
}

void MainWindow::on_actionCallFirst_toggled(bool checked)
{
  m_callMode=1;
  if (checked) {
    if (m_callPrioCQ) {
      ui->actionCallPriorityAndSearchCQ->setChecked(false);
      m_callPrioCQ=false;
    }
    ui->actionCallPriorityAndSearchCQ->setEnabled(false);
    ui->AutoSeqButton->setText(tr("AutoSeq1"));
  }
}

void MainWindow::on_actionCallMid_toggled(bool checked)
{
  m_callMode=2;
  if (checked) {
    ui->actionCallPriorityAndSearchCQ->setEnabled(true);
	if (!m_callPrioCQ) ui->AutoSeqButton->setText(tr("AutoSeq2"));
	else ui->AutoSeqButton->setText(tr("AutoSeq6"));
  }  
}

void MainWindow::on_actionCallEnd_toggled(bool checked)
{
  m_callMode=3;
  if (checked) {
    ui->actionCallPriorityAndSearchCQ->setEnabled(true);
	if (!m_callPrioCQ) ui->AutoSeqButton->setText(tr("AutoSeq3"));
	else ui->AutoSeqButton->setText(tr("AutoSeq7"));
  }
}

 void MainWindow::on_actionCallPriorityAndSearchCQ_toggled(bool checked)
 {
   m_callPrioCQ=checked;
   if (checked) {
     ui->actionAutoFilter->setChecked(false);
     ui->actionAutoFilter->setEnabled(false);
     m_autofilter=false;
     if (m_callMode==2) ui->AutoSeqButton->setText(tr("AutoSeq6"));
     else ui->AutoSeqButton->setText(tr("AutoSeq7"));
   } else {
      /* CE3TSK: give AutoFilter back unless a contest is holding it down. It is otherwise
         independent of the waiting-caller behaviour, which no longer touches it. */
      if (!m_wwDigi) ui->actionAutoFilter->setEnabled(true);
      if (m_callMode==2) ui->AutoSeqButton->setText(tr("AutoSeq2"));
      else ui->AutoSeqButton->setText(tr("AutoSeq3"));
   }
 }

void MainWindow::on_actionMaxDistance_toggled(bool checked)
{
  if(checked && m_rprtPriority) { ui->actionReport_message_priority->setChecked(false); m_rprtPriority=false; }
  m_maxDistance=checked;
}

void MainWindow::on_actionAnswerWorkedB4_toggled(bool checked) { m_answerWorkedB4=checked; ui->AnsB4Button->setChecked(checked); }
void MainWindow::on_actionCallWorkedB4_toggled(bool checked) { m_callWorkedB4=checked; }
void MainWindow::on_actionCallHigherNewCall_toggled(bool checked) { m_callHigherNewCall=checked; }

void MainWindow::on_actionSingleShot_toggled(bool checked)
{
  m_singleshot=checked;
  setAutoSeqButtonStyle(m_autoseq);
  ui->singleQSOButton->setChecked(checked);
}

void MainWindow::on_actionAutoFilter_toggled(bool checked)
{
  m_autofilter=checked;
  if(m_autofilter && !ui->actionSwitch_Filter_OFF_at_sending_73->isChecked() && !ui->actionSwitch_Filter_OFF_at_getting_73->isChecked()) {
     ui->actionSwitch_Filter_OFF_at_sending_73->setChecked(true); m_FilterState=1;
  }
}

void MainWindow::on_actionEnable_hound_mode_toggled(bool checked)
{
// Don't allow Hound frequency control in common FT8 bands if VFO Split mode is switched off
  QString message = "";
  if(checked && !m_config.split_mode() && !m_commonFT8b && m_config.rig_name() != "None") {
    message =  tr ("Hound mode TX frequency control requires"
                                                        " *Split* rig control (either *Rig* or *Fake It* set"
                                                        " in the *Settings | Radio* tab.)");
    JTDXMessageBox::warning_message (this, "", tr ("Hound TX frequency control warning"), message);
    ui->actionEnable_hound_mode->setChecked(false);
    return;
  }
  m_houndTXfreqJumps=checked && !m_commonFT8b && m_config.split_mode() && m_config.rig_name() != "None";
  ui->actionUse_TX_frequency_jumps->setChecked(m_houndTXfreqJumps);
  m_houndMode=checked;
  m_wideGraph->setHoundFilter(m_houndMode);
  ui->HoundButton->setChecked(m_houndMode);
  if(m_houndMode) {
    setHoundButtonStyle ();
    if(m_skipTx1) { m_skipTx1=false; ui->skipTx1->setChecked(false); ui->skipGrid->setChecked(false); on_txb1_clicked(); m_wasSkipTx1=true; }
    ui->skipTx1->setEnabled(false); ui->skipGrid->setEnabled(false);
    if(!m_commonFT8b && m_config.rig_name() != "None") ui->actionUse_TX_frequency_jumps->setEnabled(true); }
  else {
    setHoundButtonStyle ();
    if(!m_wwDigi) { ui->skipTx1->setEnabled(true); ui->skipGrid->setEnabled(true); }   /* CE3TSK */
    if(m_wasSkipTx1) { 
      m_skipTx1=true; ui->skipTx1->setChecked(true); ui->skipGrid->setChecked(true);
      if(ui->txrb1->isChecked()) on_txb2_clicked();
      if(ui->genMsg->text() == ui->tx1->text()) ui->genMsg->setText(ui->tx2->text());
      m_wasSkipTx1=false;
	}
    ui->actionUse_TX_frequency_jumps->setEnabled(false);
  }
  setHoundAppearance(m_houndMode); if(!ui->spotLineEdit->text().isEmpty() && ui->spotLineEdit->text().contains("#H")) on_spotLineEdit_textChanged(ui->spotLineEdit->text());
}

void MainWindow::on_actionUse_TX_frequency_jumps_triggered (bool checked) { m_houndTXfreqJumps=checked; }
void MainWindow::on_actionMTAuto_triggered() { m_ft8threads=0; }
void MainWindow::on_actionMT1_triggered() { m_ft8threads=1; }
void MainWindow::on_actionMT2_triggered() { m_ft8threads=2; }
void MainWindow::on_actionMT3_triggered() { m_ft8threads=3; }
void MainWindow::on_actionMT4_triggered() { m_ft8threads=4; }
void MainWindow::on_actionMT5_triggered() { m_ft8threads=5; }
void MainWindow::on_actionMT6_triggered() { m_ft8threads=6; }
void MainWindow::on_actionMT7_triggered() { m_ft8threads=7; }
void MainWindow::on_actionMT8_triggered() { m_ft8threads=8; }
void MainWindow::on_actionMT9_triggered() { m_ft8threads=9; }
void MainWindow::on_actionMT10_triggered() { m_ft8threads=10; }
void MainWindow::on_actionMT11_triggered() { m_ft8threads=11; }
void MainWindow::on_actionMT12_triggered() { m_ft8threads=12; }
void MainWindow::on_actionMT13_triggered() { m_ft8threads=13; }
void MainWindow::on_actionMT14_triggered() { m_ft8threads=14; }
void MainWindow::on_actionMT15_triggered() { m_ft8threads=15; }
void MainWindow::on_actionMT16_triggered() { m_ft8threads=16; }
void MainWindow::on_actionMT17_triggered() { m_ft8threads=17; }
void MainWindow::on_actionMT18_triggered() { m_ft8threads=18; }
void MainWindow::on_actionMT19_triggered() { m_ft8threads=19; }
void MainWindow::on_actionMT20_triggered() { m_ft8threads=20; }
void MainWindow::on_actionMT21_triggered() { m_ft8threads=21; }
void MainWindow::on_actionMT22_triggered() { m_ft8threads=22; }
void MainWindow::on_actionMT23_triggered() { m_ft8threads=23; }
void MainWindow::on_actionMT24_triggered() { m_ft8threads=24; }
void MainWindow::on_actionAcceptUDPCQ_triggered() { m_acceptUDP=1; }
void MainWindow::on_actionAcceptUDPCQ73_triggered() { m_acceptUDP=2; }
void MainWindow::on_actionAcceptUDPAny_triggered() { m_acceptUDP=3; }
void MainWindow::on_actionDisableTx73_toggled(bool checked) { m_disable_TX_on_73 = checked; }
/* CE3TSK: at the end of a QSO answer a station that called us while we were busy, CQ only if nobody is waiting */
/* CE3TSK: the special operating activity is chosen on the Special operating tab of the
   settings dialog. Cache it and hand it to the parts that hold no Configuration pointer.
   Call after construction and after the settings dialog is accepted. The DisplayText
   widgets pick it up themselves in setConfiguration(). */
/* CE3TSK: the main window controls a contest takes over - Hound off, AutoTx on, single
   shot off, and the mode restricted to FT8 and FT4. AutoSeq, the call mode and priority
   CQ are deliberately left alone; the operator keeps whatever AutoSeq setting they run.

   Hound and single shot are switched off on the way in and left off: neither is parked
   and neither is handed back. Hound is a DXpedition mode and single shot stops the
   sequencer after one QSO; turning either back on silently after a contest is not a
   favour to the operator. Both are two widgets - a menu item and a button - kept in sync
   by their slots, so both have to be locked or the button stays live.

   Everything is driven through the slots rather than by poking the widgets, because the
   button colours and labels are derived state those slots maintain.

   Hound is the exception that proves the rule. It is an FT8 only feature, so every mode
   slot already drives its enabled flag through enableHoundAccess(), and a mode change
   does not run this function - selecting FT8 during a contest therefore re-enabled a
   control the contest had locked. Rather than lock it again from here, the contest rule
   was folded into enableHoundAccess(), which now owns that flag outright. */
/* CE3TSK: set the controls to what a contest requires. Deliberately separate from the
   greying: applyContestLock() runs on every refreshSpecialOp(), including the one that
   precedes the parking, so forcing from there would capture the forced values as the
   operator's own and make the restore a no-op - the same harvest-before-force trap that
   clear_DX_ fell into in the settings dialog. This is called only after parking. */
void MainWindow::forceContestControls ()
{
  if (ui->actionEnable_hound_mode->isChecked ()) ui->actionEnable_hound_mode->setChecked (false);
  if (!ui->AutoTxButton->isChecked ()) { ui->AutoTxButton->setChecked (true); on_AutoTxButton_clicked (true); }
  if (ui->actionSingleShot->isChecked ()) ui->actionSingleShot->setChecked (false);
  /* through the slot: it syncs skipGrid, sets m_skipTx1 and moves the Tx selection off
     Tx1. Forced unconditionally - testing ui->skipTx1->isEnabled() here looked like a
     guard against the protocol cases that forbid skipping Tx1, but applyContestLock()
     has already greyed the widget by this point, so it only ever read false and the
     force never ran. The real protocol case, two compound callsigns, still wins:
     on_dxCallEntry_textChanged() unchecks and disables it when one is entered, and that
     branch is deliberately not contest guarded. A non standard callsign cannot enter
     the contest at all. */
  if (!ui->skipTx1->isChecked ())
    { ui->skipTx1->setChecked (true); on_skipTx1_clicked (true); }
  /* RRR off: the contest wants RR73, which ends the QSO a cycle earlier. setChecked()
     is enough - these are stateChanged slots, not clicked, so they fire either way and
     keep rrr1CheckBox and m_rrr in step. */
  if (m_rrr) ui->rrrCheckBox->setChecked (false);
  /* CE3TSK: Max distance instead of best SNR OFF, and report priority off with it: the points
     tiers already rank callers by distance bracket, so among callers of equal value the only
     thing left to optimise is that the QSO completes - the strongest one. Both through
     setChecked(): the toggled slots own m_maxDistance / m_rprtPriority. */
  if (ui->actionMaxDistance->isChecked ()) ui->actionMaxDistance->setChecked (false);
  if (ui->actionReport_message_priority->isChecked ()) ui->actionReport_message_priority->setChecked (false);
  /* AutoFilter off and left off, not parked: it clamps decoding to +-60Hz of the RX
     frequency, which hides the stations calling us elsewhere in the passband - the ones
     a contest most wants. Through the slot, which owns m_autofilter and drops any
     filter already applied. */
  if (ui->actionAutoFilter->isChecked ()) ui->actionAutoFilter->setChecked (false);
}

void MainWindow::applyContestLock (bool locked)
{
  /* Hound through enableHoundAccess(), which owns both widgets and already folds in the
     contest rule, so this cannot re-enable it in a mode where it does not belong. */
  enableHoundAccess (m_mode == "FT8");
  ui->AutoTxButton->setEnabled (!locked);
  /* CE3TSK: AutoSeq, the call mode and priority CQ are deliberately NOT touched - the
     operator keeps whatever AutoSeq setting they run with, including its label and colour. */
  /* single shot is two widgets kept in sync by on_singleQSOButton_clicked() and
     on_actionSingleShot_toggled(); locking only the menu item leaves the button live. */
  ui->actionSingleShot->setEnabled (!locked);
  ui->singleQSOButton->setEnabled (!locked);

  /* skipTx1 and skipGrid are one setting in two widgets. Unlocking hands them to
     toggle_skipTx1(), which re-tests what legitimately forbids them - a non standard
     callsign, hound mode - rather than enabling them blindly. */
  if (locked) { ui->skipTx1->setEnabled (false); ui->skipGrid->setEnabled (false); }
  else toggle_skipTx1 ();

  /* CE3TSK: in WW Digi the grid is the exchange, so Tx1 (HISCALL MYCALL GRID) says exactly
     what Tx2 says and skip Tx1 is forced by forceContestControls() - the whole Tx1 row
     (button, text box, radio) is greyed so it cannot be picked by hand either. The radio
     is also owned by enableTab1TXRB() (AutoSeq greys all six), which folds this rule in. */
  ui->txb1->setEnabled (!locked);
  ui->tx1->setEnabled (!locked);
  ui->txrb1->setEnabled (!locked && !m_autoseq);

  /* RRR is two widgets as well, and FT4 has no RRR at all - commonActions() parks and
     disables it there through m_savedRRR. Unlocking must not override that. */
  ui->rrrCheckBox->setEnabled (!locked && m_mode != "FT4");
  ui->rrr1CheckBox->setEnabled (!locked && m_mode != "FT4");

  /* CE3TSK: Max distance and report priority are both forced off and greyed - either one
     would change the SNR tie-break the contest wants. */
  ui->actionMaxDistance->setEnabled (!locked);
  ui->actionReport_message_priority->setEnabled (!locked);

  /* AutoFilter: greyed while a contest runs. Priority CQ holds it down as well, so on
     unlock only give it back if that is not also asking for it. */
  ui->actionAutoFilter->setEnabled (!locked && !m_callPrioCQ);

  /* CE3TSK: the Contest lamp above the Tune button - greyed in normal operation, green with
     black text while a contest is selected. autoFillBackground is needed for a QLabel to
     paint a stylesheet background at all. */
  ui->labelContest->setAutoFillBackground (true);
  ui->labelContest->setEnabled (locked);
  if (locked)
    /* the pale green the status bar uses for its highlight lamps */
    ui->labelContest->setStyleSheet (QString ("QLabel{color: %1; background: %2; border: 1px solid %3; border-radius: 3px; padding: 1px 4px}")
                                     .arg (Radio::convert_dark ("#000000", m_useDarkStyle),
                                           Radio::convert_dark ("#99ff99", m_useDarkStyle),
                                           Radio::convert_dark ("#005400", m_useDarkStyle)));
  else
    /* at rest: the same grey box the preset lamp below it uses for its uncoloured cases */
    ui->labelContest->setStyleSheet ("QLabel{border: 1px solid #808080; border-radius: 3px; padding: 1px 4px}");

  /* FT8 and FT4 stay selectable; every other member of the mode group is greyed. WSPR is the
     seventh member of that same group, so it needs no case of its own. */
  ui->actionJT65->setEnabled (!locked);
  ui->actionJT9_JT65->setEnabled (!locked);
  ui->actionJT9->setEnabled (!locked);
  ui->actionT10->setEnabled (!locked);
  ui->actionWSPR_2->setEnabled (!locked);
}

/* CE3TSK: (re)open the contest's own log, or drop it when no contest is running. The
   filename carries the contest and its period, so enabling and disabling contest mode during
   a contest reopens the same file rather than moving any boundary. Country data is borrowed
   from m_logBook so cty.dat is not parsed twice. */
/* CE3TSK: bind the band selector and its validator to whichever frequency list is
   current. Configuration::frequencies() returns the contest list while a contest is
   selected; these are the only two consumers that capture the pointer rather than
   asking each time, so they are the only two that have to be redone on a change. */
void MainWindow::bindBandCombo ()
{
  /* CE3TSK: refreshSpecialOp() calls this on every settings accept, but setModel() resets
     the combo to its first row - so do nothing while the bound list is already the active
     one, and preserve the shown band across a genuine rebind. */
  if (m_bandValidator && ui->bandComboBox->model () == m_config.frequencies ()) return;
  QString const shown = ui->bandComboBox->currentText ();

  // Hook up working frequencies.
  ui->bandComboBox->setModel (m_config.frequencies ());
  ui->bandComboBox->setModelColumn (FrequencyList_v2::frequency_mhz_column);

  // combo box drop down width defaults to the line edit + decorator width,
  // here we change that to the column width size hint of the model column
  ui->bandComboBox->view ()->setMinimumWidth (ui->bandComboBox->view ()->sizeHintForColumn (FrequencyList_v2::frequency_mhz_column));

  /* the previous one is parented to this window, so without deleting it every contest
     toggle would leave another validator alive and still connected */
  if (m_bandValidator) { ui->bandComboBox->setValidator (nullptr); delete m_bandValidator; }

  // Enable live band combo box entry validation and action.
  /* the concrete type is needed for connect(); m_bandValidator keeps the base pointer purely
     so the previous one can be deleted, which avoids pulling the header into mainwindow.h */
  auto band_validator = new LiveFrequencyValidator {ui->bandComboBox
                                                    , m_config.bands ()
                                                    , m_config.frequencies ()
                                                    , &m_freqNominal
                                                    , this};
  ui->bandComboBox->setValidator (band_validator);
  m_bandValidator = band_validator;

  // Hook up signals.
  connect (band_validator, &LiveFrequencyValidator::valid, this, &MainWindow::band_changed);

  /* the list swapped in still carries whatever region and mode filter it last had -
     for a fresh contest list that is no filter at all, which would offer rows for every
     region. Re-apply the current filter the way switch_mode() does. */
  m_config.frequencies ()->filter (m_config.region (), Modes::value (m_mode));

  /* put the previous band text back; if it is not in the new list the validator will deal
     with it exactly as it deals with any typed frequency */
  if (!shown.isEmpty ()) ui->bandComboBox->setCurrentText (shown);
  scheduleBandButtons ();   // CE3TSK: the band buttons follow the list just bound
}

void MainWindow::refreshContestLog (bool reload)
{
  QString const want = contest_log_filename (m_config.special_op_id (),
                                             m_jtdxtime->currentDateTimeUtc2 ());
  /* reload is not optional and not defaulted. It must be true after m_logBook.init(), where
     the shared country data has only just been read: ADIF::load() resolves each QSO's country
     as it parses, so a contest log parsed earlier - at startup m_logBook has not been
     initialised at all - holds empty country hashes until it is parsed again. Everywhere else
     it is false, so pressing OK in the settings dialog does not re-read the file for nothing;
     refreshSpecialOp() runs on every accept. */
  bool const changed = (want != m_contestLogFile);
  m_contestLogFile = want;
  if (!want.isEmpty () && (reload || changed))
    {
      m_contestLog.init (m_config.callNotif () ? m_config.my_callsign () : "",
                         m_config.gridNotif () ? m_config.my_grid () : "",
                         m_config.timeFrom (), want, m_logBook.countryData ());
      if (m_config.write_decoded_debug ()) writeToALLTXT ("Contest log: " + want);
    }
  /* always, even when nothing was re-read: the station grid may have changed, and every
     QSO's points are measured from it */
  updateContestScore ();
}

/* CE3TSK: multipliers x points = score, as WW Digi computes it - the sum of QSO points
   across all bands multiplied by the sum of 2 character grid fields across all bands.

   Recomputed in full rather than accumulated. The contest log holds only contest QSOs, so a
   scan is about a thousand geodesics, a millisecond or two at QSO rate, and the display
   cannot drift from the file the way a running total would. The multiplier count is free:
   the field+band hash built for the highlighting is already the multiplier set. */
void MainWindow::updateContestScore ()
{
  contest_score_label->setVisible (m_wwDigi);
  if (!m_mode.startsWith ("WSPR")) countQSOs ();   /* CE3TSK: the counter follows the same log as the score */
  if (!m_wwDigi) { contest_score_label->setText (""); return; }
  /* CE3TSK: the same pale green the status bar already uses to highlight a label, through
     convert_dark so it follows the dark theme. Re-applied here rather than set once at
     construction so a theme change is picked up. */
  contest_score_label->setStyleSheet (QString ("QLabel{background: %1}")
                                      .arg (Radio::convert_dark ("#99ff99", m_useDarkStyle)));
  int points = 0;
  for (auto const& g : m_contestLog.gridList ())
    {
      /* a QSO with no grid gives -1 km and scores nothing */
      points += m_config.special_op_points (grid_distance_km (m_config.my_grid (), g, true));
    }
  int const mults = m_contestLog.fieldBandCount ();
  contest_score_label->setText (QString {"%1M x %2P = %3"}
                                .arg (mults).arg (points).arg (points * mults));
}

void MainWindow::refreshSpecialOp (bool initial)
{
  bool const was = m_wwDigi;
  m_wwDigi = m_config.wwDigi ();
  m_qsoHistory.wwdigi (m_wwDigi);
  m_qsoHistory2.wwdigi (m_wwDigi);
  refreshContestLog (false);   /* CE3TSK: follow the activity */
  /* CE3TSK: the title names the contest, so it is rebuilt whenever this runs rather than
     only on a transition - cheap, and it cannot drift out of step with the setting. */
  setWindowTitle (program_title ());
  /* CE3TSK: the greying runs on every call, not only on a transition, because a restart
     already in contest mode never sees one - the constructor's call is guarded by initial.
     It only enables and disables; the values themselves are not touched here, and must not
     be: this runs before the parking below, so forcing from here would capture the forced
     values as the operator's own. On the restart path nothing needs forcing anyway, the
     values were forced before the restart and persisted that way. */
  applyContestLock (m_wwDigi);
  /* CE3TSK: the band selector and its validator capture the frequency list pointer, and
     Configuration::frequencies() now hands out a different list once a contest is selected.
     Rebind on every call for the same reason the greying is done here: a restart already in
     contest mode never sees a transition. */
  bindBandCombo ();

  /* CE3TSK: act only on a real change of activity. This runs on every settings accept, and
     halting TX because an unrelated setting was edited would be worse than useless.
     initial guards the call from the constructor, where m_wwDigi starts false and a stored
     contest setting would look like a transition: clearDX() also forces RxFreq to TxFreq,
     which would override the RX frequency just restored from settings. */
  if (initial || was == m_wwDigi) return;

  /* The message set changes shape between a signal report and a grid exchange, so nothing
     already generated is safe to send, and a QSO in progress cannot continue across the
     change. clearDX() drops the DX call, clears Tx1..Tx5 and rebuilds Tx6 in the new
     format; blanking the fields by hand would leave the sequencer free to resume the QSO
     with messages of the other kind. */
  if(m_config.write_decoded_debug()) writeToALLTXT("Special operating activity changed, TX halted and messages cleared");
  onQsoAbandoned ();   /* CE3TSK: the QSO the sequencer hooks refer to is being abandoned */
  m_contestIgnore.clear (); /* CE3TSK: the contest's report-message skip list goes with it */
  /* CE3TSK: the directional CQ the contest calls with - "WW" for WW Digi, empty on the way
     back out. Set before clearDX() so the Tx6 it rebuilds already carries the new direction.
     Deliberately just the line edit: the textChanged slot syncs the second copy and m_cqdir,
     and nothing forces or greys it afterwards, so it can still be typed over. */
  ui->directionLineEdit->setText (m_config.special_op_cq_direction ());

  /* CE3TSK: park the main window controls on the way in and hand them back on the way out.
     Parked only on a real off to on transition - m_uiParkedValid guards a second enable from
     capturing the forced values and turning the restore into a no-op, exactly as
     specialOpSaved_ does for the settings dialog. */
  if (m_wwDigi)
    {
      if (!m_uiParkedValid)
        {
          m_uiParked.autoTx = ui->AutoTxButton->isChecked ();
          m_uiParked.skipTx1 = m_skipTx1;
          m_uiParked.rrr = m_rrr;
          m_uiParked.maxDistance = m_maxDistance;
          m_uiParked.rprtPriority = m_rprtPriority;
          /* the mode is parked only when it is not already one the contest allows, and the
             empty string then means "nothing to restore": leaving the contest keeps whichever
             of FT8 or FT4 is current. */
          m_uiParked.mode = (m_mode == "FT8" || m_mode == "FT4") ? QString {} : m_mode;
          m_uiParkedValid = true;
        }
      if (!m_uiParked.mode.isEmpty ()) on_actionFT8_triggered ();
      forceContestControls ();      /* only now, with the operator's own values safely parked */
      applyContestLock (true);
    }
  else if (m_uiParkedValid)
    {
      applyContestLock (false);     // settable again before we set them
      /* CE3TSK: the mode goes back FIRST, before any of the buttons. setAutoSeqButtonStyle()
         reads m_mode - AutoSeq off is grey in a non FT mode but light red in an FT one - and
         on_actionJT65_triggered() and its siblings do not refresh that style. Restoring the
         mode afterwards would leave the button coloured for the mode we were leaving. */
      if (!m_uiParked.mode.isEmpty ())
        {
          QString const back = m_uiParked.mode;
          if (back == "JT65") on_actionJT65_triggered ();
          else if (back == "JT9+JT65") on_actionJT9_JT65_triggered ();
          else if (back == "JT9") on_actionJT9_triggered ();
          else if (back == "T10") on_actionT10_triggered ();
          else if (back == "FT2") on_actionFT2_triggered ();   // CE3TSK
          else if (back.startsWith ("WSPR")) on_actionWSPR_2_triggered ();
          else on_actionFT8_triggered ();
        }
      if (ui->AutoTxButton->isChecked () != m_uiParked.autoTx)
        { ui->AutoTxButton->setChecked (m_uiParked.autoTx); on_AutoTxButton_clicked (m_uiParked.autoTx); }
      if (m_skipTx1 != m_uiParked.skipTx1)
        { ui->skipTx1->setChecked (m_uiParked.skipTx1); on_skipTx1_clicked (m_uiParked.skipTx1); }
      /* CE3TSK: the two are mutually exclusive and never both parked true; report priority
         first (it clears max distance if it goes on), then max distance to its parked value. */
      if (m_rprtPriority != m_uiParked.rprtPriority) ui->actionReport_message_priority->setChecked (m_uiParked.rprtPriority);
      if (m_maxDistance != m_uiParked.maxDistance) ui->actionMaxDistance->setChecked (m_uiParked.maxDistance);
      /* RRR: in FT4 the widget is legitimately unavailable, so hand the value to the stock
         m_savedRRR instead and let commonActions() give it back on leaving FT4. */
      /* Additive, never clearing. m_savedRRR means "owed a restore", and JTDX sets it true
         on entering FT4 with RRR on but never resets it. Assigning our parked value here
         would overwrite that with false in the sequence FT8 with RRR on, switch to FT4,
         enter the contest - where our park sees m_rrr already false because FT4 turned it
         off - then leave: the operator's RRR setting would be lost on returning to FT8. */
      if (m_mode == "FT4") { if (m_uiParked.rrr) m_savedRRR = true; }
      else if (m_rrr != m_uiParked.rrr) ui->rrrCheckBox->setChecked (m_uiParked.rrr);
      m_uiParkedValid = false;
    }

  clearDX (" cleared, special operating activity changed");
  if (m_enableTx || m_transmitting || m_btxok || g_iptt==1) haltTx("special operating activity changed ");
}
void MainWindow::on_actionShow_tooltips_main_window_toggled(bool checked) { m_showTooltips = checked; }
void MainWindow::on_actionColor_Tx_message_buttons_toggled(bool checked) { m_colorTxMsgButtons = checked; }
void MainWindow::on_actionCallsign_to_clipboard_toggled(bool checked) { m_callToClipboard = checked; }
void MainWindow::on_actionCrossband_160m_JA_toggled(bool checked) { m_crossbandOptionEnabled = checked; }
void MainWindow::on_actionCrossband_160m_HL_toggled(bool checked) { m_crossbandHLOptionEnabled = checked; }

void MainWindow::on_actionShow_messages_decoded_from_harmonics_toggled(bool checked)
{
  if(checked) { dec_data.params.showharmonics=1; m_showHarmonics=true; }
  else { dec_data.params.showharmonics=0; m_showHarmonics=false; }
}

void MainWindow::on_actionMyCallRXFwindow_toggled(bool checked) { m_showMyCallMsgRxWindow=checked; }
void MainWindow::on_actionWantedCallRXFwindow_toggled(bool checked) { m_showWantedCallRxWindow=checked; }
void MainWindow::on_actionFT8SensMin_toggled(bool checked) { if(checked) m_ft8Sensitivity=0; }
void MainWindow::on_actionlowFT8thresholds_toggled(bool checked) { if(checked) m_ft8Sensitivity=1; }
// CE3TSK: the Presets (green), RX (blue) and TX background (red) submenus in their own
// colours (nested submenus inherit them), so it is visible which one is open, and the same
// colour as a swatch on each one's entry in the FT8 decoding menu - a menu cannot colour
// one item differently from the next, an icon can; redone whenever the dark style changes
// CE3TSK: the presets DECODE_RECIPE_PLAN.md section 13 recommends, marked in the submenu - bold, a
// coloured dot and the tier in front of the name (measured on one hour of air against the classical
// 9-cycle recipe: +3 % for 0.4 s, +8 % for 1.1 s, +16 % for 5.1 s, +18 % for 12.2-13.4 s per period;
// the three full pipelines end within 20 messages of each other: pipeline ensemble "best results"
// (blue), run "most results" (purple), full "max effort" (red: every member sample, the longest
// background); Max decodes "best value" orange, the light preset
// "best medium effort" green, Max efficiency the least CPU and power - "best power", grey)
// CE3TSK: a 16 x 16 coloured dot icon - the presets submenu's own entry and the marked presets
static QIcon menu_dot(QColor const& colour)
{
  QPixmap dot(16, 16); dot.fill(Qt::transparent);
  QPainter q(&dot); q.setRenderHint(QPainter::Antialiasing); q.setBrush(colour); q.setPen(Qt::NoPen);
  q.drawEllipse(3, 3, 10, 10);
  return QIcon(dot);
}
/* CE3TSK: a preset entry's name for the menu, its whole description for the tooltip.

   Each preset action carries its full recipe as its label - "pipeline max decodes light:
   maximum decodes at reply time, then a ~6 s TX background - the plain 6-cycle pass, 2
   members, the residual pass" - and with the tier prefix in front the widest FT8 entry
   measured 1534 px, wider than a 1366 px laptop screen. Breaking it over several lines is not
   possible: Qt paints a menu entry with Qt::TextSingleLine, and a QProxyStyle that paints the
   lines itself is bypassed entirely as soon as a stylesheet rule matches QMenu - which the
   dark style always does (darkstyle.qss has QMenu::item rules) and setDecodeMenuColours does
   even in the light one. Measured: 0 of 6 draw calls reach such a style under either sheet.

   So the entry keeps the name and hands the detail to the tooltip, which wrap_tooltip folds
   over several lines and QMenu shows because the two preset menus set setToolTipsVisible.
   Widest entry: 625 px.

   The split is on the TRANSLATED text, so no source string changes and no catalogue entry is
   orphaned. Most of these descriptions are written "name: detail", so the colon is the cut;
   FT4's recommended preset is the one with no colon in it ("background 6 with deep OSD, the
   alternate pass and ...") and is cut at its first comma instead, which reads as the parallel
   of "background 3" above it. Whichever separator comes first wins.

   The full-width forms are in the list because zh_CN, zh_HK and ja_JP punctuate with them and
   with nothing else: without them all ten Chinese entries kept their whole text and the widest
   measured 1769 px, worse than the English it was meant to fix. The ASCII pair needs its
   trailing space so that a number written 1,024 is not mistaken for the end of a name; the
   full-width characters carry their own spacing and take none. A translation with no separator
   at all simply keeps its whole text as the label, exactly as before this change. */
static QString preset_menu_entry (QAction* action, QString const& tier)
{
  QString const description = action->text ();
  action->setToolTip (wrap_tooltip (description));
  int cut = -1;
  for (auto const* separator : {u8": ", u8", ", u8"\uFF1A", u8"\uFF0C", u8"\u3001"})
    {
      int const at = description.indexOf (QString::fromUtf8 (separator));
      if (at > 0 && (cut < 0 || at < cut)) cut = at;
    }
  QString const name = cut > 0 ? description.left (cut) : description;
  /* two of the FT4 tiers ARE the preset's name - "most at reply time (+7.9 %, 0.55 s) - most at
     reply time" says it twice, so there the tier alone is the entry. Both halves are translated,
     so this holds in every language whose catalogue words them the same - and where a translator
     chose different words the two do belong side by side, which is what happens. */
  return tier.startsWith (name, Qt::CaseInsensitive) ? tier : QString ("%1 - %2").arg (tier, name);
}
void MainWindow::markRecommendedPresets()
{
  /* CE3TSK: the tier NAME is translated, the measurement beside it is not - "+11.3 %, 0.4 s" is
     data, it reads the same in every language, and a translator who edits it introduces a
     mistake rather than a translation. QT_TR_NOOP marks the names for the catalogue; the lookup
     is the tr() in the loop, in the MainWindow context. */
  struct { QAction* action; char const* tier; char const* cost; DecodePreset preset; } const picks[] = {
    {ui->actionFT8PresetMaxEfficiency, QT_TR_NOOP("best power"), "+11.3 %, 0.4 s", DecodePreset::MaxEfficiency},
    {ui->actionFT8PresetMaxDecodes, QT_TR_NOOP("best value"), "+16.6 %, 1.1 s", DecodePreset::MaxDecodes},
    {ui->actionFT8PresetPipelineLight, QT_TR_NOOP("best medium effort"), "+25.1 %, 5.3 s", DecodePreset::PipelineMaxDecodesLight},
    {ui->actionFT8PresetPipeline, QT_TR_NOOP("best results"), "+27.2 %, 12.7 s", DecodePreset::PipelineEnsemble},
    {ui->actionFT8PresetPipelineFull, QT_TR_NOOP("max effort"), "+27.5 %, 14.0 s", DecodePreset::PipelineEnsembleFull},
    {ui->actionFT8PresetPipelineRun, QT_TR_NOOP("most results"), "+28.0 %, 13.3 s", DecodePreset::PipelineRun}};
  for (auto const& p : picks) {
    QFont f = p.action->font(); f.setBold(true); p.action->setFont(f);
    p.action->setIcon(menu_dot(QColor(preset_colour(p.preset))));   // P13: the lamp's colour table
    p.action->setText(preset_menu_entry(p.action, QString("%1 (%2)").arg(tr(p.tier), p.cost)));
  }
  // the recommended one: underlined as well, and named so; the best-results pipeline underlined too
  for (auto a : {ui->actionFT8PresetPipelineLight, ui->actionFT8PresetPipeline}) { QFont f = a->font(); f.setUnderline(true); a->setFont(f); }
  ui->actionFT8PresetPipelineLight->setText(tr("recommended: %1").arg(ui->actionFT8PresetPipelineLight->text()));
  ui->menuFT8_preset->setToolTipsVisible(true);   // the recipe preset_menu_entry moved off the entry
}
void MainWindow::setDecodeMenuColours()
{
  struct { QMenu* menu; char const* light; char const* dark; } const menus[] = {
    {ui->menuFT8_preset, "#dff3df", "#1e3a1e"}, {ui->menuFT8_RX, "#e4eeff", "#1b2a44"}, {ui->menuFT8_TX, "#ffe6dc", "#44221b"},
    {ui->menuFT4_preset, "#dff3df", "#1e3a1e"}, {ui->menuFT4_RX, "#e4eeff", "#1b2a44"}, {ui->menuFT4_TX, "#ffe6dc", "#44221b"},   // CE3TSK item 69: FT4's three the same
    {ui->menuFT2_preset, "#dff3df", "#1e3a1e"}, {ui->menuFT2_RX, "#e4eeff", "#1b2a44"}, {ui->menuFT2_TX, "#ffe6dc", "#44221b"}};   // CE3TSK step 5: and FT2's, the same three colours
  for (auto const& m : menus) {
    QColor const colour {m_useDarkStyle ? m.dark : m.light};
    m.menu->setStyleSheet(QString("QMenu { background-color: %1; }").arg(colour.name()));
    QPixmap swatch(40, 16); swatch.fill(colour);
    QPainter p(&swatch); p.setPen(m_useDarkStyle ? Qt::lightGray : Qt::darkGray); p.drawRect(0, 0, 39, 15);
    m.menu->menuAction()->setIcon(QIcon(swatch));
  }
  // the presets submenu's entry carries a green dot instead of its swatch - the green of the
  // recommended preset's mark, so the entry says "presets" the way the marks do
  ui->menuFT8_preset->menuAction()->setIcon(menu_dot(QColor(preset_colour(DecodePreset::PipelineMaxDecodesLight))));
  ui->menuFT4_preset->menuAction()->setIcon(menu_dot(QColor(ft4_preset_colour(FT4Preset::Recommended))));   // item 69
  ui->menuFT2_preset->menuAction()->setIcon(menu_dot(QColor(ft2_preset_colour(FT2Preset::Recommended))));   // CE3TSK step 5
}
// CE3TSK: decode bandwidth - the radio entries carry the table index in their data
void MainWindow::setDecodeBandwidthAction()
{
  for (auto a : ui->menuFT8_bandwidth->actions()) if(a->data().toInt()==m_decodeBandwidth) a->setChecked(true);
}
void MainWindow::on_actionFT8DeepOSD_toggled(bool checked) { m_ft8DeepOSD=checked; }   // CE3TSK
void MainWindow::on_actionFT8TwoSlicings_toggled(bool checked) { m_ft8TwoSlicings=checked; }   // CE3TSK
void MainWindow::on_actionFT8AltPass_toggled(bool checked) { m_ft8AltPass=checked; }   // CE3TSK
void MainWindow::on_actionFT4AltPass_toggled(bool checked) { m_ft4AltPass=checked; }   // CE3TSK: FT4 expert
void MainWindow::on_actionFT4DeepOSD_toggled(bool checked) { m_ft4DeepOSD=checked; }   // CE3TSK item 58

/* CE3TSK step 5: FT2's tiers. The shape is FT4's - a preset only sets the ordinary controls, and
   the menu derives the active tier back from them when it opens - but the values are FT2's own
   (ft2preset.h), because a 3.75 s period answers on a 0.58 s deadline. */
void MainWindow::applyFT2Preset (FT2Preset p)
{
  m_ft2Recipe = ft2_preset_recipe (p);
  m_ft2BgMargin = (p == FT2Preset::MaxEffort) ? 3 : FT2_BG_MARGIN_DEFAULT;   // as FT4's max effort narrows its margin
  ui->actionFT2BgEnabled->setChecked(m_ft2Recipe.bgon);
  setFT2EnsembleActions();
  refreshFT2Preset();
}

void MainWindow::setFT2EnsembleActions()
{
  QAction* const rx[] = {ui->actionFT2EnsembleOff, ui->actionFT2EnsembleAuto, ui->actionFT2EnsembleBudget};
  int const rxwant = m_ft2Recipe.members==FT4_ENSEMBLE_AUTO ? 1 : m_ft2Recipe.members==FT4_ENSEMBLE_BUDGET ? 2 : 0;
  for (int i=0; i<3; ++i) rx[i]->setChecked(i==rxwant);
  QAction* const bg[] = {ui->actionFT2BgEnsembleOff, ui->actionFT2BgEnsemble3, ui->actionFT2BgEnsemble6, ui->actionFT2BgEnsembleAuto};
  int const bgwant = m_ft2Recipe.bg==FT4_ENSEMBLE_AUTO ? 3 : m_ft2Recipe.bg==6 ? 2 : m_ft2Recipe.bg==3 ? 1 : 0;
  for (int i=0; i<4; ++i) bg[i]->setChecked(i==bgwant);
}

void MainWindow::refreshFT2Preset()
{
  QAction* const a[] = {ui->actionFT2PresetFast, ui->actionFT2PresetDefault,
                        ui->actionFT2PresetRecommended, ui->actionFT2PresetMaxEffort};
  auto const p = ft2_preset_of (m_ft2Recipe, ft4Threads());
  for (int i=0; i<4; ++i) a[i]->setChecked(false);
  if (p != FT2Preset::Custom) a[static_cast<int>(p)]->setChecked(true);
  setFT2EnsembleActions();
  refreshDecodePreset();   // CE3TSK: the lamp, exactly as refreshFT4Preset ends - without this the
                           // tier changed and the lamp went on showing the last one
}

void MainWindow::on_actionFT2PresetFast_triggered() { applyFT2Preset (FT2Preset::Fast); }
void MainWindow::on_actionFT2PresetDefault_triggered() { applyFT2Preset (FT2Preset::Default); }
void MainWindow::on_actionFT2PresetRecommended_triggered() { applyFT2Preset (FT2Preset::Recommended); }
void MainWindow::on_actionFT2PresetMaxEffort_triggered() { applyFT2Preset (FT2Preset::MaxEffort); }
void MainWindow::on_actionFT2BgEnabled_toggled(bool checked) { m_ft2Recipe.bgon=checked; }
void MainWindow::on_actionFT2EnsembleOff_triggered() { m_ft2Recipe.members=0; }
void MainWindow::on_actionFT2EnsembleAuto_triggered() { m_ft2Recipe.members=FT4_ENSEMBLE_AUTO; }
void MainWindow::on_actionFT2EnsembleBudget_triggered() { m_ft2Recipe.members=FT4_ENSEMBLE_BUDGET; }
void MainWindow::on_actionFT2BgEnsembleOff_triggered() { m_ft2Recipe.bg=0; }
void MainWindow::on_actionFT2BgEnsemble3_triggered() { m_ft2Recipe.bg=3; }
void MainWindow::on_actionFT2BgEnsemble6_triggered() { m_ft2Recipe.bg=6; }
void MainWindow::on_actionFT2BgEnsembleAuto_triggered() { m_ft2Recipe.bg=FT4_ENSEMBLE_AUTO; }

// CE3TSK: FT4 presets. A preset only sets the ordinary controls - effort, alternate pass, the
// RX member count, deep OSD and the TX background members (decodepreset.h has the recipes) -
// and the radio state is derived back from those controls when the menu opens, exactly as the
// FT8 preset menu behaves.
void MainWindow::applyFT4Preset (FT4Preset p)
{
  auto const r = ft4_preset_recipe (p);
  m_nFT4depth = r.depth;
  m_ft4AltPass = r.alt;
  m_ft4Ensemble = r.members;
  m_ft4DeepOSD = r.deeposd;      // item 67
  m_ft4BgEnsemble = r.bg;
  m_ft4BgDepth = r.bgdepth; m_ft4BgDeepOSD = r.bgdeeposd; m_ft4BgAltPass = r.bgalt; m_ft4BgTwoSlicings = r.bgtwopass;   // item 69
  ui->actionFT4BgDeepOSD->setChecked(m_ft4BgDeepOSD); ui->actionFT4BgAltPass->setChecked(m_ft4BgAltPass); ui->actionFT4BgTwoSlicings->setChecked(m_ft4BgTwoSlicings);
  setFT4BgEffortAction();
  if(m_nFT4depth==3) ui->actionFT4deep->setChecked(true);
  else if(m_nFT4depth==2) ui->actionFT4medium->setChecked(true);
  else ui->actionFT4fast->setChecked(true);
  ui->actionFT4AltPass->setChecked(m_ft4AltPass);
  ui->actionFT4DeepOSD->setChecked(m_ft4DeepOSD);
  setFT4EnsembleAction();
  setFT4BgEnsembleAction();
  m_ft4BgResidual = r.bgresidual; ui->actionFT4BgResidual->setChecked(m_ft4BgResidual);   // item 72
  m_ft4Sens = r.sens; setFT4SensAction();
  m_ft4BgSens = r.bgsens; setFT4BgSensAction();   // item 73
  m_ft4TwoSlicings = r.twopass; ui->actionFT4TwoSlicings->setChecked(m_ft4TwoSlicings);   // item 74: every control the recipe names, as FT8's applyDecodePreset does
  m_ft4BgEnabled = r.bgon; ui->actionFT4BgEnabled->setChecked(m_ft4BgEnabled);
  m_ft4BgMargin = (p == FT4Preset::MaxEffort) ? 5 : 10;   // item 80: max effort's background runs until 0.5 s before the window ends; not a recipe field - it changes when the phase stops, not what it decodes
  m_ft4RXfSens = r.rxf; m_ft4BgRXfSens = r.bgrxf; setFT4RXfActions();   // item 75
  refreshFT4Preset();
}

/* CE3TSK: the FT4 controls as a recipe, in one place. The same sixteen-field positional
   initialiser stood here and in refreshDecodePreset's lamp block, and every knob added by
   items 69, 72, 73, 74 and 75 had to be appended to both in the same position - two
   same-typed fields swapped in one copy compiles cleanly and leaves the menu radio and the
   preset lamp disagreeing about which preset is active. */
FT4Recipe MainWindow::currentFT4Recipe () const
{
  return FT4Recipe {m_nFT4depth, m_ft4AltPass, m_ft4Ensemble, m_ft4DeepOSD, m_ft4BgEnsemble,
                    m_ft4BgDepth, m_ft4BgDeepOSD, m_ft4BgAltPass, m_ft4BgTwoSlicings, m_ft4Sens,
                    m_ft4BgResidual, m_ft4BgSens, m_ft4TwoSlicings, m_ft4BgEnabled, m_ft4RXfSens,
                    m_ft4BgRXfSens};
}

void MainWindow::refreshFT4Preset()
{
  QAction* a[] = {ui->actionFT4PresetFast, ui->actionFT4PresetDefault, ui->actionFT4PresetBestPower,
                  ui->actionFT4PresetRecommended, ui->actionFT4PresetMaxDecodes, ui->actionFT4PresetMaxEffort};
  auto const p = ft4_preset_of (currentFT4Recipe (), ft4Threads());
  for (auto x : a) x->setChecked(false);
  switch (p)
    {
    case FT4Preset::Fast:        a[0]->setChecked(true); break;
    case FT4Preset::Default:     a[1]->setChecked(true); break;
    case FT4Preset::BestPower:   a[2]->setChecked(true); break;
    case FT4Preset::Recommended: a[3]->setChecked(true); break;
    case FT4Preset::MaxDecodes:  a[4]->setChecked(true); break;
    case FT4Preset::MaxEffort:   a[5]->setChecked(true); break;
    default: break;   // custom: no entry checked, as the FT8 menu shows it
    }
  refreshDecodePreset();   // item 73: the lamp
}
// CE3TSK item 67: the FT4 tiers marked as FT8's are (markRecommendedPresets): bold, the tier's dot
// and its measured gain and reply-time cost in front of the name; the recommended one underlined
void MainWindow::markFT4Presets()
{
  struct { QAction* action; char const* tier; char const* cost; FT4Preset preset; } const picks[] = {
    {ui->actionFT4PresetBestPower, QT_TR_NOOP("best power"), "+4.5 %, 0.12 s", FT4Preset::BestPower},
    {ui->actionFT4PresetRecommended, QT_TR_NOOP("best value"), "+8.0 %, 0.12 s", FT4Preset::Recommended},
    {ui->actionFT4PresetMaxDecodes, QT_TR_NOOP("most at reply time"), "+7.9 %, 0.55 s", FT4Preset::MaxDecodes},
    {ui->actionFT4PresetMaxEffort, QT_TR_NOOP("max effort"), "+8.4 %, 0.69 s", FT4Preset::MaxEffort}};
  for (auto const& p : picks) {
    QFont f = p.action->font(); f.setBold(true); p.action->setFont(f);
    p.action->setIcon(menu_dot(QColor(ft4_preset_colour(p.preset))));
    p.action->setText(preset_menu_entry(p.action, QString("%1 (%2)").arg(tr(p.tier), p.cost)));
  }
  QFont f = ui->actionFT4PresetRecommended->font(); f.setUnderline(true); ui->actionFT4PresetRecommended->setFont(f);
  ui->actionFT4PresetRecommended->setText(tr("recommended: %1").arg(ui->actionFT4PresetRecommended->text()));
  ui->menuFT4_preset->setToolTipsVisible(true);   // as FT8's
  ui->menuFT4_preset->menuAction()->setIcon(menu_dot(QColor(ft4_preset_colour(FT4Preset::Recommended))));
}

/* CE3TSK step 5: FT2's preset menu marked as FT4's is (markFT4Presets) - the same bold text, the
   same coloured dot from the mode's own colour table, the same "tier (cost)" entry with the recipe
   moved into the tooltip, the same underlined "recommended:" on the tier that carries the mark, and
   the same dot on the submenu itself. The costs are FT2's own measurements (ft2preset.h), and they
   are quoted on the CROWDED band because that is where FT2's tiers differ: on a sparse band every
   tier is within half a percent of the default. */
void MainWindow::markFT2Presets()
{
  struct { QAction* action; char const* tier; char const* cost; FT2Preset preset; } const picks[] = {
    {ui->actionFT2PresetRecommended, QT_TR_NOOP("best value"), "+7.4 % crowded, 0.10 s", FT2Preset::Recommended},
    {ui->actionFT2PresetMaxEffort, QT_TR_NOOP("max effort"), "+12.5 % crowded, 0.56 s", FT2Preset::MaxEffort}};
  for (auto const& p : picks) {
    QFont f = p.action->font(); f.setBold(true); p.action->setFont(f);
    p.action->setIcon(menu_dot(QColor(ft2_preset_colour(p.preset))));
    p.action->setText(preset_menu_entry(p.action, QString("%1 (%2)").arg(tr(p.tier), p.cost)));
  }
  QFont f = ui->actionFT2PresetRecommended->font(); f.setUnderline(true); ui->actionFT2PresetRecommended->setFont(f);
  ui->actionFT2PresetRecommended->setText(tr("recommended: %1").arg(ui->actionFT2PresetRecommended->text()));
  ui->menuFT2_preset->setToolTipsVisible(true);
  ui->menuFT2_preset->menuAction()->setIcon(menu_dot(QColor(ft2_preset_colour(FT2Preset::Recommended))));
}

void MainWindow::on_actionFT4PresetFast_triggered() { applyFT4Preset (FT4Preset::Fast); }
void MainWindow::on_actionFT4PresetDefault_triggered() { applyFT4Preset (FT4Preset::Default); }
void MainWindow::on_actionFT4PresetBestPower_triggered() { applyFT4Preset (FT4Preset::BestPower); }
void MainWindow::on_actionFT4PresetRecommended_triggered() { applyFT4Preset (FT4Preset::Recommended); }
void MainWindow::on_actionFT4PresetMaxDecodes_triggered() { applyFT4Preset (FT4Preset::MaxDecodes); }
void MainWindow::on_actionFT4PresetMaxEffort_triggered() { applyFT4Preset (FT4Preset::MaxEffort); }

// CE3TSK: FT4 ensemble members - the same radio shape as the FT8 ensemble effort menu
void MainWindow::on_actionFT4TwoSlicings_toggled(bool checked) { m_ft4TwoSlicings=checked; }
// CE3TSK item 69: the TX background submenu's own controls
void MainWindow::on_actionFT4BgAltPass_toggled(bool checked) { m_ft4BgAltPass=checked; }
void MainWindow::on_actionFT4BgDeepOSD_toggled(bool checked) { m_ft4BgDeepOSD=checked; }
void MainWindow::on_actionFT4BgTwoSlicings_toggled(bool checked) { m_ft4BgTwoSlicings=checked; }
void MainWindow::on_actionFT4BgFast_triggered() { m_ft4BgDepth=1; ui->actionFT4BgFast->setChecked(true); }
void MainWindow::on_actionFT4BgMedium_triggered() { m_ft4BgDepth=2; ui->actionFT4BgMedium->setChecked(true); }
void MainWindow::on_actionFT4BgDeep_triggered() { m_ft4BgDepth=3; ui->actionFT4BgDeep->setChecked(true); }
void MainWindow::on_actionFT4BgResidual_toggled(bool checked) { m_ft4BgResidual=checked; }   // CE3TSK item 72
void MainWindow::on_actionFT4BgEnabled_toggled(bool checked) { m_ft4BgEnabled=checked; }   // CE3TSK item 74
// CE3TSK item 75: QSO RX frequency sensitivity, FT8's three levels, per phase
void MainWindow::on_actionFT4RXfLow_triggered() { m_ft4RXfSens=1; setFT4RXfActions(); }
void MainWindow::on_actionFT4RXfMedium_triggered() { m_ft4RXfSens=2; setFT4RXfActions(); }
void MainWindow::on_actionFT4RXfHigh_triggered() { m_ft4RXfSens=3; setFT4RXfActions(); }
void MainWindow::on_actionFT4BgRXfLow_triggered() { m_ft4BgRXfSens=1; setFT4RXfActions(); }
void MainWindow::on_actionFT4BgRXfMedium_triggered() { m_ft4BgRXfSens=2; setFT4RXfActions(); }
void MainWindow::on_actionFT4BgRXfHigh_triggered() { m_ft4BgRXfSens=3; setFT4RXfActions(); }
void MainWindow::setFT4RXfActions()
{
  QAction* rx[] = {ui->actionFT4RXfLow, ui->actionFT4RXfMedium, ui->actionFT4RXfHigh};
  QAction* bg[] = {ui->actionFT4BgRXfLow, ui->actionFT4BgRXfMedium, ui->actionFT4BgRXfHigh};
  if(m_ft4RXfSens>=1 && m_ft4RXfSens<=3) rx[m_ft4RXfSens-1]->setChecked(true);
  if(m_ft4BgRXfSens>=1 && m_ft4BgRXfSens<=3) bg[m_ft4BgRXfSens-1]->setChecked(true);
}
void MainWindow::on_actionFT4SensStd_triggered() { m_ft4Sens=0; ui->actionFT4SensStd->setChecked(true); }
void MainWindow::on_actionFT4SensLow_triggered() { m_ft4Sens=1; ui->actionFT4SensLow->setChecked(true); }
void MainWindow::setFT4SensAction() { if(m_ft4Sens==1) ui->actionFT4SensLow->setChecked(true); else ui->actionFT4SensStd->setChecked(true); }
// CE3TSK item 73: the background's own sensitivity, the auto member entries, the thread count the ladders use
void MainWindow::on_actionFT4BgSensStd_triggered() { m_ft4BgSens=0; ui->actionFT4BgSensStd->setChecked(true); }
void MainWindow::on_actionFT4BgSensLow_triggered() { m_ft4BgSens=1; ui->actionFT4BgSensLow->setChecked(true); }
void MainWindow::setFT4BgSensAction() { if(m_ft4BgSens==1) ui->actionFT4BgSensLow->setChecked(true); else ui->actionFT4BgSensStd->setChecked(true); }
void MainWindow::on_actionFT4EnsembleAuto_triggered() { m_ft4Ensemble=FT4_ENSEMBLE_AUTO; }
void MainWindow::on_actionFT4EnsembleBudget_triggered() { m_ft4Ensemble=FT4_ENSEMBLE_BUDGET; }   // CE3TSK item 80
void MainWindow::on_actionFT4BgEnsembleAuto_triggered() { m_ft4BgEnsemble=FT4_ENSEMBLE_AUTO; }
int MainWindow::ft4Threads() const { return effective_ft8_threads(m_ft8threads, QThread::idealThreadCount()); }
void MainWindow::setFT4BgEffortAction()
{
  if(m_ft4BgDepth==1) ui->actionFT4BgFast->setChecked(true);
  else if(m_ft4BgDepth==2) ui->actionFT4BgMedium->setChecked(true);
  else ui->actionFT4BgDeep->setChecked(true);
}
void MainWindow::on_actionFT4EnsembleOff_triggered() { m_ft4Ensemble=0; }
void MainWindow::on_actionFT4Ensemble1_triggered() { m_ft4Ensemble=1; }
void MainWindow::on_actionFT4Ensemble2_triggered() { m_ft4Ensemble=2; }
void MainWindow::on_actionFT4Ensemble3_triggered() { m_ft4Ensemble=3; }
void MainWindow::on_actionFT4Ensemble4_triggered() { m_ft4Ensemble=4; }
void MainWindow::on_actionFT4Ensemble5_triggered() { m_ft4Ensemble=5; }
void MainWindow::on_actionFT4Ensemble6_triggered() { m_ft4Ensemble=6; }
void MainWindow::on_actionFT4BgEnsembleOff_triggered() { m_ft4BgEnsemble=0; }   // CE3TSK item 59
void MainWindow::on_actionFT4BgEnsemble1_triggered() { m_ft4BgEnsemble=1; }
void MainWindow::on_actionFT4BgEnsemble2_triggered() { m_ft4BgEnsemble=2; }
void MainWindow::on_actionFT4BgEnsemble3_triggered() { m_ft4BgEnsemble=3; }
void MainWindow::on_actionFT4BgEnsemble4_triggered() { m_ft4BgEnsemble=4; }
void MainWindow::on_actionFT4BgEnsemble5_triggered() { m_ft4BgEnsemble=5; }
void MainWindow::on_actionFT4BgEnsemble6_triggered() { m_ft4BgEnsemble=6; }
void MainWindow::setFT4BgEnsembleAction()
{
  QAction* levels[] = {ui->actionFT4BgEnsembleOff, ui->actionFT4BgEnsemble1, ui->actionFT4BgEnsemble2,
                       ui->actionFT4BgEnsemble3, ui->actionFT4BgEnsemble4, ui->actionFT4BgEnsemble5,
                       ui->actionFT4BgEnsemble6};
  if(m_ft4BgEnsemble>=0 && m_ft4BgEnsemble<=6) levels[m_ft4BgEnsemble]->setChecked(true);
  else if(m_ft4BgEnsemble==FT4_ENSEMBLE_AUTO) ui->actionFT4BgEnsembleAuto->setChecked(true);   // item 73
}
void MainWindow::setFT4EnsembleAction()
{
  QAction* levels[] = {ui->actionFT4EnsembleOff, ui->actionFT4Ensemble1, ui->actionFT4Ensemble2,
                       ui->actionFT4Ensemble3, ui->actionFT4Ensemble4, ui->actionFT4Ensemble5,
                       ui->actionFT4Ensemble6};
  if(valid_ft4_ensemble(m_ft4Ensemble)) levels[m_ft4Ensemble]->setChecked(true);
  else if(m_ft4Ensemble==FT4_ENSEMBLE_AUTO) ui->actionFT4EnsembleAuto->setChecked(true);   // item 73
  else if(m_ft4Ensemble==FT4_ENSEMBLE_BUDGET) ui->actionFT4EnsembleBudget->setChecked(true);   // item 80
}
// CE3TSK: ensemble effort - one radio entry per level; the checked entry follows the value
void MainWindow::on_actionFT8EnsembleOff_triggered() { m_ft8EnsembleEffort=0; setEnsembleEffortAction(); }
void MainWindow::on_actionFT8EnsembleAuto_triggered() { m_ft8EnsembleEffort=-1; setEnsembleEffortAction(); }
void MainWindow::on_actionFT8EnsembleBudget_triggered() { m_ft8EnsembleEffort=ENSEMBLE_BUDGET; setEnsembleEffortAction(); }   // CE3TSK P8
void MainWindow::on_actionFT8Ensemble1_triggered() { m_ft8EnsembleEffort=1; setEnsembleEffortAction(); }
void MainWindow::on_actionFT8Ensemble2_triggered() { m_ft8EnsembleEffort=2; setEnsembleEffortAction(); }
void MainWindow::on_actionFT8Ensemble3_triggered() { m_ft8EnsembleEffort=3; setEnsembleEffortAction(); }
void MainWindow::on_actionFT8Ensemble4_triggered() { m_ft8EnsembleEffort=4; setEnsembleEffortAction(); }
void MainWindow::on_actionFT8Ensemble5_triggered() { m_ft8EnsembleEffort=5; setEnsembleEffortAction(); }
// CE3TSK: TX background (pipeline ensemble) - its own recipe, one slot per control; the checked
// entries follow the values
void MainWindow::on_actionFT8BgOff_triggered() { m_bgEnsembleEffort=0; setBackgroundActions(); }
void MainWindow::on_actionFT8BgAuto_triggered() { m_bgEnsembleEffort=ENSEMBLE_AUTO; setBackgroundActions(); }
void MainWindow::on_actionFT8Bg1_triggered() { m_bgEnsembleEffort=1; setBackgroundActions(); }
void MainWindow::on_actionFT8Bg2_triggered() { m_bgEnsembleEffort=2; setBackgroundActions(); }
void MainWindow::on_actionFT8Bg3_triggered() { m_bgEnsembleEffort=3; setBackgroundActions(); }
void MainWindow::on_actionFT8Bg4_triggered() { m_bgEnsembleEffort=4; setBackgroundActions(); }
void MainWindow::on_actionFT8Bg5_triggered() { m_bgEnsembleEffort=5; setBackgroundActions(); }
void MainWindow::on_actionFT8BgEnabled_toggled(bool checked) { m_bgEnabled=checked; }
void MainWindow::on_actionFT8BgSWL_toggled(bool checked) { m_bgSWL=checked; }
void MainWindow::on_actionBgCycles3_triggered() { m_nBgCycles=3; setBackgroundActions(); }
void MainWindow::on_actionBgCycles4_triggered() { m_nBgCycles=4; setBackgroundActions(); }
void MainWindow::on_actionBgCycles5_triggered() { m_nBgCycles=5; setBackgroundActions(); }
void MainWindow::on_actionBgCycles6_triggered() { m_nBgCycles=6; setBackgroundActions(); }
void MainWindow::on_actionBgCycles7_triggered() { m_nBgCycles=7; setBackgroundActions(); }
void MainWindow::on_actionBgCycles8_triggered() { m_nBgCycles=8; setBackgroundActions(); }
void MainWindow::on_actionBgCycles9_triggered() { m_nBgCycles=9; setBackgroundActions(); }
void MainWindow::on_actionBgSWLcycles3_triggered() { m_nBgSWLCycles=3; setBackgroundActions(); }
void MainWindow::on_actionBgSWLcycles4_triggered() { m_nBgSWLCycles=4; setBackgroundActions(); }
void MainWindow::on_actionBgSWLcycles5_triggered() { m_nBgSWLCycles=5; setBackgroundActions(); }
void MainWindow::on_actionBgSWLcycles6_triggered() { m_nBgSWLCycles=6; setBackgroundActions(); }
void MainWindow::on_actionBgSWLcycles7_triggered() { m_nBgSWLCycles=7; setBackgroundActions(); }
void MainWindow::on_actionBgSWLcycles8_triggered() { m_nBgSWLCycles=8; setBackgroundActions(); }
void MainWindow::on_actionBgSWLcycles9_triggered() { m_nBgSWLCycles=9; setBackgroundActions(); }
void MainWindow::on_actionBgRXfLow_triggered() { m_bgRXfSens=1; setBackgroundActions(); }
void MainWindow::on_actionBgRXfMedium_triggered() { m_bgRXfSens=2; setBackgroundActions(); }
void MainWindow::on_actionBgRXfHigh_triggered() { m_bgRXfSens=3; setBackgroundActions(); }
void MainWindow::on_actionBgSensMin_triggered() { m_bgSensitivity=0; setBackgroundActions(); }
void MainWindow::on_actionBgSensLow_triggered() { m_bgSensitivity=1; setBackgroundActions(); }
void MainWindow::on_actionBgSensSubpass_triggered() { m_bgSensitivity=2; setBackgroundActions(); }
void MainWindow::on_actionFT8BgDeepOSD_toggled(bool checked) { m_bgDeepOSD=checked; }
void MainWindow::on_actionFT8BgTwoSlicings_toggled(bool checked) { m_bgTwoSlicings=checked; }
void MainWindow::on_actionFT8BgAltPass_toggled(bool checked) { m_bgAltPass=checked; }
void MainWindow::on_actionFT8BgClassic_toggled(bool checked) { m_bgClassic=checked ? CLASSIC_CYCLES : 0; refreshDecodePreset(); }   // P9
// CE3TSK: a band or mode change must not let the TX background keep decoding the previous
// context's retained audio and print it under the new dial - the .bgabort sentinel stops it
// within a pass (bg_lock_gone); decode() removes the sentinel so every fresh period runs
void MainWindow::abortTxBackground(QString const& reason)
{
  if(!m_bgPhase) return;
  QFile {m_config.temp_dir ().absoluteFilePath (".bgabort")}.open(QIODevice::ReadWrite);
  m_bgAbortAsked=true;   // item 81: this abort is ours, not the load's
  m_bgPhase=false; updateDecodeLabel();
  if(m_config.write_decoded_debug()) writeToALLTXT("TX background aborted: " + reason);
}

void MainWindow::setBackgroundActions()
{
  ui->actionFT8BgEnabled->setChecked(m_bgEnabled); ui->actionFT8BgSWL->setChecked(m_bgSWL);
  QAction* cyc[] = {ui->actionBgCycles3, ui->actionBgCycles4, ui->actionBgCycles5, ui->actionBgCycles6, ui->actionBgCycles7, ui->actionBgCycles8, ui->actionBgCycles9};
  QAction* swl[] = {ui->actionBgSWLcycles3, ui->actionBgSWLcycles4, ui->actionBgSWLcycles5, ui->actionBgSWLcycles6, ui->actionBgSWLcycles7, ui->actionBgSWLcycles8, ui->actionBgSWLcycles9};
  if(m_nBgCycles>=3 && m_nBgCycles<=9) cyc[m_nBgCycles-3]->setChecked(true);
  if(m_nBgSWLCycles>=3 && m_nBgSWLCycles<=9) swl[m_nBgSWLCycles-3]->setChecked(true);
  QAction* rxf[] = {ui->actionBgRXfLow, ui->actionBgRXfMedium, ui->actionBgRXfHigh};
  if(m_bgRXfSens>=1 && m_bgRXfSens<=3) rxf[m_bgRXfSens-1]->setChecked(true);
  QAction* sens[] = {ui->actionBgSensMin, ui->actionBgSensLow, ui->actionBgSensSubpass};
  if(m_bgSensitivity>=0 && m_bgSensitivity<=2) sens[m_bgSensitivity]->setChecked(true);
  ui->actionFT8BgDeepOSD->setChecked(m_bgDeepOSD); ui->actionFT8BgTwoSlicings->setChecked(m_bgTwoSlicings); ui->actionFT8BgAltPass->setChecked(m_bgAltPass);
  ui->actionFT8BgClassic->setChecked(m_bgClassic>0);
  QAction* levels[] = {ui->actionFT8BgOff, ui->actionFT8Bg1, ui->actionFT8Bg2, ui->actionFT8Bg3, ui->actionFT8Bg4, ui->actionFT8Bg5};
  if(m_bgEnsembleEffort<0) ui->actionFT8BgAuto->setChecked(true);
  else if(m_bgEnsembleEffort<=ENSEMBLE_MAX_MEMBERS) levels[m_bgEnsembleEffort]->setChecked(true);
}
void MainWindow::on_actionFT8PresetPipeline_triggered() { applyDecodePreset(DecodePreset::PipelineEnsemble); }
void MainWindow::on_actionFT8PresetPipelineFull_triggered() { applyDecodePreset(DecodePreset::PipelineEnsembleFull); }
void MainWindow::on_actionFT8PresetPipelineRun_triggered() { applyDecodePreset(DecodePreset::PipelineRun); }

void MainWindow::setEnsembleEffortAction()
{
  QAction* levels[] = {ui->actionFT8EnsembleOff, ui->actionFT8Ensemble1, ui->actionFT8Ensemble2, ui->actionFT8Ensemble3,
                       ui->actionFT8Ensemble4, ui->actionFT8Ensemble5};
  if(m_ft8EnsembleEffort==ENSEMBLE_BUDGET) ui->actionFT8EnsembleBudget->setChecked(true);   // CE3TSK P8
  else if(m_ft8EnsembleEffort<0) ui->actionFT8EnsembleAuto->setChecked(true);
  else if(m_ft8EnsembleEffort<=ENSEMBLE_MAX_MEMBERS) levels[m_ft8EnsembleEffort]->setChecked(true);
}

// CE3TSK: a preset only sets the ordinary controls (decodepreset.h has the recipes and the
// numbers behind them); the radio state is derived from the controls when the menu opens
void MainWindow::on_actionFT8PresetDefault_triggered() { applyDecodePreset(DecodePreset::Default); }
void MainWindow::on_actionFT8PresetMaxDecodes_triggered() { applyDecodePreset(DecodePreset::MaxDecodes); }
void MainWindow::on_actionFT8PresetPipelineLight_triggered() { applyDecodePreset(DecodePreset::PipelineMaxDecodesLight); }
void MainWindow::on_actionFT8PresetMaxEfficiency_triggered() { applyDecodePreset(DecodePreset::MaxEfficiency); }

void MainWindow::applyDecodePreset(DecodePreset p)
{
  auto const r = preset_recipe(p, effective_ft8_threads(m_ft8threads, QThread::idealThreadCount()));
  // the RX phase: the ordinary controls
  m_swl=r.rx.swl; ui->swlButton->setChecked(r.rx.swl);
  QAction* cycles[] = {ui->actionDecFT8cycles3, ui->actionDecFT8cycles4, ui->actionDecFT8cycles5, ui->actionDecFT8cycles6,
                       ui->actionDecFT8cycles7, ui->actionDecFT8cycles8, ui->actionDecFT8cycles9};
  QAction* swlcycles[] = {ui->actionDecFT8SWLcycles3, ui->actionDecFT8SWLcycles4, ui->actionDecFT8SWLcycles5, ui->actionDecFT8SWLcycles6,
                          ui->actionDecFT8SWLcycles7, ui->actionDecFT8SWLcycles8, ui->actionDecFT8SWLcycles9};
  if(r.rx.cycles>=3 && r.rx.cycles<=9) cycles[r.rx.cycles-3]->trigger();
  if(r.rx.swl_cycles>=3 && r.rx.swl_cycles<=9) swlcycles[r.rx.swl_cycles-3]->trigger();
  QAction* sens[] = {ui->actionFT8SensMin, ui->actionlowFT8thresholds, ui->actionFT8subpass};
  if(r.rx.sensitivity>=0 && r.rx.sensitivity<=2) sens[r.rx.sensitivity]->setChecked(true);
  QAction* rxf[] = {ui->actionRXfLow, ui->actionRXfMedium, ui->actionRXfHigh};
  if(r.rx.rxf_sens>=1 && r.rx.rxf_sens<=3) rxf[r.rx.rxf_sens-1]->trigger();
  ui->actionFT8DeepOSD->setChecked(r.rx.deep_osd);
  ui->actionFT8TwoSlicings->setChecked(r.rx.two_pass);
  ui->actionFT8AltPass->setChecked(r.rx.alt_pass);
  m_ft8EnsembleEffort = (p==DecodePreset::Ensemble) ? ENSEMBLE_AUTO : r.rx.ensemble; setEnsembleEffortAction();   // the ensemble preset means "auto"; the budgeted ones name their member count
  // the TX background phase and its recipe
  m_bgEnabled=r.background; m_bgSWL=r.bg.swl; m_nBgCycles=r.bg.cycles; m_nBgSWLCycles=r.bg.swl_cycles;
  m_bgSensitivity=r.bg.sensitivity; m_bgRXfSens=r.bg.rxf_sens; m_bgDeepOSD=r.bg.deep_osd; m_bgTwoSlicings=r.bg.two_pass;
  m_bgAltPass=r.bg.alt_pass; m_bgEnsembleEffort=r.bg.ensemble; m_bgClassic=r.bg_classic; setBackgroundActions();
  // P12: the side settings of every preset - the toggled slots keep the members, the decode
  // trigger follows the early-start value at once
  ui->actionFT8EarlyStart->setChecked(PRESET_EARLY_START);
  ui->actionFT8WidebandDXCallSearch->setChecked(PRESET_WIDE_DXCALL_SEARCH);
  setStopHSym();
  refreshDecodePreset();
}

/* CE3TSK: the RX and TX timing lamps, between the Contest and Preset lamps.

   RX carries the same figure the decode label prints as "Lag=" - how far past the start of the
   next period the receive decode ran - against the moment the reply has to be decided. TX says
   whether the background decoding that keeps working through your own transmission finished or
   was cut short by the next period, which is the "X" the decode count already carries. The
   thresholds and the colours live in decodelabel.h, beside the tints of the line they summarise.

   Both are grey and disabled where they mean nothing: outside FT8 and FT4, before the first
   period has been decoded, and - for TX - whenever no background is enabled. */
void MainWindow::updateTimingLamps()
{
  bool const ft8 = m_mode == "FT8";
  bool const ft4 = m_mode == "FT4";
  bool const ft2 = m_mode == "FT2";   // CE3TSK step 5
  bool const ft = ft8 || ft4 || ft2;
  bool const bg_on = (ft8 && m_bgEnabled) || (ft4 && m_ft4BgEnabled) || (ft2 && m_ft2Recipe.bgon);

  ui->labelRxTiming->setAutoFillBackground (true);
  ui->labelTxTiming->setAutoFillBackground (true);
  ui->labelRxTiming->setEnabled (ft && m_rxLagKnown);
  ui->labelTxTiming->setEnabled (ft && bg_on && m_bgLastKnown);

  auto const rx = (ft && m_rxLagKnown) ? rx_timing_level (m_rxLag, ft4, m_TRperiod) : TimingLevel::Unknown;   // CE3TSK: the period tells FT2 apart
  /* the background either made the period or it did not - two colours, never amber */
  auto const tx = (ft && bg_on && m_bgLastKnown)
                    ? (m_bgLastCut ? TimingLevel::Late : TimingLevel::Good) : TimingLevel::Unknown;
  ui->labelRxTiming->setStyleSheet (timing_lamp_style (rx, m_useDarkStyle));
  ui->labelTxTiming->setStyleSheet (timing_lamp_style (tx, m_useDarkStyle));
}

void MainWindow::refreshDecodePreset()
{
  updateTimingLamps();   // CE3TSK: the mode or the TX background switch may have moved
  int const threads = effective_ft8_threads(m_ft8threads, QThread::idealThreadCount());
  DecodeRecipe const r {{m_swl, m_nFT8Cycles, m_nFT8SWLCycles, m_ft8Sensitivity, m_nFT8RXfSens, m_ft8DeepOSD, m_ft8TwoSlicings, m_ft8AltPass,
                         ensemble_effort_members(m_ft8EnsembleEffort, threads)},
                        m_bgEnabled,
                        {m_bgSWL, m_nBgCycles, m_nBgSWLCycles, m_bgSensitivity, m_bgRXfSens, m_bgDeepOSD, m_bgTwoSlicings, m_bgAltPass, m_bgEnsembleEffort},
                        m_bgClassic};
  auto const p = recipe_preset(r, threads);
  ui->actionFT8PresetDefault->setChecked(p==DecodePreset::Default);
  ui->actionFT8PresetMaxEfficiency->setChecked(p==DecodePreset::MaxEfficiency);
  ui->actionFT8PresetMaxDecodes->setChecked(p==DecodePreset::MaxDecodes);
  ui->actionFT8PresetPipelineLight->setChecked(p==DecodePreset::PipelineMaxDecodesLight);
  ui->actionFT8PresetPipeline->setChecked(p==DecodePreset::PipelineEnsemble);
  ui->actionFT8PresetPipelineFull->setChecked(p==DecodePreset::PipelineEnsembleFull);
  ui->actionFT8PresetPipelineRun->setChecked(p==DecodePreset::PipelineRun);
  // P13: the preset lamp - styled like the Contest lamp, lit in the preset's dot colour
  // (white text on the dark blue/red/purple, black on the light ones); 3, E and Custom have no
  // colour and show as plain text in a grey box, enabled - not greyed. Outside FT8 the presets
  // do not apply: the lamp is greyed (disabled, the neutral box) whatever the controls say
  bool const ft8 = m_mode=="FT8";
  bool const ft4 = m_mode=="FT4";   // item 73: in FT4 the lamp reads the FT4 preset (letters F3PROM, FT8's colour scheme)
  bool const ft2 = m_mode=="FT2";    // CE3TSK step 5: and FT2's own in FT2
  auto const p4 = ft4_preset_of (currentFT4Recipe (), ft4Threads());
  auto const p2 = ft2_preset_of (m_ft2Recipe, ft4Threads());
  ui->labelPreset->setAutoFillBackground(true);
  ui->labelPreset->setEnabled(ft8 || ft4 || ft2);
  if (ft2) ui->labelPreset->setText(p2==FT2Preset::Custom ? QString("Custom") : QString("Preset %1").arg(QChar(ft2_preset_letter(p2))));
  else if (ft4) ui->labelPreset->setText(p4==FT4Preset::Custom ? QString("Custom") : QString("Preset %1").arg(QChar(ft4_preset_letter(p4))));
  else ui->labelPreset->setText(p==DecodePreset::Custom ? QString("Custom") : QString("Preset %1").arg(QChar(preset_letter(p))));   // the C of the table is never shown
  auto const c = ft8 ? preset_colour(p) : ft4 ? ft4_preset_colour(p4) : ft2 ? ft2_preset_colour(p2) : nullptr;   // CE3TSK step 5: FT2's tier colours its lamp too
  if (c) {
    QColor const bg(c);
    ui->labelPreset->setStyleSheet(QString("QLabel{color: %1; background: %2; border: 1px solid %3; border-radius: 3px; padding: 1px 4px}")
                                   .arg(bg.lightness() < 140 ? "#ffffff" : "#000000", bg.name(), bg.darker(150).name()));
  } else {
    ui->labelPreset->setStyleSheet("QLabel{border: 1px solid #808080; border-radius: 3px; padding: 1px 4px}");
  }
  QAction* const names[] = {ui->actionFT8PresetDefault, ui->actionFT8PresetMaxEfficiency, ui->actionFT8PresetMaxDecodes, ui->actionFT8PresetPipelineLight,
                            nullptr /* Ensemble: recipe kept, menu entry removed 2026-09-05 */, ui->actionFT8PresetPipeline, ui->actionFT8PresetPipelineFull, ui->actionFT8PresetPipelineRun};
  QAction* const names4[] = {ui->actionFT4PresetFast, ui->actionFT4PresetDefault, ui->actionFT4PresetBestPower,
                             ui->actionFT4PresetRecommended, ui->actionFT4PresetMaxDecodes, ui->actionFT4PresetMaxEffort};
  QAction* const names2[] = {ui->actionFT2PresetFast, ui->actionFT2PresetDefault,
                             ui->actionFT2PresetRecommended, ui->actionFT2PresetMaxEffort};   // CE3TSK step 5
  /* CE3TSK: the four fallbacks are translated too - every other branch hands over an action's
     text, which the catalogues already carry, so these were the last English left on the lamp.
     "Custom" itself stays the English word in every language: it is what the lamp reads, and the
     lamp's own tooltip quotes it verbatim in all 20 catalogues. */
  ui->labelPreset->setToolTip(wrap_tooltip(ft2 ? (p2==FT2Preset::Custom ? tr("Custom - the FT2 RX / TX background controls match no preset") : names2[static_cast<int>(p2)]->text())
                              : ft4 ? (p4==FT4Preset::Custom ? tr("Custom - the FT4 RX / TX background controls match no preset") : names4[static_cast<int>(p4)]->text())
                              : !ft8 ? tr("FT* decoding preset - greyed while the mode is none of them")
                              : p==DecodePreset::Custom ? tr("Custom - the RX / TX background controls match no preset")
                              : !names[static_cast<int>(p)] ? tr("ensemble - the RX-only recipe of the former Ensemble preset (no menu entry)") : names[static_cast<int>(p)]->text()));
}

// CE3TSK: the decoder label's count: "/D-" while the period's decode runs, "/D" when it is
// done, "/D+N=S|" while the TX background runs (D from the decode, N from the background,
// S their sum), "/D+N=S" when it is done - written tight, with no spaces or brackets, so the
// trailing marker survives the clipping the label suffers at the right edge on Windows
/* CE3TSK: the header line above the decodes is pinned to exactly one text line, so that it
   cannot steal room from the messages. The pin was QFontMetrics(decoded text font).height() -
   the metric of the monospace font alone. But the line is RICH text (the tinted Avg, Lag and
   count), which Qt lays out through QTextDocument, and in Japanese, Chinese and Korean the
   translated "Avg=" and "Lag=" are drawn from a CJK fallback font whose ascent and descent are
   taller than the monospace one. The line box then outgrew its pin and was cut off at the top -
   on screen it looked as though the text had slipped downwards, and only once something was
   decoded, because that is when the tinted figures appear.

   So measure what is actually about to be drawn, in the font it will be drawn in, and give the
   label that height - never less than the plain metric, so nothing shrinks in the Latin
   languages where it was always right. */
void MainWindow::fitDecodeLabels()
{
  QFont const font = m_config.decoded_text_font ();
  int const floor_height = QFontMetrics {font}.height ();
  auto fit = [&font, floor_height] (QLabel * label) {
    QTextDocument doc;
    doc.setDefaultFont (font);
    doc.setDocumentMargin (0);
    QString const text = label->text ();
    if (Qt::mightBeRichText (text)) doc.setHtml (text); else doc.setPlainText (text);
    int const wanted = qMax (floor_height, int (doc.size ().height () + 0.5));
    if (label->maximumHeight () != wanted)
      {
        label->setMinimumHeight (wanted);
        label->setMaximumHeight (wanted);
      }
  };
  fit (ui->decodedTextLabel);
  fit (ui->decodedTextLabel2);
}

void MainWindow::updateDecodeLabel()
{
  // the label is Qt::AutoText, which takes a string for rich text only when it starts with a
  // tag - hence the enclosing span; the other setText sites stay plain
  ui->decodedTextLabel->setTextFormat(Qt::RichText);   // the .ui declares it plain; the other sites set it back
  ui->decodedTextLabel->setText("<span>"+decode_label_prefix_html(m_decodeLabelPrefix)+decode_avg_html(m_decodeAvg, m_useDarkStyle)
                                +decode_label_prefix_html(m_decodeLabelMid)+decode_lag_html(m_decodeLag, m_useDarkStyle)
                                +decode_count_label_html(m_rxPhase, m_bgPhase, m_bgRan, m_nDecodes, m_nDecodesRx, m_useDarkStyle, m_bgCut)+"</span>");
  fitDecodeLabels();   // CE3TSK: the tinted figures make the line taller in CJK
}
void MainWindow::on_actionFT8subpass_toggled(bool checked) { if(checked) m_ft8Sensitivity=2; }
void MainWindow::on_actionFT8EarlyStart_toggled(bool checked) { m_FT8EarlyStart=checked; }
void MainWindow::on_actionFT8WidebandDXCallSearch_toggled(bool checked) { m_FT8WideDxCallSearch=checked; }
void MainWindow::on_actionBypass_text_filters_on_RX_frequency_toggled(bool checked) { m_bypassRxfFilters=checked; }
void MainWindow::on_actionBypass_all_text_filters_toggled(bool checked) { m_bypassAllFilters=checked; ui->bypassButton->setChecked(checked); }
void MainWindow::on_actionEnable_main_window_popup_toggled(bool checked) { m_windowPopup=checked; }
void MainWindow::on_actionAutoErase_toggled(bool checked) { m_autoErase=checked; }
void MainWindow::on_actionEraseWindowsAtBandChange_toggled(bool checked) { m_autoEraseBC=checked; }

void MainWindow::on_actionReport_message_priority_toggled(bool checked)
{
  if(checked && m_maxDistance) { ui->actionMaxDistance->setChecked(false); m_maxDistance=false; }
  m_rprtPriority=checked;
}

void MainWindow::on_actionKeyboard_shortcuts_triggered()
{
  if(!m_shortcuts) {
    QFont font;
    font.setPointSize (10);
    m_shortcuts.reset (new HelpTextWindow {tr ("Keyboard Shortcuts"),
                                               //: Keyboard shortcuts help window contents
                                               tr (R"(<table cellspacing=1>
  <tr><td><b>F1       </b></td><td>Online User's Guide</td><td><b>Ctrl+F1  </b></td><td>About JTDX_contest</td></tr>
  <tr><td><b>F2       </b></td><td>Open configuration window</td></tr>
  <tr><td><b>F3       </b></td><td>Display keyboard shortcuts</td></tr>
  <tr><td><b>F4       </b></td><td>Clear DX Call/Grid and Tx messages</td><td><b>Alt+F4   </b></td><td>Exit program</td></tr>
  <tr><td><b>F5       </b></td><td>Display special mouse commands</td></tr>
  <tr><td><b>F6       </b></td><td>Open next file in directory</td><td><b>Shift+F6 </b></td><td>Decode all remaining files in directory</td></tr>
  <tr><td><b>F7       </b></td><td>Open log by assigned in the operating system viewer</td></tr>
  <tr><td><b>F11      </b></td><td>Move Rx frequency down 1 Hz</td><td><b>Ctrl+F11 </b></td><td>Move Rx and Tx frequencies down 1 Hz</td></tr>
  <tr><td><b>F12      </b></td><td>Move Rx frequency up 1 Hz</td><td><b>Ctrl+F12 </b></td><td>Move Rx and Tx frequencies up 1 Hz</td></tr>
  <tr><td><b>Alt+1-6  </b></td><td>Set now transmission to this number on Tab 1</td></tr>
  <tr><td><b>Ctl+1-6  </b></td><td>Set next transmission to this number on Tab 1</td></tr>
  <tr><td><b>Alt+Ctrl+A    </b></td><td>Clear wanted callsign list</td></tr>
  <tr><td><b>Alt+B/C  </b></td><td>Switch to FT8/FT4 mode</td></tr>
  <tr><td><b>Alt+D    </b></td><td>Decode again at QSO frequency</td><td><b>Shift+D  </b></td><td>Full decode (both windows)</td></tr>
  <tr><td><b>Alt+E    </b></td><td>Erase</td></tr>
  <tr><td><b>Alt+F    </b></td><td>Toggle bypass all text filters</td><td><b>Ctrl+F   </b></td><td>Edit the free text message box</td></tr>
  <tr><td><b>Alt+G    </b></td><td>Generate standard messages</td></tr>
  <tr><td><b>Alt+H    </b></td><td>Halt Tx</td></tr>
  <tr><td><b>Ctrl+L   </b></td><td>Lookup callsign in database, generate standard messages</td></tr>
  <tr><td><b>Alt+M    </b></td><td>Monitor</td></tr>
  <tr><td><b>Alt+N    </b></td><td>Enable Tx</td></tr>
  <tr><td><b>Alt+Q    </b></td><td>Log QSO</td></tr>
  <tr><td><b>Alt+S    </b></td><td>Stop monitoring</td></tr>
  <tr><td><b>Alt+T    </b></td><td>Tune</td></tr>
  <tr><td><b>Alt+V    </b></td><td>Save the most recently completed *.wav file</td></tr>
  <tr><td><b>Alt+Z    </b></td><td>Filter, this shortcut is being supported in main UI and widegraph UI</td></tr>
  <tr><td><b>Esc    </b></td><td>Halt Tx</td></tr>
</table>)"), font});
  }
  m_shortcuts->showNormal ();
  m_shortcuts->raise ();
}

void MainWindow::on_actionSpecial_mouse_commands_triggered()
{
  if(!m_mouseCmnds) {
    QFont font;
    font.setPointSize (10);
    m_mouseCmnds.reset (new HelpTextWindow {tr ("Special Mouse Commands"),tr(R"(<table cellpadding=5>
  <tr>
    <th align="right">Click on</th>
    <th align="left">Action</th>
  </tr>
  <tr>
    <td align="right">Waterfall:</td>
    <td>Set Rx frequency.<br/>
        Double-click to set Rx frequency and decode there.<br/>
        Ctrl-click to set Rx and Tx frequencies.<br/>
        Unlocked TX=RX:<br/>
        use left button to set RX frequency<br/>
        use ALT+left button to set RX frequency and switch on Filter<br/>
        use right button to set TX frequency
    </td>
  </tr>
  <tr>
    <td align="right">Decoded text:</td>
    <td>Double-click to copy second callsign to Dx Call,<br/>
        locator to Dx Grid; change Rx and Tx frequencies to<br/>
        decoded signal's frequency; generate standard messages.<br/>
        If first callsign is your own, Tx frequency is not<br/>
        changed unless CTRL is held down when double-clicking.<br/><br/>
        ALT+double-click will also halt Tx if Enable Tx button is active.<br/><br/>
        CTRL+ALT+double-click will only add second callsign from decoded<br/>
        message into wanted callsign list.
    </td>
  </tr>
  <tr>
    <td align="right">Erase button:</td>
    <td>Click right button to erase QSO window.<br/>
        Click left button to erase Band Activity window.<br/>
        Double-click left or right button to erase QSO <br/>
        and Band Activity windows.
    </td>
  </tr>
</table>
)"), font});
  }
  m_mouseCmnds->showNormal ();
  m_mouseCmnds->raise ();
}

void MainWindow::on_DecodeButton_clicked (bool /* checked */)	//Decode request
{
  if(!m_rxDone) { ui->DecodeButton->setChecked (false); return; }
  if(!m_decoderBusy && m_mode != "WSPR-2") {
    dec_data.params.newdat=0;
    dec_data.params.nagain=1;
    m_blankLine=false; // don't insert the separator again
	m_manualDecode=true;
    decode();
  }
}

void MainWindow::freezeDecode(int n)                          //freezeDecode()
{
  if(!m_decoderBusy) {
    if((n%100)==2) on_DecodeButton_clicked (true);
    dec_data.params.nagainfil=1;
  }
}

void MainWindow::decode()                                       //decode()
{
  if(!m_dataAvailable or m_TRperiod==0.0) { m_manualDecode=false; return; }
  decodeBusy(true); // shall be second line
  m_bgPhase=false;   // CE3TSK: a new period's decode ends any background phase
  if(m_autoErase) ui->decodedTextBrowser->clear();
  if(m_hint && !m_hisCall.isEmpty() && (m_enableTx || m_lasthint)) {
    if(m_curMsgTx.startsWith(m_hisCall+" ")) dec_data.params.nstophint = 0; // DxCall matched to last transmitted message
    m_lasthint=false;
  }
//  printf("%s(%0.1f) Timing decode start: %d\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),m_jtdxtime->GetOffset(),dec_data.params.nstophint);
  m_nDecodes = 0;
  m_rxPhase=true; m_bgRan=false; m_nDecodesRx=0;   // CE3TSK: the label counts from here: "/0 -"
  if(m_mode=="FT8") { m_decodeLabelPrefix="UTC     dB   DT "+tr("Freq  ")+" "; m_decodeAvg.clear(); m_decodeLabelMid.clear(); m_decodeLag.clear(); updateDecodeLabel(); }
  m_reply_me = false;
  m_reply_other = false;
  m_reply_CQ73 = false;
  ui->DecodeButton->setChecked (true);
  if(!m_manualDecode) { m_processAuto_done = false; m_callFirst73 = false; }
  m_used_freq = 0;
  if(m_diskData && !m_mode.startsWith("FT")) dec_data.params.nutc=dec_data.params.nutc/100;
  if(dec_data.params.newdat==1) {
    m_msDecStarted=m_jtdxtime->currentMSecsSinceEpoch2();
    if(!m_mode.startsWith("FT")) {
      qint64 ms = m_msDecStarted % 86400000;
      int imin=ms/60000;
      int ihr=imin/60;
      imin=imin % 60;
      if(m_TRperiod>=60.0) imin=imin - (imin % (int(m_TRperiod)/60));
      dec_data.params.nutc=100*ihr + imin;
    }
  }
  if(dec_data.params.newdat==1 && (!m_diskData) && m_mode.startsWith("FT")) {
    qint64 ms=1000.0*(2.0-m_TRperiod);
    QDateTime time=m_jtdxtime->currentDateTimeUtc2().addMSecs(ms);
    int ihr=time.toString("hh").toInt();
    int imin=time.toString("mm").toInt();
    int isec=time.toString("ss").toInt();
    isec=isec - fmod(double(isec),m_TRperiod);
    dec_data.params.nutc=10000*ihr + 100*imin + isec;
  }

//FT8 block of parameters
  dec_data.params.nQSOProgress = m_QSOProgress;
  dec_data.params.nftx = ui->TxFreqSpinBox->value ();
  if(m_freqNominal < 30000000) dec_data.params.napwid=5; // FT8AP decoding bandwidth for 'mycall hiscall ???' and RRR,RR73,73 messages
  else if(m_freqNominal < 100000000) dec_data.params.napwid=15;
  else dec_data.params.napwid=50;
  dec_data.params.nmt=m_ft8threads;
  dec_data.params.ncandthin=m_ncandthin;
  dec_data.params.ndtcenter=100 * ui->DTCenterSpinBox->value();
  dec_data.params.nft8cycles=m_nFT8Cycles;
  dec_data.params.nft8swlcycles=m_nFT8SWLCycles;
  if(m_houndMode) { dec_data.params.nft8rxfsens=1; } else { dec_data.params.nft8rxfsens=m_nFT8RXfSens; }
  dec_data.params.nft4depth=(m_mode=="FT2") ? m_ft2Recipe.depth : m_nFT4depth;   // CE3TSK step 5
  if(m_ft8Sensitivity==0) dec_data.params.lft8lowth=false;
  else  dec_data.params.lft8lowth=true;
  if(m_ft8Sensitivity==2) dec_data.params.lft8subpass=true;
  else dec_data.params.lft8subpass=false;
  dec_data.params.ltxing=(m_enableTx && (m_jtdxtime->currentMSecsSinceEpoch2()-m_mslastTX) < 26000) ? 1 : 0;
  dec_data.params.lhidetest=(ui->actionHide_FT_contest_messages->isChecked() && !m_bypassAllFilters) ? 1 : 0;
  dec_data.params.lhidetelemetry=(ui->actionHide_telemetry_messages->isChecked() && !m_bypassAllFilters) ? 1 : 0;
  dec_data.params.lhideft8dupes=ui->actionHide_FT8_dupe_messages->isChecked() ? 1 : 0;
  dec_data.params.lhound=m_houndMode ? 1 : 0;
  dec_data.params.lhidehash=m_config.hide2ndHash() && !m_bypassAllFilters;
  dec_data.params.lcommonft8b=m_commonFT8b;
  dec_data.params.lmycallstd=m_bMyCallStd; dec_data.params.lhiscallstd=m_bHisCallStd;
  dec_data.params.lapmyc=m_lapmyc;
  dec_data.params.lmodechanged=m_modeChanged ? 1 : 0; m_modeChanged=false;
  dec_data.params.lbandchanged=m_bandChanged ? 1 : 0; m_bandChanged=false;
  dec_data.params.lmultinst=m_multInst ? 1 : 0;
  dec_data.params.lskiptx1=m_skipTx1 ? 1 : 0;
  dec_data.params.nlasttx=m_nlasttx;
  dec_data.params.lforcesync=ui->syncButton->isChecked() && m_mode=="FT8";
  dec_data.params.learlystart=m_FT8EarlyStart ? 1 : 0;
  dec_data.params.lft8deeposd=m_ft8DeepOSD ? 1 : 0;   // CE3TSK: OSD order 2 for every candidate
  dec_data.params.lft8twopass=m_ft8TwoSlicings ? 1 : 0;   // CE3TSK: second slicing pass
  dec_data.params.lft8altpass=m_ft8AltPass ? 1 : 0;   // CE3TSK: alternate-approach pass
  /* CE3TSK step 5: FT2 shares every one of these fields with FT4 - it is the same decoder - but
     carries its OWN values, so the recipe that fills them is the active mode's. */
  FT4Recipe const fr = (m_mode=="FT2") ? m_ft2Recipe : currentFT4Recipe();
  dec_data.params.lft4altpass=fr.alt ? 1 : 0;   // CE3TSK: FT4 expert
  dec_data.params.lft4deeposd=fr.deeposd ? 1 : 0;   // CE3TSK item 58
  dec_data.params.nft4bgensemble=ft4_bg_effort_members(fr.bg, ft4Threads());   // CE3TSK items 59/73: the target, auto resolved here
  dec_data.params.nft4bgeffort=((m_mode=="FT4" || m_mode=="FT2") && fr.bgon) ? 1 : 0;   // CE3TSK item 78: the switch, sent as FT8's nft8bgeffort is - it alone decides whether the phase runs
  dec_data.params.nft4bgdepth=fr.bgdepth;   // CE3TSK item 69
  dec_data.params.lft4bgdeeposd=fr.bgdeeposd ? 1 : 0;
  dec_data.params.lft4bgaltpass=fr.bgalt ? 1 : 0;
  dec_data.params.lft4bgtwopass=fr.bgtwopass ? 1 : 0;
  dec_data.params.lft4bgresidual=fr.bgresidual ? 1 : 0;   // CE3TSK item 72
  dec_data.params.nft4sens=fr.sens;
  dec_data.params.nft4bgsens=fr.bgsens;   // CE3TSK item 73
  dec_data.params.nft4rxfsens=fr.rxf;   // CE3TSK item 75
  dec_data.params.nft4bgrxfsens=fr.bgrxf;
  dec_data.params.nft4ensemble=ft4_effort_members(fr.members, ft4Threads());   // item 73
  dec_data.params.lft4twopass=fr.twopass;
  dec_data.params.nft8ensemble=ensemble_effort_members(m_ft8EnsembleEffort, effective_ft8_threads(m_ft8threads, QThread::idealThreadCount()));   // CE3TSK (P8: budget auto passes -2 through)
  /* CE3TSK: FT2 has a 0.58 s reply deadline, so FT8's 2.7 s budget would never bite; until FT2
     gets its own setting (step 5 of the port) it uses the same 5 tenths file mode gives it. */
  dec_data.params.nrxbudget=(m_mode=="FT4") ? m_ft4RXBudget : (m_mode=="FT2") ? m_ft2RXBudget : m_ft8RXBudget;   // CE3TSK P8; item 80: FT4's own budget, step 5: FT2's own
  dec_data.params.nft8bgeffort=(m_mode=="FT8" && m_bgEnabled) ? 1 : 0;   // CE3TSK: the TX background phase (pipeline ensemble)
  dec_data.params.nbgmargin=(m_mode=="FT4") ? m_ft4BgMargin : (m_mode=="FT2") ? m_ft2BgMargin : m_ft8BackgroundMargin;   // item 80: FT4's own margin, step 5: FT2's own
  {   // CE3TSK P7: two periods of background when TX is enabled and the coming period is ours
    int const thisPeriod = period_index_of_trigger (0.001 * (m_jtdxtime->currentMSecsSinceEpoch2 () % 86400000), m_TRperiod);
    bool const bgmode = (m_mode=="FT8" && m_bgEnabled) || (m_mode=="FT4" && m_ft4BgEnabled)
                        || (m_mode=="FT2" && m_ft2Recipe.bgon);   // item 80: the two-period window reaches FT4's background too; step 5: FT2's
    dec_data.params.nbgbudget = background_budget_tenths (m_enableTx && !m_diskData && bgmode, tx_period_next (m_txFirst, thisPeriod), m_TRperiod,
                                                          m_diskData && bgmode);   // a replay gets the full window: the whole unit list, 100 on the benchmark
  }
  dec_data.params.lbgswl=m_bgSWL; dec_data.params.nft8bgcycles=m_nBgCycles; dec_data.params.nft8bgswlcycles=m_nBgSWLCycles;
  dec_data.params.lbglowth=(m_bgSensitivity>=1); dec_data.params.lbgsubpass=(m_bgSensitivity>=2); dec_data.params.nft8bgrxfsens=m_bgRXfSens;
  dec_data.params.lbgdeeposd=m_bgDeepOSD; dec_data.params.lbgtwopass=m_bgTwoSlicings; dec_data.params.lbgaltpass=m_bgAltPass;
  dec_data.params.nft8bgensemble=m_bgEnsembleEffort;
  dec_data.params.nft8bgclassic=m_bgClassic;   // P9

  dec_data.params.nsecbandchanged=m_nsecBandChanged; m_nsecBandChanged=0;
  dec_data.params.nswl=m_swl ? 1 : 0;
  dec_data.params.nfilter=m_filter ? 1 : 0;
  dec_data.params.nagcc=m_agcc ? 1 : 0;
  dec_data.params.nhint=m_hint ? 1 : 0;
  dec_data.params.ndelay=m_delay;
  dec_data.params.nfqso=m_wideGraph->rxFreq();
  dec_data.params.ndepth=m_ndepth;
  dec_data.params.nranera=m_config.ntrials();
  dec_data.params.ntrials10=m_config.ntrials10();
  dec_data.params.ntrialsrxf10=m_config.ntrialsrxf10();
  dec_data.params.nprepass=m_config.npreampass();
  dec_data.params.naggressive=m_config.aggressive();
  dec_data.params.nharmonicsdepth=m_config.harmonicsdepth();
  dec_data.params.ntopfreq65=m_config.ntopfreq65();
  dec_data.params.nsdecatt=m_config.nsingdecatt();
  dec_data.params.fmaskact=m_config.fmaskact();
  dec_data.params.ndiskdat=0;
  if(m_diskData) dec_data.params.ndiskdat=1;
  decode_range (m_mode.startsWith ("FT"), m_decodeBandwidth, m_wideGraph->nStartFreq (), m_wideGraph->Fmax (),   // CE3TSK
               dec_data.params.nfa, dec_data.params.nfb);
  dec_data.params.nfSplit=m_wideGraph->Fmin();
  dec_data.params.ntol=50; // this value is not being used 
  if(m_mode=="JT9+JT65") {
    dec_data.params.ntol=20;
  }
  if(dec_data.params.nutc < m_nutc0) m_RxLog = 1;       //Date and Time to all.txt
  m_nutc0=dec_data.params.nutc;

  if(m_modeTx=="JT65") { dec_data.params.ntxmode=65; }
  else if(m_modeTx=="JT9") { dec_data.params.ntxmode=9; }
  
  if(m_mode=="FT8") dec_data.params.nmode=8;
  else if(m_mode=="FT4") dec_data.params.nmode=4;
  /* CE3TSK: FT2 rides the FT4 decoder - nmode 52 tells decoder.f90 to stretch the period x2 and
     halve the frequency limits. It carries FT2's settings in FT4's own parameter fields, which is
     why the shared block did not have to grow; the two modes never decode at the same time. */
  else if(m_mode=="FT2") dec_data.params.nmode=52;
  else if(m_mode=="JT9+JT65") dec_data.params.nmode=9+65;
  else if(m_mode=="JT9") dec_data.params.nmode=9;
  else if(m_mode=="JT65") dec_data.params.nmode=65;
  else if(m_mode=="T10") {
    dec_data.params.nmode=10;
    dec_data.params.ntxmode=10;
    dec_data.params.ntol=20; }
  
  dec_data.params.ntrperiod=int(m_TRperiod);

//  strncpy(dec_data.params.datetime, m_dateTime.toLatin1(), 20);
  strncpy(dec_data.params.mycall, (m_config.my_callsign()+"            ").toLatin1(),12);
  strncpy(dec_data.params.mybcall, (Radio::base_callsign(m_config.my_callsign())+"            ").toLatin1(),12);
//  strncpy(dec_data.params.mygrid, (m_config.my_grid()+"      ").toLatin1(),6);
  QString hisCall=m_hisCall;
  QString hisGrid=m_hisGrid;
  strncpy(dec_data.params.hiscall,(hisCall+"            ").toLatin1(),12);
  if(hisCall.length()<3) { hisCall.clear(); strncpy(dec_data.params.hisbcall,(hisCall+"            ").toLatin1(),12); }
  else strncpy(dec_data.params.hisbcall,(Radio::base_callsign(hisCall)+"            ").toLatin1(),12);
  strncpy(dec_data.params.hisgrid,(hisGrid+"      ").toLatin1(),6);

  if(!hisCall.isEmpty() && !m_enableTx && hisCall != m_lastloggedcall) dec_data.params.lenabledxcsearch=true;
  else dec_data.params.lenabledxcsearch=false;
  dec_data.params.lwidedxcsearch=m_FT8WideDxCallSearch ? 1 : 0; 

  //newdat=1  ==> this is new data, must do the big FFT
  //nagain=1  ==> decode only at fQSO +/- Tol

  char *to = (char*)mem_jtdxjt9->data();
  char *from = (char*) dec_data.ss;
  int size=sizeof(struct dec_data);
  if(dec_data.params.newdat==0) {
    int noffset {offsetof (struct dec_data, params.nutc)};
    to += noffset;
    from += noffset;
    size -= noffset;
  }
  /* CE3TSK: number this request before it is published. The decoder serves by number, so a
     request cannot be lost the way the .lock edge could be (commons.h, ndecreq). Incremented
     here, with the data already staged and the lock still in place, so the value and the audio
     reach the decoder together. */
  ++dec_data.params.ndecreq;
  memcpy(to, from, qMin(mem_jtdxjt9->size(), size));
  QFile {m_config.temp_dir ().absoluteFilePath (".bgabort")}.remove ();   // CE3TSK: a fresh decode clears any context-change abort
  QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.remove (); // Allow jtdxjt9 to start
  if(m_config.write_decoded_debug()) {
    QString swl{""}, cycles{""};
    if(m_mode=="FT8") {
      if(m_swl) cycles = "SWL On, cycles: " + QString::number(m_nFT8SWLCycles);
      else cycles = "SWL Off, cycles: " + QString::number(m_nFT8Cycles);
    }
    else swl = (m_swl ? "SWL On " : "SWL Off ");
    writeToALLTXT("Decoder started " + swl + cycles);
  }
  m_msDecoderStarted = m_jtdxtime->currentMSecsSinceEpoch2();   // CE3TSK: the recovery watchdog's clock
}

void MainWindow::process_Auto()
{
  /* CE3TSK: contest - stations that answer the exchange with a signal report are not in the
     contest (contestReportAbort). Take them out of the QSO history before anything looks at
     it: autoseq() picks one station per pass and is disarmed by that pick, so discarding an
     unwanted choice afterwards would waste the whole period. Keeping them out of the list
     means the caller behind them is answered in this period. */
  if (m_wwDigi)
    {
      qint64 const nowms = m_jtdxtime->currentMSecsSinceEpoch2 ();
      m_contestIgnore.prune (nowms);
      for (auto const& skipped : m_contestIgnore.calls (nowms)) m_qsoHistory.forget (skipped);
    }
  int count = 0;
  int prio = 0;
  bool counters = true;
  bool counters2 = true;
  bool qsoJustFinished = false; /* CE3TSK: set when the post-QSO hook dealt with this pass */
  m_status = QsoHistory::NONE;
  QString hisCall = m_hisCall;
  QString rpt = m_rpt;
  QString grid = m_hisGrid;
  QString mode = "";
  unsigned time = 0;
  int rx = ui->RxFreqSpinBox->value ();
  int tx = ui->TxFreqSpinBox->value ();
  QStringList StrStatus = {"NONE","RFIN","RCQ","SCQ","RCALL","SCALL","RREPORT","SREPORT","RRREPORT","SRREPORT","RRR","SRR","RRR73","SRR73","R73","S73","FIN"};
  if (!hisCall.isEmpty ()) {
    if (m_houndMode) count = -1; //marker for changing status to FIN when status is RRR73
    m_status = m_qsoHistory.autoseq(hisCall,grid,rpt,rx,tx,time,count,prio,mode);
    if (m_transmitting) count -=1;
    if(m_config.write_decoded_debug()) {
      QString StrDirection = "";
      if(m_status == QsoHistory::FIN) StrDirection = " auto sequence is finished;";
      else if(m_status == QsoHistory::NONE) StrDirection = " auto sequence is not started;";
      writeToALLTXT("hisCall:" + hisCall + " time:" + QString::number(time) + " autoseq: " + StrDirection + " status: " + StrStatus[m_status] + " count: " + QString::number(count)+ " prio: " + QString::number(prio));
    }
    if (m_houndMode ) { //WSJT-X Fox will drop QSO if R+Report from Hound is not decoded after three attempts 
      if (m_status == QsoHistory::SRREPORT || m_status == QsoHistory::RREPORT) {
        if(count > 3) {
          haltTx("DXpQSO failed after three TX of R+REPORT message ");
          count = m_qsoHistory.reset_count(hisCall,QsoHistory::RCQ);
          ui->TxFreqSpinBox->setValue (m_lastCallingFreq);
          m_status = QsoHistory::RCQ;
         } else if (m_houndTXfreqJumps && rx > 199 && rx < 1000) {
          if (count == 1) {
             ui->TxFreqSpinBox->setValue (rx);
           } else if (rx < 600) {
             ui->TxFreqSpinBox->setValue (rx+300);
           } else {
             ui->TxFreqSpinBox->setValue (rx-300);
           }
         }
      }
    } else if ((qsoJustFinished = afterQsoFinished (hisCall, grid, rpt, prio, count, counters, StrStatus))) {
      /* CE3TSK: the post-QSO hook dealt with this pass. A policy that does nothing returns
         false here and the chain below continues exactly as it would have. */
    } else if ((m_status == QsoHistory::SRR73 || m_status >= QsoHistory::S73) && !m_singleshot && !m_config.autolog() && m_lastloggedcall == m_hisCall && !m_lockTxFreq &&
        (tx == 1 || abs(rx - ui->TxFreqSpinBox->value ()) > m_nguardfreq)) {
      clearDX (" cleared, AutoSeq QSO finished");
      hisCall = m_hisCall;
      grid = m_hisGrid;
      m_status = QsoHistory::NONE;
    } else if ((m_status == QsoHistory::RCQ || m_status == QsoHistory::SCALL || (m_status == QsoHistory::SREPORT && m_skipTx1 && !m_houndMode)) && m_config.answerCQCount() &&
        ((prio > 4 && prio < 17) || prio < 2 || m_strictdirCQ) && (m_config.nAnswerCQCounter() <= count || replyOtherOverridesCounters ())) {
      clearDX (" cleared, RCQ/SCALL/SREPORT count reached");
      if (replyOtherOverridesCounters ())
          counters2 = false;
      else {
          m_counter = m_config.nAnswerCQCounter(); 
          m_qsoHistory.calllist(hisCall,rpt.toInt(),time);
      }
      count = m_qsoHistory.reset_count(hisCall);
      hisCall = m_hisCall;
      grid = m_hisGrid;
      m_status = QsoHistory::NONE;
      if (m_singleshot)
        counters = false;
    } else if ((m_status == QsoHistory::RCALL || (m_status == QsoHistory::SREPORT && !m_skipTx1)) && m_config.answerInCallCount() && 
        (m_config.nAnswerInCallCounter() <= count || replyOtherOverridesCounters ())) {
      clearDX (" cleared, RCALL/SREPORT count reached");
      m_qsoHistory.calllist(hisCall,rpt.toInt(),time);
      count = m_qsoHistory.reset_count(hisCall);
      hisCall = m_hisCall;
      grid = m_hisGrid;
      m_status = QsoHistory::NONE;
      counters2 = false;
      if (m_singleshot)
        counters = false;
    } else if ((m_status == QsoHistory::RREPORT || m_status == QsoHistory::SRREPORT) && m_config.sentRReportCount() && 
        m_config.nSentRReportCounter() <= count) {
      clearDX (" cleared, RREPORT/SRREPORT count reached");
      count = m_qsoHistory.reset_count(hisCall);
      hisCall = m_hisCall;
      grid = m_hisGrid;
      m_status = QsoHistory::NONE;
      counters2 = false;
      if (m_singleshot)
        counters = false;
    } else if ((m_status == QsoHistory::RRR || m_status == QsoHistory::RRR73 || m_status == QsoHistory::R73 || m_status == QsoHistory::SRR73 || m_status == QsoHistory::S73) && 
        m_config.sentRR7373Count() && m_config.nSentRR7373Counter() <= count) {
      clearDX (" cleared, RRR|RR73|R73 count reached");
      count = m_qsoHistory.reset_count(hisCall);
      hisCall = m_hisCall;
      grid = m_hisGrid;
      m_status = QsoHistory::NONE;
      counters2 = false;
      if (m_singleshot)
        counters = false;
    }
  }
  if (hisCall.isEmpty () && counters && !m_houndMode && m_callMode!=0) {
    auto ms = m_msDecStarted % 86400000;
    auto secs = round(ms / 1000.0) +1;
    int nmod = fmod(double(secs),2.0*m_TRperiod);
    if(m_callPrioCQ && !m_lockTxFreq && counters2 && m_counter == 0 && m_txFirst != (nmod!=0)) { time=1; }    //highiest priority, evaluating response to CQ first, then searching CQ decoded messages 
    else { if (m_counter > 0) m_counter -= 1; time=0; } //highiest priority, evaluating response to CQ only
    /* CE3TSK: newGridUserChoice() not newGrid(): a contest forces grid highlighting on, and
       reading the forced value here would make autoselect behave as if wanted-criteria had
       been set when the operator set none, quietly refusing worked-before stations. */
    if ((!m_config.newDXCC() && !m_config.newGridUserChoice() && !m_config.newPx() && !m_config.newCall()) || m_answerWorkedB4) time |= 128;
    if ((!m_config.newDXCC() && !m_config.newGridUserChoice() && !m_config.newPx() && !m_config.newCall()) || m_callWorkedB4) time |= 64;
    else if (m_callHigherNewCall) time |= 256;
    if (m_rprtPriority) time |= 16;
    if (m_maxDistance) time |= 32;
    m_status = m_qsoHistory.autoseq(hisCall,grid,rpt,rx,tx,time,count,prio,mode);
    if(m_config.write_decoded_debug()) {
      QString StrDirection = "";
      if(m_status == QsoHistory::FIN) StrDirection = " auto sequence is finished;";
      else if(m_status == QsoHistory::NONE) StrDirection = " auto sequence is not started;";
      QString StrPriority = "";
      if (!hisCall.isEmpty ()) {
        if (prio > 45) StrPriority = " New Grid Field "; /* CE3TSK: WW Digi multiplier, 48/49 (46/47 no longer used since 2026-08-29) */
        else if (prio > 31) StrPriority = QString {" Contest points %1 "}.arg ((prio - 30) / 2); /* CE3TSK: 32/33 is 1 point, 44/45 is 7 */
        else if (prio > 27) StrPriority = " New CQZ ";
        else if (prio > 23) StrPriority = " New ITUZ ";
        else if (prio > 19) StrPriority = " New DXCC ";
        else if (prio == 19 ||  prio == 4) StrPriority = " Wanted Call ";
        else if (prio == 18 ||  prio == 3) StrPriority = " Wanted Prefix ";
        else if (prio == 17 ||  prio == 2) StrPriority = " Wanted Country ";
        if (m_status > QsoHistory::RREPORT) StrPriority += " Resume interrupted QSO ";
      }
      writeToALLTXT("hisCall:" + hisCall + "mode:" + mode + StrPriority + " time:" + QString::number(time) +  " autoselect: " + StrDirection + " status: " + StrStatus[m_status] + " count: " + QString::number(count)+ " prio: " + QString::number(prio));
    }
    beforeAutoselectPick (hisCall);   /* CE3TSK: the policy may reject this candidate */
    /* CE3TSK: contest - and never pick a station that answers the exchange with a signal
       report; he is not in the contest (contestReportAbort), five minute window */
    if (!hisCall.isEmpty () && m_wwDigi
        && m_contestIgnore.has (Radio::base_callsign (hisCall), m_jtdxtime->currentMSecsSinceEpoch2 ())) {
      if(m_config.write_decoded_debug()) writeToALLTXT("contest: " + hisCall + " skipped, he answers with a signal report");
      hisCall.clear ();
      m_status = QsoHistory::NONE;
    }
    afterAutoselectPick (qsoJustFinished, hisCall, rx, prio, StrStatus);   /* CE3TSK */
    if (!hisCall.isEmpty ()) {
      if (m_callToClipboard) clipboard->setText(hisCall);
      ui->dxCallEntry->setText(hisCall);
      if(m_mode=="JT9+JT65" && m_modeTx != mode) {
      m_modeTx = mode;
      if (m_modeTx == "JT9") ui->pbTxMode->setText("Tx JT9  @");
      else ui->pbTxMode->setText("Tx JT65  #");
      m_wideGraph->setModeTx(m_modeTx);
      ui->TxFreqSpinBox->setValue (rx);
      }
      if (!rpt.isEmpty () && rpt == m_rpt) m_rpt = "-60";
    } else  if (m_transmittedQSOProgress != CALLING){
        on_txb6_clicked();
        if(ui->tabWidget->currentIndex()==1) ui->genMsg->setText(ui->tx6->text());
    }
  }
//  printf("%s(%0.1f) process_Auto: %s,%s,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),m_jtdxtime->GetOffset(),m_hisCall.toStdString().c_str(),hisCall.toStdString().c_str(),m_lastloggedcall.toStdString().c_str(),mode.toStdString().c_str(),m_status,prio,ui->TxFreqSpinBox->value (),m_used_freq,m_callMode,m_callPrioCQ,m_reply_other,m_reply_me,counters2);

  if (rx > 0 && rx != ui->RxFreqSpinBox->value ()) ui->RxFreqSpinBox->setValue (rx);
  //if (tx > 0 && tx != ui->TxFreqSpinBox->value ()) ui->TxFreqSpinBox->setValue (tx);
  if (m_status > QsoHistory::NONE){
    if(!grid.isEmpty() && m_hisGrid.left(4) != grid) {
       ui->dxGridEntry->setText(grid);
    }
    if (time < 86400 && m_status < QsoHistory::R73) {
      m_dateTimeQSOOn = m_jtdxtime->currentDateTimeUtc2();
      m_dateTimeQSOOn.setTime(QTime::fromMSecsSinceStartOfDay(time*1000));
      if (m_jtdxtime->currentDateTimeUtc2() < m_dateTimeQSOOn) m_dateTimeQSOOn = m_dateTimeQSOOn.addSecs(-86400);
    }
    /* CE3TSK: WW Digi contest - rpt carries the 4 character grid exchange. Pushing it into
       the report spin box would set the report to 0, toInt() of a grid is 0. */
    bool rptIsExchange = false;
    if (m_wwDigi && !rpt.isEmpty ()) { bool numeric=false; rpt.toInt(&numeric); rptIsExchange = !numeric; }
    if (!rpt.isEmpty () && rpt != m_rpt && !rptIsExchange) {
      ui->rptSpinBox->setValue(rpt.toInt());
      if (m_status == QsoHistory::SREPORT) {
        if (m_skipTx1 && !m_houndMode) { m_status=QsoHistory::RCQ; }
        else { m_status=QsoHistory::RREPORT; }
      }
      genStdMsgs(rpt);
    }
    switch (m_status) {
      case QsoHistory::RFIN:
      case QsoHistory::RCQ: {
        if (m_skipTx1 && !m_houndMode) {
          on_txb2_clicked();
          if(ui->tabWidget->currentIndex()==1) { ui->genMsg->setText(ui->tx2->text()); m_ntx=7; } }
        else {
          on_txb1_clicked();
          if(ui->tabWidget->currentIndex()==1) { ui->genMsg->setText(ui->tx1->text()); m_ntx=7; } }
        break;
      }
      case QsoHistory::RCALL: {
        if(m_enableTx && m_autoseq && m_callMode>0) txwatchdog (false);
        /* CE3TSK: WW Digi contest - his call carries his exchange already, so acknowledge it
           with Tx3 "HISCALL MYCALL R MYGRID" instead of repeating a bare exchange with Tx2.
           That produces the published 4 message contest sequence. Only when his grid is
           known, otherwise there is nothing to roger yet. */
        if(m_wwDigi && !m_hisGrid.isEmpty()) {
          on_txb3_clicked();
          if(ui->tabWidget->currentIndex()==1) ui->genMsg->setText(ui->tx3->text());
        } else {
        on_txb2_clicked();
        if(ui->tabWidget->currentIndex()==1) ui->genMsg->setText(ui->tx2->text());
        }
        if(m_autofilter && m_enableTx && !m_filter) autoFilter (true);
        break;
      }
      case QsoHistory::RREPORT: {
        if(m_enableTx && m_autoseq && m_callMode>0) txwatchdog (false);
        if(m_autofilter && m_enableTx && !m_filter) autoFilter (true);
        on_txb3_clicked();
        if(ui->tabWidget->currentIndex()==1) ui->genMsg->setText(ui->tx3->text());
        break;
      }
      case QsoHistory::RRREPORT: {
        on_txb4_clicked();
        if(ui->tabWidget->currentIndex()==1) ui->genMsg->setText(ui->tx4->text());
        break;
      }
      case QsoHistory::RRR: {
        on_txb5_clicked();
        if(ui->tabWidget->currentIndex()==1) ui->genMsg->setText(ui->tx5->currentText());
        break;
      }
      case QsoHistory::RRR73: {
        if(!m_houndMode) { on_txb5_clicked(); if(ui->tabWidget->currentIndex()==1) ui->genMsg->setText(ui->tx5->currentText()); }
        else { 
          auto curtime=m_jtdxtime->currentDateTimeUtc2();
          if(m_lastloggedcall!=m_hisCall || qAbs(curtime.toMSecsSinceEpoch()-m_lastloggedtime.toMSecsSinceEpoch()) > int(m_TRperiod) * 7000) {
            m_logqso73=true;
            logQSOTimer.start (0);
          }
          if(m_enableTx || m_transmitting || m_btxok || g_iptt==1) haltTx("end of DXpedition QSO ");
        }
        break;
      }
      /* CE3TSK: the SRR73/S73/FIN cases below are the fallback to CQ. They are reached only
         when the post-QSO hook above did not deal with the pass (sequencer_hooks.cpp);
         otherwise the QSO was cleared there and m_status is NONE by now. */
      case QsoHistory::SRR73: {
        if (!m_singleshot && !m_config.autolog() && m_lastloggedcall == m_hisCall)
          endOfQsoStopTx("SRR73, none received ");
        break;
      }
      case QsoHistory::R73: {
        on_txb5_clicked();
        if(ui->tabWidget->currentIndex()==1) ui->genMsg->setText(ui->tx5->currentText());
        break;
      }
      case QsoHistory::S73: {
//        if (!m_singleshot && !m_config.autolog() && m_lastloggedcall == m_hisCall)
          endOfQsoStopTx("S73, none received ");
        break;
      }
      case QsoHistory::FIN: {
        if (m_singleshot)
          endOfQsoStopTx("FIN, end of QSO, Singleshot ");
        else if (m_config.autolog())
          endOfQsoStopTx("FIN, end of QSO, Autolog ");
        else if (m_lastloggedcall != m_hisCall)
          endOfQsoStopTx("FIN, end of QSO, Call not logged ");
        else
          endOfQsoStopTx("FIN, end of QSO, not owner of the frequency ");
        break;
      }
      default: {
        break;
      }
    }
  } else {
    if (m_enableTx && m_hisCall.isEmpty()) ui->RxFreqSpinBox->setValue (ui->TxFreqSpinBox->value ());
    if (!counters) {
       if(m_singleshot) { endOfQsoStopTx("m_singleshot, counter triggered "); }
       else if(m_houndMode) { endOfQsoStopTx("m_houndMode, counter triggered "); }
    }
  }
}

void MainWindow::readFromStdout()                             //readFromStdout
{
  while(proc_jtdxjt9.canReadLine()) {
    QByteArray t=proc_jtdxjt9.readLine();
    // CE3TSK: pipeline ensemble - the background phase's closing line: count, time, whether
    // the next decode cut it short. The decodes themselves arrived as ordinary lines.
    if(t.startsWith("<BackgroundFinished>")) {
      int units=t.mid(t.indexOf("<units>")+7,3).trimmed().toInt();
      int msgs=t.mid(t.indexOf("<msgs>")+6,4).trimmed().toInt();
      QString secs=t.mid(t.indexOf("<secs>")+6,6).trimmed();
      bool aborted=t.mid(t.indexOf("<abort>")+7,1)=="1";
      m_bgPhase=false; m_bgRan=true; m_blankLine=true;
      m_bgCut=aborted && !m_bgAbortAsked; m_bgAbortAsked=false;   // item 81: cut short by the next decode (not by our own band/mode abort): the label shows X
      m_bgLastCut=m_bgCut; m_bgLastKnown=true; updateTimingLamps();   // CE3TSK: the TX lamp holds this until the next background finishes
      updateDecodeLabel();
      if(m_config.write_decoded_debug()) writeToALLTXT(QString("Background finished: %1 units, %2 messages, %3 s%4").arg(units).arg(msgs).arg(secs).arg(aborted ? ", cut short by the next decode" : ""));
      continue;
    }
    if(t.startsWith("<DecodeFinished>")) {
      dec_data.params.nstophint=1;
      m_bDecoded = t.mid (20).trimmed ().toInt () > 0;
      int mswait=750.0*m_TRperiod;
      if(!m_diskData) killFileTimer.start (mswait); //Kill in 3/4 of period
    // autoseq guard frequency band
      if(m_modeTx == "FT8") m_nguardfreq = 51;
      else if(m_modeTx == "FT4") m_nguardfreq = 84;
      else if(m_modeTx == "FT2") m_nguardfreq = 168;   // CE3TSK: 4 tones at 41.667 Hz, twice FT4's width
      else if(m_modeTx == "JT65") m_nguardfreq = 176;
      else if(m_modeTx == "JT9") m_nguardfreq = 16;
      else if(m_modeTx == "T10") m_nguardfreq = 67;
      int indxncand=t.indexOf("<ncand>")+7; m_ncand=t.mid(indxncand,5).toInt();
      int indxdt=t.indexOf("<avexdt>")+8; QString avexdt = t.mid(indxdt,6).trimmed();
//  printf("%s(%0.1f) Timing decode stop, %d candidates\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),m_jtdxtime->GetOffset(),m_ncand);
      if (m_autoseq && !m_processAuto_done && !m_manualDecode) { m_processAuto_done = true; process_Auto(); }
      m_okToPost=true;
      m_RxLog=0;
      m_startAnother=m_loopall;
      m_blankLine=true;
      m_notified=false;
      if(m_config.write_decoded_debug()) {
        QString rxm{""};
        if((m_mode=="FT8" && m_ft8EnsembleEffort==ENSEMBLE_BUDGET) || (m_mode=="FT4" && m_ft4Ensemble==FT4_ENSEMBLE_BUDGET)
           || (m_mode=="FT2" && m_ft2Recipe.members==FT4_ENSEMBLE_BUDGET)) {   // CE3TSK step 5: FT2's max effort uses it too
          int i=t.indexOf("<rxm>"); if(i>=0) rxm=" - RX budget auto: "+t.mid(i+5,3).trimmed()+" member(s)"; }   // CE3TSK P8; item 80 FT4
        writeToALLTXT("Decoding finished"+rxm);
      }
      QString slag="";
      qint64 msDecFin=m_jtdxtime->currentMSecsSinceEpoch2();
      if(!m_manualDecode && !m_diskData) {
        qint64 periodms=m_TRperiod*1000;
        qint64 lagms=msDecFin - periodms*((m_msDecStarted / periodms)+1); // rounding to base int
        float lag=lagms/1000.0; slag.setNum(lag,'f',2); if(lag > 0.0) slag="+"+slag;
      }
      if(m_diskData && m_mode.startsWith("FT")) { // estimated lag for FT4||FT8 wav file
        qint64 lagms=msDecFin-m_msDecStarted;
        if(m_mode=="FT8") {
          if(m_swl) lagms-=300; 
          else if(m_FT8EarlyStart) lagms-=1200;
          else lagms-=900;
        } else {
          lagms-=1430;	
        }
        float lag=lagms/1000.0; slag.setNum(lag,'f',2); if(lag > 0.0) slag="+"+slag;
      }
      int navexdt=qAbs(100.*avexdt.toFloat());
      if(m_mode.startsWith("FT")) {
        m_decodeLabelPrefix="UTC     dB   DT "+tr("Freq  ")+" "+tr("Avg="); m_decodeAvg=avexdt; m_decodeLabelMid=tr("Lag="); m_decodeLag=slag;
        if(!slag.isEmpty()) { m_rxLag=slag.toDouble(); m_rxLagKnown=true; }   /* CE3TSK: the RX lamp reads the same figure; a manual decode prints none and leaves the lamp as it was */   // CE3TSK: coloured parts follow
        /* CE3TSK: FT4 has a TX background of its own since item 59, and this line still tested
           for FT8 alone - so m_bgPhase stayed false in FT4 and the two places that consult it
           behaved as if no background existed. The spacer was printed BEFORE FT4's late
           background decodes (the `!m_bgPhase` guard below let it through, and the
           `if(m_bgPhase) m_blankLine=false` under <DecodeFinished> never fired), and again
           before the next period's RX decodes - so the late decodes were separated from the
           period they belong to. FT8 puts them under that period's spacer, which is right:
           they are that period's audio, decoded late. */
        m_rxPhase=false; m_nDecodesRx=m_nDecodes;
        /* item 78: both modes alike - the switch alone decides whether a background phase (and its
           <BackgroundFinished> line, which re-arms the spacer) will come. Until item 78 the FT4
           decoder ran the phase only when the member target exceeded the RX count, and this
           flag had to copy that rule or every later separator was swallowed. */
        m_bgPhase=((m_mode=="FT8" && m_bgEnabled) || (m_mode=="FT4" && m_ft4BgEnabled)
                   || (m_mode=="FT2" && m_ft2Recipe.bgon));   // CE3TSK step 5
        updateDecodeLabel();
        updateTimingLamps();   // CE3TSK
        if(m_mode=="FT8") {
          if(!m_lostaudio) {
            if(navexdt<76) setBandLabelColour ("#fdedc5");
            else if(navexdt>75 && navexdt<151) setBandLabelColour ("#ffff00");
            else if(navexdt>150) setBandLabelColour ("#ff8000");
            if(navexdt>75) ui->label_6->setText(tr("check time"));
            else  ui->label_6->setText(tr("Band"));
          }
          else m_lostaudio=false;
          if (ui->syncButton->isChecked()) {
            if (navexdt > 19) m_jtdxtime->SetOffset(m_jtdxtime->GetOffset() - avexdt.toFloat());
            ui->syncButton->setChecked(false);
          } else if (!ui->syncButton->isEnabled()) ui->syncButton->setEnabled(true);
        }
        else if (m_mode=="FT4") {
          if(navexdt<41) setBandLabelColour ("#fdedc5");
          else if(navexdt>40 && navexdt<81) setBandLabelColour ("#ffff00");
          else if(navexdt>80) setBandLabelColour ("#ff8000");
          if(navexdt>40) ui->label_6->setText(tr("check time"));
          else  ui->label_6->setText(tr("Band"));
        }
        else if (m_mode=="FT2") {   // CE3TSK: half FT4's thresholds, for half its period
          if(navexdt<21) setBandLabelColour ("#fdedc5");
          else if(navexdt>20 && navexdt<41) setBandLabelColour ("#ffff00");
          else if(navexdt>40) setBandLabelColour ("#ff8000");
          if(navexdt>20) ui->label_6->setText(tr("check time"));
          else  ui->label_6->setText(tr("Band"));
        }
      }
      else { ui->decodedTextLabel->setTextFormat(Qt::PlainText); ui->decodedTextLabel->setText("UTC     dB   DT "+tr("Freq  ")+" "+tr("Lag=")+slag); }
      dec_data.params.nagain=0; dec_data.params.nagainfil=0; dec_data.params.ndiskdat=0;
      m_manualDecode=false; ui->DecodeButton->setChecked (false);
      QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.open(QIODevice::ReadWrite);
      // CE3TSK: pipeline ensemble - late lines follow; keep them under this period's spacer
      m_rxPhase=false; if(m_bgPhase) m_blankLine=false;
      decodeBusy(false); // shall be last line
      return;
    } else {
      /* CE3TSK: WW Digi - a message ending in a signal report is not contest traffic (the
         exchange is the grid) and is as likely a false decode as not; dropped here, before
         ALL.TXT, both windows, the UDP clients and the sequencer (contestreport.h). */
      if(m_wwDigi && contest_report_message (QString::fromUtf8 (t))) {   /* UTF-8: the decoder's markers include the multibyte ┼ */
        if(m_config.write_decoded_debug()) writeToALLTXT("contest: report message rejected " + QString::fromUtf8 (t).trimmed());
        /* the two branches below decide: drop it (the usual case), or let it through because it
           is our partner acknowledging and we already have his exchange */
        /* CE3TSK: a report addressed to us needs one distinction, learned the hard way on
           2026-08-30. 3D2USU (Fiji, RH82 - a new field) called CQ *with his grid*, we answered,
           he acknowledged with "CE3TSK 3D2USU +05" because his software runs reports, and this
           filter swallowed it: the sequencer never saw the acknowledgement, kept resending Tx2,
           and the rule then dropped him and skipped him for five minutes, silently. Two other
           stations completed with him in those same minutes.

           His grid was already ours, from his CQ - so the exchange existed and the QSO was worth
           finishing. The deadlock item 26 was written for is the case where we have *no*
           exchange from him at all. So: if he is the station we are working and we already hold
           his grid, let the line through to the ordinary path, where it advances the QSO the way
           his R+grid would. Otherwise drop it and skip him, as before. */
        QString const reportToUs = contest_report_to_me (QString::fromUtf8 (t), m_baseCall);
        if(!reportToUs.isEmpty ()) {
          QString hisKnownGrid;
          m_qsoHistory.status (reportToUs, hisKnownGrid);
          bool const working = !m_hisCall.isEmpty ()
              && Radio::base_callsign (m_hisCall) == Radio::base_callsign (reportToUs);
          if(working && gridOK (hisKnownGrid.left (4))) {
            if(!m_contestReportSeen.contains (Radio::base_callsign (reportToUs))) {
              m_contestReportSeen << Radio::base_callsign (reportToUs);
              QString const why = reportToUs + " answers with reports, not the grid exchange - his grid "
                                  + hisKnownGrid.left (4) + " is already known, so the QSO goes on";
              ui->decodedTextBrowser->displayContestNotice (why);
              ui->decodedTextBrowser2->displayContestNotice (why);
              writeToALLTXT ("contest: " + why);
            }
            /* fall through: the line is displayed and drives the sequencer */
          } else {
            contestReportAbort (reportToUs);
            continue;
          }
        } else continue;
      }
      if(t.indexOf(m_baseCall.toLatin1()) >= 0 || m_config.write_decoded() || m_config.write_decoded_debug()) {
        QFile f {m_dataDir.absoluteFilePath (m_jtdxtime->currentDateTimeUtc2().toString("yyyyMM_")+"ALL.TXT")};
        if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
          QTextStream out(&f);
          if (m_RxLog==1) {
            out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss")
                << "  " << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6) << " MHz  "
                << m_mode << 
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

            m_RxLog=0;
          }
          out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_") << t.trimmed() << 
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

          f.close();
        } else {
          JTDXMessageBox::warning_message (this, "", tr ("File Open Error")
                                       , tr ("Cannot open \"%1\" for append: %2")
                                       .arg (f.fileName ()).arg (f.errorString ()));
        }
      }

        if (m_config.insert_blank () && m_blankLine && !m_bgPhase)   // CE3TSK: no new spacer for background decodes
          {
            QString band;
            if (m_jtdxtime->currentMSecsSinceEpoch2() / 1000 - m_secBandChanged > 50 
			|| (m_jtdxtime->currentMSecsSinceEpoch2() / 1000 - m_secBandChanged > 14 && m_mode == "FT8")
			|| (m_jtdxtime->currentMSecsSinceEpoch2() / 1000 - m_secBandChanged > 6 && m_mode == "FT4")
			|| (m_jtdxtime->currentMSecsSinceEpoch2() / 1000 - m_secBandChanged > 3 && m_mode == "FT2"))   // CE3TSK
              {
                band = ' ' + m_config.bands ()->find (m_freqNominal) + ' ';
              }
              
            QString blnklinetime;
            if (m_mode.startsWith("FT")) {
				blnklinetime = m_jtdxtime->currentDateTimeUtc2().toString("dd.MM.yy hh:mm:ss' UTC ----'");
            } else {
				blnklinetime = m_jtdxtime->currentDateTimeUtc2().toString("dd.MM.yy hh:mm' UTC '");
            }
            if (!m_diskData) {
                    ui->decodedTextBrowser->insertLineSpacer ("----- " + blnklinetime + band.rightJustified (13, '-') + "----");
            } else {
                    QString wavfile = " wav file "; 
                    ui->decodedTextBrowser->insertLineSpacer ("----- " + blnklinetime + wavfile.rightJustified (17, '-') + "----");
            }
            m_blankLine = false;
          }
 
       if (!m_notified && m_config.beepOnFirstMsg () && !m_diskData) {
          if (m_windowPopup) {
			 this->showNormal();
			 this->raise();
			 QApplication::setActiveWindow(this);
          }
          QApplication::beep();
          m_notified=true;
       }
	   
      DecodedText decodedtext {QString::fromUtf8 (t.constData ()).remove (QRegularExpression {"\r|\n"}),this};
//      DecodedText decodedtext {"161545  -4  0.1 1939 & CQ RT9K/4    ",this};
	  QString tcut = t.replace("\n","");
	  if (!m_mode.startsWith("FT")) {
		tcut = tcut.remove(0,21);
	  } else {
		tcut = tcut.remove(0,23);
	  }
      if (!decodedtext.isDebug()) m_nDecodes ++;
      if(m_rxPhase) m_nDecodesRx=m_nDecodes;
      if(m_mode=="FT8") updateDecodeLabel();   // CE3TSK: "/D-" while decoding, "/D+N=S|" in the background
      auto decodedtextmsg = decodedtext.message();
      bool mycallinmsg = false;
      if (!m_baseCall.isEmpty () && Radio::base_callsign (decodedtext.call()) == m_baseCall) mycallinmsg = true;

      // extract de
      QString deCall="";
      QString grid="";
      decodedtext.deCallAndGrid(/*out*/deCall,grid);
      if (!m_hisCall.isEmpty() && !deCall.isEmpty() && Radio::base_callsign (m_hisCall) == Radio::base_callsign (deCall)) ui->RxFreqSpinBox->setValue (decodedtext.frequencyOffset());
      if (!deCall.isEmpty() && !m_reply_me && Radio::base_callsign (deCall) == Radio::base_callsign (m_hisCall)) {
          if (mycallinmsg) {
             m_reply_me = true;
             m_reply_other = false;
          } else if (decodedtextmsg.contains ("73") || decodedtextmsg.left(2) == "CQ") {
             m_reply_other = false;
             m_reply_CQ73 = true;
          } else if (!m_reply_CQ73) m_reply_other = true;
      }      
      //Left (Band activity) window
      bool bypassRxfFilters = false;
      int notified = 2;
      notified = ui->decodedTextBrowser->displayDecodedText (&decodedtext
                                                    , m_baseCall
                                                    , Radio::base_callsign (m_hisCall)
                                                    , m_hisGrid.left(4)
                                                    , m_notified
                                                    , (m_wwDigi ? m_contestLog : m_logBook)
                                                    , m_qsoHistory
                                                    , m_qsoHistory
                                                    , m_freqNominal
                                                    , m_mode
                                                    , bypassRxfFilters
                                                    , m_bypassAllFilters
                                                    , m_wideGraph->rxFreq()
                                                    , m_wantedCallList
                                                    , m_wantedPrefixList
                                                    , m_wantedGridList
                                                    , m_wantedCountryList
                                                    , m_windowPopup
                                                    , this
                                                    );
      if(m_position != 0) ui->decodedTextBrowser->horizontalScrollBar()->setValue(m_position);
	  if (notified & 1) m_notified = true;

      //Right (Rx Frequency) window
	  
	  // content free messages to (Rx Frequency) window functionality
	  bool bcontent = false;
	  if (m_config.enableContent ()) {
		if (tcut.contains ("/")) {
			QStringList slcontent = m_config.content ().split(",");
			foreach (QString item, slcontent) {
				if (item.length() > 2 && item.length() < 7) {
					if (tcut.contains ("/" + item)) {
						bcontent = true;
						break;
					}
				}
			}
		}
	  }
	  
	  // DXCall 73/RR73 message to RX frequency window in scenario where Enable TX is turned off
	  bool bdxcall73 = false;
	  QStringList message_words = words_re.match (tcut).capturedTexts ();
	  if(message_words.length () == 4 && !m_enableTx) {
	    QString dxbasecall = Radio::base_callsign (m_hisCall);
	    if (!dxbasecall.isEmpty () && message_words.at (2).contains (dxbasecall) && message_words.at (3).contains ("73")) bdxcall73 = true;
	  }
	  
      if (qAbs(decodedtext.frequencyOffset() - m_wideGraph->rxFreq()) <= 10 || (m_showMyCallMsgRxWindow && mycallinmsg) || bcontent || bdxcall73 || (m_showWantedCallRxWindow && notified & 8)) {

        if(m_bypassRxfFilters || dec_data.params.nagain==1 || dec_data.params.nagainfil==1) 
			bypassRxfFilters = true;

        // This msg is within 10 hertz of our tuned frequency



        ui->decodedTextBrowser2->displayDecodedText(&decodedtext
                                                    , m_baseCall
                                                    , Radio::base_callsign (m_hisCall)
                                                    , m_hisGrid.left(4)
                                                    , m_notified
                                                    , (m_wwDigi ? m_contestLog : m_logBook)
                                                    , m_qsoHistory2
                                                    , m_qsoHistory
                                                    , m_freqNominal
                                                    , m_mode
                                                    , bypassRxfFilters
                                                    , m_bypassAllFilters
                                                    , m_wideGraph->rxFreq());


        m_QSOText=decodedtext.string();
		bcontent = false;
      }

      if (mycallinmsg && !m_manualDecode) {
         if (!deCall.isEmpty() && Radio::base_callsign (deCall) == Radio::base_callsign (m_hisCall)) {
           if (!m_processAuto_done && m_autoseq && ((!decodedtextmsg.contains(" 73") && !decodedtextmsg.contains("RR73")) 
           || (decodedtextmsg.contains(" 73") && m_status==QsoHistory::RRREPORT && m_rrr) || (decodedtextmsg.contains("RR73") && m_status==QsoHistory::RREPORT) || m_callMode==0 || m_singleshot || m_houndMode)) {
             m_processAuto_done = true;
             process_Auto();
           } else if ((decodedtextmsg.contains(" 73") || decodedtextmsg.contains("RR73")) && m_callMode<=1) m_callFirst73 = true;
         } else if (m_callMode==1 && !m_processAuto_done && m_autoseq && m_hisCall.isEmpty()) {
           m_processAuto_done = true;
           process_Auto();
         }
      } else if (!deCall.isEmpty() && Radio::base_callsign (deCall) == Radio::base_callsign (m_hisCall) && decodedtextmsg.left(3) != "CQ " && decodedtextmsg.left(3) != "DE " && decodedtextmsg.left(4) != "QRZ " && !decodedtextmsg.contains(" 73") && !decodedtextmsg.contains(" RR73") && !decodedtextmsg.contains(" RRR")) {
        m_used_freq = decodedtext.frequencyOffset();
         if (haltTxWhenFrequencyTaken ()) {
           haltTx("readFromStdout, not owner of the frequency or reply to other ");/* if(m_skipTx1) m_qsoHistory.remove(m_hisCall); */
         }
      }

      if((!m_config.prevent_spotting_false () || (m_config.prevent_spotting_false () && !decodedtext.isWrong ()))
         && (!m_config.filterUDP () || (m_config.filterUDP () && notified & 2))) {
         postDecode (true, decodedtext.string ());
      }
      // find and extract any report for myCall
      QString rpt_type;
      bool stdMsg = decodedtext.report(m_baseCall,
          Radio::base_callsign(m_hisCall), m_rptRcvd,rpt_type,m_wwDigi);
      // extract details and send to PSKreporter
      if(m_okToPost and m_config.spot_to_psk_reporter () and stdMsg and !m_diskData) {
        QString msgmode="FT8";
        if (m_mode=="FT4") msgmode="FT4";
        else if (m_mode=="FT2") msgmode="FT2";   // CE3TSK
        else if (decodedtext.isJT65()) msgmode="JT65";
        else if (m_mode.startsWith("JT9")) msgmode="JT9";
        else if (m_mode=="T10") msgmode="T10";

        int audioFrequency = decodedtext.frequencyOffset();
        int snr = decodedtext.snr();
        Frequency frequency = m_freqNominal + audioFrequency;
        pskSetLocal ();
        if(gridOK(grid) && !gridRR73(grid) && !decodedtext.isHint() && !decodedtext.isWrong())
          {
            // qDebug() << "To PSKreporter:" << deCall << grid << frequency << msgmode << snr;
            psk_Reporter->addRemoteStation(deCall,grid,QString::number(frequency),msgmode,
                                           QString::number(snr),
                                           QString::number(m_jtdxtime->currentDateTime2().toTime_t()));
          }
      }
    }
  }
}

void MainWindow::killFile ()
{
  if (m_fnameWE.size () &&
      !(m_saveWav==2 || (m_saveWav==1 && m_bDecoded) || m_fnameWE == m_fileToSave)) {
    QFile f1 {m_fnameWE + ".wav"};
    if(f1.exists()) f1.remove();
    if(m_mode.startsWith ("WSPR")) {
      QFile f2 {m_fnameWE + ".c2"};
      if(f2.exists()) f2.remove();
    }
  }
  initLogIfNeeded ();
}

/* CE3TSK: the log initialisation a changed wsjtx_log.adi or a data file download left pending.
   Called at the end of the period and when the operator stops monitoring, which has no period
   left to protect - without that a download made just before Stop waited until the next close. */
void MainWindow::initLogIfNeeded ()
{
  if(m_logInitNeeded) {
    printf("%s(%0.1f) Timing Log_init_needed\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),m_jtdxtime->GetOffset());
    if(m_config.write_decoded_debug()) writeToALLTXT(m_dataFilesChanged ? "Log initialization is started: cty.dat or the LoTW user list was downloaded" : "Log initialization is started: wsjtx_log.adi file was changed");
    m_logBook.init(m_config.callNotif() ? m_config.my_callsign() : "",m_config.gridNotif() ? m_config.my_grid() : "",m_config.timeFrom(),"wsjtx_log.adi",nullptr,m_config.countryNameTranslated());
    refreshContestLog(true);   /* CE3TSK: country data has just been read */
    countQSOs ();
    m_logInitNeeded=false;
    m_dataFilesChanged=false;
  }
}

void MainWindow::set_language (QString const& lang)
{
  if (m_lang != lang) {
    bool olek;
    QString tolge;
    QTranslator translator;
    olek = translator.load (QLocale(lang),"jtdx","_",":/Translations");
    if (!olek) olek = translator.load (QString {"jtdx_"} + lang);
    JTDXMessageBox msgbox;
    msgbox.setWindowTitle(tr("Confirm change Language"));
    msgbox.setIcon(JTDXMessageBox::Question);
    msgbox.setText(tr("Are You sure to change UI Language to English? JTDX will close, please start it again."));
    msgbox.setStandardButtons(JTDXMessageBox::Yes | JTDXMessageBox::No);
    msgbox.setDefaultButton(JTDXMessageBox::No);
    if (olek) {
      tolge = translator.translate("MainWindow","Confirm change Language");
      if (!tolge.isEmpty()) msgbox.setWindowTitle(tolge);
      else msgbox.setWindowTitle("Confirm change Language");
      tolge = translator.translate("MainWindow","Are You sure to change UI Language to English? JTDX will close, please start it again.");
      if (!tolge.isEmpty()) msgbox.setText(tolge);
      else msgbox.setText("Are You sure to change UI Language to English? JTDX will close, please start it again.");
      tolge = translator.translate("JTDXMessageBox","&Yes");
      if (!tolge.isEmpty()) msgbox.button(JTDXMessageBox::Yes)->setText(tolge);
      else msgbox.button(JTDXMessageBox::Yes)->setText("&Yes");
      tolge = translator.translate("JTDXMessageBox","&No");
      if (!tolge.isEmpty()) msgbox.button(JTDXMessageBox::No)->setText(tolge);
      else msgbox.button(JTDXMessageBox::No)->setText("&No");
    }
    if(msgbox.exec() == JTDXMessageBox::Yes) {
            /* CE3TSK: the chosen language is persisted by writeSettings() from closeEvent();
               closing normally is all that is needed - the operator restarts the program. */
            m_lang = lang;
            QMainWindow::close();
    }
  }
  if(m_lang=="et_EE") ui->actionEstonian->setChecked(true);
  else if(m_lang=="ru_RU") ui->actionRussian->setChecked(true);
  else if(m_lang=="ca_ES") ui->actionCatalan->setChecked(true);
  else if(m_lang=="hr_HR") ui->actionCroatian->setChecked(true);
  else if(m_lang=="da_DK") ui->actionDanish->setChecked(true);
  else if(m_lang=="nl_NL") ui->actionDutch->setChecked(true);
  else if(m_lang=="es_ES") ui->actionSpanish->setChecked(true);
  else if(m_lang=="fr_FR") ui->actionFrench->setChecked(true);
  else if(m_lang=="hu_HU") ui->actionHungarian->setChecked(true);
  else if(m_lang=="it_IT") ui->actionItalian->setChecked(true);
  else if(m_lang=="de_DE") ui->actionGerman->setChecked(true);   // CE3TSK
  else if(m_lang=="lv_LV") ui->actionLatvian->setChecked(true);
  else if(m_lang=="pl_PL") ui->actionPolish->setChecked(true);
  else if(m_lang=="pt_PT") ui->actionPortuguese->setChecked(true);
  else if(m_lang=="pt_BR") ui->actionPortuguese_BR->setChecked(true);
  else if(m_lang=="sv_SE") ui->actionSwedish->setChecked(true);
  else if(m_lang=="zh_CN") ui->actionChinese_simplified->setChecked(true);
  else if(m_lang=="zh_HK") ui->actionChinese_traditional->setChecked(true);
  else if(m_lang=="ja_JP") ui->actionJapanese->setChecked(true);
  else if(m_lang=="ko_KR") ui->actionKorean->setChecked(true);   // CE3TSK
  else ui->actionEnglish->setChecked(true);
}

void MainWindow::on_EraseButton_clicked()                          //Erase
{
  qint64 ms=m_jtdxtime->currentMSecsSinceEpoch2();
  ui->decodedTextBrowser->clear();
  if(m_mode.left(4)=="WSPR") {
    ui->decodedTextBrowser->clear();
  } else {
    m_QSOText.clear();
    m_messageClient->clear_decodes ();
//    QFile f(m_config.temp_dir ().absoluteFilePath ("decoded.txt"));
//    if(f.exists()) f.remove();
    if((ms-m_msErase)<500) ui->decodedTextBrowser2->clear();
  }
  m_msErase=ms;
}

void MainWindow::on_ClearDxButton_clicked() { clearDX (" is cleared by ClearDxButton, user action"); } //Erase

void MainWindow::decodeBusy(bool b)                             //decodeBusy()
{
  m_decoderBusy=b; //shall be first line
  if (b && m_firstDecode < 65 && ("JT65" == m_mode || "JT9+JT65" == m_mode)) {
      m_firstDecode += 65;
      if ("JT9+JT65" == m_mode) m_firstDecode = 65 + 9;
  }
  if (b && m_firstDecode != 9 && m_firstDecode != 65 + 9 && ("JT9" == m_mode)) m_firstDecode += 9;
  if (!b)  m_optimizingProgress.reset ();
  ui->DecodeButton->setEnabled(!b);
  ui->actionOpen->setEnabled(!b);
  ui->actionOpen_next_in_directory->setEnabled(!b);
  ui->actionDecode_remaining_files_in_directory->setEnabled(!b);
  statusUpdate ();
}

//------------------------------------------------------------- //guiUpdate()
void MainWindow::guiUpdate()
{
  static int iptt0=0;
  static bool btxok0=false;
  static char message[38];
  static char msgsent[38];
  static double onAirFreq0=0.0;
  double txDuration;

  if(m_TRperiod==0.0) m_TRperiod=60.0;
  txDuration=0.0;
  if(m_modeTx=="FT8") txDuration=13.64; //1.0 + 79*1920/12000.0;
  else if(m_modeTx=="FT4")  txDuration=6.04; //1.0 + 105*576/12000.0;
  else if(m_modeTx=="FT2")  txDuration=3.02; //0.5 + 105*288/12000.0 - CE3TSK step 6: 2.52 s of signal in a 3.75 s period, so the margin is half a second, not FT4's whole one
  else if(m_modeTx=="JT65") txDuration=47.81142857142857; //1.0 + 126*4096/11025.0;
  else if(m_modeTx=="JT9") txDuration=49.96; //1.0 + 85.0*m_nsps/12000.0;
  else if(m_modeTx=="T10") txDuration=49.96; //1.0 + 85.0*m_nsps/12000.0;
  else if(m_mode=="WSPR-2") txDuration=112.592; // 2.0 + 162*8192/12000.0;

  double tx1=0.0;
  double tx2=txDuration;
  if(m_mode.startsWith("FT")) icw[0]=0;                                   //No CW ID in FT8 mode
  if(icw[0]>0) tx2 += icw[0]*2560.0/48000.0;  //Full length including CW ID
  if(tx2>m_TRperiod) tx2=m_TRperiod;
  
  if(!m_txFirst and m_mode.left(4)!="WSPR") {
    tx1 += m_TRperiod;
    tx2 += m_TRperiod;
  }

  qint64 ms = m_jtdxtime->currentMSecsSinceEpoch2() % 86400000;
  int nsec=ms/1000;
  double tsec=0.001*ms;
  double t2p=fmod(tsec,2.0*m_TRperiod);
  m_nseq = fmod(double(nsec),m_TRperiod);

  if(m_mode.left(4)=="WSPR") {
    if(m_nseq==0 and m_ntr==0) {                   //Decide whether to Tx or Rx
      m_tuneup=false;                              //This is not an ATU tuneup
      if(m_pctx==0) m_WSPR_tx_next = false; //Don't transmit if m_pctx=0
      bool btx = m_enableTx && m_WSPR_tx_next; // To Tx, we need m_enableTx and
                                // scheduled transmit
      if(m_enableTx and m_txNext) btx=true;            //TxNext button overrides
      if(m_enableTx and m_pctx==100) btx=true;         //Always transmit

      if(btx) {
        m_ntr=-1;                          //This says we will have transmitted
        m_txNext=false;
        ui->pbTxNext->setChecked(false);
        m_bTxTime=true;                      //Start a WSPR Tx sequence
      } else {
// This will be a WSPR Rx sequence.
        m_ntr=1;                           //This says we will have received
        m_bTxTime=false;                     //Start a WSPR Rx sequence
      }
    }

  } else {
 // For all modes other than WSPR
    m_bTxTime = (t2p >= tx1) and (t2p < tx2);
  }
  if(m_tune) m_bTxTime=true;                 //"Tune" takes precedence

  if(m_transmitting or m_enableTx or m_tune) {
// Check for "txboth" (testing purposes only)
    QFile f(m_appDir + "/txboth");
    if(f.exists() and fmod(tsec,m_TRperiod)<49.96) m_bTxTime=true; //<(1.0 + 85.0*m_nsps/12000.0)

// Don't transmit another mode in the WSPR sub-band
    Frequency onAirFreq = m_freqNominal + ui->TxFreqSpinBox->value();
    auto mhz = onAirFreq / 1000000;
    Frequency f_from = 0;
    Frequency f_to = 0;
    switch (mhz)
    {
        case 1: {f_from = 1837930; f_to = 1838220; break;}
        case 3: {f_from = 3569930; f_to = 3570220; break;}
        case 5: {f_from = 5288530; f_to = 5288820; break;}
        case 7: {f_from = 7039930; f_to = 7040220; break;}
        case 10: {
            if (m_mode.contains("JT65")) f_from = 10139900; else f_from = 10140030;
            f_to = 10140320;
            break;}
        case 14: {f_from = 14096930; f_to = 14097220; break;}
        case 18: {f_from = 18105930; f_to = 18106220; break;}
        case 21: {f_from = 21095930; f_to = 21096220; break;}
        case 24: {f_from = 24925930; f_to = 24926220; break;}
        case 28: {f_from = 28125930; f_to = 28126220; break;}
        case 50: {f_from = 50294370; f_to = 50294620; break;}
        case 70: {f_from = 70092370; f_to = 70092620; break;}
        default: {f_from = 0;  f_to = 0; break;}
    }
    if ((f_from >0 and (onAirFreq > f_from and onAirFreq < f_to) and m_mode.left(4)!="WSPR") || (m_houndTXfreqJumps &&  ui->TxFreqSpinBox->value() < 1000 && ui->txrb1->isChecked())) {
      m_bTxTime=false;
//      if (m_tune) stop_tuning ();
      if (m_enableTx) enableTx_mode (false);
      if(onAirFreq!=onAirFreq0) {
        onAirFreq0=onAirFreq;
        if (onAirFreq > f_from and onAirFreq < f_to) {
          auto const& message1 = tr ("Please choose another Tx frequency."
                                  " JTDX will not knowingly transmit another"
                                  " mode in the WSPR sub-band.");
#if QT_VERSION >= 0x050400
          QTimer::singleShot (0, [=] { // don't block guiUpdate
            JTDXMessageBox::warning_message (this, "", tr ("WSPR Guard Band"), message1);
          });
#else
          JTDXMessageBox::warning_message (this, "", tr ("WSPR Guard Band"), message1);
#endif
        } else {
          auto const& message1 = tr ("Please choose another Tx frequency."
                                  " JTDX will not allow to Call below 1000 Hz"
                                  " in DXped mode.");
#if QT_VERSION >= 0x050400
          QTimer::singleShot (0, [=] { // don't block guiUpdate
            JTDXMessageBox::warning_message (this, "", tr ("FT8 F/H Tx Guard"), message1);
          });
#else
          JTDXMessageBox::warning_message (this, "", tr ("FT8 F/H Tx Guard"), message1);
#endif
        }
      }
    }

    float fTR=float((ms%int(1000.0*m_TRperiod)))/(1000.0*m_TRperiod);

    if (m_bTxTime && iptt0==0 && fTR<99.0 && m_autoseq && !m_processAuto_done && (m_callMode==2 || (m_callMode<=1 && (m_callFirst73 || m_status >= QsoHistory::RRR73)))) {
      m_processAuto_done = true;
      process_Auto();
    }
    if(g_iptt==0 and ((m_bTxTime and fTR<99.0) or m_tune )) {   //### Allow late starts
      icw[0]=m_ncw;
      g_iptt = 1;
      setRig ();
      setXIT (ui->TxFreqSpinBox->value ());
      Q_EMIT m_config.transceiver_ptt (true);       //Assert the PTT
      m_tx_when_ready = true;
    }
    if(!m_bTxTime and !m_tune) m_btxok=false;       //Time to stop transmitting
  }

  if(m_mode.left(4)=="WSPR" and
     ((m_ntr==1 and m_rxDone) or (m_ntr==-1 and m_nseq>tx2))) {
    if(m_monitoring) m_rxDone=false;
    if(m_transmitting) {
      WSPR_history(m_freqNominal,-1);
      m_bTxTime=false;                        //Time to stop a WSPR transmission
      m_btxok=false;
    }
    else if (m_ntr != -1) {
      WSPR_scheduling ();
      m_ntr=0;                                //This WSPR Rx sequence is complete
    }
  }

  bool haltedEmpty=false;
  // Calculate Tx tones when needed
  if((g_iptt==1 && iptt0==0) || m_restart) {
//----------------------------------------------------------------------
//    printf("%s(%0.1f) Timing transmission start %d %d\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),m_jtdxtime->GetOffset(),g_iptt,iptt0);
    QByteArray ba;
    QByteArray ba0;

    if(m_mode.left(4)=="WSPR") {
      QString sdBm,msg0,msg1,msg2;
      sdBm = QString::asprintf(" %d",m_dBm);
      m_tx=1-m_tx;
      int i2=m_config.my_callsign().indexOf("/");
      if(i2>0 or m_grid6) {
        if(i2<0) {                                                 // "Type 2" WSPR message
          msg1=m_config.my_callsign() + " " + m_config.my_grid().left(4) + sdBm;
        } else {
          msg1=m_config.my_callsign() + sdBm;
        }
        msg0="<" + m_config.my_callsign() + "> " + m_config.my_grid()+ sdBm;
        if(m_tx==0) msg2=msg0;
        if(m_tx==1) msg2=msg1;
      } else {
        msg2=m_config.my_callsign() + " " + m_config.my_grid().left(4) + sdBm; // Normal WSPR message
      }
      ba=msg2.toLatin1();
    } else {
      QString txMsg{""};
      if(m_ntx == 1) { txMsg=ui->tx1->text(); }
      else if(m_ntx == 2) { txMsg=ui->tx2->text(); }
      else if(m_ntx == 3) { txMsg=ui->tx3->text(); }
      else if(m_ntx == 4) { txMsg=ui->tx4->text(); }
      else if(m_ntx == 5) { txMsg=ui->tx5->currentText(); txMsg.remove(" ^"); }
      else if(m_ntx == 6) { txMsg=ui->tx6->text(); }
      else if(m_ntx == 7) { txMsg=ui->genMsg->text(); }
      else if(m_ntx == 8) { txMsg=ui->freeTextMsg->currentText(); txMsg.remove(" ^"); }
      int msgLength=txMsg.trimmed().length();
      if(!m_tune && (msgLength==0 || m_config.my_callsign().isEmpty() || (m_houndMode && (m_QSOProgress == CALLING || m_ntx == 6)))) { 
        haltedEmpty=true;
        if(m_houndMode) m_currentMessage=txMsg; else m_currentMessage.clear();
        if(msgLength==0) haltTx("Transmission of the empty message is not allowed ");
        else if(m_config.my_callsign().isEmpty()) haltTx("Transmission halted: user callsign is not configured ");
        else haltTx("Transmission of CQ message is not allowed in the Hound mode ");
      }
	  else { ba=txMsg.toLocal8Bit(); }
    }

    ba2msg(ba,message);
    int ichk=0;
    if (m_lastMessageSent != m_currentMessage
        || m_lastMessageType != m_currentMessageType)
      {
        m_lastMessageSent = m_currentMessage;
        m_lastMessageType = m_currentMessageType;
      }
    m_currentMessageType = 0;
    if(m_tune) { itone[0]=0; }
    else {
      int len1=22;
      if(m_modeTx=="FT8") {
        int i3=0; int n3=0; char ft8msgbits[77]; int ntxhash=1;
        genft8_(message,&i3,&n3,&ntxhash,msgsent,const_cast<char *> (ft8msgbits),const_cast<int *> (itone),37,37);
        int nsym=79; int nsps=4*1920; float fsample=48000.0; float bt=2.0; float f0=ui->TxFreqSpinBox->value() - m_XIT; int icmplx=0; int nwave=nsym*nsps;
        gen_ft8wave_(const_cast<int *>(itone),&nsym,&nsps,&bt,&fsample,&f0,foxcom_.wave,foxcom_.wave,&icmplx,&nwave);
      }
      else if(m_modeTx=="FT4") {
        int ichk=0; char ft4msgbits[77]; int ntxhash=1;
        genft4_(message, &ichk, &ntxhash, msgsent, const_cast<char *> (ft4msgbits),const_cast<int *>(itone),37,37);
        int nsym=103; int nsps=4*576; float fsample=48000.0; float f0=ui->TxFreqSpinBox->value() - m_XIT; int nwave=(nsym+2)*nsps; int icmplx=0;
        gen_ft4wave_(const_cast<int *>(itone),&nsym,&nsps,&fsample,&f0,foxcom_.wave,foxcom_.wave,&icmplx,&nwave);
      }
      /* CE3TSK step 6: FT2 transmits FT4's frame at twice the rate, so it is the SAME message
         coding and the SAME waveform generator - only the symbol length changes, 288 samples at
         12 kHz and so 4*288 at the 48 kHz transmit rate. 103 symbols plus the two ramp symbols
         gen_ft4wave adds is the 105 of the frame. */
      else if(m_modeTx=="FT2") {
        int ichk=0; char ft4msgbits[77]; int ntxhash=1;
        genft4_(message, &ichk, &ntxhash, msgsent, const_cast<char *> (ft4msgbits),const_cast<int *>(itone),37,37);
        int nsym=103; int nsps=4*288; float fsample=48000.0; float f0=ui->TxFreqSpinBox->value() - m_XIT; int nwave=(nsym+2)*nsps; int icmplx=0;
        gen_ft4wave_(const_cast<int *>(itone),&nsym,&nsps,&fsample,&f0,foxcom_.wave,foxcom_.wave,&icmplx,&nwave);
      }
      else if(m_modeTx=="JT65") { gen65_(message, &ichk, msgsent, const_cast<int *> (itone), &m_currentMessageType, len1, len1); }
      else if(m_modeTx=="JT9") { gen9_(message, &ichk, msgsent, const_cast<int *> (itone), &m_currentMessageType, len1, len1); }
      else if(m_modeTx=="T10") { gen10_(message, &ichk, msgsent, const_cast<int *> (itone), &m_currentMessageType, len1, len1); }							  
      else if(m_mode.left(4)=="WSPR") { genwspr_(message, msgsent, const_cast<int *> (itone), len1, len1); }

      if(m_colorTxMsgButtons && m_mode.left(4)!="WSPR") { if(m_restart) resetTxMsgBtnColor(); setTxMsgBtnColor(); m_txbColorSet=true; }
      msgsent[37]=0;
    }
    m_curMsgTx = QString::fromLatin1 (msgsent);
    if(m_mode.startsWith("FT")) { m_curMsgTx.remove (QChar {'<'}); m_curMsgTx.remove (QChar {'>'}); }
    m_currentMessage = m_curMsgTx;
    if(m_tune) { m_currentMessage = tr("TUNE"); m_currentMessageType = -1; m_nlasttx=0; }
    last_tx_label->setText(tr("LastTx: ") + m_currentMessage.trimmed());
	
    if(m_restart && !haltedEmpty) {
      QFile f {m_dataDir.absoluteFilePath (m_jtdxtime->currentDateTimeUtc2().toString("yyyyMM_")+"ALL.TXT")};
      if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
        {
          QTextStream out(&f);
          out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")"
              << "  Retransmitting " << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6)
              << " MHz +" << ui->TxFreqSpinBox->value () <<"Hz  " << m_modeTx
              << ":  " << m_currentMessage << 
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

          if(m_config.write_decoded_debug()) {
            QString autoseqa=ui->AutoSeqButton->text(); int indx=autoseqa.length()-1; QString autoseq="AutoSeq"+autoseqa[indx];
            out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")"
                << "  AF TX/RX " << ui->TxFreqSpinBox->value () << "/" << ui->RxFreqSpinBox->value ()
                << "Hz " << autoseq << (m_autoseq ? "-On" : "-Off") << " AutoTx" 
                << (m_autoTx ? "-On" : "-Off") << " SShotQSO" << (m_singleshot ? "-On" : "-Off")
                << " Hound mode" << (m_houndMode ? "-On" : "-Off") << " Skip Tx1" << (m_skipTx1 ? "-On" : "-Off") <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

          }
          f.close();
        }
      else
        {
          JTDXMessageBox::warning_message (this, "", tr ("File Open Error")
                                       , tr ("Cannot open \"%1\" for append: %2")
                                       .arg (f.fileName ()).arg (f.errorString ()));
        }
      if (m_config.TX_messages () || m_autoseq)
        {
        if (0 == ui->tabWidget->currentIndex ()) {
          if (ui->txrb5->isChecked() && m_autoseq && m_Tx5setAutoSeqOff) { m_wasAutoSeq=true; on_AutoSeqButton_clicked(false); }
        }
        else if (1 == ui->tabWidget->currentIndex ()) {
          if (ui->rbFreeText->isChecked() && m_autoseq && m_FTsetAutoSeqOff) { m_wasAutoSeq=true; on_AutoSeqButton_clicked(false); }
        }
          ui->decodedTextBrowser2->displayTransmittedText(m_currentMessage,m_baseCall,Radio::base_callsign (m_hisCall),(m_skipTx1 && !m_houndMode ? "S" : ""),m_modeTx,
                     ui->TxFreqSpinBox->value(),m_config.color_TxMsg(),m_qsoHistory);
        }
    }

    icw[0] = 0;
    auto msg_parts = m_currentMessage.split (' ', SkipEmptyParts);
/*    if (msg_parts.size () > 2) {
      // clean up short code forms
      msg_parts[0].remove (QChar {'<'});
      msg_parts[1].remove (QChar {'>'});
    } */

// m_currentMessageType m_lastMessageType are always 0 in FT8 mode
    auto is_73 = (m_QSOProgress >= ROGER_REPORT || m_ntx==4 || m_ntx==5) && message_is_73 (m_currentMessageType, msg_parts);
	auto lastmsg_is73 = message_is_73 (m_lastMessageType, m_lastMessageSent.split (' ', SkipEmptyParts));
    m_sentFirst73 = is_73 && m_lastloggedcall!=m_hisCall
      && (!lastmsg_is73 || (lastmsg_is73 && !m_lastMessageSent.contains(m_hisCall)));

    if (m_sentFirst73) {
      if (!m_mode.startsWith("FT") && m_config.id_after_73 ()) icw[0] = m_ncw;
      if ((m_config.prompt_to_log() || m_config.autolog ()) && !m_tune) {
		auto curtime=m_jtdxtime->currentDateTimeUtc2();
		// 4*m_TRperiod guard against duplicate logging the same callsign
		if (m_lastloggedcall!=m_hisCall || qAbs(curtime.toMSecsSinceEpoch()-m_lastloggedtime.toMSecsSinceEpoch()) > int(m_TRperiod) * 2000) {
		  m_logqso73=true;
          logQSOTimer.start (0);
		}
      }
    }

    if (is_73) {
		if (m_disable_TX_on_73 && !m_autoseq) {
			if (!enableTxButtonTimer.isActive())
				enableTxButtonTimer.start (100);
		}
    }

    if(!m_mode.startsWith("FT") && m_config.id_interval () >0) {
      int nmin=(m_sec0-m_secID)/60;
      if(nmin >= m_config.id_interval ()) {
        icw[0]=m_ncw;
        m_secID=m_sec0;
      }
    }

    if (m_currentMessageType < 6 && msg_parts.length() >= 3
        && (msg_parts[1] == m_config.my_callsign () ||
            msg_parts[1] == m_baseCall))
    {
      int i1;
      bool ok;
      i1 = msg_parts[2].toInt(&ok);
      if(ok and i1>=-50 and i1<50)
      {
        m_rptSent = msg_parts[2];
      } else {
        if (msg_parts[2].mid (0, 1) == "R")
        {
          i1 = msg_parts[2].mid (1).toInt (&ok);
          if (ok and i1 >= -50 and i1 < 50)
          {
            m_rptSent = msg_parts[2].mid (1);
          }
        }
      }
    }
    m_restart=false;
//----------------------------------------------------------------------
  } else {
    if (!m_enableTx && m_sentFirst73)
    {
      m_sentFirst73 = false;
      if (1 == ui->tabWidget->currentIndex())
      {
        ui->genMsg->setText(ui->tx6->text());
        m_ntx=7;
        ui->rbGenMsg->setChecked(true);
      }
    }
  }

  if (g_iptt == 1 && iptt0 == 0) {
    if(m_config.watchdog () && !m_mode.startsWith ("WSPR")
      && m_curMsgTx != m_msgSent0) {
      txwatchdog (false);  // in case we are auto sequencing
      m_msgSent0 = m_curMsgTx;
    }
// DX Call window must be empty if CQ msg being transmitted under AutoSeq
    if (m_autoseq && (m_QSOProgress == CALLING || m_ntx == 6) && !m_hisCall.isEmpty()) clearDXfields(" field cleared, CQ message triggered by AutoSeq");
    if (m_houndMode && (m_QSOProgress == REPLYING || m_ntx == 1)) m_lastCallingFreq = ui->TxFreqSpinBox->value ();
    if(m_curMsgTx!=m_msgSent0) m_msgSent0=m_curMsgTx;
    if(!m_tune && !haltedEmpty) {
      QFile f {m_dataDir.absoluteFilePath (m_jtdxtime->currentDateTimeUtc2().toString("yyyyMM_")+"ALL.TXT")};
      if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QTextStream out(&f);
        if(m_config.write_decoded_debug()) {
          QString autoseqa=ui->AutoSeqButton->text(); int indx=autoseqa.length()-1; QString autoseq="AutoSeq"+autoseqa[indx];
          out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")"
              << "  JTDX v" << QCoreApplication::applicationVersion () << revision () <<" Transmitting " << qSetRealNumberPrecision (12)
              << (m_freqNominal / 1.e6) << " MHz + " << ui->TxFreqSpinBox->value () <<"Hz  " << m_modeTx << ":  " << m_currentMessage <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl
#else
                 Qt::endl
#endif
 << "                   "
              << "  AF TX/RX " << ui->TxFreqSpinBox->value () << "/" << ui->RxFreqSpinBox->value ()
              << "Hz " << autoseq << (m_autoseq ? "-On" : "-Off") << " AutoTx" 
              << (m_autoTx ? "-On" : "-Off") << " SShotQSO" << (m_singleshot ? "-On" : "-Off")
              << " Hound mode" << (m_houndMode ? "-On" : "-Off") <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl
#else
                 Qt::endl
#endif
 << " Skip Tx1" << (m_skipTx1 ? "-On" : "-Off")
              << " HaltTxReplyOther" << (m_config.halttxreplyother () ? "-On" : "-Off") <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif
 }
        else {
          out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")"
              << "  Transmitting " << qSetRealNumberPrecision (12)
              << (m_freqNominal / 1.e6) << " MHz + " << ui->TxFreqSpinBox->value () <<"Hz  " << m_modeTx << ":  " << m_currentMessage <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif
 }
        f.close();
      } else {
        JTDXMessageBox::warning_message (this, "", tr ("File Open Error")
                                     , tr ("Cannot open \"%1\" for append: %2")
                                     .arg (f.fileName ()).arg (f.errorString ()));
      }
    }

    if ((m_config.TX_messages () || m_autoseq) && !m_tune) {
      if (0 == ui->tabWidget->currentIndex ()) {
        if (ui->txrb5->isChecked() && m_autoseq && m_Tx5setAutoSeqOff) { m_wasAutoSeq=true; on_AutoSeqButton_clicked(false); }
      }
      else if (1 == ui->tabWidget->currentIndex ()) {
        if (ui->rbFreeText->isChecked() && m_autoseq && m_FTsetAutoSeqOff) { m_wasAutoSeq=true; on_AutoSeqButton_clicked(false); }
      }
      if(!haltedEmpty) ui->decodedTextBrowser2->displayTransmittedText(m_curMsgTx,m_baseCall,Radio::base_callsign (m_hisCall),(m_skipTx1 && !m_houndMode ? "S" : ""),m_modeTx,
            ui->TxFreqSpinBox->value(),m_config.color_TxMsg(),m_qsoHistory);
    }
    switch (m_ntx)
      {
      case 1: m_QSOProgress = REPLYING; break;
      case 2: m_QSOProgress = REPORT; break;
      case 3: m_QSOProgress = ROGER_REPORT; break;
      case 4: m_QSOProgress = ROGERS; break;
      case 5: m_QSOProgress = SIGNOFF; break;
      case 6: m_QSOProgress = CALLING; break;
      default: break;             // determined elsewhere
      } 

    m_transmitting = true;
    m_txPeriod = period_index (0.001 * (m_jtdxtime->currentMSecsSinceEpoch2 () % 86400000), m_TRperiod);   // CE3TSK P7
    if (!m_config.tx_QSY_allowed ()) ui->TxFreqSpinBox->setDisabled(true);
    m_transmittedQSOProgress = m_QSOProgress;
    transmitDisplay (true);
    if(m_filter) {
      if(m_FilterState==2) { if(m_QSOProgress == CALLING) autoFilter (false); } // switch filter off at getting 73
      else if(m_FilterState==1) { if (m_QSOProgress == SIGNOFF || (!m_rrr && m_QSOProgress == ROGERS)) autoFilter (false); } // switch filter off at sending 73
    }
    statusUpdate ();
  }

  if((!m_btxok && btxok0 && g_iptt==1) || m_haltTrans) { stopTx(); if(m_haltTrans) m_haltTrans=false; }

  if(m_startAnother) {
    m_startAnother=false;
    on_actionOpen_next_in_directory_triggered();
  }

//Once per second:
  if(nsec != m_sec0) {
    if (m_config.watchdog() && !m_transmitting && !m_mode.startsWith ("WSPR")
        && m_idleMinutes >= m_config.watchdog ()) {
      txwatchdog (true);       // switch off Enable Tx button
    }
    if(m_tune && m_config.tunetimer() && !m_tuneup) { //shall not count at WSPR band hopping
      QString remtime;
      remtime = QString::asprintf("%.0f s",StopTuneTimer.remainingTime()/1000.0);
      ui->tuneButton->setText(remtime);
    }
    if(m_monitoring or m_transmitting) {
        int isec=int(fmod(tsec,m_TRperiod));
        progressBar->setValue(isec);
    } else {
        progressBar->setValue(0);
    }


    if(m_transmitting) {
      setTxStatusColour ("#ffff33");
      if(m_tune) tx_status_label->setText(tr("Tx: TUNE"));
      else tx_status_label->setText(tr("Tx: ") + m_curMsgTx.trimmed());
	  setProgressBarStyle ();
    } else if(m_monitoring) {
	  if (!m_txwatchdog) {
		setTxStatusColour ("#00ff00");
		QString t=tr("Receiving ");
		tx_status_label->setText(t);
	  }
      transmitDisplay(false);
      setProgressBarStyle ();
    } else if (!m_diskData && !m_txwatchdog) {
      setTxStatusColour ("");
      tx_status_label->setText("");
      setProgressBarStyle ();
    }
    if(m_transmitting && !m_tune && (m_nseq==10 || m_nseq==11)) { m_lapmyc=1; m_mslastTX = m_jtdxtime->currentMSecsSinceEpoch2(); } //setting twice: make sure it is not skipped
    QDateTime tme = m_jtdxtime->currentDateTimeUtc2();
    QString currentDate = tme.date().toString("dd.MM.yyyy");
    QString utc = tme.time().toString();
    QString hour = tme.time().toString("hh");
    QString minute = tme.time().toString("mm");
    QString second = tme.time().toString("ss");
    int isecond=second.toInt();
    ui->labUTC->setText(utc);
    date_label->setText(currentDate);
	// setting labUTC clock style at start
	if(m_start) setClockStyle(true);
	// setting labUTC clock style at operation
	if (clock_refresh_due(m_mode,isecond)) setClockStyle(false);   // CE3TSK: modetiming.h
	// setting band scheduler
	if((minute.toInt())%5==0 && second == "01" && m_config.usesched() && !m_enableTx) {
        if (m_config.sched_hh_1() == hour && m_config.sched_mm_1() == minute) {
          set_scheduler(m_config.sched_band_1(),m_config.sched_mix_1());
        } else if (!m_config.sched_band_2().isEmpty ()) {
          if (m_config.sched_hh_2() == hour && m_config.sched_mm_2() == minute) {
            set_scheduler(m_config.sched_band_2(),m_config.sched_mix_2());
          } else if (!m_config.sched_band_3().isEmpty ()) {
            if (m_config.sched_hh_3() == hour && m_config.sched_mm_3() == minute) {
              set_scheduler(m_config.sched_band_3(),m_config.sched_mix_3());
            } else if (!m_config.sched_band_4().isEmpty ()) {
              if (m_config.sched_hh_4() == hour && m_config.sched_mm_4() == minute) {
                set_scheduler(m_config.sched_band_4(),m_config.sched_mix_4());
              } else if (!m_config.sched_band_5().isEmpty () && m_config.sched_hh_4() == hour && m_config.sched_mm_4() == minute) {
                set_scheduler(m_config.sched_band_5(),m_config.sched_mix_5());
              }
            }
          }
        }
    }
    m_sec0=nsec;
    if(!m_monitoring and !m_diskData) ui->signal_meter_widget->setValue(0);
    displayDialFrequency ();
    if (m_geometry_restored > 0) { m_geometry_restored -=1;
      /* CE3TSK: the delayed re-restore would undo the clamp applied at start-up */
      if (m_geometry_restored == 0) restoreMainGeometry (); }
    /* CE3TSK: the safety net behind ndecreq. The request counter should make a lost decode
       impossible; this catches anything that still wedges the pair - the decoder killed, a
       shared-memory mishap - and turns a dead session into one lost period. Recreating .lock
       releases the decoder's wait and decodeBusy(false) gives the operator back File -> Open,
       Open next and the Decode button, which is what "hung" looks like from the outside.
       This was in the tree, commented out, with 10 s for FT4 and 18 s for FT8. Those predate
       the pipeline: the clock runs from decode() to <DecodeFinished>, which is 1-2 s in
       practice, but a slow machine on a heavy preset can take far longer, and a watchdog that
       fires during honest work is worse than the fault. The bounds below are far above any
       real decode and still recover within seconds of a wedge. */
    quint64 timeout=120000;
    if(m_mode=="FT4") timeout=30000;
    else if(m_mode=="FT2") timeout=15000;   // CE3TSK
    else if(m_mode=="FT8") timeout=60000;
    if(m_decoderBusy && m_msDecoderStarted>0 && !m_mode.startsWith("WSPR")
       && (m_jtdxtime->currentMSecsSinceEpoch2()-m_msDecoderStarted)>timeout) {
      m_manualDecode=false; ui->DecodeButton->setChecked (false);
      QFile {m_config.temp_dir ().absoluteFilePath (".lock")}.open(QIODevice::ReadWrite);
      decodeBusy(false);
      m_msDecoderStarted=0;   // one recovery per stall, not one per GUI tick
      writeToALLTXT("Decoding forcibly recovered");
    }
  }
  iptt0=g_iptt;
  btxok0=m_btxok;
}               //End of GUIupdate

void MainWindow::set_scheduler(QString const& setto,bool mixed)
{
  QString newband;
  Frequency frq = 0;

  if (setto.contains(",")) { frq = setto.left(setto.indexOf(" ")).replace("*","").replace(",","").toInt(); }
  else { frq = setto.left(setto.indexOf(" ")).replace("*","").replace(".","").toInt(); }

  if (mixed) {
    newband="JT9+JT65";
    on_actionJT9_JT65_triggered();
  } else {
    newband=setto.mid(setto.indexOf(" ")+1,4);
    if (newband == "FT8") { on_actionFT8_triggered(); }
    else if (newband == "FT4") { on_actionFT4_triggered(); }
    else if (newband == "FT2") { on_actionFT2_triggered(); }   // CE3TSK
    else if (newband == "JT65") { on_actionJT65_triggered(); }
	else if (newband == "JT9") { on_actionJT9_triggered(); }
	else if (newband == "T10") { on_actionT10_triggered(); }
	else if (newband == "WSPR") { on_actionWSPR_2_triggered(); } 
  }

  m_bandEdited = true;
  band_changed (frq); if(m_config.write_decoded_debug()) writeToALLTXT("Band changed from scheduler, frequency: " + QString::number(frq));
  m_wideGraph->setRxBand (m_config.bands ()->find (frq));
}

void MainWindow::haltTx(QString reason) { m_haltTxWritten=true; writeHaltTxEvent(reason); on_stopTxButton_clicked(); }

void MainWindow::haltTxTuneTimer()
{
  if(m_config.write_decoded_debug()) { writeToALLTXT("Halt Tx triggered: tuning stopped"); m_haltTxWritten=true; }
  on_stopTxButton_clicked();
}

/* CE3TSK: cty.dat or the LoTW user activity file was replaced from Settings. Reading both again
   holds the GUI for about 2.7 s, so while monitoring or transmitting (a transmission turns
   monitoring off) it waits in initLogIfNeeded(), the path a changed wsjtx_log.adi already takes;
   otherwise there is no period to protect. */
void MainWindow::dataFilesUpdated ()
{
  m_logInitNeeded = true;
  m_dataFilesChanged = true;
  if (!m_monitoring && !m_transmitting) initLogIfNeeded ();
}

void MainWindow::logChanged()
{
  if(!m_qsoLogged) {
    m_logInitNeeded = true;
//workaround to handle file overwriting scenario under Linux
#ifndef WIN32
    QFileInfo checkFile(m_dataDir.absoluteFilePath ("wsjtx_log.adi"));
        int counter=0;
    while(!checkFile.exists() && counter<15) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); counter++; }
    fsWatcher->addPath(m_dataDir.absoluteFilePath ("wsjtx_log.adi"));
#endif
  }
  else m_qsoLogged=false;
}

void MainWindow::startTx2()
{
//  for(int i=0; i<15; i++) { // workaround to modulator start failure
//    if(i==1) QThread::currentThread()->msleep(5);
//    else if(i>1) QThread::currentThread()->msleep(10);
//    printf("%s(%0.1f) Timing modulator",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),m_jtdxtime->GetOffset());
    bool modulator_active;
    bool tci_active = m_tci;
    if (tci_active) modulator_active=m_tci_mod_active;
    else modulator_active=m_modulator->isActive ();
    if (!modulator_active) { // TODO - not thread safe
      double fSpread=0.0;
      double snr=99.0;
      QString t=ui->tx5->currentText();
      if(t.left(1)=="#") fSpread=t.mid(1,5).toDouble();
      if (tci_active) Q_EMIT m_config.transceiver_spread(fSpread);
      else m_modulator->setSpread(fSpread); // TODO - not thread safe
      t=ui->tx6->text();
      if(t.left(1)=="#") snr=t.mid(1,5).toDouble();
      if(snr>0.0 or snr < -50.0) snr=99.0;
      transmit (snr);
//      printf(" started %s\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str());
      if(m_config.write_decoded_debug()) writeToALLTXT("Modulator started");
//      QThread::currentThread()->setPriority(QThread::HighPriority);
      ui->signal_meter_widget->setValue(0);

      if(m_mode.left(4)=="WSPR" and !m_tune) {
        if (m_config.TX_messages ()) {
          t = " Transmitting " + m_mode + " ----------------------- " + m_config.bands ()->find (m_freqNominal);
          t=WSPR_hhmm(0) + ' ' + t.rightJustified (66, '-');
          ui->decodedTextBrowser->appendText(t,Radio::convert_dark("#ffffff",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle),0," ",Radio::convert_dark("#000000",m_useDarkStyle));
        }

        QFile f {m_dataDir.absoluteFilePath ("ALL_WSPR.TXT")};
        if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
          QTextStream out(&f);
          out << m_jtdxtime->currentDateTimeUtc2().toString("yyMMdd hhmm")
              << "  Transmitting " << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6) << " MHz:  "
              << m_currentMessage << "  " + m_mode <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

          f.close();
        } else {
          JTDXMessageBox::warning_message (this, "", tr ("File Open Error")
                                       , tr ("Cannot open \"%1\" for append: %2")
                                       .arg (f.fileName ()).arg (f.errorString ()));
        }
      }
      return;
    }
  if(m_config.write_decoded_debug()) writeToALLTXT("MainWindow::startTx2() failed to start modulator: m_modulator is active");
}

void MainWindow::stopTx()
{
  if (m_tci) Q_EMIT m_config.transceiver_modulator_stop();
  else Q_EMIT endTransmitMessage ();
  m_btxok = false;
  m_transmitting = false;
  ui->TxFreqSpinBox->setDisabled(false);
  g_iptt=0;
  if (!m_txwatchdog) {
	  setTxStatusColour ("");
	  tx_status_label->setText("");
  }
  if (m_tci) ptt0Timer.start(0); else {
    ptt0Timer.start(200);                //Sequencer delay
    if(!m_monitoroff) monitor (true);
    statusUpdate ();
    m_secTxStopped=m_jtdxtime->currentMSecsSinceEpoch2()/1000;
  }
}

void MainWindow::stopTx2()
{
  Q_EMIT m_config.transceiver_ptt (false);      //Lower PTT
  if (m_tci) {
    if(!m_monitoroff) monitor (true);
    statusUpdate ();
    m_secTxStopped=m_jtdxtime->currentMSecsSinceEpoch2()/1000;
  }
//  QThread::currentThread()->setPriority(QThread::HighPriority);
  if(m_mode.left(4)=="WSPR" and m_ntr==-1 and !m_tuneup) {
    m_wideGraph->setWSPRtransmitted();
    WSPR_scheduling ();
    m_ntr=0;
  }
  last_tx_label->setText(tr("LastTx: ") + m_currentMessage.trimmed());
}

void MainWindow::RxQSY()
{
  // this appears to be a null directive
  if (m_config.is_transceiver_online ()) {
    Q_EMIT m_config.transceiver_frequency(m_freqNominal);
  }
}

void MainWindow::ba2msg(QByteArray ba, char message[])             //ba2msg()
{
  int iz=ba.length();
  for(int i=0; i<37; i++) {
    if(i<iz) {
//      if(int(ba[i])>=97 and int(ba[i])<=122) ba[i]=int(ba[i])-32;
      message[i]=ba[i];
    } else {
      message[i]=32;
    }
  }
  message[37]=0;
}

void MainWindow::on_TxMinuteButton_clicked(bool checked)        //TxFirst
{
  m_txFirst=checked;
  if(m_transmitting && m_config.write_decoded_debug()) writeToALLTXT("Tx halted: period changed via TX period button");
  if (m_txGenerated != checked && m_enableTx && m_autoseq)  clearDX (" cleared, Tx Minute button clicked");
  setMinButton();
}

void MainWindow::on_rrrCheckBox_stateChanged(int nrrr)        //RRR or RR73
{
  m_rrr = (nrrr==2);
  if(!m_rrr) { ui->pbSendRRR->setText("RR73"); }
  else { ui->pbSendRRR->setText("RRR"); }
  ui->rrr1CheckBox->setChecked(m_rrr);
  if(ui->genMsg->text().contains (" RRR") || ui->genMsg->text().contains (" RR73")) on_pbSendRRR_clicked();
}

void MainWindow::on_rrr1CheckBox_stateChanged(int nrrr)        //RRR or RR73
{
  m_rrr = (nrrr==2);
  ui->rrrCheckBox->setChecked(m_rrr);
  if(!ui->tx4->text().isEmpty ()) genStdMsgs(m_rpt); // can change message for non tx4 transmission?
}

void MainWindow::set_ntx(int n) { m_ntx=n; m_nlasttx=n; } //set_ntx()

void MainWindow::on_txb1_clicked()                                //txb1
{
  m_ntx=1;
  m_QSOProgress = REPLYING;
  m_nlasttx=1;
  ui->txrb1->setChecked(true);
  if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb2_clicked()                                //txb2
{
  m_ntx=2;
  m_QSOProgress = REPORT;
  m_nlasttx=2;
  ui->txrb2->setChecked(true);
  if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb3_clicked()                                //txb3
{
  m_ntx=3;
  m_QSOProgress = ROGER_REPORT;
  m_nlasttx=3;
  ui->txrb3->setChecked(true);
  if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb4_clicked()                                //txb4
{
  m_ntx=4;
  m_QSOProgress = ROGERS;
  m_nlasttx=4;
  ui->txrb4->setChecked(true);
  if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb5_clicked()                                //txb5
{
  m_ntx=5;
  m_QSOProgress = SIGNOFF;
  m_nlasttx=5;
  ui->txrb5->setChecked(true);
  if (m_transmitting) m_restart=true;
}

void MainWindow::on_txb6_clicked()                                //txb6
{
  if(!m_hisCall.isEmpty()) clearDXfields(" field cleared, SLOT on_txb6_clicked()");
//added on 20180907 step .92_33 needs in testing
  ui->labAz->setText("");
  ui->labDist->setText("");
  ui->tx1->setText("");
  ui->tx2->setText("");
  ui->tx3->setText("");
  ui->tx4->setText("");
  ui->tx5->setCurrentText("");
  ui->genMsg->setText("");
//
  if(!m_autoseq && m_wasAutoSeq) { m_wasAutoSeq=false; on_AutoSeqButton_clicked(true); }
  if (m_spotDXsummit){
     ui->pbSpotDXCall->setText(tr("DX Call"));
     m_dxCallHidden=false;   // CE3TSK
     if(m_config.spot_to_dxsummit()) { ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#c4c4ff",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
     else { ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#aabec8",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
     m_spotDXsummit=false;
  }
  m_ntx=6;
  m_QSOProgress = CALLING;
  m_nlasttx=6;
  ui->txrb6->setChecked(true);
  ui->genMsg->setText(ui->tx6->text()); // directional CQ tab1/tab2 sync
  if (m_transmitting) m_restart=true;
}

void MainWindow::doubleClickOnCall(bool alt, bool ctrl)
{
  txwatchdog (false);
  QTextCursor cursor;
  QString t;                         //Full contents
  if(m_decodedText2) {
    cursor=ui->decodedTextBrowser->textCursor();
    t= ui->decodedTextBrowser->toPlainText();
  } else {
    cursor=ui->decodedTextBrowser2->textCursor();
    t= ui->decodedTextBrowser2->toPlainText();
  }
  cursor.select(QTextCursor::LineUnderCursor);
  int position {cursor.position()};

  QString messages;
  if(!m_decodedText2) messages = ui->decodedTextBrowser2->toPlainText();
  if(m_decodedText2) messages = ui->decodedTextBrowser->toPlainText();
  processMessage(messages, position, alt, ctrl);
}

void MainWindow::doubleClickOnCall2(bool alt, bool ctrl)
{
  m_decodedText2=true;
  doubleClickOnCall(alt,ctrl);
  m_decodedText2=false;
}

void MainWindow::processMessage(QString const& messages, int position, bool alt, bool ctrl)
{
//QString result;
//QTextStream(&result) << position;
//msgBox (result);
  QString t1 = messages.left(position);              //contents up to \n on selected line
  int i1=t1.lastIndexOf("\n") + 1;       //points to first char of line
  QString t2 = messages.mid(i1,position-i1+2).replace("\n","   ");         //selected line
  QString t2disp;
  bool period_changed = false;
  bool tx_message = false;
  auto tcut = t2;
  
  if (!m_mode.startsWith("FT")) {
    t2disp=t2.left(40)+"     "+t2.mid(40,1);
    tcut.remove(0,21);
    tx_message = t2.mid(6,2) == "Tx";
  } else {
    t2disp=t2.left(50);
    tcut.remove(0,23);
    tx_message = t2.mid(8,2) == "Tx";
  }
  
  /* CE3TSK: WW Digi contest - a double click on a message that ends in a signal report does
     nothing at all. The exchange is the grid, so such a station is not in the contest (item 26
     drops the QSO and skips him); starting one by hand would only repeat the deadlock, and the
     click is far more likely to be a slip than a decision. Deliberately the first thing in
     here, before the period logic below touches m_txFirst or the Tx minute button. A click on
     any other line of the same station - his CQ, his grid - still works and still overrules the
     five minute skip, which is the operator's way in. */
  if (m_wwDigi && contest_report_message (contest_displayed_message (t2, m_mode.startsWith ("FT"))))
    {
      if (m_config.write_decoded_debug ())
        writeToALLTXT ("contest: double click ignored, signal report message " + t2.trimmed ());
      return;
    }

  QString t2a;
  t2a = t2;
  DecodedText decodedtext {t2a,this};

  bool addWanted = (alt && ctrl);
  if(!addWanted) {
//    int nmod=decodedtext.timeInSeconds () % (2*int(m_TRperiod));
    int nmod = fmod(double(decodedtext.timeInSeconds()),2.0*m_TRperiod);
//    printf ("periods %d,%f,%d,%s,%d\n",decodedtext.timeInSeconds (),m_TRperiod,nmod,t2.mid(6,4).toStdString().c_str(),tx_message);
    if (m_txFirst != (nmod!=0) && !tx_message) {
      m_txFirst=(nmod!=0);
//      ui->TxMinuteButton->setChecked(m_txFirst);
//      an attempt to provide stable UDP Reply operation:
      if(m_txFirst && !ui->TxMinuteButton->isChecked()) ui->TxMinuteButton->click();
      else if(!m_txFirst && ui->TxMinuteButton->isChecked()) ui->TxMinuteButton->click();
//      setMinButton();
      period_changed = true;
      if(m_transmitting && m_config.write_decoded_debug()) writeToALLTXT("Tx halted: period changed via message click");
    }
  }

  auto t3 = decodedtext.string ().left(47);
  auto t4 = t3.replace (QRegularExpression {" CQ ([A-Z]{1,2}|[0-9]{3,3}) "}, " CQ_\\1 ").split (" ", SkipEmptyParts);
  if(t4.size () < 6) return;             //Skip the rest if no decoded text


  QString hiscall;
  QString hisgrid;
  decodedtext.deCallAndGrid(/*out*/hiscall,hisgrid);

  if(addWanted) {
    if(!hiscall.isEmpty() && hiscall != m_hisCall && hiscall != m_config.my_callsign()) {
      QString t5{""}; if(m_mode.startsWith("FT")) t5=hiscall; else t5=Radio::base_callsign(hiscall);
      QString t6{""};
      if(m_wantedCall.isEmpty())  ui->wantedCall->setText(t5);
      else if(m_wantedCallList.length()<20 && !m_wantedCall.startsWith(t5+",") && !m_wantedCall.endsWith(","+t5) 
              && !m_wantedCall.contains(","+t5+",")) { t6 = m_wantedCall; ui->wantedCall->setText(t6.append(","+t5)); }
    }
    return;
  }

  if (!Radio::is_callsign (hiscall) // not interested if not from QSO partner
      && !(t4.size () == 7          // unless it is of the form
           && (t4.at (5) == m_baseCall // "<our-call> 73"
               || t4.at (5).startsWith (m_baseCall + '/')
               || t4.at (5).endsWith ('/' + m_baseCall))
           && t4.at (6) == "73"))
    {
      qDebug () << "Not processing message - hiscall:" << hiscall << "hisgrid:" << hisgrid;
//SNR from free messages to report for operation with special callsigns in the manual mode
	  if (!m_autoseq) {
         QString rpt = decodedtext.report();
         ui->rptSpinBox->setValue(rpt.toInt());
         genStdMsgs(rpt);
      }
      return;
    }
  QString curBand;
  QString prevmodeTx = m_modeTx;
  // only allow automatic mode changes between JT9 and JT65, and when not transmitting
  if (!m_transmitting and m_mode == "JT9+JT65") {
      if (decodedtext.isJT9())
        {
          if (m_modeTx != "JT9" && m_config.pwrBandTxMemory() && !m_tune)
            {
              curBand = ui->bandComboBox->currentText()+m_mode;
              if (m_pwrBandTxMemory.contains(curBand))
                {
                  ui->outAttenuation->setValue(m_pwrBandTxMemory[curBand].toInt());
                }
              else if (m_outAttenuationRestored)
                {
                  m_pwrBandTxMemory[curBand] = ui->outAttenuation->value();   /* CE3TSK: never the .ui default */
                }
            }
          m_modeTx="JT9";
          ui->pbTxMode->setText("Tx JT9  @");
          m_wideGraph->setModeTx(m_modeTx);
        } 
      else if (decodedtext.isJT65())
        {
          if (m_modeTx != "JT65" && m_config.pwrBandTxMemory() && !m_tune)
            {
              curBand = ui->bandComboBox->currentText()+"JT65";
              if (m_pwrBandTxMemory.contains(curBand))
                {
//                  printf("auto mode changed %s JT65 %s:%d\n",m_mode.toStdString().c_str(),curBand.toStdString().c_str(),m_pwrBandTxMemory[curBand].toInt());
                  ui->outAttenuation->setValue(m_pwrBandTxMemory[curBand].toInt());
                }
              else if (m_outAttenuationRestored)
                {
                  m_pwrBandTxMemory[curBand] = ui->outAttenuation->value();   /* CE3TSK: never the .ui default */
                }
            }
          m_modeTx="JT65";
          ui->pbTxMode->setText("Tx JT65  #");
          m_wideGraph->setModeTx(m_modeTx);
        }
    } else if ((decodedtext.isJT9 () and m_modeTx != "JT9") or
               (decodedtext.isJT65 () and m_modeTx != "JT65")) {
      // if we are not allowing mode change then don't process decode
      return;
    }

  int frequency = decodedtext.frequencyOffset();
  QString firstcall = decodedtext.call();
  bool TxModeChanged = false;
  if (m_mode == "JT9+JT65") {
	if((decodedtext.isJT9 () and prevmodeTx != "JT9") or (decodedtext.isJT65 () and prevmodeTx != "JT65")) {
      TxModeChanged = true; on_spotLineEdit_textChanged(ui->spotLineEdit->text()); 
    }
  }

  // Don't change Tx freq if in a fast mode;
  // unless m_lockTxFreq is true or CTRL is held down
  if (m_lockTxFreq or ctrl or TxModeChanged) {
     if (ui->TxFreqSpinBox->isEnabled ()) {
        ui->TxFreqSpinBox->setValue(frequency);
     } else {
        return;
     }
  }

  int i9=m_QSOText.indexOf(decodedtext.string());
  if (i9<0 and !decodedtext.isTX() and m_decodedText2) {
    DecodedText decodedtext {t2disp,this};
	if (!t2.contains (m_baseCall) || !m_showMyCallMsgRxWindow) {
		ui->decodedTextBrowser2->displayDecodedText(&decodedtext
                                                  ,m_baseCall
                                                  ,Radio::base_callsign (m_hisCall)
                                                  ,m_hisGrid.left(4)
                                                  ,m_notified
                                                  ,(m_wwDigi ? m_contestLog : m_logBook)
                                                  ,m_qsoHistory2
                                                  ,m_qsoHistory
                                                  ,m_freqNominal
                                                  ,m_mode
                                                  ,m_bypassRxfFilters
                                                  ,m_bypassAllFilters
                                                  ,m_wideGraph->rxFreq());
	}
      m_QSOText=decodedtext.string();
  }

  if (ui->RxFreqSpinBox->isEnabled ())
    {
      ui->RxFreqSpinBox->setValue (frequency); //Set Rx freq
    }
  if (decodedtext.isTX())
    {
      if (ctrl && ui->TxFreqSpinBox->isEnabled ())
        {
          ui->TxFreqSpinBox->setValue(frequency); //Set Tx freq
        }
      return;
    }

  // prior DX call (possible QSO partner)
  auto qso_partner_base_call = Radio::base_callsign (m_hisCall);
  bool call_changed = false;
  auto base_call = Radio::base_callsign (hiscall);
  if (base_call != Radio::base_callsign (m_hisCall) || base_call != hiscall)
    {
  // his base call different or his call more qualified
  // i.e. compound version of same base call
      if (!m_hisGrid.isEmpty()) ui->dxGridEntry->clear();
      i1=m_qsoHistory.reset_count(hiscall);
      if (m_callToClipboard) clipboard->setText(hiscall);
      ui->dxCallEntry->setText(hiscall); setDxCallEntryColour ("#ffffff");
      call_changed = true;
      onCallPickedByHand (base_call);   /* CE3TSK: the operator's own choice overrides the policy */
      m_contestIgnore.remove (base_call);   /* CE3TSK: ... and the contest's report-message skip */
    }
  if (gridOK(hisgrid)) {
    if(m_hisGrid.left(4) != hisgrid) ui->dxGridEntry->setText(hisgrid);
  }
  if (m_hisGrid.isEmpty ())
    lookup();

  QString rpt = decodedtext.report();
  ui->rptSpinBox->setValue(rpt.toInt());
  genStdMsgs(rpt);

// Determine appropriate response to received message
  auto dtext = " " + decodedtext.string () + " ";
  if(dtext.contains (" " + m_baseCall + " ")
     || dtext.contains ("/" + m_baseCall + " ")
     || dtext.contains (" " + m_baseCall + "/")
     || (firstcall == "DE" && ((t4.size () > 7 && t4.at(7) != "73") || t4.size () <= 7)))
    {



      /* CE3TSK: the choice of the answer lives in contestreply.h (unit test test_reply): the
         WW Digi contest answers a caller's grid with Tx3 "R GRID", everything else as JTDX did */
      int tx = 2;
      if (t4.size () == 7 && t4.at (6) == "73") tx = 5;   // 73 back to compound call holder
      else tx = reply_tx_to_me (m_wwDigi, t4.size () > 7, t4.size () > 7 ? t4.at (7) : QString {},
                                t4.size () > 7 && gridOK (t4.at (7)));
      m_ntx = tx; m_nlasttx = tx;
      switch (tx) {
        case 5: m_QSOProgress = SIGNOFF;      ui->txrb5->setChecked(true); break;
        case 4: m_QSOProgress = ROGERS;       ui->txrb4->setChecked(true); break;
        case 3: m_QSOProgress = ROGER_REPORT; ui->txrb3->setChecked(true); break;
        default: m_QSOProgress = REPORT;      ui->txrb2->setChecked(true); break;
      }
      if(ui->tabWidget->currentIndex()==1) {
        if (tx == 5) ui->genMsg->setText(ui->tx5->currentText());
        else if (tx == 4) ui->genMsg->setText(ui->tx4->text());
        else if (tx == 3) ui->genMsg->setText(ui->tx3->text());
        else ui->genMsg->setText(ui->tx2->text());
        m_ntx=7;
        ui->rbGenMsg->setChecked(true);
      }
    QString rpt_type;
    decodedtext.report(m_baseCall,
      Radio::base_callsign(m_hisCall), m_rptRcvd, rpt_type,m_wwDigi);
    }
  else if (firstcall == "DE" && t4.size () == 8 && t4.at (7) == "73") {
    if (base_call == qso_partner_base_call) {
      // 73 back to compound call holder
      m_ntx=5;
      m_QSOProgress = SIGNOFF;
      m_nlasttx=5;
      ui->txrb5->setChecked(true);
      if(ui->tabWidget->currentIndex()==1) {
        ui->genMsg->setText(ui->tx5->currentText());
        m_ntx=7;
        ui->rbGenMsg->setChecked(true);
      }
    }
    else {
      // treat like a CQ/QRZ
      if (m_skipTx1 && !m_houndMode) {
         if (0 == ui->tabWidget->currentIndex()) m_ntx = 2;
            m_QSOProgress = REPORT;
            m_nlasttx = 2;
            ui->txrb2->setChecked(true); }
      else {
        if(0 == ui->tabWidget->currentIndex()) m_ntx = 1;
        m_QSOProgress = REPLYING;
        m_nlasttx = 1;
        ui->txrb1->setChecked(true); }
      if(1 == ui->tabWidget->currentIndex()) {
        if (m_skipTx1 && !m_houndMode) { ui->genMsg->setText(ui->tx2->text()); }
        else { ui->genMsg->setText(ui->tx1->text()); }
        m_ntx=7;
        ui->rbGenMsg->setChecked(true);
      }
    }
  }
  else // myCall not in msg
    {
      if (m_skipTx1 && !m_houndMode) {
        if(0 == ui->tabWidget->currentIndex()) m_ntx=2;
        if (call_changed) m_qsoHistory.remove(hiscall); //prevent braking Auto Sequence by QSO history values 
        m_QSOProgress = REPORT;
        m_nlasttx=2;
        ui->txrb2->setChecked(true); }
      else {
        if(0 == ui->tabWidget->currentIndex()) m_ntx=1;
        m_QSOProgress = REPLYING;
        m_nlasttx=1;
        ui->txrb1->setChecked(true); }
      if(1 == ui->tabWidget->currentIndex()) {
        if (m_skipTx1 && !m_houndMode) { ui->genMsg->setText(ui->tx2->text()); }
        else { ui->genMsg->setText(ui->tx1->text()); }
        m_ntx=7;
        ui->rbGenMsg->setChecked(true);
      }
    }
  //RX3ASP request for manually toggling Enable Tx under AutoSeq while AutoTx is switched off
  if(m_autofilter && m_autoseq && !m_filter && (m_QSOProgress == REPORT || m_QSOProgress == REPLYING)) autoFilter (true);
  if(m_transmitting && !period_changed) m_restart=true;
//  if(m_autoTx && !alt) enableTx_mode(true);
// an attempt to provide stable UDP Reply operation:
  if(m_autoTx && !alt) { if(!ui->enableTxButton->isChecked()) ui->enableTxButton->click(); }
  if(alt && m_enableTx) haltTx("TX halted: ALT modifier is used at double click on message ");
  if(m_config.write_decoded_debug()) {
    QString EnTXstate = m_enableTx ? "On" : "OFF"; QString AuTXstate = m_autoTx ? "On" : "OFF";
    writeToALLTXT("double click on call processed: " + hiscall + "; EnableTx " + EnTXstate + ", AutoTx " + AuTXstate);
  }
  if(!ui->spotLineEdit->text().isEmpty() && ui->spotLineEdit->text().contains("#")) on_spotLineEdit_textChanged(ui->spotLineEdit->text());
}

bool MainWindow::stdCall(QString const& w)
{
  static QRegularExpression standard_call_re {
    R"(
        ^\s*				# optional leading spaces
        ( [A-Z]{0,2} | [A-Z][0-9] | [0-9][A-Z] )  # part 1
        ( [0-9][A-Z]{0,3} )                       # part 2
        (/R | /P)?			# optional suffix
        \s*$				# optional trailing spaces
    )", QRegularExpression::CaseInsensitiveOption | QRegularExpression::ExtendedPatternSyntaxOption};
  return standard_call_re.match (w).hasMatch ();
}

void MainWindow::ScrollBarPosition(int n) { m_position=n; }

void MainWindow::on_S_meter_button_clicked(bool checked)
{
  ui->S_meter_button->setText(Radio::convert_Smeter(m_rigState.level(),checked));
}

/* CE3TSK: WW Digi contest - true when the contest is selected but the station grid is
   missing or malformed, so no valid 4 character exchange can be built. A placeholder grid
   is deliberately not substituted: it would reach the other operator's log as a real
   exchange. Refuse to transmit instead. */
bool MainWindow::wwDigiNoGrid ()
{
  return m_wwDigi && !gridOK(m_config.my_grid ().left (4));
}

void MainWindow::genCQMsg ()
{
  if(wwDigiNoGrid ()) { ui->tx6->clear (); return; } /* CE3TSK: no exchange, no CQ */
  QString grid{m_config.my_grid()};
  if(m_config.my_callsign().size () && grid.size ()) {
    if(stdCall(m_config.my_callsign())) {
//      msgtype (QString {"%1 %2 %3"}.arg(m_CQtype).arg(m_config.my_callsign())
//               .arg(grid.left(4)),ui->tx6);
      msgtype (QString {"CQ %1 %2"}.arg(m_config.my_callsign())
               .arg(grid.left(4)),ui->tx6);
    } else {
//      msgtype (QString {"%1 %2"}.arg(m_CQtype).arg(m_config.my_callsign()),ui->tx6);
      msgtype (QString {"CQ %1"}.arg(m_config.my_callsign()),ui->tx6);
    }


/*    QString t=ui->tx6->text();
    if(m_mode=="FT8" and SpecOp::NONE != m_config.special_op_id() and
       t.split(" ").at(1)==m_config.my_callsign() and stdCall(m_config.my_callsign())) {
      if(SpecOp::NA_VHF == m_config.special_op_id())    t="CQ TEST" + t.mid(2,-1);
      if(SpecOp::EU_VHF == m_config.special_op_id())    t="CQ TEST" + t.mid(2,-1);
      if(SpecOp::FIELD_DAY == m_config.special_op_id()) t="CQ FD" + t.mid(2,-1);
      if(SpecOp::RTTY == m_config.special_op_id())      t="CQ RU" + t.mid(2,-1);
      ui->tx6->setText(t);
    } */
// code below shall be combined into m_CQtype or CQ CONTEST shall be combined into m_cqdir
    if(!m_cqdir.isEmpty () && stdCall(m_config.my_callsign())) { QString t="CQ " + m_cqdir + " " + m_config.my_callsign() + " " + grid.left(4); /* CE3TSK: same local grid as the plain CQ above */
      msgtype(t, ui->tx6);
    }
// end of the code area to be reworked
  } else {
    ui->tx6->clear ();
  }
}

void MainWindow::genStdMsgs(QString rpt)                       //genStdMsgs()
{
  if(!m_autoseq && m_wasAutoSeq) { m_wasAutoSeq=false; on_AutoSeqButton_clicked(true); }
  QString myrpt,t;
  m_txGenerated = m_txFirst;

  genCQMsg ();

  /* CE3TSK: WW Digi contest with no usable station grid. There is no valid exchange to
     send, so clear every message and switch Enable Tx off: with nothing to transmit the
     station must not answer anyone. genCQMsg() above has already cleared Tx6. */
  if(wwDigiNoGrid ()) {
    ui->labAz->setText("");
    ui->labDist->setText("");
    ui->tx1->setText("");
    ui->tx2->setText("");
    ui->tx3->setText("");
    ui->tx4->setText("");
    ui->tx5->setCurrentText("");
    ui->genMsg->setText("");
    if(m_enableTx || m_transmitting || m_btxok || g_iptt==1) haltTx("WW Digi contest, station grid is not set ");
    return;
  }

  QString hisCall=m_hisCall;
  setDxCallEntryColour ("#ffffff");

  if(hisCall.isEmpty ()) {
    ui->labAz->setText("");
    ui->labDist->setText("");
    ui->tx1->setText("");
    ui->tx2->setText("");
    ui->tx3->setText("");
    ui->tx4->setText("");
    ui->tx5->setCurrentText("");
    ui->genMsg->setText("");
    return;
  }

  auto const& my_callsign = m_config.my_callsign ();
//  auto is_compound = my_callsign != m_baseCall;
//  auto is_type_one = is_compound && shortList (my_callsign);
  auto const& my_grid = m_config.my_grid ().left (4);
  auto const& hisBase = Radio::base_callsign (hisCall);
  m_bMyCallStd=stdCall(my_callsign);
  m_bHisCallStd=stdCall(hisCall);
  bool bothCallsP=(my_callsign.endsWith("/P") and hisCall.endsWith("/P")); bool bothCallsR=(my_callsign.endsWith("/R") and hisCall.endsWith("/R"));
  bool mixedPR=((my_callsign.endsWith("/R") and hisCall.endsWith("/P")) or (my_callsign.endsWith("/P") and hisCall.endsWith("/R")));
  bool bothCallsCompound=(m_myCallCompound and m_hisCallCompound);
// longCompoundMix being not supported by message packing on the long callsign side
// an attempt to simplify such QSO from compound callsign side makes confusion with manipulation of the SkipTx1 option
//  bool longCompoundMix=((m_myCallCompound and !m_hisCallCompound and !m_bHisCallStd) or (m_hisCallCompound and !m_myCallCompound and !m_bMyCallStd));
  QString t0=hisBase + " " + m_baseCall + " ";

  if(m_mode.startsWith("FT")) {
    QString t0a,t0b;
    if(m_bHisCallStd and m_bMyCallStd) t0=hisCall + " " + my_callsign + " ";
    t0a="<"+hisCall + "> " + my_callsign + " ";
    t0b=hisCall + " <" + my_callsign + "> ";
    QString t {t0 + my_grid};
    if(!m_bMyCallStd) t=t0a;
    msgtype(t, ui->tx1);
    int n=rpt.toInt();
    myrpt = QString::asprintf("%+2.2d",n);
    QString t2,t3;
    QString sent=myrpt;
    /* CE3TSK: WW Digi contest - the exchange is my 4 character grid, not a signal report.
       Tx2 becomes "HISCALL MYCALL GRID" and the contest branch a few lines below builds
       Tx3 as "HISCALL MYCALL R GRID". */
    if(m_wwDigi) sent=my_grid; /* CE3TSK: the exchange is my grid, validated by wwDigiNoGrid() above */
    QString rs,rst;
    int nn=(n+36)/6;
    if(nn<2) nn=2;
    if(nn>9) nn=9;
    rst = QString::asprintf("5%1d9 ",nn);
    rs=rst.mid(0,2);
    t=t0;
    if(!mixedPR) {
      if(!m_bMyCallStd) { t=t0b; msgtype(t0a, ui->tx1); }
      if(!m_bHisCallStd) { 
        t=t0a; 
        if(m_bMyCallStd && !my_callsign.endsWith("/P")) msgtype(t0a + my_grid, ui->tx1); 
        else msgtype(t0a, ui->tx1);
      }
    }
    else msgtype("<"+hisCall + "> " + my_callsign, ui->tx1);
//    if((bothCallsCompound and !bothCallsP and !bothCallsR) or longCompoundMix) t="<"+hisCall + "> " + Radio::base_callsign(my_callsign) + " ";
    if(bothCallsCompound and !bothCallsP and !bothCallsR) t="<"+hisCall + "> " + Radio::base_callsign(my_callsign) + " ";
    msgtype(t + sent, ui->tx2);
//    if((m_houndMode and !m_bHisCallStd and !m_bMyCallStd) or (!m_houndMode and ((bothCallsCompound and !bothCallsP and !bothCallsR) or longCompoundMix)))
    if((m_houndMode and !m_bHisCallStd and !m_bMyCallStd) or (!m_houndMode and bothCallsCompound and !bothCallsP and !bothCallsR))
      t="<"+hisCall + "> " + Radio::base_callsign(my_callsign) + " ";
    if(sent==myrpt) msgtype(t + "R" + sent, ui->tx3);
    if(sent!=myrpt) msgtype(t + "R " + sent, ui->tx3);

    t=t0 + (m_rrr ? "RRR" : "RR73");
//    if((!m_bHisCallStd and m_bMyCallStd) or (bothCallsCompound and !bothCallsP and !bothCallsR) or longCompoundMix)
    if((!m_bHisCallStd and m_bMyCallStd) or (bothCallsCompound and !bothCallsP and !bothCallsR))
      t=hisCall + " <" + my_callsign + "> " + (m_rrr ? "RRR" : "RR73");
    else if((m_bHisCallStd and !m_bMyCallStd)) t="<" + hisCall + "> " + my_callsign + " " + (m_rrr ? "RRR" : "RR73");
    msgtype(t, ui->tx4);

    t=t0 + "73";
//    if((!m_bHisCallStd and m_bMyCallStd) or (bothCallsCompound and !bothCallsP and !bothCallsR) or longCompoundMix)
    if((!m_bHisCallStd and m_bMyCallStd) or (bothCallsCompound and !bothCallsP and !bothCallsR))
      t=hisCall + " <" + my_callsign + "> 73";
    else if((m_bHisCallStd and !m_bMyCallStd)) t="<" + hisCall + "> " + my_callsign + " 73";

/*    if(unconditional || hisBase != m_lastCallsign || !m_lastCallsign.size ()) {
      // only update tx5 when (forced-unconditional or ) callsign changes */
      msgtype(t, ui->tx5->lineEdit());
/*      m_lastCallsign = hisBase;
    } */

    if (m_skipTx1 && !m_houndMode) {
      m_QSOProgress = REPORT;  
      m_nlasttx=2;
      ui->txrb2->setChecked(true);
      if (0 == ui->tabWidget->currentIndex ()) { m_ntx=2; } else if (1 == ui->tabWidget->currentIndex ()) { m_ntx=7; } }
    else {
      m_QSOProgress = REPLYING;
      m_nlasttx=1;
      ui->txrb1->setChecked(true);
      if (0 == ui->tabWidget->currentIndex ()) { m_ntx=1; } else if (1 == ui->tabWidget->currentIndex ()) { m_ntx=7; } }
      m_rpt=myrpt;

    return;
  }

  t0=hisBase + " " + m_baseCall + " ";
  t=t0 + m_config.my_grid ().left(4);
  msgtype(t, ui->tx1);
//this part is VHF one and being not supported, shall we delete it? 
  if(rpt.isEmpty ()) {
    t=t+" OOO";
    msgtype(t, ui->tx2);
    msgtype("RO", ui->tx3);
    if(!m_rrr) {
      msgtype("RR73", ui->tx4);
    } else {
      msgtype("RRR", ui->tx4);
    }
    msgtype("73", ui->tx5->lineEdit ());
//end of VHF functionality
  } else {
    int n=rpt.toInt();
    myrpt = QString::asprintf("%+2.2d",n);
    t=t0 + myrpt;
    msgtype(t, ui->tx2);
    t=t0 + "R" + myrpt;
    msgtype(t, ui->tx3);
    if(!m_rrr) {
      t=t0 + "RR73";
    } else {
      t=t0 + "RRR";
    }
    msgtype(t, ui->tx4);
    t=t0 + "73";
    msgtype(t, ui->tx5->lineEdit ());
  }

  if(m_config.my_callsign () != m_baseCall) {
    if(shortList(m_config.my_callsign ())) {
      t=hisBase + " " + m_config.my_callsign ();
      msgtype(t, ui->tx1);
      t="CQ " + m_config.my_callsign ();
      msgtype(t, ui->tx6);
    } else {
      switch (m_config.type_2_msg_gen ())
        {
        case Configuration::type_2_msg_1_full:
          t="DE " + m_config.my_callsign () + " " + m_config.my_grid ().left(4);
          msgtype(t, ui->tx1);
          t=t0 + "R" + myrpt;
          msgtype(t, ui->tx3);
          break;

        case Configuration::type_2_msg_3_full:
          t = t0 + m_config.my_grid ().left(4);
          msgtype(t, ui->tx1);
          t="DE " + m_config.my_callsign () + " R" + myrpt;
          msgtype(t, ui->tx3);
          break;

        case Configuration::type_2_msg_5_only:
          t = t0 + m_config.my_grid ().left(4);
          msgtype(t, ui->tx1);
          t=t0 + "R" + myrpt;
          msgtype(t, ui->tx3);
          break;
        }
      t="DE " + m_config.my_callsign () + " 73";
      msgtype(t, ui->tx5->lineEdit ());
    }
    if (hisCall != hisBase
        && m_config.type_2_msg_gen () != Configuration::type_2_msg_5_only) {
      // cfm we have his full call copied as we could not do this earlier
      t = hisCall + " 73";
      msgtype(t, ui->tx5->lineEdit ());
    }
  } else {
    if(hisCall!=hisBase) {
      if(shortList(hisCall)) {
        // cfm we know his full call with a type 1 tx1 message
        t=hisCall + " " + m_config.my_callsign ();
        msgtype(t, ui->tx1);
      }
      else {
        t=hisCall + " 73";
        msgtype(t, ui->tx5->lineEdit());
      }
    }
  }
  
  if (m_skipTx1 && !m_houndMode) {
	m_QSOProgress = REPORT;  
    m_nlasttx=2;
    ui->txrb2->setChecked(true);
    if (0 == ui->tabWidget->currentIndex ()) { m_ntx=2; } else if (1 == ui->tabWidget->currentIndex ()) { m_ntx=7; } }
  else {
	m_QSOProgress = REPLYING;
    m_nlasttx=1;
    ui->txrb1->setChecked(true);
    if (0 == ui->tabWidget->currentIndex ()) { m_ntx=1; } else if (1 == ui->tabWidget->currentIndex ()) { m_ntx=7; } }
  m_rpt=myrpt;
  if(!ui->spotLineEdit->text().isEmpty() && ui->spotLineEdit->text().contains("#")) on_spotLineEdit_textChanged(ui->spotLineEdit->text());
}

void MainWindow::TxAgain() { enableTx_mode(true); }

/* CE3TSK: WW Digi contest - the station answered our exchange with a signal report instead
   of his grid ("CE3TSK DL6FKR R-10"). He is running ordinary FT8/FT4: neither side can send
   what the other waits for, so the QSO is over. Back to CQ if it was the station in the DX
   field, and skipped by the autoselect for five minutes either way - without that he would
   be picked again immediately, calling us being the highest priority there is. A double
   click on him overrules this. */
void MainWindow::contestReportAbort (QString const& call)
{
  QString const base = Radio::base_callsign (call);
  if (base.isEmpty ()) return;
  qint64 const now = m_jtdxtime->currentMSecsSinceEpoch2 ();
  bool const known = m_contestIgnore.has (base, now);
  m_contestIgnore.add (base, now);
  m_contestIgnore.prune (now);
  bool const working = !m_hisCall.isEmpty () && Radio::base_callsign (m_hisCall) == base;
  /* CE3TSK: out of the waiting list whatever he was - the station we are working, a station
     calling us that we have not answered yet, or one we have never heard before. A caller
     whose message to us carries a report instead of a grid is not a contest station, so he
     must not sit in the QSO history waiting to be picked; process_Auto keeps him out for as
     long as the skip list holds him. */
  m_qsoHistory.forget (call);   /* not remove(): his blacklist entry, if any, is the operator's */
  if (working)
    {
      clearDX (" cleared, " + call + " answers with a signal report - not a contest station");
      /* CE3TSK: clearDX() leaves us calling CQ, which is only the fallback. With the DX field
         empty, process_Auto()'s autoselect can answer a station that is already waiting
         instead. It runs at
         <DecodeFinished>, once all of this period's decodes are in; clearing the flag makes it
         run even if an earlier line in this same period had already triggered it, so the
         handover happens in this period and not one period later. */
      m_processAuto_done = false;
    }
  /* CE3TSK: tell the operator. The report line itself is dropped (item 21), so without this the
     station simply stops being answered and the newest thing on screen is his previous message -
     the QSO looks like it evaporated. One line in both windows, and in ALL.TXT unconditionally
     so it can be reconstructed afterwards; only the first time for each station, or a repeating
     caller would fill the window. */
  if (!known)
    {
      QString const why = call + (working ? QString {" answered with a report - not the contest exchange, QSO dropped, skipped 5 min"}
                                          : QString {" is sending reports, not the grid exchange - skipped 5 min"});
      ui->decodedTextBrowser->displayContestNotice (why);
      ui->decodedTextBrowser2->displayContestNotice (why);
      writeToALLTXT ("contest: " + why);
    }
}

void MainWindow::clearDX (QString reason)
{
  QString dxcallclr=m_hisCall;
  clearDXfields("");
  genStdMsgs(QString {});
  ui->RxFreqSpinBox->setValue (ui->TxFreqSpinBox->value ());
  if (1 == ui->tabWidget->currentIndex())
    {
      ui->genMsg->setText(ui->tx6->text());
      m_ntx=7;
      ui->rbGenMsg->setChecked(true);
    }
  else if (0 == ui->tabWidget->currentIndex())
    {
      m_ntx=6;
      ui->txrb6->setChecked(true);
    }
  m_QSOProgress = CALLING;
  m_nlasttx=6;
  if (m_spotDXsummit) {
     ui->pbSpotDXCall->setText(tr("DX Call"));
     m_spotDXsummit=false;
  }    
  m_dxCallHidden=false;   // CE3TSK
  if(m_config.spot_to_dxsummit()) { ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#c4c4ff",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
  else { ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#aabec8",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }

  if(!reason.isEmpty() && m_config.write_decoded_debug()) writeToALLTXT("DX Call " + dxcallclr + reason);
}

void MainWindow::clearDXfields (QString reason)
{
  m_name = "";
  QString dxcallclr=m_hisCall;
  if (!m_hisCall.isEmpty()) ui->dxCallEntry->clear();
  if (!m_hisGrid.isEmpty()) ui->dxGridEntry->clear();
  setDxCallEntryColour ("#ffffff");
  if(!reason.isEmpty() && m_config.write_decoded_debug()) writeToALLTXT("DX Call " + dxcallclr + reason);
}

void MainWindow::logClearDX ()
{
  clearDX (" cleared by logClearDXTimer");
  m_QSOProgress = CALLING;  // guiUpdate() every second checking m_QSOProgress based on m_ntx value for tab1, in tab2 m_ntx=7
}

void MainWindow::countQSOs ()
{
  //printing QSO count
  /* CE3TSK: while a contest runs the counter is the contest log's - the QSOs of THIS contest
     in the current mode - not the everyday log's; and the date and "Logd" labels are hidden
     meanwhile, the score label next to them says what matters and the status bar is crowded
     enough. Called
     from updateContestScore() as well, so the switch-over and every contest QSO refresh it. */
  LogBook& book = m_wwDigi ? m_contestLog : m_logBook;
  date_label->setVisible (!m_wwDigi);
  lastlogged_label->setVisible (!m_wwDigi);   /* the "Logd" last-logged label too */
  char c_txt [20];
  if (m_mode == "FT8") { sprintf(c_txt,"FT8  %d",book.get_qso_count("FT8")); }
  else if (m_mode == "FT4") { sprintf(c_txt,"FT4  %d",book.get_qso_count("FT4")); }
  else if (m_mode == "FT2") { sprintf(c_txt,"FT2  %d",book.get_qso_count("FT2")); }   // CE3TSK
  else if (m_mode == "JT9+JT65") { sprintf(c_txt,"JT65/9 %d/%d",book.get_qso_count("JT65"),book.get_qso_count("JT9")); }
  else if (m_mode == "JT9") { sprintf(c_txt,"JT9  %d",book.get_qso_count("JT9")); }
  else if (m_mode == "JT65") { sprintf(c_txt,"JT65  %d",book.get_qso_count("JT65")); }
  else if (m_mode == "T10") { sprintf(c_txt,"T10  %d",book.get_qso_count("T10")); }
  else { sprintf(c_txt," "); }
  qso_count_label->setText(c_txt);
}

void MainWindow::autoFilter (bool action) { ui->filterButton->setChecked(action); on_filterButton_clicked (action); }

void MainWindow::enableTab1TXRB(bool state)
{
  ui->txrb1->setEnabled(state && !m_wwDigi);   /* CE3TSK: the Tx1 row stays greyed in WW Digi (applyContestLock) */
  ui->txrb2->setEnabled(state);
  ui->txrb3->setEnabled(state);
  ui->txrb4->setEnabled(state);
  ui->txrb5->setEnabled(state);
  ui->txrb6->setEnabled(state);
}

void MainWindow::dxbcallTxHaltedClear () { m_dxbcallTxHalted = ""; }

void MainWindow::lookup()                                       //lookup()
{
  QString hisCall=m_hisCall;
  if (hisCall.isEmpty () || hisCall.endsWith("/P") || hisCall.endsWith("/MM") || hisCall.endsWith("/A") || hisCall.endsWith("/M")) return;
  QFile f {m_dataDir.absoluteFilePath ("CALL3.TXT")};
  if (f.open (QIODevice::ReadOnly | QIODevice::Text))
    {
      char c[132];
      qint64 n=0;
      for(int i=0; i<999999; i++) {
        n=f.readLine(c,sizeof(c));
        if(n <= 0) {
          if (!m_hisGrid.isEmpty()) ui->dxGridEntry->clear();
          break;
        }
        QString t=QString(c);
        if(t.indexOf(hisCall)==0) {
          int i1=t.indexOf(",");
          QString hisgrid=t.mid(i1+1,6);
          i1=hisgrid.indexOf(",");
          if(i1>0) {
            hisgrid=hisgrid.left(4);
          } else {
            hisgrid=hisgrid.left(4) + hisgrid.mid(4,2).toLower();
          }
          ui->dxGridEntry->setText(hisgrid);
          break;
        }
      }
      f.close();
    }
}

void MainWindow::on_lookupButton_clicked() { lookup(); }

void MainWindow::on_addButton_clicked()
{
  if(m_hisGrid.isEmpty()) {
    JTDXMessageBox::warning_message (this, "", tr ("Add to CALL3.TXT")
                                 , tr ("Please enter a valid grid locator"));
    return;
  }
  m_call3Modified=false;
  QString hisCall=m_hisCall;
  QString hisgrid=m_hisGrid;
  QString newEntry=hisCall + "," + hisgrid;
  newEntry += ",,,";
  
  QFile f1 {m_dataDir.absoluteFilePath ("CALL3.TXT")};
  if(!f1.open(QIODevice::ReadWrite | QIODevice::Text)) {
    JTDXMessageBox::warning_message (this, "", tr ("Add to CALL3.TXT")
                                 , tr ("Cannot open \"%1\" for read/write: %2")
                                 .arg (f1.fileName ()).arg (f1.errorString ()));
    return;
  }
  if(f1.size()==0) {
    QTextStream out(&f1);
    out << "ZZZZZZ" <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

    f1.close();
    f1.open(QIODevice::ReadOnly | QIODevice::Text);
  }
  QFile f2 {m_dataDir.absoluteFilePath ("CALL3.TMP")};
  if(!f2.open(QIODevice::WriteOnly | QIODevice::Text)) {
    JTDXMessageBox::warning_message (this, "", tr ("Add to CALL3.TXT")
                                 , tr ("Cannot open \"%1\" for writing: %2")
                                 .arg (f2.fileName ()).arg (f2.errorString ()));
    return;
  }
  QTextStream in(&f1);          //Read from CALL3.TXT
  QTextStream out(&f2);         //Copy into CALL3.TMP
  QString hc=hisCall;
  QString hc1="";
  QString hc2="000000";
  QString s;
  do {
    s=in.readLine();
    hc1=hc2;
    if(s.left(2)=="//") {
      out << s + QChar::LineFeed;          //Copy all comment lines
    } else {
      int i1=s.indexOf(",");
      hc2=s.left(i1);
      if(hc>hc1 && hc<hc2) {
        out << newEntry + QChar::LineFeed;
        out << s + QChar::LineFeed;
        m_call3Modified=true;
      } else if(hc==hc2) {
        QString t {tr ("%1\nis already in CALL3.TXT"
                       ", do you wish to replace it?").arg (s)};
        int ret = JTDXMessageBox::query_message (this, "", tr ("Add to CALL3.TXT"), t);
        if(ret==JTDXMessageBox::Yes) {
          out << newEntry + QChar::LineFeed;
          m_call3Modified=true;
        }
      } else {
        if(!s.isEmpty ()) out << s + QChar::LineFeed;
      }
    }
  } while(!s.isNull());

  f1.close();
  if(hc>hc1 && !m_call3Modified) out << newEntry + QChar::LineFeed;
  if(m_call3Modified) {
    QFile f0 {m_dataDir.absoluteFilePath ("CALL3.OLD")};
    if(f0.exists()) f0.remove();
    QFile f1 {m_dataDir.absoluteFilePath ("CALL3.TXT")};
    f1.rename(m_dataDir.absoluteFilePath ("CALL3.OLD"));
    f2.rename(m_dataDir.absoluteFilePath ("CALL3.TXT"));
    f2.close();
  }
}

void MainWindow::msgtype(QString t, QLineEdit* tx)               //msgtype()
{
  char message[38];
  char msgsent[38];
  int len1=22;
  QByteArray s=t.toUpper().toLocal8Bit();
  ba2msg(s,message);
  int ichk=1,itype=0;
  gen9_(message,&ichk,msgsent,const_cast<int *>(itone),&itype,len1,len1);
  msgsent[22]=0;
  bool text=false;
  if(itype==6) text=true;
  /* CE3TSK: WW Digi contest - "HISCALL MYCALL R GRID" is a valid FT8 77 bit message
     (i3=1), but gen9() above is the old 72 bit packer and reports it as free text, which
     would paint the Tx3 field pink and read as an error to the operator. */
  if(text && m_wwDigi && m_mode.startsWith("FT")) {
    QStringList w=t.toUpper().split(" ",SkipEmptyParts);
    if(w.size()==4 && w[2]=="R" && w[3].length()==4 && gridOK(w[3])) text=false;
  }
  QPalette p(tx->palette());
  if(text) {
    p.setColor(QPalette::Base,Radio::convert_dark("#ffccff",m_useDarkStyle));
  } else {
    p.setColor(QPalette::Base,Radio::convert_dark("#ffffff",m_useDarkStyle));
  }
  tx->setPalette(p);
  auto pos  = tx->cursorPosition ();
  tx->setText(t.toUpper());
  tx->setCursorPosition (pos);
}

void MainWindow::on_tx1_editingFinished() { QString t=ui->tx1->text(); msgtype(t, ui->tx1); }
void MainWindow::on_tx2_editingFinished() { QString t=ui->tx2->text(); msgtype(t, ui->tx2); }
void MainWindow::on_tx3_editingFinished() { QString t=ui->tx3->text(); msgtype(t, ui->tx3); }
void MainWindow::on_tx4_editingFinished() { QString t=ui->tx4->text(); msgtype(t, ui->tx4); }

void MainWindow::on_tx5_currentTextChanged (QString const& text) //tx5 edited
{
  bool isAllowedAuto73=isAutoSeq73(text);
  if(!m_Tx5setAutoSeqOff && !isAllowedAuto73) m_Tx5setAutoSeqOff=true;
  if(isAllowedAuto73) m_Tx5setAutoSeqOff=false;
  if(!text.contains(QRegularExpression {R"([@#&^])"}) && !text.isEmpty()) {
    QString t="161545  -4  0.1 1939 & " + text;
    DecodedText decodedtext {t,this};
//      DecodedText decodedtext {"161545  -4  0.1 1939 & CQ RT9K/4    "};
    bool stdfreemsg = decodedtext.isStandardMessage();
    if(stdfreemsg) {
      ui->tx5->setStyleSheet(QString("QComboBox {background: %1}").arg(Radio::convert_dark("#7bff7b",m_useDarkStyle)));
    } else {
      if(!text.isEmpty () && text.length() < 14) { ui->tx5->setStyleSheet(QString("QComboBox {background: %1}").arg(Radio::convert_dark("#7bff7b",m_useDarkStyle))); }
      else if(text.length() >= 14) { ui->tx5->setStyleSheet(QString("QComboBox {background: %1}").arg(Radio::convert_dark("#ffff00",m_useDarkStyle))); }
      msgtype(text, ui->tx5->lineEdit ());
    }
  } else ui->tx5->setStyleSheet(QString("QComboBox {background: %1}").arg(Radio::convert_dark("#ffffff",m_useDarkStyle)));
}

void MainWindow::on_tx5_currentIndexChanged(int index)
{
//it is dedicated to change free message during Tx if new message is selected from the list
  if(m_transmitting && ui->txrb5->isChecked() && index != m_oldTx5Index) m_restart=true;
  m_oldTx5Index=index;
}

void MainWindow::on_tx6_editingFinished() { QString t=ui->tx6->text(); msgtype(t, ui->tx6); }

void MainWindow::on_wantedCall_textChanged(const QString &wcall)
{
  m_wantedCall = wcall.trimmed().toUpper();
  if (ui->wantedCall->text() != m_wantedCall) {
    auto pos = ui->wantedCall->cursorPosition ();
    ui->wantedCall->setText(m_wantedCall);
    ui->wantedCall->setCursorPosition (pos);
  } else if (m_wantedCall.size() < 3){ 
    m_wantedCallList.clear();
  } else { 
    m_wantedCallList = m_wantedCall.split(',');
  }
}

void MainWindow::on_wantedCountry_textChanged(const QString &wcountry)
{
  m_wantedCountry = wcountry.trimmed().toUpper();
  if (ui->wantedCountry->text() != m_wantedCountry) {
    auto pos = ui->wantedCountry->cursorPosition ();
    ui->wantedCountry->setText(m_wantedCountry);
    ui->wantedCountry->setCursorPosition (pos);
  } else if (m_wantedCountry.size() < 1){ 
    m_wantedCountryList.clear();
  } else { 
    m_wantedCountryList = m_wantedCountry.split(',');
  }
}

void MainWindow::on_wantedPrefix_textChanged(const QString &wprefix)
{
  m_wantedPrefix = wprefix.trimmed().toUpper();
  if (ui->wantedPrefix->text() != m_wantedPrefix) {
    auto pos = ui->wantedPrefix->cursorPosition ();
    ui->wantedPrefix->setText(m_wantedPrefix);
    ui->wantedPrefix->setCursorPosition (pos);
  } else if (m_wantedPrefix.size() < 2) {
    m_wantedPrefixList.clear();
  } else {
    m_wantedPrefixList = m_wantedPrefix.split(',');
  }
}

void MainWindow::on_wantedGrid_textChanged(const QString &wgrid)
{
  m_wantedGrid = wgrid.trimmed().toUpper();
  if (ui->wantedGrid->text() != m_wantedGrid) {
    auto pos = ui->wantedGrid->cursorPosition ();
    ui->wantedGrid->setText(m_wantedGrid);
    ui->wantedGrid->setCursorPosition (pos);
  } else if (m_wantedGrid.size() < 4) {
    m_wantedGridList.clear();
  } else {
    m_wantedGridList = m_wantedGrid.split(',');
  }
}

void MainWindow::on_directionLineEdit_textChanged(const QString &dir) //CQ direction changed
{
  QString cqdirection = dir.toUpper();
  if(cqdirection.isEmpty ()) { ui->directionLineEdit->setStyleSheet(QString("QLineEdit {background: %1}").arg(Radio::convert_dark("#ffffff",m_useDarkStyle))); }
  else if(cqdirection.length() == 1 || cqdirection.length() > 2) { ui->directionLineEdit->setStyleSheet(QString("QLineEdit {background: %1}").arg(Radio::convert_dark("#fffa82",m_useDarkStyle))); }
  else if(cqdirection.length() == 2) { ui->directionLineEdit->setStyleSheet(QString("QLineEdit {background: %1}").arg(Radio::convert_dark("#82ff8c",m_useDarkStyle))); }
  m_cqdir = cqdirection;
  if (cqdirection.isEmpty ()) { ui->pbCallCQ->setText("CQ"); }
  else if (cqdirection.length() == 2) { ui->pbCallCQ->setText("CQ " + m_cqdir); }
  auto curpos = ui->directionLineEdit->cursorPosition(); ui->directionLineEdit->setText(m_cqdir);
  ui->direction1LineEdit->setText(m_cqdir); ui->directionLineEdit->setCursorPosition(curpos);
  if (cqdirection.isEmpty () || cqdirection.length() == 2) {
      if(ui->genMsg->text().startsWith ("CQ ") && ui->txrb6->isChecked()) {
         ui->pbCallCQ->click ();
      }
  }
}

void MainWindow::on_direction1LineEdit_textChanged(const QString &dir) //CQ direction changed
{
  QString cqdirection = dir.toUpper();
  if(cqdirection.isEmpty ()) { ui->direction1LineEdit->setStyleSheet(QString("QLineEdit {background: %1}").arg(Radio::convert_dark("#ffffff",m_useDarkStyle))); }
  else if(cqdirection.length() == 1 || cqdirection.length() > 2) { ui->direction1LineEdit->setStyleSheet(QString("QLineEdit {background: %1}").arg(Radio::convert_dark("#fffa82",m_useDarkStyle))); }
  else if(cqdirection.length() == 2) { ui->direction1LineEdit->setStyleSheet(QString("QLineEdit {background: %1}").arg(Radio::convert_dark("#82ff8c",m_useDarkStyle))); }
  m_cqdir = cqdirection;
  if (cqdirection.isEmpty ()) { ui->pbCallCQ->setText("CQ"); }
  else if (cqdirection.length() == 2) { ui->pbCallCQ->setText("CQ " + m_cqdir); }
  auto curpos = ui->direction1LineEdit->cursorPosition(); ui->direction1LineEdit->setText(m_cqdir);
  ui->directionLineEdit->setText(m_cqdir); ui->direction1LineEdit->setCursorPosition(curpos);
  if (cqdirection.isEmpty () || cqdirection.length() == 2) {
      if(ui->tx1->text().isEmpty()) { if(ui->txrb6->isChecked()) ui->txb6->click (); }
      else { 
        if(cqdirection.length() == 2) ui->tx6->setText("CQ " + m_cqdir + " " + m_config.my_callsign() + " " + m_config.my_grid().left(4));
        else ui->tx6->setText("CQ " + m_config.my_callsign() + " " + m_config.my_grid().left(4));
      }
  }
}

void MainWindow::on_spotLineEdit_textChanged(const QString &text)
{
  m_spotText=text;
  QString spotTextTmp=text;
  spotTextTmp.replace("#G",m_config.my_grid() + "<" + ui->propLineEdit->text() + ">" + ui->dxGridEntry->text(), Qt::CaseInsensitive);
  spotTextTmp.replace("#D", ui->labDist->text().replace(" ",""), Qt::CaseInsensitive);
  spotTextTmp.replace("#R", m_rpt+"dB", Qt::CaseInsensitive);
  spotTextTmp.replace("#H", m_houndMode ? "F/H" : "");
  QString spotText;
  if(ui->spotLineEdit->text().isEmpty() && ui->propLineEdit->text().isEmpty()) { spotText="info: " + m_modeTx; }
  else if(ui->spotLineEdit->text().isEmpty() && !ui->propLineEdit->text().isEmpty()) { spotText="info: " + m_modeTx + " " + ui->propLineEdit->text(); }
  else if(!m_spotText.contains("#G", Qt::CaseInsensitive)) { spotText="info: " + m_modeTx + " " + ui->propLineEdit->text() + " " + spotTextTmp; }
  else { spotText="info: " + m_modeTx + " " + spotTextTmp; }
  ui->spotMsgLabel->setText(spotText);
}

void MainWindow::on_propLineEdit_textChanged(const QString &text) {
  if(text != text.toUpper()) ui->propLineEdit->setText(text.toUpper());
  on_spotLineEdit_textChanged(ui->spotLineEdit->text());
}

void MainWindow::on_dxCallEntry_textChanged(const QString &t) //dxCall changed
{
  int n=t.length();
  if (n < 3 ) {
      if (t != t.toUpper().trimmed()) ui->dxCallEntry->setText(t.toUpper().trimmed());
      if (m_hisCall.isEmpty()) { m_hisCallCompound=false; return; }
      else m_hisCall.clear();
  } else m_hisCall=t.toUpper().trimmed();
  m_hisCallCompound=(!m_hisCall.isEmpty() && m_hisCall.contains("/")); // && !m_hisCall.endsWith("/P") && !m_hisCall.endsWith("/R"));
  if(m_myCallCompound && m_hisCallCompound) {
    if(m_skipTx1) { m_skipTx1=false; ui->skipTx1->setChecked(false); ui->skipGrid->setChecked(false); on_txb1_clicked(); }
    ui->skipTx1->setEnabled(false); ui->skipGrid->setEnabled(false);
  }
  /* CE3TSK: !m_wwDigi - this runs on every DX call change, so without it the contest lock
     would be handed back the moment a callsign is entered. */
  else { if(!m_houndMode && !m_wwDigi) {ui->skipTx1->setEnabled(true); ui->skipGrid->setEnabled(true); }}
  auto pos = ui->dxCallEntry->cursorPosition (); 
  if (t != m_hisCall && !m_hisCall.isEmpty()) { ui->dxCallEntry->setText(m_hisCall); ui->dxCallEntry->setCursorPosition (pos); }
  else {
      QString grid;
      if (m_logBook.getData(m_hisCall,grid,m_name)) {
//            printf("DXCall log_data %s %s %s\n",m_hisCall.toStdString().c_str(),grid.toStdString().c_str(),m_name.toStdString().c_str());
            if (!grid.isEmpty()) {
              ui->dxGridEntry->setText(grid);
            }
          } else {
             m_name = "";
          }
      setDxCallEntryColour ("#ffffff");
      if (logClearDXTimer.isActive()) logClearDXTimer.stop();
      // Refresh Tx macros
      QStringListModel* model1 = m_config.macros();
      QStringList list1 = model1->stringList();
      QStringList list2 = list1.replaceInStrings("@", m_hisCall);
      list2 = list2.replaceInStrings("&", m_baseCall);
      list2 = list2.replaceInStrings("#", m_rpt);
      QString name=m_name.trimmed().split(" ").at(0);
      if(!name.isEmpty() && name.length()<=7) { 
        name = name.toUpper();
        QRegularExpression name_re("^[A-Z]{2,7}$"); QRegularExpressionMatch match = name_re.match(name);
        bool hasMatch = match.hasMatch();
        if(hasMatch) list2 = list2.replaceInStrings("^", name); 
      }
      QStringListModel* model2 = new QStringListModel(list2);
      ui->tx5->setModel(model2);
      ui->freeTextMsg->setModel(model2);
      if (m_spotDXsummit){
         ui->pbSpotDXCall->setText(tr("DX Call"));
         m_spotDXsummit=false;
      }
      m_dxCallHidden=false;   // CE3TSK
      if(m_config.spot_to_dxsummit()) { ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#c4c4ff",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
      else { ui->pbSpotDXCall->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: outset;border-width: 1px;border-color: %3;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#aabec8",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle))); }
      m_bHisCallStd=stdCall(m_hisCall);
      statusChanged();
  }
}

void MainWindow::on_dxGridEntry_textChanged(const QString &t) //dxGrid changed
{
  int n=t.length();
  if(n!=4 and n!=6 and n!=8 and n!=10) {
    if (n < 4 || n==5) {
        if (t != t.left(2).toUpper() + t.mid(2,2) + t.mid(4,1).toLower()) ui->dxGridEntry->setText(t.left(2).toUpper() + t.mid(2,2) + t.mid(4,1).toLower());
        /* CE3TSK: m_dxPoints goes with the grid it was computed from. It is assigned in one
           place only, the 4/6/8/10 character branch below, so without this a station worth
           3 points logged and then cleared left those 3 points on the next QSO logged
           without a grid - the stale-global class logfields.h exists to remove. */
        if (n < 4 && !m_hisGrid.isEmpty()) { ui->labAz->clear(); ui->labDist->clear(); m_hisGrid.clear(); m_dxPoints=0; statusUpdate (); }
    } else if (n==7){
        if (t != t.left(2).toUpper() + t.mid(2,2) + t.mid(4,2).toLower() + t.mid(6,1)) ui->dxGridEntry->setText(t.left(2).toUpper() + t.mid(2,2) + t.mid(4,2).toLower() + t.mid(6,1));
    } else if (n==9){
        if (t != t.left(2).toUpper() + t.mid(2,2) + t.mid(4,2).toLower() + t.mid(6,2) + t.mid(8,1).toLower()) ui->dxGridEntry->setText(t.left(2).toUpper() + t.mid(2,2) + t.mid(4,2).toLower() + t.mid(6,2) + t.mid(8,1).toLower());
    }  
 
    return;
  }
  m_hisGrid=t.left(2).toUpper() + t.mid(2,2) + t.mid(4,2).toLower() + t.mid(6,2) + t.mid(8,2).toLower();
  auto pos = ui->dxGridEntry->cursorPosition ();
  if (t != m_hisGrid) { ui->dxGridEntry->setText(m_hisGrid); ui->dxGridEntry->setCursorPosition (pos); }
  else {
        statusUpdate ();
        qint64 nsec = m_jtdxtime->currentMSecsSinceEpoch2() % 86400;
        double utch=nsec/3600.0;
          int nAz=0,nEl=0,nDmiles=0,nDkm=0,nHotAz,nHotABetter;
          /* CE3TSK: in a contest both grids are cut to the 4 characters the exchange
             carries, through contest_grid() so the DX panel and the per decode scoring use
             one rule. Scoring from a 6 character home grid would compute a distance the
             other station cannot reproduce from what we sent him. The points come from this
             same nDkm, so the distance shown, the distance logged and the score agree. */
          QString const myG = contest_grid (m_config.my_grid (), m_wwDigi);
          QString const hisG = contest_grid (m_hisGrid, m_wwDigi);
          bool const distanceKnown = !hisG.isEmpty() && !myG.isEmpty();
          if (distanceKnown) {
            /* azdist pads these buffers in place, so they must be mutable locals */
            QByteArray a = (myG + "        ").left (8).toLatin1();
            QByteArray b = (hisG + "        ").left (8).toLatin1();
            azdist_(a.data(), b.data(), &utch,
                  &nAz,&nEl,&nDmiles,&nDkm,&nHotAz,&nHotABetter,8,8);
          }
          /* nDkm is 0 when unknown, and 0 km scores a point, so gate on distanceKnown */
          m_dxPoints = distanceKnown ? m_config.special_op_points (nDkm) : 0;
        QString t;
        t = QString::asprintf("Az: %d",nAz);
        ui->labAz->setText(t);
        if (m_config.miles ()) { t = QString::asprintf ("%d mi", nDmiles); }
        else { t = QString::asprintf ("%d km", nDkm); }
        ui->labDist->setText(t);
  }
  if(!ui->spotLineEdit->text().isEmpty() && ui->spotLineEdit->text().contains("#")) on_spotLineEdit_textChanged(ui->spotLineEdit->text());
}

void MainWindow::on_genStdMsgsPushButton_clicked() { genStdMsgs(m_rpt); }

void MainWindow::on_logQSOButton_clicked()
{
  if (m_hisCall.isEmpty()) return;
  auto currenttime = m_jtdxtime->currentDateTimeUtc2();
  auto dateTimeQSOOff = currenttime;
  QString rrep,srep,distance;
  unsigned time = 0;

  if (!m_houndMode && (m_config.prompt_to_log() || m_config.autolog())) {
    if(m_mode == "FT8") dateTimeQSOOff = currenttime.addSecs (14);
    else if(m_mode == "FT4") dateTimeQSOOff = currenttime.addSecs (7);
    else if(m_mode == "FT2") dateTimeQSOOff = currenttime.addSecs (3);   // CE3TSK
    else dateTimeQSOOff = currenttime.addSecs (50);
  }
  if (dateTimeQSOOff < m_dateTimeQSOOn) m_dateTimeQSOOn = dateTimeQSOOff;
  // 75 for FT4, 150 seconds delta for FT8, 600 seconds delta for other modes
  if (qAbs(dateTimeQSOOff.toMSecsSinceEpoch() - m_dateTimeQSOOn.toMSecsSinceEpoch()) > int(m_TRperiod) * 10000) m_dateTimeQSOOn = dateTimeQSOOff;
  bool autolog = false;
  if(m_logqso73) autolog = m_config.autolog();
  distance=ui->labDist->text();
  /* CE3TSK: the grid the station actually sent is in the QSO history; m_hisGrid is only the DX
     box, which clearDX empties - so a QSO completed from a late re-selection logged without a
     gridsquare although the grid had been decoded (logfields.h, WW_DIGI_CONTEST_SUPPORT 6b) */
  QString hisGridLogged = m_hisGrid;
  { QString g; m_qsoHistory.status (m_hisCall, g); hisGridLogged = log_grid (m_hisGrid, g); }
  if (m_qsoHistory.log_data(m_hisCall,time,rrep,srep) > QsoHistory::SREPORT) {
      if (log_time_known (time)) {
          currenttime.setTime(QTime::fromMSecsSinceStartOfDay(time*1000));
          if (dateTimeQSOOff < currenttime) currenttime = currenttime.addSecs(-86400);
      }
      /* CE3TSK: no start recorded - leave currenttime as it is, which is now. Contest checking
         tolerates minutes of difference, so "the moment we logged it" is always inside
         tolerance, while m_dateTimeQSOOn is a global that may still hold the previous QSO's
         start - the same stale-global mistake as the exchange below. Midnight, which the old
         `time < 86400` test produced, is inside nothing. */
      rrep = log_exchange (rrep, m_rptRcvd, m_wwDigi);
      if (srep.isEmpty()) srep=m_rptSent;
      m_logDlg->initLogQSO (m_hisCall, hisGridLogged, m_modeTx, srep, rrep, distance, m_name,
                        currenttime, dateTimeQSOOff, m_freqNominal + ui->TxFreqSpinBox->value(),autolog,m_dxPoints);
  } else { 
      m_logDlg->initLogQSO (m_hisCall, hisGridLogged, m_modeTx, m_rptSent,
                        log_exchange (QString {}, m_rptRcvd, m_wwDigi), distance, m_name,
                        m_dateTimeQSOOn, dateTimeQSOOff, m_freqNominal + ui->TxFreqSpinBox->value(),autolog,m_dxPoints);
  }
  m_logqso73=false;
}

void MainWindow::acceptQSO2(QDateTime const& QSO_date_off, QString const& call, QString const& grid
                            , Frequency dial_freq, QString const& mode
                            , QString const& rpt_sent, QString const& rpt_received
                            , QString const& tx_power, QString const& comments
                            , QString const& name, QDateTime const& QSO_date_on
                            , QString const& eqslcomments, QByteArray const& myadif2)
{
  QString date = QSO_date_on.toString("yyyyMMdd");
  m_qsoLogged=true;
  m_logBook.addAsWorked (call, m_config.bands ()->find (dial_freq), mode, date, grid, name);
  /* CE3TSK: the contest log has its own hashes; without this the multiplier and dupe
     highlighting would not reflect a QSO until the next restart. */
  if (m_wwDigi) m_contestLog.addAsWorked (call, m_config.bands ()->find (dial_freq), mode, date, grid, name);
  if (m_wwDigi) updateContestScore ();   /* CE3TSK */
  QString operator_call = m_config.my_callsign(); QString my_call = m_config.my_callsign(); QString my_grid = m_config.my_grid();
  m_messageClient->qso_logged (QSO_date_off, call, grid, dial_freq, mode, rpt_sent, rpt_received, tx_power, comments, name, QSO_date_on, operator_call, my_call, my_grid);
  if(m_config.enable_udp1_adif_sending()) m_messageClient->logged_ADIF(myadif2);
  if(m_config.enable_udp2_broadcast() && m_config.valid_udp2()) {
    QUdpSocket sock;
    if(-1 == sock.writeDatagram (myadif2, QHostAddress {m_config.udp2_server_name()}, m_config.udp2_server_port()))
      { JTDXMessageBox::warning_message (this, "", tr ("Error sending QSO ADIF data to secondary UDP server"), tr ("Write returned \"%1\"").arg (sock.errorString ())); }
  }
  if (m_config.send_to_eqsl())
      Eqsl->upload(m_config.eqsl_username(),m_config.eqsl_passwd(),m_config.eqsl_nickname(),call,mode,QSO_date_on,rpt_sent,m_config.bands ()->find (dial_freq),eqslcomments);
  setDxCallEntryColour ("#7fff7f");
  m_lastloggedcall=call;
  m_lastloggedtime=m_jtdxtime->currentDateTimeUtc2();
  if (m_config.clear_DX () && !logClearDXTimer.isActive() && !m_autoTx && !m_autoseq) logClearDXTimer.start ((qAbs(int(m_TRperiod)-m_nseq))*1000);
  countQSOs ();
  writeToALLTXT("QSO logged: " + m_lastloggedcall);
  setLastLogdLabel();
//clean up wanted call from window/list if QSO with this call is logged in
  if(ui->cbClearCallsign->isChecked ()) {
    int wcallidx=-1;
    if(m_mode.startsWith("FT")) wcallidx=m_wantedCallList.indexOf(call);
    else wcallidx=m_wantedCallList.indexOf(Radio::base_callsign (call));
    if(wcallidx >= 0) {
      m_wantedCallList.removeAt(wcallidx); ui->wantedCall->setText(m_wantedCallList.join(","));
    }
  }
//clean up wanted grid from window/list if QSO with this grid is logged in
  if(ui->cbClearGrid->isChecked () && grid.trimmed().length()==4) {
    int wgrididx=-1;
    wgrididx=m_wantedGridList.indexOf(grid);
    if(wgrididx >= 0) {
      m_wantedGridList.removeAt(wgrididx); ui->wantedGrid->setText(m_wantedGridList.join(","));
    }
  }
  if (m_houndMode && !m_hisCall.isEmpty()) { clearDX (" cleared: QSO logged in DXpedition mode"); setDxCallEntryColour ("#ffffff"); }
}

void MainWindow::on_actionJT9_triggered()
{
  if (m_mode=="WSPR-2") killFile();
  m_mode="JT9";
  WSPR_config(false);
  switch_mode (Modes::JT9);
  if(m_modeTx!="JT9") on_pbTxMode_clicked();
  m_hsymStop=173; if(m_config.decode_at_52s()) m_hsymStop=179;
  setModeLabelStyle ("JT9");
  ui->actionJT9->setChecked(true);
  ui->pbTxMode->setText("Tx JT9  @");
  ui->pbTxMode->setEnabled(false);
  m_TRperiod=60.0;
  commonActions();
  enableHoundAccess(false);
}

void MainWindow::on_actionT10_triggered()
{
  if (m_mode=="WSPR-2") killFile();
  m_mode="T10";
  WSPR_config(false);
  switch_mode (Modes::T10);
  m_modeTx="T10";
  m_hsymStop=173; if(m_config.decode_at_52s()) m_hsymStop=179;
  setModeLabelStyle ("T10");
  ui->actionT10->setChecked(true);
  ui->pbTxMode->setText("Tx T10  +");
  ui->pbTxMode->setEnabled(false);
  m_TRperiod=60.0;
  commonActions();
  enableHoundAccess(false);
}

void MainWindow::on_actionFT4_triggered()
{
  if (m_mode=="WSPR-2") killFile();
  m_mode="FT4";
  WSPR_config(false);
  switch_mode (Modes::FT4);
  m_modeTx="FT4";
  m_hsymStop=21;
  setModeLabelStyle ("FT4"); //to be changed
  ui->actionFT4->setChecked(true);
  ui->pbTxMode->setText("Tx FT4 :");
  ui->pbTxMode->setEnabled(false);
  on_AutoSeqButton_clicked(true);
  m_TRperiod=7.5;
  if(!m_hint) ui->hintButton->click();
  commonActions();
  enableHoundAccess(false);
}

/* CE3TSK: FT2 - FT4's frame at twice the rate, 3.75 s periods. The decoder reaches it through
   nmode 52 and does the work in the FT4 chain (decoder.f90, which stretches the period x2), and
   since step 6 it transmits as well: the same genft4 coding and gen_ft4wave waveform as FT4, at
   288 samples per symbol, started 150 ms into the period - the instant WSJT-X improved's own FT2
   transmitter uses (Modulator.cpp, `if(mode=="FT2") delay_ms=150;`), verified against its source. */
void MainWindow::on_actionFT2_triggered()
{
  if (m_mode=="WSPR-2") killFile();
  m_mode="FT2";
  WSPR_config(false);
  switch_mode (Modes::FT2);
  m_modeTx="FT2";
  m_hsymStop=22;                 // 22*1728 = 38016 samples, the 36864 the decoder stretches plus a margin
  setModeLabelStyle ("FT2");
  ui->actionFT2->setChecked(true);
  ui->pbTxMode->setText("Tx FT2 ;");
  ui->pbTxMode->setEnabled(false);
  on_AutoSeqButton_clicked(true);
  m_TRperiod=3.75;
  if(!m_hint) ui->hintButton->click();
  commonActions();
  enableHoundAccess(false);
}

void MainWindow::on_actionFT8_triggered()
{
  if (m_mode=="WSPR-2") killFile();
  m_mode="FT8";
  WSPR_config(false);
  switch_mode (Modes::FT8);
  m_modeTx="FT8";
  m_hsymStop=50;
  setModeLabelStyle ("FT8");
  ui->actionFT8->setChecked(true);
  ui->pbTxMode->setText("Tx FT8 ~");
  ui->pbTxMode->setEnabled(false);
  on_AutoSeqButton_clicked(true);
  m_TRperiod=15.0;
  commonActions();
  if(!m_hint) ui->hintButton->click();
  ui->hintButton->setEnabled(false); ui->hintButton->setVisible(false);
  ui->candListSpinBox->setEnabled(true); ui->DTCenterSpinBox->setEnabled(true);
  ui->DTCenterSpinBox->setVisible(true); ui->pbTxMode->setVisible(false);
  enableHoundAccess(true);
}

void MainWindow::on_actionJT65_triggered()
{
  if (m_mode=="WSPR-2") killFile();
  if(m_mode.startsWith("FT") or m_mode=="T10" or m_mode.left(4)=="WSPR") {
// If coming from FT,T10 or WSPR mode, pretend temporarily that we're coming
// from JT9 and click the pbTxMode button
    m_modeTx="JT9";
    on_pbTxMode_clicked();
  }
  m_mode="JT65";
  WSPR_config(false);
  switch_mode (Modes::JT65);
  if(m_modeTx!="JT65") on_pbTxMode_clicked();
  m_TRperiod=60.0;
  m_hsymStop=173; if(m_config.decode_at_52s()) m_hsymStop=179;
  setModeLabelStyle ("JT65");
  ui->actionJT65->setChecked(true);
  ui->pbTxMode->setText("Tx JT65  #");
  ui->pbTxMode->setEnabled(false);
  commonActions();
  enableHoundAccess(false);
}

void MainWindow::on_actionJT9_JT65_triggered()
{
  if (m_mode=="WSPR-2") killFile();
  m_mode="JT9+JT65";
  WSPR_config(false);
  switch_mode (Modes::JT65);
  ui->pbTxMode->setText("Tx JT65  #"); ui->pbTxMode->setEnabled(true);
  m_modeTx="JT65";
  m_TRperiod=60.0;
  m_hsymStop=173; if(m_config.decode_at_52s()) m_hsymStop=179;
  setModeLabelStyle ("JT9+JT65");
  ui->actionJT9_JT65->setChecked(true);
  commonActions();
  enableHoundAccess(false);
}

void MainWindow::on_actionWSPR_2_triggered()
{
  m_mode="WSPR-2";
  WSPR_config(true);
  switch_mode (Modes::WSPR);
  m_modeTx="WSPR-2";                                    //### not needed ?? ###
  m_TRperiod=120.0;
  if (m_tci) Q_EMIT m_config.transceiver_period(m_TRperiod); // TODO - not thread safe
  else {  m_modulator->setPeriod(m_TRperiod); m_detector->setPeriod(m_TRperiod); }// TODO - not thread safe
  m_nsps=6912;                   //For symspec only
  m_FFTSize = m_nsps / 2;
  if (m_tci) Q_EMIT m_config.transceiver_blocksize (m_FFTSize);
  else Q_EMIT FFTSize (m_FFTSize);
  m_hsymStop=396;
  m_toneSpacing=12000.0/8192.0;
  setModeLabelStyle ("WSPR-2");
  mode_label->setText(m_mode);
  ui->actionWSPR_2->setChecked(true);
  m_wideGraph->setPeriod(m_TRperiod,m_nsps);
  m_wideGraph->setMode(m_mode);
  m_wideGraph->setModeTx(m_modeTx);
  ui->TxFreqSpinBox->setValue(ui->WSPRfreqSpinBox->value());
  ui->pbTxMode->setText(tr("Tx WSPR"));
  ui->pbTxMode->setEnabled(false);
  setMinButton();
  ui->TxMinuteButton->setEnabled(false); ui->candListSpinBox->setEnabled(false);
  ui->DTCenterSpinBox->setEnabled(false); ui->DTCenterSpinBox->setVisible(false);
  setClockStyle(true);
  progressBar->setMaximum(int(m_TRperiod));
  progressBar->setFormat("%v/%m");
  statusChanged();
  on_spotLineEdit_textChanged(ui->spotLineEdit->text());
}

void MainWindow::switch_mode (Mode mode)
{
// m_lastMode value is deliberately not assigned in constructor to let qsohistory init at SW startup 
  if(m_lastMode!=m_mode) {
     if (m_lastMode == "FT4") Q_EMIT m_config.transceiver_ft4_mode (false);
     else if (m_mode == "FT4") Q_EMIT m_config.transceiver_ft4_mode (true);
     m_qsoHistory.init(); 
     m_lastMode=m_mode; 
     if(m_config.write_decoded_debug()) writeToALLTXT("QSO history initialized by switch_mode");
  } 
  m_okToPost = false;
  m_config.frequencies ()->filter (m_config.region (),mode);
  auto const& row = m_config.frequencies ()->best_working_frequency (m_freqNominal);
  if (row >= 0) {
    ui->bandComboBox->setCurrentIndex (row);
    on_bandComboBox_activated (row);
  }
  if (!m_mode.startsWith ("WSPR")) {
	countQSOs ();
	if (m_config.prompt_to_log ()) { qso_count_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#99ff99",m_useDarkStyle))); }
	else if (m_config.autolog ()) { qso_count_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#9999ff",m_useDarkStyle))); }
	else { qso_count_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#ffffff",m_useDarkStyle))); }
  }
  refreshDecodePreset();   // P13: the preset lamp is greyed outside FT8 - every mode slot comes through here with m_mode set
}

void MainWindow::commonActions ()
{
  /* CE3TSK: every mode change funnels through here, and the AutoSeq button's colour depends on
     the mode - with AutoSeq off it is grey in a non FT mode but light red in an FT one. None
     of the seven mode slots refreshed it, so switching mode left the button coloured for the
     mode being left; the contest restore made that visible by changing mode and button state
     together, but it is reachable by switching mode by hand with AutoSeq off. */
  setAutoSeqButtonStyle (m_autoseq);
//  m_modulator->setPeriod(m_TRperiod); // TODO - not thread safe
//  m_detector->setPeriod(m_TRperiod);   // TODO - not thread safe
  m_nsps=6912;                   //For symspec only
  if (m_mode=="FT2") m_nsps=3456;   // CE3TSK: half the block, so the waterfall keeps its rate in a half-length period
  m_FFTSize = m_nsps / 2;
  if (m_tci) Q_EMIT m_config.transceiver_blocksize(m_FFTSize);
  else Q_EMIT FFTSize (m_FFTSize);
  m_toneSpacing=0.0;
  m_wideGraph->setMode(m_mode);
  m_wideGraph->setModeTx(m_modeTx);
  m_wideGraph->show();
  mode_label->setText(m_mode);
  QString t;
  if (m_mode.startsWith("FT")) t = "UTC     dB   DT "+tr("Freq   Message");
  else t = "UTC   dB   DT "+tr("Freq   Message");
  ui->decodedTextLabel->setTextFormat(Qt::PlainText); ui->decodedTextLabel->setText(t);
  setBandLabelColour ("#fdedc5");
  ui->label_6->setText(tr("Band"));
  ui->decodedTextLabel2->setText(t);
  m_wideGraph->setPeriod(m_TRperiod,m_nsps);
  if (m_tci) Q_EMIT m_config.transceiver_period(m_TRperiod); // TODO - not thread safe
  else {m_modulator->setPeriod(m_TRperiod); m_detector->setPeriod(m_TRperiod); }  // TODO - not thread safe
  ui->label_6->setText(tr("Band"));
  ui->label_7->setText(tr("Rx Frequency"));
  ui->TxMinuteButton->setEnabled(true);
  setMinButton();
  setClockStyle(true);
  progressBar->setMaximum(int(m_TRperiod));
  progressBar->setFormat("%v/"+QString::number(m_TRperiod));
  statusChanged();
  on_spotLineEdit_textChanged(ui->spotLineEdit->text());
  if(m_mode=="FT4" || m_mode=="FT2") {   // CE3TSK: FT2 has FT4's message set and hint memory
    if(m_rrr) { m_savedRRR=m_rrr; ui->rrrCheckBox->click(); }
    ui->rrrCheckBox->setEnabled(false); ui->rrr1CheckBox->setEnabled(false);
    if(!m_hint) ui->hintButton->click();
    ui->hintButton->setEnabled(false);
  }
  else {
    /* CE3TSK: !m_wwDigi - this runs on every mode change, so switching FT4 to FT8 during
       a contest would otherwise hand back a control the contest locked. */
    /* CE3TSK: m_savedRRR is cleared as it is consumed. JTDX only ever set it true - on
       entering FT4 with RRR on - and never reset it, so after one FT8/FT4/FT8 round trip
       it stayed true: unticking RRR and repeating the trip silently re-ticked it. Making
       it a one shot "owed a restore" flag is also what the contest restore assumes. */
    if(!m_wwDigi) { ui->rrrCheckBox->setEnabled(true); ui->rrr1CheckBox->setEnabled(true);
                    if(m_savedRRR) { m_savedRRR=false; ui->rrrCheckBox->click(); } }
    if(m_mode!="FT8") ui->hintButton->setEnabled(true);
  }
  if(m_mode!="FT8") {
    ui->candListSpinBox->setEnabled(false); ui->DTCenterSpinBox->setEnabled(false);
    ui->DTCenterSpinBox->setVisible(false); ui->pbTxMode->setVisible(true);
    ui->hintButton->setVisible(true); ui->syncButton->setVisible(false);
  } else {
    ui->syncButton->setVisible(true);
  }
  abortTxBackground("mode change");   // CE3TSK
  m_modeChanged=true;
}

void MainWindow::WSPR_config(bool b)
{
  ui->decodedTextBrowser2->setVisible(!b);
  ui->decodedTextLabel2->setVisible(!b);
  ui->controls_stack_widget->setCurrentIndex (b ? 1 : 0);
  ui->QSO_controls_widget->setVisible (!b);
  ui->DX_controls_widget->setEnabled (!b);
  ui->WSPR_controls_widget->setVisible (b);
  /* CE3TSK: HoundButton is owned by enableHoundAccess(), which every mode slot calls after
     this and which folds in the contest rule - but this is the one site that fights it, so
     the condition is repeated here rather than left to depend on call order. */
  ui->label_6->setVisible(!b and ui->cbMenus->isChecked()); ui->label_7->setVisible(!b); ui->HoundButton->setEnabled(!b && !m_wwDigi);
  ui->logQSOButton->setEnabled(!b); ui->DecodeButton->setEnabled(!b); ui->filterButton->setEnabled(!b);
  ui->AGCcButton->setEnabled(!b); ui->swlButton->setEnabled(!b); ui->ClearDxButton->setEnabled(!b);
  /* CE3TSK: AutoTx carries !m_wwDigi for the same reason as the single shot button above.
     AutoSeq and hint are not part of the contest set. */
  ui->hintButton->setEnabled(!b); ui->AutoTxButton->setEnabled(!b && !m_wwDigi); ui->AutoSeqButton->setEnabled(!b);
  /* CE3TSK: !m_wwDigi on the single shot button only - every mode slot calls this, so
     switching FT8 to FT4 during a contest would otherwise hand back a control the contest
     locked. bypass and AnsB4 are not part of the contest set and keep the stock behaviour. */
  ui->bypassButton->setEnabled(!b); ui->singleQSOButton->setEnabled(!b && !m_wwDigi); ui->AnsB4Button->setEnabled(!b);
  ui->syncButton->setEnabled(!b); ui->syncButton->setVisible(!b);
  if(b) {
    ui->decodedTextLabel->setTextFormat(Qt::PlainText); ui->decodedTextLabel->setText("UTC    dB   DT "+tr("    Freq     Drift  Call          Grid    dBm   Dist"));
    setBandLabelColour ("#fdedc5");
    ui->label_6->setText(tr("Band"));
    if (m_config.is_transceiver_online ()) {
      Q_EMIT m_config.transceiver_tx_frequency (0); // turn off split
    }
    m_bSimplex = true;
  } else {
    QString t;
    if (m_mode.startsWith("FT")) t = "UTC     dB   DT "+tr("Freq   Message");
    else t = "UTC   dB   DT "+tr("Freq   Message");
    ui->decodedTextLabel->setTextFormat(Qt::PlainText); ui->decodedTextLabel->setText(t);
    setBandLabelColour ("#fdedc5");
    ui->label_6->setText(tr("Band"));
    m_bSimplex = false;
  }
  enable_DXCC_entity ();  // sets text window proportions and (re)inits the logbook
}

void MainWindow::on_TxFreqSpinBox_valueChanged(int n)
{
//  if(n<200 && (!m_rigOk || ui->readFreq->text()!="S")) n=200;
  m_wideGraph->setTxFreq(n);
  if(m_lockTxFreq) ui->RxFreqSpinBox->setValue(n);
  if (m_tci) Q_EMIT m_config.transceiver_trfrequency(n - m_XIT);
  else Q_EMIT transmitFrequency (n - m_XIT);
  statusUpdate ();
}

void MainWindow::on_RxFreqSpinBox_valueChanged(int n)
{
  m_wideGraph->setRxFreq(n);
  if (m_lockTxFreq && ui->TxFreqSpinBox->isEnabled ()) ui->TxFreqSpinBox->setValue (n);
  statusUpdate ();
}

void MainWindow::on_candListSpinBox_valueChanged(int n) { m_ncandthin=n; }

void MainWindow::on_actionQuickDecode_triggered() { m_ndepth=1; ui->actionQuickDecode->setChecked(true); }
void MainWindow::on_actionMediumDecode_triggered() { m_ndepth=2; ui->actionMediumDecode->setChecked(true); }
void MainWindow::on_actionDeepestDecode_triggered() { m_ndepth=3; ui->actionDeepestDecode->setChecked(true); }

void MainWindow::on_actionDecFT8cycles3_triggered() { m_nFT8Cycles=3; ui->actionDecFT8cycles3->setChecked(true); }
void MainWindow::on_actionDecFT8cycles4_triggered() { m_nFT8Cycles=4; ui->actionDecFT8cycles4->setChecked(true); }
void MainWindow::on_actionDecFT8cycles5_triggered() { m_nFT8Cycles=5; ui->actionDecFT8cycles5->setChecked(true); }
void MainWindow::on_actionDecFT8cycles6_triggered() { m_nFT8Cycles=6; ui->actionDecFT8cycles6->setChecked(true); }
void MainWindow::on_actionDecFT8cycles7_triggered() { m_nFT8Cycles=7; ui->actionDecFT8cycles7->setChecked(true); }
void MainWindow::on_actionDecFT8cycles8_triggered() { m_nFT8Cycles=8; ui->actionDecFT8cycles8->setChecked(true); }
void MainWindow::on_actionDecFT8cycles9_triggered() { m_nFT8Cycles=9; ui->actionDecFT8cycles9->setChecked(true); }

void MainWindow::on_actionDecFT8SWLcycles3_triggered() { m_nFT8SWLCycles=3; ui->actionDecFT8SWLcycles3->setChecked(true); }
void MainWindow::on_actionDecFT8SWLcycles4_triggered() { m_nFT8SWLCycles=4; ui->actionDecFT8SWLcycles4->setChecked(true); }
void MainWindow::on_actionDecFT8SWLcycles5_triggered() { m_nFT8SWLCycles=5; ui->actionDecFT8SWLcycles5->setChecked(true); }
void MainWindow::on_actionDecFT8SWLcycles6_triggered() { m_nFT8SWLCycles=6; ui->actionDecFT8SWLcycles6->setChecked(true); }
void MainWindow::on_actionDecFT8SWLcycles7_triggered() { m_nFT8SWLCycles=7; ui->actionDecFT8SWLcycles7->setChecked(true); }
void MainWindow::on_actionDecFT8SWLcycles8_triggered() { m_nFT8SWLCycles=8; ui->actionDecFT8SWLcycles8->setChecked(true); }
void MainWindow::on_actionDecFT8SWLcycles9_triggered() { m_nFT8SWLCycles=9; ui->actionDecFT8SWLcycles9->setChecked(true); }

void MainWindow::on_actionRXfLow_triggered() { m_nFT8RXfSens=1; ui->actionRXfLow->setChecked(true); }
void MainWindow::on_actionRXfMedium_triggered() { m_nFT8RXfSens=2; ui->actionRXfMedium->setChecked(true); }
void MainWindow::on_actionRXfHigh_triggered() { m_nFT8RXfSens=3; ui->actionRXfHigh->setChecked(true); }

void MainWindow::on_actionFT4fast_triggered() { m_nFT4depth=1; ui->actionFT4fast->setChecked(true); }
void MainWindow::on_actionFT4medium_triggered() { m_nFT4depth=2; ui->actionFT4medium->setChecked(true); }
void MainWindow::on_actionFT4deep_triggered() { m_nFT4depth=3; ui->actionFT4deep->setChecked(true); }

void MainWindow::on_actionSwitch_Filter_OFF_at_sending_73_triggered(bool checked)
{
  if(checked) {
    if(ui->actionSwitch_Filter_OFF_at_getting_73->isChecked()) ui->actionSwitch_Filter_OFF_at_getting_73->setChecked(false);
    m_FilterState=1;
    ui->actionSwitch_Filter_OFF_at_sending_73->setChecked(true);
  } else {
    ui->actionSwitch_Filter_OFF_at_sending_73->setChecked(false);
    if(!ui->actionSwitch_Filter_OFF_at_getting_73->isChecked()) {
      m_FilterState=0;
      on_actionAutoFilter_toggled(false); ui->actionAutoFilter->setChecked(false);
    } else {
      m_FilterState=2;
    }
  }
}

void MainWindow::on_actionSwitch_Filter_OFF_at_getting_73_triggered(bool checked)
{
  if(checked) {
    if(ui->actionSwitch_Filter_OFF_at_sending_73->isChecked()) ui->actionSwitch_Filter_OFF_at_sending_73->setChecked(false);
    m_FilterState=2;
    ui->actionSwitch_Filter_OFF_at_getting_73->setChecked(true);
  } else {
    ui->actionSwitch_Filter_OFF_at_getting_73->setChecked(false);
    if(!ui->actionSwitch_Filter_OFF_at_sending_73->isChecked()) {
      m_FilterState=0;
      on_actionAutoFilter_toggled(false); ui->actionAutoFilter->setChecked(false);
    } else {
      m_FilterState=1;
    }
  }
}

void MainWindow::on_actionErase_ALL_TXT_triggered()          //Erase ALL.TXT
{
  int ret = JTDXMessageBox::warning_message(this, "", tr("Confirm Erase"),
                                 tr("Are you sure you want to erase file ALL.TXT ?"),
                                 "", JTDXMessageBox::Yes | JTDXMessageBox::No, JTDXMessageBox::Yes);
  if(ret==JTDXMessageBox::Yes) {
    QFile f {m_dataDir.absoluteFilePath (m_jtdxtime->currentDateTimeUtc2().toString("yyyyMM_")+"ALL.TXT")};
    f.remove();
    m_RxLog=1;
  }
}

void MainWindow::on_actionErase_wsjtx_log_adi_triggered()
{
  int ret = JTDXMessageBox::warning_message(this, "", tr("Confirm Erase"),
                                 tr("Are you sure you want to erase your QSO LOG?"),
                                 "", JTDXMessageBox::Yes | JTDXMessageBox::No, JTDXMessageBox::Yes);
  if(ret==JTDXMessageBox::Yes) {
    QFile f {m_dataDir.absoluteFilePath ("wsjtx_log.adi")};
    f.remove();
  }
}

void MainWindow::on_actionOpen_wsjtx_log_adi_triggered()
{
  QDesktopServices::openUrl (QUrl::fromLocalFile (m_dataDir.absoluteFilePath ("wsjtx_log.adi")));
}

void MainWindow::on_actionOpen_log_directory_triggered ()
{
  QDesktopServices::openUrl (QUrl::fromLocalFile (m_dataDir.absolutePath ()));
}

bool MainWindow::gridOK(QString g)
{
  bool b=false;
  if(g.length()>=4) {
    b=g.left(1).compare("A")>=0 and
        g.left(1).compare("R")<=0 and
        g.mid(1,1).compare("A")>=0 and
        g.mid(1,1).compare("R")<=0 and
        g.mid(2,1).compare("0")>=0 and
        g.mid(2,1).compare("9")<=0 and
        g.mid(3,1).compare("0")>=0 and
        g.mid(3,1).compare("9")<=0;
		
    if(g.length()==4 and g=="RR73") b=false;
  }
  return b;
}

bool MainWindow::gridRR73(QString g)
{
  bool RR73 = false;
  if (g.length()==4 and g=="RR73") RR73 = true;
  return RR73;
}

bool MainWindow::reportRCVD(QStringList msg)
{
  bool rprt_rcvd = false;
  if (msg.length() == 4) { // three words(max index 3) in the message where "CQ DX" is counted as single word
	if (msg.at (1).contains (m_baseCall) && msg.at (2).contains (Radio::base_callsign (m_hisCall))
		&& (msg.at (3).contains ("-") || msg.at (3).contains ("+"))) rprt_rcvd = true;
  }
  return rprt_rcvd;
}

bool MainWindow::rReportRCVD(QStringList msg)
{
  bool rogerrprt_rcvd = false;
  if (msg.length() == 4) { // three words(max index 3) in the message where "CQ DX" is counted as single word
	if (msg.at (1).contains (m_baseCall) && msg.at (2).contains (Radio::base_callsign (m_hisCall))
		&& (msg.at (3).contains ("R-") || msg.at (3).contains ("R+"))) rogerrprt_rcvd = true;
	}
  return rogerrprt_rcvd;
}

void MainWindow::on_bandComboBox_currentIndexChanged (int index)
{
  auto const& frequencies = m_config.frequencies ();
  auto const& source_index = frequencies->mapToSource (frequencies->index (index, FrequencyList_v2::frequency_column));
  Frequency frequency {m_freqNominal};
  if (source_index.isValid ())
    {
      frequency = frequencies->frequency_list ()[source_index.row ()].frequency_;
      m_callingFrequency = frequency;
    }

  // Lookup band
  auto const& band  = m_config.bands ()->find (frequency);
  if (!band.isEmpty ())
    {
      ui->bandComboBox->lineEdit ()->setStyleSheet ({});
      ui->bandComboBox->setCurrentText (band);
    }
  else
    {
      ui->bandComboBox->lineEdit ()->setStyleSheet (QString("QLineEdit {color: %1; background-color : %2}").arg(Radio::convert_dark("#ffff00",m_useDarkStyle),Radio::convert_dark("#ff0000",m_useDarkStyle)));
      ui->bandComboBox->setCurrentText (m_config.bands ()->oob ());
    }
  displayDialFrequency ();
}

void MainWindow::on_bandComboBox_activated (int index)
{
  auto const& frequencies = m_config.frequencies ();
  auto const& source_index = frequencies->mapToSource (frequencies->index (index, FrequencyList_v2::frequency_column));
  Frequency frequency {m_freqNominal};
  if (source_index.isValid ()) frequency = frequencies->frequency_list ()[source_index.row ()].frequency_;
  m_bandEdited = true;
  band_changed (frequency); if(m_config.write_decoded_debug()) writeToALLTXT("Band changed from bandComboBox, frequency: " + QString::number(frequency));
  m_wideGraph->setRxBand (m_config.bands ()->find (frequency));
}

void MainWindow::band_changed (Frequency f)
{
  m_dialWheelTimer.stop (); m_dialWheelClock.invalidate ();   // CE3TSK: any QSY ends a wheel burst, applyDialWheel () restarts the hold
  abortTxBackground("band change");   // CE3TSK
  if (m_bandEdited) {
    if (!m_mode.startsWith ("WSPR")) { // band hopping preserves auto Tx
      if (f + m_wideGraph->nStartFreq () > m_freqNominal + ui->TxFreqSpinBox->value ()
          || f + m_wideGraph->nStartFreq () + m_wideGraph->fSpan () <=
          m_freqNominal + ui->TxFreqSpinBox->value ()) {
        qDebug () << "start f:" << m_wideGraph->nStartFreq () << "span:" << m_wideGraph->fSpan () << "DF:" << ui->TxFreqSpinBox->value ();
        // disable auto Tx if "blind" QSY outside of waterfall
//        ui->stopTxButton->click (); // halt any transmission
        if(m_transmitting || g_iptt==1) haltTx("band changed ");
        enableTx_mode (false);       // switch off Enable Tx button
      }
    }
    auto const& newband = m_config.bands ()->find (f);
    auto const& oldband = m_config.bands ()->find (m_lastMonitoredFrequency);
    bool cleared=false;
    if (oldband != newband || m_oldmode != m_mode) {
      clearDX (" cleared, triggered by erase both windows option upon band change, new band/mode"); // Request from Boris UX8IW
      if (m_autoEraseBC) { // option: erase both windows if band is changed
          ui->decodedTextBrowser->clear();
          ui->decodedTextBrowser2->clear();
          cleared=true;
      }
    }
//    m_lastBand.clear ();
    m_bandEdited = false;
    psk_Reporter->sendReport();      // Upload any queued spots before changing band
    m_okToPost = false;
    if(!m_transmitting && !m_start2 && m_monitoroff) { monitor (true); m_monitoroff=false; }

    m_nsecBandChanged=0;
    if(!m_transmitting && (oldband != newband || m_oldmode != m_mode) && m_rigOk && !m_config.rig_name().startsWith("None")) {
      m_bandChanged=true;
      qint64 ms = m_jtdxtime->currentMSecsSinceEpoch2() % 86400000; int nsec=ms/1000;
      // TR period is the only reliable way in this point of code at the mode change
      double TRperiod=tr_period_of(m_mode);   // CE3TSK: modetiming.h - this chain used to forget FT2
      int nseqmod = fmod(double(nsec),TRperiod);
      m_nsecBandChanged=nseqmod;
    }

    m_freqNominal = f;
    m_freqTxNominal = m_freqNominal;
    highlightBandButton ();   // CE3TSK
    setRig ();
    setXIT (ui->TxFreqSpinBox->value ());
    qint64 fDelta = m_lastDisplayFreq - m_freqNominal;
    /* CE3TSK: was `... && m_outAttenuation != 1) {setValue (m_outAttenuation); m_outAttenuation = 1;}`,
       which used m_outAttenuation itself as the "already done" sentinel and so could not tell
       "not restored yet" from "restored, and the value is 1".  writeSettings needs that
       distinction to avoid saving the slider's .ui default over a good stored value. */
    if (!m_outAttenuationRestored)
      {
        if (ui->outAttenuation->value () == 1) ui->outAttenuation->setValue (m_outAttenuation);
        m_outAttenuationRestored = true;
      }
    if (qAbs(fDelta)>1000000) {
        m_qsoHistory.init(); if(m_config.write_decoded_debug()) writeToALLTXT("QSO history initialized by band_changed");
        clearDX (" cleared, triggered by erase both windows option upon band change, delta frequency"); // Request from Boris UX8IW
        if (m_autoEraseBC && !cleared) { // option: erase both windows if band is changed
            ui->decodedTextBrowser->clear();
            ui->decodedTextBrowser2->clear();
        }
    // Set the attenuation value if options are checked
        QString curBand;
        if (m_config.pwrBandTxMemory() && !m_tune) {
            if (m_mode == "JT9+JT65" && m_modeTx == "JT65") { curBand = m_config.bands ()->find (m_freqNominal)+m_modeTx; }
            else { curBand = m_config.bands ()->find (m_freqNominal)+m_mode; }
            if (m_pwrBandTxMemory.contains(curBand)) { m_PwrBandSetOK = false; ui->outAttenuation->setValue(m_pwrBandTxMemory[curBand].toInt()); m_PwrBandSetOK = true;/* printf("set power from bandchange % lld %lld %s %d\n",m_lastDisplayFreq,m_freqNominal,curBand.toStdString().c_str(),m_pwrBandTxMemory[curBand].toInt());*/}
            else if (m_outAttenuationRestored) { m_pwrBandTxMemory[curBand] = ui->outAttenuation->value(); }   /* CE3TSK: only record a band once the slider holds a real value, never the .ui default */
        }
        ui->bandComboBox->setCurrentText (m_config.bands ()->find (m_freqNominal));
        m_wideGraph->setRxBand (m_config.bands ()->find (m_freqNominal));
    }
    m_lastDisplayFreq=m_freqNominal;
    m_oldmode=m_mode;
    bool commonFT8b=false;
//    Switch off Hound mode if coming to the regular FT8 band
    if(m_mode!="FT8") {
      if(m_houndMode) ui->actionEnable_hound_mode->setChecked(false);
    } else {
      for(long unsigned int i=0; i < sizeof (m_ft8Freq) / sizeof (m_ft8Freq[0]); i++) {
        int kHzdiff=m_freqNominal/1000 - m_ft8Freq[i];
        if(kHzdiff > -2 && kHzdiff < 3) { if(m_houndMode) ui->actionEnable_hound_mode->setChecked(false); commonFT8b=true; break; }
      }
      m_commonFT8b=commonFT8b;
      if (m_houndMode) {
      // Don't allow Hound frequency control in common FT8 bands if VFO Split mode is switched off
        QString message = "";
        if(!m_config.split_mode() && !m_commonFT8b && m_config.rig_name() != "None") {
          message =  tr ("Hound mode TX frequency control requires"
                                                              " *Split* rig control (either *Rig* or *Fake It* set"
                                                              " in the *Settings | Radio* tab.)");
          JTDXMessageBox::warning_message (this, "", tr ("Hound TX frequency control warning"), message);
          ui->actionEnable_hound_mode->setChecked(false);
        } else {
          m_houndTXfreqJumps=!m_commonFT8b && m_config.split_mode() && m_config.rig_name() != "None";
          ui->actionUse_TX_frequency_jumps->setChecked(m_houndTXfreqJumps);
          if(m_commonFT8b || m_config.rig_name() == "None") ui->actionUse_TX_frequency_jumps->setEnabled(false);
          else ui->actionUse_TX_frequency_jumps->setEnabled(true);
        }
      }
    }
    m_lastloggedtime=m_lastloggedtime.addSecs(-7*int(m_TRperiod));
    m_lastloggedcall.clear(); setLastLogdLabel();
  }
}

void MainWindow::enable_DXCC_entity ()
{
  if (m_mode.left(4)!="WSPR" && (m_callNotif != m_config.callNotif() || m_callsign != m_config.my_callsign() || m_gridNotif != m_config.gridNotif() || m_grid != m_config.my_grid() || m_timeFrom != m_config.timeFrom() || m_countryNameTranslated != m_config.countryNameTranslated() || m_strictdirCQ != m_config.strictdirCQ())) {
    if (m_callNotif != m_config.callNotif() || m_callsign != m_config.my_callsign() || m_gridNotif != m_config.gridNotif() || m_grid != m_config.my_grid() || m_timeFrom != m_config.timeFrom() || m_countryNameTranslated != m_config.countryNameTranslated()) {
      m_qsoHistory.init(); if(m_config.write_decoded_debug()) writeToALLTXT("QSO history initialized by enable_DXCC_entity");
      m_logBook.init(m_config.callNotif() ? m_config.my_callsign() : "",m_config.gridNotif() ? m_config.my_grid() : "",m_config.timeFrom(),"wsjtx_log.adi",nullptr,m_config.countryNameTranslated());
      refreshContestLog(true);   /* CE3TSK: country data has just been read */
      m_callsign = m_config.my_callsign();
      m_grid = m_config.my_grid();
      m_callNotif = m_config.callNotif();
      m_gridNotif = m_config.gridNotif();
      m_timeFrom = m_config.timeFrom();
      m_countryNameTranslated = m_config.countryNameTranslated();
    }
    QString countryName;
    m_logBook.getDXCC(m_config.my_callsign(),countryName);
    QStringList items=countryName.split(",");
    m_m_continent = items[0];
    ui->decodedTextBrowser->setMyContinent (m_m_continent);
    ui->decodedTextBrowser2->setMyContinent (m_m_continent);
    m_m_prefix = items[1];
    m_qsoHistory.owndata(items[0],items[1],m_config.my_grid(),m_config.strictdirCQ ());
    m_strictdirCQ = m_config.strictdirCQ();
  }
  updateGeometry ();
 }

void MainWindow::on_pbCallCQ_clicked()
{
//  clearDXfields(" field cleared, SLOT on_pbCallCQ_clicked()"); // this line is duplicated in SLOT on_txb6_clicked()
//need to sync CQ direction, instead of  ui->txrb6->setChecked(true); :
  ui->txb6->click (); // check if there is any dependency
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx6->text());
  m_ntx=7;
  m_QSOProgress = CALLING;
  m_nlasttx=6;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::on_pbAnswerCaller_clicked()
{
  genStdMsgs(m_rpt);
  QString t=ui->tx3->text();
  int i0=t.indexOf(" R-");
  if(i0<0) i0=t.indexOf(" R+");
  t=t.left(i0+1)+t.mid(i0+2,3);
  ui->genMsg->setText(t);
  m_ntx=7;
  m_QSOProgress = REPORT;
  m_nlasttx=2;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::on_pbSendRRR_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx4->text());
  m_ntx=7;
  m_QSOProgress = ROGERS;
  m_nlasttx=4;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::resizeEvent(QResizeEvent *event) { 
  if(event->size().height() != event->oldSize().height()) dynamicButtonsInit(); 
  // CE3TSK: the share is put back when the splitter itself is resized - see eventFilter
}

/* CE3TSK: the share is read once the window is up and laid out, and after that only the operator's
   own drag changes it. It must NOT be read during a resize: by then Qt has already redistributed the
   new width, so reading it there captures the drift and preserves THAT - measured 41.3 % becoming
   48.5 % on the first widening and sticking. */
void MainWindow::showEvent (QShowEvent *event)
{
  QMainWindow::showEvent (event);
  if (!m_splitLearned) {
    m_splitLearned = true;
    QTimer::singleShot (0, this, [this] { rememberSplitRatio (); });
  }
}

/* CE3TSK: the splitter keeps the share the operator gave it.
 *
 * QSplitter has no stretch factors here, so Qt hands new width to the panes by its own rule and the
 * bar slides: measured, the left pane held 41.3 % of a 1000 px window and 47.8 % of a 1600 px one,
 * the same either way whether the edge was dragged or the window resized in one step. Widening the
 * window therefore meant dragging the bar back every time.
 *
 * So the ratio is remembered whenever the operator moves the handle - that is the only thing that
 * should change it - and re-applied whenever the window's WIDTH changes. setSizes respects each
 * pane's minimum, so a window too narrow for the right-hand controls still gives them their hint
 * rather than the ratio; the ratio is restored as soon as there is room again. */
void MainWindow::rememberSplitRatio ()
{
  auto const s = ui->splitter->sizes ();
  if (s.size () != 2 || s[0] <= 0 || s[1] <= 0) return;
  m_splitRatio = double (s[0]) / (s[0] + s[1]);
  keepSplitRatio ();
}

/* The share is expressed to Qt as STRETCH FACTORS rather than corrected afterwards. Qt divides new
   width between the panes in proportion to these, so the panes grow and shrink together and no
   correction has to race the layout - which is what three earlier attempts did, each one measurably
   losing to it. Widths beyond a pane's own minimum or maximum are still Qt's to clamp: the
   right-hand controls keep their minimum on a narrow window, and the ratio returns as soon as there
   is room for it. */
void MainWindow::keepSplitRatio ()
{
  if (m_splitRatio <= 0.0 || m_splitRatio >= 1.0) return;
  m_splitApplying = true;
  ui->splitter->setStretchFactor (0, qRound (1000.0 * m_splitRatio));
  ui->splitter->setStretchFactor (1, qRound (1000.0 * (1.0 - m_splitRatio)));
  m_splitApplying = false;
}

void MainWindow::mousePressEvent(QMouseEvent *event)             //mousePressEvent
{
  if(ui->ClearDxButton->hasFocus() && (event->button() & Qt::RightButton)) {
    if (!m_hisCall.isEmpty()) {
      if (event->modifiers() & Qt::ControlModifier)
          m_qsoHistory.blacklist(m_hisCall);    
      else
          m_qsoHistory.remove(m_hisCall);    
      clearDX (" cleared and erased from QSO history by right mouse button click, user action");
    }   
  }
  
  if(ui->syncButton->hasFocus() && (event->button() & Qt::RightButton) && m_jtdxtime->GetOffset() != 0.0) {
    m_jtdxtime->SetOffset(0);
    ui->syncButton->setEnabled(false);
  }

  if(ui->EraseButton->hasFocus() && (event->button() & Qt::RightButton)) {
    qint64 ms=m_jtdxtime->currentMSecsSinceEpoch2();
    ui->decodedTextBrowser2->clear();
    if(m_mode.left(4)=="WSPR") { ui->decodedTextBrowser->clear(); }
    else {
      m_QSOText.clear();
      if((ms-m_msErase)<500) {
      ui->decodedTextBrowser->clear();
      m_messageClient->clear_decodes ();
//     QFile f(m_config.temp_dir ().absoluteFilePath ("decoded.txt"));
//     if(f.exists()) f.remove();
      }
    }
    m_msErase=ms;
  }

  if(ui->pbSpotDXCall->hasFocus() && (event->button() & Qt::RightButton)) {
    QString basecall = Radio::base_callsign (m_hisCall);
    if(basecall.length () > 2) {
        m_config.add_callsign_hideFilter (basecall);
        m_dxCallHidden=true; setSpotButtonStyle ();   // CE3TSK: remembered so a style switch keeps it
    }
  }
}

void MainWindow::on_pbAnswerCQ_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx1->text());
  if(!m_mode.startsWith("FT")) { QString t=ui->tx2->text(); int i0=t.indexOf("/"); int i1=t.indexOf(" "); if(i0>0 and i0<i1) ui->genMsg->setText(t); }
  m_ntx=7;
  m_QSOProgress = REPLYING;
  m_nlasttx=1;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::on_pbSendReport_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx3->text());
  m_ntx=7;
  m_QSOProgress = ROGER_REPORT;
  m_nlasttx=3;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::on_pbSend73_clicked()
{
  genStdMsgs(m_rpt);
  ui->genMsg->setText(ui->tx5->currentText());
  m_ntx=7;
  m_QSOProgress = SIGNOFF;
  m_nlasttx=5;
  ui->rbGenMsg->setChecked(true);
  if(m_transmitting) m_restart=true;
}

void MainWindow::on_rbGenMsg_clicked(bool checked)
{
  m_freeText=!checked;
  if(!m_freeText) {
    if(m_ntx != 7 && m_transmitting) m_restart=true;
    m_ntx=7;
  }
}

void MainWindow::on_rbFreeText_clicked(bool checked)
{
  m_freeText=checked;
  if(m_freeText) {
    m_ntx=8;
    m_QSOProgress = SIGNOFF; // is one more value required for free text?
    m_nlasttx=8;
    if (m_transmitting) m_restart=true;
  }
}

void MainWindow::on_freeTextMsg_currentTextChanged (QString const& text)
{
  bool isAllowedAuto73=isAutoSeq73(text);
  if(!m_FTsetAutoSeqOff && !isAutoSeq73(text)) m_FTsetAutoSeqOff=true;
  if(isAllowedAuto73) m_FTsetAutoSeqOff=false;
  if(!text.contains(QRegularExpression {R"([@#&^])"}) && !text.isEmpty()) {
    QString t="161545  -4  0.1 1939 & " + text;
    DecodedText decodedtext {t,this};
//      DecodedText decodedtext {"161545  -4  0.1 1939 & CQ RT9K/4    ",this};
    bool stdfreemsg = decodedtext.isStandardMessage();
    if(stdfreemsg) {
      ui->freeTextMsg->setStyleSheet(QString("background: %1;color: %2").arg(Radio::convert_dark("#7bff7b",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));
    } else {
      if(text.length() < 14) { ui->freeTextMsg->setStyleSheet(QString("background: %1;color: %2").arg(Radio::convert_dark("#7bff7b",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle))); }
      else if(text.length() >= 14) { ui->freeTextMsg->setStyleSheet(QString("background: %1;color: %2").arg(Radio::convert_dark("#ffff00",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle))); }
      msgtype(text, ui->freeTextMsg->lineEdit ());
    }
  }
}

void MainWindow::on_freeTextMsg_currentIndexChanged(int index)
{
//this functionality can affect AutoSeq operation
//it is dedicated to change free message during Tx if new message is selected from the list
  if(m_transmitting && ui->rbFreeText->isChecked() && index != m_oldFreeMsgIndex) m_restart=true;	
  m_oldFreeMsgIndex=index;
}

void MainWindow::on_rptSpinBox_valueChanged(int n)
{
  m_rpt=QString::number(n);
  int ntx0=m_ntx;
  QString t=ui->tx5->currentText();
  genStdMsgs(m_rpt);
  ui->tx5->setCurrentText(t);
  m_ntx=ntx0;
  if(m_ntx==1) { ui->txrb1->setChecked(true); }
  else if(m_ntx==2) { ui->txrb2->setChecked(true); }
  else if(m_ntx==3) { ui->txrb3->setChecked(true); }
  else if(m_ntx==4) { ui->txrb4->setChecked(true); }
  else if(m_ntx==5) { ui->txrb5->setChecked(true); }
  else if(m_ntx==6) { ui->txrb6->setChecked(true); }
  statusChanged();
}

void MainWindow::on_tuneButton_clicked (bool checked)
{
  static bool lastChecked = false;
  if (lastChecked == checked) return;
  lastChecked = checked;
  if(!checked) m_addtx = -2;
  QString curBand;
  if (m_mode == "JT9+JT65" && m_modeTx == "JT65") { curBand = ui->bandComboBox->currentText()+m_modeTx; }
  else { curBand = ui->bandComboBox->currentText()+m_mode; }
  if (checked && m_tune==false) { // we're starting tuning so remember Tx and change pwr to Tune value
    /* CE3TSK: what the slider held before the tune, so the end of the tune can come back to it
       even on a band that has nothing remembered - otherwise the operator is left transmitting
       at tune drive. */
    m_outAttenuationPreTune = ui->outAttenuation->value();
    if (m_config.pwrBandTuneMemory ()) {
      /* CE3TSK: only record a band once the slider holds a real value, never the .ui default -
         the guard every power-memory write carries. */
      if (m_outAttenuationRestored) m_pwrBandTxMemory[curBand] = ui->outAttenuation->value(); // remember our Tx pwr
      if (m_pwrBandTuneMemory.contains(curBand)) {
        m_PwrBandSetOK = false;
        ui->outAttenuation->setValue(m_pwrBandTuneMemory[curBand].toInt()); // set to Tune pwr
        m_PwrBandSetOK = true;
      }
    }
  } else { // we're turning off so remember our Tune pwr setting and reset to Tx pwr
	if (m_config.pwrBandTuneMemory() || m_config.pwrBandTxMemory()) {
		/* CE3TSK: the same guard the other power-memory writes carry - a slider still at the .ui
		   default of 1 (the rig never came up, so band_changed () never restored it) must not be
		   memorised as this band's tune drive. */
		if (m_outAttenuationRestored) m_pwrBandTuneMemory[curBand] = ui->outAttenuation->value(); // remember our Tune pwr
		m_PwrBandSetOK = false;
		/* CE3TSK: come back to the remembered Tx drive, or failing that to whatever the slider
		   held before the tune. Reading an absent key would give 0, i.e. minimum drive, and
		   leaving the slider alone would transmit at tune drive. */
		if (m_pwrBandTxMemory.contains(curBand)) ui->outAttenuation->setValue(m_pwrBandTxMemory[curBand].toInt()); // set to Tx pwr
		else if (m_outAttenuationPreTune >= 0) ui->outAttenuation->setValue(m_outAttenuationPreTune);
		m_PwrBandSetOK = true;
    }
	m_outAttenuationPreTune = -1;
  }
  if (m_tune) {
	if (!tuneButtonTimer.isActive())
		tuneButtonTimer.start(250);
  } else {
    m_sentFirst73=false;
    itone[0]=0;
//    on_monitorButton_clicked (true);
    m_tune=true;
  }
  if (m_tci) Q_EMIT m_config.transceiver_tune(checked);
  else Q_EMIT tune (checked);
  if(checked && m_config.tunetimer() && !m_tuneup) StopTuneTimer.start(m_config.tunetimer()*1000); // shall not start at WSPR band hopping
}

void MainWindow::stop_tuning ()
{
  if(StopTuneTimer.isActive()) StopTuneTimer.stop();
  on_tuneButton_clicked(false);
  ui->tuneButton->setChecked (false);
  ui->tuneButton->setText(tr("&Tune"));
  m_bTxTime=false;
  m_tune=false;
}

void MainWindow::stopTuneATU() { on_tuneButton_clicked(false); m_bTxTime=false; }

void MainWindow::on_stopTxButton_clicked()                    //Stop Tx
{
  if (m_transmitting || m_tune) m_addtx = -1;
  if (m_tune) stop_tuning ();
  if (m_enableTx and !m_tuneup) enableTx_mode (false);
  m_btxok=false;
  m_nlasttx=0;
//  if(m_autofilter && m_autoseq && m_filter) autoFilter (false);
  if(m_filter && (m_FilterState==1 || m_FilterState==2)) autoFilter (false);
  if(!m_transmitting && g_iptt==1) m_haltTrans=true;
  if (m_haltTxWritten) m_haltTxWritten=false;
  else writeHaltTxEvent("Halt Tx button clicked ");
  if(m_transmitting && m_skipTx1 && !m_hisCall.isEmpty() && (m_ntx==2 || m_QSOProgress==REPORT)) m_qsoHistory.remove(m_hisCall);
// sync TX variables, RX scenario is covered in on_enableTxButton_clicked ()
  m_bTxTime=false; m_tx_when_ready=false; m_transmitting=false; m_restart=false; m_txNext=false;
}

void MainWindow::rigOpen ()
{
  setRigLamp ("#ffa500");
  m_rigOk=false;
  ui->readFreq->setText ("");
  ui->readFreq->setEnabled (true);
  m_config.transceiver_online ();
  Q_EMIT m_config.sync_transceiver (true, true);
}

void MainWindow::on_pbR2T_clicked()
{
  if (ui->TxFreqSpinBox->isEnabled ()) ui->TxFreqSpinBox->setValue(ui->RxFreqSpinBox->value ());
}

void MainWindow::on_pbT2R_clicked()
{
  if (ui->RxFreqSpinBox->isEnabled ()) ui->RxFreqSpinBox->setValue (ui->TxFreqSpinBox->value ());
}

void MainWindow::on_readFreq_clicked()
{
  if (m_transmitting) return;
  if (m_config.transceiver_online ()) Q_EMIT m_config.sync_transceiver (true, true);
}

void MainWindow::on_pbTxMode_clicked()
{
  QString curBand;
  if(m_modeTx=="JT9") {
    m_modeTx="JT65";
    ui->pbTxMode->setText("Tx JT65  #");
  } else {
    m_modeTx="JT9";
    ui->pbTxMode->setText("Tx JT9  @");
  }
  if (m_config.pwrBandTxMemory() && !m_tune) {
      if (m_mode == "JT9+JT65" && m_modeTx == "JT65") { curBand = ui->bandComboBox->currentText()+m_modeTx; }
      else { curBand = ui->bandComboBox->currentText()+m_mode; }
      if (m_pwrBandTxMemory.contains(curBand)) { ui->outAttenuation->setValue(m_pwrBandTxMemory[curBand].toInt()); }
      else if (m_outAttenuationRestored) { m_pwrBandTxMemory[curBand] = ui->outAttenuation->value(); }   /* CE3TSK: only record a band once the slider holds a real value, never the .ui default */
  }
  m_wideGraph->setModeTx(m_modeTx);
  statusChanged();
  on_spotLineEdit_textChanged(ui->spotLineEdit->text());
}

void MainWindow::setXIT(int n, Frequency base)
{
  
  if(m_mode=="JT9+JT65") {
	if(m_wideGraph->Fmin() <= m_config.ntopfreq65()) {
		if(m_modeTx == "JT65" && n >= m_config.ntopfreq65()) {
			on_pbTxMode_clicked();
			on_pbT2R_clicked();
		} else {
			if(m_modeTx == "JT9" && n <= m_wideGraph->Fmin()) {
				on_pbTxMode_clicked();
				on_pbT2R_clicked();
			}
		}
	} else {
		if(m_modeTx == "JT65" && n >= m_wideGraph->Fmin()) {
			on_pbTxMode_clicked();
			on_pbT2R_clicked();
		} else {
			if(m_modeTx == "JT9" && n <= m_config.ntopfreq65()) {
				on_pbTxMode_clicked();
				on_pbT2R_clicked();
			}
		}
	}
  }
  
  if (!base) base = m_freqNominal;
  m_XIT = 0;
  if (!m_bSimplex && base) {
    // m_bSimplex is false, so we can use split mode if requested
    if (m_config.split_mode ()) {if (m_tci) m_XIT = n - 1500; else m_XIT = (n/500)*500 - 1500;}
    if ((m_monitoring || m_transmitting) && m_config.is_transceiver_online () && m_config.split_mode ()) {
        // All conditions are met, reset the transceiver Tx dial
        // frequency
        m_freqTxNominal = base + m_XIT;
        if (base == 1908000) {
          if (m_crossbandOptionEnabled && m_m_prefix != "JA") m_freqTxNominal -= 68000;
        } else if (base == 1810000) {
          if (m_crossbandHLOptionEnabled && m_m_prefix != "HL") m_freqTxNominal += 30000;
        } else if (base == 1840000) {
          if (m_crossbandOptionEnabled && m_m_prefix == "JA") m_freqTxNominal += 68000;
          if (m_crossbandHLOptionEnabled && m_m_prefix == "HL") m_freqTxNominal -= 30000;
        }
        Q_EMIT m_config.transceiver_tx_frequency (m_freqTxNominal);
	}
  }
  //Now set the audio Tx freq
  if (m_tci) Q_EMIT m_config.transceiver_trfrequency(ui->TxFreqSpinBox->value () - m_XIT);
  else Q_EMIT transmitFrequency (ui->TxFreqSpinBox->value () - m_XIT);
  if(m_transmitting) m_restart=true;
}

void MainWindow::setRxFreq4(int rxFreq)
{
    txwatchdog (false);
	if (m_mode=="JT9+JT65" && !m_transmitting && !m_lockTxFreq) {
		if(m_wideGraph->Fmin() <= m_config.ntopfreq65()) {
			if(m_modeTx == "JT65" && rxFreq >= m_config.ntopfreq65()) {
				on_pbTxMode_clicked();
				on_pbR2T_clicked();
			} else {
				if(m_modeTx == "JT9" && rxFreq <= m_wideGraph->Fmin()) {
					on_pbTxMode_clicked();
					on_pbR2T_clicked();
				}
			}
		} else {
			if(m_modeTx == "JT65" && rxFreq >= m_wideGraph->Fmin()) {
				on_pbTxMode_clicked();
				on_pbR2T_clicked();
			}
			if(m_modeTx == "JT65" && rxFreq > m_config.ntopfreq65() && rxFreq < m_wideGraph->Fmin()) {
				ui->RxFreqSpinBox->setValue (m_config.ntopfreq65());
			}
			if(m_modeTx == "JT9" && rxFreq <= m_config.ntopfreq65()) {
				on_pbTxMode_clicked();
				on_pbR2T_clicked();
			}
			if(m_modeTx == "JT9" && rxFreq < m_wideGraph->Fmin() && rxFreq > m_config.ntopfreq65()) {
				ui->RxFreqSpinBox->setValue (m_wideGraph->Fmin());
			}
		}
	}
}

void MainWindow::setFreq4(int rxFreq, int txFreq)
{
  txwatchdog (false);
  if (ui->RxFreqSpinBox->isEnabled ()) ui->RxFreqSpinBox->setValue(rxFreq);
  if(m_mode.left(4)=="WSPR") {
    ui->WSPRfreqSpinBox->setValue(txFreq);
  } else {
    if (ui->TxFreqSpinBox->isEnabled ()) {
      ui->TxFreqSpinBox->setValue(txFreq);
    }
  }
}

void MainWindow::on_pbTxLock_clicked(bool checked)
{
  m_lockTxFreq=checked;
  if(checked) {
     ui->pbTxLock->setText(tr("Lockd Tx=Rx"));
	 ui->pbTxLock->setToolTip(tr("<html><head/><body><p>Push button to allow Tx/Rx AF frequencies split operation.</p></body></html>"));
     ui->pbTxLock->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-radius: 5px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#ffff88",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle)));
  } else {
  	 ui->pbTxLock->setText(tr("Tx/Rx Split"));
     ui->pbTxLock->setToolTip(tr("<html><head/><body><p>Push button to lock Tx frequency to the Rx AF frequency.</p></body></html>"));
     ui->pbTxLock->setStyleSheet(QString("QPushButton {color: %1;background: %2;border-style: solid;border-width: 1px;border-color: %3;min-width: 5em;padding: 3px}").arg(Radio::convert_dark("#000000",m_useDarkStyle),Radio::convert_dark("#00ff00",m_useDarkStyle),Radio::convert_dark("#808080",m_useDarkStyle)));
  }
  m_wideGraph->setLockTxFreq(m_lockTxFreq);
  if(m_lockTxFreq) on_pbR2T_clicked();
}

void MainWindow::on_skipTx1_clicked(bool checked)
{
  m_skipTx1=checked;
  ui->skipGrid->setChecked(checked);
  if(checked) { if(ui->txrb1->isChecked()) on_txb2_clicked(); if(ui->genMsg->text() == ui->tx1->text()) ui->genMsg->setText(ui->tx2->text()); }
  else { if(ui->txrb2->isChecked()) on_txb1_clicked(); if(ui->genMsg->text() == ui->tx2->text()) ui->genMsg->setText(ui->tx1->text()); }
}

void MainWindow::on_skipGrid_clicked(bool checked)
{
  m_skipTx1=checked;
  ui->skipTx1->setChecked(checked);
  if(checked) { if(ui->txrb1->isChecked()) { ui->txrb2->setChecked(true); m_QSOProgress = REPORT; m_nlasttx=2; m_ntx=7; if(m_transmitting) m_restart=true;
                                             if(ui->genMsg->text() == ui->tx1->text()) ui->genMsg->setText(ui->tx2->text()); }}
  else { if(ui->txrb2->isChecked()) { ui->txrb1->setChecked(true); m_QSOProgress = REPLYING; m_nlasttx=1; m_ntx=7; if(m_transmitting) m_restart=true;
                                      if(ui->genMsg->text() == ui->tx2->text()) ui->genMsg->setText(ui->tx1->text()); }}	  
}

void MainWindow::handle_transceiver_update (Transceiver::TransceiverState const& s)
{
  // qDebug () << "MainWindow::handle_transceiver_update:" << s;
  Transceiver::TransceiverState old_state {m_rigState};

  if(m_config.write_decoded_debug()) {
    QString curPttState = m_rigState.ptt () ? "PTT On" : "PTT Off";
    QString reqPttState = s.ptt () ? "PTT On" : "PTT Off";
    QString tx_when_ready = m_tx_when_ready ? "true" : "false";
    writeToALLTXT("handle_transceiver_update started, current rig state: " + curPttState + 
      ", requested state: " + reqPttState + ", m_tx_when_ready: " + tx_when_ready + ", g_iptt=" + QString::number(g_iptt));
   }
//   printf("%s(%0.1f) tranceiver update %d %d old %d new %d\n",m_jtdxtime->currentDateTimeUtc2().toString("hh:mm:ss.zzz").toStdString().c_str(),
//     m_jtdxtime->GetOffset(),m_tx_when_ready,g_iptt,m_rigState.ptt (),s.ptt ());
  if (!s.ptt() && m_rigState.ptt () && (m_transmitting || m_tune)) haltTx("Halt Tx from rig detected ");
  if (s.ptt () && !m_rigState.ptt ()) // safe to start audio
                                      // (caveat - DX Lab Suite Commander)
    {
 // waiting to Tx and still needed
 //Start-of-transmission sequencer delay
      if (m_tx_when_ready && g_iptt) {
//          QThread::currentThread()->setPriority(QThread::HighestPriority);
          int ms_delay=1000*m_config.txDelay();
          if(m_mode=="FT4" || m_mode=="FT2") ms_delay=20;   // CE3TSK step 6: FT2 starts 150 ms into the period, so the sequencer cannot take the default second
          ptt1Timer.start(ms_delay);
//          printf("ptt1Timer started\n");
          if(m_config.write_decoded_debug()) writeToALLTXT("ptt1Timer started");
      }
      m_tx_when_ready = false;
    }
  if(m_config.do_snr() && m_rigState.level() != s.level()) {
      ui->S_meter_button->setText(Radio::convert_Smeter(s.level(),ui->S_meter_button->isChecked()));
  }
  if(m_config.do_pwr()) {
    if (m_rigState.power() != s.power()) {
      ui->PWRlabel->setText(QString {tr("Pwr<br>%1 W")}.arg (round(s.power()/1000.)));
    }
    if (m_rigState.swr() != s.swr()) {
      if (s.swr() > 0) {
        if (s.swr()<1000) ui->SWRlabel->setText(QString {"swr%1"}.arg (s.swr()/100.,0,'f',2)); else ui->SWRlabel->setText(QString {"swr%1"}.arg (s.swr()/100.,0,'f',1));}
      else
        ui->SWRlabel->setText("");
    }
  }    
  m_rigState = s;
  auto old_freqNominal = m_freqNominal;
  m_freqNominal = s.frequency ();
  highlightBandButton ();   // CE3TSK
  // initializing
  if (old_state.online () == false && s.online () == true) {
      on_monitorButton_clicked(true);
      if(m_config.write_decoded_debug()) writeToALLTXT("handle_transceiver_update: transceiver state transition from offline to online");
      if(m_mode=="FT8") on_actionFT8_triggered();
      else if(m_mode=="FT4") on_actionFT4_triggered();
      else if(m_mode=="FT2") on_actionFT2_triggered();   // CE3TSK
      else if(m_mode=="JT9+JT65") on_actionJT9_JT65_triggered();
      else if(m_mode=="JT9") on_actionJT9_triggered();
      else if(m_mode=="JT65") on_actionJT65_triggered();
      else if(m_mode=="T10") on_actionT10_triggered();
      else if(m_mode=="WSPR-2") on_actionWSPR_2_triggered();
  }
  if (s.frequency () != old_state.frequency () || s.split () != m_splitMode) {
      m_splitMode = s.split ();
      if (!s.ptt ()) {
          if (old_freqNominal != m_freqNominal) m_freqTxNominal = m_freqNominal;
          if (m_monitoring) m_lastMonitoredFrequency = m_freqNominal;
          if (m_lastDialFreq != m_freqNominal)
            {
              m_lastDialFreq = m_freqNominal;
              m_secBandChanged=m_jtdxtime->currentMSecsSinceEpoch2()/1000;
//              if((s.frequency () < 30000000u || (s.frequency () > 30000000u && !m_config.tx_QSY_allowed ()))  && m_mode.left(4)!="WSPR") {
                // Write freq changes to ALL.TXT.
                QFile f2 {m_dataDir.absoluteFilePath (m_jtdxtime->currentDateTimeUtc2().toString("yyyyMM_")+"ALL.TXT")};
                if (f2.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                  QTextStream out(&f2);
                  out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss")
                      << "  " << qSetRealNumberPrecision (12) << (m_freqNominal / 1.e6) << " MHz  "
                      << m_mode << " JTDX v" << QCoreApplication::applicationVersion () << revision () <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

                  f2.close();
                } else {
                  JTDXMessageBox::warning_message (this, "", tr ("File Open Error")
                                               , tr ("Cannot open \"%1\" for append: %2")
                                               .arg (f2.fileName ()).arg (f2.errorString ()));
                }
//              }

              if (m_config.spot_to_psk_reporter ()) {
                pskSetLocal ();
              }
              statusChanged();
              m_wideGraph->setDialFreq(m_freqNominal / 1.e6);
            }
      } else {
          m_freqTxNominal = s.split () ? s.tx_frequency (): s.frequency ();
      }
      if (s.split ()) {
        setXIT (ui->TxFreqSpinBox->value ());
      }
  }

  displayDialFrequency ();
  setRigLamp ("#00ff00");
  m_rigOk=true;
  ui->readFreq->setEnabled (false);
  ui->readFreq->setText (s.split () ? "S" : "");
  if(m_config.write_decoded_debug()) {
    QString pttstate = s.ptt () ? "PTT On" : "PTT Off";
    QString splitstate = s.split () ? " Split On" : " Split Off";
    writeToALLTXT("handle_transceiver_update " + pttstate + splitstate  + " s.frequency:" + QString::number(s.frequency(),10) + " s.tx_frequency:"  + QString::number(s.tx_frequency(),10));
  }
  if(m_start2) { if(m_config.monitor_off_at_startup ()) on_monitorButton_clicked(false); m_start2=false; }
}

void MainWindow::handle_transceiver_failure (QString const& reason)
{
  setRigLamp ("#ff0000");
  m_rigOk=false;
  ui->readFreq->setEnabled (true);
  haltTx("Rig control error: " + reason + " ");
  rigFailure (tr("Rig Control Error"), reason);
}

void MainWindow::rigFailure (QString const& reason, QString const& detail)
{
  static bool first_error {true};
  if (first_error) {
      // one automatic retry
      QTimer::singleShot (0, this, SLOT (rigOpen ()));
      first_error = false;
  } else {
      m_rigErrorMessageBox.setText (reason);
      m_rigErrorMessageBox.setDetailedText (detail);

      // don't call slot functions directly to avoid recursion
      switch (m_rigErrorMessageBox.exec ())
        {
        case JTDXMessageBox::Ok:
          QTimer::singleShot (0, this, SLOT (on_actionSettings_triggered ()));
          break;

        case JTDXMessageBox::Retry:
          QTimer::singleShot (0, this, SLOT (rigOpen ()));
          break;

        case JTDXMessageBox::Cancel:
          QTimer::singleShot (0, this, SLOT (close ()));
          break;
        }
      first_error = true;       // reset
  }
}

void MainWindow::transmit (double snr)
{
  double toneSpacing=0.0;
  if (m_modeTx == "FT8") {
//    toneSpacing=12000.0/1920.0;
    toneSpacing=-3.0; //GFSK wave
    if (m_tci) Q_EMIT m_config.transceiver_modulator_start(NUM_FT8_SYMBOLS,1920.0,ui->TxFreqSpinBox->value()-m_XIT,toneSpacing,true,snr,m_TRperiod);
    else Q_EMIT sendMessage (NUM_FT8_SYMBOLS,1920.0,ui->TxFreqSpinBox->value()-m_XIT,toneSpacing,m_soundOutput,
                        m_config.audio_output_channel(),true,snr,m_TRperiod);
  }
  else if (m_modeTx == "FT4") {
    toneSpacing=-2.0;                     //Transmit a pre-computed, filtered waveform.
    if (m_tci) Q_EMIT m_config.transceiver_modulator_start(NUM_FT4_SYMBOLS,576.0,ui->TxFreqSpinBox->value()-m_XIT,toneSpacing,true,snr,m_TRperiod);
    else Q_EMIT sendMessage (NUM_FT4_SYMBOLS,576.0,ui->TxFreqSpinBox->value()-m_XIT,toneSpacing,m_soundOutput,
                        m_config.audio_output_channel(),true,snr,m_TRperiod);
  }
  else if (m_modeTx == "FT2") {   // CE3TSK step 6: FT4's 105 symbols, 288 samples each
    toneSpacing=-2.0;                     //Transmit a pre-computed, filtered waveform.
    if (m_tci) Q_EMIT m_config.transceiver_modulator_start(NUM_FT4_SYMBOLS,288.0,ui->TxFreqSpinBox->value()-m_XIT,toneSpacing,true,snr,m_TRperiod);
    else Q_EMIT sendMessage (NUM_FT4_SYMBOLS,288.0,ui->TxFreqSpinBox->value()-m_XIT,toneSpacing,m_soundOutput,
                        m_config.audio_output_channel(),true,snr,m_TRperiod);
  }
  else if (m_modeTx == "JT65") {
    toneSpacing=11025.0/4096.0;
    if (m_tci) Q_EMIT m_config.transceiver_modulator_start(NUM_JT65_SYMBOLS,4096.0*12000.0/11025.0,ui->TxFreqSpinBox->value()-m_XIT,toneSpacing,true,snr,m_TRperiod);
    else Q_EMIT sendMessage (NUM_JT65_SYMBOLS,4096.0*12000.0/11025.0,ui->TxFreqSpinBox->value()-m_XIT,toneSpacing,m_soundOutput,
                        m_config.audio_output_channel(),true,snr,m_TRperiod);
  }
  else if (m_modeTx == "JT9") {
    double sps=m_nsps; m_toneSpacing=12000.0/6912.0;
    if (m_tci) Q_EMIT m_config.transceiver_modulator_start(NUM_JT9_SYMBOLS,sps,ui->TxFreqSpinBox->value()-m_XIT, m_toneSpacing,true,snr,m_TRperiod);
    else Q_EMIT sendMessage (NUM_JT9_SYMBOLS,sps,ui->TxFreqSpinBox->value()-m_XIT, m_toneSpacing,m_soundOutput,
                        m_config.audio_output_channel(),true,snr,m_TRperiod);
  }
  else if (m_modeTx == "T10") {
    double sps=m_nsps; m_toneSpacing=4.0*12000.0/6912.0;
    if (m_tci) Q_EMIT m_config.transceiver_modulator_start(NUM_T10_SYMBOLS,sps,ui->TxFreqSpinBox->value()-m_XIT,m_toneSpacing,true,snr,m_TRperiod);
    else Q_EMIT sendMessage (NUM_T10_SYMBOLS,sps,ui->TxFreqSpinBox->value()-m_XIT,m_toneSpacing,m_soundOutput,
                        m_config.audio_output_channel(),true,snr,m_TRperiod);
  }
  else if (m_mode=="WSPR-2") {
    if (m_tci) Q_EMIT m_config.transceiver_modulator_start(NUM_WSPR_SYMBOLS,8192.0,ui->TxFreqSpinBox->value()-1.5*12000/8192,m_toneSpacing,true,snr,m_TRperiod);
    else Q_EMIT sendMessage (NUM_WSPR_SYMBOLS,8192.0,ui->TxFreqSpinBox->value()-1.5*12000/8192,m_toneSpacing,m_soundOutput,
                        m_config.audio_output_channel(),true,snr,m_TRperiod);
  }
  if(m_nlasttx==0 && !m_tune) m_nlasttx=m_ntx;
}

void MainWindow::on_outAttenuation_valueChanged (int a)
{
  /* CE3TSK: only a change the operator made says the slider holds a real value. m_PwrBandSetOK is
     false around our own setValue calls (the band and tune memories), and setting the flag there
     defeated the guard it exists for: a tune started before band_changed () had restored the drive
     set the flag from its own setValue and then memorised the .ui default. band_changed () sets
     the flag itself once it restores. */
  if (m_PwrBandSetOK) m_outAttenuationRestored = true;
  QString tt_str; int areversed=450-a;
  qreal dBAttn {areversed / 10.};       // slider interpreted as dB / 100
  QString curBand;
  if (m_tune && m_config.pwrBandTuneMemory()) { tt_str = tr ("Tune digital gain"); }
  else { tt_str = tr ("Transmit digital gain"); }
  tt_str += (areversed ? QString::number (-dBAttn, 'f', 1) : " 0") + "dB";
  if (!m_block_pwr_tooltip) QToolTip::showText (QCursor::pos (), tt_str, ui->outAttenuation);
  if (m_mode == "JT9+JT65" && m_modeTx == "JT65") { curBand = ui->bandComboBox->currentText()+m_modeTx; }
  else { curBand = ui->bandComboBox->currentText()+m_mode; }
  if (m_PwrBandSetOK && !m_tune && m_config.pwrBandTxMemory ()) {
      m_pwrBandTxMemory[curBand] = a; // remember our Tx pwr
      qDebug () << "Tx=" << QString::number(a);
  }
  if (m_PwrBandSetOK && m_tune && m_config.pwrBandTuneMemory()) {
      m_pwrBandTuneMemory[curBand] = a; // remember our Tune pwr
      qDebug () << "Tune=" << QString::number(a);
  }
  // Updating attenuation for tuning is done in stop_tuning
  if (m_tci) Q_EMIT m_config.transceiver_txvolume(dBAttn);
  else Q_EMIT outAttenuationChanged (dBAttn);
}

void MainWindow::on_actionShort_list_of_add_on_prefixes_and_suffixes_triggered()
{
  if (!m_prefixes) m_prefixes.reset (new HelpTextWindow {tr ("Prefixes"), R"(Short-list of Add-On DXCC Prefixes:

 1A    1S    3A    3B6   3B8   3B9   3C    3C0   3D2   3D2C  3D2R  3DA   3V    3W    3X   
 3Y    3YB   3YP   4J    4L    4S    4U1I  4U1U  4W    4X    5A    5B    5H    5N    5R   
 5T    5U    5V    5W    5X    5Z    6W    6Y    7O    7P    7Q    7X    8P    8Q    8R   
 9A    9G    9H    9J    9K    9L    9M2   9M6   9N    9Q    9U    9V    9X    9Y    A2   
 A3    A4    A5    A6    A7    A9    AP    BS7   BV    BV9   BY    C2    C3    C5    C6   
 C9    CE    CE0X  CE0Y  CE0Z  CE9   CM    CN    CP    CT    CT3   CU    CX    CY0   CY9  
 D2    D4    D6    DL    DU    E3    E4    EA    EA6   EA8   EA9   EI    EK    EL    EP   
 ER    ES    ET    EU    EX    EY    EZ    F     FG    FH    FJ    FK    FKC   FM    FO   
 FOA   FOC   FOM   FP    FR    FRG   FRJ   FRT   FT5W  FT5X  FT5Z  FW    FY    M     MD   
 MI    MJ    MM    MU    MW    H4    H40   HA    HB    HB0   HC    HC8   HH    HI    HK   
 HK0A  HK0M  HL    HM    HP    HR    HS    HV    HZ    I     IS    IS0   J2    J3    J5   
 J6    J7    J8    JA    JDM   JDO   JT    JW    JX    JY    K     KG4   KH0   KH1   KH2  
 KH3   KH4   KH5   KH5K  KH6   KH7   KH8   KH9   KL    KP1   KP2   KP4   KP5   LA    LU   
 LX    LY    LZ    OA    OD    OE    OH    OH0   OJ0   OK    OM    ON    OX    OY    OZ   
 P2    P4    PA    PJ2   PJ7   PY    PY0F  PT0S  PY0T  PZ    R1F   R1M   S0    S2    S5   
 S7    S9    SM    SP    ST    SU    SV    SVA   SV5   SV9   T2    T30   T31   T32   T33  
 T5    T7    T8    T9    TA    TF    TG    TI    TI9   TJ    TK    TL    TN    TR    TT   
 TU    TY    TZ    UA    UA2   UA9   UK    UN    UR    V2    V3    V4    V5    V6    V7   
 V8    VE    VK    VK0H  VK0M  VK9C  VK9L  VK9M  VK9N  VK9W  VK9X  VP2E  VP2M  VP2V  VP5  
 VP6   VP6D  VP8   VP8G  VP8H  VP8O  VP8S  VP9   VQ9   VR    VU    VU4   VU7   XE    XF4  
 XT    XU    XW    XX9   XZ    YA    YB    YI    YJ    YK    YL    YN    YO    YS    YU   
 YV    YV0   Z2    Z3    ZA    ZB    ZC4   ZD7   ZD8   ZD9   ZF    ZK1N  ZK1S  ZK2   ZK3
 ZL    ZL7   ZL8   ZL9   ZP    ZS    ZS8   KC4   E5

Short-list of Add-on Suffixes:    /0 /1 /2 /3 /4 /5 /6 /7 /8 /9 /A /P
)", {"Courier", 10}});
  m_prefixes->showNormal();
  m_prefixes->raise ();
}

bool MainWindow::shortList(QString callsign)
{
  int n=callsign.length();
  int i1=callsign.indexOf("/");
  Q_ASSERT(i1>0 and i1<n);
  QString t1=callsign.left(i1);
  QString t2=callsign.mid(i1+1,n-i1-1);
  bool b=(m_pfx.contains(t1) or m_sfx.contains(t2));
  return b;
}

bool MainWindow::isAutoSeq73(QString const& text)
{
  QStringList parts = text.split (' ', SkipEmptyParts);
  auto partsSize = parts.size ();
  bool b=((partsSize > 0 && (parts[0] == "73" || parts[0] == "TNX" || parts[0] == "TKS" || parts[0] == "TU"))
       || (partsSize > 1 && (parts[1] == "73" || parts[1] == "TNX" || parts[1] == "TKS" || parts[1] == "TU"))
       || (partsSize > 2 && (parts[2] == "73" || parts[2] == "TNX" || parts[2] == "TKS" || parts[2] == "TU"))
       || (partsSize > 3 && (parts[3] == "73" || parts[3] == "TNX" || parts[3] == "TKS" || parts[3] == "TU")));
  return b;
}

void MainWindow::enableHoundAccess(bool b)
{
  /* CE3TSK: one owner for whether Hound is reachable. It is an FT8 only feature - every mode
     slot calls this, true from FT8 and false from the rest - and a contest forbids it as well,
     so the two conditions meet here rather than fighting. Without this, selecting FT8 during a
     contest called enableHoundAccess(true) and quietly un-greyed a control the contest had
     locked, because applyContestLock() does not run again on a mode change. */
  b = b && !m_wwDigi;
  if(b) { ui->actionEnable_hound_mode->setEnabled(true); ui->HoundButton->setEnabled(true); }
  else { ui->actionEnable_hound_mode->setChecked(false); ui->actionEnable_hound_mode->setEnabled(false); ui->HoundButton->setEnabled(false);
         ui->actionUse_TX_frequency_jumps->setChecked(false);
  }
}

void MainWindow::setHoundAppearance(bool hound)
{
  bool b = !hound;
  ui->tx2->setEnabled(b); ui->txb2->setEnabled(b);
  ui->tx4->setEnabled(b); ui->txb4->setEnabled(b);
  ui->tx5->setEnabled(b); ui->txb5->setEnabled(b);
  ui->tx6->setEnabled(b); ui->txb6->setEnabled(b);
  ui->pbCallCQ->setEnabled(b); ui->pbAnswerCaller->setEnabled(b);
  ui->pbSendRRR->setEnabled(b); ui->pbSend73->setEnabled(b);
  ui->freeTextMsg->setEnabled(b); ui->rbFreeText->setEnabled(b);
}

void MainWindow::setLastLogdLabel()
{
  if(m_lastloggedcall.isEmpty()) { lastlogged_label->setText(tr("Logd ")); lastlogged_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#ffffff",m_useDarkStyle))); }
  else { lastlogged_label->setText(tr("Logd ") + m_lastloggedcall); lastlogged_label->setStyleSheet(QString("QLabel{background: %1}").arg(Radio::convert_dark("#7fff7f",m_useDarkStyle))); }
}

void MainWindow::writeToALLTXT(QString const& text)
{
  QFile f {m_dataDir.absoluteFilePath (m_jtdxtime->currentDateTimeUtc2().toString("yyyyMM_")+"ALL.TXT")};
  if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
     QTextStream out(&f);
     out << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz")  << "(" << m_jtdxtime->GetOffset() << ")" << "  " << text <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

     if(text.endsWith("count reached")) out << "Counters: "
       << "answerCQ" << (m_config.answerCQCount() ? "-On value=" : "-Off value=") << m_config.nAnswerCQCounter()
       << "; answerInCall" << (m_config.answerInCallCount() ? "-On value=" : "-Off value=") << m_config.nAnswerInCallCounter()
       << "; sentRReport" << (m_config.sentRReportCount() ? "-On value=" : "-Off value=") << m_config.nSentRReportCounter()
       << "; sentRR7373" << (m_config.sentRR7373Count() ? "-On value=" : "-Off value=") << m_config.nSentRR7373Counter()
       <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

     f.close();
  } else {
     JTDXMessageBox::warning_message (this, "", tr ("File Open Error")
                                  , tr ("Cannot open \"%1\" for append: %2")
                                  .arg (f.fileName ()).arg (f.errorString ()));
  }
}

void MainWindow::setTxMsgBtnColor()
{
  if(0 == ui->tabWidget->currentIndex ()) {
    if(m_ntx==1) ui->txb1->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_ntx==2) ui->txb2->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_ntx==3) ui->txb3->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_ntx==4) ui->txb4->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_ntx==5) ui->txb5->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_ntx==6) ui->txb6->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
  }
  else if(1 == ui->tabWidget->currentIndex ()) {
    if(m_QSOProgress == CALLING) ui->pbCallCQ->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_QSOProgress == REPLYING) ui->pbAnswerCQ->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_QSOProgress == REPORT) ui->pbAnswerCaller->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_QSOProgress == ROGER_REPORT) ui->pbSendReport->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_QSOProgress == ROGERS) ui->pbSendRRR->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
    else if(m_QSOProgress == SIGNOFF) ui->pbSend73->setStyleSheet(QString("QPushButton{background: %1}").arg(Radio::convert_dark("#88ff88",m_useDarkStyle)));
  }
}

void MainWindow::resetTxMsgBtnColor()
{
  if(0 == ui->tabWidget->currentIndex ()) {
    ui->txb1->setStyleSheet(""); ui->txb2->setStyleSheet(""); ui->txb3->setStyleSheet("");
    ui->txb4->setStyleSheet(""); ui->txb5->setStyleSheet(""); ui->txb6->setStyleSheet("");
  }
  else if(1 == ui->tabWidget->currentIndex ()) {
    ui->pbCallCQ->setStyleSheet(""); ui->pbAnswerCQ->setStyleSheet(""); ui->pbAnswerCaller->setStyleSheet("");
    ui->pbSendReport->setStyleSheet(""); ui->pbSendRRR->setStyleSheet(""); ui->pbSend73->setStyleSheet("");
  }
}

void MainWindow::pskSetLocal ()
{
  // find the station row, if any, that matches the band we are on
  auto stations = m_config.stations ();
  auto matches = stations->match (stations->index (0, StationList::band_column)
                                  , Qt::DisplayRole
                                  , ui->bandComboBox->currentText ()
                                  , 1
                                  , Qt::MatchExactly);
  QString antenna_description;
  if (!matches.isEmpty ()) antenna_description = stations->index (matches.first ().row (), StationList::description_column).data ().toString ();
  // qDebug() << "To PSKreporter: local station details";
  /* CE3TSK: the fork identifies itself to PSK Reporter as "JTDX v<version> <revision> contest",
     so a spot from this build is distinguishable from stock JTDX in the reporter's statistics.
     The program name stays JTDX - the reporter groups by it and the fork is a JTDX derivative. */
  psk_Reporter->setLocalStation(m_config.my_callsign (), m_config.my_grid (), antenna_description, QString {"JTDX v" + version() + (m_tci ? " tci " : " ") + revision() + " contest"}.simplified ());
}

void MainWindow::transmitDisplay (bool transmitting)
{
  if (transmitting == m_transmitting) {
    if (transmitting) {
      ui->signal_meter_widget->setValue(0);
      if (m_monitoring) monitor (false);
      m_btxok=true;
    }
    auto QSY_allowed = !transmitting or m_config.tx_QSY_allowed () or !m_config.split_mode ();
    if (ui->pbTxLock->isChecked ()) {
      ui->RxFreqSpinBox->setEnabled (QSY_allowed);
      ui->pbT2R->setEnabled (QSY_allowed);
    }
    if(m_mode!="WSPR") {
      ui->TxFreqSpinBox->setEnabled (QSY_allowed);
      ui->pbR2T->setEnabled (QSY_allowed);
      ui->pbTxLock->setEnabled (QSY_allowed);
    }
    // the following are always disallowed in transmit
    ui->menuMode->setEnabled (!transmitting);
    if (!transmitting) {
      // allow mode switch in Rx when in dual mode
      if ("JT9+JT65" == m_mode) ui->pbTxMode->setEnabled (true);
    } else {
      ui->pbTxMode->setEnabled (false);
    }
  }
}

// Takes a decoded message line and sets it up for reply
void MainWindow::replyToUDP (QTime time, qint32 snr, float delta_time, quint32 delta_frequency, QString const& mode, QString const& message_text
							, bool /*low_confidence*/, quint8 /*modifiers*/)
{
  if(m_config.write_decoded_debug()) writeToALLTXT("UDP Reply request received: " + message_text);
  if (!m_config.accept_udp_requests ()) { if(m_config.write_decoded_debug()) writeToALLTXT("UDP Reply request ignored: UDP accept option disabled"); return; }

  bool acceptMsg=false;
  if (m_acceptUDP==1) { acceptMsg=message_text.contains (QRegularExpression {R"(^(CQ |CQDX |QRZ))"}); }
  else if (m_acceptUDP==2) { acceptMsg=message_text.contains (QRegularExpression {R"(^(CQ |CQDX |QRZ)|(.+ 73|.+ RR73))"}); }
  else if (m_acceptUDP==3) { acceptMsg=true; }
  if (acceptMsg) {
//msgBox(message_text);
      // a message we are willing to accept
      QString format_string {"%1 %2 %3 %4 %5 %6"};
      auto const& time_string = time.toString (("~" == mode || ":" == mode) ? "hhmmss" : "hhmm");
      auto msgText = format_string
        .arg (time_string)
        .arg (snr, 3)
        .arg (delta_time, 4, 'f', 1)
        .arg (delta_frequency, 4)
        .arg (mode, -1)
        .arg (message_text);
      auto messages = ui->decodedTextBrowser->toPlainText ();
      int position;
      if(m_mode.startsWith("FT")) position = messages.lastIndexOf (msgText.left(42));
      else position = messages.lastIndexOf (msgText.left(40));
      if (position < 0) {
          // try again with with -0.0 delta time
          position = messages.lastIndexOf (format_string
                                           .arg (time_string)
                                           .arg (snr, 3)
                                           .arg ('-' + QString::number (delta_time, 'f', 1), 4)
                                           .arg (delta_frequency, 4)
                                           .arg (mode, -1)
                                           .arg (message_text));
      }
      if (position >= 0) {
// provide JTAlert->Log4OM interaction in scenario where the call is in DX Call window:
          if(message_text.contains(" " + m_hisCall + " ")) {
            QChar submode {0};
            m_messageClient->status_update (m_freqNominal, m_mode, m_hisCall, QString::number (ui->rptSpinBox->value ()),
                                            m_modeTx, ui->enableTxButton->isChecked (), m_transmitting, m_decoderBusy,
                                            ui->RxFreqSpinBox->value (), ui->TxFreqSpinBox->value (),
                                            m_config.my_callsign (), m_config.my_grid (), m_hisGrid, m_txwatchdog,
                                            submode != QChar::Null ? QString {submode} : QString {}, false, m_txFirst, true);
          }
          if (m_config.udpWindowToFront ()) {
              show ();
              raise ();
              activateWindow ();
          }
          if (m_config.udpWindowRestore () && isMinimized ()) {
              showNormal ();
              raise ();
          }
          // find the linefeed at the end of the line
          position = ui->decodedTextBrowser->toPlainText().indexOf("\n",position);
          auto start = messages.left (position).lastIndexOf (QChar::LineFeed) + 1;
          DecodedText message {messages.mid (start, position - start),this};
          m_decodedText2 = true;
// keyboard modifiers and low confidence(Hint) '*' symbol are not supported yet in UDP 'reply' procedure
//          Qt::KeyboardModifiers kbmod {modifiers << 24};
//          processMessage (message, kbmod);
          processMessage (messages, position, false, false);
          txwatchdog (false);
          if(m_windowPopup) QApplication::alert (this); // raise up task bar under window popup option, Logger32 compatibility
          m_decodedText2 = false;
          if(m_config.write_decoded_debug()) writeToALLTXT("UDP Reply request processed");
      } else {
//          qDebug () << "reply to message request ignored, decode not found:" << msgText;
          if(m_config.write_decoded_debug()) writeToALLTXT("UDP Reply request ignored, decode not found: " + msgText);
      }
  } else {
//    qDebug () << "rejecting UDP request to reply as decode is not valid";
    if(m_config.write_decoded_debug()) writeToALLTXT("rejecting UDP request to reply as decode is not valid: " + message_text);
  }
}

void MainWindow::replayDecodes ()
{
  // we accept this request even if the setting to accept UDP requests
  // is not checked

  // attempt to parse the decoded text
  Q_FOREACH (auto const& message, ui->decodedTextBrowser->toPlainText ().split ('\n', SkipEmptyParts))
    {
      if (message.size() >= 4 && message.left (4) != "----") {
          QStringList parts = message.split (' ', SkipEmptyParts);
          if (parts.size () >= 5 && parts[3].contains ('.')) { // WSPR
              postWSPRDecode (false, parts);
          } else {
              auto eom_pos = message.indexOf (' ', 35);
              // we always want at least the characters to position 35
              if (eom_pos < 35)
                {
                  eom_pos = message.size () - 1;
                }
              postDecode (false, message.left (eom_pos + 1));
          }
      }
    }
  statusChanged ();
}

void MainWindow::postDecode (bool is_new, QString const& message)
{
  auto const& decode = message.trimmed ();
  QStringList parts = decode.left (22).split (' ', SkipEmptyParts);
  if (parts.size () >= 5) {
      auto has_seconds = parts[0].size () > 4;
      bool low_confidence=(QChar {'*'} == decode.mid (has_seconds ? 23 + 24 : 21 + 24, 1)) || (QChar {'^'} == decode.mid (has_seconds ? 23 + 24 : 21 + 24, 1));
      m_messageClient->decode (is_new
                               , QTime::fromString (parts[0], has_seconds ? "hhmmss" : "hhmm")
                               , parts[1].toInt ()
                               , parts[2].toFloat (), parts[3].toUInt (), parts[4]
                               , decode.mid (has_seconds ? 23 : 21, 23)
                               , low_confidence
                               , m_diskData);
  }
}

void MainWindow::postWSPRDecode (bool is_new, QStringList parts)
{
  if (parts.size () < 8) parts.insert (6, "");

  m_messageClient->WSPR_decode (is_new, QTime::fromString (parts[0], "hhmm"), parts[1].toInt ()
                                , parts[2].toFloat (), Radio::frequency (parts[3].toFloat (), 6)
                                , parts[4].toInt (), parts[5], parts[6], parts[7].toInt (), m_diskData);
}

void MainWindow::networkError (QString const& e)
{
  if (JTDXMessageBox::Retry == JTDXMessageBox::warning_message (this, "", tr ("Network Error")
                                                  , tr ("Error: %1\nUDP server %2:%3")
                                                  .arg (e)
                                                  .arg (m_config.udp_server_name ())
                                                  .arg (m_config.udp_server_port ())
                                                  , "", JTDXMessageBox::Cancel | JTDXMessageBox::Retry, JTDXMessageBox::Cancel))
    {
      // retry server lookup
      m_messageClient->set_server (m_config.udp_server_name ());
    }
}

void MainWindow::p1ReadFromStdout()                        //p1readFromStdout
{
  while(p1.canReadLine()) {
    QString t = QString(p1.readLine());
    if(ui->cbNoOwnCall->isChecked()) {
      if(t.contains(" " + m_config.my_callsign() + " ")) continue;
      if(t.contains(" <" + m_config.my_callsign() + "> ")) continue;
    }
    QString t1;
    if(t.indexOf("<DecodeFinished>") >= 0) {
      m_bDecoded = m_nWSPRdecodes > 0;
      if(!m_diskData) {
        WSPR_history(m_dialFreqRxWSPR, m_nWSPRdecodes);
        if(m_nWSPRdecodes==0 and ui->band_hopping_group_box->isChecked()) {
          t = " Receiving " + m_mode + " ----------------------- " +
              m_config.bands ()->find (m_dialFreqRxWSPR);
          t=WSPR_hhmm(-60) + ' ' + t.rightJustified (66, '-');
          ui->decodedTextBrowser->appendText(t,Radio::convert_dark("#ffffff",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle),0," ",Radio::convert_dark("#000000",m_useDarkStyle));
        }
        killFileTimer.start (int(750.0*m_TRperiod)); //Kill 3/4 period from now
      }
      m_nWSPRdecodes=0;
      ui->DecodeButton->setChecked (false);
      if(m_uploadSpots
         && m_config.is_transceiver_online ()) { // need working rig control
#if QT_VERSION >= QT_VERSION_CHECK (5, 15, 0)
        uploadTimer.start(QRandomGenerator::global ()->bounded (0, 20000)); // Upload delay
#else
        uploadTimer.start(20000 * qrand()/((double)RAND_MAX + 1.0)); // Upload delay
#endif
      } else {
        QFile f(QDir::toNativeSeparators(m_dataDir.absolutePath()) + "/wspr_spots.txt");
        if(f.exists()) f.remove();
      }
      m_RxLog=0;
      m_startAnother=m_loopall;
      m_blankLine=true;
      m_decoderBusy = false;
      statusUpdate ();
    } else {

      int n=t.length();
      t=t.left(n-2) + "                                                  ";
      t.remove(QRegularExpression("\\s+$"));
      QStringList rxFields = t.split(QRegularExpression("\\s+"));
      QString rxLine;
      QString grid="";
      if ( rxFields.count() == 8 ) {
          rxLine = QString("%1 %2 %3 %4 %5   %6  %7  %8")
                  .arg(rxFields.at(0), 4)
                  .arg(rxFields.at(1), 4)
                  .arg(rxFields.at(2), 5)
                  .arg(rxFields.at(3), 11)
                  .arg(rxFields.at(4), 4)
                  .arg(rxFields.at(5).leftJustified (12))
                  .arg(rxFields.at(6), -6)
                  .arg(rxFields.at(7), 3);
          postWSPRDecode (true, rxFields);
          grid = rxFields.at(6);
      } else if ( rxFields.count() == 7 ) { // Type 2 message
          rxLine = QString("%1 %2 %3 %4 %5   %6  %7  %8")
                  .arg(rxFields.at(0), 4)
                  .arg(rxFields.at(1), 4)
                  .arg(rxFields.at(2), 5)
                  .arg(rxFields.at(3), 11)
                  .arg(rxFields.at(4), 4)
                  .arg(rxFields.at(5).leftJustified (12))
                  .arg("", -6)
                  .arg(rxFields.at(6), 3);
          postWSPRDecode (true, rxFields);
      } else {
          rxLine = t;
      }
      if(!grid.isEmpty ()) {
        double utch=0.0;
        int nAz=0,nEl=0,nDmiles=0,nDkm=0,nHotAz,nHotABetter;
        if (!grid.isEmpty() && !m_config.my_grid ().isEmpty())
          azdist_(const_cast <char *> ((m_config.my_grid () + "        ").left (8).toLatin1().constData()),
                const_cast <char *> ((grid + "        ").left (8).toLatin1().constData()),&utch,
                &nAz,&nEl,&nDmiles,&nDkm,&nHotAz,&nHotABetter,8,8);
        if(m_config.miles()) {
          t1 = QString::asprintf("%7d",nDmiles);
        } else {
          t1 = QString::asprintf("%7d",nDkm);
        }
        rxLine += t1;
      }

      if (m_config.insert_blank () && m_blankLine) {
        QString band;
        Frequency f=1000000.0*rxFields.at(3).toDouble()+0.5;
        band = ' ' + m_config.bands ()->find (f);
        ui->decodedTextBrowser->appendText(band.rightJustified (71, '-'),Radio::convert_dark("#ffffff",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle),0," ",Radio::convert_dark("#000000",m_useDarkStyle));
        m_blankLine = false;
      }
      m_nWSPRdecodes += 1;
      ui->decodedTextBrowser->appendText(rxLine,Radio::convert_dark("#ffffff",m_useDarkStyle),Radio::convert_dark("#000000",m_useDarkStyle),0," ",Radio::convert_dark("#000000",m_useDarkStyle));
    }
  }
}

QString MainWindow::WSPR_hhmm(int n)
{
  QDateTime t=m_jtdxtime->currentDateTimeUtc2().addSecs(n);
  int m=t.toString("hhmm").toInt()/2;
  QString t1;
  t1 = QString::asprintf("%04d",2*m);
  return t1;
}

void MainWindow::WSPR_history(Frequency dialFreq, int ndecodes)
{
  QDateTime t=m_jtdxtime->currentDateTimeUtc2().addSecs(-60);
  QString t1=t.toString("yyMMdd");
  QString t2=WSPR_hhmm(-60);
  QString t3;
  t3 = QString::asprintf("%13.6f",0.000001*dialFreq);
  if(ndecodes<0) {
    t1=t1 + " " + t2 + t3 + "  T";
  } else {
    QString t4;
    t4 = QString::asprintf("%4d",ndecodes);
    t1=t1 + " " + t2 + t3 + "  R" + t4;
  }
  QFile f {m_dataDir.absoluteFilePath ("WSPR_history.txt")};
  if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
    QTextStream out(&f);
    out << t1 <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

    f.close();
  } else {
    JTDXMessageBox::warning_message (this, "", tr ("File Error")
                                 , tr ("Cannot open \"%1\" for append: %2")
                                 .arg (f.fileName ()).arg (f.errorString ()));
  }
}

void MainWindow::uploadSpots()
{
  // do not spot replays or if rig control not working
  if(m_diskData || !m_config.is_transceiver_online ()) return;
  if(m_uploading) {
    qDebug() << "Previous upload has not completed, spots were lost";
    wsprNet->abortOutstandingRequests ();
    m_uploading = false;
  }
  QString rfreq = QString("%1").arg(0.000001*(m_dialFreqRxWSPR + 1500), 0, 'f', 6);
  QString tfreq = QString("%1").arg(0.000001*(m_dialFreqRxWSPR +
                        ui->TxFreqSpinBox->value()), 0, 'f', 6);
  wsprNet->upload(m_config.my_callsign(), m_config.my_grid(), rfreq, tfreq,
                  m_mode, QString::number(ui->enableTxButton->isChecked() ? m_pctx : 0),
                  QString::number(m_dBm), version(),
                  QDir::toNativeSeparators(m_dataDir.absolutePath()) + "/wspr_spots.txt");
  m_uploading = true;
}

void MainWindow::uploadResponse(QString response)
{
  if (response == "done") {
    m_uploading=false;
  } else {
    if (response.startsWith ("Upload Failed")) m_uploading=false;
    qDebug () << "WSPRnet.org status:" << response;
  }
}

void MainWindow::on_TxPowerComboBox_currentIndexChanged(const QString &arg1)
{
  int i1=arg1.indexOf(" ");
  m_dBm=arg1.left(i1).toInt();
}

void MainWindow::on_sbTxPercent_valueChanged(int n)
{
  m_pctx=n;
  if(m_pctx>0) {
    ui->pbTxNext->setEnabled(true);
  } else {
    m_txNext=false;
    ui->pbTxNext->setChecked(false);
    ui->pbTxNext->setEnabled(false);
  }
}

void MainWindow::on_cbUploadWSPR_Spots_toggled(bool b)
{
  m_uploadSpots=b;
  if(m_uploadSpots) ui->cbUploadWSPR_Spots->setStyleSheet("");
  if(!m_uploadSpots) ui->cbUploadWSPR_Spots->setStyleSheet(QString("QCheckBox{background: %1}").arg(Radio::convert_dark("#ffff00",m_useDarkStyle)));
}

void MainWindow::on_WSPRfreqSpinBox_valueChanged(int n) { ui->TxFreqSpinBox->setValue(n); }
void MainWindow::on_pbTxNext_clicked(bool b) { m_txNext=b; }

void MainWindow::WSPR_scheduling ()
{
  m_WSPR_tx_next = false;
  if (m_config.is_transceiver_online () // need working rig control for hopping
      && !m_config.is_dummy_rig ()
      && ui->band_hopping_group_box->isChecked ()) {
    auto hop_data = m_WSPR_band_hopping.next_hop (m_enableTx);
    qDebug () << "hop data: period:" << hop_data.period_name_
              << "frequencies index:" << hop_data.frequencies_index_
              << "tune:" << hop_data.tune_required_
              << "tx:" << hop_data.tx_next_;
    m_WSPR_tx_next = hop_data.tx_next_;
    if (hop_data.frequencies_index_ >= 0) { // new band
      ui->bandComboBox->setCurrentIndex (hop_data.frequencies_index_);
      on_bandComboBox_activated (hop_data.frequencies_index_);
      m_cmnd.clear ();
      QString hardware_band; /* CE3TSK: the argument, kept apart from the program path */
      QStringList prefixes {".bat", ".cmd", ".exe", ""};
      for (auto const& prefix : prefixes)
        {
          auto const& path = m_appDir + "/user_hardware" + prefix;
          QFile f {path};
          if (f.exists ()) {
            m_cmnd = QDir::toNativeSeparators (f.fileName ());
            hardware_band = m_config.bands ()->find (m_freqNominal).remove ('m');
          }
        }
      /* CE3TSK: the single string overload of QProcess::start() is deprecated and splits the
         command on whitespace, so an installation path containing a space was either invoked
         with a mangled argument list or not found at all. Pass the program and its argument
         separately, which is also what the deprecation message asks for. */
      if(!m_cmnd.isEmpty ()) p3.start(m_cmnd, QStringList {hardware_band});     // Execute user's hardware controller

      // Produce a short tuneup signal
      m_tuneup = false;
      if (hop_data.tune_required_) {
        m_tuneup = true;
        on_tuneButton_clicked (true);
        tuneATU_Timer.start (2500);
      }
    }
  }
  else {
    m_WSPR_tx_next = m_WSPR_band_hopping.next_is_tx ();
  }
}

void MainWindow::setRig ()
{
  if(m_transmitting && !m_config.tx_QSY_allowed ()) return;
  if ((m_monitoring || m_transmitting) && m_config.transceiver_online ()) {
      if(m_transmitting && m_config.split_mode ()) { Q_EMIT m_config.transceiver_tx_frequency (m_freqTxNominal); }
      else { Q_EMIT m_config.transceiver_frequency (m_freqNominal); }
      Q_EMIT m_config.transceiver_ft4_mode (m_mode == "FT4");
  }
}

void MainWindow::on_the_minute ()
{
  if(minuteTimer.isSingleShot ()) { minuteTimer.setSingleShot (false); minuteTimer.start (60 * 1000); } // run free
  else {
    auto const& ms_error = ms_minute_error (m_jtdxtime);
    // keep drift within +-1s
    if (qAbs (ms_error) > 1000) { minuteTimer.setSingleShot (true); minuteTimer.start (ms_error + 60 * 1000); }
    }
  if(m_config.watchdog () && !m_mode.startsWith ("WSPR")) {
    qint64 deltasec=(m_jtdxtime->currentMSecsSinceEpoch2()/1000) - m_secTxStopped;
    bool update=true;
    if(!m_txwatchdog) {
       if(m_modeTx=="FT8") { if(deltasec > 32) update=false; }
       else if(m_modeTx=="FT4") { if(deltasec > 16) update=false; } //to be checked
       else { if(deltasec > 134) update=false; }
    }
    if (update && (m_idleMinutes < m_config.watchdog ())) { ++m_idleMinutes; update_watchdog_label (); }
  }
  else { txwatchdog (false); }
  //3...4 minutes to stop AP decoding
  if(!m_transmitting && m_mode=="FT8" && (m_jtdxtime->currentMSecsSinceEpoch2()-m_mslastTX) > 120000) m_lapmyc=0;
}

void MainWindow::toggle_skipTx1 ()
{
  if(m_bMyCallStd) {
    if(!ui->skipTx1->isEnabled() && !m_wwDigi) { if(!m_houndMode) ui->skipTx1->setEnabled(true); ui->skipGrid->setEnabled(true); }   /* CE3TSK */
  } else {
    if(m_skipTx1) { m_skipTx1=false; ui->skipTx1->setChecked(false); ui->skipGrid->setChecked(false); }
    ui->skipTx1->setEnabled(false); ui->skipGrid->setEnabled(false);
  }
}

void MainWindow::statusUpdate () const
{
  if (!ui) return;
  QChar submode {0};
  m_messageClient->status_update (m_freqNominal, m_mode, m_hisCall,
                                  QString::number (ui->rptSpinBox->value ()),
                                  m_modeTx, ui->enableTxButton->isChecked (),
                                  m_transmitting, m_decoderBusy,
                                  ui->RxFreqSpinBox->value (), ui->TxFreqSpinBox->value (),
                                  m_config.my_callsign (), m_config.my_grid (),
                                  m_hisGrid, m_txwatchdog, submode != QChar::Null ? QString {submode} : QString {},
                                  false, m_txFirst, false);
}

void MainWindow::childEvent (QChildEvent * e)
{
  if (e->child ()->isWidgetType ())
    {
      switch (e->type ())
        {
        case QEvent::ChildAdded: add_child_to_event_filter (e->child ()); break;
        case QEvent::ChildRemoved: remove_child_from_event_filter (e->child ()); break;
        default: break;
        }
    }
  QMainWindow::childEvent (e);
}

// add widget and any child widgets to our event filter so that we can
// take action on key press and mouse press events anywhere in the main window
void MainWindow::add_child_to_event_filter (QObject * target)
{
  if (target && target->isWidgetType ())
    {
      target->installEventFilter (this);
    }
  auto const& children = target->children ();
  for (auto iter = children.begin (); iter != children.end (); ++iter)
    {
      add_child_to_event_filter (*iter);
    }
}

// recursively remove widget and any child widgets from our event filter
void MainWindow::remove_child_from_event_filter (QObject * target)
{
  auto const& children = target->children ();
  for (auto iter = children.begin (); iter != children.end (); ++iter)
    {
      remove_child_from_event_filter (*iter);
    }
  if (target && target->isWidgetType ())
    {
      target->removeEventFilter (this);
    }
}


void MainWindow::txwatchdog (bool triggered)
{
  auto prior = m_txwatchdog;
  m_txwatchdog = triggered;
  if (triggered)
    {
      m_bTxTime=false;
      if (m_enableTx) enableTx_mode (false);
      setTxStatusColour ("#ff8080");
      tx_status_label->setText (tr("Tx watchdog expired"));
    }
  else
    {
      m_idleMinutes = 0;
      update_watchdog_label ();
    }
  if (prior != triggered) statusUpdate ();
}

void MainWindow::update_watchdog_label ()
{
  if (m_config.watchdog () && !m_mode.startsWith ("WSPR"))
    {
      txwatchdog_label->setText (QString {tr("WD %1m")}.arg (m_config.watchdog () - m_idleMinutes));
      txwatchdog_label->setVisible (true);
    }
  else
    {
      txwatchdog_label->setText (QString {});
      txwatchdog_label->setVisible (false);
    }
}

void MainWindow::on_cbMenus_toggled(bool b)
{
  m_menus=b;
  hideMenus(!b);
  minimumSize().setHeight(422); 
  minimumSize().setWidth(733);
  dynamicButtonsInit();
}

void MainWindow::on_cbShowWanted_toggled(bool b)
{
  m_wantedchkd=b;
  ui->labWantCall->setVisible(b); ui->wantedCall->setVisible(b); ui->labWantCountry->setVisible(b); ui->wantedCountry->setVisible(b);
  ui->labWantPfx->setVisible(b); ui->wantedPrefix->setVisible(b); ui->labWantGrid->setVisible(b); ui->wantedGrid->setVisible(b);
  ui->cbClearCallsign->setVisible(b); ui->cbClearGrid->setVisible(b); dynamicButtonsInit();
}

void MainWindow::on_cbShowSpot_toggled(bool b) 
{
  ui->spotMsgLabel->setVisible(b); ui->spotEditLabel->setVisible(b);  ui->spotLineEdit->setVisible(b); ui->propEditLabel->setVisible(b); ui->propLineEdit->setVisible(b);
}

void MainWindow::dynamicButtonsInit()
{
  QSize size=this->size();
  int height=size.height();
  if(m_menus) {
    if(!m_wantedchkd) {
      if(height <= 435) { ui->bypassButton->hide(); ui->singleQSOButton->hide(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
      else if(height > 435 && height <= 475) { ui->bypassButton->show(); ui->singleQSOButton->hide(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
      else if(height > 475 && height <= 500) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
      else if(height > 500 && height <= 525) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->show(); ui->stopButton->hide(); }
      else if(height > 525) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->show(); ui->stopButton->show(); }
    } else {
      if(height <= 415) { ui->bypassButton->hide(); ui->singleQSOButton->hide(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
      else if(height > 415 && height <= 450) { ui->bypassButton->show(); ui->singleQSOButton->hide(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
      else if(height > 450 && height <= 485) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
      else if(height > 485 && height <= 520) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->show(); ui->stopButton->hide(); }
      else if(height > 520) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->show(); ui->stopButton->show(); }
    }
  } else {
    if(height <= 425) { ui->bypassButton->hide(); ui->singleQSOButton->hide(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
    else if(height > 425 && height <= 450) { ui->bypassButton->show(); ui->singleQSOButton->hide(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
    else if(height > 450 && height <= 475) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->hide(); ui->stopButton->hide(); }
    else if(height > 475 && height <= 500) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->show(); ui->stopButton->hide(); }
    else if(height > 500) { ui->bypassButton->show(); ui->singleQSOButton->show(); ui->AnsB4Button->show(); ui->stopButton->show(); }
  }
}
