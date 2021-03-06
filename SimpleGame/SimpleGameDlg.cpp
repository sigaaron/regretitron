#include "stdafx.h"
#include "MfcExceptions.h"
#include "SimpleGame.h"
#include "SimpleGameDlg.h"
#include "XSleep.h"
#include "UserInput.h"
#include <sstream>
using std::istringstream;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//cool function to get type-safe input from a dialog box!!
template<typename T> bool getuserinput(string inputtext, T &value)
{
	UserInput diagbox(inputtext);
	if(diagbox.DoModal() == IDCANCEL)
		return false;
	else
	{
		if(diagbox.inputtext2.GetLength()<1)
			return false;
		istringstream i((LPCTSTR)diagbox.inputtext2);
		i >> value;
		if(i.fail())
			return false;
		return true;
	}
}

// --------------------- MFC Class, window stuff ---------------------------

// CSimpleGameDlg dialog
CSimpleGameDlg::CSimpleGameDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSimpleGameDlg::IDD, pParent),
#ifdef USE_NETWORK_CLIENT
	  _mybot( new ClientAPI ),
#else
	  _mybot(NULL),
#endif
	  cardrandom(),
	  handsplayed(0),
	  handsplayedtourney(0),
	  cashsblind(10),
	  cashbblind(20),
	  humanstacksize(1500),
	  botstacksize(1500),
	  effstacksize(1500), // = mymin(humanstacksize, botstacksize)
	  human(P0), 
	  bot(P1),
	  totalhumanwon(0),
	  istournament(false)
{ MFC_STD_EH_PROLOGUE
	CardMask_RESET(botcm0); //ensures show bot cards produces jokers if done early
	CardMask_RESET(botcm1);
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	Create(CSimpleGameDlg::IDD, NULL);
	ShowWindow(SW_SHOW);
MFC_STD_EH_EPILOGUE }

void CSimpleGameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HCARD0, hCard0);
	DDX_Control(pDX, IDC_HCARD1, hCard1);
	DDX_Control(pDX, IDC_CCARD0, cCard0);
	DDX_Control(pDX, IDC_CCARD1, cCard1);
	DDX_Control(pDX, IDC_CCARD2, cCard2);
	DDX_Control(pDX, IDC_CCARD3, cCard3);
	DDX_Control(pDX, IDC_CCARD4, cCard4);
	DDX_Control(pDX, IDC_BCARD0, bCard0);
	DDX_Control(pDX, IDC_BCARD1, bCard1);
	DDX_Control(pDX, IDC_CHECK1, ShowBotCards);
	DDX_Control(pDX, IDC_EDIT5, TotalWon);
	DDX_Control(pDX, IDC_EDIT2, InvestedHum);
	DDX_Control(pDX, IDC_EDIT3, InvestedBot);
	DDX_Control(pDX, IDC_EDIT1, BetAmount);
	DDX_Control(pDX, IDC_CHECK2, ShowDiagnostics);
	DDX_Control(pDX, IDC_BUTTON1, FoldCheckButton);
	DDX_Control(pDX, IDC_BUTTON2, CallButton);
	DDX_Control(pDX, IDC_BUTTON3, BetRaiseButton);
	DDX_Control(pDX, IDC_BUTTON4, AllInButton);
	DDX_Control(pDX, IDC_BUTTON6, MakeBotGoButton);
	DDX_Control(pDX, IDC_BSTACK, BotStack);
	DDX_Control(pDX, IDC_HSTACK, HumanStack);
	DDX_Control(pDX, IDC_BUTTON5, NewGameButton);
	DDX_Control(pDX, IDC_CHECK3, AutoNewGame);
	DDX_Control(pDX, IDC_CHECK4, AutoBotPlay);
	DDX_Control(pDX, IDC_POTVAL, PotValue);
	DDX_Control(pDX, IDC_OPENFILE, OpenBotButton);
	DDX_Control(pDX, IDC_HUMANDEAL, HumanDealerChip);
	DDX_Control(pDX, IDC_BOTDEAL, BotDealerChip);
}

