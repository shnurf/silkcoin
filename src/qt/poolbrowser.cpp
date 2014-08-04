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
#include <QSslSocket>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

using namespace json_spirit;

//Coinbase API
const QString apiCoinbasePrice = "http://api.silkcoin.io/coinbase/exchange_rates.php";

//Bittrex API
const QString apiBittrexMarketSummary = "http://api.silkcoin.io/bittrex/market.php";
const QString apiBittrexTrades = "http://api.silkcoin.io/bittrex/trades.php";
const QString apiBittrexOrders = "http://api.silkcoin.io/bittrex/orders.php";

//Mintpal API
const QString apiMintpalMarketSummary = "http://api.silkcoin.io/mintpal/market.php";
const QString apiMintpalTrades = "http://api.silkcoin.io/mintpal/trades.php";
const QString apiMintpalOrdersSell = "http://api.silkcoin.io/mintpal/orders_sell.php";
const QString apiMintpalOrdersBuy = "http://api.silkcoin.io/mintpal/orders_buy.php";

//Poloniex API
const QString apiPoloniexMarketSummary = "http://api.silkcoin.io/poloniex/market.php";
const QString apiPoloniexTrades = "http://api.silkcoin.io/poloniex/trades.php";
const QString apiPoloniexOrders = "http://api.silkcoin.io/poloniex/orders.php";
const QString apiPoloniexVolume = "http://api.silkcoin.io/poloniex/volume.php";

//Cryptsy API
const QString apiCryptsyTrades = "http://api.silkcoin.io/cryptsy/trades.php";
const QString apiCryptsyOrders = "http://api.silkcoin.io/cryptsy/orders.php";

//Common Globals
double _dScPriceLast = 0;
double _dBtcPriceCurrent = 0;
double _dBtcPriceLast = 0;

//Bittrex Globals
BittrexMarketSummary* _bittrexMarketSummary = new BittrexMarketSummary();
BittrexTrades* _bittrexTrades = new BittrexTrades();
BittrexOrders* _bittrexOrders = new BittrexOrders();

//Mintpal Globals
MintpalMarketSummary* _mintpalMarketSummary = new MintpalMarketSummary();
MintpalTrades* _mintpalTrades = new MintpalTrades();
MintpalOrders* _mintpalOrders = new MintpalOrders();
QStringList _mintpalApiResponseOrdersSell;

//Poloniex Globals
PoloniexMarketSummary* _poloniexMarketSummary = new PoloniexMarketSummary();
PoloniexTrades* _poloniexTrades = new PoloniexTrades();
PoloniexOrders* _poloniexOrders = new PoloniexOrders();

//Cryptsy Globals
CryptsyTrades* _cryptsyTrades = new CryptsyTrades();
CryptsyOrders* _cryptsyOrders = new CryptsyOrders();

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

    ui->qCustomPlotPoloniexTrades->addGraph();
    ui->qCustomPlotPoloniexTrades->setBackground(QBrush(QColor("#edf1f7")));

    ui->qCustomPlotPoloniexOrderDepth->addGraph();
    ui->qCustomPlotPoloniexOrderDepth->addGraph();
    ui->qCustomPlotPoloniexOrderDepth->setBackground(QBrush(QColor("#edf1f7")));

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

void PoolBrowser::on_lblBittrexMarketLink_linkActivated(const QString &link) {
    QDesktopServices::openUrl(QUrl(link));
}
void PoolBrowser::on_lblCryptsyMarketLink_linkActivated(const QString &link) {
    QDesktopServices::openUrl(QUrl(link));
}
void PoolBrowser::on_lblMintpalMarketLink_linkActivated(const QString &link) {
    QDesktopServices::openUrl(QUrl(link));
}

void PoolBrowser::pollAPIs() {
    ui->iconOverviewUpdateWait->setVisible(true);

    getRequest(apiCoinbasePrice);

    getRequest(apiBittrexMarketSummary);
    getRequest(apiBittrexTrades);
    getRequest(apiBittrexOrders);

    getRequest(apiMintpalMarketSummary);
    getRequest(apiMintpalTrades);
    getRequest(apiMintpalOrdersSell);
    getRequest(apiMintpalOrdersBuy);

    getRequest(apiPoloniexMarketSummary);
    getRequest(apiPoloniexTrades);
    getRequest(apiPoloniexOrders);
    getRequest(apiPoloniexVolume);

    getRequest(apiCryptsyTrades);
    getRequest(apiCryptsyOrders);
}

