/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lb_graphics_linux.h"

#include "lb_gl_headers.h"

#include <X11/xpm.h>

#include <map>
#include <string>
#include <vector>

#include "external/chromium/base/bind.h"
#include "external/chromium/base/debug/trace_event.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/skia/ext/SkMemory_new_handler.h"
#include "external/chromium/third_party/libpng/png.h"
#include "external/chromium/third_party/WebKit/Source/WTF/wtf/ExportMacros.h"  // needed before InspectorCounters
#include "external/chromium/third_party/WebKit/Source/WebCore/inspector/InspectorCounters.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "external/chromium/third_party/WebKit/Source/WTF/config.h"
#include "external/chromium/third_party/WebKit/Source/WTF/wtf/OSAllocator.h"
#include "external/chromium/ui/gl/gl_implementation.h"
#include "external/chromium/ui/gl/gl_surface.h"
#include "external/chromium/gpu/command_buffer/service/context_group.h"
#include "lb_gl_image_utils.h"
#include "lb_globals.h"
#include "lb_memory_manager.h"
#include "lb_on_screen_display.h"
#include "lb_spinner_overlay.h"
#include "lb_web_view_host.h"
#include "steel_icon_data.xpm"

LBGraphics* LBGraphics::instance_ = NULL;

void LBGraphics::SetupGraphics() {
  LBGraphicsLinux* instance = new LBGraphicsLinux();
  instance_ = instance;
  instance->Initialize();
}

// static
void LBGraphics::TeardownGraphics() {
  DCHECK(instance_) << "The instance should not have been deleted already!";
  if (instance_) {
    delete LBGraphics::instance_;
    instance_ = NULL;
  }
}

LBGraphicsLinux::LBGraphicsLinux()
  : graphics_thread_("Graphics")
  , device_width_(0)
  , device_height_(0)
  , device_color_pitch_(0) {
  x_visual_info_ = 0;
}

void LBGraphicsLinux::Initialize() {
  // Setup the GLES2 system
  LBWebGraphicsContext3DCommandBuffer::InitializeCommandBufferSystem();

  graphics_thread_.StartWithOptions(base::Thread::Options());
  graphics_message_loop_ = graphics_thread_.message_loop();
  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBGraphicsLinux::GraphicsThreadInitialize,
                 base::Unretained(this)));

  // Sync with the message loop to make sure initialization is finished
  // before continuing.
  base::WaitableEvent wait_event(true, false);
  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(&wait_event)));
  wait_event.Wait();

  // Setup the graphics context used for rendering all UI elements
  // as well as the WebKit rendered texture
  lb_screen_context_ = make_scoped_ptr(
      new LBWebGraphicsContext3DCommandBuffer(
              LBWebGraphicsContext3DCommandBuffer::InitOptions(
                  graphics_message_loop_,
                  GetDeviceWidth(), GetDeviceHeight(),
                  x_window_)));

  // Setup the graphics context that WebKit will use to render
  // the current web page in to.  Note that lb_screen_context_
  // is it's parent so that lb_screen_context_ can access
  // and render the output of WebKit.
  LBWebGraphicsContext3DCommandBuffer::InitOptions cc_context_options(
      graphics_message_loop_,
      GetDeviceWidth(), GetDeviceHeight(),
      static_cast<gfx::AcceleratedWidget>(NULL));
  // Setup the overlay context to be able to access the output texture
  // of the chrome compositor.
  cc_context_options.parent = lb_screen_context_.get();
  compositor_context_ = make_scoped_ptr(
      new LBWebGraphicsContext3DCommandBuffer(cc_context_options));

  // Set the inital context output surface to black
  compositor_context_->makeContextCurrent();
  compositor_context_->clearColor(0.0f, 0.0f, 0.0f, 1.0f);
  compositor_context_->clear(GraphicsContext3D::COLOR_BUFFER_BIT);

  // Setup the classes responsible for all UI design/rendering functionality
  spinner_overlay_.reset(new LB::SpinnerOverlay(this,
                                                lb_screen_context_.get()));
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LB::OnScreenDisplay::Create(this, lb_screen_context_.get());
#endif
  quad_drawer_.reset(new LB::QuadDrawer(this, lb_screen_context_.get()));
}

LBGraphicsLinux::~LBGraphicsLinux() {
  quad_drawer_.reset(NULL);
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LB::OnScreenDisplay::Terminate();
#endif
  spinner_overlay_.reset(NULL);
  compositor_context_.reset(NULL);
  lb_screen_context_.reset(NULL);

  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBGraphicsLinux::GraphicsThreadShutdown,
                 base::Unretained(this)));

  // Wait for graphics thread shutdown to complete
  graphics_thread_.Stop();

  LBWebGraphicsContext3DCommandBuffer::ShutdownCommandBufferSystem();
}

namespace {
int LB_XErrorHandler(Display *display, XErrorEvent *errorEvent) {
  char msg[256];
  XGetErrorText(display, errorEvent->error_code, msg, sizeof(msg));
  DLOG(ERROR) << "XWindows Error code " << msg;
  return 0;
}
}

