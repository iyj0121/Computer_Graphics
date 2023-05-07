//
//  main.cpp
//  CurveInter
//
//  Created by Hyun Joon Shin on 2021/03/24.
//

#include <iostream>
#include <JGL/JGL.hpp>
#include <JGL/JGL_Window.hpp>
#include <JGL/JGL_Widget.hpp>
#include <JGL/JGL_Options.hpp>
#include <JGL/JGL_Toolbar.hpp>
#include <JGL/JGL_Aligner.hpp>



enum {
	LAGLANGIAN,
	LINEAR,
	BEZIER,
	CATMULL,
	BSPLINE,
	NATURAL,
};

enum {
	DRAW_LINES,
	DRAW_DOTS,
};

int curveType = LINEAR;
int drawType = DRAW_LINES;
bool closed = false;


std::vector<glm::vec2> srcPts;
std::vector<glm::vec2> samplePts;

glm::vec2 evaluateCurve( int curveType, const std::vector<glm::vec2>& srcPts, bool closed, size_t k, float t ) {//p는 k번째에서 0부터 1까지
	using std::vector;
	using glm::vec2;
	glm::vec2 ret(0,0);
	switch( curveType ) {
		case LAGLANGIAN: { //local controllability가 없다(다른 곳에 영향을 준다)/ convexhull, variation diminishing만족x
			float T = k+t;
			for( auto i=0; i<srcPts.size(); i++ ) {
				float L = 1;
				for( auto j=0; j<srcPts.size(); j++ ) {
					if( j!=i ) {
						L/=(i-j);
						L*=T-j;
					}
				}
				ret+=L*srcPts[i];
			}
		} break;
		
		case CATMULL: //dot으로 보면 속도가 나온다
		{
			glm::vec2 v0(0), v1(0);
			if (k > 0) {
				v0 = (srcPts[k + 1] - srcPts[k - 1]) / 2.f;
			}
			else {//행렬을 세로로 쓴다
				glm::mat3 A = { 0,1,4,0,1,2,1,1,1 };
				{
					glm::vec3 b = { srcPts[0].x, srcPts[1].x , srcPts[2].x };
					glm::vec3 x = glm::inverse(A) * b; //이 점을 만족하는 parabola(2차 포물선)가 있었을 것. 이걸 미분
					v0.x = x.y;
				}
				glm::vec3 b = { srcPts[0].y, srcPts[1].y , srcPts[2].y };
				glm::vec3 x = glm::inverse(A) * b; //이 점을 만족하는 parabola(2차 포물선)가 있었을 것. 이걸 미분
				v0.y = x.y;

			}
			if (k < srcPts.size() - 2) {
				v1 = (srcPts[k + 2] - srcPts[k]) / 2.f;
			}
			glm::vec2 c0 = srcPts[k] + v0 / 3.f;
			glm::vec2 c1 = srcPts[k + 1] - v1 / 3.f;
			return (1 - t) * (1 - t) * (1 - t) * srcPts[k] + 3 * t * (1 - t) * (1 - t) * c0
				+ 3 * t * t * (1 - t) * c1 + t * t * t * srcPts[k + 1];
		}break;

		case BEZIER: 
		{ //p가 0부터 1일때 까지만
			if (k == 0) { //k가 0일떄 0, 1, 2, 3번만 만든다
				return (1 - t) * (1 - t) * (1 - t) * srcPts[0] + 3 * t * (1 - t) * (1 - t) * srcPts[1]
					+ 3 * t * t * (1 - t) * srcPts[2] + t * t * t * srcPts[3];
			}
			else {
				return srcPts[3];
			}
		}break;

		case NATURAL:
		{

			size_t N = srcPts.size();
			//Dx = b - Lx - Ux
			//한개의 element만 생각 
			//4[i,i]*x[i] = b[i] - x[i+1] - x[i-1] if i!=0 or i!=N-1
			vector<float> Dx;
			vector<float> Dy;
			{
				vector<float> D0(N, 0); //점이 N개가 있고 선이 N-1개 우선 시작은 다 0으로
				vector<float> D1(N, 0);
				for (int i = 0; i < 10; i++) { //diagonal이 많이 dominant할 수록 다른거에 비해서 크면 적은 반복으로도 가능하다
					vector<float>& x_n = (i % 2 == 0) ? D0 : D1;
					vector<float>& x_n1 = (i % 2 == 0) ? D1 : D0;
					for (int j = 1; j < x_n.size() - 1; j++) {
						x_n1[j] = (3 * (srcPts[j + 1].x - srcPts[j - 1].x) - x_n[j + 1] - x_n[j - 1]) / 4; //이게 b에 해당한다
					}
				}
				Dx = D0;
			}
			{
				vector<float> D0(N, 0); //점이 N개가 있고 선이 N-1개 우선 시작은 다 0으로
				vector<float> D1(N, 0);
				for (int i = 0; i < 10; i++) {
					vector<float>& x_n = (i % 2 == 0) ? D0 : D1;
					vector<float>& x_n1 = (i % 2 == 0) ? D1 : D0;
					for (int j = 1; j < x_n.size() - 1; j++) {
						x_n1[j] = (3 * (srcPts[j + 1].y - srcPts[j - 1].y) - x_n[j + 1] - x_n[j - 1]) / 4; //이게 b에 해당한다
					}
				}
				Dy = D0;
			}
			vec2 a = srcPts[k];
			vec2 b = vec2(Dx[k], Dy[k]);
			vec2 c = 3.f * (srcPts[k + 1] - srcPts[k]) - 2.f * vec2(Dx[k], Dy[k]) - vec2(Dx[k + 1], Dy[k + 1]);
			vec2 d = 2.f * (srcPts[k] - srcPts[k + 1]) + vec2(Dx[k], Dy[k]) + vec2(Dx[k + 1], Dy[k + 1]);
			return a + b * t + c * t * t + d * t * t * t;
		}break;

		case LINEAR : //convexhull, variation diminishing, affine invariance을 만족한다. C0 continuity, local control 가능
		default: {
			ret = (srcPts[k+1]-srcPts[k])*t + srcPts[k];
		}
	}
	return ret;
}

