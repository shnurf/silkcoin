#include "poolbrowser.h"
#include "ui_poolbrowser.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"
#include "clientmodel.h"
#include "bitcoinrpc.h"
#include <QDesktopServices>

#include <sstream>
#include <string>

using namespace json_spirit;

const QString kBaseUrl = "https://bittrex.com/api/v1/public/getmarketsummaries";
const QString kBaseUrl2 = "http://blockchain.info/tobtc?currency=USD&value=1";
const QString kBaseUrl3 = "https://bittrex.com/api/v1/public/getorderbook?market=BTC-SC&type=both&depth=50";
const QString kBaseUrl4 = "https://poloniex.com/public?command=returnTicker";
const QString kBaseUrl5 = "https://www.poloniex.com/public?command=returnOrderBook&currencyPair=BTC_SC";
const QString kBaseUrl6 = "https://bittrex.com/api/v1/public/getmarkethistory?market=BTC-SC&count=100";

QString bitcoinp = "";
double bitcoin2;
double lastuG;
QString bitcoing;
QString dollarg;
int mode=1;
QString lastp = "";
QString askp = "";
QString bidp = "";
QString highp = "";
QString lowp = "";
QString volumebp = "";
QString volumesp = "";
QString bop = "";
QString sop = "";
QString lastp2 = "";
QString askp2 = "";
QString bidp2 = "";
QString highp2 = "";
QString lowp2 = "";
QString volumebp2 = "";
QString volumesp2 = "";
QString bop2 = "";
QString sop2 = "";

PoolBrowser::PoolBrowser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PoolBrowser)
{
    ui->setupUi(this);
    ui->buyquan->header()->resizeSection(0,120);
    ui->sellquan->header()->resizeSection(0,120);
    setFixedSize(400, 420);
    ui->customPlot->addGraph();
    ui->customPlot->setBackground(Qt::transparent);
    ui->customPlot2->addGraph();
    ui->customPlot2->addGraph();
    ui->customPlot2->setBackground(Qt::transparent);




randomChuckNorrisJoke();
QObject::connect(&m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseNetworkResponse(QNetworkReply*)));
connect(ui->startButton, SIGNAL(pressed()), this, SLOT( randomChuckNorrisJoke()));
connect(ui->bittrex, SIGNAL(pressed()), this, SLOT( bittrex()));
connect(ui->poloniex, SIGNAL(pressed()), this, SLOT( poloniex()));
connect(ui->egal, SIGNAL(pressed()), this, SLOT( egaldo()));

}
void PoolBrowser::egaldo()
{
    QString temps = ui->egals->text();
    double totald = lastuG * temps.toDouble();
    double totaldq = bitcoing.toDouble() * temps.toDouble();
    ui->egald->setText(QString::number(totald) + " $ / "+QString::number(totaldq)+" BTC");

}


void PoolBrowser::bittrex()
{
    QDesktopServices::openUrl(QUrl("https://www.bittrex.com/Market/Index?MarketName=BTC-SC"));
}

void PoolBrowser::poloniex()
{
    QDesktopServices::openUrl(QUrl("https://poloniex.com/exchange/btc_sc"));
}

void PoolBrowser::randomChuckNorrisJoke()
{
    getRequest(kBaseUrl);
    getRequest(kBaseUrl2);
    getRequest(kBaseUrl3);
    getRequest(kBaseUrl6);
}

void PoolBrowser::getRequest( const QString &urlString )
{
    QUrl url ( urlString );
    QNetworkRequest req ( url );
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::SslV3);
    req.setSslConfiguration(config);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    m_nam.get(req);
}

