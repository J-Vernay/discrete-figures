#pragma once

#include <raylib.h>

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

    void UpdateEnumeration();

    // Martin algorithm step-by-step management.
    void UpdateMartinAlgorithm();
    bool _showingState = false;

    void UpdateRandomGeneration();

    void UpdateAnalysis();

    void UpdateAbout();

};