BEGIN_MESSAGE_MAP(CSimpleGameDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CSimpleGameDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CSimpleGameDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON5, &CSimpleGameDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_CHECK1, &CSimpleGameDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON6, &CSimpleGameDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON3, &CSimpleGameDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CSimpleGameDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_CHECK2, &CSimpleGameDlg::OnBnClickedCheck2)
	ON_COMMAND(ID_MENU_NEWTOURNEY, &CSimpleGameDlg::OnMenuNewTourney)
	ON_COMMAND(ID_MENU_PLAYCASHGAME, &CSimpleGameDlg::OnMenuPlayCashGame)
	ON_COMMAND(ID_MENU_SETBOTBANKROLL, &CSimpleGameDlg::OnMenuSetBotBankroll)
	ON_COMMAND(ID_MENU_SETALLBANKROLL, &CSimpleGameDlg::OnMenuSetAllBankroll)
	ON_COMMAND(ID_MENU_SETYOURBANKROLL, &CSimpleGameDlg::OnMenuSetYourBankroll)
	ON_COMMAND(ID_MENU_FIXBANKROLL, &CSimpleGameDlg::OnMenuFixBankroll)
	ON_COMMAND(ID_LOADBOTFILE, &CSimpleGameDlg::OnLoadbotfile)
	ON_COMMAND(ID_SHOWBOTFILE, &CSimpleGameDlg::OnShowbotfile)
	ON_COMMAND(ID_MENU_EXIT, &CSimpleGameDlg::OnMenuExit)
END_MESSAGE_MAP()

// CSimpleGameDlg message handlers

//http://www.codeproject.com/KB/dialog/gettingmodeless.aspx
//http://msdn.microsoft.com/en-us/library/sk933a19(VS.80).aspx
//both say to use this function:
//Due to the following, this only frees C++ memory. Not neccesary, but nice.
void CSimpleGameDlg::PostNcDestroy() 
{	
    CDialog::PostNcDestroy();
	if(_mybot != NULL)
		delete _mybot;
    delete this;
}
//but then more digging makes me realize that the problem was that
//the window was being "cancelled" not "closed". Closed would have
//destroyed the window, but cancelled does nothing.
//http://msdn.microsoft.com/en-us/library/kw3wtttf.aspx
//"If you implement the Cancel button in a modeless dialog box, 
// you must override the OnCancel method and call DestroyWindow 
// inside it. Do not call the base-class method, because it calls 
// EndDialog, which will make the dialog box invisible but not 
// destroy it."
//Okay.
void CSimpleGameDlg::OnCancel()
{ MFC_STD_EH_PROLOGUE
	_mybot->setdiagnostics(false); //this will DestroyWindow the diagnostics page if it exists.
	DestroyWindow();
MFC_STD_EH_EPILOGUE }

//this is a hack to make the enter key call the raise button
//handler and do nothing else (like close the window).
//fixes everything in one fell swoop.
//http://www.flounder.com/dialogapp.htm
void CSimpleGameDlg::OnOK()
{ MFC_STD_EH_PROLOGUE
	if(MakeBotGoButton.IsWindowEnabled()) // bot's turn
		OnBnClickedButton6();
	else if(BetRaiseButton.IsWindowEnabled() && BetAmount.GetWindowTextLength()>0)
		OnBnClickedButton3();
	else if(CallButton.IsWindowEnabled())
		OnBnClickedButton2();
	else if(NewGameButton.IsWindowEnabled())
		OnBnClickedButton5();
MFC_STD_EH_EPILOGUE }

BOOL CSimpleGameDlg::OnInitDialog()
{ MFC_STD_EH_PROLOGUE

	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//make a menu!
	//"Create a CMenu object on the stack frame as a local, then call CMenu's 
	// member functions to manipulate the new menu as needed. Next, call 
	// CWnd::SetMenu to set the menu to a window, followed immediately by a 
	// call to the CMenu object's Detach member function."
	//(I save the menu to an object here so I can check/uncheck and such)
	MyMenu.LoadMenu(IDR_MYMENU);
	SetMenu(&MyMenu);
	MyMenu.CheckMenuItem(ID_MENU_PLAYCASHGAME, MF_CHECKED);

	//find a strategy/portfolio file and create the bot api

#ifdef USE_NETWORK_CLIENT
	_mybot->Init( );
#endif
	if(!loadbotfile()) 
		exit(0);

	//initialize the window controls that need it

	TotalWon.SetWindowText(TEXT(""));
	graygameover();

	//set the picture on the File Open Button

	OpenBotButton.LoadBitmaps(IDB_OPEN1, IDB_OPEN1, IDB_OPEN1, IDB_OPEN2);
	OpenBotButton.SizeToContent();

	OnMenuPlayCashGame();

	return TRUE;  // return TRUE  unless you set the focus to a control
MFC_STD_EH_EPILOGUE }

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSimpleGameDlg::OnPaint()
{ MFC_STD_EH_PROLOGUE
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
MFC_STD_EH_EPILOGUE }

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSimpleGameDlg::OnQueryDragIcon()
{ MFC_STD_EH_PROLOGUE
	return static_cast<HCURSOR>(m_hIcon);
MFC_STD_EH_EPILOGUE }




