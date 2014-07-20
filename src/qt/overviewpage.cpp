#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "poolbrowser.h"
#include "bitcoingui.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QPicture>
#include <QMovie>
#include <QFrame>

#define DECORATION_SIZE 43
#define NUM_ITEMS 5

class TxViewDelegate : public QAbstractItemDelegate {
    Q_OBJECT
  public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC) {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index) const {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.left() - 8, mainRect.top() - 12, DECORATION_SIZE, DECORATION_SIZE);
        int xspace = DECORATION_SIZE - 8;
        int ypad = 0;
        int halfheight = (mainRect.height() - 2 * ypad) / 2;

        QRect amountRect(mainRect.left() + xspace + 10, mainRect.top(), mainRect.width() - xspace - 10, halfheight);
        QRect addressRect(mainRect.left() + xspace + 130, mainRect.top(), mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);

        if (qVariantCanConvert<QColor>(value)) {
            foreground = qvariant_cast<QColor>(value);
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft | Qt::AlignVCenter, address);

        if (amount < 0) {
            foreground = COLOR_NEGATIVE;
        } else if (!confirmed) {
            foreground = COLOR_UNCONFIRMED;
        } else {
            foreground = option.palette.color(QPalette::Text);
        }

        painter->setPen(foreground);
        QString amountText = "";

        if (convertmode == 0) {
            amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        }

        if (convertmode == 1) {
            amountText = BitcoinUnits::formatWithUnit(unit, (_dScPriceLast * amount), true);
        }

        if (convertmode == 2) {
            amountText = BitcoinUnits::formatWithUnit(unit, (_dScPriceLast / _dBtcPriceLast * amount), true);
        }

        if (!confirmed) {
            amountText = QString("[") + amountText + QString("]");
        }

        painter->drawText(amountRect, Qt::AlignRight | Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft | Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
        return QSize(DECORATION_SIZE, DECORATION_SIZE - 5);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0) {
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->lblDetailsSlot3->setText("out of sync");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    this->setStyleSheet("background-image:url(:/images/background);");
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index) {
    if (filter) {
        emit transactionClicked(filter->mapToSource(index));
    }
}

OverviewPage::~OverviewPage() {
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance) {

    int unit = model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;

    if (convertmode == 0) {
        ui->lblBalanceSlot0->setText(BitcoinUnits::formatWithUnit(unit, balance));
        ui->lblBalanceSlot1->setText(BitcoinUnits::formatWithUnit(unit, stake));
        ui->lblDetailsSlot0->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
        ui->lblBalanceSlot3->setText(BitcoinUnits::formatWithUnit(unit, balance + stake + unconfirmedBalance + immatureBalance));

    } else if (convertmode == 1) {
        ui->lblBalanceSlot0->setText(BitcoinUnits::formatWithUnit(unit, (_dScPriceLast * balance)));
        ui->lblBalanceSlot1->setText(BitcoinUnits::formatWithUnit(unit, (_dScPriceLast * stake)));
        ui->lblDetailsSlot0->setText(BitcoinUnits::formatWithUnit(unit, (_dScPriceLast * unconfirmedBalance)));
        ui->lblBalanceSlot3->setText(BitcoinUnits::formatWithUnit(unit, (_dScPriceLast * (balance + stake + unconfirmedBalance + immatureBalance))));

    } else if (convertmode == 2) {
        ui->lblBalanceSlot0->setText(BitcoinUnits::formatWithUnit(unit, (_dScPriceLast / _dBtcPriceLast * balance)));
        ui->lblBalanceSlot1->setText(BitcoinUnits::formatWithUnit(unit, (_dScPriceLast / _dBtcPriceLast * stake)));
        ui->lblDetailsSlot0->setText(BitcoinUnits::formatWithUnit(unit, (_dScPriceLast / _dBtcPriceLast * unconfirmedBalance)));
        ui->lblBalanceSlot3->setText(BitcoinUnits::formatWithUnit(unit, (_dScPriceLast / _dBtcPriceLast * (balance + stake + unconfirmedBalance + immatureBalance))));
    }
}

void OverviewPage::setNumTransactions(int count) {
    ui->lblDetailsSlot1->setText(QLocale::system().toString(count));
}

void OverviewPage::setModel(WalletModel *model) {
    this->model = model;

    if (model && model->getOptionsModel()) {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));

        setNumTransactions(model->getNumTransactions());
        connect(model, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit() {
    if (model && model->getOptionsModel()) {
        if (currentBalance != -1) {
            setBalance(currentBalance, model->getStake(), currentUnconfirmedBalance, currentImmatureBalance);
        }

        // Update txdelegate->unit with the current unit
        txdelegate->unit = model->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::showOutOfSyncWarning(bool fShow) {
    if (fShow == true) {
        ui->lblDetailsSlot3->setText("<font color=\"red\">Out of sync</font>");
    }

    if (fShow == false) {
        ui->lblDetailsSlot3->setText("<font color=\"color:#495122\">Synced</font>");
    }
}
