//
//   MFCNIDAQReadDlg.cpp 
//
//   (c) 1999-2012 Plexon Inc. Dallas Texas 75205 
//   www.plexoninc.com
//
//   Simple dialog-based MFC app that reads MAP events from the Server 
//   and periodically displays the first eight NIDAQ samples and timestamps on the
//   first NIDAQ channel (ACH0).
//
//   Built using Microsoft Visual C++ 8.0.  Must include Plexon.h and link with PlexClient.lib.
//
//   See MFCSampleClients.doc for more information.
//

#include "stdafx.h"
#include "MFCNIDAQRead.h"
#include "MFCNIDAQReadDlg.h"

//** header file containing the Plexon APIs (link with PlexClient.lib, run with PlexClient.dll)
#include "../../include/plexon.h"

//** maximum number of MAP events to be read at one time from the Server
#define MAX_MAP_EVENTS_PER_READ 500000

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//** these are global variables, for simplicity
BOOL          bConnectedToServer = FALSE;   //** TRUE once we've successfully connected to the Server
BOOL          bFetchingTimeStamps = FALSE;  //** TRUE when we're getting timestamps from the Server (i.e. not paused)
UINT          nTimerID = -1;                //** ID of the one second timer
PL_WaveLong*  pServerEventBuffer;           //** buffer in which the Server will return MAP events
int           NumMAPEvents;                 //** number of MAP events returned from the Server
int           NumNIDAQSamples;              //** number of samples within a NIDAQ sample block 
UINT          SampleTime;                   //** timestamp a NIDAQ sample
int           MAPSampleRate;                //** samples/sec for MAP channels
int           NIDAQSampleRate;              //** samples/sec for NIDAQ channels
int           ServerDropped;                //** nonzero if server dropped any data
int           MMFDropped;                   //** nonzero if MMF dropped any data
int           PollHigh;                     //** high 32 bits of polling time
int           PollLow;                      //** low 32 bits of polling time
int           MAPEventIndex;                //** loop counter
int           SampleIndex;                  //** loop counter
char          szTemp[255];          
int           Dummy[64];

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMFCNIDAQReadDlg dialog

CMFCNIDAQReadDlg::CMFCNIDAQReadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMFCNIDAQReadDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMFCNIDAQReadDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCNIDAQReadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMFCNIDAQReadDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMFCNIDAQReadDlg, CDialog)
	//{{AFX_MSG_MAP(CMFCNIDAQReadDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCONNECT, OnConnect)
	ON_BN_CLICKED(IDPAUSE, OnPause)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMFCNIDAQReadDlg message handlers

BOOL CMFCNIDAQReadDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFCNIDAQReadDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCNIDAQReadDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFCNIDAQReadDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMFCNIDAQReadDlg::OnConnect() 
{
	if (!bConnectedToServer)
  {
    //** connect to the server and register ourselves as a client
    PL_InitClientEx3(0, NULL, GetSafeHwnd());
    
    //** get the MAP sampling rate (spike timestamps)
    switch (PL_GetTimeStampTick()) //** returns timestamp resolution in microseconds
    {
      case 25: //** 25 usec = 40 kHz, default
        MAPSampleRate = 40000;
        break;
      case 40: //** 40 usec = 25 kHz
        MAPSampleRate = 25000;
        break;
      case 50: //** 50 usec = 20 kHz
        MAPSampleRate = 20000;
        break;
      default:
        MessageBox("Unsupported MAP sampling time!");
        return;
    }
  
    //** get the NIDAQ sampling rate
    PL_GetSlowInfo(&NIDAQSampleRate, Dummy, Dummy); // last two params are unused here

    //** allocate memory in which the server will return MAP events
    pServerEventBuffer = (PL_WaveLong*)malloc(sizeof(PL_WaveLong)*MAX_MAP_EVENTS_PER_READ);
    if (pServerEventBuffer == NULL)
    {
      MessageBox("Couldn't allocate memory!\r\n");
      return;
    }

    bConnectedToServer = bFetchingTimeStamps = TRUE;

    //** disable Connect button, since we're connected now
    GetDlgItem(IDCONNECT)->EnableWindow(FALSE);

    //** set 250 msec timer
    nTimerID = SetTimer(1, 250, NULL);
  }
}

void CMFCNIDAQReadDlg::OnPause() 
{
  if (bConnectedToServer)
  {
    //** toggle this boolean to indicate whether we should read timestamps from the server
    bFetchingTimeStamps = !bFetchingTimeStamps;	
    SetDlgItemText(IDPAUSE, bFetchingTimeStamps ? "Pause" : "Resume");
  }
}

void CMFCNIDAQReadDlg::OnOk() 
{
	CDialog::OnOK();
}

void CMFCNIDAQReadDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
  //** kill the one second timer
  if (nTimerID != -1)
    KillTimer(nTimerID);

  //** release the memory allocated for the timestamps
  free(pServerEventBuffer);
}

void CMFCNIDAQReadDlg::OnTimer(UINT nIDEvent) 
{
  char sz[16];

	if (bFetchingTimeStamps && (nIDEvent == nTimerID))
  {
    int NumMAPEvents = MAX_MAP_EVENTS_PER_READ;

    //** read the MAP events from the server
    PL_GetLongWaveFormStructuresEx2(&NumMAPEvents, pServerEventBuffer, 
      &ServerDropped, &MMFDropped, &PollHigh, &PollLow);

    //** total number of MAP events read
    sprintf(sz, "%d", NumMAPEvents);
    SetDlgItemText(IDC_EVENTSREAD, sz); 
   
    //** look for a MAP event containing NIDAQ samples
    for (MAPEventIndex = 0; MAPEventIndex < NumMAPEvents; MAPEventIndex++)
    {
      //** is this a block of NIDAQ samples from the first NIDAQ channel?
      if (pServerEventBuffer[MAPEventIndex].Type == PL_ADDataType &&
          pServerEventBuffer[MAPEventIndex].Channel == 0) //** channel 0 only - reduces amount of output
      {
        NumNIDAQSamples = pServerEventBuffer[MAPEventIndex].NumberOfDataWords;
        SampleTime = pServerEventBuffer[MAPEventIndex].TimeStamp;
        //** display up to the first 8 samples in the dialog box
        for (SampleIndex = 0; SampleIndex < min(NumNIDAQSamples, 8); SampleIndex++)
        {
          //** note: the ratio MAPSampleRate/NIDAQSampleRate is always an integer
          if (SampleIndex > 0)
            SampleTime += MAPSampleRate/NIDAQSampleRate; 
          sprintf(szTemp, "%f", (float)SampleTime/(float)MAPSampleRate);
          SetDlgItemText(IDC_TIMESTAMP0+SampleIndex, szTemp);
          sprintf(szTemp, "%d", pServerEventBuffer[MAPEventIndex].WaveForm[SampleIndex]);
          SetDlgItemText(IDC_VALUE0+SampleIndex, szTemp);
        }
        break; //** from for loop
      }
    }
  }
	
	CDialog::OnTimer(nIDEvent);
}
