#ifndef RICHLIST_H
#define RICHLIST_H

#include "clientmodel.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"

#include <QWidget>
#include <QObject>
#include <QtNetwork/QtNetwork>
#include <QLabel>

namespace Ui {
class RichList;
}

class RichList : public QWidget {
    Q_OBJECT

  public:
    explicit RichList(QWidget *parent = 0);
    ~RichList();

    void setModel(ClientModel* model);

    void pollAPIs();

    void processOverview(QNetworkReply* response);

  private:
    Ui::RichList *ui;
    QNetworkAccessManager m_nam;
    ClientModel* model;

    void getRequest(const QString& url);

  signals:
    void networkError(QNetworkReply::NetworkError err);

  public slots:
    void parseNetworkResponse(QNetworkReply* response);

  private slots:
    void on_btnRefresh_clicked();
};

class HighRollers {
  private:
    QString _rank;
    QString _address;
    QString _silk;
    QString _btc;
    QString _usd;

  public:
    HighRollers() {
        _rank = "";
        _address = "";
        _silk = "";
        _btc = "";
        _usd = "";
    }

    QString getRank(QString) {
        return _rank;
    }
    void setRank(QString value) {
        _rank = value;
    }

    QString getAddress(QString) {
        return _address;
    }
    void setAddress(QString value) {
        _address = value;
    }

    QString getSilk(QString) {
        return _silk;
    }
    void setSilk(QString value) {
        _silk = value;
    }

    QString getBtc(QString) {
        return _btc;
    }
    void setBtc(QString value) {
        _btc = value;
    }

    QString getUsd(QString) {
        return _usd;
    }
    void setUsd(QString value) {
        _usd = value;
    }
};

#endif // RICHLIST_H
