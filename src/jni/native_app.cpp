#include <memory>
#include "log.h"
#include "app_context.h"
#include "scene.h"
#include "render.h"
#include "native_app.h"

using namespace std;

namespace dzy {

NativeApp::NativeApp() {
    ALOGD("NativeApp::NativeApp()");
}

NativeApp::~NativeApp() {
    ALOGD("NativeApp::~NativeApp()");
}

bool NativeApp::init(struct android_app* app) {
    mApp = app;
    mApp->userData = this;
    mApp->onAppCmd = NativeApp::handleAppCmd;
    mApp->onInputEvent = NativeApp::handleInputEvent;
    mAppContext.reset(new AppContext(this));

    bool ret = initApp();
    if (!ret) ALOGE("Init NativeApp class failed\n");
    return ret;
}

void NativeApp::fini() {
    releaseApp();
}

int32_t NativeApp::handleInputEvent(struct android_app* app, AInputEvent* event) {
    NativeApp *nativeApp = (NativeApp *)(app->userData);
    return nativeApp->inputEvent(event);
}

void NativeApp::handleAppCmd(struct android_app* app, int32_t cmd) {
    NativeApp *nativeApp = (NativeApp *)(app->userData);
    nativeApp->appCmd(cmd);
}

int32_t NativeApp::inputMotionEvent(int action) {
    int32_t ret = 1;
    switch(action) {
        case AMOTION_EVENT_ACTION_DOWN:
            ALOGD("AMOTION_EVENT_ACTION_DOWN");
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            ALOGD("AMOTION_EVENT_ACTION_MOVE");
            break;
        case AMOTION_EVENT_ACTION_UP:
            ALOGD("AMOTION_EVENT_ACTION_UP");
            break;
    }
    return ret;
}

int32_t NativeApp::inputEvent(AInputEvent* event) {
    int32_t ret = 0;
    switch(AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_KEY:
            ret = inputKeyEvent(AKeyEvent_getAction(event), AKeyEvent_getKeyCode(event));
            break;
        case AINPUT_EVENT_TYPE_MOTION:
            ret = inputMotionEvent(AMotionEvent_getAction(event));
            break;
        default:
            ALOGW("Unknown input event");
            break;
    }
    return ret;
}

void NativeApp::appCmd(int32_t cmd) {
    switch (cmd) {
        case APP_CMD_START:
            ALOGD("APP_CMD_START");
            break;
        case APP_CMD_RESUME:
            ALOGD("APP_CMD_RESUME");
            break;
        case APP_CMD_GAINED_FOCUS:
            ALOGD("APP_CMD_GAINED_FOCUS");
            getAppContext()->setRenderState(true);
            break;

        case APP_CMD_PAUSE:
            ALOGD("APP_CMD_PAUSE");
            break;
        case APP_CMD_LOST_FOCUS:
            ALOGD("APP_CMD_LOST_FOCUS");
            getAppContext()->setRenderState(false);
            break;
        case APP_CMD_SAVE_STATE:
            ALOGD("APP_CMD_SAVE_STATE");
            break;
        case APP_CMD_STOP:
            ALOGD("APP_CMD_STOP");
            break;

        case APP_CMD_INIT_WINDOW:
            ALOGD("APP_CMD_INIT_WINDOW");
            getAppContext()->initDisplay();
            getAppContext()->updateDisplay(getCurrentScene());
            break;
        case APP_CMD_TERM_WINDOW:
            ALOGD("APP_CMD_TERM_WINDOW");
            getAppContext()->releaseDisplay();
            break;

        case APP_CMD_WINDOW_RESIZED:
            // Not implemented in NativeActivity framework ?
            ALOGD("APP_CMD_WINDOW_RESIZED");
            break;

        case APP_CMD_DESTROY:
            ALOGD("APP_CMD_DESTROY");
            break;
        default:
            break;
    }
}

int32_t NativeApp::inputKeyEvent(int action, int code) {
    int32_t ret = 0;
    if (action == AKEY_EVENT_ACTION_DOWN) {
        switch(code) {
            case AKEYCODE_BACK:
                ALOGD("AKEYCODE_BACK");
                getAppContext()->requestQuit();
                break;
            default:
                ALOGD("Not supported key code: %d", code);
                break;
        }
    }
    return ret;
}

shared_ptr<AppContext> NativeApp::getAppContext() {
    return mAppContext;
}

shared_ptr<Scene> NativeApp::getCurrentScene() {
    return SceneManager::get()->getCurrentScene();
}

shared_ptr<Render> NativeApp::getCurrentRender() {
    return RenderManager::get()->getCurrentRender();
}

void NativeApp::mainLoop() {
    int ident;
    int events;
    struct android_poll_source* source;
    shared_ptr<AppContext> appContext(getAppContext());
    app_dummy();

    while (true) {
        // If not animating, block forever waiting for events.
        // otherwise,  loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident = ALooper_pollAll(
                    appContext->isRendering() ? 0 : -1,
                    NULL, &events, (void**)&source)) >= 0) {
            if (ident == LOOPER_ID_MAIN || ident == LOOPER_ID_INPUT) {
                if (source != NULL) {
                    source->process(mApp, source);
                }
            }

            if (mApp->destroyRequested != 0) {
                ALOGD("destroy request received");
                return;
            }
        }

        // check needQuit() to give an early chance to stop drawing.
        if (appContext->isRendering() && !appContext->needQuit()) {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            getAppContext()->updateDisplay(getCurrentScene());
        }
    }

}

} // namespace

