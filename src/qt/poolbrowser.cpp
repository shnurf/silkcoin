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
const QString apiMintpalMarketSummary = "https://api.mintpal.com/v1/market/stats/SC/BTC";
const QString apiMintpalTrades = "https://api.mintpal.com/v1/market/trades/SC/BTC";
const QString apiMintpalOrdersSell = "https://api.mintpal.com/v1/market/orders/SC/BTC/SELL";
const QString apiMintpalOrdersBuy = "https://api.mintpal.com/v1/market/orders/SC/BTC/BUY";

//Common Globals
double _dScPriceLast = 0;
double _dBtcPriceCurrent = 0;
double _dBtcPriceLast = 0;

//Bittrex Globals
BittrexMarketSummary* _bittrexMarketSummary = new BittrexMarketSummary();
BittrexTrades* _bittrexTrades = new BittrexTrades();
BittrexOrders* _bittrexOrders = new BittrexOrders();

//Cryptsy Globals
CryptsyTrades* _cryptsyTrades = new CryptsyTrades();
CryptsyOrders* _cryptsyOrders = new CryptsyOrders();

//Mintpal Globals
MintpalMarketSummary* _mintpalMarketSummary = new MintpalMarketSummary();
MintpalTrades* _mintpalTrades = new MintpalTrades();
MintpalOrders* _mintpalOrders = new MintpalOrders();
QStringList _mintpalApiResponseOrdersSell;

PoolBrowser::PoolBrowser(QWidget* parent) : QWidget(parent), ui(new Ui::PoolBrowser) {
    //TODO: Complete multi-threading so we don't have to call this as a primer
    getRequest(apiCoinbasePrice);

    ui->setupUi(this);
    setFixedSize(400, 420);

    ui->qCustomPlotBittrexTrades->addGraph();
    ui->qCustomPlotBittrexTrades->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotBittrexOrderDepth->addGraph();
    ui->qCustomPlotBittrexOrderDepth->addGraph();
    ui->qCustomPlotBittrexOrderDepth->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotCryptsyTrades->addGraph();
    ui->qCustomPlotCryptsyTrades->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotCryptsyOrderDepth->addGraph();
    ui->qCustomPlotCryptsyOrderDepth->addGraph();
    ui->qCustomPlotCryptsyOrderDepth->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotMintpalTrades->addGraph();
    ui->qCustomPlotMintpalTrades->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotMintpalOrderDepth->addGraph();
    ui->qCustomPlotMintpalOrderDepth->addGraph();
    ui->qCustomPlotMintpalOrderDepth->setBackground(QBrush(QColor("#edf1f7")));

    QObject::connect(&m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseNetworkResponse(QNetworkReply*)), Qt::AutoConnection);

    //One time primer
    pollAPIs();
}

void PoolBrowser::on_btnConvertSilkoin_clicked() {
    double silkcoinQty = ui->txtConvertSilkcoinQty->text().toDouble();
    double totalBtc = _bittrexMarketSummary->getLastCurrent(double()) * silkcoinQty;
    double totalUsd = totalBtc * _dBtcPriceCurrent;

    ui->lblConvertSilkcoinResults->setText("$" + QString::number(totalUsd, 'f', 2) +
                                           "  /  B" + QString::number(totalBtc, 'f', 8));

}
void PoolBrowser::on_btnUpdateMarketData_clicked() {
    pollAPIs();
}

void PoolBrowser::openBittrex() {
    QDesktopServices::openUrl(QUrl("https://www.bittrex.com/Market/Index?MarketName=BTC-SC"));
}
void PoolBrowser::openPoloniex() {
    QDesktopServices::openUrl(QUrl("https://poloniex.com/exchange/btc_sc"));
}

void PoolBrowser::pollAPIs() {
    ui->iconOverviewUpdateWait->setVisible(true);

    getRequest(apiCoinbasePrice);

    getRequest(apiBittrexMarketSummary);
    getRequest(apiBittrexTrades);
    getRequest(apiBittrexOrders);

    getRequest(apiCryptsyTrades);
    getRequest(apiCryptsyOrders);

    getRequest(apiMintpalMarketSummary);
    getRequest(apiMintpalTrades);
    getRequest(apiMintpalOrdersSell);
    getRequest(apiMintpalOrdersBuy);
}

