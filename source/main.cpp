#define TESLA_INIT_IMPL

#include <tesla.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr u32 kTouchWidth = 1280;
constexpr u32 kTouchHeight = 720;
constexpr u16 kCursorFramebufferSize = 96;
constexpr float kCompositorScale = 1.5F;
constexpr float kDeadzone = 0.16F;
constexpr float kPixelsPerFrame = 13.0F;
constexpr u32 kFingerDiameter = 24;

bool g_hiddbgReady = false;

void releaseTouchPilot() {
    if (!g_hiddbgReady) {
        return;
    }

    // Keep the empty state active for one HID polling interval so the target
    // observes finger-up before AutoPilot is removed.
    hiddbgSetTouchScreenAutoPilotState(nullptr, 0);
    svcSleepThread(17'000'000ULL);
    hiddbgUnsetTouchScreenAutoPilotState();
}

class CursorElement final : public tsl::elm::Element {
public:
    float cursorX = kTouchWidth / 2.0F;
    float cursorY = kTouchHeight / 2.0F;
    bool touching = false;

    void draw(tsl::gfx::Renderer* renderer) override {
        const s32 centerX = static_cast<s32>(localCursorX());
        const s32 centerY = static_cast<s32>(localCursorY());

        if (!g_hiddbgReady) {
            const auto red = renderer->a(tsl::Color{0xF, 0x2, 0x2, 0xF});
            renderer->drawCircle(centerX, centerY, 15, false, red);
            renderer->drawRect(centerX - 13, centerY - 2, 26, 4, red);
            return;
        }

        const auto outline = renderer->a(tsl::Color{0x0, 0x0, 0x0, 0xF});
        const auto fill = renderer->a(touching
            ? tsl::Color{0xF, 0x5, 0x2, 0xF}
            : tsl::Color{0x2, 0xD, 0xF, 0xF});
        const auto center = renderer->a(tsl::Color{0xF, 0xF, 0xF, 0xF});

        renderer->drawCircle(centerX, centerY, touching ? 14 : 12, true, outline);
        renderer->drawCircle(centerX, centerY, touching ? 10 : 8, true, fill);
        renderer->drawCircle(centerX, centerY, 2, true, center);
    }

    void layout(u16, u16, u16, u16) override {
        setBoundaries(0, 0, kCursorFramebufferSize, kCursorFramebufferSize);
    }

    tsl::elm::Element* requestFocus(tsl::elm::Element*, tsl::FocusDirection) override {
        return this;
    }

    float localCursorX() const {
        return ((cursorX * kCompositorScale) -
                static_cast<float>(tsl::cfg::LayerPosX)) / kCompositorScale;
    }

    float localCursorY() const {
        return ((cursorY * kCompositorScale) -
                static_cast<float>(tsl::cfg::LayerPosY)) / kCompositorScale;
    }
};

class CursorGui final : public tsl::Gui {
public:
    ~CursorGui() override {
        releaseTouch();
    }

    tsl::elm::Element* createUI() override {
        m_cursor = new CursorElement();
        updateLayerPosition();
        return m_cursor;
    }

    void update() override {
        updateLayerPosition();
    }

    bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState&,
                     HidAnalogStickState leftStick,
                     HidAnalogStickState rightStick) override {
        const auto stick = chooseStick(leftStick, rightStick);
        moveCursor(stick);

        if (keysDown & HidNpadButton_X) {
            m_cursor->cursorX = kTouchWidth / 2.0F;
            m_cursor->cursorY = kTouchHeight / 2.0F;
        }

        const bool dragRequested = (keysDown | keysHeld) & HidNpadButton_ZR;
        const bool tapRequested = (keysDown | keysHeld) & HidNpadButton_A;
        const bool shouldTouch = dragRequested || tapRequested;

        if (shouldTouch) {
            sendTouch();
        } else if (m_cursor->touching) {
            releaseTouch();
        }

        if (keysDown & HidNpadButton_B) {
            releaseTouch();
            tsl::Overlay::get()->close();
        }

        return true;
    }

