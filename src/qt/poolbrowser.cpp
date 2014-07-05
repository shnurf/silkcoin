#include "poolbrowser.h"
#include "ui_poolbrowser.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"
#include "clientmodel.h"
#include "bitcoinrpc.h"

#include <QDesktopServices>
#include <QString>
#include <sstream>
#include <string>

using namespace json_spirit;

//Coinbase API
const QString apiCoinbasePrice = "https://coinbase.com/api/v1/currencies/exchange_rates";

//Bittrex API
const QString apiBittrexMarketSummary = "http://bittrex.com/api/v1/public/getmarketsummaries";
const QString apiBittrexTrades = "http://bittrex.com/api/v1/public/getmarkethistory?market=BTC-SC&count=100";
const QString apiBittrexOrders = "http://bittrex.com/api/v1/public/getorderbook?market=BTC-SC&type=both&depth=50";

//Cryptsy API
const QString apiCryptsyTrades = "http://pubapi.cryptsy.com/api.php?method=singlemarketdata&marketid=225";
const QString apiCryptsyOrders = "http://pubapi.cryptsy.com/api.php?method=singleorderdata&marketid=225";

//Mintpal API
const QString apiMintpalStats = "https://api.mintpal.com/v1/market/stats/SC/BTC";
const QString apiMintpalTrades = "https://api.mintpal.com/v1/market/trades/SC/BTC";
const QString apiMintpalSell = "https://api.mintpal.com/v1/market/orders/SC/BTC/SELL";
const QString apiMintpalBuy = "https://api.mintpal.com/v1/market/orders/SC/BTC/BUY";

//Common Globals
int mode=1;
double _lastScUsd;
double _dBtcPriceCurrent;
QString _qsBtcPriceCurrent;
QString _btcPricePrev = "";
QString _lastBtcUsd;

//Bittrex Globals
QString _askLast_Bittrex = "";
QString _baseVolumeLast_Bittrex = "";
QString _bidLast_Bittrex = "";
QString _highLast_Bittrex = "";
QString _openingBuy_Bittrex = "";
QString _openingSell_Bittrex = "";
QString _priceClose_Bittrex = "";
QString _priceLastLast_Bittrex = "";
QString _priceLast_Bittrex = "";
QString _volumeLast_Bittrex = "";

//Cryptsy Globals
double _baseVolumeLast_Cryptsy;
QString _priceLastLast_Cryptsy = "";
QString _priceLast_Cryptsy = "";
QString _volumeLast_Cryptsy = "";

//Mintpal Globals
QString _askLast_Mintpal = "";
QString _baseVolumeLast_Mintpal = "";
QString _bidLast_Mintpal = "";
QString _highLast_Mintpal = "";
QString _lowLast_Bittrex = "";
QString _lowLast_Mintpal = "";
QString _priceClose_Mintpal = "";
QString _priceLastLast_Mintpal = "";
QString _priceLast_Mintpal = "";
QString _volumeLast_Mintpal = "";
QStringList _apiResponseSells_Mintpal;

PoolBrowser::PoolBrowser(QWidget *parent) : QWidget(parent), ui(new Ui::PoolBrowser)
{
    //One time primer to ensure Coinbase has enough
    //time to respond to avoid the need to refreshfee
    getRequest(apiCoinbasePrice);

    ui->setupUi(this);
    setFixedSize(400, 420);

    ui->customPlot->addGraph();
    ui->customPlot->setBackground(QBrush(QColor("#edf1f7")));

    ui->customPlot2->addGraph();
    ui->customPlot2->addGraph();
    ui->customPlot2->setBackground(QBrush(QColor("#edf1f7")));

    ui->customPlot_2->addGraph();
    ui->customPlot_2->setBackground(QBrush(QColor("#edf1f7")));

    ui->customPlot2_2->addGraph();
    ui->customPlot2_2->addGraph();
    ui->customPlot2_2->setBackground(QBrush(QColor("#edf1f7")));

    ui->customPlot_3->addGraph();
    ui->customPlot_3->setBackground(QBrush(QColor("#edf1f7")));

    ui->customPlot2_3->addGraph();
    ui->customPlot2_3->addGraph();
    ui->customPlot2_3->setBackground(QBrush(QColor("#edf1f7")));

    pollApis();

    QObject::connect(&m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseNetworkResponse(QNetworkReply*)));
    connect(ui->startButton, SIGNAL(pressed()), this, SLOT( pollApis()));
    connect(ui->egal, SIGNAL(pressed()), this, SLOT( egaldo()));
}

void PoolBrowser::egaldo()
{
    QString silkcoinQty = ui->egals->text();
    double totalUsd = _lastScUsd * silkcoinQty.toDouble();
    double totalBtc = _qsBtcPriceCurrent.toDouble() * silkcoinQty.toDouble();

    ui->egald->setText(QString::number(totalUsd, 'f', 2) + " $ / "+QString::number(totalBtc, 'f', 2)+" BTC");
}

void PoolBrowser::openBittrex()
{
    QDesktopServices::openUrl(QUrl("https://www.bittrex.com/Market/Index?MarketName=BTC-SC"));
}
void PoolBrowser::openPoloniex()
{
    QDesktopServices::openUrl(QUrl("https://poloniex.com/exchange/btc_sc"));
}

void PoolBrowser::processOverview()
{
    double yes = (_priceClose_Bittrex.toDouble() + _priceClose_Mintpal.toDouble()) / 2;
    double average2 = (_priceLast_Bittrex.toDouble() + _priceLast_Mintpal.toDouble() + _priceLast_Cryptsy.toDouble()) / 3;
    QString average3 = QString::number((_priceLast_Bittrex.toDouble() - average2) / (average2 / 100),'f',2);
    QString average4 = QString::number((_priceLast_Mintpal.toDouble() - average2) / (average2 / 100),'f',2);
    QString average5 = QString::number((_priceLast_Cryptsy.toDouble() - average2) / (average2 / 100),'f',2);

    if (average3.toDouble() > 0) {
        ui->diff1->setText("<font color=\"green\">+" + average3 + " %</font>");
    }
    else {
        ui->diff1->setText("<font color=\"red\">" + average3 + " %</font>");
    }

    if (average4.toDouble() > 0) {
        ui->diff2->setText("<font color=\"green\">+" + average4 + " %</font>");
    }
    else {
        ui->diff2->setText("<font color=\"red\">" + average4 + " %</font>");
    }

    if (average5.toDouble() > 0) {
        ui->diff3->setText("<font color=\"green\">+" + average5 + " %</font>");
    }
    else {
        ui->diff3->setText("<font color=\"red\">" + average5 + " %</font>");
    }

    if ((_priceClose_Bittrex.toDouble()+_priceClose_Mintpal.toDouble()) > 0) {
        ui->yest_3->setText("<font color=\"green\">+" + QString::number(yes, 'f', 2) + " %</font>");
    }
    else {
        ui->yest_3->setText("<font color=\"red\">" + QString::number(yes, 'f', 2) + " %</font>");
    }

    if (_priceLast_Bittrex > _priceLastLast_Bittrex) {
        ui->last_4->setText("<font color=\"green\">" + _priceLast_Bittrex + "</font>");
    }
    else if (_priceLast_Bittrex < _priceLast_Bittrex) {
        ui->last_4->setText("<font color=\"red\">" + _priceLast_Bittrex + "</font>");
    }
    else {
        ui->last_4->setText(_priceLast_Bittrex);
    }

    if (_priceLast_Mintpal > _priceLastLast_Mintpal) {
        ui->last_5->setText("<font color=\"green\">" + _priceLast_Mintpal + "</font>");
    }
    else if (_priceLast_Mintpal < _priceLastLast_Mintpal) {
        ui->last_5->setText("<font color=\"red\">" + _priceLast_Mintpal + "</font>");
    }
    else {
        ui->last_5->setText(_priceLast_Mintpal);
    }

    if (_priceLast_Cryptsy > _priceLastLast_Cryptsy) {
        ui->last_6->setText("<font color=\"green\">" + _priceLast_Cryptsy + "</font>");
    }
    else if (_priceLast_Cryptsy < _priceLastLast_Cryptsy) {
        ui->last_6->setText("<font color=\"red\">" + _priceLast_Cryptsy + "</font>");
    }
    else {
        ui->last_6->setText(_priceLast_Cryptsy);
    }

    _priceLastLast_Bittrex = _priceLast_Bittrex;
    _priceLastLast_Mintpal = _priceLast_Mintpal;
    _priceLastLast_Cryptsy = _priceLast_Cryptsy;
}

