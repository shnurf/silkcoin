#include "poolbrowser.h"
#include "ui_poolbrowser.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"
#include "clientmodel.h"
#include "bitcoinrpc.h"

#include <sstream>
#include <string>

#include <QDesktopServices>
#include <QString>

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
double _dScPriceLast = 0;
double _dBtcPriceCurrent = 0;
double _dBtcPriceLast = 0;

//Bittrex Globals
BittrexMarketSummary* _bittrexMarketSummary = new BittrexMarketSummary();
BittrexTrades* _bittrexTrades = new BittrexTrades();
BittrexOrders* _bittrexOrders = new BittrexOrders();

//Cryptsy Globals
double _baseVolumeLast_Cryptsy;
QString _priceLastLast_Cryptsy = "";
QString _priceLast_Cryptsy = "";
QString _volumeLast_Cryptsy = "";
double _priceCurrent_Cryptsy = 0;

//Mintpal Globals
QString _askLast_Mintpal = "";
QString _baseVolumeLast_Mintpal = "";
QString _bidLast_Mintpal = "";
QString _highLast_Mintpal = "";
QString _lowLast_Mintpal = "";
QString _priceClose_Mintpal = "";
QString _priceLastLast_Mintpal = "";
QString _priceLast_Mintpal = "";
QString _volumeLast_Mintpal = "";
QStringList _apiResponseSells_Mintpal;
double _priceCurrent_Mintpal = 0;

PoolBrowser::PoolBrowser(QWidget* parent) : QWidget(parent), ui(new Ui::PoolBrowser)
{
    //TODO: Complete multi-threading so we don't have to call this as a primer
    getRequest(apiCoinbasePrice);

    ui->setupUi(this);
    setFixedSize(400, 420);

    ui->qCustomPlotBittrexTrades->addGraph();
    ui->qCustomPlotBittrexTrades->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotBittrexOrderDepth->addGraph();
    ui->qCustomPlotBittrexOrderDepth->addGraph();
    ui->qCustomPlotBittrexOrderDepth->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotMintpalTrades->addGraph();
    ui->qCustomPlotMintpalTrades->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotMintpalTrades->addGraph();
    ui->qCustomPlotMintpalTrades->addGraph();
    ui->qCustomPlotMintpalTrades->setBackground(QBrush(QColor("#edf1f7")));

    ui->customPlot_3->addGraph();
    ui->customPlot_3->setBackground(QBrush(QColor("#edf1f7")));

    ui->customPlot2_3->addGraph();
    ui->customPlot2_3->addGraph();
    ui->customPlot2_3->setBackground(QBrush(QColor("#edf1f7")));

    QObject::connect(&m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseNetworkResponse(QNetworkReply*)), Qt::AutoConnection);

    //One time primer
    pollAPIs();
}

void PoolBrowser::on_btnConvertSilkoin_clicked()
{
    double silkcoinQty = ui->txtConvertSilkcoinQty->text().toDouble();
    double totalBtc = _bittrexMarketSummary->getLastCurrent(double()) * silkcoinQty;
    double totalUsd = totalBtc * _dBtcPriceCurrent;

    ui->lblConvertSilkcoinResults->setText("$" + QString::number(totalUsd, 'f', 2) +
                                           "  /  B"+ QString::number(totalBtc, 'f', 8));

}
void PoolBrowser::on_btnUpdateMarketData_clicked()
{
    pollAPIs();
}

void PoolBrowser::openBittrex()
{
    QDesktopServices::openUrl(QUrl("https://www.bittrex.com/Market/Index?MarketName=BTC-SC"));
}
void PoolBrowser::openPoloniex()
{
    QDesktopServices::openUrl(QUrl("https://poloniex.com/exchange/btc_sc"));
}

void PoolBrowser::pollAPIs()
{
    ui->iconOverviewUpdateWait->setVisible(true);

    getRequest(apiCoinbasePrice);

    getRequest(apiBittrexMarketSummary);
    getRequest(apiBittrexTrades);
    getRequest(apiBittrexOrders);

    getRequest(apiCryptsyTrades);
    getRequest(apiCryptsyOrders);

    //getRequest(apiMintpalStats);
    //getRequest(apiMintpalTrades);
    //getRequest(apiMintpalSell);
    //getRequest(apiMintpalBuy);
}