// -------------------------- Card Printing Functions -------------------------

void CSimpleGameDlg::printbotcards()
{
	bCard0.LoadFromCardMask(botcm0);
	bCard1.LoadFromCardMask(botcm1);
}
void CSimpleGameDlg::printbotcardbacks()
{
	bCard0.LoadFromFile(TEXT("cards/b1fv.png"));
	bCard1.LoadFromFile(TEXT("cards/b1fv.png"));
}
void CSimpleGameDlg::printhumancards()
{
	hCard0.LoadFromCardMask(humancm0);
	hCard1.LoadFromCardMask(humancm1);
}
void CSimpleGameDlg::printflop()
{
	cCard0.LoadFromCardMask(flop0);
	cCard1.LoadFromCardMask(flop1);
	cCard2.LoadFromCardMask(flop2);
}
void CSimpleGameDlg::printturn()
{
	cCard3.LoadFromCardMask(turn);
}
void CSimpleGameDlg::printriver()
{
	cCard4.LoadFromCardMask(river);
}
void CSimpleGameDlg::eraseboard()
{
	cCard0.FreeData();
	cCard1.FreeData();
	cCard2.FreeData();
	cCard3.FreeData();
	cCard4.FreeData();
	this->RedrawWindow();
}



// ---------------- On-screen Update Functions -------------------

void CSimpleGameDlg::updatepot()
{
	//reprint the pot to the screen
	CString val;
	val.Format(TEXT("Common Pot: %.0f"), 2*pot);
	PotValue.SetWindowText(val);
}
void CSimpleGameDlg::updatess()
{
	CString val;
	val.Format(TEXT("You: %.0f"), humanstacksize-invested[human]-pot);
	HumanStack.SetWindowText(val);
	val.Format(TEXT("Bot: %.0f"), botstacksize-invested[bot]-pot);
	BotStack.SetWindowText(val);
}

void CSimpleGameDlg::updateinvested(Player justacted)
{
	CString val;

	if(fpequal(invested[human], invested[bot]))
	{
		if(fpequal(invested[human], 0))
			val = TEXT("CHECK");
		else
			val = TEXT("CALL");
	}
	else
		val.Format(TEXT("%.0f"), invested[justacted]);

	if(justacted == human)
		InvestedHum.SetWindowText(val);
	else if(justacted == bot)
		InvestedBot.SetWindowText(val);
	else
	{
		InvestedHum.SetWindowText(TEXT(""));
		InvestedBot.SetWindowText(TEXT(""));
	}

	updatess();
}

bool CSimpleGameDlg::loadbotfile()
{
#ifdef USE_NETWORK_CLIENT
	unsigned gametype;
	if( ! getuserinput("Choices:\n   Enter \"1\" for Limit\n   Enter \"2\" for No-Limit", gametype )
			|| ( gametype != 1 && gametype != 2 ) )
		return false;

	if( _mybot->GetSessionType( ) != SESSION_NONE )
		_mybot->CancelSession( );

	MessageCreateNewSession message;
	if( gametype == 1 )
		message.gametype = GAME_PLAYMONEY_LIMIT;
	else if(gametype == 2)
		message.gametype = GAME_PLAYMONEY_NOLIMIT;
	_mybot->CreateNewSession( message );
#else
	CFileDialog filechooser(TRUE, "xml", NULL, OFN_NOCHANGEDIR, "Bot Definition XML files (*.xml)|*.xml||");
	if(filechooser.DoModal() == IDCANCEL) 
		return false;
	//load in the new file now.
	delete _mybot; //will kill diag
	_mybot = new BotAPI(
			string((LPCTSTR)filechooser.GetPathName()),
			false,
			MTRand::gettimeclockseed( ),
			m_logger, // this one is the 'logger'
			BotAPI::botapinulllogger ); // this one is the reclog
	if(ShowDiagnostics.GetCheck())
		_mybot->setdiagnostics(true, this);
#endif

	totalhumanwon = 0;
	handsplayed = 0;
	if(_mybot->islimit())
		BetAmount.EnableWindow(FALSE);
	else
		BetAmount.EnableWindow(TRUE);
	return true;
}


// ------------------------- Game Logic --------------------------------

double CSimpleGameDlg::mintotalwager(Player acting)
{
	if( fpgreater( invested[ acting ], invested[ 1 - acting ] ) )
		REPORT( "Invested amounts are not consistant with acting player." );

	double minwager = invested[ 1 - acting ] + betincrement( );

	if(pot + minwager > effstacksize) //fp routines unneccessary
		return effstacksize - pot;
	else
		return minwager;
}