void PoolBrowser::processOverview() {
    double averageBittrexCurrent = _bittrexMarketSummary->getLastCurrent(double()) > 0 ? _bittrexMarketSummary->getLastCurrent(double()) : 0.00000001;
    double averageCryptsyCurrent = _cryptsyTrades->getLastCurrent(double()) > 0 ? _cryptsyTrades->getLastCurrent(double()) : 0.00000001;
    double averageMintpalCurrent = _mintpalMarketSummary->getLastCurrent(double()) > 0 ? _mintpalMarketSummary->getLastCurrent(double()) : 0.00000001;
    double averageAllCurrent = (averageBittrexCurrent + averageCryptsyCurrent + averageMintpalCurrent) / 3;

    double averageBittrexLast = _bittrexMarketSummary->getLastPrev(double()) > 0 ? _bittrexMarketSummary->getLastPrev(double()) : 0.00000001;
    double averageCryptsyLast = _cryptsyTrades->getLastPrev(double()) > 0 ? _cryptsyTrades->getLastPrev(double()) : 0.00000001;
    double averageMintpalLast = _mintpalMarketSummary->getLastPrev(double()) > 0 ? _mintpalMarketSummary->getLastPrev(double()) : 0.00000001;

    double averageAllLast = (averageBittrexLast + averageCryptsyLast + averageMintpalLast) / 3;

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
                (_bittrexMarketSummary->getLastCurrent(double()) - averageAllCurrent) / averageAllCurrent * 100,
                0,
                QString(""),
                QString("%"),
                2);

    updateLabel(ui->lblOverviewCryptsyBtc,
                _cryptsyTrades->getLastCurrent(double()),
                _cryptsyTrades->getLastPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblOverviewCryptsyPerc,
                (_cryptsyTrades->getLastCurrent(double()) - averageAllCurrent) / averageAllCurrent * 100,
                0,
                QString(""),
                QString("%"),
                2);

    updateLabel(ui->lblOverviewMintpalBtc,
                _mintpalMarketSummary->getLastCurrent(double()),
                _mintpalMarketSummary->getLastPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblOverviewMintpalPerc,
                (_mintpalMarketSummary->getLastCurrent(double()) - averageAllCurrent) / averageAllCurrent * 100,
                0,
                QString(""),
                QString("%"),
                2);
}

void PoolBrowser::getRequest(const QString &urlString) {
    QUrl url(urlString);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    m_nam.get(req);
}

