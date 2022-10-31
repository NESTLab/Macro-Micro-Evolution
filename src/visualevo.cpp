#include "visualevo.h"

#ifdef WEXTRA_COMPONENTS

#include "memloader.h"
#include "aspr.h"

#endif

/*	External Interface */

VisualEvo::VisualEvo(uint32_t w, uint32_t h, uint32_t resx, uint32_t resy):
	w(w), h(h), agSprite(nullptr), resolution({resx, resy}) {
	window = new OlcWindow("EvoAlgo 3.0 Visual Inspector", resolution);
	handle = new std::thread(&visualHandle, this);
}

VisualEvo::~VisualEvo() {
	if(window != nullptr){
		window->running = false; // kill event loop
	}
	handle->join(); // end thread - deletes window automatically
	delete handle; // free thread
}

bool VisualEvo::isRunning() {
	std::scoped_lock lock(wMtx);
	return window == nullptr ? false : (!!window->running);
}

void VisualEvo::beginCommunicate() {
	window->externalConnect.lock(); // lock for sync communication
}

void VisualEvo::endCommunicate() {
	window->externalConnect.unlock(); // lock for sync communication
}

VisualEvo::GraphPtr VisualEvo::createGraph(float xscale, float yscale) {
	GraphPtr graph = std::make_shared<Graph>(this, resolution.x, resolution.y, xscale, yscale);
	graphsLeak.push_back(graph);
	return graph;
}

/*
void VisualEvo::drawPoint(int32_t x, int32_t y, uint32_t color) {
	if(!window->running) return;
	if(window->surface == nullptr) return;

	beginCommunicate();

	window->SetDrawTarget(window->surface); // surface
	window->Draw(x,y,color);
	window->SetDrawTarget(nullptr); // reset

	endCommunicate();
}

void VisualEvo::drawLine(int32_t x, int32_t y, int32_t x2, int32_t y2, uint32_t color, uint32_t pattern) {
	if(!window->running) return;
	if(window->surface == nullptr) return;

	beginCommunicate();
	window->SetDrawTarget(window->surface); // surface

	window->DrawLine(x,y,x2,y2,color,pattern);
	window->SetDrawTarget(nullptr); // reset

	endCommunicate();
}
*/

void VisualEvo::visualHandle(VisualEvo* _this) {

	#ifdef WEXTRA_COMPONENTS
	if(_this->agSprite == nullptr){
		memloader::MemoryImageLoader loader;
		_this->agSprite = new olc::Sprite;
		if(!loader.LoadImageResource(_this->agSprite, resource26840, sizeof(resource26840))){
			delete _this->agSprite;
			_this->agSprite = nullptr;
		}
	}
	#else
		_this->agSprite = new olc::Sprite(16,16);
		olc::Pixel* p = _this->agSprite->GetData();
		for(int i=0; i < 16*16; ++i) *p++ = olc::WHITE;
	#endif

	if(_this->window->Construct(_this->w, _this->h, 1, 1)){
		_this->window->Start();
	}
	// for(GraphPtr g : _this->graphsLeak){
		// g.reset();
	// }
	_this->graphsLeak.clear();
	{
		std::scoped_lock lock(_this->wMtx);
		_this->window->running = false;
		delete _this->window;
		_this->window = nullptr;
	}

	delete _this->agSprite;
	_this->agSprite = nullptr;
}

/* Internal Things */

VisualEvo::Graph::Graph(VisualEvo* evo, uint32_t w, uint32_t h, float xs, float ys):
	evo(evo), window(evo->window), xoff(0), yoff(0), surface(nullptr) {
		if(w == 0) w = window->ScreenWidth();
		if(h == 0) h = window->ScreenHeight();

		xscale = float(w) / float(window->ScreenWidth()) * xs;
		yscale = float(h) / float(window->ScreenHeight()) * ys;

		evo->beginCommunicate();
		surface = new olc::Sprite(w, h);
		texture = window->makeDecal(surface);
		
		window->textures.push_back(texture);
		evo->endCommunicate();

		drawAxis();

	}

VisualEvo::Graph::~Graph() {
	delete surface;
}

void VisualEvo::Graph::drawAxis() {
	if(evo->window == nullptr) return;

	evo->beginCommunicate();
	window->SetDrawTarget(surface); // surface
	window->DrawLine(0, surface->height / 2 - yoff, surface->width, surface->height / 2 - yoff);// x axis
	window->DrawLine(surface->width / 2 + xoff, 0, surface->width / 2 + xoff, surface->height);			// y axis

	evo->endCommunicate();
}

