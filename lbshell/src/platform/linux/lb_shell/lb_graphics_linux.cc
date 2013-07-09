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

#include <map>
#include <string>
#include <vector>

#include <X11/xpm.h>

#include "external/chromium/base/bind.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/skia/ext/SkMemory_new_handler.h"
#include "external/chromium/third_party/libpng/png.h"
#include "external/chromium/third_party/WebKit/Source/WTF/wtf/ExportMacros.h" // needed before InspectorCounters
#include "external/chromium/third_party/WebKit/Source/WebCore/inspector/InspectorCounters.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "external/chromium/third_party/WebKit/Source/WTF/config.h"
#include "external/chromium/third_party/WebKit/Source/WTF/wtf/OSAllocator.h"

#include "lb_memory_manager.h"
#include "lb_opengl_helpers.h"
#include "lb_web_graphics_context_3d_linux.h"
#include "lb_web_view_host.h"
#include "steel_icon_data.xpm"

typedef GLXContext (*glXCreateContextAttribsARBProc)(
    Display*, GLXFBConfig, GLXContext, Bool, const int*);

static const float kSpinnerOffsetX = 0.75f;
static const float kSpinnerOffsetY = -0.65f;
// desired width of spinner image in a 1920 pixel wide screen
static const float kSpinnerScalePixels = 107.0f;
static const double kSpinnerAngularVelocity = 3.141;  // in radians/sec

#if defined(_DEBUG)
static const float kSplashScreenMaxAlpha = 0.666f;
#else
static const float kSplashScreenMaxAlpha = 1.0f;
#endif

LBGraphics* LBGraphics::instance_ = NULL;

extern std::string *global_game_content_path;
extern const char *global_screenshot_output_path;

// Made available for use by continuous memory logging in lb_memory_debug.c
extern "C" {
  uint64_t global_webkit_frame_counter = 0;
  int global_lifetime = 0;  // minutes
}

// A timestamp when the app was started.
static time_t s_start_time = 0;

