#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>
#include <array>
#include <cmath>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define BUTTON_L2 16
#define BUTTON_R2 17
#define MAX_BUTTON_INDEX 18

struct Color {
    uint8_t r, g, b, a;
    
    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}
        
    SDL_Color toSDLColor() const {
        return {r, g, b, a};
    }
};

class Button {
private:
    SDL_Rect rect;
    std::string name;
    bool pressed;
    Color normalColor;
    Color pressedColor;

public:
    Button() = default;
    
    Button(const SDL_Rect& rect, const std::string& name, const Color& normalColor, const Color& pressedColor)
        : rect(rect), name(name), pressed(false), normalColor(normalColor), pressedColor(pressedColor) {}

    const SDL_Rect& getRect() const { return rect; }
    const std::string& getName() const { return name; }
    bool isPressed() const { return pressed; }
    void setPressed(bool p) { pressed = p; }
    const Color& getCurrentColor() const { return pressed ? pressedColor : normalColor; }
};

class AnalogStick {
private:
    int centerX;
    int centerY;
    int radius;
    int stickRadius;
    float xValue;
    float yValue;
    std::string name;
    Color baseColor;
    Color stickColor;

public:
    AnalogStick() = default;
    
    AnalogStick(int centerX, int centerY, int radius, int stickRadius, const std::string& name, const Color& baseColor, const Color& stickColor)
        : centerX(centerX), centerY(centerY), radius(radius), stickRadius(stickRadius), xValue(0.0f), yValue(0.0f), name(name), baseColor(baseColor), stickColor(stickColor) {}

    int getCenterX() const { return centerX; }
    int getCenterY() const { return centerY; }
    int getRadius() const { return radius; }
    int getStickRadius() const { return stickRadius; }
    float getXValue() const { return xValue; }
    float getYValue() const { return yValue; }
    void setXValue(float value) { xValue = value; }
    void setYValue(float value) { yValue = value; }
    const std::string& getName() const { return name; }
    const Color& getBaseColor() const { return baseColor; }
    const Color& getStickColor() const { return stickColor; }
    
    SDL_Point getStickPosition() const {
        return {
            centerX + static_cast<int>(xValue * (radius - stickRadius)), centerY + static_cast<int>(yValue * (radius - stickRadius))
        };
    }
    
    // Get formatted value text
    std::string getValueText() const {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "X: %.2f Y: %.2f", xValue, yValue);
        return buffer;
    }
};

// Circle outline
void DrawCircle(SDL_Renderer* renderer, int x, int y, int radius, const Color& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int offsetx = 0;
    int offsety = radius;
    int d = radius - 1;
    
    while (offsety >= offsetx) {
        SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
        SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
        SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
        SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
        SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
        SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
        SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
        SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);
        
        if (d >= 2 * offsetx) {
            d -= 2 * offsetx + 1;
            offsetx += 1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }
}

void DrawFilledCircle(SDL_Renderer* renderer, int x, int y, int radius, const Color& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int offsetx = 0;
    int offsety = radius;
    int d = radius - 1;
    
    while (offsety >= offsetx) {
        SDL_RenderDrawLine(renderer, x - offsety, y + offsetx, x + offsety, y + offsetx);
        SDL_RenderDrawLine(renderer, x - offsetx, y + offsety, x + offsetx, y + offsety);
        SDL_RenderDrawLine(renderer, x - offsetx, y - offsety, x + offsetx, y - offsety);
        SDL_RenderDrawLine(renderer, x - offsety, y - offsetx, x + offsety, y - offsetx);
        
        if (d >= 2*offsetx) {
            d -= 2*offsetx + 1;
            offsetx += 1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }
}

void RenderTextCentered(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, const Color& color) {
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color.toSDLColor());
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect destRect = {x - surface->w / 2, y - surface->h / 2, surface->w, surface->h};
    
    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    
    // Clean up
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Main controller visualizer class
class ControllerVisualizer {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_GameController* controller;
    std::array<Button, MAX_BUTTON_INDEX> buttons;
    std::array<AnalogStick, 2> sticks;
    bool running = false;

public:
    ~ControllerVisualizer() {
        cleanup();
    }
    
    // Initialize the visualizer
    bool initialize() {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize SDL_ttf
        if (TTF_Init() == -1) {
            std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
            SDL_Quit();
            return false;
        }
        
        // Set video hints for embedded systems like Trimui
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "mali");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); // Maximum opengl es version is 3.2
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        
        // Create window
        window = SDL_CreateWindow("Controller Tester", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        
        if (!window) {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            return false;
        }
        
        // Create renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return false;
        }
        
        // Load font
        font = TTF_OpenFont("res/arial.ttf", 18);
        if (!font) {
            std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return false;
        }
        
        initController();
        
        setupLayout();
        
        running = true;
        return true;
    }
    
    void run() {
        SDL_Event e;
        
        while (running) {
            // Process events
            while (SDL_PollEvent(&e)) {
                handleEvent(e);
            }
            
            // Render current controller state
            render();
        }
    }

