#include "poll.h"
#include "ui_poll.h"

#include <QWidget>
#include <QMessageBox>

Choices* _choices = new Choices();
Nightcharts* _chart = new Nightcharts();

Poll::Poll(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Poll) {

    ui->setupUi(this);

    QObject::connect(&m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseNetworkResponse(QNetworkReply*)), Qt::AutoConnection);

    //One time primer
    pollAPIs();
}

void Poll::paintEvent(QPaintEvent *e) {
    QWidget::paintEvent(e);
    QPainter painter;

    painter.begin(this);
    _chart->draw(&painter);
    _chart->drawLegend(&painter);
}

void Poll::updatePreview() {
    _donation = ui->nsVotes->value() * getCost(double());

    ui->lblDonationPreview->setText(QString::number(ui->nsVotes->value()).append(QString(" vote(s) will cost ").append(QString::number(_donation)).append(QString(" SILK for this poll."))));
}

void Poll::pollAPIs() {
    getRequest(apiGet);
}

void Poll::parseNetworkResponse(QNetworkReply* response) {
    QUrl apiCall = response->url();

    if (response->error() != QNetworkReply::NoError) {
        //Communication error has occurred
        emit networkError(response->error());
        return;
    }

    if (apiCall == apiGet) {
        getResponse(response);
    } else if (apiCall == apiPost) {
        postResponse(response);
    } else { } //Should NEVER get here unless something went completely awry

    response->deleteLater();
}

void Poll::getRequest(const QString &urlString) {
    QUrl url(urlString);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    m_nam.get(req);
}

void Poll::postRequest() {
    QString raw = QString()
                  .append("{\"vote\":{\"address\": \"")
                  .append(_choiceAddress.toString())
                  .append("\",\"tx\": \"")
                  .append(_txId)
                  .append("\"}}");

    QByteArray byteArray = QByteArray();
    byteArray.append(raw);

    QUrl url(apiPost);
    QNetworkRequest req(url);

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    m_nam.post(req, byteArray);
}

void Poll::getResponse(QNetworkReply *response) {
    QJsonParseError *error = new QJsonParseError();
    QJsonDocument apiResponse = QJsonDocument::fromJson(response->readAll(), error);

    processOverview(apiResponse.object());
}

void Poll::postResponse(QNetworkReply *response) {

    if (response->error() != QNetworkReply::NoError) {
        //Communication error has occurred
        emit networkError(response->error());
        return;
    }

    QString qs = response->readAll();
}

void Poll::processOverview(QJsonObject jsonObject) {
    setId(jsonObject["id"].toDouble());
    setCost(jsonObject["cost"].toDouble());
    setStart(jsonObject["format_start"].toString().toStdString());
    setStop(jsonObject["format_stop"].toString().toStdString());
    setServerTime(jsonObject["server_time"].toInt());
    setName(jsonObject["name"].toString());
    setDescription(jsonObject["description"].toString());

    ui->lblName->setText(QString("You're Voting For: ").append(getName()));
    ui->lblDescription->setText(QString("Description: ").append(getDescription()));
    ui->lblExpiry->setText(QString("Expiry Date/Time: ").append(getStop().toString()));
    ui->lblServerTime->setText(QString("Server Date/Time: ").append(getServerTime().toString()));

    QJsonArray choices = jsonObject["choices"].toArray();
    QJsonArray colors = jsonObject["colours"].toArray();

    ui->cboChoices->clear();
    _totalVotes = 0;

    QObject::disconnect(ui->cboChoices, SIGNAL(currentIndexChanged(int)), this, SLOT(on_cboChoices_currentIndexChanged(int)));

    for(int i = 0; i < choices.count(); i++) {
        QJsonObject choice = choices[i].toObject();

        _choices->setId(choice["id"].toDouble());
        _choices->setName(choice["name"].toString().toStdString());
        _choices->setAddress(choice["address"].toString().toStdString());
        _choices->setTotalAmount(choice["total_amount"].toDouble());

        ui->cboChoices->addItem(_choices->getName(), QVariant(_choices->getAddress()));
        ui->cboChoices->setCurrentIndex(-1);

        _totalVotes += _choices->getTotalAmount(double());
    }

    QObject::connect(ui->cboChoices, SIGNAL(currentIndexChanged(int)), this, SLOT(on_cboChoices_currentIndexChanged(int)));

    _chart->clearPieces();
    _chart->setType(Nightcharts::Pie);
    _chart->setLegendType(Nightcharts::Round);

    _chart->setCords(this->width() / 2 - 125, 50, 250, 175);

    for(int i = 0; i < choices.count(); i++) {
        QJsonObject choice = choices[i].toObject();
        QJsonArray rgb = colors[i].toArray();

        QString name = choice["name"].toString();

        int r = rgb[0].toInt();
        int g = rgb[1].toInt();
        int b = rgb[2].toInt();

        QColor color = QColor(r, g, b);
        float perc = choice["total_amount"].toDouble() / _totalVotes * 100;

        _chart->addPiece(name, color, perc);
    }

    ui->cboChoices->setCurrentIndex(_selectedIndex);

    this->repaint();

    if(getServerTime() > getStop()) {
        ui->btnVote->setToolTip(QString("This poll has expired.  Please try again later."));
        ui->btnVote->setStyleSheet(QString("background:#bfbfbf;color:black;margin:0px;text-align:center;padding:2px;border-radius:3px;font-style:italic;width:100px;font-size:14px;font-family:calibri;font-weight:600;"));
        ui->cboChoices->setEnabled(false);
        ui->btnVote->setEnabled(false);

        return;
    } else {
        ui->btnVote->setToolTip(QString(""));
        ui->btnVote->setStyleSheet(QString("background:#34495e;color:white;margin:0px;text-align:center;padding:2px;border-radius:3px;font-style:italic;width:100px;font-size:14px;font-family:calibri;font-weight:600;"));
        ui->cboChoices->setEnabled(true);
        ui->btnVote->setEnabled(true);
    }
}

