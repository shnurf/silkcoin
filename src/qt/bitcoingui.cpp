/*
* Qt4 bitcoin GUI.
*
* W.J. van der Laan 2011-2012
* The Bitcoin Developers 2011-2012
*/
#include "bitcoingui.h"
#include "transactiontablemodel.h"
#include "addressbookpage.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "tutoStackDialog.h"
#include "tutoWriteDialog.h"
#include "transactiondescdialog.h"
#include "addresstablemodel.h"
#include "transactionview.h"
#include "overviewpage.h"
#include "statisticspage.h"
#include "blockbrowser.h"
#include "poolbrowser.h"
#include "richlist.h"
#include "poll.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "notificator.h"
#include "guiutil.h"
#include "rpcconsole.h"
#include "wallet.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QStackedWidget>
#include <QDateTime>
#include <QMovie>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QStyle>
#include <QFont>
#include <QFontDatabase>

#include <iostream>

extern CWallet* pwalletMain;
extern int64_t nLastCoinStakeSearchInterval;
extern unsigned int nTargetSpacing;
double GetPoSKernelPS();
int convertmode = 0;

BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent),
    clientModel(0),
    walletModel(0),
    encryptWalletAction(0),
    changePassphraseAction(0),
    actionLockUnlockWallet_ActionScreen(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0) {
    setFixedSize(1000, 534);
    setWindowFlags(Qt::FramelessWindowHint);
    QFontDatabase::addApplicationFont(":/fonts/preg");
    QFontDatabase::addApplicationFont(":/fonts/pxbold");
    QFontDatabase::addApplicationFont(":/fonts/mohave");
    setWindowTitle(tr("Silkcoin") + " " + tr("Wallet"));
    qApp->setStyleSheet("QMainWindow { background:rgb(237, 241, 247); font-family:'Proxima Nova Rg'; } #toolbar2 { border:none;width:30px; background:rgb(52, 56, 65); }");
#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":icons/bitcoin"));
    setWindowIcon(QIcon(":icons/bitcoin"));
#else
    setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    createActions();

    // Create the toolbars
    createToolBars();

    // Create the tray icon (or setup the dock icon)
    createTrayIcon();

    // Create tabs
    overviewPage = new OverviewPage(this);
    statisticsPage = new StatisticsPage(this);

    blockBrowser = new BlockBrowser(this);
    poolBrowser = new PoolBrowser(this);
    richList = new RichList(this);
    poll = new Poll(this);

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    transactionView = new TransactionView(this);
    QLabel *mylabel2 = new QLabel(this);
    mylabel2->show();
    mylabel2->setText("<html><head/><body><p><img src=\":/icons/res/icons/transact.png\"/></p></body></html>");
    mylabel2->setAlignment(Qt::AlignHCenter);
    mylabel2->setStyleSheet("margin-bottom:20px;");
    vbox->addWidget(mylabel2);
    vbox->addWidget(transactionView);
    vbox->setContentsMargins(52, 21, 22, 10);
    transactionsPage->setLayout(vbox);

    //Create sett toolbar
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    QWidget* spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    QWidget* spacer3 = new QWidget();
    spacer3->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    spacer3->setMinimumHeight(20);
    QWidget* spacer4 = new QWidget();
    spacer4->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    spacer4->setMinimumHeight(20);

    gifCamelUnlocked = new QMovie(":/movies/camel_unlocked", "gif", this);
    gifCamelLocked = new QMovie(":/movies/camel_locked", "gif", this);
    lblCamelTraveling = new QLabel(this);
    lblCamelTraveling->setMovie(gifCamelLocked);
    lblCamelTraveling->show();
    gifCamelLocked->start();
    gifCamelLocked->stop();

    QToolBar *toolbar3 = addToolBar(tr("Settings2"));
    toolbar3->addWidget(spacer);
    toolbar3->addWidget(lblCamelTraveling);
    toolbar3->addAction(actionLockUnlockWallet_ActionScreen);
    toolbar3->addWidget(spacer2);
    QToolBar *toolbar5 = addToolBar(tr("Settings4"));
    toolbar5->addWidget(spacer3);
    toolbar5->addAction(encryptWalletAction);
    toolbar5->addAction(signMessageAction);
    toolbar5->addAction(tutoStackAction);
    toolbar5->addAction(aboutAction);
    QToolBar *toolbar6 = addToolBar(tr("Settings4"));
    toolbar6->addWidget(spacer4);
    toolbar6->addAction(backupWalletAction);
    toolbar6->addAction(verifyMessageAction);
    toolbar6->addAction(changePassphraseAction);
    toolbar6->addAction(openRPCConsoleAction);
    toolbar3->setObjectName("toolbar3");
    toolbar3->setMovable(false);
    toolbar3->setOrientation(Qt::Vertical);
    toolbar3->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar5->setObjectName("toolbar5");
    toolbar5->setMovable(false);
    toolbar5->setOrientation(Qt::Vertical);
    toolbar5->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar6->setObjectName("toolbar6");
    toolbar6->setMovable(false);
    toolbar6->setOrientation(Qt::Vertical);
    toolbar6->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar3->setIconSize((QSize(164, 96)));
    toolbar5->setIconSize((QSize(164, 96)));
    toolbar6->setIconSize((QSize(164, 96)));
    settingsPage = new QWidget(this);
    QHBoxLayout *vbox2 = new QHBoxLayout();

    vbox2->addWidget(toolbar3);
    vbox2->addWidget(toolbar5);
    vbox2->addWidget(toolbar6);
    vbox2->setAlignment(Qt::AlignCenter);
    vbox2->setSpacing(30);
    settingsPage->setStyleSheet("QToolBar { border:none; } QToolBar QToolButton { border:none;height:110px; }");

    settingsPage->setLayout(vbox2);

    addressBookPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab);

    receiveCoinsPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab);

    sendCoinsPage = new SendCoinsDialog(this);

    signVerifyMessageDialog = new SignVerifyMessageDialog(this);

    centralWidget = new QStackedWidget(this);
    centralWidget->addWidget(overviewPage);
    centralWidget->addWidget(statisticsPage);
    centralWidget->addWidget(blockBrowser);
    centralWidget->addWidget(poolBrowser);
    centralWidget->addWidget(richList);
    centralWidget->addWidget(poll);
    centralWidget->addWidget(transactionsPage);
    centralWidget->addWidget(addressBookPage);
    centralWidget->addWidget(sendCoinsPage);
    centralWidget->addWidget(receiveCoinsPage);
    centralWidget->addWidget(settingsPage);

    setCentralWidget(centralWidget);

    // Status bar notification icons
    labelStakingIcon = new QLabel();
    labelStakingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, 44));
    labelConnectionsIcon = new QLabel();
    labelConnectionsIcon->setPixmap(QIcon(":/icons/connect_0").pixmap(STATUSBAR_ICONSIZE, 44));
    lblBlockStatus = new QLabel();
    lblBlockStatus->setPixmap(QIcon(":/icons/connecting").pixmap(STATUSBAR_ICONSIZE, 44)); //Initialize with 'searching' icon so people with slow connections see something
    lblBlockStatus->setToolTip("Looking for more network connections");
    actionConvertCurrency = new QAction(QIcon(":/icons/sctask"), tr(""), this);
    actionConvertCurrency->setToolTip("Click here to convert your Silkcoin to USD and BTC.  Silkcoin price is pegged to BTC/SILK market and BTC/USD market on Coinbase.");
    actionLockUnlockWallet_Toolbar = new QAction(QIcon(":/icons/lock_closed"), tr(""), this);
    actionHowToStake = new QAction(QIcon(":/icons/help"), tr(""), this);
    actionHowToStake->setToolTip("If you need some help on how to start staking, click here for a short tutorial");

    actionFacebook = new QAction(QIcon(":/icons/facebook"), tr("Like us on Facebook!"), this);
    actionTwitter = new QAction(QIcon(":/icons/twitter"), tr("Follow us on Twitter!"), this);
    actionReddit = new QAction(QIcon(":/icons/reddit"), tr("Check us out on Reddit!"), this);

    if (GetBoolArg("-staking", true)) {
        QTimer *timerStakingIcon = new QTimer(labelStakingIcon);
        connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingIcon()));
        timerStakingIcon->start(5 * 1000);
        updateStakingIcon();
    }

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    addToolBarBreak(Qt::LeftToolBarArea);
    QToolBar *toolbar2 = addToolBar(tr("Toolbar"));
    addToolBar(Qt::LeftToolBarArea, toolbar2);
    toolbar2->setOrientation(Qt::Vertical);
    toolbar2->setMovable(false);
    toolbar2->setObjectName("toolbar2");
    toolbar2->setFixedWidth(30);
    toolbar2->setIconSize(QSize(28, 28));
    toolbar2->addAction(actionConvertCurrency);
    toolbar2->addAction(actionLockUnlockWallet_Toolbar);
    toolbar2->addWidget(labelStakingIcon);
    toolbar2->addWidget(labelConnectionsIcon);
    toolbar2->addWidget(lblBlockStatus);
    toolbar2->addAction(actionTwitter);
    toolbar2->addAction(actionFacebook);
    toolbar2->addAction(actionReddit);
    toolbar2->addAction(actionHowToStake);
    toolbar2->setStyleSheet("#toolbar2 QToolButton { border:none;padding:0px;margin:0px;height:20px;width:28px;margin-top:20px; }");

    gifSyncing = new QMovie(":/movies/spinner", "gif", this);
    gifSyncing->setCacheMode(QMovie::CacheAll);

    connect(actionConvertCurrency, SIGNAL(triggered()), this, SLOT(sConvert()));

    connect(actionFacebook, SIGNAL(triggered()), this, SLOT(openFacebook()));
    connect(actionTwitter, SIGNAL(triggered()), this, SLOT(openTwitter()));
    connect(actionReddit, SIGNAL(triggered()), this, SLOT(openReddit()));

    connect(actionLockUnlockWallet_Toolbar, SIGNAL(trigered()), this, SLOT(lockUnlockWallet()));
    connect(actionHowToStake, SIGNAL(triggered()), this, SLOT(tutoStackClicked()));

    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), this, SLOT(gotoHistoryPage()));
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    rpcConsole = new RPCConsole(this);
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));
    connect(openRPCConsoleAction2, SIGNAL(triggered()), rpcConsole, SLOT(show()));

    // Clicking on "Verify Message" in the address book sends you to the verify message tab
    connect(addressBookPage, SIGNAL(verifyMessage(QString)), this, SLOT(gotoVerifyMessageTab(QString)));
    // Clicking on "Sign Message" in the receive coins page sends you to the sign message tab
    connect(receiveCoinsPage, SIGNAL(signMessage(QString)), this, SLOT(gotoSignMessageTab(QString)));

    gotoOverviewPage();
}