private:
    void cleanup() {
        SDL_GameControllerClose(controller);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }
    
    void initController() {
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                if (controller) {
                    std::cout << "Found controller: " << SDL_GameControllerName(controller) << std::endl;
                    break;
                }
            }
        }
        
        if (!controller) {
            std::cout << "Warning: No controller found" << std::endl;
        }
    }
    
    void setupLayout() {
        Color gray(100, 100, 100, 255);
        Color yellow(255, 255, 0, 255);
        
        // D-Pad
        buttons[SDL_CONTROLLER_BUTTON_DPAD_UP] = Button({240, 260, 60, 60}, "UP", gray, yellow);
        buttons[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = Button({240, 380, 60, 60}, "DOWN", gray, yellow);
        buttons[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = Button({180, 320, 60, 60}, "LEFT", gray, yellow);
        buttons[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = Button({300, 320, 60, 60}, "RIGHT", gray, yellow);
        
        // Face buttons
        buttons[SDL_CONTROLLER_BUTTON_A] = Button({980, 380, 60, 60}, "B", gray, yellow); // A is B
        buttons[SDL_CONTROLLER_BUTTON_B] = Button({1040, 320, 60, 60}, "A", gray, yellow); // B is A
        buttons[SDL_CONTROLLER_BUTTON_X] = Button({920, 320, 60, 60}, "Y", gray, yellow); // X is Y
        buttons[SDL_CONTROLLER_BUTTON_Y] = Button({980, 260, 60, 60}, "X", gray, yellow); // Y is X
        
        // L1, R1 buttons
        buttons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = Button({200, 180, 80, 40}, "L1", gray, yellow);
        buttons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = Button({1000, 180, 80, 40}, "R1", gray, yellow);
        
        // L2, R2 buttons
        buttons[BUTTON_L2] = Button({200, 130, 80, 40}, "L2", gray, yellow);
        buttons[BUTTON_R2] = Button({1000, 130, 80, 40}, "R2", gray, yellow);
        
        // START, SELECT, MENU
        buttons[SDL_CONTROLLER_BUTTON_START] = Button({900, 520, 80, 40}, "START", gray, yellow);
        buttons[SDL_CONTROLLER_BUTTON_BACK] = Button({820, 520, 80, 40}, "SELECT", gray, yellow);
        buttons[SDL_CONTROLLER_BUTTON_GUIDE] = Button({320, 520, 80, 40}, "MENU", gray, yellow);
        
        // Analog sticks
        Color darkGray(80, 80, 80, 255);
        Color lightGray(150, 150, 150, 255);
        
        sticks[0] = AnalogStick(160, 520, 80, 30, "LEFT STICK", darkGray, lightGray);
        sticks[1] = AnalogStick(1120, 520, 80, 30, "RIGHT STICK", darkGray, lightGray);
    }
    
    // Handle SDL events
    void handleEvent(const SDL_Event& e) {
        switch (e.type) {
            case SDL_QUIT:
                running = false;
                break;
                
            case SDL_CONTROLLERBUTTONDOWN:
                {
                    Uint8 button = e.cbutton.button;
                    if (button < MAX_BUTTON_INDEX) {
                        buttons[button].setPressed(true);
                        
                        // Check for (start + select) combination
                        if (buttons[SDL_CONTROLLER_BUTTON_START].isPressed() && buttons[SDL_CONTROLLER_BUTTON_BACK].isPressed()) {
                            running = false;
                        }
                    }
                }
                break;
                
            case SDL_CONTROLLERBUTTONUP:
                {
                    Uint8 button = e.cbutton.button;
                    if (button < MAX_BUTTON_INDEX) {
                        buttons[button].setPressed(false);
                    }
                }
                break;
                
            case SDL_CONTROLLERAXISMOTION:
                handleStickMovement(e.caxis);
                break;
        }
    }
    
    // Handle analog stick and trigger axis motion
    void handleStickMovement(const SDL_ControllerAxisEvent& axis) {
        float value = axis.value / 32767.0f;  // Normalize to -1.0 to 1.0
        
        // Apply small deadzone
        if (std::abs(value) < 0.1f) {
            value = 0.0f;
        }
        
        switch (axis.axis) {
            case SDL_CONTROLLER_AXIS_LEFTX:
                sticks[0].setXValue(value);
                break;
            case SDL_CONTROLLER_AXIS_LEFTY:
                sticks[0].setYValue(value);
                break;
            case SDL_CONTROLLER_AXIS_RIGHTX:
                sticks[1].setXValue(value);
                break;
            case SDL_CONTROLLER_AXIS_RIGHTY:
                sticks[1].setYValue(value);
                break;

            // Handle L2/R2 triggers
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                buttons[BUTTON_L2].setPressed(value > 0.5f);
                break;
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                buttons[BUTTON_R2].setPressed(value > 0.5f);
                break;
        }
    }
    
    void drawButton(const Button& button) {
        const SDL_Rect& rect = button.getRect();
        const Color& color = button.getCurrentColor();
        
        // background
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
        
        // border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
        
        // label
        RenderTextCentered(renderer, font, button.getName(), rect.x + rect.w / 2, rect.y + rect.h / 2, Color(255, 255, 255, 255));
    }
    
    void drawStick(const AnalogStick& stick) {
        SDL_Point stickPos = stick.getStickPosition();

        // base circle
        DrawFilledCircle(renderer, stick.getCenterX(), stick.getCenterY(), stick.getRadius(), stick.getBaseColor());
        
        // circle border
        DrawCircle(renderer, stick.getCenterX(), stick.getCenterY(), stick.getRadius(), Color(0, 0, 0, 255));
        
        // stick
        DrawFilledCircle(renderer, stickPos.x, stickPos.y, stick.getStickRadius(), stick.getStickColor());
        
        // stick border
        DrawCircle(renderer, stickPos.x, stickPos.y, stick.getStickRadius(), Color(0, 0, 0, 255));
        
        // label
        RenderTextCentered(renderer, font, stick.getValueText(), stick.getCenterX(), stick.getCenterY() + stick.getRadius() + 20, Color(255, 255, 255, 255));
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
        SDL_RenderClear(renderer);
        
        RenderTextCentered(renderer, font, "Trimui Smart Pro Controller Tester", SCREEN_WIDTH / 2, 30, Color(255, 255, 255, 255));
        RenderTextCentered(renderer, font, "Press START + SELECT to exit", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 40, Color(200, 200, 200, 255));
        
        // outline
        SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
        SDL_Rect outline = {50, 50, SCREEN_WIDTH - 100, 600};
        SDL_RenderDrawRect(renderer, &outline);
        
        // Draw all buttons
        for (int i = 0; i < MAX_BUTTON_INDEX; i++) {
            if (!buttons[i].getName().empty()) {
                drawButton(buttons[i]);
            }
        }
        
        // Draw analog sticks
        drawStick(sticks[0]);
        drawStick(sticks[1]);
        
        SDL_RenderPresent(renderer);
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Starting Trimui Controller Tester" << std::endl;
    
    ControllerVisualizer visualizer;
    
    if (!visualizer.initialize()) {
        std::cerr << "Failed to initialize visualizer" << std::endl;
        return 1;
    }
    
    visualizer.run();
    
    return 0;
}