double CSimpleGameDlg::betincrement()
{
	if( _mybot->islimit( ) )
		return ( _gameround == PREFLOP || _gameround == FLOP ) ? _bblind : 2 * _bblind;
	else
		return mymax( _bblind, myabs( invested[ P0 ] - invested[ P1 ] ) );
}

void CSimpleGameDlg::dofold(Player pl)
{
	//set the winner to the other player
	_winner = 1-pl; //1-pl returns the other player, since they are 0 and 1
	dogameover(true);
}
void CSimpleGameDlg::docall(Player pl)
{
	//inform bot unless game is over
	if(!_isallin && _gameround != RIVER)
		_mybot->doaction(pl,CALL,invested[1-pl]);
	//show call
	invested[pl] = invested[1-pl];
	updateinvested(pl);
	XSleep(650, this);
	//increase pot
	pot += invested[1-pl];
	updatepot();
	//reset invested
	invested[bot]=invested[human]=0;
	updateinvested();

	//we may be calling an all-in if the previous player bet that.
	//if so, show our cards and fast forward to the river, 
	if(_isallin)
	{
		printbotcards();
		switch(_gameround) //correct amount of suspense needed.
		{
		case PREFLOP:
			XSleep(1000, this); 
			printflop();
		case FLOP:
			XSleep(1000, this);
			printturn();
		case TURN:
			XSleep(1000, this);
			printriver();
		}
		_gameround=RIVER;
	}

	switch(_gameround)
	{
	//for the first three rounds, we just inform the bot and then print the cards.
	case PREFLOP: 
		_mybot->setnextround(FLOP, flop, pot); 
		printflop();
		break;

	case FLOP: 
		_mybot->setnextround(TURN, turn, pot); 
		printturn();
		break;

	case TURN: 
		_mybot->setnextround(RIVER, river, pot); 
		printriver();
		break;

	case RIVER:
		//game over
		printbotcards();
		break;

	default:
		REPORT("invalid _gameround in docall");
	}

	//finally, move on to the next _gameround, rendering it invalid if this was river
	_gameround ++;

	if(_gameround>RIVER)
		dogameover(false);
	else
		graypostact(P0); //P0 is first to act post flop
}
void CSimpleGameDlg::dobet(Player pl, double amount)
{
	//we can check the amount for validity. This is important since we
	//will be getting values from the bot here.
	if( ! ( 
				( fpequal( amount, invested[ 1 - pl ] ) && _gameround == PREFLOP && pl==P1 && fpgreatereq(_bblind,invested[1-pl]) ) || 
				( fpequal( amount, 0 ) && _gameround != PREFLOP && pl==P0 && fpequal(invested[1-pl],0) ) || 
				( fpgreatereq( amount, mintotalwager( pl ) ) && fpgreatereq( effstacksize, amount + pot ) ) 
			) )
		REPORT( "SimpleGame: Invalid bet amount. "
				"(amount=" + tostr( amount )
				+ " mintotalwager( " + tostr( pl ) + " )=" + tostr( mintotalwager( pl ) )
				+ " effstacksize=" + tostr( effstacksize )
				+ " pot=" + tostr( pot )
				+ ")" );

	if(fpequal(amount + pot, effstacksize))
		_isallin = true;

	//inform the bot
	_mybot->doaction(pl,BET,amount);
	//set our invested amount
	invested[pl] = amount;
	updateinvested(pl);
	//other players turn now
	graypostact(Player(1-pl));
}