void LBGraphics::SetupGraphics() {
  instance_ = LB_NEW LBGraphicsLinux();
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
  : device_width_(0)
  , device_height_(0)
  , device_color_pitch_(0)
  , webkit_message_loop_(NULL)
  , spinner_state_(VISIBLE)
  , spinner_texture_(-1)
  , spinner_rotation_(0.0f)
#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  , screenshot_taken_cond_(&screenshot_lock_)
#endif
{

  // get current time
  s_start_time = time(NULL);

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  osd_visible_ = true;
  last_webkit_frame_time_ = -1;
  longest_webkit_frame_time_ = 0;
  webkit_fps_ = 0.0;
  webkit_frame_counter_ = 0;
  frame_counter_ = 0;
  worst_webkit_fps_ = 0.0;
  num_samples_for_fps_ = 10;
#endif

  webkit_main_texture_ = 0;

  for (int i = 0; i < 2; ++i) {
    spinner_offset_[i] = 0.0f;
    spinner_scale_[i] = 0.0f;
  }
  spinner_frame_time_ = 0.0;

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  screenshot_requested_ = false;
#endif

  flip_thread_assigned_ = false;

  x_visual_info_ = 0;

  InitializeGraphics();

  compositor_context_ = make_scoped_ptr(new LBWebGraphicsContext3DLinux(
      this, GetDeviceWidth(), GetDeviceHeight()));
}

LBGraphicsLinux::~LBGraphicsLinux() {
  glXMakeCurrent(x_display_, x_window_, gl_context_);

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  gl_text_printer_.reset(0);
#endif

  glDeleteProgram(program_webquad_);
  glDeleteProgram(program_spinner_);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteTextures(1, &webkit_main_texture_);

  glXMakeCurrent(x_display_, 0, 0);
  glXDestroyContext(x_display_, gl_context_);

  if (x_visual_info_ != 0) {
    XFree(x_visual_info_);
  }
}

static int LB_XErrorHandler(Display *display, XErrorEvent *errorEvent) {
  char msg[256];
  XGetErrorText(display, errorEvent->error_code, msg, sizeof(msg));
  DLOG(ERROR) << "XWindows Error code " << msg;
  return 0;
}

void LBGraphicsLinux::initializeDevice() {
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
      GLX_DOUBLEBUFFER, true,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      None
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
  GLXFBConfig fbc = fbcArray[0];
  XFree(fbcArray);
  fbcArray = 0;

  DLOG(INFO) << "Getting XVisualInfo";
  x_visual_info_ = glXGetVisualFromFBConfig(x_display_, fbc);

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

  // Set the application icon.
  Pixmap icon_pixmap = 0;
  Pixmap icon_mask = 0;
  XpmAttributes attributes;
  memset(&attributes, 0, sizeof(attributes));
  int xpm_ret = XpmCreatePixmapFromData(x_display_,
                                        x_window_,
                                        (char **)steel_icon_data,
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

  DLOG(INFO) << "Creating context";
  gl_context_ = glXCreateContext(x_display_,
                                 x_visual_info_,
                                 NULL,
                                 true);


  if (!gl_context_) {
    DLOG(FATAL) << "Failed to create GL context.";
  }


  DLOG(INFO) << "Making context current";
  glXMakeCurrent(x_display_, x_window_, gl_context_);

  {
    GLint major = 0, minor = 0;
    // These require OpenGL 3 or better:
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    if (major == 0 && minor == 0) {
      const char *version =
          reinterpret_cast<const char *>(glGetString(GL_VERSION));
      DLOG(FATAL) << "OpenGL 3 APIs not available: " << version;
    } else {
      DLOG(INFO) << "OpenGL version: " << major << "." << minor;
      DCHECK_GE(major, 2);
    }
  }

  {
    const char* shaderVersion =
        reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    DLOG(INFO) << "Shader language version: " << shaderVersion;
  }

  // Lock to vsync, if this function exists in the driver.
  PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXTProc =
  (PFNGLXSWAPINTERVALEXTPROC)
    glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
  if (glXSwapIntervalEXTProc) {
    glXSwapIntervalEXTProc(x_display_, x_window_, 1);
  }
}

void LBGraphicsLinux::initializeRenderTextures() {
  glGenTextures(1, &webkit_main_texture_);

  glBindTexture(GL_TEXTURE_2D, webkit_main_texture_);
  glTexImage2D(GL_TEXTURE_2D,
    0,
    GL_RGBA,
    device_width_,
    device_height_,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    0);
  GLVERIFY();
}

struct Vert {
  float pos[2];
  float texCoord[2];
};

void LBGraphicsLinux::initializeRenderGeometry() {
  // In our main render loop we use only glViewport to set up a simple
  // normalized viewport geometry on which we draw at most two textured
  // full-screen quads.  The first quad is textured with the current WebKit
  // draw texture and the second quad is textured with the splash screen if
  // it's currently VISIBLE, FADING_OUT, or FADING_IN.  As the splash screen
  // is at 1080p and WebKit is not necessarily, and we use unormalized (pixel)
  // texture coordinates we cannot reuse the texture coordinates from WebKit
  // for the splash screen. However both quads can use the same vertex array,
  // configured thusly:
  //
  //    [-1,1]           [1,1]
  //        0 +-----------+ 2
  //          |           |
  //          |           |
  //          |     +     |
  //          |           |
  //          |           |
  //        1 +-----------+ 3
  //     [-1,-1]           [1,-1]
  //
  // which we initalize here:
  float full_screen_verts[4][2] = {
    -1.0f, 1.0f,
    -1.0f, -1.0f,
    1.0f, 1.0f,
    1.0f, -1.0f};

  // initialize texture coordinates for full texture render, coordinate
  // setup is as follows:
  //
  //           [0,1]            [1,1]
  //              0 +-----------+ 2
  //                |           |
  //                |           |
  //                |           |
  //                |           |
  //                |           |
  //              1 +-----------+ 3
  //             [0,0]         [1,0]
  //
  float full_texcoords[4][2] = {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
  };

  Vert verts[4];
  for (int i = 0; i < 4; ++i) {
    verts[i].pos[0] = full_screen_verts[i][0];
    verts[i].pos[1] = full_screen_verts[i][1];
    verts[i].texCoord[0] = full_texcoords[i][0];
    verts[i].texCoord[1] = full_texcoords[i][1];
  }

  glGenBuffers(1, &full_screen_vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, full_screen_vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(verts),
               verts,
               GL_STATIC_DRAW);
}

void LBGraphicsLinux::initializeShaders() {
  program_webquad_ = glCreateProgram();
  program_spinner_ = glCreateProgram();

  std::string shader_dir_path = *global_game_content_path + "/shaders/";

  std::vector<std::string> shadersToBuild;
  shadersToBuild.push_back("vertex_pass.glsl");
  shadersToBuild.push_back("vertex_spinner.glsl");
  shadersToBuild.push_back("fragment_solid.glsl");
  shadersToBuild.push_back("fragment_reftex.glsl");

  // This variable maps shader filenames to their compiled GL shader id
  std::map<std::string, int> shaderNameToCompiled;
  for (std::vector<std::string>::const_iterator iter = shadersToBuild.begin();
       iter != shadersToBuild.end(); ++iter) {
    std::string path = shader_dir_path + *iter;

    std::vector<char> shader_source = GLHelpers::ReadEntireFile(path);

    bool is_vertex = iter->find("vertex") != std::string::npos;

    GLuint gl_shader_id = 0;
    if (is_vertex) {
      gl_shader_id = glCreateShader(GL_VERTEX_SHADER);
    } else {
      gl_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    }
    GLVERIFY();
    GLchar* source_glchar = &shader_source[0];
    glShaderSource(gl_shader_id, 1,
                    const_cast<const GLchar**>(&source_glchar), NULL);
    glCompileShader(gl_shader_id);
    GLVERIFY();

    GLHelpers::CheckShaderCompileStatus(gl_shader_id, *iter);

    shaderNameToCompiled[*iter] = gl_shader_id;
  }

  glAttachShader(program_webquad_,
                 shaderNameToCompiled["vertex_pass.glsl"]);
  GLVERIFY();
  glAttachShader(program_webquad_,
                 shaderNameToCompiled["fragment_solid.glsl"]);
  GLVERIFY();
  glAttachShader(program_spinner_,
                 shaderNameToCompiled["vertex_spinner.glsl"]);
  GLVERIFY();
  glAttachShader(program_spinner_,
                 shaderNameToCompiled["fragment_reftex.glsl"]);
  GLVERIFY();


  glBindAttribLocation(program_webquad_, kPositionAttrib, "a_position");
  glBindAttribLocation(program_spinner_, kPositionAttrib, "a_position");

  glBindAttribLocation(program_webquad_, kTexcoordAttrib, "a_texCoord");
  glBindAttribLocation(program_spinner_, kTexcoordAttrib, "a_texCoord");
  GLVERIFY();

  glLinkProgram(program_webquad_);
  glLinkProgram(program_spinner_);

  vert_spinner_scale_param_ = glGetUniformLocation(
      program_spinner_, "scale");
  vert_spinner_offset_param_ = glGetUniformLocation(
      program_spinner_, "offset");
  vert_spinner_rotation_param_ = glGetUniformLocation(
      program_spinner_, "rotation");
  GLVERIFY();

  // Get the uniform for our sampler and set it to texture unit 0.
  GLint ref_tex_location = glGetUniformLocation(program_webquad_,
      "reftex");
  GLVERIFY();
  glUseProgram(program_webquad_);
  glUniform1i(ref_tex_location, 0);

  ref_tex_location = glGetUniformLocation(program_spinner_,
      "reftex");
  glUseProgram(program_spinner_);
  glUniform1i(ref_tex_location, 0);

  glUseProgram(0);
  GLVERIFY();
}

void LBGraphicsLinux::initializeSpinner() {
  // load spinner texture
  std::string file_path = *global_game_content_path + "/spinner.png";
  FILE* fp = fopen(file_path.c_str(), "rb");
  DCHECK(fp);
  uint8_t header[8];
  fread(header, 1, 8, fp);
  // make sure we have a valid png header
  DCHECK(!png_sig_cmp(header, 0, 8));
  // setup structures for read
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);
  DCHECK(png);
  png_infop info = png_create_info_struct(png);
  DCHECK(info);
  png_infop end = png_create_info_struct(png);
  // setup error callback to this if block
  if (setjmp(png->jmpbuf)) {
    png_destroy_read_struct(&png, &info, &end);
    fclose(fp);
    NOTREACHED() << "libpng returned error reading spinner.png";
  }
  // set up for file i/o
  png_init_io(png, fp);
  // tell png we already read 8 bytes
  png_set_sig_bytes(png, 8);
  // read the image info
  png_read_info(png, info);
  uint32_t width = png_get_image_width(png, info);
  uint32_t height = png_get_image_height(png, info);

  // allocate texture memory and set up row pointers for read
  int pitch = width * 4;

  uint8_t* spinner_pixels = LB_NEW uint8_t[pitch * height];
  std::vector<png_bytep> rows(height);
  uint8_t* row = spinner_pixels;
  for (int i = height-1; i >= 0; --i) {
    rows[i] = row;
    row += pitch;
  }
  // read png image data
  png_read_image(png, &rows[0]);
  // clean up
  png_read_end(png, end);
  png_destroy_read_struct(&png, &info, &end);
  fclose(fp);

  glGenTextures(1, &spinner_texture_);
  glBindTexture(GL_TEXTURE_2D, spinner_texture_);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               width,
               height,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               spinner_pixels);
  delete[] spinner_pixels;

  glBindTexture(GL_TEXTURE_2D, 0);
  // initialize spinner geometry
  spinner_rotation_ = 0.0f;
  spinner_offset_[0] = kSpinnerOffsetX;
  spinner_offset_[1] = kSpinnerOffsetY;
  // adjust scale for aspect ratio to keep things circular
  spinner_scale_[0] = kSpinnerScalePixels / static_cast<float>(device_width_);
  spinner_scale_[1] = kSpinnerScalePixels / static_cast<float>(device_height_);
  // initialize timing
  spinner_frame_time_ = static_cast<double>(LB::Platform::TickCount() /
    (1000.0 * 1000.0));
}