BitcoinGUI::~BitcoinGUI() {
    if (trayIcon) { // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
    }
}

void BitcoinGUI::createActions() {
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Dashboard"), this);
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    statisticsAction = new QAction(QIcon(":/icons/statistics"), tr("&Statistics"), this);
    statisticsAction->setCheckable(true);
    tabGroup->addAction(statisticsAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    addressBookAction = new QAction(QIcon(":/icons/address-book"), tr("&Address Book"), this);
    addressBookAction->setCheckable(true);
    addressBookAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    tabGroup->addAction(addressBookAction);

    blockAction = new QAction(QIcon(":/icons/block"), tr("&Block Explorer"), this);
    blockAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    blockAction->setCheckable(true);
    tabGroup->addAction(blockAction);

    poolAction = new QAction(QIcon(":/icons/ex"), tr("&Market Data"), this);
    poolAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_7));
    poolAction->setCheckable(true);
    tabGroup->addAction(poolAction);

    richListAction = new QAction(QIcon(":/icons/richlist"), tr("&Rich List"), this);
    richListAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R));
    richListAction->setCheckable(true);
    tabGroup->addAction(richListAction);

    pollAction = new QAction(QIcon(":/icons/poll"), tr("&Polls"), this);
    pollAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_V));
    pollAction->setCheckable(true);
    tabGroup->addAction(pollAction);

    settingsAction = new QAction(QIcon(":/icons/actions"), tr("&Actions"), this);
    settingsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_7));
    settingsAction->setCheckable(true);
    tabGroup->addAction(settingsAction);

    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Settings"), this);
    tabGroup->addAction(optionsAction);

    QToolBar *toolbarsend = addToolBar(tr("Send"));
    QToolBar *toolbarsend2 = addToolBar(tr("Receive"));

    sendCoinsAction = new QAction(QIcon(":/icons/null"), tr("&Send Silk"), this);
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    toolbarsend2->addAction(sendCoinsAction);
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/null"), tr("&Receive Silk"), this);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    receiveCoinsAction->setCheckable(true);
    toolbarsend->addAction(receiveCoinsAction);
    tabGroup->addAction(receiveCoinsAction);

    actionSendReceive = new QAction(QIcon(":/icons/send"), tr("&Send / Receive"), this);
    actionSendReceive->setCheckable(true);
    actionSendReceive->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));

    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(actionSendReceive, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(actionSendReceive, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));

    toolbarsend->setIconSize(QSize(396, 46));
    toolbarsend2->setIconSize(QSize(400, 46));
    toolbarsend->setStyleSheet("QToolBar {border:none;} QToolBar QToolButton { background-image:url(:/icons/res/icons/receiveg.png);height:46;width:396px;border:none; } QToolBar QToolButton:checked { background-image:url(:/icons/res/icons/receiver.png);border:none; } QToolBar QToolButton:hover { background-image:url(:/icons/res/icons/receiver.png);border:none; }");
    toolbarsend2->setStyleSheet("QToolBar {border:none;} QToolBar QToolButton { background-image:url(:/icons/res/icons/sendg.png);height:46;width:396px;border:none; } QToolBar QToolButton:checked { background-image:url(:/icons/res/icons/sendr.png);border:none; } QToolBar QToolButton:hover { background-image:url(:/icons/res/icons/sendr.png);border:none; }");
    wId2 = new QWidget(this);
    wId2->setContentsMargins(0, 0, 0, 0);
    toolbarsend->setFixedSize(396, 46);
    toolbarsend2->setFixedSize(400, 46);
    QHBoxLayout *vbox4 = new QHBoxLayout();
    vbox4->setContentsMargins(0, 0, 0, 0);
    vbox4->setSpacing(0);
    vbox4->addWidget(toolbarsend2);
    vbox4->addWidget(toolbarsend);
    wId2->setFixedSize(794, 46);
    wId2->move(207, -1);
    wId2->setLayout(vbox4);
    wId2->setFocus();
    wId2->hide();

    connect(blockAction, SIGNAL(triggered()), this, SLOT(gotoBlockBrowser()));
    connect(poolAction, SIGNAL(triggered()), this, SLOT(gotoPoolBrowser()));
    connect(richListAction, SIGNAL(triggered()), this, SLOT(gotoRichList()));
    connect(pollAction, SIGNAL(triggered()), this, SLOT(gotoPoll()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(statisticsAction, SIGNAL(triggered()), this, SLOT(gotoStatisticsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(gotoSettingsPage()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));

    QActionGroup *appMenuBar = new QActionGroup(this);

    quitAction = new QAction(QIcon(":/icons/quit"), tr("&Exit"), this);
    quitAction->setToolTip(tr("Exit"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));

    exitAction = new QAction(QIcon(":/icons/quit"), tr("&Exit"), this);
    exitAction->setToolTip(tr("Exit"));
    exitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X));

    toggleHideAction = new QAction(QIcon(":/icons/minimize"), tr("&Minimize"), this);

    tutoStackAction = new QAction(QIcon(":/icons/staki"), tr(""), this);
    aboutAction = new QAction(QIcon(":/icons/abouti"), tr(""), this);
    aboutQtAction = new QAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
    encryptWalletAction = new QAction(QIcon(":/icons/encrypti"), tr(""), this);
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/icons/backupi"), tr(""), this);
    changePassphraseAction = new QAction(QIcon(":/icons/passi"), tr(""), this);
    actionLockUnlockWallet_ActionScreen = new QAction(QIcon(":/icons/unlocki"), tr(""), this);
    signMessageAction = new QAction(QIcon(":/icons/signi"), tr(""), this);
    verifyMessageAction = new QAction(QIcon(":/icons/verifyi"), tr(""), this);
    exportAction = new QAction(QIcon(":/icons/export"), tr("&Export..."), this);
    openRPCConsoleAction = new QAction(QIcon(":/icons/debugi"), tr(""), this);

    signMessageAction2 = new QAction(QIcon(":/icons/edit"), tr("Sign &message..."), this);
    verifyMessageAction2 = new QAction(QIcon(":/icons/transaction_0"), tr("&Verify message..."), this);
    openRPCConsoleAction2 = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug window"), this);

    appMenuBar->addAction(exportAction);
    appMenuBar->addAction(signMessageAction);
    appMenuBar->addAction(verifyMessageAction);
    appMenuBar->addAction(quitAction);

    appMenuBar->addAction(encryptWalletAction);
    appMenuBar->addAction(changePassphraseAction);
    appMenuBar->addAction(actionLockUnlockWallet_ActionScreen);

    appMenuBar->addAction(openRPCConsoleAction);

    appMenuBar->addAction(aboutAction);
    appMenuBar->addAction(aboutQtAction);
    appMenuBar->addAction(tutoStackAction);
    appMenuBar->addAction(backupWalletAction);


    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quitApplication()));
    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(tutoStackAction, SIGNAL(triggered()), this, SLOT(tutoStackClicked()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(encryptWalletAction, SIGNAL(triggered(bool)), this, SLOT(encryptWallet(bool)));
    connect(backupWalletAction, SIGNAL(triggered()), this, SLOT(backupWallet()));
    connect(changePassphraseAction, SIGNAL(triggered()), this, SLOT(changePassphrase()));
    connect(actionLockUnlockWallet_ActionScreen, SIGNAL(triggered()), this, SLOT(lockUnlockWallet()));
    connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
    connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
    connect(signMessageAction2, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
    connect(verifyMessageAction2, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
}

void BitcoinGUI::createToolBars() {
    QLabel *mylabel = new QLabel(this);
    mylabel->setPixmap(QPixmap(":images/head"));
    mylabel->show();

    QToolBar *toolbar = addToolBar(tr("Menu"));
    toolbar->setObjectName("toolbar");
    addToolBar(Qt::LeftToolBarArea, toolbar);
    toolbar->setOrientation(Qt::Vertical);
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(50, 20));
    toolbar->addWidget(mylabel);
    toolbar->addAction(overviewAction);
    toolbar->addAction(actionSendReceive);
    toolbar->addAction(historyAction);
    toolbar->addAction(addressBookAction);
    toolbar->addAction(statisticsAction);
    toolbar->addAction(blockAction);
    toolbar->addAction(poolAction);
    toolbar->addAction(richListAction);
    toolbar->addAction(pollAction);
    toolbar->addAction(settingsAction);
    toolbar->addAction(optionsAction);
    toolbar->addAction(exportAction);
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(spacer);
    spacer->setObjectName("spacer");
    toolbar->setStyleSheet("#toolbar { font-weight:600;border:none;height:100%;padding-top:20px; background: rgb(37, 40, 46); text-align: left; color: white;min-width:180px;max-width:180px;} QToolBar QToolButton:hover {background:rgb(28, 29, 33);} QToolBar QToolButton:checked {background:rgba(28, 29, 33, 100);}  QToolBar QToolButton { font-weight:600;font-size:10px;font-family:'Century Gothic';padding-left:20px;padding-right:181px;padding-top:4px;padding-bottom:4px; width:100%; color: white; text-align: left; background:transparent;text-transform:uppercase; }");

    wId = new QWidget(this);

    QToolBar *toolbar7 = addToolBar(tr("WindowState"));
    addToolBar(Qt::RightToolBarArea, toolbar7);
    toolbar7->setOrientation(Qt::Horizontal);
    toolbar7->setMovable(false);
    toolbar7->addAction(toggleHideAction);
    toolbar7->addAction(quitAction);
    toolbar7->setStyleSheet("QMenu::item {border:0px} QMenu {border:0px;padding-left:10px;} QToolBar QToolButton {border:none;} QToolBar{ border:0px; }");

    QHBoxLayout *vbox3 = new QHBoxLayout();
    vbox3->addWidget(toolbar7);
    vbox3->setContentsMargins(0, 0, 0, 0);
    wId->setFixedSize(120, 40);
    wId->move(915, 1);
    wId->setLayout(vbox3);
    wId->setFocus();
}

void BitcoinGUI::setClientModel(ClientModel *clientModel) {
    this->clientModel = clientModel;

    if (clientModel) {
        // Replace some strings and icons, when using the testnet
        if (clientModel->isTestNet()) {
            setWindowTitle(windowTitle() + QString(" ") + tr("[testnet]"));
#ifndef Q_OS_MAC
            qApp->setWindowIcon(QIcon(":icons/bitcoin_testnet"));
            setWindowIcon(QIcon(":icons/bitcoin_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/bitcoin_testnet"));
#endif

            if (trayIcon) {
                trayIcon->setToolTip(tr("Silkcoin client") + QString(" ") + tr("[testnet]"));
                trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
                toggleHideAction->setIcon(QIcon(":/icons/toolbar_testnet"));
            }

            aboutAction->setIcon(QIcon(":/icons/toolbar_testnet"));
        }

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks(), clientModel->getNumBlocksOfPeers());
        connect(clientModel, SIGNAL(numBlocksChanged(int, int)), this, SLOT(setNumBlocks(int, int)));

        // Report errors from network/worker thread
        connect(clientModel, SIGNAL(error(QString, QString, bool)), this, SLOT(error(QString, QString, bool)));

        rpcConsole->setClientModel(clientModel);
        addressBookPage->setOptionsModel(clientModel->getOptionsModel());
        receiveCoinsPage->setOptionsModel(clientModel->getOptionsModel());
    }
}

void BitcoinGUI::setWalletModel(WalletModel *walletModel) {
    this->walletModel = walletModel;

    if (walletModel) {
        // Report errors from wallet thread
        connect(walletModel, SIGNAL(error(QString, QString, bool)), this, SLOT(error(QString, QString, bool)));

        // Put transaction list in tabs
        transactionView->setModel(walletModel);

        overviewPage->setModel(walletModel);
        addressBookPage->setModel(walletModel->getAddressTableModel());
        receiveCoinsPage->setModel(walletModel->getAddressTableModel());
        sendCoinsPage->setModel(walletModel);
        signVerifyMessageDialog->setModel(walletModel);

        statisticsPage->setModel(clientModel);
        blockBrowser->setModel(clientModel);
        poolBrowser->setModel(clientModel);
        richList->setModel(clientModel);
        poll->setModel(walletModel);
        setEncryptionStatus(walletModel->getEncryptionStatus());
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex, int, int)),
                this, SLOT(incomingTransaction(QModelIndex, int, int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(lockUnlockWallet()));
    }
}