void PoolBrowser::parseNetworkResponse(QNetworkReply* response) {
    QUrl apiCall = response->url();

    if (response->error() != QNetworkReply::NoError) {
        //Communication error has occurred
        emit networkError(response->error());
        return;
    }

    if (apiCall == apiCoinbasePrice) {
        coinbasePrice(response);
    } else if (apiCall == apiBittrexMarketSummary) {
        bittrexMarketSummary(response);
    } else if (apiCall == apiBittrexTrades) {
        bittrexTrades(response);
    } else if (apiCall == apiBittrexOrders) {
        bittrexOrders(response);
    } else if (apiCall == apiCryptsyTrades) {
        cryptsyTrades(response);
    } else if (apiCall == apiCryptsyOrders) {
        cryptsyOrders(response);
    } else if (apiCall == apiMintpalMarketSummary) {
        mintpalMarketSummary(response);
    } else if (apiCall == apiMintpalTrades) {
        mintpalTrades(response);
    } else if (apiCall == apiMintpalOrdersSell) {
        mintpalOrdersSell(response);
    } else if (apiCall == apiMintpalOrdersBuy) {
        mintpalOrdersBuy(response);
    } else { } //Should NEVER get here unless something went completely awry

    if (_bittrexMarketSummary->getLastPrev(double()) > 0 &&
            _cryptsyTrades->getLastCurrent(double()) > 0 &&
            _mintpalMarketSummary->getLastCurrent(double()) > 0) {
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
void PoolBrowser::coinbasePrice(QNetworkReply* response) {
    mValue jsonResponse = new mValue();
    QString apiResponse = response->readAll();

    //Make sure the response is valid
    if (!read_string(apiResponse.toStdString(), jsonResponse)) {
        return;
    }

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
 *  "success" : true,
 *  "message" : "",
 *  "result" : [{
 *          "MarketName" : "BTC-LTC",
 *          "High" : 0.02590000,
 *          "Low" : 0.02400000,
 *          "Volume" : 114.84340665,
 *          "Last" : 0.02480000,
 *          "BaseVolume" : 2.85028800,
 *          "TimeStamp" : "2014-04-19T20:49:23.483"
 *         }, {
 *          "MarketName" : "BTC-WC",
 *          "High" : 0.00002456,
 *          "Low" : 0.00001352,
 *          "Volume" : 4574426.27271220,
 *          "Last" : 0.00002006,
 *          "BaseVolume" : 82.96629666,
 *          "TimeStamp" : "2014-04-19T20:49:50.053"
 *         }
 *  ]
 * }
 *************************************************************************************/
void PoolBrowser::bittrexMarketSummary(QNetworkReply* response) {
    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("{\"success\":true,\"message\":\"\",\"result\":[", "").replace("]}", "").replace("},{", "}{");

    QStringList qslApiResponse = apiResponse.split("{", QString::SkipEmptyParts);

    for (int i = 0; i < qslApiResponse.count(); i++) {
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponse[i].replace("\"MarketName", "{\"MarketName");

        //json_spirit does not handle null so make it "null"
        qslApiResponse[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if (read_string(qslApiResponse[i].toStdString(), jsonResponse)) {
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
                } catch (exception) {} //API did not return all needed data so skip processing market summary

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
                4);

    updateLabel(ui->lblBittrexVolumeBtc,
                _bittrexMarketSummary->getBaseVolumeCurrent(double()),
                _bittrexMarketSummary->getBaseVolumePrev(double()),
                QString(""),
                4);

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
 *  "success" : true,
 *  "message" : "",
 *  "result" : [{
 *          "OrderId" : "12323",
 *          "TimeStamp" : "2014-02-25T07:40:08.68",
 *          "Quantity" : 185.06100000,
 *          "Price" : 0.00000174,
 *          "Total" : 0.00032200
 *      }, {
 *          "OrderUuid" : "12322",
 *          "TimeStamp" : "2014-02-25T07:39:18.603",
 *          "Quantity" : 10.74500000,
 *          "Price" : 0.00000172,
 *          "Total" : 0.00001848
 *      }, {
 *          "OrderUuid" : "12321",
 *          "TimeStamp" : "2014-02-25T07:39:18.6",
 *          "Quantity" : 5.62100000,
 *          "Price" : 0.00000172,
 *          "Total" : 0.00000966
 *      }, {
 *          "OrderUuid" : "12319",
 *          "TimeStamp" : "2014-02-25T07:39:18.6",
 *          "Quantity" : 76.23000000,
 *          "Price" : 0.00000173,
 *          "Total" : 0.00013187
 *      }, {
 *          "OrderUuid" : "12317",
 *          "TimeStamp" : "2014-02-25T07:39:18.6",
 *          "Quantity" : 52.47500000,
 *          "Price" : 0.00000174,
 *          "Total" : 0.00009130
 *      }
 *  ]
 * }
 *************************************************************************************/
void PoolBrowser::bittrexTrades(QNetworkReply* response) {
    int z = 0;
    double high = 0;
    double low = 100000;

    ui->tblBittrexTrades->clear();
    ui->tblBittrexTrades->setColumnWidth(0, 60);
    ui->tblBittrexTrades->setColumnWidth(1, 110);
    ui->tblBittrexTrades->setColumnWidth(2, 110);
    ui->tblBittrexTrades->setColumnWidth(3, 100);
    ui->tblBittrexTrades->setColumnWidth(4, 160);
    ui->tblBittrexTrades->setSortingEnabled(false);

    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("{\"success\":true,\"message\":\"\",\"result\":[", "").replace("]}", "").replace("},{", "}{");

    QStringList qslApiResponse = apiResponse.split("{", QString::SkipEmptyParts);

    int tradeCount = qslApiResponse.count();
    QVector<double> xAxis(tradeCount), yAxis(tradeCount);

    for (int i = 0; i < tradeCount; i++) {
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponse[i].replace("\"Id", "{\"Id");

        //json_spirit does not handle null so make it "null"
        qslApiResponse[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if (read_string(qslApiResponse[i].toStdString(), jsonResponse)) {
            mObject jsonObject = jsonResponse.get_obj();

            try {
                _bittrexTrades->setId(getPairValue(jsonObject, "Id").get_real());
                _bittrexTrades->setTimeStamp(getPairValue(jsonObject, "TimeStamp").get_str());
                _bittrexTrades->setQuantity(getPairValue(jsonObject, "Quantity").get_real());
                _bittrexTrades->setPrice(getPairValue(jsonObject, "Price").get_real());
                _bittrexTrades->setTotal(getPairValue(jsonObject, "Total").get_real());
                _bittrexTrades->setFillType(getPairValue(jsonObject, "FillType").get_str());
                _bittrexTrades->setOrderType(getPairValue(jsonObject, "OrderType").get_str());
            } catch (exception) {} //API did not return all needed data so skip this trade

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
 * market   (required)  a string literal for the market (ex: BTC-LTC)
 * type (required)  buy, sell or both to identify the type of orderbook to return.
 * depth    (optional)  defaults to 20 - how deep of an order book to retrieve. Max is 100
 *
 * Response
 *     {
 *  "success" : true,
 *  "message" : "",
 *  "result" : {
 *      "buy" : [{
 *              "Quantity" : 12.37000000,
 *              "Rate" : 0.02525000
 *          }
 *      ],
 *      "sell" : [{
 *              "Quantity" : 32.55412402,
 *              "Rate" : 0.02540000
 *          }, {
 *              "Quantity" : 60.00000000,
 *              "Rate" : 0.02550000
 *          }, {
 *              "Quantity" : 60.00000000,
 *              "Rate" : 0.02575000
 *          }, {
 *              "Quantity" : 84.00000000,
 *              "Rate" : 0.02600000
 *          }
 *      ]
 *  }
 * }
 ************************************************************************************/
void PoolBrowser::bittrexOrders(QNetworkReply* response) {
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

    apiResponse = apiResponse.replace("{\"success\":true,\"message\":\"\",\"result\":{\"buy\":[", "");
    QStringList qslApiResponse = apiResponse.split("],\"sell\":[");

    QStringList qslApiResponseBuys = qslApiResponse[0].replace("},{", "}{").split("{", QString::SkipEmptyParts);
    QStringList qslApiResponseSells = qslApiResponse[1].replace("]}}", "").replace("},{", "}{").split("{", QString::SkipEmptyParts);

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

    for (int i = 0; i < depth; i++) {
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponseBuys[i].replace("\"Quantity", "{\"Quantity");
        qslApiResponseSells[i].replace("\"Quantity", "{\"Quantity");

        //json_spirit does not handle null so make it "null"
        qslApiResponseBuys[i].replace("null", "\"null\"");
        qslApiResponseSells[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if (read_string(qslApiResponseBuys[i].toStdString(), jsonResponse)) {
            mObject jsonObjectBuys = jsonResponse.get_obj();

            try {
                _bittrexOrders->setQuantity(getPairValue(jsonObjectBuys, "Quantity").get_real());
                _bittrexOrders->setPrice(getPairValue(jsonObjectBuys, "Rate").get_real());
                _bittrexOrders->setOrderType("Buy");
            } catch (exception) {} //API did not return all needed data so skip this order

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
        if (read_string(qslApiResponseSells[i].toStdString(), jsonResponse)) {
            mObject jsonObjectSells = jsonResponse.get_obj();

            try {
                _bittrexOrders->setQuantity(getPairValue(jsonObjectSells, "Quantity").get_real());
                _bittrexOrders->setPrice(getPairValue(jsonObjectSells, "Rate").get_real());
                _bittrexOrders->setOrderType("Sell");
            } catch (exception) {} //API did not return all needed data so skip this order

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
void PoolBrowser::cryptsyTrades(QNetworkReply* response) {
    int z = 0;
    double high = 0;
    double low = 100000;

    ui->tblCryptsyTrades->clear();
    ui->tblCryptsyTrades->setColumnWidth(0, 60);
    ui->tblCryptsyTrades->setColumnWidth(1, 110);
    ui->tblCryptsyTrades->setColumnWidth(2, 110);
    ui->tblCryptsyTrades->setColumnWidth(3, 100);
    ui->tblCryptsyTrades->setColumnWidth(4, 160);
    ui->tblCryptsyTrades->setSortingEnabled(false);

    QString apiResponse = response->readAll();

    QStringList qslApiResponse = apiResponse.replace("{\"success\":1,\"return\":{\"markets\":{\"SC\":", "").replace(",\"recenttrades\":", "}").replace("]}}}}", "").replace("},{", "}{").split("[");
    QString apiResponseMarketSummary = qslApiResponse[0].replace("\\", "");
    QStringList qslApiResponseTrades = qslApiResponse[1].split("{", QString::SkipEmptyParts);

    mValue jsonResponseMarketSummary = new mValue();

    if (!read_string(apiResponseMarketSummary.toStdString(), jsonResponseMarketSummary)) {
        return;
    }

    mObject jsonObjectMarketSummary = jsonResponseMarketSummary.get_obj();

    _cryptsyTrades->setLastCurrent(getPairValue(jsonObjectMarketSummary, "lasttradeprice").get_str());
    _cryptsyTrades->setVolumeCurrent(getPairValue(jsonObjectMarketSummary, "volume").get_str());

    int tradeCount = qslApiResponseTrades.count();

    //Prevent overflow by limiting trade data to no more than 100
    tradeCount = tradeCount > 100
                 ? 100 : tradeCount % 2 == 1
                 ? tradeCount - 1 : tradeCount;

    QVector<double> xAxis(tradeCount), yAxis(tradeCount);

    for (int i = 0; i < tradeCount; i++) {
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponseTrades[i].replace("\"id", "{\"id");

        //json_spirit does not handle null so make it "null"
        qslApiResponseTrades[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if (read_string(qslApiResponseTrades[i].toStdString(), jsonResponse)) {
            mObject jsonObject = jsonResponse.get_obj();

            try {
                _cryptsyTrades->setId(getPairValue(jsonObject, "id").get_str());
                _cryptsyTrades->setTimeStamp(getPairValue(jsonObject, "time").get_str());
                _cryptsyTrades->setPrice(getPairValue(jsonObject, "price").get_str());
                _cryptsyTrades->setQuantity(getPairValue(jsonObject, "quantity").get_str());
                _cryptsyTrades->setTotal(getPairValue(jsonObject, "total").get_str());
                _cryptsyTrades->setOrderType(getPairValue(jsonObject, "type").get_str());
            } catch (exception) {} //API did not return all needed data so skip this trade

            QTreeWidgetItem * qtTrades = new QTreeWidgetItem();

            qtTrades->setText(0, _cryptsyTrades->getOrderType());
            qtTrades->setText(1, _cryptsyTrades->getPrice(QString()));
            qtTrades->setText(2, _cryptsyTrades->getQuantity(QString()));
            qtTrades->setText(3, _cryptsyTrades->getTotal(QString()));
            qtTrades->setText(4, _cryptsyTrades->getTimeStamp());

            ui->tblCryptsyTrades->addTopLevelItem(qtTrades);

            xAxis[z] = tradeCount - z;
            yAxis[z] = _cryptsyTrades->getPrice(double()) * 100000000;

            high = _cryptsyTrades->getPrice(double()) > high ? _cryptsyTrades->getPrice(double()) : high;
            low = _cryptsyTrades->getPrice(double()) < low ? _cryptsyTrades->getPrice(double()) : low;

            z++;
        }
    }

    high *=  100000000;
    low *=  100000000;

    updateLabel(ui->lblCryptsyVolumeSc,
                _cryptsyTrades->getVolumeCurrent(double()),
                _cryptsyTrades->getVolumePrev(double()),
                QString(""),
                4);

    updateLabel(ui->lblCryptsyVolumeBtc,
                _cryptsyTrades->getVolumeCurrent(double()) * _cryptsyTrades->getLastCurrent(double()),
                _cryptsyTrades->getVolumePrev(double()) * _cryptsyTrades->getLastCurrent(double()),
                QString(""),
                4);

    updateLabel(ui->lblCryptsyVolumeUsd,
                _cryptsyTrades->getVolumeCurrent(double()) * _cryptsyTrades->getLastCurrent(double()) * _dBtcPriceCurrent,
                _cryptsyTrades->getVolumePrev(double()) * _cryptsyTrades->getLastCurrent(double()) * _dBtcPriceCurrent,
                QString(""),
                4);

    updateLabel(ui->lblCryptsyLastBtc,
                _cryptsyTrades->getLastCurrent(double()),
                _cryptsyTrades->getLastPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblCryptsyLastUsd,
                _cryptsyTrades->getLastCurrent(double()) * _dBtcPriceCurrent,
                _cryptsyTrades->getLastPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    ui->qCustomPlotCryptsyTrades->graph(0)->setData(xAxis, yAxis);
    ui->qCustomPlotCryptsyTrades->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotCryptsyTrades->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

    ui->qCustomPlotCryptsyTrades->xAxis->setRange(1, tradeCount);
    ui->qCustomPlotCryptsyTrades->yAxis->setRange(low, high);

    ui->qCustomPlotCryptsyTrades->replot();

    _cryptsyTrades->setLastPrev(_cryptsyTrades->getLastCurrent(QString()).toStdString());
    _cryptsyTrades->setVolumePrev(_cryptsyTrades->getVolumeCurrent(QString()).toStdString());
}
/*************************************************************************************
 * Method: PoolBrowser::cryptsyOrders
 * Parameter(s): QNetworkReply* response
 *
 * General Orderbook Data (Single Market - Realtime):
 *************************************************************************************/
void PoolBrowser::cryptsyOrders(QNetworkReply* response) {
    int z = 0;
    double high = 0;
    double low = 100000;
    double sumBuys = 0;
    double sumSells = 0;
    double sumHighest = 0;

    ui->qTreeWidgetCryptsyBuy->clear();
    ui->qTreeWidgetCryptsyBuy->sortByColumn(0, Qt::DescendingOrder);
    ui->qTreeWidgetCryptsyBuy->setSortingEnabled(true);

    ui->qTreeWidgetCryptsySell->clear();
    ui->qTreeWidgetCryptsySell->sortByColumn(0, Qt::AscendingOrder);
    ui->qTreeWidgetCryptsySell->setSortingEnabled(true);

    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("{\"success\":1,\"return\":{\"SC\":{\"marketid\":\"225\",\"label\":\"SC\\/BTC\",\"primaryname\":\"SilkCoin\",\"primarycode\":\"SC\",\"secondaryname\":\"BitCoin\",\"secondarycode\":\"BTC\",\"sellorders\":[", "");
    QStringList qslApiResponse = apiResponse.split("],\"buyorders\":[");

    QStringList qslApiResponseSells = qslApiResponse[0].replace("},{", "}{").split("{", QString::SkipEmptyParts);
    QStringList qslApiResponseBuys = qslApiResponse[1].replace("]}}}", "").replace("},{", "}{").split("{", QString::SkipEmptyParts);

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

    for (int i = 0; i < depth; i++) {
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponseBuys[i].replace("\"price", "{\"price");
        qslApiResponseSells[i].replace("\"price", "{\"price");

        //json_spirit does not handle null so make it "null"
        qslApiResponseBuys[i].replace("null", "\"null\"");
        qslApiResponseSells[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if (read_string(qslApiResponseBuys[i].toStdString(), jsonResponse)) {
            mObject jsonObjectBuys = jsonResponse.get_obj();

            try {
                _cryptsyOrders->setQuantity(getPairValue(jsonObjectBuys, "quantity").get_str());
                _cryptsyOrders->setPrice(getPairValue(jsonObjectBuys, "price").get_str());
                _cryptsyOrders->setOrderType("Buy");
            } catch (exception) {} //API did not return all needed data so skip this order

            QTreeWidgetItem * qtBuys = new QTreeWidgetItem();

            qtBuys->setText(0, _cryptsyOrders->getPrice(QString()));
            qtBuys->setText(1, _cryptsyOrders->getQuantity(QString()));

            ui->qTreeWidgetCryptsyBuy->addTopLevelItem(qtBuys);

            sumBuys += _cryptsyOrders->getQuantity(double());
            xAxisBuys[z] = _cryptsyOrders->getPrice(double()) * 100000000;
            yAxisBuys[z] = sumBuys;
        }

        high = _cryptsyOrders->getPrice(double()) > high ? _cryptsyOrders->getPrice(double()) : high;
        low = _cryptsyOrders->getPrice(double()) < low ? _cryptsyOrders->getPrice(double()) : low;

        string WTF = qslApiResponseSells[i].toStdString();

        //Make sure the response is valid
        if (read_string(qslApiResponseSells[i].toStdString(), jsonResponse)) {
            mObject jsonObjectSells = jsonResponse.get_obj();

            try {
                _cryptsyOrders->setQuantity(getPairValue(jsonObjectSells, "quantity").get_str());
                _cryptsyOrders->setPrice(getPairValue(jsonObjectSells, "price").get_str());
                _cryptsyOrders->setOrderType("Sell");
            } catch (exception) {} //API did not return all needed data so skip this order

            QTreeWidgetItem * qtSells = new QTreeWidgetItem();

            qtSells->setText(0, _cryptsyOrders->getPrice(QString()));
            qtSells->setText(1, _cryptsyOrders->getQuantity(QString()));

            ui->qTreeWidgetCryptsySell->addTopLevelItem(qtSells);

            sumSells += _cryptsyOrders->getQuantity(double());
            xAxisSells[z] = _cryptsyOrders->getPrice(double()) * 100000000;
            yAxisSells[z] = sumSells;
        }

        high = _cryptsyOrders->getPrice(double()) > high ? _cryptsyOrders->getPrice(double()) : high;
        low = _cryptsyOrders->getPrice(double()) < low ? _cryptsyOrders->getPrice(double()) : low;

        z++;
    }

    high *=  100000000;
    low *=  100000000;

    sumHighest = sumBuys > sumSells ? sumBuys : sumBuys < sumSells ? sumSells : sumBuys;

    ui->qCustomPlotCryptsyOrderDepth->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->qCustomPlotCryptsyOrderDepth->graph(1)->setData(xAxisSells, yAxisSells);

    ui->qCustomPlotCryptsyOrderDepth->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotCryptsyOrderDepth->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    ui->qCustomPlotCryptsyOrderDepth->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->qCustomPlotCryptsyOrderDepth->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    ui->qCustomPlotCryptsyOrderDepth->xAxis->setRange(low, high);
    ui->qCustomPlotCryptsyOrderDepth->yAxis->setRange(low, sumHighest);

    ui->qCustomPlotCryptsyOrderDepth->replot();
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
void PoolBrowser::mintpalMarketSummary(QNetworkReply* response) {

    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("[", "").replace("]", "");

    mValue jsonResponse = new mValue();

    //Make sure the response is valid
    if (read_string(apiResponse.toStdString(), jsonResponse)) {
        mObject jsonObject = jsonResponse.get_obj();

        try {
            _mintpalMarketSummary->setHighCurrent(getPairValue(jsonObject, "24hhigh").get_str());
            _mintpalMarketSummary->setLowCurrent(getPairValue(jsonObject, "24hlow").get_str());
            _mintpalMarketSummary->setVolumeCurrent(getPairValue(jsonObject, "24hvol").get_str());
            _mintpalMarketSummary->setLastCurrent(getPairValue(jsonObject, "last_price").get_str());
            _mintpalMarketSummary->setBaseVolumeCurrent(getPairValue(jsonObject, "24hvol").get_str());
            _mintpalMarketSummary->setBidCurrent(getPairValue(jsonObject, "top_bid").get_str());
            _mintpalMarketSummary->setAskCurrent(getPairValue(jsonObject, "top_ask").get_str());
            _mintpalMarketSummary->setPrevDayCurrent(getPairValue(jsonObject, "yesterday_price").get_str());
            _mintpalMarketSummary->setChangeCurrent(getPairValue(jsonObject, "change").get_str());
        } catch (exception) {} //API did not return all needed data so skip processing market summary

    }

    updateLabel(ui->lblMintpalHighBtc,
                _mintpalMarketSummary->getHighCurrent(double()),
                _mintpalMarketSummary->getHighPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblMintpalLowBtc,
                _mintpalMarketSummary->getLowCurrent(double()),
                _mintpalMarketSummary->getLowPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblMintpalCloseBtc,
                _mintpalMarketSummary->getPrevDayCurrent(double()),
                _mintpalMarketSummary->getPrevDayPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblMintpalChangePerc,
                _mintpalMarketSummary->getChangeCurrent(double()),
                _mintpalMarketSummary->getChangePrev(double()),
                QString(""),
                QString("%"),
                2);

    updateLabel(ui->lblMintpalVolumeUsd,
                _mintpalMarketSummary->getBaseVolumeCurrent(double()) * _dBtcPriceCurrent,
                _mintpalMarketSummary->getBaseVolumePrev(double()) * _dBtcPriceCurrent,
                QString(""),
                2);

    updateLabel(ui->lblMintpalVolumeSc,
                _mintpalMarketSummary->getVolumeCurrent(double()) / _mintpalMarketSummary->getLastCurrent(double()),
                _mintpalMarketSummary->getVolumePrev(double()) / _mintpalMarketSummary->getLastCurrent(double()),
                QString(""),
                4);

    updateLabel(ui->lblMintpalVolumeBtc,
                _mintpalMarketSummary->getBaseVolumeCurrent(double()),
                _mintpalMarketSummary->getBaseVolumePrev(double()),
                QString(""),
                4);

    updateLabel(ui->lblMintpalLastBtc,
                _mintpalMarketSummary->getLastCurrent(double()),
                _mintpalMarketSummary->getLastPrev(double()),
                QString("B"),

                8);

    updateLabel(ui->lblMintpalLastUsd,
                _mintpalMarketSummary->getLastCurrent(double()) * _dBtcPriceCurrent,
                _mintpalMarketSummary->getLastPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    updateLabel(ui->lblMintpalAskBtc,
                _mintpalMarketSummary->getAskCurrent(double()),
                _mintpalMarketSummary->getAskPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblMintpalAskUsd,
                _mintpalMarketSummary->getAskCurrent(double()) * _dBtcPriceCurrent,
                _mintpalMarketSummary->getAskPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    updateLabel(ui->lblMintpalBidBtc,
                _mintpalMarketSummary->getBidCurrent(double()),
                _mintpalMarketSummary->getBidPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblMintpalBidUsd,
                _mintpalMarketSummary->getBidCurrent(double()) * _dBtcPriceCurrent,
                _mintpalMarketSummary->getBidPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    _mintpalMarketSummary->setAskPrev(_mintpalMarketSummary->getAskCurrent(double()));
    _mintpalMarketSummary->setBaseVolumePrev(_mintpalMarketSummary->getBaseVolumeCurrent(double()));
    _mintpalMarketSummary->setBidPrev(_mintpalMarketSummary->getBidCurrent(double()));
    _mintpalMarketSummary->setChangePrev(_mintpalMarketSummary->getChangeCurrent(double()));
    _mintpalMarketSummary->setHighPrev(_mintpalMarketSummary->getHighCurrent(double()));
    _mintpalMarketSummary->setLowPrev(_mintpalMarketSummary->getLowCurrent(double()));
    _mintpalMarketSummary->setPrevDayPrev(_mintpalMarketSummary->getPrevDayCurrent(double()));
    _mintpalMarketSummary->setLastPrev(_mintpalMarketSummary->getLastCurrent(double()));
    _mintpalMarketSummary->setVolumePrev(_mintpalMarketSummary->getVolumeCurrent(double()));

    _dScPriceLast = _dBtcPriceCurrent * _mintpalMarketSummary->getLastCurrent(double());
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
void PoolBrowser::mintpalTrades(QNetworkReply* response) {
    int z = 0;
    double high = 0;
    double low = 100000;

    ui->tblMintpalTrades->clear();
    ui->tblMintpalTrades->setColumnWidth(0, 60);
    ui->tblMintpalTrades->setColumnWidth(1, 110);
    ui->tblMintpalTrades->setColumnWidth(2, 110);
    ui->tblMintpalTrades->setColumnWidth(3, 100);
    ui->tblMintpalTrades->setColumnWidth(4, 160);
    ui->tblMintpalTrades->setSortingEnabled(false);

    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("{\"count\":100,\"trades\":[", "").replace("]}", "").replace("},{", "}{");

    QStringList qslApiResponse = apiResponse.split("{", QString::SkipEmptyParts);

    int tradeCount = qslApiResponse.count();
    QVector<double> xAxis(tradeCount), yAxis(tradeCount);

    for (int i = 0; i < tradeCount; i++) {
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponse[i].replace("\"time", "{\"time");

        //json_spirit does not handle null so make it "null"
        qslApiResponse[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if (read_string(qslApiResponse[i].toStdString(), jsonResponse)) {
            mObject jsonObject = jsonResponse.get_obj();

            try {
                _mintpalTrades->setTimeStamp(getPairValue(jsonObject, "time").get_str());
                _mintpalTrades->setQuantity(getPairValue(jsonObject, "amount").get_str());
                _mintpalTrades->setPrice(getPairValue(jsonObject, "price").get_str());
                _mintpalTrades->setTotal(getPairValue(jsonObject, "total").get_str());
                _mintpalTrades->setOrderType(getPairValue(jsonObject, "type").get_real());
            } catch (exception) {} //API did not return all needed data so skip this trade

            QTreeWidgetItem * qtTrades = new QTreeWidgetItem();

            qtTrades->setText(0, _mintpalTrades->getOrderType());
            qtTrades->setText(1, _mintpalTrades->getPrice(QString()));
            qtTrades->setText(2, _mintpalTrades->getQuantity(QString()));
            qtTrades->setText(3, _mintpalTrades->getTotal(QString()));
            qtTrades->setText(4, _mintpalTrades->getTimeStamp());

            ui->tblMintpalTrades->addTopLevelItem(qtTrades);

            xAxis[z] = tradeCount - z;
            yAxis[z] = _mintpalTrades->getPrice(double()) * 100000000;

            high = _mintpalTrades->getPrice(double()) > high ? _mintpalTrades->getPrice(double()) : high;
            low = _mintpalTrades->getPrice(double()) < low ? _mintpalTrades->getPrice(double()) : low;

            z++;
        }
    }

    high *=  100000000;
    low *=  100000000;

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
void PoolBrowser::mintpalOrdersSell(QNetworkReply* response) {
    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("]}" , "");
    QStringList qslApiResponse = apiResponse.split("\"orders\":[");
    _mintpalApiResponseOrdersSell = qslApiResponse[1].replace("},{", "}{").split("{", QString::SkipEmptyParts);
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
void PoolBrowser::mintpalOrdersBuy(QNetworkReply* response) {
    int z = 0;
    double high = 0;
    double low = 100000;
    double sumBuys = 0;
    double sumSells = 0;
    double sumHighest = 0;

    ui->qTreeWidgetMintpalBuy->clear();
    ui->qTreeWidgetMintpalBuy->sortByColumn(0, Qt::DescendingOrder);
    ui->qTreeWidgetMintpalBuy->setSortingEnabled(true);

    ui->qTreeWidgetMintpalSell->clear();
    ui->qTreeWidgetMintpalSell->sortByColumn(0, Qt::AscendingOrder);
    ui->qTreeWidgetMintpalSell->setSortingEnabled(true);

    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("]}" , "");
    QStringList qslApiResponse = apiResponse.split("\"orders\":[");
    QStringList qslApiResponseBuys = qslApiResponse[1].replace("},{", "}{").split("{", QString::SkipEmptyParts);
    QStringList qslApiResponseSells = _mintpalApiResponseOrdersSell;

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

    for (int i = 0; i < depth; i++) {
        mValue jsonResponse = new mValue();

        //Fix missing leading brace caused by split string, otherwise it will not be recognized an an mObject
        qslApiResponseBuys[i].replace("\"price", "{\"price");
        qslApiResponseSells[i].replace("\"price", "{\"price");

        //json_spirit does not handle null so make it "null"
        qslApiResponseBuys[i].replace("null", "\"null\"");
        qslApiResponseSells[i].replace("null", "\"null\"");

        //Make sure the response is valid
        if (read_string(qslApiResponseBuys[i].toStdString(), jsonResponse)) {
            mObject jsonObjectBuys = jsonResponse.get_obj();

            try {
                _mintpalOrders->setQuantity(getPairValue(jsonObjectBuys, "amount").get_str());
                _mintpalOrders->setPrice(getPairValue(jsonObjectBuys, "price").get_str());
                _mintpalOrders->setOrderType("Buy");
            } catch (exception) {} //API did not return all needed data so skip this order

            QTreeWidgetItem * qtBuys = new QTreeWidgetItem();

            qtBuys->setText(0, _mintpalOrders->getPrice(QString()));
            qtBuys->setText(1, _mintpalOrders->getQuantity(QString()));

            ui->qTreeWidgetMintpalBuy->addTopLevelItem(qtBuys);

            sumBuys += _mintpalOrders->getQuantity(double());
            xAxisBuys[z] = _mintpalOrders->getPrice(double()) * 100000000;
            yAxisBuys[z] = sumBuys;
        }

        high = _mintpalOrders->getPrice(double()) > high ? _mintpalOrders->getPrice(double()) : high;
        low = _mintpalOrders->getPrice(double()) < low ? _mintpalOrders->getPrice(double()) : low;

        //Make sure the response is valid
        if (read_string(qslApiResponseSells[i].toStdString(), jsonResponse)) {
            mObject jsonObjectSells = jsonResponse.get_obj();

            try {
                _mintpalOrders->setQuantity(getPairValue(jsonObjectSells, "amount").get_str());
                _mintpalOrders->setPrice(getPairValue(jsonObjectSells, "price").get_str());
                _mintpalOrders->setOrderType("Sell");
            } catch (exception) {} //API did not return all needed data so skip this order

            QTreeWidgetItem * qtSells = new QTreeWidgetItem();

            qtSells->setText(0, _mintpalOrders->getPrice(QString()));
            qtSells->setText(1, _mintpalOrders->getQuantity(QString()));

            ui->qTreeWidgetMintpalSell->addTopLevelItem(qtSells);

            sumSells += _mintpalOrders->getQuantity(double());
            xAxisSells[z] = _mintpalOrders->getPrice(double()) * 100000000;
            yAxisSells[z] = sumSells;
        }

        high = _mintpalOrders->getPrice(double()) > high ? _mintpalOrders->getPrice(double()) : high;
        low = _mintpalOrders->getPrice(double()) < low ? _mintpalOrders->getPrice(double()) : low;

        z++;
    }

    high *=  100000000;
    low *=  100000000;

    sumHighest = sumBuys > sumSells ? sumBuys : sumBuys < sumSells ? sumSells : sumBuys;

    ui->qCustomPlotMintpalOrderDepth->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->qCustomPlotMintpalOrderDepth->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotMintpalOrderDepth->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

    ui->qCustomPlotMintpalOrderDepth->graph(1)->setData(xAxisSells, yAxisSells);
    ui->qCustomPlotMintpalOrderDepth->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->qCustomPlotMintpalOrderDepth->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    ui->qCustomPlotMintpalOrderDepth->xAxis->setRange(low, high);
    ui->qCustomPlotMintpalOrderDepth->yAxis->setRange(low, sumHighest);

    ui->qCustomPlotMintpalOrderDepth->replot();
}

const mValue& PoolBrowser::getPairValue(const mObject& obj, const string& name) {
    mObject::const_iterator iter = obj.find(name);

    assert(iter != obj.end());
    assert(iter->first == name);

    return iter->second;
}

void PoolBrowser::updateLabel(QLabel* qLabel, double d1, double d2, QString prefix, int decimalPlaces) {
    qLabel->setText("");

    if (d1 > d2) {
        qLabel->setText(prefix + "<font color=\"green\"><b>" + QString::number(d1, 'f', decimalPlaces) + "</b></font>");
    } else if (d1 < d2) {
        qLabel->setText(prefix + "<font color=\"red\"><b>" + QString::number(d1, 'f', decimalPlaces) + "</b></font>");
    } else {
        qLabel->setText(prefix + QString::number(d1, 'f', decimalPlaces));
    }
}
void PoolBrowser::updateLabel(QLabel* qLabel, double d1, double d2, QString prefix, QString suffix, int decimalPlaces) {
    qLabel->setText("");

    if (d1 > d2) {
        qLabel->setText(prefix + "<font color=\"green\"><b>" + QString::number(d1, 'f', decimalPlaces) + suffix + "</b></font>");
    } else if (d1 < d2) {
        qLabel->setText(prefix + "<font color=\"red\"><b>" + QString::number(d1, 'f', decimalPlaces) + suffix + "</b></font>");
    } else {
        qLabel->setText(prefix + QString::number(d1, 'f', decimalPlaces) + suffix);
    }
}

void PoolBrowser::setModel(ClientModel *model) {
    this->model = model;
}

PoolBrowser::~PoolBrowser() {
    delete ui;
}