void CSimpleGameDlg::dogameover(bool fold)
{
	//Update the total amount won and print to screeen

	if (_winner==human)
	{
		totalhumanwon += (pot + invested[bot]);
		humanstacksize += (pot + invested[bot]);
		botstacksize -= (pot + invested[bot]);
	}
	else if (_winner==bot)
	{
		totalhumanwon -= (pot + invested[human]);
		humanstacksize -= (pot + invested[human]);
		botstacksize += (pot + invested[human]);
	}
	effstacksize = mymin(humanstacksize, botstacksize);

	//reset values to reprint the stacks to the screen
	pot = invested[bot] = invested[human] = 0;
	updatess();

	//print user friendly hints to invested amounts
	if(fold)
	{
		if (_winner==human)
			InvestedBot.SetWindowText(TEXT("FOLD"));
		else if(_winner==bot)
			InvestedHum.SetWindowText(TEXT("FOLD"));
	}
	else if(_winner==human)
	{
		InvestedBot.SetWindowText(TEXT("LOSE"));
		InvestedHum.SetWindowText(TEXT("WIN"));
	}
	else if(_winner==bot)
	{
		InvestedHum.SetWindowText(TEXT("LOSE"));
		InvestedBot.SetWindowText(TEXT("WIN"));
	}
	else
	{
		InvestedHum.SetWindowText(TEXT("TIE"));
		InvestedBot.SetWindowText(TEXT("TIE"));
	}

	//set the bankrolls back if they are fixed

	if( isfixedbankroll( ) )
	{
		CString val;
		val.Format(TEXT("P/L: %.0f"), totalhumanwon);
		TotalWon.SetWindowText(val);
		humanstacksize = humanfixedstacksize;
		botstacksize = botfixedstacksize;
		effstacksize = mymin(humanstacksize, botstacksize);
	}
	else
	{
		if(fpequal(humanstacksize, 0))
			MessageBox( "bot won", "game over.", MB_OK );
		if(fpequal(botstacksize, 0))
			MessageBox( "you won", "Good job.", MB_OK );
	}

	//if auto, Start new game, else, Set grayness values

	if(AutoNewGame.GetCheck())
	{
		if(fpgreater(mymin(humanstacksize, botstacksize), 0))
		{
			//sleep less when there's folds. more when there's cards to look at!
			XSleep((fold?2000:4000), this);
			OnBnClickedButton5();
		}
		else
		{
			AutoNewGame.SetCheck(BST_UNCHECKED);
			graygameover();
		}
	}
	else
		graygameover();

}

// ---------------------------- Setting grayness -----------------------------

void CSimpleGameDlg::graygameover()
{
	//gray out all but newgame
	OpenBotButton.EnableWindow(TRUE);
	NewGameButton.EnableWindow(TRUE); //only allow new game & bankroll when NOT in a game
	MyMenu.EnableMenuItem(ID_MENU_SETBOTBANKROLL, MF_ENABLED);
	MyMenu.EnableMenuItem(ID_MENU_SETYOURBANKROLL, MF_ENABLED);
	MyMenu.EnableMenuItem(ID_MENU_SETALLBANKROLL, MF_ENABLED);
	FoldCheckButton.SetWindowText(TEXT("Fold/Check"));
	FoldCheckButton.EnableWindow(FALSE);
	CallButton.EnableWindow(FALSE);
	BetRaiseButton.EnableWindow(FALSE);
	AllInButton.EnableWindow(FALSE);
	MakeBotGoButton.EnableWindow(FALSE);
}

void CSimpleGameDlg::graypostact(Player nexttoact)
{
	if(AutoBotPlay.GetCheck() && nexttoact == bot)
	{
		XSleep(500, this);
		OnBnClickedButton6();
		return;
	}


	OpenBotButton.EnableWindow(FALSE);
	NewGameButton.EnableWindow(FALSE); //only allow new game & bankroll when NOT in a game
	MyMenu.EnableMenuItem(ID_MENU_SETBOTBANKROLL, MF_GRAYED);
	MyMenu.EnableMenuItem(ID_MENU_SETYOURBANKROLL, MF_GRAYED);
	MyMenu.EnableMenuItem(ID_MENU_SETALLBANKROLL, MF_GRAYED);

	if(nexttoact == bot)
	{
		FoldCheckButton.SetWindowText(TEXT("Fold/Check"));
		FoldCheckButton.EnableWindow(FALSE);
		CallButton.EnableWindow(FALSE);
		BetRaiseButton.EnableWindow(FALSE);
		AllInButton.EnableWindow(FALSE);
		MakeBotGoButton.EnableWindow(TRUE);
	}
	else //next to act is human
	{
		//gray out makebotgo
		MakeBotGoButton.EnableWindow(FALSE);

		if(_isallin || _mybot->islimit()) //can't go all in twice
			AllInButton.EnableWindow(FALSE);
		else
			AllInButton.EnableWindow(TRUE);

		if(fpequal(invested[bot], 0) || (_gameround == PREFLOP && fpequal(invested[bot], _bblind) && fpequal(invested[human], _bblind)))
		{
			//set fold/check to check, not grayed
			FoldCheckButton.SetWindowText(TEXT("Check"));
			FoldCheckButton.EnableWindow(TRUE);
			//gray out call
			CallButton.EnableWindow(FALSE);
		}
		else
		{
			//set fold/check to fold, not grayed
			FoldCheckButton.SetWindowText(TEXT("Fold"));
			FoldCheckButton.EnableWindow(TRUE);
			//call active
			CallButton.EnableWindow(TRUE);
		}

		//if min bet + our pot share < effstacksize
		if(_mybot->islimit())
		{
			if(fpgreatereq(invested[1-nexttoact], 4*betincrement()) || fpgreatereq(pot+invested[1-nexttoact], effstacksize))
				BetRaiseButton.EnableWindow(FALSE);
			else
				BetRaiseButton.EnableWindow(TRUE);
		}
		else
		{
			if(fpgreater(effstacksize, pot + mintotalwager(human)))
				BetRaiseButton.EnableWindow(TRUE);
			else
				BetRaiseButton.EnableWindow(FALSE);
		}
	}
}

