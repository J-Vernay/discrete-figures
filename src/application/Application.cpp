#include "Application.hpp"
#include <raylib.h>
#include "raygui.h"

#include <array>
#include <cstdlib>

#if defined(_WIN32)
#define OPEN_URL(url) std::system("start " url)
#elif defined(__APPLE__)
#define OPEN_URL(url) std::system("open " url)
#elif defined(__linux__)
#define OPEN_URL(url) std::system("xdg-open " url)
#endif


constexpr int INIT_WIDTH = 800;
constexpr int INIT_HEIGHT = 600;
constexpr char const* WINDOW_TITLE = "Discrete figures app";

constexpr int TAB_HEIGHT = 30;
constexpr int TAB_FONT_HEIGHT = TAB_HEIGHT - 5;

Application::Application() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(INIT_WIDTH, INIT_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(20); // This is not a real-time app: low framerate is sufficient.
}

Application::~Application() {
    CloseWindow();
}

void Application::Run() {
    while (!WindowShouldClose()) {
        Application::Update();
    }
}

void Application::Update() {
    _width = GetScreenWidth();
    _height = GetScreenHeight();
    BeginDrawing();
    ClearBackground(RAYWHITE);
    UpdateTabs();

    switch (_tabIndex) {
    case 0: UpdateEnumeration(); break;
    case 1: UpdateMartinAlgorithm(); break;
    case 2: UpdateRandomGeneration(); break;
    case 3: UpdateAnalysis(); break;
    default: UpdateAbout(); break;
    }

    EndDrawing();
}

void Application::UpdateTabs() {
    static constexpr std::array<char const*, 4> TABS = {
        "Enumeration", "Martin Algorithm", "Random Generation", "Analysis"
    };

    // Separator line between tabs and content
    DrawLineEx(Vector2{0, TAB_HEIGHT-1}, Vector2{_width, TAB_HEIGHT-1}, 2, GRAY);

    GuiSetStyle(DEFAULT, TEXT_SIZE, TAB_FONT_HEIGHT - 5);
    Rectangle tab_location = { 0, 0, 0, TAB_HEIGHT };
    for (int i = 0; i < TABS.size(); ++i) {
        tab_location.width = MeasureText(TABS[i], TAB_FONT_HEIGHT);
        if (_tabIndex == i) { // current tab
            GuiButton_EX_State(tab_location, TABS[i], GUI_STATE_PRESSED);
        } else { // other tab
            if (GuiButton(tab_location, TABS[i]))
                _tabIndex = i;
        }
        tab_location.x += tab_location.width - 2;
    }

    // Adding help tab separately, so it is on the right of the tabs bar.
    float help_width = MeasureText("About", TAB_FONT_HEIGHT);
    Rectangle help_location = { _width - help_width, 0, help_width, TAB_HEIGHT };
    if (_tabIndex == TABS.size()) { // Help is current tab
        GuiButton_EX_State(help_location, "About", GUI_STATE_PRESSED);
    } else {
        if (GuiButton(help_location, "About"))
            _tabIndex = TABS.size();
    }
}


void Application::UpdateEnumeration() {

}

void Application::UpdateMartinAlgorithm() {
    
}

void Application::UpdateRandomGeneration() {
    
}

void Application::UpdateAnalysis() {
    
}

void Application::UpdateAbout() {
    constexpr char const* TITLE = "About this application...";
    constexpr int TITLE_SIZE = 40;
    int title_width = MeasureText(TITLE, TITLE_SIZE);
    DrawText("About this application...", (_width - title_width)/2, TAB_HEIGHT + 50, TITLE_SIZE, BLACK);

    constexpr char const* TEXT[] = {
        "This app was made by Julien Vernay to accompany a research paper ",
        "cowritten with Hugo Tremblay on the generation of discrete figures.",
        "",
        "It contains an illustration of Martin algorithm, along with random",
        "variants of the algorithm and an empirical analysis."
    };
    constexpr int TEXT_SIZE = 20;
    constexpr int LINE_WIDTH = 700;
    Vector2 text_pos = { (_width - LINE_WIDTH)/2, TAB_HEIGHT + 50 + TITLE_SIZE + 50 };

    for (char const* line : TEXT) {
        DrawText(line, text_pos.x, text_pos.y, TEXT_SIZE, BLACK);
        text_pos.y += TEXT_SIZE * 2;
    }

    constexpr int LINK_HEIGHT = TEXT_SIZE + 10;
    Rectangle link_location = { text_pos.x + 100, text_pos.y + 50, LINE_WIDTH - 200, LINK_HEIGHT };
    if (GuiButton(link_location, "See source code"))
        OPEN_URL("http://github.com/J-Vernay/discrete-figures");
    link_location.y += LINK_HEIGHT + 10;
    if (GuiButton(link_location, "See research paper"))
        OPEN_URL("http://www.example.org");
    link_location.y += LINK_HEIGHT + 10;
    if (GuiButton(link_location, "Graphics library: raylib and raygui"))
        OPEN_URL("http://www.raylib.com");
    
    DrawText("Licensed under MIT license (Julien Vernay 2022)", 10, _height - TEXT_SIZE -10, TEXT_SIZE, GRAY);
}