void PoolBrowser::pollApis()
{
    ui->Ok->setVisible(true);

    getRequest(apiCoinbasePrice);

    getRequest(apiBittrexMarketSummary);
    getRequest(apiMintpalTrades);
    getRequest(apiBittrexOrders);

    getRequest(apiCryptsyTrades);
    getRequest(apiCryptsyOrders);

    getRequest(apiBittrexTrades);
    getRequest(apiMintpalStats);
    getRequest(apiMintpalBuy);
    getRequest(apiMintpalSell);
}

void PoolBrowser::getRequest( const QString &urlString )
{
    QUrl url ( urlString );
    QNetworkRequest req ( url );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    m_nam.get(req);
}

void PoolBrowser::parseNetworkResponse(QNetworkReply *response)
{
    QUrl apiCall = response->url();

    if ( response->error() != QNetworkReply::NoError ) {
        // A communication error has occurred
        emit networkError( response->error() );
        return;
    }

    if (apiCall == apiCoinbasePrice) { coinbasePrice(response); }
    else if (apiCall == apiBittrexMarketSummary) { bittrexMarketSummary(response); }
    else if (apiCall == apiBittrexTrades) { bittrexTrades(response); }
    else if (apiCall == apiBittrexOrders) { bittrexOrders(response); }
    else if (apiCall == apiCryptsyTrades) { cryptsyTrades(response); }
    else if (apiCall == apiCryptsyOrders) { cryptsyOrders(response); }
    else if (apiCall == apiMintpalStats) { mintpalStats(response); }
    else if (apiCall == apiMintpalTrades) { mintpalTrades(response); }
    else if (apiCall == apiMintpalSell) { mintpalSell(response); }
    else if (apiCall == apiMintpalBuy) { mintpalBuy(response); }
    else { }  //Sould NEVER get here unless something went completely awry

    if (_priceLast_Bittrex != 0 && _priceLast_Mintpal != 0 && _priceLast_Cryptsy != 0) {
        ui->Ok->setVisible(false);
    }

    processOverview();

    response->deleteLater();
}

/*************************************************************************************
 * Method: PoolBrowser::bittrexMarketSummary
 * Parameter(s): QNetworkReply *response
 *
 * Unauthenticated resource that returns BTC to fiat (and vice versus) exchange rates in various currencies.
 * It has keys for both btc_to_xxx and xxx_to_btc so you can convert either way.
 * The key always contains downcase representations of the currency ISO.
 * Note that some small numbers may use E notation such as 2.8e-05.
 *
 * Response: {"btc_to_pgk":"28.152994","btc_to_gyd":"2743.906541","btc_to_mmk":"11611.550858", ... ,"brl_to_btc":"0.037652"}
 *************************************************************************************/
void PoolBrowser::coinbasePrice(QNetworkReply *response)
{
    QString apiResponse = response->readAll();
    QStringList btcPrices = apiResponse.split(",\"btc_to_usd\":", QString::KeepEmptyParts);

    QString btcPrice = btcPrices[1].split(",", QString::KeepEmptyParts)[0].replace("\"", "");
    _dBtcPriceCurrent = btcPrice.toDouble();
    btcPrice = QString::number(_dBtcPriceCurrent, 'f', 2);

    if (btcPrice > _btcPricePrev) {
        ui->bitcoin->setText("<font color=\"green\">" + btcPrice + " $</font>");
    }
    else if (btcPrice < _btcPricePrev) {
        ui->bitcoin->setText("<font color=\"red\">" + btcPrice + " $</font>");
    }
    else {
        ui->bitcoin->setText(btcPrice + " $");
    }

    _btcPricePrev = btcPrice;
}

/*************************************************************************************
 * Method: PoolBrowser::bittrexMarketSummary
 * Parameter(s): QNetworkReply *response
 *
 * Used to get the last 24 hour summary of all active exchanges
 *
 * Parameter(s): None
 * Response:
 * {
 * 	"success" : true,
 * 	"message" : "",
 * 	"result" : [{
 * 	        "MarketName" : "BTC-LTC",
 * 	        "High" : 0.02590000,
 * 	        "Low" : 0.02400000,
 * 	        "Volume" : 114.84340665,
 * 	        "Last" : 0.02480000,
 * 	        "BaseVolume" : 2.85028800,
 * 	        "TimeStamp" : "2014-04-19T20:49:23.483"
 *         }, {
 * 	        "MarketName" : "BTC-WC",
 * 	        "High" : 0.00002456,
 * 	        "Low" : 0.00001352,
 * 	        "Volume" : 4574426.27271220,
 * 	        "Last" : 0.00002006,
 * 	        "BaseVolume" : 82.96629666,
 * 	        "TimeStamp" : "2014-04-19T20:49:50.053"
 *         }
 * 	]
 * }
 *************************************************************************************/