// ------------------------- Button/Event Handlers ---------------------------

// ------------------------------ Check Boxes --------------------------------
// clicked the check box to show/unshow bot cards
void CSimpleGameDlg::OnBnClickedCheck1()
{ MFC_STD_EH_PROLOGUE
	//we indescriminately print cards or cardbacks. 
	//if called before any game starts, this will print garbage cards. that's ok.
	if(ShowBotCards.GetCheck())
		printbotcards();
	else
		printbotcardbacks();
MFC_STD_EH_EPILOGUE }

// clicked the check box to show/unshow diagnostics window
void CSimpleGameDlg::OnBnClickedCheck2()
{ MFC_STD_EH_PROLOGUE
	//all window handling is done by MyBot. 
	//This function is our only interaction
	//(it decides when it's appropriate to pop up the window, and it closes it)
	if(ShowDiagnostics.GetCheck())
		_mybot->setdiagnostics(true, this);
	else
		_mybot->setdiagnostics(false);
MFC_STD_EH_EPILOGUE }

// ---------------------------- Action Buttons -------------------------------
// This is the fold/check button.
void CSimpleGameDlg::OnBnClickedButton1()
{ MFC_STD_EH_PROLOGUE
	//if this is true, we are checking
	if(fpequal(invested[human], 0) && fpequal(invested[bot], 0))
	{
		if(_gameround == PREFLOP)
			REPORT("Invested[human and bot] is zero in preflop");

		//if we are first to act, then this is a "bet" as the gr continues
		if(human == P0)
			dobet(human, 0);
		//otherwise we are second to act. then we are calling a check and the gr ends.
		else
			docall(human);
	}
	//it's the preflop, the bet has called from the SB(will never happen), human can check, and is not already all-in
	else if (_gameround == PREFLOP && human == P0 
		&& fpequal(invested[bot], _bblind) && fpequal(invested[human], _bblind) && fpgreater(effstacksize, _bblind))
		docall(human);
	//there is a difference in invested which we are unwilling to reconcile
	else if (fpgreater(invested[bot], invested[human]))
		dofold(human);
	else
		REPORT("inconsistant invested amounts");
MFC_STD_EH_EPILOGUE }

// This is the call button.
void CSimpleGameDlg::OnBnClickedButton2()
{ MFC_STD_EH_PROLOGUE
	//if this is preflop and we are calling from the SB, 
	// then that's actually a "bet" as it doesn't end the gr
	if(_gameround == PREFLOP && fpequal(invested[human], _sblind) 
		&& fpgreater(effstacksize, _bblind)) //this would end the gave via a call
		dobet(human, min(_bblind,effstacksize));
	//all other cases, a call is a call
	else
		docall(human);
MFC_STD_EH_EPILOGUE }

// This is the bet/raise button
void CSimpleGameDlg::OnBnClickedButton3()
{ MFC_STD_EH_PROLOGUE
	//the bet/raise button always has the meaning of betting/raising,
	// (that is, meaning as defined by my poker api or, rather, BotAPI)

	if(_mybot->islimit())
	{
		dobet(human, min(effstacksize-pot, invested[bot] + betincrement()));
	}
	else
	{
		//first get value of edit box
		CString valstr;
		double val;
		BetAmount.GetWindowText(valstr);
		BetAmount.SetWindowText(TEXT(""));
		val = strtod(valstr, NULL); //converts string to double

		//input checking: make sure it's not too small or too big, then do it.
		if(fpgreatereq(val, mintotalwager(human)) && fpgreater(effstacksize, val+pot))
			dobet(human, val);
		else
			MessageBox( ( "$" + tostr( val ) + " is an invalid amount. Need " + tostr( mintotalwager( human ) ) + " <= amount < " + tostr( effstacksize - pot ) ).c_str( ) );
	}
MFC_STD_EH_EPILOGUE }

// This is the all-in button
void CSimpleGameDlg::OnBnClickedButton4()
{ MFC_STD_EH_PROLOGUE

	//we can do an all-in at any time, for any reason, baby.
	dobet(human, effstacksize - pot);

MFC_STD_EH_EPILOGUE }

