// MFCTimeStampReadDlg.h : header file
//

#if !defined(AFX_MFCTIMESTAMPREADDLG_H__412D2780_DD96_11D3_A359_00C04F796B88__INCLUDED_)
#define AFX_MFCTIMESTAMPREADDLG_H__412D2780_DD96_11D3_A359_00C04F796B88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMFCTimeStampReadDlg dialog

class CMFCTimeStampReadDlg : public CDialog
{
// Construction
public:
	CMFCTimeStampReadDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMFCTimeStampReadDlg)
	enum { IDD = IDD_MFCTIMESTAMPREAD_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMFCTimeStampReadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMFCTimeStampReadDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnConnect();
	afx_msg void OnPause();
	virtual void OnOK();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MFCTIMESTAMPREADDLG_H__412D2780_DD96_11D3_A359_00C04F796B88__INCLUDED_)