void PoolBrowser::bittrexMarketSummary(QNetworkReply *response)
{
    double dAsk, dLast, dBid, dVolume, dClose;
    QString qsAsk, qsLast, qsBid, qsVolume, qsClose;

    QString apiResponse = response->readAll();
    QStringList qslApiResponse = apiResponse.split("{\"MarketName\":\"BTC-SC\",\"High\":", QString::KeepEmptyParts);

    QStringList qslHigh = qslApiResponse[1].split(",\"Low\":", QString::KeepEmptyParts);
    QStringList qslLow = qslHigh[1].split(",\"Volume\":", QString::KeepEmptyParts);
    QStringList qslVolume = qslLow[1].split(",\"Last\":", QString::KeepEmptyParts);
    QStringList qslLast = qslVolume[1].split(",\"BaseVolume\":", QString::KeepEmptyParts);
    QStringList qslBaseVolume = qslLast[1].split(",\"TimeStamp\":\"", QString::KeepEmptyParts);
    QStringList qslTme = qslBaseVolume[1].split("\",\"Bid\":", QString::KeepEmptyParts);
    QStringList qslBid = qslTme[1].split(",\"Ask\":", QString::KeepEmptyParts);
    QStringList qslAsk = qslBid[1].split(",\"OpenBuyOrders\":", QString::KeepEmptyParts);
    QStringList qslOpeningBuy = qslAsk[1].split(",\"OpenSellOrders\":", QString::KeepEmptyParts);
    QStringList qslOpeningSell = qslOpeningBuy[1].split(",\"PrevDay\":", QString::KeepEmptyParts);
    QStringList qslClose = qslOpeningSell[1].split(",\"Created\":", QString::KeepEmptyParts);

    dLast = qslLast[0].toDouble() * _dBtcPriceCurrent;
    _lastScUsd = dLast;
    qsLast = QString::number(dLast, 'f', 8);
    _lastBtcUsd = qsLast;
    _qsBtcPriceCurrent = qslLast[0];

    if (qslLast[0] > _priceLast_Bittrex) {
        ui->last->setText("<font color=\"green\">" + qslLast[0] + "</font>");
        ui->lastu->setText("<font color=\"green\">" + qsLast + " $</font>");
    }
    else if (qslLast[0] < _priceLast_Bittrex) {
        ui->last->setText("<font color=\"red\">" + qslLast[0] + "</font>");
        ui->lastu->setText("<font color=\"red\">" + qsLast + " $</font>");
    }
    else {
        ui->last->setText(qslLast[0]);
        ui->lastu->setText(qsLast + " $");
    }

    dAsk = qslAsk[0].toDouble() * _dBtcPriceCurrent;
    qsAsk = QString::number(dAsk, 'f', 8);

    if (qslAsk[0] > _askLast_Bittrex) {
        ui->ask->setText("<font color=\"green\">" + qslAsk[0] + "</font>");
        ui->asku->setText("<font color=\"green\">" + qsAsk + " $</font>");
    }
    else if (qslAsk[0] < _askLast_Bittrex) {
        ui->ask->setText("<font color=\"red\">" + qslAsk[0] + "</font>");
        ui->asku->setText("<font color=\"red\">" + qsAsk + " $</font>");
    }
    else {
        ui->ask->setText(qslAsk[0]);
        ui->asku->setText(qsAsk + " $");
    }

    dBid = qslBid[0].toDouble() * _dBtcPriceCurrent;
    qsBid = QString::number(dBid, 'f', 8);

    if (qslBid[0] > _bidLast_Bittrex) {
        ui->bid->setText("<font color=\"green\">" + qslBid[0] + "</font>");
        ui->bidu->setText("<font color=\"green\">" + qsBid + " $</font>");
    }
    else if (qslBid[0] < _bidLast_Bittrex) {
        ui->bid->setText("<font color=\"red\">" + qslBid[0] + "</font>");
        ui->bidu->setText("<font color=\"red\">" + qsBid + " $</font>");
    }
    else {
        ui->bid->setText(qslBid[0]);
        ui->bidu->setText(qsBid + " $");
    }

    if (qslHigh[0] > _highLast_Bittrex) {
        ui->high->setText("<font color=\"green\">" + qslHigh[0] + "</font>");
    }
    else if (qslHigh[0] < _highLast_Bittrex) {
        ui->high->setText("<font color=\"red\">" + qslHigh[0] + "</font>");
    }
    else {
        ui->high->setText(qslHigh[0]);
    }

    if (qslLow[0] > _lowLast_Bittrex) {
        ui->low->setText("<font color=\"green\">" + qslLow[0] + "</font>");
    }
    else if (qslLow[0] < _lowLast_Bittrex) {
        ui->low->setText("<font color=\"red\">" + qslLow[0] + "</font>");
    }
    else {
        ui->low->setText(qslLow[0]);
    }

    if (qslVolume[0] > _volumeLast_Bittrex) {
        ui->volumeb->setText("<font color=\"green\">" + qslVolume[0] + "</font>");
    }
    else if (qslVolume[0] < _volumeLast_Bittrex) {
        ui->volumeb->setText("<font color=\"red\">" + qslVolume[0] + "</font>");
        ui->volumeu->setText("<font color=\"red\">" + qsVolume + " $</font>");
    }
    else {
        ui->volumeb->setText(qslVolume[0]);
        ui->volumeu->setText(qsVolume + " $");
    }

    dVolume = qslBaseVolume[0].toDouble() * _dBtcPriceCurrent;
    qsVolume = QString::number(dVolume, 'f', 2);

    if (qslBaseVolume[0] > _baseVolumeLast_Bittrex) {
        ui->volumes->setText("<font color=\"green\">" + qslBaseVolume[0] + "</font>");
        ui->volumeu->setText("<font color=\"green\">" + qsVolume + " $</font>");
    }
    else if (qslBaseVolume[0] < _baseVolumeLast_Bittrex) {
        ui->volumes->setText("<font color=\"red\">" + qslBaseVolume[0] + "</font>");
        ui->volumeu->setText("<font color=\"red\">" + qsVolume + " $</font>");
    }
    else {
        ui->volumes->setText(qslBaseVolume[0]);
        ui->volumeu->setText(qsVolume + " $");
    }

    if (qslLast[0].toDouble() > qslClose[0].toDouble()) {
        dClose = ((qslLast[0].toDouble() - qslClose[0].toDouble())/qslLast[0].toDouble()) * 100;
        qsClose = QString::number(dClose, 'f', 2);

        ui->yest->setText("<font color=\"green\"> + " + qsClose + " %</font>");
    }
    else {
        dClose = ((qslClose[0].toDouble() - qslLast[0].toDouble())/qslClose[0].toDouble()) * 100;
        qsClose = QString::number(dClose, 'f', 2);
        ui->yest->setText("<font color=\"red\"> - " + qsClose + " %</font>");
    }

    _priceLast_Bittrex = qslLast[0];
    _askLast_Bittrex = qslAsk[0];
    _bidLast_Bittrex = qslBid[0];
    _highLast_Bittrex = qslHigh[0];
    _lowLast_Bittrex = qslLow[0];
    _volumeLast_Bittrex = qslVolume[0];
    _baseVolumeLast_Bittrex = qslBaseVolume[0];
    _openingBuy_Bittrex = qslOpeningBuy[0];
    _openingSell_Bittrex = qslOpeningSell[0];

    _priceClose_Bittrex = qslLast[0].toDouble() > qslClose[0].toDouble()
            ? qsClose : qslLast[0].toDouble() < qslClose[0].toDouble()
            ? qsClose.prepend("-") : qsClose;

    //Update conversion price automatically
    ui->egal->click();
}

