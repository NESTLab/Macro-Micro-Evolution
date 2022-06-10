#ifndef __VISUAL_EVO_H__
#define __VISUAL_EVO_H__

#include "olcPGE.h"
#include "operators.h" // ability to read points

#include "syslog.h"
#include "clock.h"
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

class OlcWindow;

class VisualEvo {
public:
    class Graph {

        VisualEvo* evo;
        OlcWindow* window;
        olc::Sprite* surface;
        std::shared_ptr<olc::Decal> texture;
        float xscale, yscale;
        int32_t xoff, yoff;

    public:
        enum DrawType {
            LINES, DOTS, POINTS, CUBES, BUBBLES, BOXES, GAIO
        };

        Graph(VisualEvo* evo, uint32_t w, uint32_t h, float xs, float ys);
        virtual ~Graph();

        void drawAxis(); // draw the graph axis (this is automatically drawn)
        void drawAdd(const Operators::EqPoints &points, const olc::Pixel& color=0xFFFFFFFF, const DrawType& type=LINES); // add computed graph data
        void drawClear(); // clear graph data
    };

    typedef std::shared_ptr<VisualEvo::Graph> GraphPtr;

private:
    OlcWindow* window;
    std::thread* handle;
    uint32_t w,h;
    std::vector<GraphPtr> graphsLeak;
    olc::Sprite* agSprite;

    void beginCommunicate();
    void endCommunicate();

public:
    olc::v2d_generic<uint32_t> resolution;

    VisualEvo(uint32_t w, uint32_t h, uint32_t resx = 512, uint32_t resy = 512);
    virtual ~VisualEvo();

    bool isRunning();


    GraphPtr createGraph(float xscale, float yscale);
    // void drawLine(int32_t x, int32_t y, int32_t x2, int32_t y2, uint32_t color=0xFFFFFFFF, uint32_t pattern=0xFFFFFFFF);
    // void drawPoint(int32_t x, int32_t y, uint32_t color=0xFFFFFFFF);

    static void visualHandle(VisualEvo* _this);
};


class OlcWindow : olc::PixelGameEngine {
protected:
    friend class VisualEvo;
    friend class VisualEvo::Graph;
    olc::v2d_generic<uint32_t>& resolution;
    std::atomic<bool> running;
    std::mutex externalConnect;

    struct View {
        olc::vf2d position;
        olc::vf2d scale;
    } view;


    enum RequestValue {
        NONE, DECAL
    } requestValue;
    void* returnValue;
    void* paramValue;

    std::mutex internalConnect;
    std::shared_ptr<olc::Decal> makeDecal(olc::Sprite* sprite);

public:
    std::vector<std::shared_ptr<olc::Decal>> textures;

    OlcWindow(const std::string& title, olc::v2d_generic<uint32_t>& resolution);
    virtual ~OlcWindow();

    bool OnUserCreate() override;
    
    bool OnUserUpdate(float elapsedTime) override;

};








#endif //__VISUAL_EVO_H__