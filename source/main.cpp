#define TESLA_INIT_IMPL

#include <tesla.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <mutex>

namespace {

constexpr u32 kTouchWidth = 1280;
constexpr u32 kTouchHeight = 720;
constexpr u16 kCursorFramebufferWidth = 960;
constexpr u16 kCursorFramebufferHeight = 540;
constexpr float kCompositorScale = 1.5F;
constexpr float kLayerPanMarginX = 96.0F;
constexpr float kLayerPanMarginY = 72.0F;
constexpr float kDeadzone = 0.16F;
constexpr float kPixelsPerFrame = 13.0F;
constexpr u32 kFingerDiameter = 24;
constexpr u64 kTeslaMenuCombo = KEY_L | KEY_DDOWN | KEY_RSTICK;

std::mutex g_touchPilotMutex;
bool g_hiddbgInitialized = false;
bool g_touchPilotActive = false;
std::atomic<bool> g_touchInjectionAvailable{false};

void releaseTouchPilot() {
    std::scoped_lock lock(g_touchPilotMutex);
    if (!g_hiddbgInitialized) {
        return;
    }

    if (g_touchPilotActive) {
        // Keep the empty state active for one HID polling interval so the
        // target observes finger-up before AutoPilot is removed.
        hiddbgSetTouchScreenAutoPilotState(nullptr, 0);
        svcSleepThread(17'000'000ULL);
    }
    hiddbgUnsetTouchScreenAutoPilotState();
    g_touchPilotActive = false;
}

class CursorElement final : public tsl::elm::Element {
public:
    std::atomic<float> cursorX{kTouchWidth / 2.0F};
    std::atomic<float> cursorY{kTouchHeight / 2.0F};
    std::atomic<bool> touching{false};

    void draw(tsl::gfx::Renderer* renderer) override {
        // libultrahand reuses both framebuffer slots. Clear the current slot
        // before drawing, otherwise old cursor positions remain as trails.
        renderer->clearScreen();
        const s32 centerX = static_cast<s32>(localCursorX());
        const s32 centerY = static_cast<s32>(localCursorY());
        const bool isTouching = touching.load(std::memory_order_acquire);

        if (!g_touchInjectionAvailable.load(std::memory_order_acquire)) {
            const auto red = renderer->a(tsl::Color{0xF, 0x2, 0x2, 0xF});
            renderer->drawCircle(centerX, centerY, 15, false, red);
            renderer->drawRect(centerX - 13, centerY - 2, 26, 4, red);
            return;
        }

        const auto outline = renderer->a(tsl::Color{0x0, 0x0, 0x0, 0xF});
        const auto fill = renderer->a(isTouching
            ? tsl::Color{0xF, 0x5, 0x2, 0xF}
            : tsl::Color{0x2, 0xD, 0xF, 0xF});
        const auto center = renderer->a(tsl::Color{0xF, 0xF, 0xF, 0xF});

        renderer->drawCircle(centerX, centerY, isTouching ? 14 : 12, true, outline);
        renderer->drawCircle(centerX, centerY, isTouching ? 10 : 8, true, fill);
        renderer->drawCircle(centerX, centerY, 2, true, center);
    }

    void layout(u16, u16, u16, u16) override {
        setBoundaries(0, 0, kCursorFramebufferWidth, kCursorFramebufferHeight);
    }

    tsl::elm::Element* requestFocus(tsl::elm::Element*, tsl::FocusDirection) override {
        return this;
    }

    float localCursorX() const {
        return ((cursorX.load(std::memory_order_acquire) * kCompositorScale) -
                static_cast<float>(tsl::cfg::LayerPosX)) / kCompositorScale;
    }