void LBGraphicsLinux::GraphicsThreadInitialize() {
  DCHECK_EQ(MessageLoop::current(), graphics_message_loop_);

  const int kDeviceWidth = 1280;
  const int kDeviceHeight = 720;

  device_width_ = kDeviceWidth;
  device_height_ = kDeviceHeight;
  device_color_pitch_ = device_width_ * 4;

  XInitThreads();
  x_display_ = XOpenDisplay(NULL);
  if (!x_display_) {
    DLOG(FATAL) << "Failed to open X display.";
  }

  {
    int major = 0, minor = 0;
    glXQueryVersion(x_display_, &major, &minor);
    DLOG(INFO) << "GLX version: " << major << "." << minor;
    if (major < 1 || (major == 1 && minor < 3)) {
      DLOG(FATAL) << "GLX version 1.3 or higher required.";
    }
  }

  static int visual_attribs[] = {
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_DOUBLEBUFFER, True,
      GLX_RED_SIZE, 8,
      GLX_GREEN_SIZE, 8,
      GLX_BLUE_SIZE, 8,
      GLX_ALPHA_SIZE, 8,
      0
  };

  // This requires GLX 1.3 or higher.
  DLOG(INFO) << "Getting framebuffer config";
  int fbcount;
  GLXFBConfig *fbcArray = glXChooseFBConfig(x_display_,
      DefaultScreen(x_display_),
      visual_attribs,
      &fbcount);

  if (!fbcArray) {
    DLOG(FATAL) << "Failed to retrieve a framebuffer config";
  }
  fb_config_ = fbcArray[0];
  XFree(fbcArray);
  fbcArray = 0;

  DLOG(INFO) << "Getting XVisualInfo";
  x_visual_info_ = glXGetVisualFromFBConfig(x_display_, fb_config_);

  XSetWindowAttributes swa;
  DLOG(INFO) << "Creating colormap";
  swa.colormap = XCreateColormap(x_display_,
                                 RootWindow(x_display_, x_visual_info_->screen),
                                 x_visual_info_->visual,
                                 AllocNone);
  swa.border_pixel = 0;
  swa.event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask;

  DLOG(INFO) << "Creating window";
  x_window_ = XCreateWindow(x_display_,
                            RootWindow(x_display_, x_visual_info_->screen),
                            0,
                            0,
                            device_width_,
                            device_height_,
                            0,
                            x_visual_info_->depth,
                            InputOutput,
                            x_visual_info_->visual,
                            CWBorderPixel|CWColormap|CWEventMask, &swa);
  if (!x_window_) {
    DLOG(FATAL) << "Failed to create window.";
  }

  XStoreName(x_display_, x_window_, "Steel on Linux");

  // Register for the window manager's close event, WM_DELETE_WINDOW.
  Atom wm_delete = XInternAtom(x_display_, "WM_DELETE_WINDOW", True);
  XSetWMProtocols(x_display_, x_window_, &wm_delete, 1);

  // Set the application icon.
  Pixmap icon_pixmap = 0;
  Pixmap icon_mask = 0;
  XpmAttributes attributes;
  memset(&attributes, 0, sizeof(attributes));
  int xpm_ret = XpmCreatePixmapFromData(
      x_display_,
      x_window_,
      const_cast<char **>(steel_icon_data),
      &icon_pixmap,
      &icon_mask,
      &attributes);
  if (xpm_ret == 0) {
    XWMHints hints;
    memset(&hints, 0, sizeof(hints));
    hints.icon_pixmap = icon_pixmap;
    hints.icon_mask = icon_mask;
    hints.flags = IconPixmapHint | IconMaskHint;
    XSetWMHints(x_display_, x_window_, &hints);
  }

  DLOG(INFO) << "Mapping window";
  XMapWindow(x_display_, x_window_);

  XSetErrorHandler(&LB_XErrorHandler);

  XSync(x_display_, False);

  gfx::InitializeGLBindings(gfx::kGLImplementationDesktopGL);

  // Lock to vsync, if this function exists in the driver.
  PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXTProc =
  (PFNGLXSWAPINTERVALEXTPROC)
    glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
  if (glXSwapIntervalEXTProc) {
    glXSwapIntervalEXTProc(x_display_, x_window_, 1);
  }

  gfx::GLSurface::InitializeDisplayAndConfig(x_display_, fb_config_);
}

void LBGraphicsLinux::GraphicsThreadShutdown() {
  DCHECK_EQ(MessageLoop::current(), graphics_message_loop_);

  if (x_visual_info_ != 0) {
    XFree(x_visual_info_);
  }
}

void LBGraphicsLinux::SetWebViewHost(LBWebViewHost* host) {
  web_view_host_ = host;
}

LBWebViewHost* LBGraphicsLinux::GetWebViewHost() const {
  return web_view_host_;
}