void BitcoinGUI::createTrayIcon() {
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip(tr("Silkcoin client"));
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction2);
    trayIconMenu->addAction(verifyMessageAction2);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction2);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(exitAction);
#endif

    notificator = new Notificator(qApp->applicationName(), trayIcon);
}

#ifndef Q_OS_MAC
void BitcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
#endif

void BitcoinGUI::optionsClicked() {
    if (!clientModel || !clientModel->getOptionsModel()) {
        return;
    }

    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void BitcoinGUI::sConvert() {
    if (convertmode == 0) {
        actionConvertCurrency->setIcon(QIcon(":/icons/dollar").pixmap(STATUSBAR_ICONSIZE, 44));
        convertmode = 1;
    } else if (convertmode == 1) {
        actionConvertCurrency->setIcon(QIcon(":/icons/bitcoiniconn").pixmap(STATUSBAR_ICONSIZE, 44));
        convertmode = 2;
    } else if (convertmode == 2) {
        actionConvertCurrency->setIcon(QIcon(":/icons/sctask").pixmap(STATUSBAR_ICONSIZE, 44));
        convertmode = 0;
    }
}


void BitcoinGUI::tutoWriteClicked() {
    tutoWriteDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitcoinGUI::tutoStackClicked() {
    tutoStackDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitcoinGUI::aboutClicked() {
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitcoinGUI::setNumConnections(int count) {
    QString icon;

    switch (count) {
        case 0:
            icon = ":/icons/connect_0";
            break;

        case 1:
        case 2:
        case 3:
            icon = ":/icons/connect_1";
            break;

        case 4:
        case 5:
        case 6:
            icon = ":/icons/connect_2";
            break;

        case 7:
        case 8:
        case 9:
            icon = ":/icons/connect_3";
            break;

        default:
            icon = ":/icons/connect_4";
            break;
    }

    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE, 44));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to Silkcoin network", "", count));
}

void BitcoinGUI::setNumBlocks(int count, int nTotalBlocks) {
    // don't show / hide progress bar and its label if we have no connection to the network
    if (!clientModel || clientModel->getNumConnections() == 0) {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);

        return;
    }

    QString strStatusBarWarnings = clientModel->getStatusBarWarnings();
    QString tooltip;

    if (count < nTotalBlocks) {
        //int nRemainingBlocks = nTotalBlocks - count;
        float nPercentageDone = count / (nTotalBlocks * 0.01f);

        if (strStatusBarWarnings.isEmpty()) {
            progressBarLabel->setVisible(false);
            progressBar->setFormat(tr("%n%", "", nPercentageDone));
            progressBar->setMaximum(nTotalBlocks);
            progressBar->setValue(count);
            progressBar->setVisible(false);
        }

        tooltip = tr("Downloaded %1 of %2 blocks of transaction history (%3% done).").arg(count).arg(nTotalBlocks).arg(nPercentageDone, 0, 'f', 2);
    } else {
        if (strStatusBarWarnings.isEmpty()) {
            progressBarLabel->setVisible(false);
        }

        progressBar->setVisible(false);
        tooltip = tr("Downloaded %1 blocks of transaction history.").arg(count);
    }

    // Override progressBarLabel text and hide progress bar, when we have warnings to display
    if (!strStatusBarWarnings.isEmpty()) {
        progressBarLabel->setText(strStatusBarWarnings);
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
    }

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    QString text;

    // Represent time from last generated block in human readable text
    if (secs <= 0) {
        // Fully up to date. Leave text empty.
    } else if (secs < 60) {
        text = tr("%n second(s) ago", "", secs);
    } else if (secs < 60 * 60) {
        text = tr("%n minute(s) ago", "", secs / 60);
    } else if (secs < 24 * 60 * 60) {
        text = tr("%n hour(s) ago", "", secs / (60 * 60));
    } else {
        text = tr("%n day(s) ago", "", secs / (60 * 60 * 24));
    }

    // Set icon state: spinning if catching up, tick otherwise
    if (clientModel->getNumConnections() < 2) {
        lblBlockStatus->setPixmap(QIcon(":/icons/connecting").pixmap(STATUSBAR_ICONSIZE, 44));
        lblBlockStatus->setToolTip("Looking for more network connections");

        overviewPage->showOutOfSyncWarning(true);

        return;
    } else if (secs < 90 * 60 && count >= nTotalBlocks) {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        lblBlockStatus->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, 44));

        overviewPage->showOutOfSyncWarning(false);
    } else {
        tooltip = tr("Catching up...") + QString("<br>") + tooltip;

        lblBlockStatus->setMovie(gifSyncing);

        bool isValid = gifSyncing->isValid();
        bool isNotRunning = gifSyncing->state() == QMovie::NotRunning;
        bool hasEnoughFrames = gifSyncing->frameCount() > 1;

        if(isValid && isNotRunning && hasEnoughFrames) {
            gifSyncing->start();
        }

        overviewPage->showOutOfSyncWarning(true);
    }

    if (!text.isEmpty()) {
        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1.").arg(text);
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    lblBlockStatus->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::error(const QString &title, const QString &message, bool modal) {
    // Report errors from network/worker thread
    if (modal) {
        QMessageBox::critical(this, title, message, QMessageBox::Ok, QMessageBox::Ok);
    } else {
        notificator->notify(Notificator::Critical, title, message);
    }
}

void BitcoinGUI::changeEvent(QEvent *e) {
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac

    if (e->type() == QEvent::WindowStateChange) {
        if (clientModel && clientModel->getOptionsModel()->getMinimizeToTray()) {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);

            if (!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized()) {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }

#endif
}

void BitcoinGUI::closeEvent(QCloseEvent *event) {
    if (clientModel) {
#ifndef Q_OS_MAC // Ignored on Mac

        if (!clientModel->getOptionsModel()->getMinimizeToTray() &&
                !clientModel->getOptionsModel()->getMinimizeOnClose()) {
            qApp->quit();
        }

#endif
    }

    QMainWindow::closeEvent(event);
}

void BitcoinGUI::askFee(qint64 nFeeRequired, bool *payFee) {
    QString strMessage =
        tr("This transaction is over the size limit.  You can still send it for a fee of %1, "
           "which goes to the nodes that process your transaction and helps to support the network.  "
           "Do you want to pay the fee?").arg(
            BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
            this, tr("Confirm transaction fee"), strMessage,
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void BitcoinGUI::incomingTransaction(const QModelIndex& parent, int start, int end) {
    if (!walletModel || !clientModel) {
        return;
    }

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                    .data(Qt::EditRole).toULongLong();

    if (!clientModel->inInitialBlockDownload()) {
        // On new transaction, make an info balloon
        // Unless the initial block download is in progress, to prevent balloon-spam
        QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
        QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
        QString address = ttm->index(start, TransactionTableModel::ToAddress, parent).data().toString();
        QIcon icon = qvariant_cast<QIcon>(ttm->index(start, TransactionTableModel::ToAddress, parent).data(Qt::DecorationRole));

        if (convertmode == 0) {
            notificator->notify(Notificator::Information,
                                (amount) < 0 ? tr("Sent transaction") :
                                tr("Incoming transaction"),
                                tr("Date: %1\n"
                                   "Amount: %2\n"
                                   "Type: %3\n"
                                   "Address: %4\n")
                                .arg(date)
                                .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), amount, true))
                                .arg(type)
                                .arg(address), icon);
        }

        if (convertmode == 1) {
            notificator->notify(Notificator::Information,
                                (amount) < 0 ? tr("Sent transaction") :
                                tr("Incoming transaction"),
                                tr("Date: %1\n"
                                   "Amount: %2\n"
                                   "Type: %3\n"
                                   "Address: %4\n")
                                .arg(date)
                                .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), _dScPriceLast * amount, true))
                                .arg(type)
                                .arg(address), icon);

        }

        if (convertmode == 2) {
            notificator->notify(Notificator::Information,
                                (amount) < 0 ? tr("Sent transaction") :
                                tr("Incoming transaction"),
                                tr("Date: %1\n"
                                   "Amount: %2\n"
                                   "Type: %3\n"
                                   "Address: %4\n")
                                .arg(date)
                                .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), _dScPriceLast / _dBtcPriceCurrent * amount, true))
                                .arg(type)
                                .arg(address), icon);
        }
    }
}