void PoolBrowser::processOverview() {
    double averageBittrexCurrent = _bittrexMarketSummary->getLastCurrent(double()) > 0 ? _bittrexMarketSummary->getLastCurrent(double()) : 0.00000001;
    double averageMintpalCurrent = _mintpalMarketSummary->getLastCurrent(double()) > 0 ? _mintpalMarketSummary->getLastCurrent(double()) : 0.00000001;
    double averagePoloniexCurrent = _poloniexMarketSummary->getLastCurrent(double()) > 0 ? _poloniexMarketSummary->getLastCurrent(double()) : 0.00000001;
    double averageCryptsyCurrent = _cryptsyTrades->getLastCurrent(double()) > 0 ? _cryptsyTrades->getLastCurrent(double()) : 0.00000001;
    double averageAllCurrent = (averageBittrexCurrent + averageCryptsyCurrent + averageMintpalCurrent + averagePoloniexCurrent) / 4;

    double averageBittrexLast = _bittrexMarketSummary->getLastPrev(double()) > 0 ? _bittrexMarketSummary->getLastPrev(double()) : 0.00000001;
    double averageMintpalLast = _mintpalMarketSummary->getLastPrev(double()) > 0 ? _mintpalMarketSummary->getLastPrev(double()) : 0.00000001;
    double averagePoloniexLast = _poloniexMarketSummary->getLastPrev(double()) > 0 ? _poloniexMarketSummary->getLastPrev(double()) : 0.00000001;
    double averageCryptsyLast = _cryptsyTrades->getLastPrev(double()) > 0 ? _cryptsyTrades->getLastPrev(double()) : 0.00000001;

    double averageAllLast = (averageBittrexLast + averageCryptsyLast + averageMintpalLast + averagePoloniexLast) / 4;

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

    updateLabel(ui->lblOverviewPoloniexBtc,
                _poloniexMarketSummary->getLastCurrent(double()),
                _poloniexMarketSummary->getLastPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblOverviewPoloniexPerc,
                (_poloniexMarketSummary->getLastCurrent(double()) - averageAllCurrent) / averageAllCurrent * 100,
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
    } else if (apiCall == apiMintpalMarketSummary) {
        mintpalMarketSummary(response);
    } else if (apiCall == apiMintpalTrades) {
        mintpalTrades(response);
    } else if (apiCall == apiMintpalOrdersSell) {
        mintpalOrdersSell(response);
    } else if (apiCall == apiMintpalOrdersBuy) {
        mintpalOrdersBuy(response);
    } else if (apiCall == apiPoloniexMarketSummary) {
        poloniexMarketSummary(response);
    } else if (apiCall == apiPoloniexTrades) {
        poloniexTrades(response);
    } else if (apiCall == apiPoloniexOrders) {
        poloniexOrders(response);
    } else if (apiCall == apiPoloniexVolume) {
        poloniexVolume(response);
    } else if (apiCall == apiCryptsyTrades) {
        cryptsyTrades(response);
    } else if (apiCall == apiCryptsyOrders) {
        cryptsyOrders(response);
    } else { } //Should NEVER get here unless something went completely awry

    if (_bittrexMarketSummary->getLastPrev(double()) > 0 &&
            _cryptsyTrades->getLastCurrent(double()) > 0 &&
            _mintpalMarketSummary->getLastCurrent(double()) > 0 &&
            _poloniexMarketSummary->getLastCurrent(double()) > 0) {
        ui->iconOverviewUpdateWait->setVisible(false);
    }

    processOverview();

    response->deleteLater();
}

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
void PoolBrowser::mintpalOrdersSell(QNetworkReply* response) {
    QString apiResponse = response->readAll();

    apiResponse = apiResponse.replace("]}" , "");
    QStringList qslApiResponse = apiResponse.split("\"orders\":[");
    _mintpalApiResponseOrdersSell = qslApiResponse[1].replace("},{", "}{").split("{", QString::SkipEmptyParts);
}
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

