/* Copyright (C) 2020-2024 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of gscore.
 *
 * gscore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * gscore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "renderwindow.h"

#include "common/constants/input.h"
#include "common/util/alloc.h"
#include "common/util/inputmatcher.h"
#include "common/util/log.h"
#include "common/util/math.h"
#include "common/structs/charevent.h"
#include "common/structs/keyevent.h"
#include "common/structs/queryrequest.h"
#include "common/structs/queryresult.h"
#include "common/structs/vector2.h"
#include "common/structs/vector2i.h"
#include "events/events.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <X11/Xatom.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char* const WINDOW_TITLE = "gscore";

static const char* const EVENT_MOUSE_MOTION_INPUT_PIXEL_SPACE = "EVENT_MOUSE_MOTION_INPUT_PIXEL_SPACE";

enum {
    WINDOW_WIDTH = 1280,
    WINDOW_HEIGHT = 720,
};

struct RenderWindow {
    GLFWwindow* glfwWindow;
    Vector2i windowSize;
    float timePrevious;
    bool shouldClose;
    int exitCode;
};


static void RenderWindow_processQueries(RenderWindow* self);
static void RenderWindow_onRequestQuit(RenderWindow* self, void* sender, int* exitCode);
static void RenderWindow_onMouseMotionEventPixelSpace(RenderWindow* self, void* sender, Vector2i* positionPixelSpace);
static void RenderWindow_onViewportSizeUpdated(RenderWindow* self, void* sender, Vector2i* size);
static void RenderWindow_onRequestQuery(RenderWindow* self, void* sender, QueryRequest* queryRequest);
static void RenderWindow_glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void RenderWindow_glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
static void RenderWindow_glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
static void RenderWindow_glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
static void RenderWindow_glfwWindowSizeCallback(GLFWwindow* window, int width, int height);


RenderWindow* RenderWindow_new(void) {
    RenderWindow* self = ecalloc(1, sizeof(*self));

    self->windowSize.x = WINDOW_WIDTH;
    self->windowSize.y = WINDOW_HEIGHT;

    if (!glfwInit()) {
        Log_fatal("Failed to initialize GLFW");
    }

    self->glfwWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!self->glfwWindow) {
        Log_fatal("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(self->glfwWindow);

    Event_subscribe(EVENT_REQUEST_QUIT, self, EVENT_CALLBACK(RenderWindow_onRequestQuit), sizeof(int));
    Event_subscribe(EVENT_VIEWPORT_SIZE_UPDATED, self, EVENT_CALLBACK(RenderWindow_onViewportSizeUpdated), sizeof(Vector2i));
    Event_subscribe(EVENT_REQUEST_QUERY, self, EVENT_CALLBACK(RenderWindow_onRequestQuery), sizeof(QueryRequest));

    Events_defineNewLocalEventType(EVENT_MOUSE_MOTION_INPUT_PIXEL_SPACE, sizeof(Vector2i), self);
    Event_subscribe(EVENT_MOUSE_MOTION_INPUT_PIXEL_SPACE, self, EVENT_CALLBACK(RenderWindow_onMouseMotionEventPixelSpace), sizeof(Vector2i));

    glfwSetKeyCallback(self->glfwWindow, RenderWindow_glfwKeyCallback);
    glfwSetMouseButtonCallback(self->glfwWindow, RenderWindow_glfwMouseButtonCallback);
    glfwSetCursorPosCallback(self->glfwWindow, RenderWindow_glfwCursorPosCallback);
    glfwSetScrollCallback(self->glfwWindow, RenderWindow_glfwScrollCallback);
    glfwSetWindowSizeCallback(self->glfwWindow, RenderWindow_glfwWindowSizeCallback);

    self->timePrevious = glfwGetTime();

    return self;
}


void RenderWindow_free(RenderWindow** pself) {
    RenderWindow* self = *pself;

    Event_unsubscribe(EVENT_REQUEST_QUIT, self, EVENT_CALLBACK(RenderWindow_onRequestQuit), sizeof(int));
    Event_unsubscribe(EVENT_VIEWPORT_SIZE_UPDATED, self, EVENT_CALLBACK(RenderWindow_onViewportSizeUpdated), sizeof(Vector2i));
    Event_unsubscribe(EVENT_REQUEST_QUERY, self, EVENT_CALLBACK(RenderWindow_onRequestQuery), sizeof(QueryRequest));

    Event_unsubscribe(EVENT_MOUSE_MOTION_INPUT_PIXEL_SPACE, self, EVENT_CALLBACK(RenderWindow_onMouseMotionEventPixelSpace), sizeof(Vector2i));

    glfwSetKeyCallback(self->glfwWindow, NULL);
    glfwSetMouseButtonCallback(self->glfwWindow, NULL);
    glfwSetCursorPosCallback(self->glfwWindow, NULL);
    glfwSetScrollCallback(self->glfwWindow, NULL);
    glfwSetWindowSizeCallback(self->glfwWindow, NULL);

    glfwDestroyWindow(self->glfwWindow);
    glfwTerminate();

    sfree((void**)pself);
}


int RenderWindow_run(RenderWindow* self) {
    while (!glfwWindowShouldClose(self->glfwWindow)) {
        float timeNow = glfwGetTime();
        float timeDelta = timeNow - self->timePrevious;
        self->timePrevious = timeNow;
        Event_post(self, EVENT_PROCESS_FRAME_BEGIN, &timeDelta, sizeof(timeDelta));
        Event_post(self, EVENT_PROCESS_FRAME, &timeDelta, sizeof(timeDelta));
        Event_post(self, EVENT_PROCESS_FRAME_END, &timeDelta, sizeof(timeDelta));

        glfwSwapBuffers(self->glfwWindow);
        glfwPollEvents();
        RenderWindow_processQueries(self);

        if (self->shouldClose) {
            glfwSetWindowShouldClose(self->glfwWindow, GLFW_TRUE);
        }
    }

    return self->exitCode;
}


static void RenderWindow_processQueries(RenderWindow* self) {
    Display* x11Display = glfwGetX11Display();
    Window x11Window = glfwGetX11Window(self->glfwWindow);

    int nProperties = 0;
    Atom* atoms = XListProperties(x11Display, x11Window, &nProperties);

    for (int i = 0; i < nProperties; i++) {
        unsigned char* propertyValue = NULL;
        Atom atomDummy;
        int intDummy;
        unsigned long longDummy;
        int result = XGetWindowProperty(
            x11Display,
            x11Window,
            atoms[i],
            0L,
            sizeof(Atom),
            true,
            XA_STRING,
            &atomDummy,
            &intDummy,
            &longDummy,
            &longDummy,
            &propertyValue
        );
        if (result == Success && propertyValue && strlen((char*)propertyValue)) {
            char* propertyKey = XGetAtomName(x11Display, atoms[i]);
            if (propertyKey) {
                Log_info("Received: %s='%s'", propertyKey, propertyValue);
                QueryResult queryResult = {propertyKey, (char*)propertyValue};
                Event_post(self, EVENT_QUERY_RESULT, &queryResult, sizeof(queryResult));
                XFree(propertyKey);
            }
        }
        if (propertyValue) {
            XFree(propertyValue);
        }
    }

    if (atoms) {
        XFree(atoms);
    }
}


static void RenderWindow_onRequestQuit(RenderWindow* self, void* sender, int* exitCode) {
    (void)sender;

    Log_info("Exiting...");
    self->exitCode = *exitCode;
    self->shouldClose = true;
}


static void RenderWindow_onMouseMotionEventPixelSpace(RenderWindow* self, void* sender, Vector2i* positionPixelSpace) {
    (void)sender;
    Vector2 position = {
        Math_clampf((float)positionPixelSpace->x / (float)self->windowSize.x, 0.0f, 0.999f),
        Math_clampf((float)positionPixelSpace->y / (float)self->windowSize.y, 0.0f, 0.999f),
    };
    Event_post(self, EVENT_MOUSE_MOTION_INPUT, &position, sizeof(position));
}


static void RenderWindow_onViewportSizeUpdated(RenderWindow* self, void* sender, Vector2i* size) {
    (void)sender;
    self->windowSize.x = size->x;
    self->windowSize.y = size->y;
}


static void RenderWindow_onRequestQuery(RenderWindow* self, void* sender, QueryRequest* queryRequest) {
    (void)sender;
    Window x11Window = glfwGetX11Window(self->glfwWindow);
    enum {
        commandBufferSize = 1024,
    };
    char commandBuffer[commandBufferSize] = {0};
    int bytesWritten = snprintf(commandBuffer, commandBufferSize,
        "export WINDOWID=\"%lu\";"
        "export PROMPT=\"%s\";"
        "export ATOM_NAME=\"%s\";"
        "dmenu -b -i -p \"$PROMPT\" -w \"$WINDOWID\""
        "| tr '\\n' '\\0' "
        "| xargs -r0 xprop -id \"$WINDOWID\" -f \"$ATOM_NAME\" 8s -set \"$ATOM_NAME\"",
    x11Window, queryRequest->prompt, queryRequest->key);
    Log_assert(bytesWritten >= 0 && bytesWritten < commandBufferSize, "Invalid write size: %d", bytesWritten);

    FILE* pipe = popen(commandBuffer, "w");
    Log_assert(pipe, "Failed to run popen()");
    fprintf(pipe, "%s", queryRequest->items);
    pclose(pipe);
}


static void RenderWindow_glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    const char* const keyName = glfwGetKeyName(key, scancode);
    if (keyName) {
        // Keyboard layout dependent
        char c = keyName[0];
        if (isascii(c) && keyName[1] == 0) {
            CharEvent charEvent = {c, action, mods};
            Event_post(window, EVENT_CHAR_INPUT, &charEvent, sizeof(charEvent));
        }
    } else {
        // Not keyboard layout dependent
        KeyEvent keyEvent = {key, action, mods};
        Event_post(window, EVENT_KEY_INPUT, &keyEvent, sizeof(keyEvent));
    }
}


static void RenderWindow_glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    MouseButtonEvent mouseButtonEvent = {button, action, mods};
    Event_post(window, EVENT_MOUSE_BUTTON_INPUT, &mouseButtonEvent, sizeof(mouseButtonEvent));
}


static void RenderWindow_glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Vector2i position = {xpos, ypos};
    Event_post(window, EVENT_MOUSE_MOTION_INPUT_PIXEL_SPACE, &position, sizeof(position));
}


static void RenderWindow_glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Vector2i offset = {xoffset, yoffset};
    if (offset.x != 0 || offset.y != 0) {
        Event_post(window, EVENT_MOUSE_SCROLLWHEEL_INPUT, &offset, sizeof(offset));
    } else {
        Log_warning("Ignoring zero-value srollwheel input");
    }
}


static void RenderWindow_glfwWindowSizeCallback(GLFWwindow* window, int width, int height) {
    Vector2i size = {width, height};
    Event_post(window, EVENT_VIEWPORT_SIZE_UPDATED, &size, sizeof(size));
}
