#include "ContextGDIPlus.h"

#include "utf8.h"

bool canvas::ContextGDIPlus::is_initialized = false;
ULONG_PTR canvas::ContextGDIPlus::m_gdiplusToken;

using namespace std;
using namespace canvas;

static std::wstring convert_to_wstring(const std::string & input) {
  const char * str = input.c_str();
  const char * str_i = str;
  const char * end = str + input.size();
  std::wstring output;
  while (str_i < end) {
    output += (wchar_t)utf8::next(str_i, end);
  }
  return output;
}

static void toGDIPath(const Path & path, Gdiplus::GraphicsPath & output) {
  output.StartFigure();
  Gdiplus::PointF current_pos;

  for (auto pc : path.getData()) {
    switch (pc.type) {
    case PathComponent::MOVE_TO:
      current_pos = Gdiplus::PointF(Gdiplus::REAL(pc.x0), Gdiplus::REAL(pc.y0));
      break;
    case PathComponent::LINE_TO:
      {
	Gdiplus::PointF point(Gdiplus::REAL(pc.x0), Gdiplus::REAL(pc.y0));
	output.AddLine(current_pos, point);
	current_pos = point;
      }
      break;
    case PathComponent::CLOSE:
      output.CloseFigure();
      break;
    case PathComponent::ARC:
      {
	double span = 0;
	if (0 && ((!pc.anticlockwise && (pc.ea - pc.sa >= 2 * M_PI)) || (pc.anticlockwise && (pc.sa - pc.ea >= 2 * M_PI)))) {
	  // If the anticlockwise argument is false and endAngle-startAngle is equal to or greater than 2*PI, or, if the
	  // anticlockwise argument is true and startAngle-endAngle is equal to or greater than 2*PI, then the arc is the whole
	  // circumference of this circle.
	  span = 2 * M_PI;
	} else {
	  if (!pc.anticlockwise && (pc.ea < pc.sa)) {
	    span += 2 * M_PI;
	  } else if (pc.anticlockwise && (pc.sa < pc.ea)) {
	    span -= 2 * M_PI;
	  }
 
#if 0
    // this is also due to switched coordinate system
    // we would end up with a 0 span instead of 360
    if (!(qFuzzyCompare(span + (ea - sa) + 1, 1.0) && qFuzzyCompare(qAbs(span), 360.0))) {
      // mod 360
      span += (ea - sa) - (static_cast<int>((ea - sa) / 360)) * 360;
    }
#else
	  span += pc.ea - pc.sa;
#endif
	}
 
#if 0
  // If the path is empty, move to where the arc will start to avoid painting a line from (0,0)
  // NOTE: QPainterPath::isEmpty() won't work here since it ignores a lone MoveToElement
  if (!m_path.elementCount())
    m_path.arcMoveTo(xs, ys, width, height, sa);
  else if (!radius) {
    m_path.lineTo(xc, yc);
    return;
  }
#endif

#if 0
  if (anticlockwise) {
    span = -M_PI / 2.0;
  } else {
    span = M_PI / 2.0;
  }
#endif
  Gdiplus::RectF rect(Gdiplus::REAL(pc.x0 - pc.radius), Gdiplus::REAL(pc.y0 - pc.radius), Gdiplus::REAL(2 * pc.radius), Gdiplus::REAL(2 * pc.radius));

	output.AddArc(rect, Gdiplus::REAL(pc.sa * 180.0f / M_PI), Gdiplus::REAL(span * 180.0f / M_PI));
	output.GetLastPoint(&current_pos);
      }
      break;
    }
  }
}

static Gdiplus::Color toGDIColor(const Color & input) {
  int red = int(input.red * 255), green = int(input.green * 255), blue = int(input.blue * 255), alpha = int(input.alpha * 255);
  if (red < 0) red = 0;
  else if (red > 255) red = 255;
  if (green < 0) green = 0;
  else if (green > 255) green = 255;
  if (blue < 0) blue = 0;
  else if (blue > 255) blue = 255;
  if (alpha < 0) alpha = 0;
  else if (alpha > 255) alpha = 255;
#if 0
  return Gdiplus::Color::FromArgb(alpha, red, green, blue);
#else
  return Gdiplus::Color(alpha, red, green, blue);
#endif
}

GDIPlusSurface::GDIPlusSurface(const std::string & filename) : Surface(0, 0) {
  std::wstring tmp = convert_to_wstring(filename);
  bitmap = std::shared_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(tmp.data()));
  Surface::resize(bitmap->GetWidth(), bitmap->GetHeight());
  g = std::shared_ptr<Gdiplus::Graphics>(new Gdiplus::Graphics(&(*bitmap)));
  initialize();
}