/*************************************************************************************
 * Method: PoolBrowser::bittrexTrades
 * Parameter(s): QNetworkReply *response
 *
 * Used to retrieve the latest trades that have occurred for a specific market
 * Parameter(s):
 * market (required): a string literal for the market (ex: BTC-LTC)
 * count (optional): a number between 1-100 for the number of entries to return (default = 20)
 *
 *     {
 * 	"success" : true,
 * 	"message" : "",
 * 	"result" : [{
 * 			"OrderId" : "12323",
 * 			"TimeStamp" : "2014-02-25T07:40:08.68",
 * 			"Quantity" : 185.06100000,
 * 			"Price" : 0.00000174,
 * 			"Total" : 0.00032200
 * 		}, {
 * 			"OrderUuid" : "12322",
 * 			"TimeStamp" : "2014-02-25T07:39:18.603",
 * 			"Quantity" : 10.74500000,
 * 			"Price" : 0.00000172,
 * 			"Total" : 0.00001848
 * 		}, {
 * 			"OrderUuid" : "12321",
 * 			"TimeStamp" : "2014-02-25T07:39:18.6",
 * 			"Quantity" : 5.62100000,
 * 			"Price" : 0.00000172,
 * 			"Total" : 0.00000966
 * 		}, {
 * 			"OrderUuid" : "12319",
 * 			"TimeStamp" : "2014-02-25T07:39:18.6",
 * 			"Quantity" : 76.23000000,
 * 			"Price" : 0.00000173,
 * 			"Total" : 0.00013187
 * 		}, {
 * 			"OrderUuid" : "12317",
 * 			"TimeStamp" : "2014-02-25T07:39:18.6",
 * 			"Quantity" : 52.47500000,
 * 			"Price" : 0.00000174,
 * 			"Total" : 0.00009130
 * 		}
 * 	]
 * }
 *************************************************************************************/
void PoolBrowser::bittrexTrades(QNetworkReply *response)
{
    // QNetworkReply is a QIODevice. So we read from it just like it was a file
    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("{\"success\":true,\"message\":\"\",\"result\":[", "");
    apiResponse = apiResponse.replace(",\"FillType\":\"FILL\"", "");
    apiResponse = apiResponse.replace(",\"FillType\":\"PARTIAL_FILL\"", "");
    apiResponse = apiResponse.replace("\"", "");
    apiResponse = apiResponse.replace("Id:", "");
    apiResponse = apiResponse.replace("TimeStamp:", "");
    apiResponse = apiResponse.replace("Quantity:", "");
    apiResponse = apiResponse.replace("Price:", "");
    apiResponse = apiResponse.replace("Total:", "");
    apiResponse = apiResponse.replace("OrderType:", "");

    QStringList qslTrades = apiResponse.split("},{", QString::KeepEmptyParts);

    //Prevent overflow by limiting trade data to no more than 100
    //Bittrex API states this can go as high as 100, but it never returns more than 50
    //Setting it to 100 to match other markets in case Bittrex changes the API
    int tradeCount = qslTrades.length() > 100
            ? 100 : qslTrades.length();

    int z = 0;

    ui->BittrexTradesTable->clear();
    ui->BittrexTradesTable->setColumnWidth(0, 110);
    ui->BittrexTradesTable->setColumnWidth(1, 110);
    ui->BittrexTradesTable->setColumnWidth(2, 110);
    ui->BittrexTradesTable->setColumnWidth(3, 110);
    ui->BittrexTradesTable->setColumnWidth(4, 110);
    ui->BittrexTradesTable->setSortingEnabled(false);

    QVector<double> xAxis(tradeCount), yAxis(tradeCount);

    double high = 0;
    double low = 100000;

    for (int i = 0; i < tradeCount; i++) {
        qslTrades[i].replace("{", "").replace("}", "");
        QStringList qslTrade = qslTrades[i].split(",", QString::KeepEmptyParts);
        QTreeWidgetItem * item = new QTreeWidgetItem();

        item->setText(0, qslTrade[5] == "BUY" ? "Buy" : qslTrade[5] ==  "SELL" ? "Sell" : "Unknown");
        item->setText(1, qslTrade[3]);
        item->setText(2, qslTrade[2]);
        item->setText(3, qslTrade[4]);

        QString time = qslTrade[1].split("T", QString::KeepEmptyParts)[1];
        time.truncate(time.indexOf(".", 0));
        item->setText(4, time);

        ui->BittrexTradesTable->addTopLevelItem(item);

        xAxis[z] = (tradeCount) - z;
        yAxis[z] = (qslTrade[3].toDouble()) * 100000000;

        if (qslTrade[3].toDouble() * 100000000 > high) {
            high = qslTrade[3].toDouble() * 100000000;
        }

        if (qslTrade[3].toDouble() * 100000000 < low) {
            low = qslTrade[3].toDouble() * 100000000;
        }

        z++;
    }

    // create graph and assign data to it:
    ui->customPlot->graph(0)->setData(xAxis, yAxis);
    ui->customPlot->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->customPlot->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(1, tradeCount);
    ui->customPlot->yAxis->setRange(low, high);

    ui->customPlot->replot();
}
void PoolBrowser::bittrexOrders(QNetworkReply *response)
{
    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("{", "");
    apiResponse = apiResponse.replace("}", "");
    apiResponse = apiResponse.replace("\"", "");
    apiResponse = apiResponse.replace("],\"sell\":", "");
    apiResponse = apiResponse.replace(" ", "");
    apiResponse = apiResponse.replace("]", "");
    apiResponse = apiResponse.replace("Quantity:", "");
    apiResponse = apiResponse.replace("Rate:", "");

    QStringList apiResponseOrders = apiResponse.split("[");
    QStringList apiResponseBuys = apiResponseOrders[1].split(",");
    QStringList apiResponseSells = apiResponseOrders[2].split(",");

    //Use shortest depth as limit and use buy length if they are the same
    int depth = apiResponseBuys.length() > apiResponseSells.length()
            ? apiResponseSells.length() : apiResponseSells.length() > apiResponseBuys.length()
            ? apiResponseBuys.length() : apiResponseBuys.length();

    //Prevent overflow by limiting depth to 100
    //Also check for odd number of trades and drop the last one
    //To avoid an overflow when there are less than 100 trades
    depth = depth > 100
            ? 100 : depth % 2 == 1
            ? depth -1 : depth;

    //Nothing to process...
    if (depth == 0){ return; }

    int z = 0;
    double high = 0;
    double low = 100000;
    double sumBuys = 0;
    double sumSells = 0;
    double sumHighest = 0;

    QVector<double> xAxisBuys(depth), yAxisBuys(depth);
    QVector<double> xAxisSells(depth), yAxisSells(depth);

    ui->sellquan->clear();
    ui->buyquan->clear();
    ui->sellquan->sortByColumn(0, Qt::AscendingOrder);
    ui->sellquan->setSortingEnabled(true);
    ui->buyquan->setSortingEnabled(true);

    for (int i = 0; i < depth; i++) {
        QTreeWidgetItem * item = new QTreeWidgetItem();

        item->setText(0,apiResponseBuys[i + 1]);
        item->setText(1,apiResponseBuys[i]);

        ui->buyquan->addTopLevelItem(item);

        QTreeWidgetItem * item2 = new QTreeWidgetItem();

        item2->setText(0,apiResponseSells[i + 1]);
        item2->setText(1,apiResponseSells[i]);

        ui->sellquan->addTopLevelItem(item2);

        if (apiResponseSells[i + 1].toDouble() * 100000000 > high) {
            high = apiResponseSells[i + 1].toDouble() * 100000000;
        }

        if (apiResponseBuys[i + 1].toDouble() * 100000000 < low) {
            low = apiResponseBuys[i + 1].toDouble() * 100000000;
        }

        xAxisBuys[z] = apiResponseBuys[i + 1].toDouble() * 100000000;
        yAxisBuys[z] = sumBuys;

        xAxisSells[z] = apiResponseSells[i + 1].toDouble() * 100000000;
        yAxisSells[z] = sumSells;

        sumBuys = apiResponseBuys[i].toDouble() + sumBuys;
        sumSells = apiResponseSells[i].toDouble() + sumSells;

        i++;
        z++;
    }

    sumHighest = sumBuys > sumSells ? sumBuys : sumBuys < sumSells ? sumSells : sumBuys;

    // create graph and assign data to it:
    ui->customPlot2->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->customPlot2->graph(1)->setData(xAxisSells, yAxisSells);

    ui->customPlot2->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->customPlot2->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    ui->customPlot2->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->customPlot2->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    // set axes ranges, so we see all data:
    ui->customPlot2->xAxis->setRange(low, high);
    ui->customPlot2->yAxis->setRange(low, sumHighest);

    ui->customPlot2->replot();
}

