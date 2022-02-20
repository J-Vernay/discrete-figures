#pragma once

#include <raylib.h>
#include "../MartinAlgoSimple.hpp"

class Application {
public:
    // Constructor. Initializes the application and prepare to be drawn.
    Application();
    // Destructor, close window.
    ~Application();

    // Main loop, doing event processing and drawing.
    void Run();

private:
    float _width, _height;

    // Process one step, redrawing the interface and updating state.
    void Update();

    // Tab management.
    void UpdateTabs();
    int _tabIndex = 0;

    // Martin algorithm step-by-step management.
    void UpdateMartinAlgorithm();
    MartinAlgoSimple _martin = {};
    bool _showingState = true;
    int _maxLevel = 8;
    int _whiteconnexity = 0;

    void UpdateAbout();
};
