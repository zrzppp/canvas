#ifndef _SURFACE_H_
#define _SURFACE_H_

#include "Color.h"
#include "FilterMode.h"
#include "InternalFormat.h"
#include "Path.h"
#include "Style.h"
#include "Font.h"
#include "TextBaseline.h"
#include "TextAlign.h"
#include "TextMetrics.h"
#include "Operator.h"

#include <memory>

namespace canvas {
  class Context;
  class Image;

  enum RenderMode {
    FILL = 1,
    STROKE
  };

  class Surface {
  public:
    friend class Context;

  Surface(unsigned int _logical_width, unsigned int _logical_height, unsigned int _actual_width, unsigned int _actual_height, bool _has_alpha) :      
      logical_width(_logical_width),
      logical_height(_logical_height),
      actual_width(_actual_width),
      actual_height(_actual_height),
      has_alpha(_has_alpha) { }
    Surface(const Surface & other) = delete;
    Surface & operator=(const Surface & other) = delete;
    virtual ~Surface() {
      delete[] scaled_buffer;
    }

    virtual void resize(unsigned int _logical_width, unsigned int _logical_height, unsigned int _actual_width, unsigned int _actual_height, bool _has_alpha) {
      logical_width = _logical_width;
      logical_height = _logical_height;
      actual_width = _actual_width;
      actual_height = _actual_height;
      has_alpha = _has_alpha;
    }

    virtual void flush() { }
    virtual void markDirty() { }

    // virtual Surface * copy() = 0;
    virtual void * lockMemory(bool write_access = false) = 0;
    virtual void * lockMemoryPartial(unsigned int x0, unsigned int y0, unsigned int required_width, unsigned int required_height);
    virtual void releaseMemory() {
      delete[] scaled_buffer;
      scaled_buffer = 0;
    }

    virtual void clip(const Path & path, float display_scale) = 0;
    virtual void resetClip() = 0;
    virtual void renderPath(RenderMode mode, const Path & path, const Style & style, float lineWidth, Operator op, float display_scale, float globalAlpha) = 0;
    virtual void renderText(RenderMode mode, const Font & font, const Style & style, TextBaseline textBaseline, TextAlign textAlign, const std::string & text, double x, double y, float lineWidth, Operator op, float display_scale, float alpha) = 0;
    virtual TextMetrics measureText(const Font & font, const std::string & text, float display_scale) = 0;
	  
    virtual void drawImage(Surface & _img, double x, double y, double w, double h, float alpha = 1.0f, bool imageSmoothingEnabled = true) = 0;
    virtual void drawImage(const Image & _img, double x, double y, double w, double h, float alpha = 1.0f, bool imageSmoothingEnabled = true) = 0;
    virtual void save() = 0;
    virtual void restore() = 0;

    void colorFill(const Color & color);
    void slowBlur(float hradius, float vradius, float alpha = 1.0f);
    void blur(float r);

    void multiply(const Color & color);
    
    std::shared_ptr<Image> createImage();

    unsigned int getLogicalWidth() const { return logical_width; }
    unsigned int getLogicalHeight() const { return logical_height; }
    unsigned int getActualWidth() const { return actual_width; }
    unsigned int getActualHeight() const { return actual_height; }
    bool hasAlpha() const { return has_alpha; }

    void setMagFilter(FilterMode mode) { mag_filter = mode; }
    void setMinFilter(FilterMode mode) { min_filter = mode; }
    void setTargetFormat(InternalFormat format) { target_format = format; }

    FilterMode getMagFilter() const { return mag_filter; }
    FilterMode getMinFilter() const { return min_filter; }
    InternalFormat getTargetFormat() const { return target_format; }
    
  protected:
    static bool isPNG(const unsigned char * buffer, size_t size);
    static bool isJPEG(const unsigned char * buffer, size_t size);
    static bool isGIF(const unsigned char * buffer, size_t size);
    static bool isXML(const unsigned char * buffer, size_t size);

  private:
    unsigned int logical_width, logical_height, actual_width, actual_height;
    FilterMode mag_filter = LINEAR;
    FilterMode min_filter = LINEAR;
    InternalFormat target_format = RGBA8;
    unsigned int * scaled_buffer = 0;
    bool has_alpha;
  };
};

#endif