/*************************************************************************************
 * Method: PoolBrowser::cryptsyTrades
 * Parameter(s): QNetworkReply *response
 *
 * General Market Data (Single Market - Realtime):
 *************************************************************************************/
void PoolBrowser::cryptsyTrades(QNetworkReply *response)
{
    double dLast, dVolume;
    QString qsLast, qsVolume;

    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("{\"success\":1,\"return\":{\"markets\":{\"SC\":{\"marketid\":\"225\",\"label\":\"SC\\/BTC\",", "");
    apiResponse = apiResponse.replace(",\"primaryname\":\"SilkCoin\",\"primarycode\":\"SC\",\"secondaryname\":\"BitCoin\",\"secondarycode\":\"BTC\"", "");
    apiResponse = apiResponse.replace("lasttradetime\":", "LastTradeTime:");
    apiResponse = apiResponse.replace("lasttradeprice\":", "LastTradePrice:");
    apiResponse = apiResponse.replace("\"", "");
    apiResponse = apiResponse.replace("id:", "");
    apiResponse = apiResponse.replace("time:", "");
    apiResponse = apiResponse.replace("type:", "");
    apiResponse = apiResponse.replace("price:", "");
    apiResponse = apiResponse.replace("quantity:", "");
    apiResponse = apiResponse.replace("total:", "");

    QStringList qslApiResponse = apiResponse.split(",recenttrades:[{", QString::KeepEmptyParts);
    QStringList qslTemp = qslApiResponse[1].split("}],sellorders:[{", QString::KeepEmptyParts);

    QStringList qslTrades = qslTemp[0].split("},{", QString::KeepEmptyParts);
    qslTrades += qslTemp[1].split("},{", QString::KeepEmptyParts);

    //-----------------------------------------------------------------

    int z = 0;

    ui->CryptsyTradesTable->clear();
    ui->CryptsyTradesTable->setColumnWidth(0, 110);
    ui->CryptsyTradesTable->setColumnWidth(1, 110);
    ui->CryptsyTradesTable->setColumnWidth(2, 110);
    ui->CryptsyTradesTable->setColumnWidth(3, 110);
    ui->CryptsyTradesTable->setColumnWidth(4, 110);
    ui->CryptsyTradesTable->setSortingEnabled(false);

    double high = 0;
    double low = 100000;

    //Prevent overflow by limiting trade data to no more than 100
    int tradeCount = qslTrades.length() > 100
            ? 100 : qslTrades.length();

    QVector<double> xAxis(tradeCount), yAxis(tradeCount);

    for (int i = 0; i < tradeCount; i++) {
        qslTrades[i].replace("{", "").replace("}", "");
        QStringList qslTrade = qslTrades[i].split(",", QString::KeepEmptyParts);

        QTreeWidgetItem * qtTrades = new QTreeWidgetItem();

        qtTrades->setText(0, qslTrade[2]);
        qtTrades->setText(1, qslTrade[3]);
        qtTrades->setText(2, qslTrade[4]);
        qtTrades->setText(3, qslTrade[5]);

        QStringList temp = qslTrade[1].split(" ", QString::KeepEmptyParts);
        qtTrades->setText(4, temp[1]);

        ui->CryptsyTradesTable->addTopLevelItem(qtTrades);

        xAxis[z] = (tradeCount) - z;
        yAxis[z] = (qslTrade[3].toDouble()) * 100000000;

        if (qslTrade[3].toDouble() * 100000000 > high) {
            high = qslTrade[3].toDouble() * 100000000;
        }

        if (qslTrade[3].toDouble() * 100000000 < low) {
            low = qslTrade[3].toDouble() * 100000000;
        }

        z++;
    }

    // create graph and assign data to it:
    ui->customPlot_3->graph(0)->setData(xAxis, yAxis);
    ui->customPlot_3->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->customPlot_3->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

    // set axes ranges, so we see all data:
    ui->customPlot_3->xAxis->setRange(1, tradeCount);
    ui->customPlot_3->yAxis->setRange(low, high);

    ui->customPlot_3->replot();

    QStringList qslSplit = apiResponse.split("LastTradePrice:", QString::KeepEmptyParts);
    QStringList qslLast = qslSplit[1].split(",volume:", QString::KeepEmptyParts);
    QStringList qslVolume = qslLast[1].split(",LastTradeTime:", QString::KeepEmptyParts);

    dLast = qslLast[0].toDouble() * _dBtcPriceCurrent;
    qsLast = QString::number(dLast, 'f', 8);

    if (qslLast[0] > _priceLast_Cryptsy) {
        ui->last_3->setText("<font color=\"green\">" + qslLast[0] + "</font>");
        ui->lastu_3->setText("<font color=\"green\">" + qsLast + " $</font>");
    }
    else if (qslLast[0] < _priceLast_Cryptsy) {
        ui->last_3->setText("<font color=\"red\">" + qslLast[0] + "</font>");
        ui->lastu_3->setText("<font color=\"red\">" + qsLast + " $</font>");
    }
    else {
        ui->last_3->setText(qslLast[0]);
        ui->lastu_3->setText(qsLast + " $");
    }

    double baseVolume = qslVolume[0].toDouble() * qslLast[0].toDouble();

    dVolume = baseVolume * _dBtcPriceCurrent;
    qsVolume = QString::number(dVolume, 'f', 4);

    if (qslVolume[0] > _volumeLast_Cryptsy) {
        ui->volumeb_3->setText("<font color=\"green\">" + qslVolume[0] + "</font>");
    }
    else if (qslVolume[0] < _volumeLast_Cryptsy) {
        ui->volumeb_3->setText("<font color=\"red\">" + qslVolume[0] + "</font>");
        ui->volumeu_3->setText("<font color=\"red\">" + qsVolume + " $</font>");
    }
    else {
        ui->volumeb_3->setText(qslVolume[0]);
        ui->volumeu_3->setText(qsVolume + " $");
    }

    // -----------------------------------------------------------------------------------------

    if (baseVolume > _baseVolumeLast_Cryptsy) {
        ui->volumes_3->setText("<font color=\"green\">" + QString::number(baseVolume, 'f', 2) + "</font>");
        ui->volumeu_3->setText("<font color=\"green\">" + qsVolume + " $</font>");
    }
    else if (baseVolume < _baseVolumeLast_Cryptsy) {
        ui->volumes_3->setText("<font color=\"red\">" + QString::number(baseVolume, 'f', 2) + "</font>");
        ui->volumeu_3->setText("<font color=\"red\">" + qsVolume + " $</font>");
    }
    else {
        ui->volumes_3->setText(QString::number(baseVolume, 'f', 2));
        ui->volumeu_3->setText(qsVolume + " $");
    }

    _priceLast_Cryptsy = qslLast[0];
    _volumeLast_Cryptsy = qslVolume[0];
    _baseVolumeLast_Cryptsy = baseVolume;
}
/*************************************************************************************
 * Method: PoolBrowser::cryptsyOrders
 * Parameter(s): QNetworkReply *response
 *
 * General Orderbook Data (Single Market - Realtime):
 *************************************************************************************/