void LBGraphicsLinux::clearWebKitBuffers() {
  // Clear the webkit_main_texture_ to black.
  // We're going to draw with it even before WebKit has done any compositing.
  GLuint frame_buffer_for_clear;
  glGenFramebuffers(1, &frame_buffer_for_clear);
  glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_for_clear);
  glFramebufferTexture(GL_FRAMEBUFFER,
                       GL_COLOR_ATTACHMENT0,
                       webkit_main_texture_,
                       0);
  GLVERIFY();
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glFlush();
  glDeleteFramebuffers(1, &frame_buffer_for_clear);
}

void LBGraphicsLinux::InitializeGraphics() {
  initializeDevice();
  initializeRenderTextures();
  initializeRenderGeometry();
  initializeShaders();
  initializeSpinner();

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  gl_text_printer_.reset(
    new LBGLTextPrinter(
          *global_game_content_path + "/shaders/",
          device_width_, device_height_,
          *global_game_content_path + "/fonts/DroidSans.ttf"));
#endif

  clearWebKitBuffers();
  glXMakeCurrent(x_display_, 0, 0);
}

void LBGraphicsLinux::SetWebKitCompositeEnable(bool enable) {
  DCHECK(web_view_host_);
  if (enable && !webkit_message_loop_) {
    webkit_message_loop_ = MessageLoop::current();
  }
  webkit_composite_enabled_ = enable;
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

bool LBGraphicsLinux::WebKitCompositeEnabled() {
  return webkit_composite_enabled_;
}


void LBGraphicsLinux::updateWebKitTexture() {
  // Query if the webkit frame is ready.
  // If it is, copy it into our texture for display via the PBO.
  if (compositor_context_->PixelBufferReady()) {
    GLuint pbo = compositor_context_->TakeNextPixelBuffer();

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBindTexture(GL_TEXTURE_2D,
                  webkit_main_texture_);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    device_width_,
                    device_height_,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFlush();

    ++global_webkit_frame_counter;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    if (last_webkit_frame_time_ > 0) {
      double current_time = base::Time::Now().ToDoubleT();
      double frame_time = current_time - last_webkit_frame_time_;
      if (frame_time > longest_webkit_frame_time_) {
        longest_webkit_frame_time_ = frame_time;
      }

      if (++webkit_frame_counter_ >= num_samples_for_fps_) {
        webkit_fps_ = num_samples_for_fps_
                        / (current_time - average_webkit_start_time_);
        worst_webkit_fps_ = 1.0 / longest_webkit_frame_time_;

        // reset for next update
        webkit_frame_counter_ = 0;
        average_webkit_start_time_ = current_time;
        longest_webkit_frame_time_ = 0;

        num_samples_for_fps_ = webkit_fps_ * 2;
        if (num_samples_for_fps_ < 5) num_samples_for_fps_ = 5;
        if (num_samples_for_fps_ > 100) num_samples_for_fps_ = 100;
      }
      last_webkit_frame_time_ = current_time;
    } else {
      last_webkit_frame_time_ = base::Time::Now().ToDoubleT();
      average_webkit_start_time_ = last_webkit_frame_time_;
    }
#endif
  }
}

void LBGraphicsLinux::setDrawState() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, device_width_, device_height_);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}