void VisualEvo::Graph::drawAdd(const Operators::EqPoints &points, uint32_t varIndex, const olc::Pixel& color, const DrawType& type) {
	if(evo->window == nullptr){
		syslog::cout << "window not loaded\n";
		return;
	}

	std::vector<olc::vf2d> dataPoints;
	for(size_t i=0; i < points.points.size(); ++i){
		dataPoints.push_back({
			float(points.points[i][varIndex]), // x
			float(points.results[i]) 	// y
		});
	}

	evo->beginCommunicate();
	window->SetDrawTarget(surface); // surface
	window->SetPixelBlend(0.75f);

	for(size_t i=0; i+1 < dataPoints.size(); ++i){
		switch(type){
			case LINES:
				window->DrawLine(
					dataPoints[i].x*xscale + surface->width / 2 + xoff,		// x1
					surface->height - dataPoints[i].y*yscale - yoff - surface->height / 2, 	// y1
					dataPoints[i+1].x*xscale + surface->width / 2 + xoff,	// x2
					surface->height - dataPoints[i+1].y*yscale - yoff - surface->height / 2,// y2
				color);
			break;
			case DOTS:
				window->Draw(
					dataPoints[i].x*xscale + surface->width / 2 + xoff,		// x1
					surface->height - dataPoints[i].y*yscale - yoff - surface->height / 2, 	// y1
					color);
			break;
			case POINTS:
				window->FillCircle(
					dataPoints[i].x*xscale + surface->width / 2 + xoff,		// x1
					surface->height - dataPoints[i].y*yscale - yoff - surface->height / 2, 	// y1
					2,// radius
					color);
			break;
			case BUBBLES:
				window->DrawCircle(
					dataPoints[i].x*xscale + surface->width / 2 + xoff,		// x1
					surface->height - dataPoints[i].y*yscale - yoff - surface->height / 2, 	// y1
					2,// radius
					color);
			break;
			case CUBES:
				window->FillRect(
					dataPoints[i].x*xscale + surface->width / 2 + xoff - 2,		// x1
					surface->height - dataPoints[i].y*yscale - yoff - surface->height / 2 - 2, 	// y1
					4,// width
					4,// height
					color);
			break;
			case BOXES:
				window->DrawRect(
					dataPoints[i].x*xscale + surface->width / 2 + xoff - 2,		// x1
					surface->height - dataPoints[i].y*yscale - yoff - surface->height / 2 - 2, 	// y1
					4,// width
					4,// height
					color);
			break;
			/* unavailable
			case GAIO:
				if(evo->agSprite == nullptr) break;
				window->DrawSprite(
					dataPoints[i].x*xscale + surface->width / 2.0f + xoff - evo->agSprite->width / 2,		// x1
					surface->height - dataPoints[i].y*yscale - yoff - surface->height / 2.0f - evo->agSprite->height / 2, 	// y1
					evo->agSprite);
			break; */
		}


	}

	window->SetPixelBlend(1.0f);

	evo->endCommunicate();
}

void VisualEvo::Graph::drawClear() {
	if(evo->window == nullptr) return;
	
	evo->beginCommunicate();
	window->SetDrawTarget(surface); // surface
	window->Clear(olc::BLANK);

	evo->endCommunicate();

	drawAxis(); // re-add axis
}

/* Window Interface */

OlcWindow::OlcWindow(const std::string& title, olc::v2d_generic<uint32_t>& resolution):
	resolution(resolution), running(false), requestValue(NONE), returnValue(nullptr), paramValue(nullptr) {
	sAppName = title;
	SetPixelMode(olc::Pixel::ALPHA);
}

OlcWindow::~OlcWindow() {
	std::scoped_lock lock(externalConnect);
	textures.clear();
}

std::shared_ptr<olc::Decal> OlcWindow::makeDecal(olc::Sprite* sprite) {
	std::shared_ptr<olc::Decal> decal;
	do {
		std::scoped_lock lock(internalConnect);
		if(returnValue != nullptr){
			decal.reset((olc::Decal*)returnValue);
			break;
		} else {
			paramValue = sprite;
			requestValue = DECAL;
		}
	} while(1);
	return decal;
}

bool OlcWindow::OnUserCreate() {

	view.position = {resolution.x / 2.0f, resolution.y / 2.0f};
	view.scale = { float(ScreenWidth()) / float(resolution.x), float(ScreenHeight()) / float(resolution.y)};

	running = true;
	return true;
}
	
bool OlcWindow::OnUserUpdate(float elapsedTime) {
	
	{ // event check for requested values from another thread
		std::scoped_lock lock(internalConnect);
		switch(requestValue){
			case NONE:
				returnValue = nullptr;
				paramValue = nullptr;
				break;
			case DECAL: // generate an openGL texture
				returnValue = new olc::Decal((olc::Sprite*)paramValue);
				break;
		}
		requestValue = NONE;
	}
	
	bool ctrl = GetKey(olc::Key::CTRL).bHeld;

	if(GetKey(olc::Key::UP).bHeld) {
		if(ctrl){
			view.scale *= {1.01f, 1.01f};
		} else {
			view.position.y -= 1.0f / view.scale.y;
		}
	}
	if(GetKey(olc::Key::DOWN).bHeld) {
		if(ctrl){
			view.scale /= {1.01f, 1.01f};
		} else {
			view.position.y += 1.0f / view.scale.y;
		}
	}

	if(GetKey(olc::Key::LEFT).bHeld) {
		view.position.x -= 1.5f / view.scale.x;
	}
	if(GetKey(olc::Key::RIGHT).bHeld) {
		view.position.x += 1.5f / view.scale.x;
	}

	if(externalConnect.try_lock()){
		static Clock refresh;
		if(refresh.getSeconds() > 0.8f) {
			for(auto& tex : textures) tex->Update();
			refresh.restart();
		}

		SetDrawTarget(nullptr);
		Clear(olc::BLANK);

		for(auto& tex : textures){
			DrawRotatedDecal({ScreenWidth() / 2.0f, ScreenHeight() / 2.0f}, tex.get(), 0.0f, view.position, view.scale);
		}
		externalConnect.unlock();
	}

	return running;
}