void BitcoinGUI::gotoOverviewPage() {
    overviewAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(overviewPage);

    actionConvertCurrency->setEnabled(true);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);
    connect(actionConvertCurrency, SIGNAL(triggered()), this, SLOT(sConvert()));

    actionLockUnlockWallet_Toolbar->setEnabled(true);
    actionLockUnlockWallet_Toolbar->setVisible(true);
    disconnect(actionLockUnlockWallet_Toolbar, SIGNAL(triggered()), 0, 0);
    connect(actionLockUnlockWallet_Toolbar, SIGNAL(triggered()), this, SLOT(lockUnlockWallet()));

    exportAction->setVisible(false);
    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoPoolBrowser() {
    poolAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(poolBrowser);

    exportAction->setEnabled(false);
    actionConvertCurrency->setEnabled(true);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);
    connect(actionConvertCurrency, SIGNAL(triggered()), this, SLOT(sConvert()));
    exportAction->setVisible(false);

    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoRichList() {
    richListAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(richList);

    exportAction->setEnabled(false);
    actionConvertCurrency->setEnabled(false);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);

    exportAction->setVisible(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoPoll() {
    pollAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(poll);

    exportAction->setEnabled(false);
    actionConvertCurrency->setEnabled(false);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);

    exportAction->setVisible(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoBlockBrowser() {
    blockAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(blockBrowser);

    actionConvertCurrency->setEnabled(true);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);
    connect(actionConvertCurrency, SIGNAL(triggered()), this, SLOT(sConvert()));
    exportAction->setVisible(false);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoStatisticsPage() {
    statisticsAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(statisticsPage);

    actionConvertCurrency->setEnabled(true);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);
    connect(actionConvertCurrency, SIGNAL(triggered()), this, SLOT(sConvert()));
    exportAction->setVisible(false);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoHistoryPage() {
    historyAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(transactionsPage);

    convertmode = 0;
    actionConvertCurrency->setIcon(QIcon(":/icons/sctask").pixmap(STATUSBAR_ICONSIZE, 44));
    actionConvertCurrency->setVisible(true);
    actionConvertCurrency->setEnabled(false);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);

    exportAction->setEnabled(true);
    exportAction->setVisible(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), transactionView, SLOT(exportClicked()));
    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoAddressBookPage() {
    addressBookAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(addressBookPage);

    convertmode = 0;
    actionConvertCurrency->setIcon(QIcon(":/icons/sctask").pixmap(STATUSBAR_ICONSIZE, 44));
    actionConvertCurrency->setEnabled(false);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);
    exportAction->setEnabled(true);
    exportAction->setVisible(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), addressBookPage, SLOT(exportClicked()));
    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoReceiveCoinsPage() {
    sendCoinsAction->setChecked(false);
    receiveCoinsAction->setChecked(true);

    centralWidget->setCurrentWidget(receiveCoinsPage);

    convertmode = 0;
    actionConvertCurrency->setIcon(QIcon(":/icons/sctask").pixmap(STATUSBAR_ICONSIZE, 44));
    actionConvertCurrency->setEnabled(false);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);
    exportAction->setEnabled(true);
    exportAction->setVisible(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), receiveCoinsPage, SLOT(exportClicked()));

    wId2->show();
    wId2->raise();
    wId->raise();
}