void PoolBrowser::parseNetworkResponse(QNetworkReply *finished )
{

        QUrl what = finished->url();

    if ( finished->error() != QNetworkReply::NoError )
    {

        // A communication error has occurred
        emit networkError( finished->error() );
        return;
    }


if (what == kBaseUrl)
{
    double asku;
    QString askus;
    double lastu;
    QString lastus;
    double bidu;
    QString bidus;
    double volumeu;
    QString volumeus;
    double yestu;
    QString yestus;

    // QNetworkReply is a QIODevice. So we read from it just like it was a file
    QString data = finished->readAll();
    QStringList data2 = data.split("{\"MarketName\":\"BTC-SC\",\"High\":");
    QStringList high = data2[1].split(",\"Low\":"); // high = high
    QStringList low = high[1].split(",\"Volume\":");
    QStringList volume = low[1].split(",\"Last\":");
    QStringList last = volume[1].split(",\"BaseVolume\":");
    QStringList basevolume = last[1].split(",\"TimeStamp\":\"");
    QStringList time = basevolume[1].split("\",\"Bid\":");
    QStringList bid = time[1].split(",\"Ask\":");
    QStringList ask = bid[1].split(",\"OpenBuyOrders\":");
    QStringList openbuy = ask[1].split(",\"OpenSellOrders\":");
    QStringList opensell = openbuy[1].split(",\"PrevDay\":");
    QStringList yest = opensell[1].split(",\"Created\":");

   // 0.00000978,"Low":0.00000214,"Volume":3718261.74455189,"Last":0.00000558,
   //"BaseVolume":22.42443460,"TimeStamp":"2014-05-13T10:08:06.553","Bid":0.00000558,"Ask":0.00000559,"OpenBuyOrders":42,"OpenSellOrders":42,"PrevDay":0.00000861}
    lastu = last[0].toDouble() * bitcoin2;
    lastuG = lastu;
    lastus = QString::number(lastu);
    dollarg = lastus;
    bitcoing = last[0];

    if(last[0] > lastp)
    {
        ui->last->setText("<b><font color=\"green\">" + last[0] + "</font></b>");
        ui->lastu->setText("<b><font color=\"green\">" + lastus + " $</font></b>");
    } else if (last[0] < lastp) {
        ui->last->setText("<b><font color=\"red\">" + last[0] + "</font></b>");
         ui->lastu->setText("<b><font color=\"red\">" + lastus + " $</font></b>");
        } else {
    ui->last->setText(last[0]);
    ui->lastu->setText(lastus + " $");
    }

    asku = ask[0].toDouble() * bitcoin2;
    askus = QString::number(asku);

    if(ask[0] > askp)
    {
        ui->ask->setText("<b><font color=\"green\">" + ask[0] + "</font></b>");
        ui->asku->setText("<b><font color=\"green\">" + askus + " $</font></b>");
    } else if (ask[0] < askp) {
        ui->ask->setText("<b><font color=\"red\">" + ask[0] + "</font></b>");
        ui->asku->setText("<b><font color=\"red\">" + askus + " $</font></b>");
        } else {
    ui->ask->setText(ask[0]);
    ui->asku->setText(askus + " $");
    }

    bidu = bid[0].toDouble() * bitcoin2;
    bidus = QString::number(bidu);


    if(bid[0] > bidp)
    {
        ui->bid->setText("<b><font color=\"green\">" + bid[0] + "</font></b>");
        ui->bidu->setText("<b><font color=\"green\">" + bidus + " $</font></b>");
    } else if (bid[0] < bidp) {
        ui->bid->setText("<b><font color=\"red\">" + bid[0] + "</font></b>");
        ui->bidu->setText("<b><font color=\"red\">" + bidus + " $</font></b>");
        } else {
    ui->bid->setText(bid[0]);
    ui->bidu->setText(bidus + " $");
    }
    if(high[0] > highp)
    {
        ui->high->setText("<b><font color=\"green\">" + high[0] + "</font></b>");
    } else if (high[0] < highp) {
        ui->high->setText("<b><font color=\"red\">" + high[0] + "</font></b>");
        } else {
    ui->high->setText(high[0]);
    }
    if(low[0] > lowp)
    {
        ui->low->setText("<b><font color=\"green\">" + low[0] + "</font></b>");
    } else if (low[0] < lowp) {
        ui->low->setText("<b><font color=\"red\">" + low[0] + "</font></b>");
        } else {
    ui->low->setText(low[0]);
    }


    if(volume[0] > volumebp)
    {
        ui->volumeb->setText("<b><font color=\"green\">" + volume[0] + "</font></b>");

    } else if (volume[0] < volumebp) {
        ui->volumeb->setText("<b><font color=\"red\">" + volume[0] + "</font></b>");
        ui->volumeu->setText("<b><font color=\"red\">" + volumeus + " $</font></b>");
        } else {
    ui->volumeb->setText(volume[0]);
    ui->volumeu->setText(volumeus + " $");
    }

    volumeu = basevolume[0].toDouble() * bitcoin2;
    volumeus = QString::number(volumeu);

    if(basevolume[0] > volumesp)
    {
        ui->volumes->setText("<b><font color=\"green\">" + basevolume[0] + "</font></b>");
        ui->volumeu->setText("<b><font color=\"green\">" + volumeus + " $</font></b>");
    } else if (basevolume[0] < volumesp) {
        ui->volumes->setText("<b><font color=\"red\">" + basevolume[0] + "</font></b>");
        ui->volumeu->setText("<b><font color=\"red\">" + volumeus + " $</font></b>");
        } else {
    ui->volumes->setText(basevolume[0]);
    ui->volumeu->setText(volumeus + " $");
    }

    if(last[0].toDouble() > yest[0].toDouble())
    {
        yestu = ((last[0].toDouble() - yest[0].toDouble())/last[0].toDouble())*100;
        yestus = QString::number(yestu);

        ui->yest->setText("<b><font color=\"green\"> + " + yestus + " %</font></b>");


    }else
    {
        yestu = ((yest[0].toDouble() - last[0].toDouble())/yest[0].toDouble())*100;
        yestus = QString::number(yestu);
        ui->yest->setText("<b><font color=\"red\"> - " + yestus + " %</font></b>");

    }

    ui->bo->setText(openbuy[0]);
    ui->so->setText(opensell[0]);

    lastp = last[0];
    askp = ask[0];
    bidp = bid[0];
    highp = high[0];
    lowp = low[0];
    volumebp = volume[0];
    volumesp = basevolume[0];
    bop = openbuy[0];
    sop = opensell[0];
}

if (what == kBaseUrl2)
{

    // QNetworkReply is a QIODevice. So we read from it just like it was a file
    QString bitcoin = finished->readAll();
    bitcoin2 = (1 / bitcoin.toDouble());
    bitcoin = QString::number(bitcoin2);
    if(bitcoin > bitcoinp)
    {
        ui->bitcoin->setText("<b><font color=\"green\">" + bitcoin + " $</font></b>");
    } else if (bitcoin < bitcoinp) {
        ui->bitcoin->setText("<b><font color=\"red\">" + bitcoin + " $</font></b>");
        } else {
    ui->bitcoin->setText(bitcoin + " $");
    }

    bitcoinp = bitcoin;
}
if (what == kBaseUrl3)
{
    // QNetworkReply is a QIODevice. So we read from it just like it was a file
    QString marketd = finished->readAll();
    marketd = marketd.replace("{","");
    marketd = marketd.replace("}","");
    marketd = marketd.replace("\"","");
    marketd = marketd.replace("],\"sell\":","");
    marketd = marketd.replace(" ","");
    marketd = marketd.replace("]","");
    marketd = marketd.replace("Quantity:","");
    marketd = marketd.replace("Rate:","");
    QStringList marketd2 = marketd.split("["); // marketd2[1] => buy order marketd2[2] => sell
    QStringList marketdb = marketd2[1].split(",");
    QStringList marketds = marketd2[2].split(",");
    int mat = 50;
    if (marketdb.length() > marketds.length()) mat = marketds.length();
    if (marketds.length() > marketdb.length()) mat = marketdb.length();
    int z = 0;
    double highh = 0;
    double loww = 100000;
    double cumul = 0;
    double cumul2 = 0;
    double cumult = 0;
    QVector<double> x(50), y(50);
    QVector<double> x2(50), y2(50);
    ui->sellquan->clear();
    ui->buyquan->clear();
     ui->sellquan->sortByColumn(0, Qt::AscendingOrder);
      ui->sellquan->setSortingEnabled(true);
       ui->buyquan->setSortingEnabled(true);


       for (int i = 0; i < mat-1; i++) {

           QTreeWidgetItem * item = new QTreeWidgetItem();
           item->setText(0,marketdb[i+1]);
           item->setText(1,marketdb[i]);
           ui->buyquan->addTopLevelItem(item);

           QTreeWidgetItem * item2 = new QTreeWidgetItem();
           item2->setText(0,marketds[i+1]);
           item2->setText(1,marketds[i]);
           ui->sellquan->addTopLevelItem(item2);

           if (marketds[i+1].toDouble()*100000000 > highh) highh = marketds[i+1].toDouble()*100000000;
           if (marketdb[i+1].toDouble()*100000000 < loww) loww = marketdb[i+1].toDouble()*100000000;


           x[z] = marketdb[i+1].toDouble()*100000000;
           y[z] = cumul;
           x2[z] = marketds[i+1].toDouble()*100000000;
           y2[z] = cumul2;

           cumul = marketdb[i].toDouble() + cumul;
           cumul2 = marketds[i].toDouble() + cumul2;

           i = i + 1;
           z = z + 1;

       }
       if (cumul > cumul2 ) cumult=cumul;
       if (cumul < cumul2 ) cumult=cumul2;



          // create graph and assign data to it:

          ui->customPlot2->graph(0)->setData(x, y);
          ui->customPlot2->graph(1)->setData(x2, y2);

          ui->customPlot2->graph(0)->setPen(QPen(QColor(34, 177, 76)));
          ui->customPlot2->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
          ui->customPlot2->graph(1)->setPen(QPen(QColor(237, 24, 35)));
          ui->customPlot2->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

          // set axes ranges, so we see all data:
          ui->customPlot2->xAxis->setRange(loww, highh);
          ui->customPlot2->yAxis->setRange(loww, cumult);

         ui->customPlot2->replot();



}

if (what == kBaseUrl6)
{
    // QNetworkReply is a QIODevice. So we read from it just like it was a file
    QString marketd = finished->readAll();
    marketd = marketd.replace("{\"success\":true,\"message\":\"\",\"result\":[","");
    marketd = marketd.replace("\"","");
    marketd = marketd.replace("Id:","");
    marketd = marketd.replace("TimeStamp:","");
    marketd = marketd.replace("Quantity:","");
    marketd = marketd.replace("Price:","");
    marketd = marketd.replace("Total:","");
    marketd = marketd.replace("OrderType:","");

    QStringList marketdb = marketd.split("},{");

    int z = 0;
    ui->trades->clear();
    ui->trades->setColumnWidth(0,  60);
        ui->trades->setColumnWidth(1,  100);
        ui->trades->setColumnWidth(2,  100);
        ui->trades->setColumnWidth(3,  100);
        ui->trades->setColumnWidth(4,  180);
        ui->trades->setColumnWidth(5,  100);
     QVector<double> x(100), y(100);
      ui->trades->setSortingEnabled(false);
double highh = 0;
double loww = 100000;
    for (int i = 0; i < 99; i++) {

        marketdb[i] = marketdb[i].replace("}","");
        marketdb[i] = marketdb[i].replace("{","");
        QStringList dad = marketdb[i].split(",");

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setText(0,dad[5]);
        item->setText(1,dad[2]);
        item->setText(2,dad[3]);
        item->setText(3,dad[4]);
        QStringList temp = dad[1].replace("T"," ").split(".");
        item->setText(4,temp[0]);
        item->setText(5,dad[0]);

        ui->trades->addTopLevelItem(item);

        x[z] = 100-z;
        y[z] = (dad[3].toDouble())*100000000;

        if (dad[3].toDouble()*100000000 > highh) highh = dad[3].toDouble()*100000000;
        if (dad[3].toDouble()*100000000 < loww) loww = dad[3].toDouble()*100000000;

        z = z + 1;
    }


       // create graph and assign data to it:

       ui->customPlot->graph(0)->setData(x, y);
       ui->customPlot->graph(0)->setPen(QPen(QColor(34, 177, 76)));
       ui->customPlot->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
       // set axes ranges, so we see all data:
      ui->customPlot->xAxis->setRange(1, 100);
      ui->customPlot->yAxis->setRange(loww, highh);
      ui->customPlot->replot();

}

//if (what == kBaseUrl4)
//{
//    double asku;
//    QString askus;
//    double lastu;
//    QString lastus;
//    double bidu;
//    QString bidus;
//    double volumeu;
//    QString volumeus;
//    double yestu;
//    QString yestus;


//    QString data = finished->readAll();

//    QStringList data2 = data.split("\"BTC-SC\":{\"last\":\"");
//    QStringList last = data2[1].split("\",\"lowestAsk\":\""); // high = high
//    QStringList ask = last[1].split("\",\"highestBid\":\"");
//    QStringList bid = ask[1].split("\",\"percentChange\":\"");
//    QStringList PrevDay = bid[1].split("\",\"baseVolume\":\"");
//    QStringList basevolume = PrevDay[1].split("\",\"quoteVolume\":\"");
//    QStringList volume = basevolume[1].split("\",\"isFrozen\":\"");
//ui->last->setText(data);

//    lastu = last[0].toDouble() * bitcoin2;
//    lastus = QString::number(lastu);

//    if(last[0] > lastp2)
//    {
//        ui->last_2->setText("<b><font color=\"green\">" + last[0] + "</font></b>");
//        ui->last_2->setText("<b><font color=\"green\">" + lastus + " $</font></b>");
//    } else if (last[0] < lastp2) {
//        ui->last_2->setText("<b><font color=\"red\">" + last[0] + "</font></b>");
//         ui->last_2->setText("<b><font color=\"red\">" + lastus + " $</font></b>");
//        } else {
//    ui->last_2->setText(last[0]);
//    ui->last_2->setText(lastus + " $");
//    }

//    asku = ask[0].toDouble() * bitcoin2;
//    askus = QString::number(asku);

//    if(ask[0] > askp2)
//    {
//        ui->ask_2->setText("<b><font color=\"green\">" + ask[0] + "</font></b>");
//        ui->asku_2->setText("<b><font color=\"green\">" + askus + " $</font></b>");
//    } else if (ask[0] < askp2) {
//        ui->ask_2->setText("<b><font color=\"red\">" + ask[0] + "</font></b>");
//        ui->asku_2->setText("<b><font color=\"red\">" + askus + " $</font></b>");
//        } else {
//    ui->ask_2->setText(ask[0]);
//    ui->asku_2->setText(askus + " $");
//    }

//    bidu = bid[0].toDouble() * bitcoin2;
//    bidus = QString::number(bidu);


//    if(bid[0] > bidp2)
//    {
//        ui->bid_2->setText("<b><font color=\"green\">" + bid[0] + "</font></b>");
//        ui->bidu_2->setText("<b><font color=\"green\">" + bidus + " $</font></b>");
//    } else if (bid[0] < bidp2) {
//        ui->bid_2->setText("<b><font color=\"red\">" + bid[0] + "</font></b>");
//        ui->bidu_2->setText("<b><font color=\"red\">" + bidus + " $</font></b>");
//        } else {
//    ui->bid_2->setText(bid[0]);
//    ui->bidu_2->setText(bidus + " $");
//    }


//    if(volume[0] > volumebp2)
//    {
//        ui->volumeb_2->setText("<b><font color=\"green\">" + volume[0] + "</font></b>");

//    } else if (volume[0] < volumebp2) {
//        ui->volumeb_2->setText("<b><font color=\"red\">" + volume[0] + "</font></b>");
//        ui->volumeu_2->setText("<b><font color=\"red\">" + volumeus + " $</font></b>");
//        } else {
//    ui->volumeb_2->setText(volume[0]);
//    ui->volumeu_2->setText(volumeus + " $");
//    }

//    volumeu = basevolume[0].toDouble() * bitcoin2;
//    volumeus = QString::number(volumeu);

//    if(basevolume[0] > volumesp2)
//    {
//        ui->volumes_2->setText("<b><font color=\"green\">" + basevolume[0] + "</font></b>");
//        ui->volumeu_2->setText("<b><font color=\"green\">" + volumeus + " $</font></b>");
//    } else if (basevolume[0] < volumesp2) {
//        ui->volumes_2->setText("<b><font color=\"red\">" + basevolume[0] + "</font></b>");
//        ui->volumeu_2->setText("<b><font color=\"red\">" + volumeus + " $</font></b>");
//        } else {
//    ui->volumes_2->setText(basevolume[0]);
//    ui->volumeu_2->setText(volumeus + " $");
//    }

//    if(PrevDay[0].toDouble() > 0)
//    {
//        yestu = PrevDay[0].toDouble()*100;
//        yestus = QString::number(yestu);

//        ui->yest_2->setText("<b><font color=\"green\"> + " + yestus + " %</font></b>");


//    }else
//    {
//        yestu = PrevDay[0].toDouble()*100;
//        yestus = QString::number(yestu);
//        ui->yest_2->setText("<b><font color=\"red\">" + yestus + " %</font></b>");

//    }


//    lastp2 = last[0];
//    askp2 = ask[0];
//    bidp2 = bid[0];
//    volumebp2 = volume[0];
//    volumesp2 = basevolume[0];

//}

finished->deleteLater();
}


void PoolBrowser::setModel(ClientModel *model)
{
    this->model = model;
}

PoolBrowser::~PoolBrowser()
{
    delete ui;
}
