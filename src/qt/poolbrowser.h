#ifndef POOLBROWSER_H
#define POOLBROWSER_H

#include "clientmodel.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"

#include <QWidget>
#include <QObject>
#include <QtNetwork/QtNetwork>

extern QString _qsBtcPriceCurrent;
extern QString _lastBtcUsd;

namespace Ui {
class PoolBrowser;
}

class ClientModel;

class PoolBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit PoolBrowser(QWidget *parent = 0);
    ~PoolBrowser();
    
    void setModel(ClientModel *model);

private:
    void getRequest(const QString &url);

signals:
    void networkError(QNetworkReply::NetworkError err);

public slots:
    void parseNetworkResponse(QNetworkReply *response);

    //API Call Methods
    void coinbasePrice(QNetworkReply *response);
    void bittrexMarketSummary(QNetworkReply *response);
    void bittrexTrades(QNetworkReply *response);
    void bittrexOrders(QNetworkReply *response);
    void cryptsyTrades(QNetworkReply *response);
    void cryptsyOrders(QNetworkReply *response);
    void mintpalStats(QNetworkReply *response);
    void mintpalTrades(QNetworkReply *response);
    void mintpalSell(QNetworkReply *response);
    void mintpalBuy(QNetworkReply *response);

    void pollApis();

    void openBittrex();
    void openPoloniex();
    void processOverview();

    void egaldo();

private:
    QNetworkAccessManager m_nam;
    Ui::PoolBrowser *ui;
    ClientModel *model;
};

#endif // POOLBROWSER_H