void PoolBrowser::cryptsyOrders(QNetworkReply *response)
{
    QString apiResponse = response->readAll();
    apiResponse = apiResponse.replace("\"", "");
    QStringList qslApiResponse = apiResponse.split("[", QString::KeepEmptyParts);
    QStringList apiResponseSells = qslApiResponse[1].replace("price:", "").replace("quantity:", "").replace("total:", "").replace("}", "").replace("{", "").replace("}],buyorders:", "").replace("]", "").split(",", QString::KeepEmptyParts);
    QStringList apiResponseBuys = qslApiResponse[2].replace("price:", "").replace("quantity:", "").replace("total:", "").replace("}", "").replace("{", "").replace("}],buyorders:", "").replace("]", "").split(",", QString::KeepEmptyParts);

    //Use shorest depth as limit and use buy length if they are the same
    int depth = apiResponseBuys.length() > apiResponseSells.length()
            ? apiResponseSells.length() : apiResponseSells.length() > apiResponseBuys.length()
            ? apiResponseBuys.length() : apiResponseBuys.length();

    //Prevent overflow by limiting depth to 150
    //Also check for odd number of trades and drop the last one
    //To avoid an overflow when there are less than 150 trades
    depth = depth > 150
            ? 150 : depth % 2 == 1
            ? depth -1 : depth;


    //Nothing to process...
    if (depth == 0){ return; }

    int z = 0;
    double high = 0;
    double low = 100000;
    double sumHighs = 0;
    double sumLows = 0;
    double sumHighest = 0;

    //Shave off space for the non-used data fields
    int adjustedDepth = depth / 3;

    QVector<double> xAxisBuys(adjustedDepth), yAxisBuys(adjustedDepth);
    QVector<double> xAxisSells(adjustedDepth), yAxisSells(adjustedDepth);

    ui->sellquan_3->clear();
    ui->buyquan_3->clear();
    ui->sellquan_3->sortByColumn(0, Qt::AscendingOrder);
    ui->sellquan_3->setSortingEnabled(true);
    ui->buyquan_3->setSortingEnabled(true);

    for (int i = 0; i < depth; i++) {
        QTreeWidgetItem * qtBuys = new QTreeWidgetItem();

        qtBuys->setText(0, apiResponseBuys[i]);
        qtBuys->setText(1, apiResponseBuys[i + 1]);

        ui->buyquan_3->addTopLevelItem(qtBuys);

        QTreeWidgetItem * qtSells = new QTreeWidgetItem();

        qtSells->setText(0, apiResponseSells[i]);
        qtSells->setText(1, apiResponseSells[i + 1]);

        ui->sellquan_3->addTopLevelItem(qtSells);

        if (apiResponseSells[i].toDouble() * 100000000 > high) {
            high = apiResponseSells[i].toDouble() * 100000000;
        }

        if (apiResponseBuys[i].toDouble() * 100000000 < low) {
            low = apiResponseBuys[i].toDouble() * 100000000;
        }

        sumHighs = apiResponseBuys[i + 1].toDouble() + sumHighs;
        sumLows = apiResponseSells[i + 1].toDouble() + sumLows;

        xAxisBuys[z] = apiResponseBuys[i].toDouble() * 100000000;
        yAxisBuys[z] = sumHighs;

        xAxisSells[z] = apiResponseSells[i].toDouble() * 100000000;
        yAxisSells[z] = sumLows;

        i += 2;
        z++;
    }

    sumHighest = sumHighs > sumLows ? sumHighs : sumHighs < sumLows ? sumLows : sumHighs;

    // create graph and assign data to it:
    ui->customPlot2_3->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->customPlot2_3->graph(1)->setData(xAxisSells, yAxisSells);
    ui->customPlot2_3->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->customPlot2_3->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    ui->customPlot2_3->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->customPlot2_3->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    // set axes ranges, so we see all data:
    ui->customPlot2_3->xAxis->setRange(low, high);
    ui->customPlot2_3->yAxis->setRange(low, sumHighest);

    ui->customPlot2_3->replot();
}

/*************************************************************************************
 * Method: PoolBrowser::mintpalStats
 * Parameter(s): QNetworkReply *response
 *
 * Provides the statistics for a single market. Data refreshes every minute.
 *
 * Response:
 * [{
 *   "market_id":"25",
 *   "code":"AUR",
 *   "exchange":"BTC",
 *   "last_price":"0.04600001",
 *   "yesterday_price":"0.04300000",
 *   "change":"+6.98",
 *   "24hhigh":"0.04980000",
 *   "24hlow":"0.04000050",
 *   "24hvol":"21.737"
 *   "top_bid":"0.04590000"
 *   "top_ask":"0.04600005"
 * }]
 *************************************************************************************/
