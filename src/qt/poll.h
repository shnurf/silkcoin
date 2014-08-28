#ifndef POLL_H
#define POLL_H

#include "clientmodel.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
//#include "addressbookpage.h"
//#include "optionsmodel.h"
//#include "guiutil.h"
//#include "askpassphrasedialog.h"

//#include "coincontrol.h"
//#include "coincontroldialog.h"

#include "nightcharts.h"

#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QtNetwork/QtNetwork>
#include <QPaintEvent>

namespace Ui {
class Poll;
}

class Poll : public QWidget {
    Q_OBJECT
  private:
    Ui::Poll *ui;
    QNetworkAccessManager m_nam;
    WalletModel* model;

    //GET
    const QString apiGet = "http://poll.silkcoin.io/api/poll";

    //POST
    const QString apiPost = "http://poll.silkcoin.io/api/votes";

    int _selectedIndex = -1;
    int _prevChoice = -1;
    int _donation;

    double _totalVotes;

    QVariant _choiceAddress;
    QString _txId;

    double _id;
    double _cost;
    QString _name;
    QString _description;
    QDateTime _start;
    QDateTime _stop;
    QDateTime _serverTime;

    void pollAPIs();
    void processOverview(QJsonObject jsonObject);

    void getRequest(const QString& url);
    void postRequest();

    void getResponse(QNetworkReply* response);
    void postResponse(QNetworkReply* response);

    void updatePreview(void);

    void paintEvent(QPaintEvent *e);

  public:
    explicit Poll(QWidget *parent = 0);
    ~Poll();

    void setModel(WalletModel* model);

    double getId(double) {
        return _id;
    }
    QString getId(QString) {
        return QString::number(_id, 'f', 0);
    }
    void setId(double value) {
        _id = value;
    }

    QString getName() {
        return _name;
    }
    void setName(QString value) {
        _name = value;
    }

    QString getDescription() {
        return _description;
    }
    void setDescription(QString value) {
        _description = value;
    }

    QDateTime getStart() {
        return _start;
    }
    void setStart(string value) {

        bool isValid = false;
        const uint seconds = QString::fromStdString(value.substr(0, value.find_first_of(".", 0))).toUInt(&isValid);

        if (!isValid) {
            return;
        }

        const QDateTime dt = QDateTime::fromTime_t(seconds);

        _start = dt;
    }

    QDateTime getStop() {
        return _stop;
    }
    void setStop(string value) {
        bool isValid = false;
        const uint seconds = QString::fromStdString(value.substr(0, value.find_first_of(".", 0))).toUInt(&isValid);

        if (!isValid) {
            return;
        }

        const QDateTime dt = QDateTime::fromTime_t(seconds);

        _stop = dt;
    }

    QDateTime getServerTime() {
        return _serverTime;
    }
    void setServerTime(int value) {
        _serverTime = QDateTime::fromTime_t((uint)value);
    }

    double getCost(double) {
        return _cost;
    }
    QString getCost(QString) {
        return QString::number(_cost, 'f', 8);
    }
    void setCost(double value) {
        _cost = value;
    }

  signals:
    void networkError(QNetworkReply::NetworkError err);

  public slots:
    void parseNetworkResponse(QNetworkReply* response);

  private slots:
    void on_btnRefresh_clicked();
    void on_btnVote_clicked();
    void on_cboChoices_currentIndexChanged(int index);
    void on_nsVotes_valueChanged(int value);
};

class Choices {

  private:
    double _id;
    QString _name;
    QString _address;
    double _totalAmount;

  public:
    Choices() {
        _id = 0;
        _name = QString();
        _address = QString();
        _totalAmount = 0;
    }

    double getId(double) {
        return _id;
    }
    QString getId(QString) {
        return QString::number(_id, 'f', 8);
    }
    void setId(double value) {
        _id = value;
    }

    QString getName() {
        return _name;
    }
    void setName(string value) {
        _name = QString::fromStdString(value);
    }

    QString getAddress() {
        return _address;
    }
    void setAddress(string value) {
        _address = QString::fromStdString(value);
    }

    double getTotalAmount(double) {
        return _totalAmount;
    }
    QString getTotalAmount(QString) {
        return QString::number(_totalAmount, 'f', 8);
    }
    void setTotalAmount(double value) {
        _totalAmount = value;
    }
};
#endif // POLL_H