void PoolBrowser::processOverview()
{
    double averageBittrexCurrent = _bittrexMarketSummary->getLastCurrent(double()) > 0 ? _bittrexMarketSummary->getLastCurrent(double()) : 0.00000001;
    double averageCryptsyCurrent = 0; //_cryptsyMarketSummary->getLastCurrent(double()) > 0 ? _cryptsyMarketSummary->getLastCurrent(double()) : 0.00000001;
    double averageMintpalCurrent = 0; //_mintpalMarketSummary->getLastCurrent(double()) > 0 ? _mintpalMarketSummary->getLastCurrent(double()) : 0.00000001;
    double averageAllCurrent = (averageBittrexCurrent + averageCryptsyCurrent + averageMintpalCurrent) / 1;

    double averageBittrexLast = _bittrexMarketSummary->getLastPrev(double()) > 0 ? _bittrexMarketSummary->getLastPrev(double()) : 0.00000001;
    double averageCryptsyLast = 0; //_cryptsyMarketSummary->getLastPrev(double()) > 0 ? _cryptsyMarketSummary->getLastPrev(double()) : 0.00000001;
    double averageMintpalLast = 0; //_mintpalMarketSummary->getLastPrev(double()) > 0 ? _mintpalMarketSummary->getLastPrev(double()) : 0.00000001;

    double averageAllLast = (averageBittrexLast + averageCryptsyLast + averageMintpalLast) / 1;

    updateLabel(ui->lblOverviewScAvgPrice,
                averageAllCurrent,
                averageAllLast,
                QString("B"),
                8);

    updateLabel(ui->lblOverviewBittrexBtc,
                _bittrexMarketSummary->getLastCurrent(double()),
                _bittrexMarketSummary->getLastPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblOverviewBittrexPerc,
                _bittrexMarketSummary->getLastCurrent(double()),
                averageAllCurrent,
                QString(""),
                QString("%"),
                2);
}

