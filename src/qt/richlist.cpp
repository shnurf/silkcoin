#include "richlist.h"
#include "ui_richlist.h"
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

//Richlist API
const QString apiRichlist = "http://api.silkcoin.io/wallet/richlist.php";

//HighRollers
HighRollers* _highRollers = new HighRollers();

RichList::RichList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RichList) {
    ui->setupUi(this);

    QObject::connect(&m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseNetworkResponse(QNetworkReply*)), Qt::AutoConnection);

    pollAPIs();
}

void RichList::on_btnRefresh_clicked() {
    pollAPIs();
}

void RichList::pollAPIs() {
    getRequest(apiRichlist);
}

void RichList::processOverview(QNetworkReply* response) {
    try {
        ui->tblRichList->clear();
        ui->tblRichList->setColumnWidth(0, 60);
        ui->tblRichList->setColumnWidth(1, 25);
        ui->tblRichList->setColumnWidth(2, 320);
        ui->tblRichList->setColumnWidth(3, 130);
        ui->tblRichList->setColumnWidth(4, 95);
        ui->tblRichList->setColumnWidth(5, 95);

        ui->tblRichList->setSortingEnabled(false);

        QJsonParseError *error = new QJsonParseError();
        QJsonDocument apiResponse = QJsonDocument::fromJson(response->readAll(), error);
        QJsonObject jsonObject = apiResponse.object();

        int highRollers = 100;

        for (int i = 0; i < highRollers; i++) {
            try {
                const char* c = QString::number(i + 1).toStdString().c_str();
                QJsonObject richList = jsonObject[c].toObject();

                _highRollers->setRank(QString::number(i + 1));
                _highRollers->setAddress(richList["address"].toString());
                _highRollers->setSilk(richList["balance"].toString());
                _highRollers->setBtc(richList["btc"].toString());
                _highRollers->setUsd(richList["usd"].toString());

            } catch (exception) {} //API did not return all needed data so skip this address

            QTreeWidgetItem * qtHighRollers = new QTreeWidgetItem();

            qtHighRollers->setText(0, _highRollers->getRank(QString()));

            qtHighRollers->setIcon(1, QIcon(i < 5 ? ":/icons/gold"
                                         : i < 20 ? ":/icons/silver"
                                         : i < 50 ? ":/icons/bronze"
                                         : "null"));

            qtHighRollers->setToolTip(1, i < 5 ? "Gold"
                                      : i < 20 ? "Silver"
                                      : i < 50 ? "Bronze"
                                      : "");

            qtHighRollers->setText(2, _highRollers->getAddress(QString()));
            qtHighRollers->setText(3, _highRollers->getSilk(QString()));
            qtHighRollers->setText(4, _highRollers->getBtc(QString()));
            qtHighRollers->setText(5, _highRollers->getUsd(QString()));

            ui->tblRichList->addTopLevelItem(qtHighRollers);
        }
    } catch (exception ex) {
        printf("RichList::processTrades: %s\r\n", ex.what());
    }
}

void RichList::getRequest(const QString &urlString) {
    QUrl url(urlString);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    m_nam.get(req);
}

void RichList::parseNetworkResponse(QNetworkReply* response) {
    QUrl apiCall = response->url();

    if (response->error() != QNetworkReply::NoError) {
        //Communication error has occurred
        emit networkError(response->error());
        return;
    }

    if (apiCall == apiRichlist) {
        processOverview(response);
    } else { } //Should NEVER get here unless something went completely awry

    response->deleteLater();
}

void RichList::setModel(ClientModel *model) {
    this->model = model;
}

RichList::~RichList() {
    delete ui;
}