void PoolBrowser::mintpalStats(QNetworkReply *response)
{
    double dAsk, dLast, dBid, dVolume, dClose;
    QString qsAsk, qsLast, qsBid, qsVolume, qsClose;

    QString apiResponse = response->readAll();
    apiResponse = apiResponse.replace("[{", "").replace("}]", "").replace("\"", "");

    QStringList qslApiResponse = apiResponse.split("market_id:163,coin:SilkCoin,code:SC,exchange:BTC,last_price:", QString::KeepEmptyParts);
    QStringList qslLast = qslApiResponse[1].split(",yesterday_price:", QString::KeepEmptyParts);
    QStringList qslCloseChange = qslLast[1].split(",change:", QString::KeepEmptyParts);
    QStringList qslClose = qslCloseChange[1].split(",24hhigh:", QString::KeepEmptyParts);
    QStringList qslHigh = qslClose[1].split(",24hlow:", QString::KeepEmptyParts);
    QStringList qslLow = qslHigh[1].split(",24hvol:", QString::KeepEmptyParts);
    QStringList qslVolume = qslLow[1].split(",top_bid:", QString::KeepEmptyParts);
    QStringList qslBidHigh = qslVolume[1].split(",top_ask:", QString::KeepEmptyParts);
    QString qsAskHigh = qslBidHigh[1];

    dLast = qslLast[0].toDouble() * _dBtcPriceCurrent;
    qsLast = QString::number(dLast, 'f', 8);

    if (qslLast[0] > _priceLast_Mintpal) {
        ui->last_2->setText("<font color=\"green\">" + qslLast[0] + "</font>");
        ui->lastu_2->setText("<font color=\"green\">" + qsLast + " $</font>");
    }
    else if (qslLast[0] < _priceLast_Mintpal) {
        ui->last_2->setText("<font color=\"red\">" + qslLast[0] + "</font>");
        ui->lastu_2->setText("<font color=\"red\">" + qsLast + " $</font>");
    }
    else {
        ui->last_2->setText(qslLast[0]);
        ui->lastu_2->setText(qsLast + " $");
    }

    dAsk = qsAskHigh.toDouble() * _dBtcPriceCurrent;
    qsAsk = QString::number(dAsk, 'f', 8);

    if (qsAsk > _askLast_Mintpal) {
        ui->ask_2->setText("<font color=\"green\">" + qsAsk + "</font>");
        ui->asku_2->setText("<font color=\"green\">" + qsAsk + " $</font>");
    }
    else if (qsAsk < _askLast_Mintpal) {
        ui->ask_2->setText("<font color=\"red\">" + qsAsk + "</font>");
        ui->asku_2->setText("<font color=\"red\">" + qsAsk + " $</font>");
    }
    else {
        ui->ask_2->setText(qsAsk);
        ui->asku_2->setText(qsAsk + " $");
    }

    dBid = qslBidHigh[0].toDouble() * _dBtcPriceCurrent;
    qsBid = QString::number(dBid, 'f', 8);

    if (qslBidHigh[0] > _bidLast_Mintpal) {
        ui->bid_2->setText("<font color=\"green\">" + qslBidHigh[0] + "</font>");
        ui->bidu_2->setText("<font color=\"green\">" + qsBid + " $</font>");
    }
    else if (qslBidHigh[0] < _bidLast_Mintpal) {
        ui->bid_2->setText("<font color=\"red\">" + qslBidHigh[0] + "</font>");
        ui->bidu_2->setText("<font color=\"red\">" + qsBid + " $</font>");
    }
    else {
        ui->bid_2->setText(qslBidHigh[0]);
        ui->bidu_2->setText(qsBid + " $");
    }

    if (qslHigh[0] > _highLast_Mintpal) {
        ui->high_2->setText("<font color=\"green\">" + qslHigh[0] + "</font>");
    }
    else if (qslHigh[0] < _highLast_Mintpal) {
        ui->high_2->setText("<font color=\"red\">" + qslHigh[0] + "</font>");
    }
    else {
        ui->high_2->setText(qslHigh[0]);
    }

    if (qslLow[0] > _lowLast_Mintpal) {
        ui->low_2->setText("<font color=\"green\">" + qslLow[0] + "</font>");
    }
    else if (qslLow[0] < _lowLast_Mintpal) {
        ui->low_2->setText("<font color=\"red\">" + qslLow[0] + "</font>");
    }
    else {
        ui->low_2->setText(qslLow[0]);
    }

    dVolume = qslVolume[0].toDouble() * _dBtcPriceCurrent;
    qsVolume = QString::number(dVolume, 'f', 2);

    if (qslVolume[0] > _volumeLast_Mintpal) {
        ui->volumeb_2->setText("<font color=\"green\">" + qslVolume[0] + "</font>");

    }
    else if (qslVolume[0] < _volumeLast_Mintpal) {
        ui->volumeb_2->setText("<font color=\"red\">" + qslVolume[0] + "</font>");
        ui->volumeu_2->setText("<font color=\"red\">" + qsVolume + " $</font>");
    }
    else {
        ui->volumeb_2->setText(qslVolume[0]);
        ui->volumeu_2->setText(qsVolume + " $");
    }

    // -----------------------------------------------------------------------------------------
    quint64 basee = qslVolume[0].toDouble() / qslLast[0].toDouble();
    QString basevolume = QString::number(basee, 'f', 2);

    if (basevolume > _baseVolumeLast_Mintpal) {
        ui->volumes_2->setText("<font color=\"green\">" + basevolume + "</font>");
        ui->volumeu_2->setText("<font color=\"green\">" + qsVolume + " $</font>");
    }
    else if (basevolume < _baseVolumeLast_Mintpal) {
        ui->volumes_2->setText("<font color=\"red\">" + basevolume + "</font>");
        ui->volumeu_2->setText("<font color=\"red\">" + qsVolume + " $</font>");
    }
    else {
        ui->volumes_2->setText(basevolume);
        ui->volumeu_2->setText(qsVolume + " $");
    }

    if (qslLast[0].toDouble() > qslCloseChange[0].toDouble()) {
        dClose = ((qslLast[0].toDouble() - qslCloseChange[0].toDouble())/qslLast[0].toDouble()) * 100;
        qsClose = QString::number(dClose, 'f', 2);

        ui->yest_2->setText("<font color=\"green\"> + " + qsClose + " %</font>");
    }
    else {
        dClose = ((qslCloseChange[0].toDouble() - qslLast[0].toDouble())/qslCloseChange[0].toDouble()) * 100;
        qsClose = QString::number(dClose, 'f', 2);

        ui->yest_2->setText("<font color=\"red\"> - " + qsClose + " %</font>");
    }

    _priceLast_Mintpal = qslLast[0];
    _askLast_Mintpal = qsAsk;
    _bidLast_Mintpal = qslBidHigh[0];
    _highLast_Mintpal = qslHigh[0];
    _lowLast_Mintpal = qslLow[0];
    _volumeLast_Mintpal = qslVolume[0];
    _baseVolumeLast_Mintpal = basevolume;

    _priceClose_Mintpal = qslLast[0].toDouble() > qslCloseChange[0].toDouble()
            ? qsClose : qslLast[0].toDouble() < qslCloseChange[0].toDouble()
            ? qsClose.prepend("-") : qsClose;
}
/*************************************************************************************
 * Method: PoolBrowser::mintpalTrades
 * Parameter(s): QNetworkReply *response
 *
 * Fetches the last 100 trades for a given market.
 *
 * Response:
 * [{
 *   "count":"100",
 *   "trades":[{
 *      "type":"1",
 *      "price":"0.00000023",
 *      "amount":"412128.80177019",
 *      "total":"0.09478962",
 *      "time":"1394498289.2727"
 *   },
 *   ...
 * }]
 *************************************************************************************/