void PoolBrowser::getRequest(const QString &urlString)
{
    QUrl url (urlString);
    QNetworkRequest req (url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    m_nam.get(req);
}

void PoolBrowser::parseNetworkResponse(QNetworkReply* response)
{
    QUrl apiCall = response->url();

    if (response->error() != QNetworkReply::NoError) {
        //Communication error has occurred
        emit networkError(response->error());
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

    if (_bittrexMarketSummary->getLastPrev(double()) != 0) { // && _priceLast_Mintpal != 0 && _priceLast_Cryptsy != 0) {
        ui->iconOverviewUpdateWait->setVisible(false);
    }

    processOverview();

    response->deleteLater();
}

/*************************************************************************************
 * Method: PoolBrowser::coinbasePrice
 * Parameter(s): QNetworkReply* response
 *
 * Unauthenticated resource that returns BTC to fiat (and vice versus) exchange rates in various currencies.
 * It has keys for both btc_to_xxx and xxx_to_btc so you can convert either way.
 * The key always contains downcase representations of the currency ISO.
 * Note that some small numbers may use E notation such as 2.8e-05.
 *
 * Response: {"btc_to_pgk":"28.152994","btc_to_gyd":"2743.906541","btc_to_mmk":"11611.550858", ... ,"brl_to_btc":"0.037652"}
 *************************************************************************************/
void PoolBrowser::coinbasePrice(QNetworkReply* response)
{
    mValue jsonResponse = new mValue();
    QString apiResponse = response->readAll();

    //Make sure the response is valid
    if(!read_string(apiResponse.toStdString(), jsonResponse)) { return; }

    mObject jsonObject = jsonResponse.get_obj();

    _dBtcPriceCurrent =  QString::fromStdString(getPairValue(jsonObject, "btc_to_usd").get_str()).toDouble();

    updateLabel(ui->lblOverviewBtcUsdPrice,
                _dBtcPriceCurrent,
                _dBtcPriceLast,
                QString('$'),
                2);

    _dBtcPriceLast = _dBtcPriceCurrent;
    _dScPriceLast = _dBtcPriceCurrent * _bittrexMarketSummary->getLastCurrent(double());
}

/*************************************************************************************
 * Method: PoolBrowser::bittrexMarketSummary
 * Parameter(s): QNetworkReply* response
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
void PoolBrowser::bittrexMarketSummary(QNetworkReply* response)
{
    QString apiResponse = response->readAll();

    apiResponse.replace("{\"success\":true,\"message\":\"\",\"result\":[", "").replace("]}","").replace("},{", "}{");

    QStringList qslApiResponse = apiResponse.split("{", QString::SkipEmptyParts);

    for(int i = 0; i < qslApiResponse.count(); i++){
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponse[i].replace("\"MarketName", "{\"MarketName");

        //json_spirit does not handle null so make it "null"
        qslApiResponse[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if(read_string(qslApiResponse[i].toStdString(), jsonResponse)) {
            mObject jsonObject = jsonResponse.get_obj();

            if (getPairValue(jsonObject, "MarketName").get_str() == "BTC-SC") {
                try {
                    _bittrexMarketSummary->setHighCurrent(getPairValue(jsonObject, "High").get_real());
                    _bittrexMarketSummary->setLowCurrent(getPairValue(jsonObject, "Low").get_real());
                    _bittrexMarketSummary->setVolumeCurrent(getPairValue(jsonObject, "Volume").get_real());
                    _bittrexMarketSummary->setLastCurrent(getPairValue(jsonObject, "Last").get_real());
                    _bittrexMarketSummary->setBaseVolumeCurrent(getPairValue(jsonObject, "BaseVolume").get_real());
                    _bittrexMarketSummary->setTimeStamp(getPairValue(jsonObject, "TimeStamp").get_str());
                    _bittrexMarketSummary->setBidCurrent(getPairValue(jsonObject, "Bid").get_real());
                    _bittrexMarketSummary->setAskCurrent(getPairValue(jsonObject, "Ask").get_real());
                    _bittrexMarketSummary->setPrevDayCurrent(getPairValue(jsonObject, "PrevDay").get_real());
                }
                catch (exception) {} //API did not return all needed data so skip processing market summary

                break;
            }
        }
    }

    updateLabel(ui->lblBittrexHighBtc,
                _bittrexMarketSummary->getHighCurrent(double()),
                _bittrexMarketSummary->getHighPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblBittrexLowBtc,
                _bittrexMarketSummary->getLowCurrent(double()),
                _bittrexMarketSummary->getLowPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblBittrexCloseBtc,
                _bittrexMarketSummary->getPrevDayCurrent(double()),
                _bittrexMarketSummary->getPrevDayPrev(double()),
                QString("B"),
                8);

    double changeCurrent = (_bittrexMarketSummary->getLastCurrent(double()) - _bittrexMarketSummary->getPrevDayCurrent(double())) / _bittrexMarketSummary->getPrevDayCurrent(double()) * 100;
    double changeLast  = (_bittrexMarketSummary->getLastPrev(double()) - _bittrexMarketSummary->getPrevDayCurrent(double())) / _bittrexMarketSummary->getPrevDayCurrent(double()) * 100;

    QString changeDirection = _bittrexMarketSummary->getLastCurrent(double()) > _bittrexMarketSummary->getPrevDayCurrent(double())
            ? QString("+") : _bittrexMarketSummary->getLastCurrent(double()) < _bittrexMarketSummary->getPrevDayCurrent(double())
            ? QString("") : QString("");

    updateLabel(ui->lblBittrexChangePerc,
                changeCurrent,
                changeLast,
                changeDirection,
                QString("%"),
                2);

    updateLabel(ui->lblBittrexVolumeUsd,
                _bittrexMarketSummary->getBaseVolumeCurrent(double()) * _dBtcPriceCurrent,
                _bittrexMarketSummary->getBaseVolumePrev(double()) * _dBtcPriceCurrent,
                QString(""),
                2);

    updateLabel(ui->lblBittrexVolumeSc,
                _bittrexMarketSummary->getVolumeCurrent(double()),
                _bittrexMarketSummary->getVolumePrev(double()),
                QString(""),
                8);

    updateLabel(ui->lblBittrexVolumeBtc,
                _bittrexMarketSummary->getBaseVolumeCurrent(double()),
                _bittrexMarketSummary->getBaseVolumePrev(double()),
                QString(""),
                8);

    updateLabel(ui->lblBittrexLastBtc,
                _bittrexMarketSummary->getLastCurrent(double()),
                _bittrexMarketSummary->getLastPrev(double()),
                QString("B"),

                8);

    updateLabel(ui->lblBittrexLastUsd,
                _bittrexMarketSummary->getLastCurrent(double()) * _dBtcPriceCurrent,
                _bittrexMarketSummary->getLastPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    updateLabel(ui->lblBittrexAskBtc,
                _bittrexMarketSummary->getAskCurrent(double()),
                _bittrexMarketSummary->getAskPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblBittrexAskUsd,
                _bittrexMarketSummary->getAskCurrent(double()) * _dBtcPriceCurrent,
                _bittrexMarketSummary->getAskPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    updateLabel(ui->lblBittrexBidBtc,
                _bittrexMarketSummary->getBidCurrent(double()),
                _bittrexMarketSummary->getBidPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblBittrexBidUsd,
                _bittrexMarketSummary->getBidCurrent(double()) * _dBtcPriceCurrent,
                _bittrexMarketSummary->getBidPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    _bittrexMarketSummary->setAskPrev(_bittrexMarketSummary->getAskCurrent(double()));
    _bittrexMarketSummary->setBaseVolumePrev(_bittrexMarketSummary->getBaseVolumeCurrent(double()));
    _bittrexMarketSummary->setBidPrev(_bittrexMarketSummary->getBidCurrent(double()));
    _bittrexMarketSummary->setHighPrev(_bittrexMarketSummary->getHighCurrent(double()));
    _bittrexMarketSummary->setLowPrev(_bittrexMarketSummary->getLowCurrent(double()));
    _bittrexMarketSummary->setPrevDayPrev(_bittrexMarketSummary->getPrevDayCurrent(double()));
    _bittrexMarketSummary->setLastPrev(_bittrexMarketSummary->getLastCurrent(double()));
    _bittrexMarketSummary->setVolumePrev(_bittrexMarketSummary->getVolumeCurrent(double()));

    _dScPriceLast = _dBtcPriceCurrent * _bittrexMarketSummary->getLastCurrent(double());
}
/*************************************************************************************
 * Method: PoolBrowser::bittrexTrades
 * Parameter(s): QNetworkReply* response
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
void PoolBrowser::bittrexTrades(QNetworkReply* response)
{
    int z = 0;
    double high, low = 100000;

    ui->tblBittrexTrades->clear();
    ui->tblBittrexTrades->setColumnWidth(0, 60);
    ui->tblBittrexTrades->setColumnWidth(1, 110);
    ui->tblBittrexTrades->setColumnWidth(2, 110);
    ui->tblBittrexTrades->setColumnWidth(3, 100);
    ui->tblBittrexTrades->setColumnWidth(4, 160);
    ui->tblBittrexTrades->setSortingEnabled(false);

    QString apiResponse = response->readAll();

    apiResponse.replace("{\"success\":true,\"message\":\"\",\"result\":[", "").replace("]}","").replace("},{", "}{");

    QStringList qslApiResponse = apiResponse.split("{", QString::SkipEmptyParts);

    int tradeCount = qslApiResponse.count();
    QVector<double> xAxis(tradeCount), yAxis(tradeCount);

    for(int i = 0; i < tradeCount; i++){
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponse[i].replace("\"Id", "{\"Id");

        //json_spirit does not handle null so make it "null"
        qslApiResponse[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if(read_string(qslApiResponse[i].toStdString(), jsonResponse)) {
            mObject jsonObject = jsonResponse.get_obj();

            try
            {
                _bittrexTrades->setId(getPairValue(jsonObject, "Id").get_real());
                _bittrexTrades->setTimeStamp(getPairValue(jsonObject, "TimeStamp").get_str());
                _bittrexTrades->setQuantity(getPairValue(jsonObject, "Quantity").get_real());
                _bittrexTrades->setPrice(getPairValue(jsonObject, "Price").get_real());
                _bittrexTrades->setTotal(getPairValue(jsonObject, "Total").get_real());
                _bittrexTrades->setFillType(getPairValue(jsonObject, "FillType").get_str());
                _bittrexTrades->setOrderType(getPairValue(jsonObject, "OrderType").get_str());
            }
            catch (exception) {} //API did not return all needed data so skip this trade

            QTreeWidgetItem * qtTrades = new QTreeWidgetItem();

            qtTrades->setText(0, _bittrexTrades->getOrderType());
            qtTrades->setText(1, _bittrexTrades->getPrice(QString()));
            qtTrades->setText(2, _bittrexTrades->getQuantity(QString()));
            qtTrades->setText(3, _bittrexTrades->getTotal(QString()));
            qtTrades->setText(4, _bittrexTrades->getTimeStamp());

            ui->tblBittrexTrades->addTopLevelItem(qtTrades);

            xAxis[z] = tradeCount - z;
            yAxis[z] = _bittrexTrades->getPrice(double()) * 100000000;

            high = _bittrexTrades->getPrice(double()) > high ? _bittrexTrades->getPrice(double()) : high;
            low = _bittrexTrades->getPrice(double()) < low ? _bittrexTrades->getPrice(double()) : low;

            z++;
        }
    }

    high *=  100000000;
    low *=  100000000;

    ui->qCustomPlotBittrexTrades->graph(0)->setData(xAxis, yAxis);
    ui->qCustomPlotBittrexTrades->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotBittrexTrades->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

    // set axes ranges, so we see all data:
    ui->qCustomPlotBittrexTrades->xAxis->setRange(1, tradeCount);
    ui->qCustomPlotBittrexTrades->yAxis->setRange(low, high);

    ui->qCustomPlotBittrexTrades->replot();

}
/*************************************************************************************
 * Method: PoolBrowser::bittrexOrders
 * Parameter(s): QNetworkReply* response
 *
 * Used to get retrieve the orderbook for a given market
 *
 * Parameters:
 * market	(required)	a string literal for the market (ex: BTC-LTC)
 * type	(required)	buy, sell or both to identify the type of orderbook to return.
 * depth	(optional)	defaults to 20 - how deep of an order book to retrieve. Max is 100
 *
 * Response
 *     {
 * 	"success" : true,
 * 	"message" : "",
 * 	"result" : {
 * 		"buy" : [{
 * 				"Quantity" : 12.37000000,
 * 				"Rate" : 0.02525000
 * 			}
 * 		],
 * 		"sell" : [{
 * 				"Quantity" : 32.55412402,
 * 				"Rate" : 0.02540000
 * 			}, {
 * 				"Quantity" : 60.00000000,
 * 				"Rate" : 0.02550000
 * 			}, {
 * 				"Quantity" : 60.00000000,
 * 				"Rate" : 0.02575000
 * 			}, {
 * 				"Quantity" : 84.00000000,
 * 				"Rate" : 0.02600000
 * 			}
 * 		]
 * 	}
 * }
 ************************************************************************************/
void PoolBrowser::bittrexOrders(QNetworkReply* response)
{
    int z = 0;
    double high = 0;
    double low = 100000;
    double sumBuys = 0;
    double sumSells = 0;
    double sumHighest = 0;

    ui->qTreeWidgetBittrexBuy->clear();
    ui->qTreeWidgetBittrexBuy->sortByColumn(0, Qt::DescendingOrder);
    ui->qTreeWidgetBittrexBuy->setSortingEnabled(true);

    ui->qTreeWidgetBittrexSell->clear();
    ui->qTreeWidgetBittrexSell->sortByColumn(0, Qt::AscendingOrder);
    ui->qTreeWidgetBittrexSell->setSortingEnabled(true);

    QString apiResponse = response->readAll();

    apiResponse.replace("{\"success\":true,\"message\":\"\",\"result\":{\"buy\":[", "");
    QStringList qslApiResponse = apiResponse.split("],\"sell\":[");

    QStringList qslApiResponseBuys = qslApiResponse[0].replace("},{", "}{").split("{", QString::SkipEmptyParts);
    QStringList qslApiResponseSells = qslApiResponse[1].replace("]}}","").replace("},{", "}{").split("{", QString::SkipEmptyParts);

    //Use shorest depth as limit and use buy length if they are the same
    int depth = qslApiResponseBuys.length() > qslApiResponseSells.length()
            ? qslApiResponseSells.length() : qslApiResponseSells.length() > qslApiResponseBuys.length()
            ? qslApiResponseBuys.length() : qslApiResponseBuys.length();

    //Prevent overflow by limiting depth to 50
    //Also check for odd number of orders and drop the last one
    //To avoid an overflow when there are less than 50 orders
    depth = depth > 50
            ? 50 : depth % 2 == 1
            ? depth - 1 : depth;

    QVector<double> xAxisBuys(depth), yAxisBuys(depth);
    QVector<double> xAxisSells(depth), yAxisSells(depth);

    for(int i = 0; i < depth; i++){
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponseBuys[i].replace("\"Quantity", "{\"Quantity");
        qslApiResponseSells[i].replace("\"Quantity", "{\"Quantity");

        //json_spirit does not handle null so make it "null"
        qslApiResponseBuys[i].replace("null", "\"null\"");
        qslApiResponseSells[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if(read_string(qslApiResponseBuys[i].toStdString(), jsonResponse)) {
            mObject jsonObjectBuys = jsonResponse.get_obj();

            try
            {
                _bittrexOrders->setQuantity(getPairValue(jsonObjectBuys, "Quantity").get_real());
                _bittrexOrders->setPrice(getPairValue(jsonObjectBuys, "Rate").get_real());
                _bittrexOrders->setOrderType("Buy");
            }
            catch (exception) {} //API did not return all needed data so skip this order

            QTreeWidgetItem * qtBuys = new QTreeWidgetItem();

            qtBuys->setText(0, _bittrexOrders->getPrice(QString()));
            qtBuys->setText(1, _bittrexOrders->getQuantity(QString()));

            ui->qTreeWidgetBittrexBuy->addTopLevelItem(qtBuys);

            sumBuys += _bittrexOrders->getQuantity(double());
            xAxisBuys[z] = _bittrexOrders->getPrice(double()) * 100000000;
            yAxisBuys[z] = sumBuys;
        }

        high = _bittrexOrders->getPrice(double()) > high ? _bittrexOrders->getPrice(double()) : high;
        low = _bittrexOrders->getPrice(double()) < low ? _bittrexOrders->getPrice(double()) : low;

        //Make sure the response is valid
        if(read_string(qslApiResponseSells[i].toStdString(), jsonResponse)) {
            mObject jsonObjectSells = jsonResponse.get_obj();

            try
            {
                _bittrexOrders->setQuantity(getPairValue(jsonObjectSells, "Quantity").get_real());
                _bittrexOrders->setPrice(getPairValue(jsonObjectSells, "Rate").get_real());
                _bittrexOrders->setOrderType("Sell");
            }
            catch (exception) {} //API did not return all needed data so skip this order

            QTreeWidgetItem * qtSells = new QTreeWidgetItem();

            qtSells->setText(0, _bittrexOrders->getPrice(QString()));
            qtSells->setText(1, _bittrexOrders->getQuantity(QString()));

            ui->qTreeWidgetBittrexSell->addTopLevelItem(qtSells);

            sumSells += _bittrexOrders->getQuantity(double());
            xAxisSells[z] = _bittrexOrders->getPrice(double()) * 100000000;
            yAxisSells[z] = sumSells;
        }

        high = _bittrexOrders->getPrice(double()) > high ? _bittrexOrders->getPrice(double()) : high;
        low = _bittrexOrders->getPrice(double()) < low ? _bittrexOrders->getPrice(double()) : low;

        z++;
    }

    high *=  100000000;
    low *=  100000000;

    sumHighest = sumBuys > sumSells ? sumBuys : sumBuys < sumSells ? sumSells : sumBuys;

    ui->qCustomPlotBittrexOrderDepth->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->qCustomPlotBittrexOrderDepth->graph(1)->setData(xAxisSells, yAxisSells);

    ui->qCustomPlotBittrexOrderDepth->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotBittrexOrderDepth->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    ui->qCustomPlotBittrexOrderDepth->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->qCustomPlotBittrexOrderDepth->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    ui->qCustomPlotBittrexOrderDepth->xAxis->setRange(low, high);
    ui->qCustomPlotBittrexOrderDepth->yAxis->setRange(low, sumHighest);

    ui->qCustomPlotBittrexOrderDepth->replot();
}

/*************************************************************************************
 * Method: PoolBrowser::cryptsyTrades
 * Parameter(s): QNetworkReply* response
 *
 * General Market Data (Single Market - Realtime):
 *************************************************************************************/
void PoolBrowser::cryptsyTrades(QNetworkReply* response)
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

    ui->customPlot_3->graph(0)->setData(xAxis, yAxis);
    ui->customPlot_3->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->customPlot_3->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

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
 * Parameter(s): QNetworkReply* response
 *
 * General Orderbook Data (Single Market - Realtime):
 *************************************************************************************/
void PoolBrowser::cryptsyOrders(QNetworkReply* response)
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
    double sumBuys = 0;
    double sumSells = 0;
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

        sumBuys += apiResponseBuys[i + 1].toDouble();
        sumSells += apiResponseSells[i + 1].toDouble();

        xAxisBuys[z] = apiResponseBuys[i].toDouble() * 100000000;
        yAxisBuys[z] = sumBuys;

        xAxisSells[z] = apiResponseSells[i].toDouble() * 100000000;
        yAxisSells[z] = sumSells;

        i += 2;
        z++;
    }

    sumHighest = sumBuys > sumSells ? sumBuys : sumBuys < sumSells ? sumSells : sumBuys;

    ui->customPlot2_3->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->customPlot2_3->graph(1)->setData(xAxisSells, yAxisSells);
    ui->customPlot2_3->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->customPlot2_3->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    ui->customPlot2_3->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->customPlot2_3->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    ui->customPlot2_3->xAxis->setRange(low, high);
    ui->customPlot2_3->yAxis->setRange(low, sumHighest);

    ui->customPlot2_3->replot();
}

/*************************************************************************************
 * Method: PoolBrowser::mintpalStats
 * Parameter(s): QNetworkReply* response
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
void PoolBrowser::mintpalStats(QNetworkReply* response)
{
    double dAsk, dLast, dBid, dVolume, dClose = 0;
    QString qsAsk, qsLast, qsBid, qsVolume, qsClose = "";

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

    quint64 base = qslVolume[0].toDouble() / qslLast[0].toDouble();
    QString baseVolume = QString::number(base, 'f', 2);

    if (baseVolume > _baseVolumeLast_Mintpal) {
        ui->volumes_2->setText("<font color=\"green\">" + baseVolume + "</font>");
        ui->volumeu_2->setText("<font color=\"green\">" + qsVolume + " $</font>");
    }
    else if (baseVolume < _baseVolumeLast_Mintpal) {
        ui->volumes_2->setText("<font color=\"red\">" + baseVolume + "</font>");
        ui->volumeu_2->setText("<font color=\"red\">" + qsVolume + " $</font>");
    }
    else {
        ui->volumes_2->setText(baseVolume);
        ui->volumeu_2->setText(qsVolume + " $");
    }

    if (qslLast[0].toDouble() > qslCloseChange[0].toDouble()) {
        dClose = ((qslLast[0].toDouble() - qslCloseChange[0].toDouble()) / qslLast[0].toDouble()) * 100;
        qsClose = QString::number(dClose, 'f', 2);

        ui->yest_2->setText("<font color=\"green\"> + " + qsClose + " %</font>");
    }
    else {
        dClose = ((qslCloseChange[0].toDouble() - qslLast[0].toDouble()) / qslCloseChange[0].toDouble()) * 100;
        qsClose = QString::number(dClose, 'f', 2);

        ui->yest_2->setText("<font color=\"red\"> - " + qsClose + " %</font>");
    }

    _priceLast_Mintpal = qslLast[0];
    _askLast_Mintpal = qsAsk;
    _bidLast_Mintpal = qslBidHigh[0];
    _highLast_Mintpal = qslHigh[0];
    _lowLast_Mintpal = qslLow[0];
    _volumeLast_Mintpal = qslVolume[0];
    _baseVolumeLast_Mintpal = baseVolume;

    _priceClose_Mintpal = qslLast[0].toDouble() > qslCloseChange[0].toDouble()
            ? qsClose : qslLast[0].toDouble() < qslCloseChange[0].toDouble()
            ? qsClose.prepend("-") : qsClose;
}
/*************************************************************************************
 * Method: PoolBrowser::mintpalTrades
 * Parameter(s): QNetworkReply* response
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
void PoolBrowser::mintpalTrades(QNetworkReply* response)
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

        item->setText(0, qsTrade[1] == "0" ? "Buy" : qsTrade[1] == "1" ? "Sell" : "Unknown");
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

    ui->qCustomPlotMintpalTrades->graph(0)->setData(xAxis, yAxis);
    ui->qCustomPlotMintpalTrades->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotMintpalTrades->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

    ui->qCustomPlotMintpalTrades->xAxis->setRange(1, tradeCount);
    ui->qCustomPlotMintpalTrades->yAxis->setRange(low, high);

    ui->qCustomPlotMintpalTrades->replot();
}
/*************************************************************************************
 * Method: PoolBrowser::mintpalSell
 * Parameter(s): QNetworkReply* response
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
void PoolBrowser::mintpalSell(QNetworkReply* response)
{
    QString apiResponse = response->readAll();
    apiResponse = apiResponse.replace("\"", "");

    QStringList qslApiResponse = apiResponse.split("[{", QString::KeepEmptyParts);
    QStringList apiResponseSells = qslApiResponse[1].replace("price:", "").replace("amount:", "").replace("total:", "").replace("}", "").replace("{", "").split(",", QString::KeepEmptyParts);

    _apiResponseSells_Mintpal = apiResponseSells;
}
/*************************************************************************************
 * Method: PoolBrowser::mintpalBuy
 * Parameter(s): QNetworkReply* response
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
void PoolBrowser::mintpalBuy(QNetworkReply* response)
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
        QTreeWidgetItem * qtBuys = new QTreeWidgetItem();

        qtBuys->setText(0,marketDataBuys[i]);
        qtBuys->setText(1,marketDataBuys[i + 1]);

        ui->buyquan_2->addTopLevelItem(qtBuys);

        QTreeWidgetItem * qtSells = new QTreeWidgetItem();

        qtSells->setText(0,marketDataSells[i]);
        qtSells->setText(1,marketDataSells[i + 1]);

        ui->sellquan_2->addTopLevelItem(qtSells);

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

    ui->qCustomPlotMintpalTrades->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->qCustomPlotMintpalTrades->graph(1)->setData(xAxisSells, yAxisSells);
    ui->qCustomPlotMintpalTrades->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotMintpalTrades->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    ui->qCustomPlotMintpalTrades->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->qCustomPlotMintpalTrades->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    ui->qCustomPlotMintpalTrades->xAxis->setRange(low, high);
    ui->qCustomPlotMintpalTrades->yAxis->setRange(low, sumHighest);

    ui->qCustomPlotMintpalTrades->replot();
}

const mValue& PoolBrowser::getPairValue(const mObject& obj, const string& name)
{
    mObject::const_iterator iter = obj.find(name);

    assert(iter != obj.end());
    assert(iter->first == name);

    return iter->second;
}

void PoolBrowser::updateLabel(QLabel* qLabel, double d1, double d2, QString prefix, int decimalPlaces)
{
    qLabel->setText("");

    if (d1 > d2) {
        qLabel->setText(prefix + "<font color=\"green\"><b>" + QString::number(d1, 'f', decimalPlaces) + "</b></font>");
    }
    else if (d1 < d2) {
        qLabel->setText(prefix + "<font color=\"red\"><b>" + QString::number(d1, 'f', decimalPlaces) + "</b></font>");
    }
    else {
        qLabel->setText(prefix + QString::number(d1, 'f', decimalPlaces));
    }
}
void PoolBrowser::updateLabel(QLabel* qLabel, double d1, double d2, QString prefix, QString suffix, int decimalPlaces)
{
    qLabel->setText("");

    if (d1 > d2) {
        qLabel->setText(prefix + "<font color=\"green\"><b>" + QString::number(d1, 'f', decimalPlaces) + suffix + "</b></font>");
    }
    else if (d1 < d2) {
        qLabel->setText(prefix + "<font color=\"red\"><b>" + QString::number(d1, 'f', decimalPlaces) + suffix + "</b></font>");
    }
    else {
        qLabel->setText(prefix + QString::number(d1, 'f', decimalPlaces) + suffix);
    }
}

void PoolBrowser::setModel(ClientModel *model)
{
    this->model = model;
}

PoolBrowser::~PoolBrowser()
{
    delete ui;
}
