#include "Canvas.h"
#include <cstdlib>

namespace ui {

Canvas::Canvas(int width, int height)
    : width_(width > 0 ? width : 1),
      height_(height > 0 ? height : 1),
      buffer_(0) {
    buffer_ = new Pixel[width_ * height_];
    clear();
}

Canvas::Canvas(const Canvas& other)
    : width_(other.width_), height_(other.height_), buffer_(0) {
    buffer_ = new Pixel[width_ * height_];
    for (int i = 0; i < width_ * height_; ++i) buffer_[i] = other.buffer_[i];
}

Canvas& Canvas::operator=(const Canvas& other) {
    if (this != &other) {
        delete[] buffer_;
        width_  = other.width_;
        height_ = other.height_;
        buffer_ = new Pixel[width_ * height_];
        for (int i = 0; i < width_ * height_; ++i) buffer_[i] = other.buffer_[i];
    }
    return *this;
}

Canvas::~Canvas() { delete[] buffer_; }

bool Canvas::inBounds(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

void Canvas::clear(Pixel p) {
    for (int i = 0; i < width_ * height_; ++i) buffer_[i] = p;
}

void Canvas::setPixel(int x, int y, Pixel p) {
    if (inBounds(x, y)) buffer_[y * width_ + x] = p;
}

Pixel Canvas::getPixel(int x, int y) const {
    if (!inBounds(x, y)) return PX_EMPTY;
    return buffer_[y * width_ + x];
}

// ---------------------------------------------------------------------------
// Bresenham's line algorithm  (CGL Practical 2)
//
// Integer arithmetic only -- no floating point, no rounding.  The decision
// variable 'err' tracks how far the true line has drifted from the pixel grid.
// ---------------------------------------------------------------------------
void Canvas::drawLineBresenham(int x0, int y0, int x1, int y1, Pixel p) {
    int dx =  std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        setPixel(x0, y0, p);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// ---------------------------------------------------------------------------
// DDA line algorithm  (CGL Practical 2 "further practice")
//
// Kept alongside Bresenham so the two can be compared: DDA uses floating-point
// increments, Bresenham does not.
// ---------------------------------------------------------------------------
void Canvas::drawLineDDA(int x0, int y0, int x1, int y1, Pixel p) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = (std::abs(dx) > std::abs(dy)) ? std::abs(dx) : std::abs(dy);
    if (steps == 0) { setPixel(x0, y0, p); return; }

    double xInc = static_cast<double>(dx) / steps;
    double yInc = static_cast<double>(dy) / steps;
    double x = x0, y = y0;

    for (int i = 0; i <= steps; ++i) {
        setPixel(static_cast<int>(x + 0.5), static_cast<int>(y + 0.5), p);
        x += xInc;
        y += yInc;
    }
}

// ---------------------------------------------------------------------------
// Bresenham's circle algorithm  (CGL Practical 2)
//
// Computes one octant and mirrors it into the other seven.
// ---------------------------------------------------------------------------
void Canvas::drawCircleBresenham(int cx, int cy, int radius, Pixel p) {
    if (radius <= 0) { setPixel(cx, cy, p); return; }

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (y >= x) {
        // eight-way symmetry
        setPixel(cx + x, cy + y, p);
        setPixel(cx - x, cy + y, p);
        setPixel(cx + x, cy - y, p);
        setPixel(cx - x, cy - y, p);
        setPixel(cx + y, cy + x, p);
        setPixel(cx - y, cy + x, p);
        setPixel(cx + y, cy - x, p);
        setPixel(cx - y, cy - x, p);

        ++x;
        if (d > 0) { --y; d = d + 4 * (x - y) + 10; }
        else       {      d = d + 4 * x + 6; }
    }
}

// ---------------------------------------------------------------------------
// Cohen-Sutherland line clipping  (CGL Practical 5)
//
// Each endpoint gets a 4-bit region code.  If both are 0 the segment is fully
// inside (trivially accepted); if their AND is non-zero they share an outside
// region so the segment is trivially rejected; otherwise we push the outside
// endpoint onto the boundary and try again.
// ---------------------------------------------------------------------------
int Canvas::regionCode(int x, int y, const ClipWindow& w) const {
    int code = INSIDE;
    if (x < w.xmin)      code |= LEFT;
    else if (x > w.xmax) code |= RIGHT;
    if (y < w.ymin)      code |= BOTTOM;
    else if (y > w.ymax) code |= TOP;
    return code;
}

bool Canvas::cohenSutherlandClip(int& x0, int& y0, int& x1, int& y1,
                                 const ClipWindow& w) {
    // local copy of the region-code helper so this can stay static
    struct Local {
        static int code(int x, int y, const ClipWindow& win) {
            int c = 0;
            if (x < win.xmin)      c |= 1;
            else if (x > win.xmax) c |= 2;
            if (y < win.ymin)      c |= 4;
            else if (y > win.ymax) c |= 8;
            return c;
        }
    };

    int c0 = Local::code(x0, y0, w);
    int c1 = Local::code(x1, y1, w);

    for (;;) {
        if ((c0 | c1) == 0) return true;    // trivially accept
        if ((c0 & c1) != 0) return false;   // trivially reject

        int cOut = c0 ? c0 : c1;
        int x = 0, y = 0;

        if (cOut & 8) {            // above
            if (y1 != y0) x = x0 + (x1 - x0) * (w.ymax - y0) / (y1 - y0);
            y = w.ymax;
        } else if (cOut & 4) {     // below
            if (y1 != y0) x = x0 + (x1 - x0) * (w.ymin - y0) / (y1 - y0);
            y = w.ymin;
        } else if (cOut & 2) {     // right
            if (x1 != x0) y = y0 + (y1 - y0) * (w.xmax - x0) / (x1 - x0);
            x = w.xmax;
        } else {                   // left
            if (x1 != x0) y = y0 + (y1 - y0) * (w.xmin - x0) / (x1 - x0);
            x = w.xmin;
        }

        if (cOut == c0) { x0 = x; y0 = y; c0 = Local::code(x0, y0, w); }
        else            { x1 = x; y1 = y; c1 = Local::code(x1, y1, w); }
    }
}

bool Canvas::clipAndDrawLine(int x0, int y0, int x1, int y1,
                             const ClipWindow& win, Pixel p) {
    if (!cohenSutherlandClip(x0, y0, x1, y1, win)) return false;
    drawLineBresenham(x0, y0, x1, y1, p);
    return true;
}

// ---------------------------------------------------------------------------
// Rectangles
// ---------------------------------------------------------------------------
void Canvas::drawRect(int x, int y, int w, int h, Pixel p) {
    if (w <= 0 || h <= 0) return;
    drawLineBresenham(x,         y,         x + w - 1, y,         p);
    drawLineBresenham(x,         y + h - 1, x + w - 1, y + h - 1, p);
    drawLineBresenham(x,         y,         x,         y + h - 1, p);
    drawLineBresenham(x + w - 1, y,         x + w - 1, y + h - 1, p);
}

void Canvas::fillRect(int x, int y, int w, int h, Pixel p) {
    for (int j = y; j < y + h; ++j)
        for (int i = x; i < x + w; ++i)
            setPixel(i, j, p);
}

// ---------------------------------------------------------------------------
// Scan-line polygon fill  (CGL Practical 3)
//
// For each scan line, find where it crosses the polygon edges, sort those
// x-intersections, and fill between consecutive pairs.
// ---------------------------------------------------------------------------
void Canvas::scanlineFillPolygon(const Point* pts, int count, Pixel p) {
    if (count < 3) return;

    int yMin = pts[0].y, yMax = pts[0].y;
    for (int i = 1; i < count; ++i) {
        if (pts[i].y < yMin) yMin = pts[i].y;
        if (pts[i].y > yMax) yMax = pts[i].y;
    }
    if (yMin < 0) yMin = 0;
    if (yMax >= height_) yMax = height_ - 1;

    int* xs = new int[count];

    for (int y = yMin; y <= yMax; ++y) {
        int n = 0;

        for (int i = 0; i < count; ++i) {
            const Point& a = pts[i];
            const Point& b = pts[(i + 1) % count];
            if (a.y == b.y) continue;                     // ignore horizontal edges

            int lo = (a.y < b.y) ? a.y : b.y;
            int hi = (a.y < b.y) ? b.y : a.y;
            // half-open rule stops double-counting shared vertices
            if (y >= lo && y < hi) {
                int x = a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
                xs[n++] = x;
            }
        }

        // insertion sort of the crossings
        for (int i = 1; i < n; ++i) {
            int key = xs[i], j = i - 1;
            while (j >= 0 && xs[j] > key) { xs[j + 1] = xs[j]; --j; }
            xs[j + 1] = key;
        }

        for (int i = 0; i + 1 < n; i += 2)
            for (int x = xs[i]; x <= xs[i + 1]; ++x)
                setPixel(x, y, p);
    }

    delete[] xs;
}

// ---------------------------------------------------------------------------
// Boundary fill and flood fill  (CGL Practical 3 "further practice")
//
// Written iteratively with an explicit stack rather than recursively, so a
// large region cannot blow the call stack.
// ---------------------------------------------------------------------------
void Canvas::boundaryFill(int x, int y, Pixel boundary, Pixel fill) {
    if (!inBounds(x, y)) return;

    int  cap = width_ * height_;
    int* stackX = new int[cap];
    int* stackY = new int[cap];
    int  top = 0;
    stackX[top] = x; stackY[top] = y; ++top;

    while (top > 0) {
        --top;
        int cx = stackX[top], cy = stackY[top];
        if (!inBounds(cx, cy)) continue;
        Pixel cur = getPixel(cx, cy);
        if (cur == boundary || cur == fill) continue;

        setPixel(cx, cy, fill);
        if (top + 4 < cap) {
            stackX[top] = cx + 1; stackY[top] = cy;     ++top;
            stackX[top] = cx - 1; stackY[top] = cy;     ++top;
            stackX[top] = cx;     stackY[top] = cy + 1; ++top;
            stackX[top] = cx;     stackY[top] = cy - 1; ++top;
        }
    }

    delete[] stackX;
    delete[] stackY;
}

void Canvas::floodFill(int x, int y, Pixel target, Pixel fill) {
    if (target == fill) return;
    if (!inBounds(x, y)) return;

    int  cap = width_ * height_;
    int* stackX = new int[cap];
    int* stackY = new int[cap];
    int  top = 0;
    stackX[top] = x; stackY[top] = y; ++top;

    while (top > 0) {
        --top;
        int cx = stackX[top], cy = stackY[top];
        if (!inBounds(cx, cy)) continue;
        if (getPixel(cx, cy) != target) continue;

        setPixel(cx, cy, fill);
        if (top + 4 < cap) {
            stackX[top] = cx + 1; stackY[top] = cy;     ++top;
            stackX[top] = cx - 1; stackY[top] = cy;     ++top;
            stackX[top] = cx;     stackY[top] = cy + 1; ++top;
            stackX[top] = cx;     stackY[top] = cy - 1; ++top;
        }
    }

    delete[] stackX;
    delete[] stackY;
}

// ---------------------------------------------------------------------------
// Text and boxes
// ---------------------------------------------------------------------------
void Canvas::drawText(int x, int y, const std::string& text) {
    for (size_t i = 0; i < text.size(); ++i)
        setPixel(x + static_cast<int>(i), y, text[i]);
}

void Canvas::drawBox(int x, int y, int w, int h, const std::string& title) {
    if (w < 2 || h < 2) return;

    for (int i = x + 1; i < x + w - 1; ++i) {
        setPixel(i, y,         '-');
        setPixel(i, y + h - 1, '-');
    }
    for (int j = y + 1; j < y + h - 1; ++j) {
        setPixel(x,         j, '|');
        setPixel(x + w - 1, j, '|');
    }
    setPixel(x,         y,         '+');
    setPixel(x + w - 1, y,         '+');
    setPixel(x,         y + h - 1, '+');
    setPixel(x + w - 1, y + h - 1, '+');

    if (!title.empty() && w > 4) {
        std::string t = title;
        if (static_cast<int>(t.size()) > w - 4) t = t.substr(0, w - 4);
        drawText(x + 2, y, t);
    }
}

void Canvas::render(std::ostream& os) const {
    for (int y = 0; y < height_; ++y) {
        // trim trailing blanks so the terminal output stays tidy
        int last = -1;
        for (int x = 0; x < width_; ++x)
            if (buffer_[y * width_ + x] != PX_EMPTY) last = x;
        for (int x = 0; x <= last; ++x)
            os << buffer_[y * width_ + x];
        os << '\n';
    }
}

} // namespace ui