    float localCursorY() const {
        return ((cursorY.load(std::memory_order_acquire) * kCompositorScale) -
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
        const u64 pressed = keysDown | keysHeld;

        // An overlay owns the Tesla process while it is open. Treat the user's
        // normal Tesla combo as a clean exit so nx-ovlloader can immediately
        // chainload the menu again, without changing its saved keybind.
        if ((pressed & kTeslaMenuCombo) == kTeslaMenuCombo &&
            (keysDown & kTeslaMenuCombo) != 0) {
            releaseTouch();
            tsl::setNextOverlay(returnOverlayPath);
            tsl::Overlay::get()->close(true);
            return true;
        }

        if (keysDown & HidNpadButton_B) {
            releaseTouch();
            tsl::setNextOverlay(returnOverlayPath);
            tsl::Overlay::get()->close(true);
            return true;
        }

        const auto stick = chooseStick(leftStick, rightStick);
        moveCursor(stick);

        if (keysDown & HidNpadButton_X) {
            m_cursor->cursorX.store(kTouchWidth / 2.0F, std::memory_order_release);
            m_cursor->cursorY.store(kTouchHeight / 2.0F, std::memory_order_release);
        }

        const bool dragRequested = pressed & HidNpadButton_ZR;
        const bool tapRequested = pressed & HidNpadButton_A;
        const bool shouldTouch = dragRequested || tapRequested;

        if (shouldTouch) {
            sendTouch();
        } else if (m_cursor->touching.load(std::memory_order_acquire)) {
            releaseTouch();
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

        const float oldX = m_cursor->cursorX.load(std::memory_order_acquire);
        const float oldY = m_cursor->cursorY.load(std::memory_order_acquire);
        m_cursor->cursorX.store(
            std::clamp(oldX + (x * speed), 0.0F, static_cast<float>(kTouchWidth - 1)),
            std::memory_order_release);
        m_cursor->cursorY.store(
            std::clamp(oldY - (y * speed), 0.0F, static_cast<float>(kTouchHeight - 1)),
            std::memory_order_release);
    }

    void updateLayerPosition() {
        if (m_cursor == nullptr) {
            return;
        }

        constexpr s32 layerWidth =
            static_cast<s32>(kCursorFramebufferWidth * kCompositorScale);
        constexpr s32 layerHeight =
            static_cast<s32>(kCursorFramebufferHeight * kCompositorScale);
        constexpr s32 halfWidth = layerWidth / 2;
        constexpr s32 halfHeight = layerHeight / 2;
        const float cursorX = m_cursor->cursorX.load(std::memory_order_acquire);
        const float cursorY = m_cursor->cursorY.load(std::memory_order_acquire);
        const s32 targetX = static_cast<s32>(cursorX * kCompositorScale);
        const s32 targetY = static_cast<s32>(cursorY * kCompositorScale);
        const s32 maxX = static_cast<s32>(tsl::cfg::ScreenWidth) - layerWidth;
        const s32 maxY = static_cast<s32>(tsl::cfg::ScreenHeight) - layerHeight;

        u32 layerX = m_lastLayerX;
        u32 layerY = m_lastLayerY;
        if (layerX == std::numeric_limits<u32>::max()) {
            layerX = static_cast<u32>(std::clamp(targetX - halfWidth, 0, maxX));
        } else {
            const float localX = ((cursorX * kCompositorScale) - layerX) /
                                 kCompositorScale;
            if (localX < kLayerPanMarginX ||
                localX > kCursorFramebufferWidth - kLayerPanMarginX) {
                layerX = static_cast<u32>(std::clamp(targetX - halfWidth, 0, maxX));
            }
        }

        if (layerY == std::numeric_limits<u32>::max()) {
            layerY = static_cast<u32>(std::clamp(targetY - halfHeight, 0, maxY));
        } else {
            const float localY = ((cursorY * kCompositorScale) - layerY) /
                                 kCompositorScale;
            if (localY < kLayerPanMarginY ||
                localY > kCursorFramebufferHeight - kLayerPanMarginY) {
                layerY = static_cast<u32>(std::clamp(targetY - halfHeight, 0, maxY));
            }
        }

        if (layerX != m_lastLayerX || layerY != m_lastLayerY) {
            tsl::gfx::Renderer::get().setLayerPos(layerX, layerY);
            m_lastLayerX = layerX;
            m_lastLayerY = layerY;
        }
    }

    void sendTouch() {
        if (!g_touchInjectionAvailable.load(std::memory_order_acquire) ||
            m_cursor == nullptr) {
            return;
        }

        HidTouchState state{};
        state.x = static_cast<u32>(m_cursor->cursorX.load(std::memory_order_acquire));
        state.y = static_cast<u32>(m_cursor->cursorY.load(std::memory_order_acquire));
        state.diameter_x = kFingerDiameter;
        state.diameter_y = kFingerDiameter;
        state.delta_time = 16'666'667ULL;

        std::scoped_lock lock(g_touchPilotMutex);
        if (!g_hiddbgInitialized) {
            return;
        }
        // A zero-initialized state describes only continuation. Some targets
        // require the first sample to carry the finger-down transition.
        state.attributes = g_touchPilotActive ? 0 : HidTouchAttribute_Start;
        if (R_SUCCEEDED(hiddbgSetTouchScreenAutoPilotState(&state, 1))) {
            g_touchPilotActive = true;
            m_cursor->touching.store(true, std::memory_order_release);
        } else {
            g_touchInjectionAvailable.store(false, std::memory_order_release);
        }
    }

    void releaseTouch() {
        if (m_cursor != nullptr) {
            m_cursor->touching.store(false, std::memory_order_release);
        }
        releaseTouchPilot();
    }
};

class TouchCursorOverlay final : public tsl::Overlay {
public:
    bool acceptsTouchInput() const override {
        // Our synthetic finger must reach the underlying app, not be handled
        // as a physical touch on this overlay's invisible GUI.
        return false;
    }

    bool exitOnLaunchCombo() const override {
        return true;
    }

    void onShow() override {
        // libultrahand normally disables HID input for the foreground title
        // while an overlay is visible. That also prevents the title from
        // observing our synthetic touchscreen samples. Restore shared input
        // after libultrahand claims focus, and cancel its reassertion burst.
        // Controller buttons will consequently also reach the title while
        // Touch Cursor is open; no global button mapping is modified.
        tsl::hlp::requestForeground(false);
    }

    void initServices() override {
        ult::DefaultFramebufferWidth = kCursorFramebufferWidth;
        ult::DefaultFramebufferHeight = kCursorFramebufferHeight;
        ult::windowedLayerPixelPerfect = false;

        std::scoped_lock lock(g_touchPilotMutex);
        g_hiddbgInitialized = R_SUCCEEDED(hiddbgInitialize());
        g_touchInjectionAvailable.store(g_hiddbgInitialized,
                                        std::memory_order_release);
    }

    void exitServices() override {
        releaseTouchPilot();
        std::scoped_lock lock(g_touchPilotMutex);
        if (g_hiddbgInitialized) {
            hiddbgExit();
            g_hiddbgInitialized = false;
        }
        g_touchInjectionAvailable.store(false, std::memory_order_release);
    }

    void onHide() override {
        // HOME, power and other normal hide events must not terminate the tool.
        releaseTouchPilot();
    }

    std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<CursorGui>();
    }
};

} // namespace

int main(int argc, char** argv) {
    return tsl::loop<TouchCursorOverlay, tsl::impl::LaunchFlags::None>(argc, argv);
}