void updateCurve( int curveType, const std::vector<glm::vec2>& srcPts, bool closed ) {
	samplePts.clear();
	for( auto i=0; i<srcPts.size()-1; i++ ) {
		for( float t = 0; t<1; t+=0.02 ) {
			samplePts.push_back( evaluateCurve( curveType, srcPts, closed, i, t ) );
		}
	}
	if( closed ) {
		for( float t = 0; t<1; t+=0.02 ) {
			samplePts.push_back( evaluateCurve( curveType, srcPts, closed, srcPts.size()-1, t ) );
		}
	}
	else
		samplePts.push_back( evaluateCurve( curveType, srcPts, closed, srcPts.size()-2, 1 ) );
}


#include <JGL/nanovg/nanovg.h>


struct CurveWidget : JGL::Widget {
	CurveWidget(float x, float y, float w, float h, const std::string& title = "" )
	: JGL::Widget(x,y,w,h,title){}
	virtual void		drawBox(NVGcontext* vg, const glm::rect& r) override {
		nvgSave(vg);
		nvgBeginPath( vg );
		nvgRect( vg, r.x, r.y, r.w, r.h );
		nvgFillColor( vg, nvgRGBAf(0,0,0,1));
		nvgFill( vg );
		nvgRestore(vg);
	}
	
