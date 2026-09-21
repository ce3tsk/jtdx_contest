
// -*- Mode: C++ -*-
#ifndef DISPLAYTEXT_H
#define DISPLAYTEXT_H

#include <QTextEdit>
#include "logbook/logbook.h"
#include "decodedtext.h"
#include "qsohistory.h"
#include "Radio.hpp"
#include "Configuration.hpp"   /* CE3TSK: SpecialOperatingActivity and contest scoring */

class Configuration;

class DisplayText : public QTextEdit
{
    Q_OBJECT;
public:
    explicit DisplayText(QWidget *parent = 0);
    void setConfiguration(Configuration const *);
    void setMyContinent (QString const&);
    void setContentFont (QFont const&);
    void insertLineSpacer(QString const&);
    int displayDecodedText(DecodedText* decodedText, QString myCall, QString hisCall, QString hisGrid,
                           bool once_notified, LogBook logBook, QsoHistory& qsoHistory,
                           QsoHistory& qsoHistory2, double dialFreq = 0, const QString app_mode = "",
                           bool bypassRxfFilters = false, bool bypassAllFilters = false, int rx_frq = 0,
                           QStringList wantedCallList = QStringList(), QStringList wantedPrefixList = QStringList(), QStringList wantedGridList = QStringList(),
                           QStringList wantedCountryList = QStringList(), bool windowPopup = false, QWidget* window = NULL);
    void displayTransmittedText(QString text, QString myCall, QString hisCall, QString skip_tx1, QString modeTx, qint32 txFreq,
                                QColor color_TxMsg, QsoHistory& qsoHistory);
    void displayQSY(QString text);
    void displayContestNotice(QString text);   // CE3TSK: why the software just dropped a QSO
    void displayFoxVerification(QString const& line, int verdict);   // CE3TSK: 1 verified (green), 2 invalid (red), 0 plain (a shown OTP line)
signals:
    void selectCallsign(bool alt, bool ctrl);

public slots:
  void appendText(QString const& text, QString const& bg = "#ffffff", QString const& color = "#000000", int std_type = 0, QString const& servis = " ", QString const& servis_color = "#000000", QString const& cntry = " ", bool forceBold = false, bool strikethrough = false, bool underline = false, bool DXped = false, bool overwrite = false, bool wanted = false);

protected:
    void mouseDoubleClickEvent(QMouseEvent *e);

private:

    bool scroll_;
    bool bold_;
    bool wwDigi_ = false; /* CE3TSK: WW Digi contest, grid is the exchange */
    /* CE3TSK: copied in setConfiguration() like every other setting, rather than keeping a
       Configuration pointer - this class deliberately holds no such pointer. my_grid can
       only change through the settings dialog, which calls setConfiguration() again. */
    QString myGrid_ = "";
    Configuration::SpecialOperatingActivity specialOp_ = Configuration::SpecialOperatingActivity::NONE;
    bool wastx_;
    bool useDarkStyle_;
    bool displayCountryName_;
    bool displayCountryPrefix_;
    bool displayNewCQZ_;
    bool displayNewCQZBand_;
    bool displayNewCQZBandMode_;
    bool displayNewITUZ_;
    bool displayNewITUZBand_;
    bool displayNewITUZBandMode_;
    bool displayNewDXCC_;
    bool displayNewDXCCBand_;
    bool displayNewDXCCBandMode_;
    bool displayNewGrid_;
    bool displayNewGridBand_;
    bool displayNewGridBandMode_;
    bool displayNewPx_;
    bool displayNewPxBand_;
    bool displayNewPxBandMode_;
    bool displayNewCall_;
    bool displayNewCallBand_;
    bool displayNewCallBandMode_;
    bool displayPotential_;
    bool displayTxtColor_;
    bool displayWorkedColor_;
    bool displayWorkedStriked_;
    bool displayWorkedUnderlined_;
    bool displayWorkedDontShow_;
    bool beepOnNewCQZ_;
    bool beepOnNewITUZ_;
    bool beepOnNewDXCC_;
    bool beepOnNewGrid_;
    bool beepOnNewPx_;
    bool beepOnNewCall_;
    bool beepOnMyCall_;
    bool RR73Marker_;
    bool otherMessagesMarker_;
    bool enableCountryFilter_;
    bool enableCallsignFilter_;
    bool enableMyConinentFilter_;
    bool hidefree_;
    bool showcq_;
    bool showcqrrr73_;
    bool showcq73_;
    bool redMarker_;
    bool blueMarker_;
    bool hidehintMarker_;
    bool hide_TX_messages_;
    QsoHistory::Status mystatus_ = QsoHistory::NONE;
    unsigned max_r_time = 0;
    QTextCharFormat m_charFormat;
    unsigned last_tx = 0;
    QString mygrid_ = "";
    QString myhisCall_ = "";
    QString myCall_ = "";
    QString myContinent_ = "";
    QString color_MyCall_;
    QString color_CQ_;
    QString color_StandardCall_;
    QString color_WorkedCall_;
    QString color_NewCQZ_;
    QString color_NewCQZBand_;
    QString color_NewITUZ_;
    QString color_NewITUZBand_;
    QString color_NewDXCC_;
    QString color_NewDXCCBand_;
    QString color_NewGrid_;
    QString color_NewGridBand_;
    QString color_NewPx_;
    QString color_NewPxBand_;
    QString color_NewCall_;
    QString color_NewCallBand_;
    QString hideContinents_;
    QString countries_;
    QString callsigns_;
    
    
};

#endif // DISPLAYTEXT_H