void BitcoinGUI::gotoSendCoinsPage() {
    sendCoinsAction->setChecked(true);
    receiveCoinsAction->setChecked(false);

    centralWidget->setCurrentWidget(sendCoinsPage);

    convertmode = 0;
    actionConvertCurrency->setIcon(QIcon(":/icons/sctask").pixmap(STATUSBAR_ICONSIZE, 44));
    actionConvertCurrency->setEnabled(false);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);
    exportAction->setEnabled(false);
    exportAction->setVisible(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    wId2->show();
    wId2->raise();
    wId->raise();
}

void BitcoinGUI::gotoSettingsPage() {
    settingsAction->setChecked(true);
    actionSendReceive->setChecked(false);

    centralWidget->setCurrentWidget(settingsPage);

    convertmode = 0;
    actionConvertCurrency->setIcon(QIcon(":/icons/sctask").pixmap(STATUSBAR_ICONSIZE, 44));
    actionConvertCurrency->setEnabled(true);
    actionConvertCurrency->setVisible(true);
    disconnect(actionConvertCurrency, SIGNAL(triggered()), 0, 0);
    connect(actionConvertCurrency, SIGNAL(triggered()), this, SLOT(sConvert()));
    exportAction->setEnabled(false);
    exportAction->setVisible(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    wId->raise();
    wId2->hide();
}

void BitcoinGUI::gotoSignMessageTab(QString addr) {
    // call show() in showTab_SM()
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty()) {
        signVerifyMessageDialog->setAddress_SM(addr);
    }
}