void Poll::setModel(WalletModel *model) {
    this->model = model;
}

void Poll::on_btnRefresh_clicked() {
    pollAPIs();
}

void Poll::on_btnVote_clicked() {
    QList<SendCoinsRecipient> recipients;
    SendCoinsRecipient recipient = SendCoinsRecipient();

    _donation = ui->nsVotes->value() * getCost(double());

    recipient.address = _choiceAddress.toString();
    recipient.amount = _donation;
    recipient.amount *= 100000000; //All send transactions are based on Satoshi, not coins
    recipient.label = QString("Silk Poll");

    recipients.append(recipient);

    if (!model) {
        return;
    }

    // Format confirmation message
    QStringList formatted;
    foreach(const SendCoinsRecipient & rcp, recipients) {
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(QString::number(_donation).append(" SILK "), QString("to vote for ").append(ui->cboChoices->currentText()), rcp.address));
    }

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm sending coins"),
                                         tr("Are you sure you want to send %1?").arg(formatted.join(tr(" and "))),
                                         QMessageBox::Yes | QMessageBox::Cancel,
                                         QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) {
        return;    //User Cancelled
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());

    WalletModel::SendCoinsReturn sendstatus;

    if (ctx.isValid()) {
        sendstatus = model->sendCoins(recipients);
    } else {
        return;
    }

    switch (sendstatus.status) {
        case WalletModel::InvalidAddress:
            QMessageBox::warning(this, tr("Send Coins"),
                                 tr("The recipient address is not valid, please recheck."),
                                 QMessageBox::Ok, QMessageBox::Ok);
            break;

        case WalletModel::InvalidAmount:
            QMessageBox::warning(this, tr("Send Coins"),
                                 tr("The amount to pay must be larger than 0."),
                                 QMessageBox::Ok, QMessageBox::Ok);
            break;

        case WalletModel::AmountExceedsBalance:
            QMessageBox::warning(this, tr("Send Coins"),
                                 tr("The amount exceeds your balance."),
                                 QMessageBox::Ok, QMessageBox::Ok);
            break;

        case WalletModel::AmountWithFeeExceedsBalance:
            QMessageBox::warning(this, tr("Send Coins"),
                                 tr("The total exceeds your balance when the %1 transaction fee is included.").
                                 arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, sendstatus.fee)),
                                 QMessageBox::Ok, QMessageBox::Ok);
            break;

        case WalletModel::TransactionCreationFailed:
            QMessageBox::warning(this, tr("Send Coins"),
                                 tr("Error: Transaction creation failed."),
                                 QMessageBox::Ok, QMessageBox::Ok);
            break;

        case WalletModel::TransactionCommitFailed:
            QMessageBox::warning(this, tr("Send Coins"),
                                 tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."),
                                 QMessageBox::Ok, QMessageBox::Ok);
            break;

        case WalletModel::Aborted: // User aborted, nothing to do
            break;

        case WalletModel::OK:
            QMessageBox::information(this, tr("Send Coins"),
                                     QString("You successfully cast ")
                                     .append(QString::number(ui->nsVotes->value()))
                                     .append(QString(" vote(s) ("))
                                     .append(QString::number(_donation))
                                     .append(QString(" SILK)"))
                                     .append(QString(" for "))
                                     .append(ui->cboChoices->currentText()),
                                     QMessageBox::Ok, QMessageBox::Ok);
            break;
    }

    _txId = sendstatus.hex;

    postRequest();
}

void Poll::on_cboChoices_currentIndexChanged(int index) {
    updatePreview();

    try {
        if(index > -1) {
            _choiceAddress = ui->cboChoices->itemData(index);

            _prevChoice = _selectedIndex;
            _selectedIndex = index;

            ui->nsVotes->setValue(_prevChoice != _selectedIndex ? 1 : ui->nsVotes->value());
            ui->nsVotes->setEnabled(true);

        } else {
            _choiceAddress = QVariant(QString(""));
            _donation = 0;

            ui->nsVotes->setEnabled(false);
        }
    } catch (exception ex) {} //Used to prevent exception before user selects a poll choice
}

void Poll::on_nsVotes_valueChanged(int value) {
    updatePreview();
}

Poll::~Poll() {
    delete ui;
}
