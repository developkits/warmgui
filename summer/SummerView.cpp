#include "StdAfx.h"
#include "warmgui_summer.h"
#include "TestSummerCanvas.h"
#include "SummerAtelier.h"
#include "SummerView.h"

#include "Resource.h"

CSummerView::CSummerView(void)
    : _atelier(0)
{
    _rectClient.left = _rectClient.top = _rectClient.bottom = _rectClient.right = 0;
}


CSummerView::~CSummerView(void)
{
    SafeDelete(_atelier);
}


BOOL CSummerView::PreCreateWindow(LPCREATESTRUCT cs)
{
	HBRUSH hbr = (HBRUSH)GetClassLong(_hwnd, GCL_HBRBACKGROUND);
	DeleteObject(hbr);
	hbr = CreateSolidBrush(RGB(0, 0, 0));
	SetClassLong(_hwnd, GCL_HBRBACKGROUND, (LONG)hbr);
	return TRUE;
}

void CSummerView::OnSize(UINT /*nType*/, int cx, int cy)
{
    //MYTRACE(L"OnSize\n");
	if (_atelier && cx > 0 && cy > 0) {
		::GetClientRect(_hwnd, &_rectClient);
		_atelier->set_rect(_rectClient);
	}
}

void CSummerView::OnDraw()
{
    MYTRACE(L"OnDraw\n");
	if (_atelier) {
		_atelier->Draw();
    }
}

void CSummerView::OnDestroy()
{
    MYTRACE(L"OnDestroy\n");
    ::PostQuitMessage(0);
}

int CSummerView::OnCreate(LPCREATESTRUCT /*cs*/)
{
    _atelier = new CSummerAtelier("summer");
    _atelier->init(_hwnd);

	SetWindowLong(_hwnd, GWL_STYLE, WS_VISIBLE | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW);

	int cx = (int)(GetSystemMetrics(SM_CXSCREEN) * .6f);
	int cy = (int)(GetSystemMetrics(SM_CYSCREEN) * .6f);

	MoveWindow(_hwnd, 0, 0, cx, cy, true);
	SetFocus(_hwnd);

	return 0;
}

int CSummerView::OnCommand(WORD nCmdId, WORD /*nSource*/, HWND /*hwnd*/)
{
	switch(nCmdId) {
	case ID_VIEW_TOOLBAR:
		_atelier->ToggleToolbar();
		break;
	}

	return (0);
}
