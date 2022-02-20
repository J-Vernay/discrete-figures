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


constexpr int INIT_WIDTH = 1200;
constexpr int INIT_HEIGHT = 800;
constexpr char const* WINDOW_TITLE = "Discrete figures app";

constexpr int TAB_HEIGHT = 40;
constexpr int TAB_FONT_HEIGHT = TAB_HEIGHT - 5;

Application::Application() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(INIT_WIDTH, INIT_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(20); // This is not a real-time app: low framerate is sufficient.
    _martin.Init();
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

    switch (_tabIndex) {
    case 0: UpdateMartinAlgorithm(); break;
    default: UpdateAbout(); break;
    }

    UpdateTabs();

    EndDrawing();
}

void Application::UpdateTabs() {
    static constexpr std::array<char const*, 1> TABS = {
        //"Enumeration", "Martin Algorithm", "Random Generation", "Analysis"
        "Martin Algorithm"
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


char buffer[200];

void Application::UpdateMartinAlgorithm() {
    constexpr float BUTTON_SIZE = 40;
    constexpr float BUTTON_MARGIN = 10;
    constexpr float BUTTON_YPOS = TAB_HEIGHT + BUTTON_MARGIN;
    constexpr float GRID_YPOS = BUTTON_YPOS + BUTTON_SIZE + BUTTON_MARGIN;
    constexpr float CELL_FONT_SIZE = 20;
    constexpr float CELL_SIZE = 55;
    
    int MAX_FIGURE_SIZE = std::max(10, _maxLevel);
    float GRID_WIDTH = (MAX_FIGURE_SIZE * 2 + 1) * CELL_SIZE;
    float GRID_HEIGHT = (MAX_FIGURE_SIZE + 1) * CELL_SIZE;

    Rectangle grid_location = { (_width - GRID_WIDTH) / 2, GRID_YPOS, GRID_WIDTH, GRID_HEIGHT };
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt(BLACK));

    // Display candidates state
    for (int i = 0; i < _martin.candidates.size(); ++i) {
        // Calculate on-screen coordinates.
        Candidate& candidate = _martin.candidates[i];
        auto [x,y] = candidate.coordinate;
        int true_x = x + MAX_FIGURE_SIZE, true_y = MAX_FIGURE_SIZE - y;
        Rectangle cell = { grid_location.x + true_x * CELL_SIZE, grid_location.y + true_y * CELL_SIZE, CELL_SIZE, CELL_SIZE };
        // Background color.
        switch (candidate.s) {
        case Candidate::Free: DrawRectangleRec(cell, ColorAlpha(YELLOW, 0.3)); break;
        case Candidate::Chosen: DrawRectangleRec(cell, ColorAlpha(BLUE, 0.5)); break;
        case Candidate::Prohibited: DrawRectangleRec(cell, ColorAlpha(RED, 0.3)); break;
        }
        // Text, which must be centered.
        char const* text;
        if (_showingState) {
            if (candidate.s == Candidate::Free)
                text = "F";
            else
                text = TextFormat("%c %d", Candidate::StateLetter[candidate.s], candidate.i);
        } else {
            text = TextFormat("%d", i);
        }
        int text_width = MeasureText(text, CELL_FONT_SIZE);
        DrawText(text, cell.x + (CELL_SIZE - text_width) / 2, cell.y + (CELL_SIZE - CELL_FONT_SIZE)/2, CELL_FONT_SIZE, BLACK);
    }
    // Display grid
    Vector2 cell_pos = GuiGrid(grid_location, CELL_SIZE, 1);
    // Gray out cell under mouse
    if (cell_pos.x >= 0 && cell_pos.y >= 0) {
        Vector2 pos_px = { grid_location.x + cell_pos.x * CELL_SIZE, grid_location.y + cell_pos.y * CELL_SIZE };
        DrawRectangle(pos_px.x, pos_px.y, CELL_SIZE, CELL_SIZE, ColorAlpha(BLACK, 0.3));
        DrawRectangleLines(pos_px.x, pos_px.y, CELL_SIZE+1, CELL_SIZE+1, BLACK);
    }

    // Display buttons
    float SHOW_BUTTON_WIDTH = (grid_location.width - 5 * BUTTON_MARGIN)  * 2/7.f;
    Rectangle button_location = { grid_location.x, BUTTON_YPOS, SHOW_BUTTON_WIDTH, BUTTON_SIZE };
    if (_showingState) {
        if (GuiButton(button_location, "Show cells ordering")) {
            _showingState = false;
        }
    } else {
        if (GuiButton(button_location, "Show cells state")) {
            _showingState = true;
        }
    }
    button_location.x += button_location.width + BUTTON_MARGIN;
    button_location.width = SHOW_BUTTON_WIDTH / 2;
    if (GuiButton(button_location, "Advance")) {
        if (_martin.level == _maxLevel || _martin.next_free == _martin.candidates.size()) {
            _martin.Pop();
        } else {
            Coordinate coord = _martin.Push(_martin.next_free);
            _martin.AddCandidates4(coord);
        }
    }
    button_location.x += button_location.width + BUTTON_MARGIN;
    if (GuiButton(button_location, "Next figure")) {
        // Same as "advance", but until level = _maxLevel or if no more push allowed
        do {
            if (_martin.level >= _maxLevel || _martin.next_free == _martin.candidates.size()) {
                if (!_martin.Pop()) // Cannot pop, out of figures
                    break;
            } else {
                Coordinate coord = _martin.Push(_martin.next_free);
                if (_whiteconnexity == 4 && _martin.WouldBreakWhiteLocal4(coord)
                        || _whiteconnexity == 8 && _martin.WouldBreakWhiteLocal8(coord)) {
                    _martin.Pop(); // Skip this figure.
                }
                else {
                    _martin.AddCandidates4(coord);
                }
            }
        } while (_martin.level != _maxLevel);
    }
    button_location.x += button_location.width + BUTTON_MARGIN;
    if (GuiButton(button_location, "Reset")) {
        _martin.Init(MAX_FIGURE_SIZE);
    }
    constexpr float SPINNER_MARGIN = 80;
    button_location.x += button_location.width + BUTTON_MARGIN + SPINNER_MARGIN;
    button_location.width -= SPINNER_MARGIN;
    GuiSpinner(button_location, "Level", &_maxLevel, 1, 20, false);
    button_location.x += button_location.width + BUTTON_MARGIN + SPINNER_MARGIN;
    GuiSpinner(button_location, "W", &_whiteconnexity, 0, 8, false);
    if (_whiteconnexity == 1) _whiteconnexity = 4;
    if (_whiteconnexity == 3) _whiteconnexity = 0;
    if (_whiteconnexity == 5) _whiteconnexity = 8;
    if (_whiteconnexity == 7) _whiteconnexity = 4;
}

void Application::UpdateAbout() {
    constexpr char const* TITLE = "About this application...";
    constexpr int TITLE_SIZE = 50;
    int title_width = MeasureText(TITLE, TITLE_SIZE);
    DrawText("About this application...", (_width - title_width)/2, TAB_HEIGHT + 50, TITLE_SIZE, BLACK);

    constexpr char const* TEXT[] = {
        "This app was made by Julien Vernay to accompany a research paper ",
        "cowritten with Hugo Tremblay on the generation of discrete figures.",
        "",
        "It contains an illustration of Martin algorithm, along with random",
        "variants of the algorithm and an empirical analysis."
    };
    constexpr int TEXT_SIZE = 30;
    constexpr int LINE_WIDTH = 1050;
    Vector2 text_pos = { (_width - LINE_WIDTH)/2, TAB_HEIGHT + 50 + TITLE_SIZE + 50 };

    for (char const* line : TEXT) {
        DrawText(line, text_pos.x, text_pos.y, TEXT_SIZE, BLACK);
        text_pos.y += TEXT_SIZE * 2;
    }

    constexpr int LINK_HEIGHT = TEXT_SIZE + 10;
    Rectangle link_location = { text_pos.x + 250, text_pos.y + 50, LINE_WIDTH - 500, LINK_HEIGHT };
    if (GuiButton(link_location, "See source code"))
        OPEN_URL("http://github.com/J-Vernay/discrete-figures");
    link_location.y += LINK_HEIGHT + 10;
    if (GuiButton(link_location, "See research paper"))
        OPEN_URL("http://www.example.org");
    link_location.y += LINK_HEIGHT + 10;
    if (GuiButton(link_location, "Graphics library: raylib and raygui"))
        OPEN_URL("http://www.raylib.com");
    
    DrawText("Licensed under MIT license (Julien Vernay 2022 jvernay.fr)", 10, _height - TEXT_SIZE -10, TEXT_SIZE, GRAY);
}