int LBGraphicsLinux::GetDeviceWidth() const {
  return device_width_;
}
int LBGraphicsLinux::GetDeviceHeight() const {
  return device_height_;
}

void LBGraphicsLinux::ShowSpinner() {
  spinner_overlay_->EnableSpinner(true);
}

void LBGraphicsLinux::HideSpinner() {
  spinner_overlay_->EnableSpinner(false);
}

void LBGraphicsLinux::EnableDimming() {
  // TODO: implement dimming
}

void LBGraphicsLinux::DisableDimming() {
  // TODO: implement dimming
}

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
namespace {
void png_write_row_callback(void*,
                            uint32_t,
                            int) {
  // no-op
}

void saveTextureToDisk(void* pixels,
                       const char* file_path,
                       int width,
                       int height,
                       int pitch) {
  // using fopen instead of Chromium's file stuff because libpng wants
  // a file pointer.
  FILE* fp = fopen(file_path, "wb");

  if (!fp) {
    DLOG(ERROR) << "Unable to open file: " << file_path;
    return;
  }

  // initialize png library and headers for writing
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);
  DCHECK(png);
  png_infop info = png_create_info_struct(png);
  DCHECK(info);
  // if error encountered png will call longjmp(), so we set up a setjmp() here
  // with a failed assert to indicate an error in one of the png functions.
  // yo libpng, 1980 called, they want their longjmp() back....
  if (setjmp(png->jmpbuf)) {
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    NOTREACHED() << "libpng encountered an error during processing.";
  }
  // turn on write callback
  png_set_write_status_fn(png,
      (void(*)(png_struct*, png_uint_32, int))png_write_row_callback);
  // set up png library for writing to file
  png_init_io(png, fp);
  // stuff and then write png header
  png_set_IHDR(png,
               info,
               width,
               height,
               8,  // 8 bits per color channel
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);

  // configure png library to strip alpha byte, which comes last in RGBA
  const int bpp = pitch / width;
  if (bpp == 4) {
    png_set_filler(png, 0, PNG_FILLER_AFTER);
  }

  // write image bytes, row by row.
  // In OpenGL the first byte is the start of the bottom row.
  // So we write our image upside down for PNG.
  png_bytep row = (png_bytep)pixels + (height - 1) * pitch;
  for (int i = 0; i < height; ++i) {
    png_write_row(png, row);
    row -= pitch;
  }
  // end write
  png_write_end(png, NULL);
  // clean up structures
  png_destroy_write_struct(&png, &info);
  // close file
  fclose(fp);
}
}  // namespace

void LBGraphicsLinux::TakeScreenshot(const std::string& filename) {
  UpdateAndDrawFrame();

  std::string file_path;
  if (filename == "") {
    // compose filename from epoch time and const prefix
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    file_path = base::StringPrintf("%s/screenshot_%ld.png",
        GetGlobalsPtr()->screenshot_output_path, ts.tv_sec);
  } else {
    // If an absolute path is given for screenshot output,
    // don't prepend the screenshot output path
    if (filename[0] == '/') {
      file_path = filename + ".png";
    } else {
      file_path = base::StringPrintf("%s/%s.png",
          GetGlobalsPtr()->screenshot_output_path, filename.c_str());
    }
  }

  DLOG(INFO) << "Writing screenshot to '" << file_path << "'";

  const int frame_buffer_pitch = device_width_ * 3;
  void* pixels = malloc(device_height_ * frame_buffer_pitch);
  lb_screen_context_->readPixels(0,
                                 0,
                                 device_width_,
                                 device_height_,
                                 GL_RGB,
                                 GL_UNSIGNED_BYTE,
                                 pixels);
  saveTextureToDisk(pixels,
                    file_path.c_str(),
                    device_width_,
                    device_height_,
                    frame_buffer_pitch);

  free(pixels);
  DLOG(INFO) << "Wrote screenshot to '" << file_path << "'";
}
#endif

void LBGraphicsLinux::UpdateAndDrawFrame() {
  TRACE_EVENT0("lb_graphics", "LBGraphicsLinux::UpdateAndDrawFrame");

  lb_screen_context_->makeContextCurrent();

  // Draw the webkit/chrome compositor full-screen texture opaquely
  lb_screen_context_->disable(GraphicsContext3D::BLEND);
  quad_drawer_->DrawQuad(
      compositor_context_->getPlatformTextureId(),
      0, 0,
      1, -1,
      0);

  spinner_overlay_->Render();
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LB::OnScreenDisplay::GetPtr()->Render();
#endif

  lb_screen_context_->prepareTexture();
}

void LBGraphicsLinux::BlockUntilFlip() {
  TRACE_EVENT0("lb_graphics", "LBGraphicsLinux::UpdateAndDrawFrame");

  base::WaitableEvent wait_event(true, false);
  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(&wait_event)));
  wait_event.Wait();
}

LBWebGraphicsContext3D* LBGraphicsLinux::GetCompositorContext() {
  return compositor_context_.get();
}