void PoolBrowser::poloniexMarketSummary(QNetworkReply* response) {

    QJsonParseError *error = new QJsonParseError();
    QJsonDocument apiResponse = QJsonDocument::fromJson(response->readAll(), error);
    QJsonObject jsonObject = apiResponse.object();

    QJsonObject scMarket = jsonObject["BTC_SC"].toObject();

    _poloniexMarketSummary->setLastCurrent(scMarket["last"].toString());
    _poloniexMarketSummary->setAskCurrent(scMarket["lowestAsk"].toString());
    _poloniexMarketSummary->setBidCurrent(scMarket["highestBid"].toString());
    _poloniexMarketSummary->setPercentChangeCurrent(scMarket["percentChange"].toString());

    updateLabel(ui->lblPoloniexChangePerc,
                _poloniexMarketSummary->getPercentChangeCurrent(double()),
                _poloniexMarketSummary->getPercentChangePrev(double()),
                QString(""),
                QString("%"),
                2);

    updateLabel(ui->lblPoloniexVolumeUsd,
                _poloniexMarketSummary->getBaseVolumeCurrent(double()) * _dBtcPriceCurrent,
                _poloniexMarketSummary->getBaseVolumePrev(double()) * _dBtcPriceCurrent,
                QString(""),
                2);

    updateLabel(ui->lblPoloniexVolumeSc,
                _poloniexMarketSummary->getVolumeCurrent(double()),
                _poloniexMarketSummary->getVolumePrev(double()),
                QString(""),
                4);

    updateLabel(ui->lblPoloniexVolumeBtc,
                _poloniexMarketSummary->getBaseVolumeCurrent(double()),
                _poloniexMarketSummary->getBaseVolumePrev(double()),
                QString(""),
                4);

    updateLabel(ui->lblPoloniexLastBtc,
                _poloniexMarketSummary->getLastCurrent(double()),
                _poloniexMarketSummary->getLastPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblPoloniexLastUsd,
                _poloniexMarketSummary->getLastCurrent(double()) * _dBtcPriceCurrent,
                _poloniexMarketSummary->getLastPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    updateLabel(ui->lblPoloniexAskBtc,
                _poloniexMarketSummary->getAskCurrent(double()),
                _poloniexMarketSummary->getAskPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblPoloniexAskUsd,
                _poloniexMarketSummary->getAskCurrent(double()) * _dBtcPriceCurrent,
                _poloniexMarketSummary->getAskPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    updateLabel(ui->lblPoloniexBidBtc,
                _poloniexMarketSummary->getBidCurrent(double()),
                _poloniexMarketSummary->getBidPrev(double()),
                QString("B"),
                8);

    updateLabel(ui->lblPoloniexBidUsd,
                _poloniexMarketSummary->getBidCurrent(double()) * _dBtcPriceCurrent,
                _poloniexMarketSummary->getBidPrev(double()) * _dBtcPriceCurrent,
                QString("$"),
                8);

    _poloniexMarketSummary->setAskPrev(_poloniexMarketSummary->getAskCurrent(double()));
    _poloniexMarketSummary->setBaseVolumePrev(_poloniexMarketSummary->getBaseVolumeCurrent(double()));
    _poloniexMarketSummary->setBidPrev(_poloniexMarketSummary->getBidCurrent(double()));
    _poloniexMarketSummary->setHighPrev(_poloniexMarketSummary->getHighCurrent(double()));
    _poloniexMarketSummary->setLowPrev(_poloniexMarketSummary->getLowCurrent(double()));
    _poloniexMarketSummary->setPrevDayPrev(_poloniexMarketSummary->getPrevDayCurrent(double()));
    _poloniexMarketSummary->setLastPrev(_poloniexMarketSummary->getLastCurrent(double()));
    _poloniexMarketSummary->setVolumePrev(_poloniexMarketSummary->getVolumeCurrent(double()));

    _dScPriceLast = _dBtcPriceCurrent * _poloniexMarketSummary->getLastCurrent(double());
}
void PoolBrowser::poloniexTrades(QNetworkReply* response) {
    int z = 0;
    double high = 0;
    double low = 100000;

    ui->tblPoloniexTrades->clear();
    ui->tblPoloniexTrades->setColumnWidth(0, 60);
    ui->tblPoloniexTrades->setColumnWidth(1, 110);
    ui->tblPoloniexTrades->setColumnWidth(2, 110);
    ui->tblPoloniexTrades->setColumnWidth(3, 100);
    ui->tblPoloniexTrades->setColumnWidth(4, 160);
    ui->tblPoloniexTrades->setSortingEnabled(false);

    QJsonParseError *error = new QJsonParseError();
    QJsonDocument apiResponse = QJsonDocument::fromJson(response->readAll(), error);
    QJsonArray jsonArray = apiResponse.array();

    int tradeCount = jsonArray.size();
    QVector<double> xAxis(tradeCount), yAxis(tradeCount);

    for (int i = 0; i < tradeCount; i++) {
        try {
            QJsonObject trade = jsonArray[i].toObject();

            _poloniexTrades->setId(trade["tradeID"].toString());
            _poloniexTrades->setTimeStamp(trade["date"].toString());
            _poloniexTrades->setOrderType(trade["type"].toString().toStdString());
            _poloniexTrades->setPrice(trade["rate"].toString());
            _poloniexTrades->setQuantity(trade["amount"].toString());
            _poloniexTrades->setTotal(trade["total"].toString());

        } catch (exception) {} //API did not return all needed data so skip this trade

        QTreeWidgetItem * qtTrades = new QTreeWidgetItem();

        qtTrades->setText(0, _poloniexTrades->getOrderType());
        qtTrades->setText(1, _poloniexTrades->getPrice(QString()));
        qtTrades->setText(2, _poloniexTrades->getQuantity(QString()));
        qtTrades->setText(3, _poloniexTrades->getTotal(QString()));
        qtTrades->setText(4, _poloniexTrades->getTimeStamp());

        ui->tblPoloniexTrades->addTopLevelItem(qtTrades);

        xAxis[z] = tradeCount - z;
        yAxis[z] = _poloniexTrades->getPrice(double()) * 100000000;

        high = _poloniexTrades->getPrice(double()) > high ? _poloniexTrades->getPrice(double()) : high;
        low = _poloniexTrades->getPrice(double()) < low ? _poloniexTrades->getPrice(double()) : low;

        z++;
    }

    high *=  100000000;
    low *=  100000000;

    ui->qCustomPlotPoloniexTrades->graph(0)->setData(xAxis, yAxis);
    ui->qCustomPlotPoloniexTrades->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotPoloniexTrades->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));

    ui->qCustomPlotPoloniexTrades->xAxis->setRange(1, tradeCount);
    ui->qCustomPlotPoloniexTrades->yAxis->setRange(low, high);

    ui->qCustomPlotPoloniexTrades->replot();

}
void PoolBrowser::poloniexOrders(QNetworkReply* response) {
    int z = 0;
    double high = 0;
    double low = 100000;
    double sumBuys = 0;
    double sumSells = 0;
    double sumHighest = 0;

    ui->qTreeWidgetPoloniexBuy->clear();
    ui->qTreeWidgetPoloniexBuy->sortByColumn(0, Qt::DescendingOrder);
    ui->qTreeWidgetPoloniexBuy->setSortingEnabled(true);

    ui->qTreeWidgetPoloniexSell->clear();
    ui->qTreeWidgetPoloniexSell->sortByColumn(0, Qt::AscendingOrder);
    ui->qTreeWidgetPoloniexSell->setSortingEnabled(true);

    QJsonParseError *error = new QJsonParseError();
    QJsonDocument apiResponse = QJsonDocument::fromJson(response->readAll(), error);
    QJsonObject jsonObject = apiResponse.object();
    QJsonArray jsonValueAsks = jsonObject["asks"].toArray();
    QJsonArray jsonValueBids = jsonObject["bids"].toArray();

    //Use shorest depth as limit and use bid size if they are the same
    int depth = jsonValueBids.size() > jsonValueAsks.size()
                ? jsonValueAsks.size() : jsonValueAsks.size() > jsonValueBids.size()
                ? jsonValueBids.size() : jsonValueBids.size();

    //Prevent overflow by limiting depth to 50
    //Also check for odd number of orders and drop the last one
    //To avoid an overflow when there are less than 50 orders
    depth = depth > 50
            ? 50 : depth % 2 == 1
            ? depth - 1 : depth;

    QVector<double> xAxisBuys(depth), yAxisBuys(depth);
    QVector<double> xAxisSells(depth), yAxisSells(depth);

    for (int i = 0; i < depth; i++) {
        QJsonArray asks = jsonValueAsks[i].toArray();

        _poloniexOrders->setPrice(asks[0].toDouble());
        _poloniexOrders->setQuantity(asks[1].toDouble());
        _poloniexOrders->setOrderType("Sell");

        QTreeWidgetItem * qtSells = new QTreeWidgetItem();

        qtSells->setText(0, _poloniexOrders->getPrice(QString()));
        qtSells->setText(1, _poloniexOrders->getQuantity(QString()));

        ui->qTreeWidgetPoloniexSell->addTopLevelItem(qtSells);

        sumSells += _poloniexOrders->getQuantity(double());
        xAxisSells[z] = _poloniexOrders->getPrice(double()) * 100000000;
        yAxisSells[z] = sumSells;

        high = _poloniexOrders->getPrice(double()) > high ? _poloniexOrders->getPrice(double()) : high;
        low = _poloniexOrders->getPrice(double()) < low ? _poloniexOrders->getPrice(double()) : low;

        QJsonArray bids = jsonValueBids[i].toArray();

        _poloniexOrders->setPrice(bids[0].toDouble());
        _poloniexOrders->setQuantity(bids[1].toDouble());
        _poloniexOrders->setOrderType("Buy");

        QTreeWidgetItem * qtBuys = new QTreeWidgetItem();

        qtBuys->setText(0, _poloniexOrders->getPrice(QString()));
        qtBuys->setText(1, _poloniexOrders->getQuantity(QString()));

        ui->qTreeWidgetPoloniexBuy->addTopLevelItem(qtBuys);

        sumBuys += _poloniexOrders->getQuantity(double());
        xAxisBuys[z] = _poloniexOrders->getPrice(double()) * 100000000;
        yAxisBuys[z] = sumBuys;

        high = _poloniexOrders->getPrice(double()) > high ? _poloniexOrders->getPrice(double()) : high;
        low = _poloniexOrders->getPrice(double()) < low ? _poloniexOrders->getPrice(double()) : low;

        z++;
    }

    high *=  100000000;
    low *=  100000000;

    sumHighest = sumBuys > sumSells ? sumBuys : sumBuys < sumSells ? sumSells : sumBuys;

    ui->qCustomPlotPoloniexOrderDepth->graph(0)->setData(xAxisBuys, yAxisBuys);
    ui->qCustomPlotPoloniexOrderDepth->graph(1)->setData(xAxisSells, yAxisSells);

    ui->qCustomPlotPoloniexOrderDepth->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    ui->qCustomPlotPoloniexOrderDepth->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    ui->qCustomPlotPoloniexOrderDepth->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    ui->qCustomPlotPoloniexOrderDepth->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    ui->qCustomPlotPoloniexOrderDepth->xAxis->setRange(low, high);
    ui->qCustomPlotPoloniexOrderDepth->yAxis->setRange(low, sumHighest);

    ui->qCustomPlotPoloniexOrderDepth->replot();
}
void PoolBrowser::poloniexVolume(QNetworkReply *response) {
    QJsonParseError *error = new QJsonParseError();
    QJsonDocument apiResponse = QJsonDocument::fromJson(response->readAll(), error);
    QJsonObject jsonObject = apiResponse.object();

    QJsonObject scMarket = jsonObject["BTC_SC"].toObject();

    _poloniexMarketSummary->setBaseVolumeCurrent(scMarket["BTC"].toString());
    _poloniexMarketSummary->setVolumeCurrent(scMarket["SC"].toString());

    updateLabel(ui->lblPoloniexVolumeUsd,
                _poloniexMarketSummary->getBaseVolumeCurrent(double()) * _dBtcPriceCurrent,
                _poloniexMarketSummary->getBaseVolumePrev(double()) * _dBtcPriceCurrent,
                QString(""),
                2);

    updateLabel(ui->lblPoloniexVolumeSc,
                _poloniexMarketSummary->getVolumeCurrent(double()),
                _poloniexMarketSummary->getVolumePrev(double()),
                QString(""),
                4);

    updateLabel(ui->lblPoloniexVolumeBtc,
                _poloniexMarketSummary->getBaseVolumeCurrent(double()),
                _poloniexMarketSummary->getBaseVolumePrev(double()),
                QString(""),
                4);

    _poloniexMarketSummary->setBaseVolumePrev(_poloniexMarketSummary->getBaseVolumeCurrent(double()));
    _poloniexMarketSummary->setVolumePrev(_poloniexMarketSummary->getVolumeCurrent(double()));
}

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