void
GDIPlusSurface::fill(const Path & input_path, const Style & style) {
  Gdiplus::GraphicsPath path;
  toGDIPath(input_path, path);
 
  if (style.getType() == Style::LINEAR_GRADIENT) {
    const std::map<float, Color> & colors = style.getColors();
    if (!colors.empty()) {
      std::map<float, Color>::const_iterator it0 = colors.begin(), it1 = colors.end();
      it1--;
      const Color & c0 = it0->second, c1 = it1->second;
      Gdiplus::LinearGradientBrush brush(Gdiplus::PointF(Gdiplus::REAL(style.x0), Gdiplus::REAL(style.y0)),
					 Gdiplus::PointF(Gdiplus::REAL(style.x1), Gdiplus::REAL(style.y1)),
					 toGDIColor(c0),
					 toGDIColor(c1));
      g->FillPath(&brush, &path);
    }
  } else {
    Gdiplus::SolidBrush brush(toGDIColor(style.color));
    g->FillPath(&brush, &path);
  }
}

void
GDIPlusSurface::stroke(const Path & input_path, const Style & style, double lineWidth) {
  Gdiplus::GraphicsPath path;
  toGDIPath(input_path, path);
  Gdiplus::Pen pen(toGDIColor(style.color), lineWidth);
  g->DrawPath(&pen, &path);
}

void
GDIPlusSurface::clip(const Path & input_path) {
  Gdiplus::GraphicsPath path;
  toGDIPath(input_path, path);

  Gdiplus::Region region(&path);
  g->SetClip(&region);
}

void
GDIPlusSurface::drawImage(Surface & _img, double x, double y, double w, double h, float alpha) {
  GDIPlusSurface & img = dynamic_cast<GDIPlusSurface&>(_img);
  if (alpha < 1.0f && 0) {
#if 0
    ImageAttributes  imageAttributes;
    ColorMatrix colorMatrix = {
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, alpha, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
   
    imageAttributes.SetColorMatrix( &colorMatrix, 
				    ColorMatrixFlagsDefault,
				    ColorAdjustTypeBitmap);
    graphics.DrawImage( &(*(img.bitmap)),
			Gdiplus::Rect(x, y, w, h),  // destination rectangle 
			0, 0,        // upper-left corner of source rectangle 
			getWidth(),       // width of source rectangle
			getHeight(),      // height of source rectangle
			Gdiplus::UnitPixel,
			&imageAttributes);
#endif
  } else {
    g->DrawImage(&(*(img.bitmap)), Gdiplus::REAL(x), Gdiplus::REAL(y), Gdiplus::REAL(w), Gdiplus::REAL(h));
  }
}

void
GDIPlusSurface::fillText(const Font & font, const Style & style, TextBaseline textBaseline, TextAlign textAlign, const std::string & text, double x, double y) {
  std::wstring text2 = convert_to_wstring(text);
  int style_bits = 0;
  if (font.weight == Font::BOLD || font.weight == Font::BOLDER) {
    style_bits |= Gdiplus::FontStyleBold;
  }
  if (font.slant == Font::ITALIC) {
    style_bits |= Gdiplus::FontStyleItalic;
  }
  Gdiplus::Font gdifont(&Gdiplus::FontFamily(L"Arial"), font.size, style_bits, Gdiplus::UnitPixel);
  Gdiplus::SolidBrush brush(toGDIColor(style.color));

  Gdiplus::RectF rect(Gdiplus::REAL(x), Gdiplus::REAL(y), 0.0f, 0.0f);
  Gdiplus::StringFormat f;

  switch (textBaseline.getType()) {
  case TextBaseline::TOP: break;
  case TextBaseline::HANGING: break;
  case TextBaseline::MIDDLE: f.SetLineAlignment(Gdiplus::StringAlignmentCenter); break;
  }

  switch (textAlign) {
  case TextAlign::CENTER: f.SetAlignment(Gdiplus::StringAlignmentCenter); break;
  case TextAlign::END: case TextAlign::RIGHT: f.SetAlignment(Gdiplus::StringAlignmentFar); break;
  case TextAlign::START: case TextAlign::LEFT: f.SetAlignment(Gdiplus::StringAlignmentNear); break;
  }

  g->DrawString(text2.data(), text2.size(), &gdifont, rect, &f, &brush);
}

TextMetrics
ContextGDIPlus::measureText(const std::string & text) {
  std::wstring text2 = convert_to_wstring(text);
  int style = 0;
  if (font.weight == Font::BOLD || font.weight == Font::BOLDER) {
    style |= Gdiplus::FontStyleBold;
  }
  if (font.slant == Font::ITALIC) {
    style |= Gdiplus::FontStyleItalic;
  }
  Gdiplus::Font font(&Gdiplus::FontFamily(L"Arial"), font.size, style, Gdiplus::UnitPixel);
  Gdiplus::RectF layoutRect(0, 0, 512, 512), boundingBox;
  default_surface.g->MeasureString(text2.data(), text2.size(), &font, layoutRect, &boundingBox);
  Gdiplus::SizeF size;
  boundingBox.GetSize(&size);
  
  return { (float)size.Width, (float)size.Height };
}