void BitcoinGUI::gotoVerifyMessageTab(QString addr) {
    // call show() in showTab_VM()
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty()) {
        signVerifyMessageDialog->setAddress_VM(addr);
    }
}

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event) {
    // Accept only URIs
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void BitcoinGUI::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls()) {
        int nValidUrisFound = 0;
        QList<QUrl> uris = event->mimeData()->urls();

        foreach(const QUrl & uri, uris) {
            if (sendCoinsPage->handleURI(uri.toString())) {
                nValidUrisFound++;
            }
        }

        // if valid URIs were found
        if (nValidUrisFound) {
            gotoSendCoinsPage();
        } else {
            notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid Silkcoin address or malformed URI parameters."));
        }
    }

    event->acceptProposedAction();
}

void BitcoinGUI::handleURI(QString strURI) {
    // URI has to be valid
    if (sendCoinsPage->handleURI(strURI)) {
        showNormalIfMinimized();
        gotoSendCoinsPage();
    } else {
        notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid Silkcoin address or malformed URI parameters."));
    }
}

void BitcoinGUI::setEncryptionStatus(int status) {
    switch (status) {
        case WalletModel::Unencrypted:
            actionLockUnlockWallet_Toolbar->setIcon(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE, 44));
            actionLockUnlockWallet_Toolbar->setToolTip("Wallet is <b>unencrypted</b>");

            actionLockUnlockWallet_ActionScreen->setVisible(false);

            encryptWalletAction->setChecked(false);
            encryptWalletAction->setEnabled(true);

            changePassphraseAction->setEnabled(false);

            lblCamelTraveling->setMovie(gifCamelLocked);
            gifCamelUnlocked->stop();
            gifCamelLocked->start();

            break;

        case WalletModel::Unlocked:
            actionLockUnlockWallet_Toolbar->setIcon(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE, 44));
            actionLockUnlockWallet_Toolbar->setToolTip("Wallet is <b>Unlocked</b> for <b>Staking</b>!");

            actionLockUnlockWallet_ActionScreen->setIcon(QIcon(":/icons/locki"));
            actionLockUnlockWallet_ActionScreen->setVisible(true);

            encryptWalletAction->setChecked(true);
            encryptWalletAction->setEnabled(false); //TODO: currently not supported

            changePassphraseAction->setEnabled(true); //TODO: currently not supported

            lblCamelTraveling->setMovie(gifCamelUnlocked);
            gifCamelUnlocked->start();
            gifCamelLocked->stop();

            break;

        case WalletModel::Locked:
            actionLockUnlockWallet_Toolbar->setIcon(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE, 44));
            actionLockUnlockWallet_Toolbar->setToolTip("Click here to <b>Unlock</b> your wallet and start <b>Staking</b>!");

            actionLockUnlockWallet_ActionScreen->setIcon(QIcon(":/icons/unlocki"));
            actionLockUnlockWallet_ActionScreen->setVisible(true);

            encryptWalletAction->setChecked(true);
            encryptWalletAction->setEnabled(false); //TODO: currently not supported

            changePassphraseAction->setEnabled(true); //TODO: currently not supported

            lblCamelTraveling->setMovie(gifCamelLocked);
            gifCamelUnlocked->stop();
            gifCamelLocked->stop();

            break;
    }
}