void LBGraphicsLinux::bindVertexData() {
  // enable positions + texcoords
  glBindBuffer(GL_ARRAY_BUFFER, full_screen_vbo_);
  glVertexAttribPointer(kPositionAttrib,
                        2, GL_FLOAT, GL_FALSE, sizeof(Vert), 0);
  glVertexAttribPointer(kTexcoordAttrib,
                        2, GL_FLOAT, GL_FALSE, sizeof(Vert),
                        static_cast<const char*>(0) + sizeof(float)*2);

  glEnableVertexAttribArray(kPositionAttrib);
  glEnableVertexAttribArray(kTexcoordAttrib);
}

void LBGraphicsLinux::drawWebKitQuad() {
  // draw solid quad, no alpha
  glDisable(GL_BLEND);

  glUseProgram(program_webquad_);
  bindVertexData();

  // configure most recent webkit texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, webkit_main_texture_);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // draw webkit textured quad
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void LBGraphicsLinux::updateSpinnerState() {
  if (spinner_state_ != VISIBLE) return;
  double current_time = static_cast<double>(LB::Platform::TickCount()) /
      (1000.0 * 1000.0);
  double diff = current_time - spinner_frame_time_;
  spinner_rotation_ += static_cast<float>(kSpinnerAngularVelocity * diff);
  spinner_frame_time_ = current_time;
}

void LBGraphicsLinux::drawSpinner() {
  if (spinner_state_ != VISIBLE) return;
  // alpha needed for the spinner
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glUseProgram(program_spinner_);

  bindVertexData();

  glUniform2fv(vert_spinner_scale_param_, 1, spinner_scale_);
  glUniform2fv(vert_spinner_offset_param_, 1, spinner_offset_);
  glUniform1f(vert_spinner_rotation_param_, spinner_rotation_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, spinner_texture_);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void LBGraphicsLinux::ShowSpinner() {
  spinner_state_ = VISIBLE;
}

void LBGraphicsLinux::HideSpinner() {
  spinner_state_ = INVISIBLE;
}

void LBGraphicsLinux::EnableDimming() {
  // TODO: implement dimming
}

void LBGraphicsLinux::DisableDimming() {
  // TODO: implement dimming
}

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
void LBGraphicsLinux::TakeScreenshot(const std::string& filename) {
  DCHECK(flip_thread_assigned_);
  if (pthread_equal(flip_thread_, pthread_self())) {
    screenshot_requested_ = true;
    screenshot_filename_ = filename;

    UpdateAndDrawFrame();
    BlockUntilFlip();
    DCHECK(!screenshot_requested_);
  } else {
    base::AutoLock lock(screenshot_lock_);
    screenshot_requested_ = true;
    screenshot_filename_ = filename;

    screenshot_taken_cond_.Wait();
    DCHECK(!screenshot_requested_);
  }
}

void LBGraphicsLinux::checkAndPossiblyTakeScreenshot() {
  base::AutoLock lock(screenshot_lock_);
  if (!screenshot_requested_) {
    return;
  }

  screenshot_requested_ = false;

  std::string file_path;
  if (screenshot_filename_ == "") {
    // compose filename from epoch time and const prefix
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    file_path = base::StringPrintf("%s/screenshot_%ld.png",
        global_screenshot_output_path, ts.tv_sec);
  } else {
    // If an absolute path is given for screenshot output,
    // don't prepend the screenshot output path
    if (screenshot_filename_[0] == '/') {
      file_path = screenshot_filename_ + ".png";
    } else {
      file_path = base::StringPrintf("%s/%s.png",
          global_screenshot_output_path, screenshot_filename_.c_str());
    }
  }

  DLOG(INFO) << "Writing screenshot to '" << file_path.c_str() << "'";

  screenshot_filename_ = "";

  const int frame_buffer_pitch = device_width_ * 3;
  void* pixels = malloc(device_height_ * frame_buffer_pitch);
  glReadPixels(0,
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
  web_view_host_->output(base::StringPrintf("wrote screenshot to %s",
                         file_path.c_str()));

  screenshot_taken_cond_.Signal();
}

namespace {
  void png_write_row_callback(void*,
                              uint32_t,
                              int) {
    // no-op
  }
}

void LBGraphicsLinux::saveTextureToDisk(void* pixels,
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

#endif

void LBGraphicsLinux::UpdateAndDrawFrame() {
  GLXContext c = glXGetCurrentContext();
  if (c) {
    DLOG(ERROR) << "glXGetCurrentContext was expected to be NULL.";
  }
  bool ret = glXMakeCurrent(x_display_, x_window_, gl_context_);
  if (!ret) {
    DLOG(ERROR) << "glXMakeCurrent failed.";
  }
  updateWebKitTexture();
  setDrawState();
  drawWebKitQuad();
  updateSpinnerState();
  drawSpinner();

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  drawOnScreenDisplay();
#endif
}

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
#define MB(x) (x >> 20)
#define KB(x) (x >> 10)

void LBGraphicsLinux::drawOnScreenDisplay() {
  // Calculate running time
  time_t running = time(NULL) - s_start_time;
  global_lifetime = running / 60;

  if (osd_visible_) {
  // update FPS statistics
    if (++frame_counter_ > 100) {
      int shell_allocator_bytes = OSAllocator::getCurrentBytesAllocated();
      frame_rate_string_ = base::StringPrintf(
          "FPS: %3.1f avg, %3.1f min    JS(KB): %5d    ",
          webkit_fps_, worst_webkit_fps_, KB(shell_allocator_bytes));
#if LB_MEMORY_COUNT
      lb_memory_info_t info;
      lb_memory_stats(&info);
      frame_rate_string_.append(base::StringPrintf(
          " RAM(M): %3d app, %2d sys, %3d free, %d exe ",
          MB(info.application_memory), MB(info.system_memory),
          MB(info.free_memory),
          MB(info.executable_size)));
#endif
      // reset for next update
      frame_counter_ = 0;
    }
    std::string status_line = frame_rate_string_;
#if ENABLE(INSPECTOR)
    status_line.append(base::StringPrintf(
        " %d DOM Nodes",
        WebCore::InspectorCounters::counterValue(
            WebCore::InspectorCounters::NodeCounter)));
#endif
    int skia_bytes = sk_get_bytes_allocated();
    status_line.append(base::StringPrintf(
        "  Skia(KB): %5d  ", KB(skia_bytes)));

    gl_text_printer_->Printf(-0.975, -0.950, status_line.c_str());
  }
}
#endif

int LBGraphicsLinux::NumCompositeBuffers() const {
  return LBWebGraphicsContext3DLinux::kNumberOfWebKitPixelBuffers;
}

void LBGraphicsLinux::BlockUntilFlip() {
  flip_thread_ = pthread_self();
  flip_thread_assigned_ = true;

  glFlush();

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  checkAndPossiblyTakeScreenshot();
#endif

  glXSwapBuffers(x_display_, x_window_);

  glXMakeCurrent(x_display_, 0, 0);
}


#if defined(__LB_SHELL__ENABLE_CONSOLE__)
const std::string LBGraphicsLinux::PrintTextureSummary() {
  NOTIMPLEMENTED();
  return std::string();
}

void LBGraphicsLinux::WatchTexture(uint32_t handle) {
  NOTIMPLEMENTED();
}
void LBGraphicsLinux::SaveTexture(uint32_t handle, const char* prefix) {
  NOTIMPLEMENTED();
}
#endif


LBWebGraphicsContext3D* LBGraphicsLinux::GetCompositorContext() {
  return compositor_context_.get();
}