// This is the new game button
void CSimpleGameDlg::OnBnClickedButton5()
{ MFC_STD_EH_PROLOGUE
	if(fpequal(0, mymin(humanstacksize, botstacksize)))
	{
		MessageBox( "no money left, use bankroll options on menu." );
		return;
	}
	else if(fpgreater(0, mymin(humanstacksize, botstacksize)))
	{
		REPORT("stacksize is negative!");
	}

	CardMask usedcards, fullhuman, fullbot;
	HandVal r0, r1;

	//increment games played
	handsplayed++;

	if(istournament) 
	{
		//then raise the blinds!
		const double bbets[] = {60, 80, 100, 120, 160, 200, 240, 300, 400, 500, 600, 800, 1000, 1200, 1600, 2000, 2400, 3000};
		if(handsplayedtourney++%18 == 0)
		{
			_bblind = bbets[handsplayedtourney/18] / 2;
			_sblind = bbets[handsplayedtourney/18] / 4;
			MessageBox( ( "Tourney bets are now "+tostring(_bblind)+"/"+tostring(_bblind*2)+"." ).c_str( ) , "Blinds", MB_OK );
		}
	}
	else
	{
		_bblind = cashbblind;
		_sblind = cashsblind;
	}

	CString text;
	if( _mybot->islimit( ) )
		text.Format("SimpleGame Limit - %s - %d hands played - bet sizes: %.0f/%.0f", 
				(istournament?"Tournament":"Cash Game"), handsplayed-1, _bblind, _bblind*2);
	else
		text.Format("SimpleGame No-Limit - %s - %d hands played - blinds: %.0f/%.0f", 
				(istournament?"Tournament":"Cash Game"), handsplayed-1, _sblind, _bblind);
	SetWindowText(text); //titlebar
	//change positions & set dealer chip
	std::swap(human,bot);
	if(human == P1)
	{
		HumanDealerChip.LoadFromFile("cards/dealerchip.png");
		BotDealerChip.FreeData();
		BotDealerChip.RedrawWindow();
	}
	else
	{
		BotDealerChip.LoadFromFile("cards/dealerchip.png");
		HumanDealerChip.FreeData();
		HumanDealerChip.RedrawWindow();
	}

	//set _gameround
	_gameround = PREFLOP;
	//reset pot
	pot = 0;
	updatepot();
	//we may have an auto-all-in shortstacked moment.
	_isallin = fpgreater(effstacksize, _bblind) ? false : true;

	//deal out the cards randomly.
	MTRand &mersenne = cardrandom; //alias random for macros.
	CardMask_RESET(usedcards);
	//set the cm's corresponding to individual cards (for displaying)
#define DEALANDOR(card) MONTECARLO_N_CARDS_D(card, usedcards, 1, 1, ); CardMask_OR(usedcards, usedcards, card);
	DEALANDOR(botcm0)
	DEALANDOR(botcm1)
	DEALANDOR(humancm0)
	DEALANDOR(humancm1)
	DEALANDOR(flop0)
	DEALANDOR(flop1)
	DEALANDOR(flop2)
	DEALANDOR(turn)
	DEALANDOR(river)
#undef DEALANDOR

	//set the cm's corresponding to groups of cards
	CardMask_OR(botcards, botcm0, botcm1);
	CardMask_OR(humancards, humancm0, humancm1);
	CardMask_OR(flop, flop0, flop1);
	CardMask_OR(flop, flop, flop2);

	//set the cm's corresponding to full final 7-card hands
	CardMask_OR(fullhuman, humancards, flop);
	CardMask_OR(fullhuman, fullhuman, turn);
	CardMask_OR(fullhuman, fullhuman, river);

	CardMask_OR(fullbot, botcards, flop);
	CardMask_OR(fullbot, fullbot, turn);
	CardMask_OR(fullbot, fullbot, river);

	//compute would-be winner, and we're done.
	r0=Hand_EVAL_N(fullhuman, 7);
	r1=Hand_EVAL_N(fullbot, 7);

	if (r0>r1)
		_winner = human;
	else if(r1>r0)
		_winner = bot;
	else
		_winner = -1;

	//inform bot of the new game
	_mybot->setnewgame(bot, botcards, _sblind, _bblind, effstacksize);

	//display the cards pics
	eraseboard();
	printhumancards();
	ShowBotCards.SetCheck(BST_UNCHECKED); //uncheck show bot cards, incase accidently left checked by user
	printbotcardbacks();

	//show blank
	updateinvested();
	XSleep(200, this);
	//show small blind
	invested[P1] = mymin(effstacksize, _sblind);
	updateinvested(P1);
	XSleep(200, this);
	//show big blind
	invested[P0] = mymin(effstacksize, _bblind);
	updateinvested(P0);
	XSleep(200, this);

	//finally set grayness
	if(fpgreatereq(_sblind, effstacksize))
		docall(P1); //we have an auto call moment. stack is smaller than or equal to the small blind
	else
		graypostact(P1); //we play the game as normal.
MFC_STD_EH_EPILOGUE }