private:
    CursorElement* m_cursor = nullptr;
    u32 m_lastLayerX = std::numeric_limits<u32>::max();
    u32 m_lastLayerY = std::numeric_limits<u32>::max();

    static HidAnalogStickState chooseStick(HidAnalogStickState left,
                                            HidAnalogStickState right) {
        const s64 leftMagnitude = static_cast<s64>(left.x) * left.x +
                                  static_cast<s64>(left.y) * left.y;
        const s64 rightMagnitude = static_cast<s64>(right.x) * right.x +
                                   static_cast<s64>(right.y) * right.y;
        return rightMagnitude >= leftMagnitude ? right : left;
    }

    void moveCursor(HidAnalogStickState stick) {
        constexpr float stickMax = 32767.0F;
        float x = std::clamp(static_cast<float>(stick.x) / stickMax, -1.0F, 1.0F);
        float y = std::clamp(static_cast<float>(stick.y) / stickMax, -1.0F, 1.0F);
        const float magnitude = std::sqrt((x * x) + (y * y));

        if (magnitude <= kDeadzone) {
            return;
        }

        const float normalized = (magnitude - kDeadzone) / (1.0F - kDeadzone);
        const float speed = kPixelsPerFrame * std::clamp(normalized, 0.0F, 1.0F);
        x /= magnitude;
        y /= magnitude;

        m_cursor->cursorX = std::clamp(m_cursor->cursorX + (x * speed),
                                       0.0F, static_cast<float>(kTouchWidth - 1));
        m_cursor->cursorY = std::clamp(m_cursor->cursorY - (y * speed),
                                       0.0F, static_cast<float>(kTouchHeight - 1));

    }

    void updateLayerPosition() {
        if (m_cursor == nullptr) {
            return;
        }

        constexpr s32 layerWidth =
            static_cast<s32>(kCursorFramebufferSize * kCompositorScale);
        constexpr s32 layerHeight =
            static_cast<s32>(kCursorFramebufferSize * kCompositorScale);
        constexpr s32 halfWidth = layerWidth / 2;
        constexpr s32 halfHeight = layerHeight / 2;
        const s32 targetX = static_cast<s32>(m_cursor->cursorX * kCompositorScale);
        const s32 targetY = static_cast<s32>(m_cursor->cursorY * kCompositorScale);
        const s32 maxX = static_cast<s32>(tsl::cfg::ScreenWidth) - layerWidth;
        const s32 maxY = static_cast<s32>(tsl::cfg::ScreenHeight) - layerHeight;
        const u32 layerX = static_cast<u32>(std::clamp(targetX - halfWidth, 0, maxX));
        const u32 layerY = static_cast<u32>(std::clamp(targetY - halfHeight, 0, maxY));

        if (layerX != m_lastLayerX || layerY != m_lastLayerY) {
            tsl::gfx::Renderer::get().setLayerPos(layerX, layerY);
            m_lastLayerX = layerX;
            m_lastLayerY = layerY;
        }
    }

    void sendTouch() {
        if (!g_hiddbgReady || m_cursor == nullptr) {
            return;
        }

        HidTouchState state{};
        state.x = static_cast<u32>(m_cursor->cursorX);
        state.y = static_cast<u32>(m_cursor->cursorY);
        state.diameter_x = kFingerDiameter;
        state.diameter_y = kFingerDiameter;
        state.delta_time = 16'666'667ULL;

        if (R_SUCCEEDED(hiddbgSetTouchScreenAutoPilotState(&state, 1))) {
            m_cursor->touching = true;
        }
    }

    void releaseTouch() {
        if (m_cursor != nullptr) {
            m_cursor->touching = false;
        }
        releaseTouchPilot();
    }
};

class TouchCursorOverlay final : public tsl::Overlay {
public:
    void initServices() override {
        ult::DefaultFramebufferWidth = kCursorFramebufferSize;
        ult::DefaultFramebufferHeight = kCursorFramebufferSize;
        ult::windowedLayerPixelPerfect = false;

        g_hiddbgReady = R_SUCCEEDED(hiddbgInitialize());
    }

    void exitServices() override {
        if (g_hiddbgReady) {
            releaseTouchPilot();
            hiddbgExit();
            g_hiddbgReady = false;
        }
    }

    std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<CursorGui>();
    }
};

} // namespace

int main(int argc, char** argv) {
    return tsl::loop<TouchCursorOverlay, tsl::impl::LaunchFlags::None>(argc, argv);
}
