#include "StdAfx.h"
#include "warmgui.h"

namespace WARMGUI {


static COLORREF _default_color[] = {
	BGR(0,     0, 255),
	BGR(0,   255,   0),
	BGR(255,   0,   0),
};
static const int __default_color_number__ = 3;
	
/*
ISeriesDataGraph::ISeriesDataGraph(void)
    : _manipulator(DATA_MANIPULATOR_TYPE_GRAPH)
{
}


ISeriesDataGraph::ISeriesDataGraph(const char* name)
    : IGlyph(name)
    , _manipulator(DATA_MANIPULATOR_TYPE_GRAPH)
{
}


ISeriesDataGraph::~ISeriesDataGraph(void)
{
}
*/



CDataLineGraph::~CDataLineGraph(void)
{
	SafeRelease(&_pathg);
    SafeRelease(&_pSink);
}

CDataLineGraph::CDataLineGraph(const char* name, GEOMETRY_PATH_TYPE path_type/* = GEOMETRY_PATH_TYPE_LINE*/, bool world_own_type/* = false*/, bool data_own_type/*  = false*/, bool own_artist/* = false*/)
	: IDataGraph(name, world_own_type, data_own_type, own_artist)
	, _color(0)
	, _data_x_offset(0)
	, _data_y_offset(0)
	//, _datasize(datasize)
	, _alpha(1.0f)
	, _pathg(0)
    , _pSink(0)
	, _stroke_width(1.0f)
    , _path_type(path_type)
    //, _rtcset(0)
{
    InitDataPtr();

    memset(&psForNew, 0, sizeof(psForNew));
    setClass();
}


void CDataLineGraph::InitDataPtr()
{
    memset(&psForNew, 0, sizeof(psForNew));
}


HRESULT CDataLineGraph::Renew()
{
#ifdef _DEBUG
    //TCHAR name[MAX_PATH];
    //CChineseCodeLib::Gb2312ToUnicode(name, MAX_PATH, _name);
    //MYTRACE(L"CDataLineGraph::Renew %s\n", name);
#endif //_DEBUG
    HRESULT hr = S_OK;
    if (_myown_artist) {
        CriticalLock::Scoped scope(_myown_artist->_lock_artist);
        eArtist* _back_artist = _artist;
        _artist = _myown_artist;

        _artist->BeginBmpDraw();
	    if (_referframe) {
    		_artist->GetTransform(&_backup_trans);
            MATRIX_2D* m = _referframe->GetTransform();
            _artist->SetTransform(m);
        }

        hr = RenewGraph();

        if (_referframe)
		    _artist->SetTransform(&_backup_trans);
        _artist->EndBmpDraw();

        _changed_type = GLYPH_CHANGED_TYPE_CHANGED;
        _artist = _back_artist;
    }
    return hr;
}

//static int iiiiiii = 0;

/// if the referframe was changed then return 1
GLYPH_CHANGED_TYPE CDataLineGraph::NewData(dataptr pdata, bool need_renew/* = true*/)
{
#ifdef _DEBUG
    //TCHAR name[MAX_PATH];
    //CChineseCodeLib::Gb2312ToUnicode(name, MAX_PATH, _name);
    //MYTRACE(
    //    L"CDataLineGraph %s %d : %.02f, %.02f\n",
    //    name,
    //    iiiiiii++,
    //    *(float*)((char*)(pdata) + _data_x_offset),
    //    *(float*)((char*)(pdata) + _data_y_offset));
#endif //_DEBUG

    Changed(GLYPH_CHANGED_TYPE_CHANGED);
    WORLD_RECT limit = _referframe->GetWorldRect();

    if (!psForNew._count) {
        psForNew._pntOld = psForNew._pntNew = FPOINT(
            *(float*)((char*)(pdata) + _data_x_offset),
            *(float*)((char*)(pdata) + _data_y_offset));
	} else {
		//keep old point
		psForNew._pntOld = psForNew._pntNew;
		//Add new point
        psForNew._pntNew = FPOINT(
            *(float*)((char*)(pdata) + _data_x_offset),
            *(float*)((char*)(pdata) + _data_y_offset));
        //_stroke_width = abs(limit.maxy - psForNew._pntNew.y) / (limit.maxy - limit.miny);
        //_stroke_width = 1.0f / _referframe->GetYScale();
	}

    //change the x axis
    //if (psForNew._pntNew.x >= limit.xn - 1)
    //    MoveBitmapToLeft();

	++psForNew._count;
	//_canvas->Changed(_changed_type);
	//_atelier->Changed(_changed_type);


	/** for test redraw-coordinate
	if (!_tcscmp(_name, L"rtdata-data_ctp_price")) {
		_changed_type |= GLYPH_CHANGED_TYPE_COORDFRAME;
		_canvas->Changed(_changed_type);
		_atelier->Changed(_changed_type);
	}
	*/

    Renew();

	return _changed_type;
}


inline void CDataLineGraph::SetDataOffset(int x_offset, int y_offset)
{
	_data_x_offset = x_offset,
		_data_y_offset = y_offset,
		_color = _default_color[0],
		_alpha = 1.0f;
}

HRESULT CDataLineGraph::_draw_lines(bool /*redraw*/)
{
	HRESULT hr = S_OK;

    SafeRelease(&_pathg);
        SafeRelease(&_pSink);

        
    D2D1_ANTIALIAS_MODE am = _artist->GetHwndRT()->GetAntialiasMode();
	_artist->GetUsingRT()->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

        hr = CDxFactorys::GetInstance()->GetD2DFactory()->CreatePathGeometry(&_pathg);

	    hr = _pathg->Open(&_pSink);

        _artist->GetTransform(&_backup_trans);

	    //MYTRACE(L"backup %.02f %.02f %.02f %.02f %.02f %.02f\n", 
	    //    backtrans._11, backtrans._12, backtrans._21, backtrans._22, backtrans._31, backtrans._32);

        MATRIX_2D rect_trans = D2D1::Matrix3x2F::Identity();
        rect_trans._31 = static_cast<float>(_rect.left), rect_trans._32 = static_cast<float>(_rect.top );
        _artist->SetTransform(&rect_trans);
        _artist->PushLayer(.0f, .0f, fRectWidth(_rect), fRectHeight(_rect));


        MATRIX_2D& newtrans = *(_referframe->GetTransform());
        newtrans._31 += static_cast<float>(_rect.left),
        newtrans._32 += static_cast<float>(_rect.top);
        _artist->SetTransform(&newtrans);

        if (SUCCEEDED(hr)) {
		    _pSink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
            _pSink->BeginFigure(
                D2D1::Point2F(_myown_data._points[0].x, _myown_data._points[0].y),
                D2D1_FIGURE_BEGIN_HOLLOW);
            psForSetData._pntOld.x = _myown_data._points[0].x, psForSetData._pntOld.y = _myown_data._points[0].y;

            int start_pos = (_path_type == CDataLineGraph::GEOMETRY_PATH_TYPE_LINE) ? 1 : 2;
            for (int i = start_pos; i < _myown_data._count; i++) {
#ifdef          _DEBUG
                {/*
	                MATRIX_2D m;
	                _artist->GetTransform(&m);
                    //MYTRACE(L"Transform: %.02f %.02f, %.02f %.02f, %.02f %.02f\n", m._11, m._12, m._21, m._22, m._31, m._32);
	                D2D1::Matrix3x2F trans(m._11, m._12, m._21, m._22, m._31, m._32);
                    POINT_f P1 = D2D1::Point2F(_myown_data._points[i - 1].x, _myown_data._points[i - 1].y);
                    POINT_f P2 = D2D1::Point2F(_myown_data._points[i].x, _myown_data._points[i].y);
                    //WORLD_RECT& wr = _referframe->GetWorldRect();
                    //POINT_f P1 = D2D1::Point2F(wr.maxpos, wr.maxy);
                    //POINT_f P2 = D2D1::Point2F(wr.minpos, wr.miny);
                    POINT_f p1 = trans.TransformPoint(P1);
	                POINT_f p2 = trans.TransformPoint(P2);
	                //MYTRACE(L"%s %.02f %.02f, %.02f %.02f ==> %.02f %.02f, %.02f %.02f\n",
                    MYTRACE(L"%d -- %.02f %.02f, %.02f %.02f ==> %.02f %.02f, %.02f %.02f\n",
		                //_name,
                        i,
		                P1.x, P1.y,
		                P2.x, P2.y,
		                p1.x, p1.y,
		                p2.x, p2.y);
                */}
#endif          //_DEBUG

                if (_path_type == CDataLineGraph::GEOMETRY_PATH_TYPE_BEZIER) {
                    _pSink->AddBezier(
                        D2D1::BezierSegment(
                            D2D1::Point2F(_myown_data._points[i - 2].x, _myown_data._points[i - 2].y),
                            D2D1::Point2F(_myown_data._points[i - 1].x, _myown_data._points[i - 1].y),
                            D2D1::Point2F(_myown_data._points[i].x, _myown_data._points[i].y)));
                } else if (_path_type == CDataLineGraph::GEOMETRY_PATH_TYPE_LINE) {
                    _pSink->AddLine( D2D1::Point2F(_myown_data._points[i].x, _myown_data._points[i].y));
                }
            }

	        _pSink->EndFigure(D2D1_FIGURE_END_OPEN);

            hr = _pSink->Close();
	        SafeRelease(&_pSink);
	        _changed_type = (GLYPH_CHANGED_TYPE)((int)_changed_type | (int)GLYPH_CHANGED_TYPE_CHANGED);
        } else {
            hr = (-3);
        }

    if (SUCCEEDED(hr)) {
   	    COLORALPHA   clr = _artist->GetSCBrush()->GetColor();
        _color = BGR(255, 255, 0);
	    _artist->GetSCBrush()->SetColor(D2D1::ColorF(_color));
	    //_artist->DrawLine(_rect.left, _rect.top, _rect.right, _rect.bottom);
        //SetTransform(_referframe->GetTransform());

        //D2D1::Matrix3x2F m = D2D1::Matrix3x2F::Identity(); 
        //_artist->SetTransform(&m);

        if (_pathg)
	        _artist->DrawGeometry(_pathg, _artist->GetSCBrush(), _stroke_width, _artist->GetStrokeStyle());
        _artist->GetSCBrush()->SetColor(clr);

        _artist->GetUsingRT()->SetAntialiasMode(am);
        _artist->SetTransform(&rect_trans);
        _artist->PopLayer();
        _artist->SetTransform(&_backup_trans);
    }
    return S_OK;
}

HRESULT CDataLineGraph::DrawGraph(bool redraw/* = false*/)
{
#ifdef _DEBUG
    //TCHAR name[MAX_PATH];
    //CChineseCodeLib::Gb2312ToUnicode(name, MAX_PATH, _name);
    //MYTRACE(L"CDataLineGraph::DrawGraph %s\n", name);
#endif //_DEBUG

    if (_data_own_type) {
        if (_myown_data._count < 2)
            return S_OK;
        else
            return _draw_lines(redraw);
    } else {
	    if (psForNew._count < 2) return S_OK;

	    HRESULT hr = S_OK;


	    COLORALPHA   clr = _artist->GetSCBrush()->GetColor();
	    _artist->GetSCBrush()->SetColor(D2D1::ColorF(_color));
	    //_artist->DrawLine(_rect.left, _rect.top, _rect.right, _rect.bottom);
        //SetTransform(_referframe->GetTransform());

        _artist->GetTransform(&_backup_trans);
        _artist->SetTransform(_referframe->GetTransform());
        //D2D1::Matrix3x2F m = D2D1::Matrix3x2F::Identity(); 
        //_artist->SetTransform(&m);

        if (_pathg)
	        _artist->DrawGeometry(_pathg, _artist->GetSCBrush(), _stroke_width, _artist->GetStrokeStyle());
        else {
            POINT_f pntOld, pntNew;
            pntOld.x = psForNew._pntOld.x,
                pntOld.y = psForNew._pntOld.y,
                pntNew.x = psForNew._pntNew.x,
                pntNew.y = psForNew._pntNew.y;
            _artist->DrawLine(pntOld, pntNew, _stroke_width);
            /*WORLD_RECT& wr = _referframe->GetWorldRect();
            POINT_f P1 = D2D1::Point2F(wr.maxpos, wr.maxy);
            POINT_f P2 = D2D1::Point2F(wr.minpos, wr.miny);
            _artist->DrawLine(P1, P2, _stroke_width);*/
        }

	    _artist->GetSCBrush()->SetColor(clr);

        {/*
	        MATRIX_2D m;
	        _artist->GetTransform(&m);
	        //MYTRACE(L"DRAW %s: %.02f %.02f, %.02f %.02f, %.02f %.02f\n", _name, m._11, m._12, m._21, m._22, m._31, m._32);
            MYTRACE(L"Transform: %.02f %.02f, %.02f %.02f, %.02f %.02f\n", m._11, m._12, m._21, m._22, m._31, m._32);
	        D2D1::Matrix3x2F trans(m._11, m._12, m._21, m._22, m._31, m._32);
            POINT_f P1 = D2D1::Point2F(psForNew._pntOld.x, psForNew._pntOld.y);
            POINT_f P2 = D2D1::Point2F(psForNew._pntNew.x, psForNew._pntNew.y);
            //WORLD_RECT& wr = _referframe->GetWorldRect();
            //POINT_f P1 = D2D1::Point2F(wr.maxpos, wr.maxy);
            //POINT_f P2 = D2D1::Point2F(wr.minpos, wr.miny);
            POINT_f p1 = trans.TransformPoint(P1);
	        POINT_f p2 = trans.TransformPoint(P2);
	        //MYTRACE(L"%s %.02f %.02f, %.02f %.02f ==> %.02f %.02f, %.02f %.02f\n",
            MYTRACE(L"%.02f %.02f, %.02f %.02f ==> %.02f %.02f, %.02f %.02f\n",
		        //_name,
		        P1.x, P1.y,
		        P2.x, P2.y,
		        p1.x, p1.y,
		        p2.x, p2.y);
        */}

        //GetbackTransform();
        _artist->SetTransform(&_backup_trans);

	    return hr;
    }
}


HRESULT CDataLineGraph::RenewGraph()
{
#ifdef _DEBUG
    //TCHAR name[MAX_PATH];
    //CChineseCodeLib::Gb2312ToUnicode(name, MAX_PATH, _name);
    //MYTRACE(L"RenewGraph %s _changed_type %d\n", name, _changed_type);
#endif //_DEBUG

	if (psForNew._count < 2) return S_OK;

	COLORALPHA   clr = _artist->GetSCBrush()->GetColor();
	_artist->GetSCBrush()->SetColor(D2D1::ColorF(_color));
	
	if (_changed_type & GLYPH_CHANGED_TYPE_COORDFRAME) {
        if (_pathg) {
#ifdef      _DEBUG
            //size_t my_count;
            //_pathg->GetSegmentCount(&my_count);
            //MYTRACE(L"CDataLineGraph::RenewGraph PATH %s %d\n", name, my_count);
#endif      //_DEBUG
            _artist->Clear();
	    	_artist->DrawGeometry(_pathg, _artist->GetSCBrush(), _stroke_width, _artist->GetStrokeStyle());
        }
    }
    else
    {
        //MYTRACE(L"CDataLineGraph::RenewGraph POINT %s\n", name);
        POINT_f pntOld, pntNew;
        pntOld.x = psForNew._pntOld.x,
            pntOld.y = psForNew._pntOld.y,
            pntNew.x = psForNew._pntNew.x,
            pntNew.y = psForNew._pntNew.y;
        _artist->DrawLine(pntOld, pntNew, _stroke_width);
    }

	_artist->GetSCBrush()->SetColor(clr);

	{/*
#   ifdef _DEBUG
		MATRIX_2D m;
		_artist->GetTransform(&m);
		D2D1::Matrix3x2F trans(m._11, m._12, m._21, m._22, m._31, m._32);
		//MYTRACE(L"Re_new %s: %.02f %.02f, %.02f %.02f, %.02f %.02f\n", _name, m._11, m._12, m._21, m._22, m._31, m._32);
        MYTRACE(L"Re_new: %.02f %.02f, %.02f %.02f, %.02f %.02f\n", m._11, m._12, m._21, m._22, m._31, m._32);
		//MATRIX_2D_t* m = _referframe->GetTransform();
		//D2D1::Matrix3x2F trans(m->_11, m->_12, m->_21, m->_22, m->_31, m->_32);
        POINT_f P1 = D2D1::Point2F(psForNew._pntOld.x, psForNew._pntOld.y), P2 = D2D1::Point2F(psForNew._pntNew.x, psForNew._pntNew.y);
		POINT_f p1 = trans.TransformPoint(P1);
		POINT_f p2 = trans.TransformPoint(P2);
		//MYTRACE(L"%s %d: %.02f %.02f, %.02f %.02f ==> %.02f %.02f, %.02f %.02f\n",
        MYTRACE(L"%d: %.02f %.02f, %.02f %.02f ==> %.02f %.02f, %.02f %.02f\n",
			//_name,
			psForNew._count,
			P1.x, P1.y,
			P2.x, P2.y,
			p1.x, p1.y,
			p2.x, p2.y);
#	endif //_DEBUG*/
	}
	return S_OK;
}


HRESULT CDataLineGraph::PreDraw()
{
    /*
    int ndown = (_rtcset->_down_intval < 2) ? 1 : _rtcset->_down_intval;
    char* p = (char*)(_graph_data._pdata);


    int mycount = (int)((float)_graph_data._count / (float)ndown) + 1;
    if (mycount < 2) return S_OK;

    BeginSetData(p);

    p += _graph_data._data_size * ndown;
    for (int i = 1; i < _graph_data._count; i += ndown, p += _graph_data._data_size * ndown) {
        AddDataToPathGeometry(p);
    }

    EndSetData();

    RedrawGraph();
    */
    return S_OK;
}

void CDataLineGraph::RedrawGraph()
{
#ifdef _DEBUG
    //TCHAR name[MAX_PATH];
    //CChineseCodeLib::Gb2312ToUnicode(name, MAX_PATH, _name);
    //MYTRACE(L"CDataLineGraph::RedrawGraph %s\n", name);
#endif //_DEBUG
    if (_myown_artist && psForNew._count > 2) {
        CriticalLock::Scoped scope(_myown_artist->_lock_artist);

        eArtist* _back_artist = _artist;
        _artist = _myown_artist;

        _artist->BeginBmpDraw();
    	_artist->GetSCBrush()->SetColor(D2D1::ColorF(_color));
	    if (_referframe) {
    		_artist->GetTransform(&_backup_trans);
            MATRIX_2D* m = _referframe->GetTransform();
            _artist->SetTransform(m);
        }

        if (_pathg) {
            _artist->Clear();
	        _artist->DrawGeometry(_pathg, _artist->GetSCBrush(), _stroke_width, _artist->GetStrokeStyle());
        }

        if (_referframe)
		    _artist->SetTransform(&_backup_trans);
        _artist->EndBmpDraw();

        _changed_type = (GLYPH_CHANGED_TYPE)((int)_changed_type | (int)GLYPH_CHANGED_TYPE_CHANGED);
        _artist = _back_artist;
    }
}

void CDataLineGraph::SetRect(RECT& rect)
{
    IGlyph::SetRect(rect);
    //RedrawGraph();
}

inline void CDataLineGraph::BeginSetData(dataptr new_data)
{
	HRESULT hr = S_OK;
	SafeRelease(&_pathg);
    SafeRelease(&_pSink);
	hr = CDxFactorys::GetInstance()->GetD2DFactory()->CreatePathGeometry(&_pathg);

	hr = _pathg->Open(&_pSink);
	if (SUCCEEDED(hr)) {
		_pSink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
        _pSink->BeginFigure(
            D2D1::Point2F(
                *(float*)((char*)(new_data) + _data_x_offset),
                *(float*)((char*)(new_data) + _data_y_offset)),
            D2D1_FIGURE_BEGIN_HOLLOW);

        psForSetData._pntNew.x = *(float*)((char*)(new_data) + _data_x_offset), psForSetData._pntNew.y = *(float*)((char*)(new_data) + _data_y_offset);
        psForSetData._count = 1;
    }
}

inline void CDataLineGraph::AddData(dataptr data)
{
    return AddDataToPathGeometry(data);
}

inline void CDataLineGraph::AddDataToPathGeometry(dataptr new_data)
{
#ifdef _DEBUG
    /*
    TCHAR name[MAX_PATH];
    CChineseCodeLib::Gb2312ToUnicode(name, MAX_PATH, _name);
    MYTRACE(
        L"%s %.02f, %.02f\n",
        name,
        *(float*)((char*)(new_data) + _data_x_offset),
        *(float*)((char*)(new_data) + _data_y_offset));
        */
#endif //_DEBUG

    if (_path_type == CDataLineGraph::GEOMETRY_PATH_TYPE_BEZIER) {
        if (psForSetData._count > 2) {
            _pSink->AddBezier(
                D2D1::BezierSegment(
                    D2D1::Point2F(psForSetData._pntOld.x, psForSetData._pntOld.y),
                    D2D1::Point2F(psForSetData._pntNew.x, psForSetData._pntNew.y),
                    D2D1::Point2F(*(float*)((char*)(new_data) + _data_x_offset),
                                  *(float*)((char*)(new_data) + _data_y_offset))));
        }
        psForSetData._pntOld = psForSetData._pntNew;
    } else if (_path_type == CDataLineGraph::GEOMETRY_PATH_TYPE_LINE) {
        _pSink->AddLine(
            D2D1::Point2F(
                *(float*)((char*)(new_data) + _data_x_offset),
                *(float*)((char*)(new_data) + _data_y_offset)));
    }
    psForSetData._pntNew.x = *(float*)((char*)(new_data) + _data_x_offset), psForSetData._pntNew.y = *(float*)((char*)(new_data) + _data_y_offset);
    psForSetData._count++;
}

inline void CDataLineGraph::EndSetData()
{
	_pSink->EndFigure(D2D1_FIGURE_END_OPEN);

    _pSink->Close();
	SafeRelease(&_pSink);
	_changed_type = (GLYPH_CHANGED_TYPE)((int)_changed_type | (int)GLYPH_CHANGED_TYPE_CHANGED);
#ifdef _DEBUG
    //TCHAR name[MAX_PATH];
    //CChineseCodeLib::Gb2312ToUnicode(name, MAX_PATH, _name);
    //MYTRACE(L"CDataLineGraph::EndSetData %s\n", name);
#endif //_DEBUG
}


void CDataLineGraph::MoveBitmapToLeft()
{
    float delta = _referframe->GetXScale();
    D2D1_RECT_F rectDest = D2D1::RectF(0.0f , 0.0f, fRectWidth(_rect) - delta, fRectHeight(_rect));
    D2D1_RECT_F rectSrc  = D2D1::RectF(delta, 0.0f, fRectWidth(_rect), fRectHeight(_rect));

    CriticalLock::Scoped scope(_myown_artist->_lock_artist);

    eArtist* _back_artist = _artist;
    _artist = _myown_artist;

    _artist->BeginBmpDraw(true);

    _artist->GetTransform(&_backup_trans);
    MATRIX_2D id = D2D1::Matrix3x2F::Identity();
    _artist->SetTransform(&id);

    _artist->DrawBitmap(_artist->GetDefaultBmp(), rectDest, rectSrc);

    _artist->SetTransform(&_backup_trans);

    _artist->EndBmpDraw();

    _artist = _back_artist;
}

} //namespace WARMGUI