//This is the "make bot go" button.
void CSimpleGameDlg::OnBnClickedButton6()
{ MFC_STD_EH_PROLOGUE
	double val;
	Action act;
	//get answer from bot
	act = _mybot->getbotaction(val);

	//the bot uses the same semantics, so this is easy
	switch(act)
	{
	case FOLD: dofold(bot); break;
	case CALL: docall(bot); break;
	case BET: dobet(bot, val); break;
	}
MFC_STD_EH_EPILOGUE }

void CSimpleGameDlg::OnMenuNewTourney()
{ MFC_STD_EH_PROLOGUE
	//set starting stacks for a tourney
	humanstacksize = 1500;
	botstacksize = 1500;
	effstacksize = 1500;
	istournament = true;
	handsplayedtourney = 0;
	MyMenu.CheckMenuItem(ID_MENU_NEWTOURNEY, MF_CHECKED);
	MyMenu.CheckMenuItem(ID_MENU_PLAYCASHGAME, MF_UNCHECKED);
	//message box should pop up notifying of new bet sizes
	OnBnClickedButton5(); //new game button
MFC_STD_EH_EPILOGUE }

void CSimpleGameDlg::OnMenuPlayCashGame()
{ MFC_STD_EH_PROLOGUE
	istournament = false;
	MyMenu.CheckMenuItem(ID_MENU_NEWTOURNEY, MF_UNCHECKED);
	MyMenu.CheckMenuItem(ID_MENU_PLAYCASHGAME, MF_CHECKED);
	MessageBox( ( "Playing Cash Game: Blinds are set to "+tostring(cashsblind)+", "+tostring(cashbblind) ).c_str( ), "New Game" );
	OnBnClickedButton5();
MFC_STD_EH_EPILOGUE }

void CSimpleGameDlg::OnMenuSetBotBankroll()
{ MFC_STD_EH_PROLOGUE
	double input;
	if(!getuserinput("How much for bot?", input) || input < 0)
		return;
	botstacksize = input;
	effstacksize = mymin(botstacksize, humanstacksize);
	updatess();
MFC_STD_EH_EPILOGUE }

void CSimpleGameDlg::OnMenuSetYourBankroll()
{ MFC_STD_EH_PROLOGUE
	double input;
	if(!getuserinput("How much for human?", input) || input < 0)
		return;
	humanstacksize = input;
	effstacksize = mymin(botstacksize, humanstacksize);
	updatess();
MFC_STD_EH_EPILOGUE }

void CSimpleGameDlg::OnMenuSetAllBankroll()
{ MFC_STD_EH_PROLOGUE
	double input;
	if(!getuserinput("How much for everybody?", input) || input < 0)
		return;
	botstacksize = humanstacksize = effstacksize = input;
	updatess();
MFC_STD_EH_EPILOGUE }

void CSimpleGameDlg::OnMenuFixBankroll()
{ MFC_STD_EH_PROLOGUE
	if( isfixedbankroll( ) )
	{
		TotalWon.SetWindowText(TEXT(""));
		MyMenu.CheckMenuItem( ID_MENU_FIXBANKROLL, MF_UNCHECKED );
	}
	else if( fpequal( humanstacksize, 0 ) || fpequal( botstacksize, 0 ) )
		MessageBox( "Can't fix bankroll; someone has zero chips." );
	else
	{
		humanfixedstacksize = humanstacksize;
		botfixedstacksize = botstacksize;
		totalhumanwon = 0;
		MyMenu.CheckMenuItem( ID_MENU_FIXBANKROLL, MF_CHECKED );
	}
MFC_STD_EH_EPILOGUE }

void CSimpleGameDlg::OnLoadbotfile()
{ MFC_STD_EH_PROLOGUE if(loadbotfile()) /*then start new game*/ OnBnClickedButton5(); MFC_STD_EH_EPILOGUE } 
void CSimpleGameDlg::OnShowbotfile()
{ MFC_STD_EH_PROLOGUE MessageBox(_mybot->getxmlfile().c_str( ), "Bot version" ); MFC_STD_EH_EPILOGUE } 
void CSimpleGameDlg::OnMenuExit()
{ MFC_STD_EH_PROLOGUE OnCancel(); MFC_STD_EH_EPILOGUE }
