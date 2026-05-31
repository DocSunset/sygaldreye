#include <android/log.h>
#include <android_native_app_glue.h>
#include <openxr/openxr.h>
#include "renderer.hpp"
#include "xr_session.hpp"

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)

XrInstance xr_create_instance(struct android_app*);
XrSystemId xr_get_system(XrInstance);

struct AppState {
    bool hasWindow = false;
    bool hasFocus  = false;
    bool renderable() const { return hasWindow && hasFocus; }
    XrInstance xrInstance = XR_NULL_HANDLE;
    XrSystemId xrSystemId = XR_NULL_SYSTEM_ID;
    Renderer renderer{};
    XrSessionObj xrSession{};
};

static void onAppCmd(struct android_app* app, int32_t cmd) {
    auto* s = static_cast<AppState*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:  s->hasWindow = true;  LOG("INIT_WINDOW");   break;
        case APP_CMD_TERM_WINDOW:  s->hasWindow = false; LOG("TERM_WINDOW");   break;
        case APP_CMD_GAINED_FOCUS: s->hasFocus  = true;  LOG("GAINED_FOCUS");  break;
        case APP_CMD_LOST_FOCUS:   s->hasFocus  = false; LOG("LOST_FOCUS");    break;
        case APP_CMD_DESTROY:                            LOG("DESTROY");       break;
    }
}

void android_main(struct android_app* app) {
    LOG("android_main: started");

    AppState state;
    state.xrInstance = xr_create_instance(app);
    state.xrSystemId = xr_get_system(state.xrInstance);
    state.renderer.init();
    state.xrSession.create(state.xrInstance, state.xrSystemId, state.renderer.graphics_binding());
    app->userData  = &state;
    app->onAppCmd  = onAppCmd;

    while (!app->destroyRequested && !state.xrSession.should_quit()) {
        int         events;
        android_poll_source* source;
        int timeout = state.xrSession.should_render() ? 0 : 10;
        if (ALooper_pollOnce(timeout, nullptr, &events,
                             reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
        }
        state.xrSession.poll_events();
        if (state.xrSession.session_running())
            state.xrSession.render_frame();
    }

    LOG("android_main: exiting");
}