void BitcoinGUI::encryptWallet(bool status) {
    if (!walletModel) {
        return;
    }

    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt :
                            AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    setEncryptionStatus(walletModel->getEncryptionStatus());
}

void BitcoinGUI::backupWallet() {
    QString saveDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    QString filename = QFileDialog::getSaveFileName(this, tr("Backup Wallet"), saveDir, tr("Wallet Data (*.dat)"));

    if (!filename.isEmpty()) {
        if (!walletModel->backupWallet(filename)) {
            QMessageBox::warning(this, tr("Backup Failed"), tr("There was an error trying to save the wallet data to the new location."));
        }
    }
}

void BitcoinGUI::changePassphrase() {
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void BitcoinGUI::lockUnlockWallet() {
    if (!walletModel) {
        return;
    }

    AskPassphraseDialog::Mode mode = AskPassphraseDialog::Init;

    switch (walletModel->getEncryptionStatus()) {

        case WalletModel::Locked:
            mode = sender() == actionLockUnlockWallet_ActionScreen || sender() == actionLockUnlockWallet_Toolbar
                   ? AskPassphraseDialog::UnlockStaking : AskPassphraseDialog::Unlock;
            break;

        case WalletModel::Unlocked:
            walletModel->setWalletLocked(true);
            return; //Not a break on purpose

        default:
            break;
    }

    AskPassphraseDialog dlg(mode, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void BitcoinGUI::showNormalIfMinimized(bool toTray, bool isToggle) {
    if (isHidden()) {
        show();
        this->setWindowState(Qt::WindowActive);
    } else if (isMinimized()) {
        this->setWindowState((this->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    } else if (GUIUtil::isObscured(this)) {
        setWindowState(Qt::WindowActive);
    } else if (toTray) {
        hide();
    } else if (isToggle) {
        showMinimized();
    } else {

    }
}

void BitcoinGUI::toggleHidden() {
    showNormalIfMinimized(OptionsModel().getMinimizeToTray(), true);
}

void BitcoinGUI::quitApplication() {
    if (OptionsModel().getMinimizeToTray() &&
            OptionsModel().getMinimizeOnClose()) {
        hide();
    } else if (!OptionsModel().getMinimizeToTray() &&
               OptionsModel().getMinimizeOnClose()) {
        showNormalIfMinimized(false, true);
    } else {
        qApp->quit();
    }
}

void BitcoinGUI::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
}

void BitcoinGUI::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalX() - m_nMouseClick_X_Coordinate, event->globalY() - m_nMouseClick_Y_Coordinate);
}

void BitcoinGUI::updateStakingIcon() {
    uint64_t nMinWeight = 0, nMaxWeight = 0, nWeight = 0;

    if (pwalletMain) {
        pwalletMain->GetStakeWeight(*pwalletMain, nMinWeight, nMaxWeight, nWeight);
    }

    if (clientModel->getNumConnections() < 2) {
        lblBlockStatus->setPixmap(QIcon(":/icons/connecting").pixmap(STATUSBAR_ICONSIZE, 44));
        lblBlockStatus->setToolTip("Looking for more network connections");
    } else if (nLastCoinStakeSearchInterval && nWeight) {
        uint64_t nNetworkWeight = GetPoSKernelPS();
        unsigned nEstimateTime = nTargetSpacing * nNetworkWeight / nWeight;

        QString text;

        if (nEstimateTime < 60) {
            text = tr("%n second(s)", "", nEstimateTime);
        } else if (nEstimateTime < 60 * 60) {
            text = tr("%n minute(s)", "", nEstimateTime / 60);
        } else if (nEstimateTime < 24 * 60 * 60) {
            text = tr("%n hour(s)", "", nEstimateTime / (60 * 60));
        } else {
            text = tr("%n day(s)", "", nEstimateTime / (60 * 60 * 24));
        }

        labelStakingIcon->setPixmap(QIcon(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE, 44));
        labelStakingIcon->setToolTip(tr("Staking.<br>Your weight is %1<br>Network weight is %2<br>Expected time to earn reward is %3").arg(nWeight).arg(nNetworkWeight).arg(text));
    } else {
        labelStakingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, 44));

        if (pwalletMain && pwalletMain->IsLocked()) {
            labelStakingIcon->setToolTip(tr("Not staking because wallet is locked"));
        } else if (vNodes.empty()) {
            labelStakingIcon->setToolTip(tr("Not staking because wallet is offline"));
        } else if (IsInitialBlockDownload()) {
            labelStakingIcon->setToolTip(tr("Not staking because wallet is syncing"));
        } else if (!nWeight) {
            labelStakingIcon->setToolTip(tr("Not staking because you don't have mature coins"));
        } else if (!pwalletMain->IsLocked()) {
            labelStakingIcon->setToolTip(tr("Preparing to stake."));
        } else {
            labelStakingIcon->setToolTip(tr("Not staking."));
        }
    }
}

void BitcoinGUI::openFacebook() {
    QDesktopServices::openUrl(QUrl("https://www.facebook.com/pages/SilkCoin/734640973258763"));
}
void BitcoinGUI::openTwitter() {
    QDesktopServices::openUrl(QUrl("https://twitter.com/silkcoinrevival"));
}
void BitcoinGUI::openReddit() {
    QDesktopServices::openUrl(QUrl("http://www.reddit.com/r/Silk/"));
}