	virtual void drawContents(NVGcontext* vg, const glm::rect& r, int align ) override {
		nvgSave(vg);
		if( drawType == DRAW_LINES ) {
			nvgBeginPath( vg );
			nvgMoveTo( vg, samplePts[0].x, samplePts[0].y );
			for( auto i=1; i<samplePts.size(); i++ ) {
				nvgLineTo( vg, samplePts[i].x, samplePts[i].y );
			}
			nvgStrokeColor(vg, nvgRGBAf(0,.8f,1,1));
			nvgStrokeWidth(vg, 2);
			nvgStroke( vg );
		}
		else {
			nvgFillColor(vg, nvgRGBAf(0,1,.3f,1));
			nvgBeginPath( vg );
			for( auto i=0; i<samplePts.size(); i++ ) {
				nvgCircle(vg, samplePts[i].x, samplePts[i].y, 1);
			}
			nvgFill( vg );
		}

		for( auto i=0; i<srcPts.size(); i++ )
			if( i!= underPt ) {
				nvgBeginPath( vg );
				nvgCircle( vg, srcPts[i].x, srcPts[i].y, 5 );
				nvgFillColor( vg, nvgRGBAf(1,1,0,.8f));
				nvgFill( vg );
			}
		if( underPt>=0 ) {
			nvgBeginPath( vg );
			nvgCircle( vg, srcPts[underPt].x, srcPts[underPt].y, 5 );
			nvgFillColor( vg, nvgRGBAf(1,.1f,0,.8f));
			nvgFill( vg );
		}
		nvgRestore(vg);
	}
	virtual bool handle( int event ) override {
		glm::vec2 pt = JGL::_JGL::eventPt();
		switch( event ) {
			case JGL::EVENT_MOVE : {
				int oldPt = underPt;
				underPt = -1;
				for( auto i=0; i<srcPts.size(); i++ ) {
					if( length(pt-srcPts[i])<6 )
						underPt = i;
				}
				if( underPt!= oldPt ) {
					redraw();
				}
			}break;
			case JGL::EVENT_PUSH : {
				if( underPt>=0 )
					ptOffset = srcPts[underPt]-pt;
			}break;
			case JGL::EVENT_DRAG : {
				if( underPt>=0 ) {
					srcPts[underPt] = pt+ptOffset;
					updateCurve( curveType, srcPts, closed );
					redraw();
				}
			}break;
		}
		return true;
	}
	int underPt = -1;
	glm::vec2 ptOffset = glm::vec2(0);
};


using namespace JGL;
CurveWidget* curveWidget;


void curveTypeCallback(Widget* w, void* ud ) {
	curveType = ((Options*)w)->value();
	//if (curveType == NATURAL) closed = true; else closed = false;
	updateCurve(curveType, srcPts, closed);
	curveWidget->redraw();
}

void drawTypeCallback(Widget* w, void* ud ) {
	drawType = ((Options*)w)->value();
	curveWidget->redraw();
}

int main(int argc, const char * argv[]) {
	Window* window = new Window(640,480,"Curves");
	window->alignment(ALIGN_ALL);
	
	Aligner* aligner = new Aligner(0,0,window->w(), window->h());
	aligner->type(Aligner::VERTICAL);
	aligner->alignment(ALIGN_ALL);
	
	Toolbar* toolbar = new Toolbar(0,0,window->w(), _size_toolbar_height() );
	Options* curveTypes = new Options(0,0,200,_size_button_height() );
	curveTypes->add("Lagrangian");
	curveTypes->add("Linear");
	curveTypes->add("Bezier");
	curveTypes->add("Catmull");
	curveTypes->add("Bspline");
	curveTypes->add("Natural Spline");
	curveTypes->value(curveType);
	curveTypes->callback( curveTypeCallback );
	Options* drawType = new Options(0,0,200,_size_button_height() );
	drawType->add("Lines");
	drawType->add("Dots");
	drawType->value(::drawType);
	drawType->callback( drawTypeCallback );
	toolbar->end();
	
	curveWidget = new CurveWidget(0,0,window->w(), window->h()-toolbar->h());
	aligner->resizable( curveWidget );
	aligner->end();
	
	for( auto i=0; i<9; i++ ) {
		srcPts.push_back(glm::vec2((i*0.1+0.1)*curveWidget->w(),curveWidget->h()/2));
	}
	updateCurve(curveType,srcPts,closed);
	
	window->show();
	_JGL::run();
	
	return 0;
}