void PoolBrowser::mintpalTrades(QNetworkReply *response)
{
    QString apiResponse = response->readAll();
    apiResponse = apiResponse.replace("\"", "");
    QStringList qslTemp = apiResponse.split("[{", QString::KeepEmptyParts);
    QStringList qslTrades = qslTemp[1].replace("time:", "").replace("type:", "").replace("price:", "").replace("amount:", "").replace("total:", "").split("},{", QString::KeepEmptyParts);

    //Prevent overflow by limiting trade data to no more than 100
    int tradeCount = qslTrades.length() > 100
            ? 100 : qslTrades.length();

    int z = 0;

    ui->MintpalTradesTable->clear();
    ui->MintpalTradesTable->setColumnWidth(0, 110);
    ui->MintpalTradesTable->setColumnWidth(1, 110);
    ui->MintpalTradesTable->setColumnWidth(2, 110);
    ui->MintpalTradesTable->setColumnWidth(3, 110);
    ui->MintpalTradesTable->setColumnWidth(4, 110);

    QVector<double> xAxis(tradeCount), yAxis(tradeCount);

    ui->MintpalTradesTable->setSortingEnabled(false);

    double high = 0;
    double low = 100000;

    for (int i = 0; i < tradeCount; i++) {
        QStringList qsTrade = qslTrades[i].replace("}", "").replace("{", "").replace("]", "").split(",", QString::KeepEmptyParts);
        QTreeWidgetItem * item = new QTreeWidgetItem();

        item->setText(0, qsTrade[1] == "0" ? "Buy" : "Sell");
        item->setText(1, qsTrade[2]);
        item->setText(2, qsTrade[3]);
        item->setText(3, qsTrade[4]);

        string sTime = qsTrade[0].toStdString();
        unsigned int uTime = atoi(sTime);
        const QDateTime qdtTime = QDateTime::fromTime_t(uTime);
        const QString qsTime = qdtTime.time().toString(Qt::TextDate);

        item->setText(4, qsTime);

        ui->MintpalTradesTable->addTopLevelItem(item);

        xAxis[z] = (tradeCount) - z;
        yAxis[z] = (qsTrade[2].toDouble()) * 100000000;

        if (qsTrade[2].toDouble() * 100000000 > high) {
            high = qsTrade[2].toDouble() * 100000000;
        }

        if (qsTrade[2].toDouble() * 100000000 < low) {
            low = qsTrade[2].toDouble() * 100000000;
        }

        z++;
    }

    // create graph and assign data to it:
    ui->customPlot_2->graph(0)->setData(xAxis, yAxis);
    ui->customPlot_2->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->customPlot_2->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

    // set axes ranges, so we see all data:
    ui->customPlot_2->xAxis->setRange(1, tradeCount);
    ui->customPlot_2->yAxis->setRange(low, high);

    ui->customPlot_2->replot();
}
/*************************************************************************************
 * Method: PoolBrowser::mintpalSell
 * Parameter(s): QNetworkReply *response
 *
 * Fetches the 50 best priced orders of a given type for a given market.
 *
 * Response:
 * [{
 *   "count":"23",
 *   "type":"BUY",
 *   "orders":[{
 *   "price":"0.00000023",
 *   "amount":"22446985.14519785",
 *   "total":"5.16280655"
 *  },
 *  ...
 * }]
 *************************************************************************************/
void PoolBrowser::mintpalSell(QNetworkReply *response)
{
    QString apiResponse = response->readAll();
    apiResponse = apiResponse.replace("\"", "");

    QStringList qslApiResponse = apiResponse.split("[{", QString::KeepEmptyParts);
    QStringList apiResponseSells = qslApiResponse[1].replace("price:", "").replace("amount:", "").replace("total:", "").replace("}", "").replace("{", "").split(",", QString::KeepEmptyParts);

    _apiResponseSells_Mintpal = apiResponseSells;
}
/*************************************************************************************
 * Method: PoolBrowser::mintpalBuy
 * Parameter(s): QNetworkReply *response
 *
 * Fetches the 50 best priced orders of a given type for a given market.
 *
 * Response:
 * [{
 *   "count":"23",
 *   "type":"BUY",
 *   "orders":[{
 *   "price":"0.00000023",
 *   "amount":"22446985.14519785",
 *   "total":"5.16280655"
 *  },
 *  ...
 * }]
 *************************************************************************************/
void PoolBrowser::mintpalBuy(QNetworkReply *response)
{
    QString responseData = response->readAll();
    responseData = responseData.replace("\"", "");
    QStringList responseDataList = responseData.split("[{", QString::KeepEmptyParts);
    QStringList marketDataBuys = responseDataList[1].replace("price:", "").replace("amount:", "").replace("total:", "").replace("}", "").replace("{", "").split(",", QString::KeepEmptyParts);
    QStringList marketDataSells = _apiResponseSells_Mintpal;

    //Use shorest depth as limit and use buy length if they are the same
    int depth = marketDataBuys.length() > marketDataSells.length()
            ? marketDataSells.length() : marketDataSells.length() > marketDataBuys.length()
            ? marketDataBuys.length() : marketDataBuys.length();

    //Prevent overflow by limiting depth to 150
    //Also check for odd number of trades and drop the last one
    //To avoid an overflow when there are less than 150 trades
    depth = depth > 150
            ? 150 : depth % 2 == 1
            ? depth -1 : depth;

    //Nothing to process...
    if (depth == 0){ return; }

    int z = 0;
    double high = 0;
    double low = 100000;
    double sumBuys = 0;
    double sumSells = 0;
    double sumHighest = 0;

    //Shave off space for the non-used data fields
    int adjustedDepth = depth / 3;

    QVector<double> xAxisBuys(adjustedDepth), yAxisBuys(adjustedDepth);
    QVector<double> xAxisSells(adjustedDepth), yAxisSells(adjustedDepth);

    ui->sellquan_2->clear();
    ui->buyquan_2->clear();
    ui->sellquan_2->sortByColumn(0, Qt::AscendingOrder);
    ui->sellquan_2->setSortingEnabled(true);
    ui->buyquan_2->setSortingEnabled(true);

    for (int i = 0; i < depth; i++) {
        QTreeWidgetItem * item = new QTreeWidgetItem();

        item->setText(0,marketDataBuys[i]);
        item->setText(1,marketDataBuys[i + 1]);

        ui->buyquan_2->addTopLevelItem(item);

        QTreeWidgetItem * item2 = new QTreeWidgetItem();

        item2->setText(0,marketDataSells[i]);
        item2->setText(1,marketDataSells[i + 1]);

        ui->sellquan_2->addTopLevelItem(item2);

        if (marketDataSells[i].toDouble() * 100000000 > high) {
            high = marketDataSells[i].toDouble() * 100000000;
        }

        if (marketDataBuys[i].toDouble() * 100000000 < low) {
            low = marketDataBuys[i].toDouble() * 100000000;
        }

        sumBuys += marketDataBuys[i + 1].toDouble();
        sumSells += marketDataSells[i + 1].toDouble();

        xAxisBuys[z] = marketDataBuys[i].toDouble() * 100000000;
        yAxisBuys[z] = sumBuys;
        xAxisSells[z] = marketDataSells[i].toDouble() * 100000000;
        yAxisSells[z] = sumSells;

        i += 2;
        z++;
    }

    sumHighest = sumBuys > sumSells ? sumBuys : sumBuys < sumSells ? sumSells : sumBuys;

    // create graph and assign data to it:
    ui->customPlot2_2->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->customPlot2_2->graph(1)->setData(xAxisSells, yAxisSells);
    ui->customPlot2_2->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->customPlot2_2->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    ui->customPlot2_2->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->customPlot2_2->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    // set axes ranges, so we see all data:
    ui->customPlot2_2->xAxis->setRange(low, high);
    ui->customPlot2_2->yAxis->setRange(low, sumHighest);

    ui->customPlot2_2->replot();
}

void PoolBrowser::setModel(ClientModel *model)
{
    this->model = model;
}

PoolBrowser::~PoolBrowser()
{
    delete ui;
}
