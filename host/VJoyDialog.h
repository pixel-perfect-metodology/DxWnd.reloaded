#pragma once


// CTimeSlider dialog

class CVJoyDialog : public CDialog
{
	DECLARE_DYNAMIC(CVJoyDialog)

public:
	CVJoyDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CVJoyDialog();
	//~VJoyDialog();

// Dialog Data
	enum { IDD = IDD_VJOYDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
public:
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
	//virtual void OnCancel();
private:
	int GetSliderPos(int);
	int SetSensivity(int);
	void InitSensivity(void);
	BOOL m_VJoyPresent;
	BOOL m_VJoyEnabled;
	BOOL m_CrossEnabled;
	BOOL m_InvertYAxis;
	BOOL m_InvertXAxis;
	BOOL m_B1AutoFire;
	BOOL m_B2AutoFire;
	BOOL m_VJAutoCenter;
	BOOL m_VJMouseWheel;
	int m_UseMode;
	int m_VJoySensivity;
	int m_Slider;
	CBrush *Background;
	CPen *CenterPen;
	CPen *FinderPen;
	CPen *SensivityPen;
	int Multipliers[41];
};
