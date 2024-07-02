#include "linux/linux_app.hpp"
#include <chrono>
#include <cstring>
#include <cassert>
#include "engine.hpp"


static inline xcb_intern_atom_reply_t* intern_atom_helper(xcb_connection_t* conn, bool only_if_exists, const char* str)
{
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_if_exists, strlen(str), str);
    xcb_flush(conn);
    return xcb_intern_atom_reply(conn, cookie, nullptr);
}

LinuxApp::LinuxApp(const CreateParameters& parameters) : Application(parameters)
{
    // xcb_connect always returns a non-NULL pointer to a xcb_connection_t,
    // even on failure. Callers need to use xcb_connection_has_error() to
    // check for failure. When finished, use xcb_disconnect() to close the
    // g_connection and free the structure.
    _connection = xcb_connect(nullptr, nullptr);
    assert(_connection);

    const xcb_setup_t* setup = xcb_get_setup(_connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    _screen = iter.data;

    uint32_t valueMask, valueList[32];
    _window = xcb_generate_id(_connection);

    valueMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    valueList[0] = _screen->black_pixel;
    valueList[1] =
            XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_STRUCTURE_NOTIFY |
            XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE;

    _width = _screen->width_in_pixels;
    _height = _screen->height_in_pixels;

    xcb_create_window(_connection,
                      XCB_COPY_FROM_PARENT,
                      _window,
                      _screen->root,
                      0, 0,
                      _width, _height,
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      _screen->root_visual,
                      valueMask, valueList);

    xcb_intern_atom_reply_t* reply = intern_atom_helper(_connection, true, "WM_PROTOCOLS");
    _atomWmDeleteWindow = intern_atom_helper(_connection, false, "WM_DELETE_WINDOW");

    xcb_change_property(_connection, XCB_PROP_MODE_REPLACE, _window, reply->atom, 4, 32, 1, &_atomWmDeleteWindow->atom);

    xcb_change_property(_connection, XCB_PROP_MODE_REPLACE, _window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, _windowTitle.size(), _windowTitle.data());

    free(reply);

    std::string wmClass;
    wmClass = wmClass.insert(0, _windowTitle);
    wmClass = wmClass.insert(wmClass.size(), 1, '\0');
    xcb_change_property(_connection, XCB_PROP_MODE_REPLACE, _window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8, wmClass.size() + 2, wmClass.c_str());

    if(_isFullscreen)
    {
        xcb_intern_atom_reply_t* atomWmState = intern_atom_helper(_connection, false, "_NET_WM_STATE");
        xcb_intern_atom_reply_t* atomWmFulscreen = intern_atom_helper(_connection, false, "_NET_WM_STATE_FULLSCREEN");
        xcb_change_property(_connection, XCB_PROP_MODE_REPLACE, _window, atomWmState->atom, XCB_ATOM_ATOM, 32, 1, &atomWmFulscreen->atom);
        free(atomWmFulscreen);
        free(atomWmState);
    }

    xcb_map_window(_connection, _window);
    xcb_flush(_connection);

    std::vector<const char*> extensions{};
    extensions.emplace_back("VK_KHR_xcb_surface");

    Engine::InitInfo initInfo{};
    initInfo.extensions = extensions.data();
    initInfo.extensionCount = extensions.size();

    _engine->Init(initInfo);
}

void LinuxApp::Run()
{
    while(!_quit)
    {
        auto tStart = std::chrono::high_resolution_clock::now();

        _frameCounter++;
        auto tEnd = std::chrono::high_resolution_clock::now();
        auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        _frameTimer = tDiff / 1000.0f;

        if(!_paused)
        {
            _timer += _frameTimer;
            if(_timer > 1.0f)
                _timer -= 1.0f;
        }

        _engine->Run();
    }
}

LinuxApp::~LinuxApp()
{
    _engine->Shutdown();

    xcb_destroy_window(_connection, _window);
    xcb_disconnect(_connection);
}
