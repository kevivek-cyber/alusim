// ---------------------------------------------------------------------------
// Canvas.h  --  a virtual pixel buffer plus our own rasterisation algorithms
//
// Everything the simulator draws goes through this class.  Nothing here calls a
// library "draw line" routine -- the line, circle, fill and clipping code is
// written out longhand, because those algorithms ARE the Computer Graphics
// practicals.  Today the buffer is rendered to the terminal as characters; the
// same Canvas can later be blitted to an SDL2/OpenGL window without any of the
// algorithms below changing.
//
// Covers: CGL Practical 2 (Bresenham line + circle, DDA line),
//         CGL Practical 3 (scan-line polygon fill, boundary fill, flood fill),
//         CGL Practical 5 (Cohen-Sutherland line clipping),
//         CG Unit 4       (2D translation used to animate signal pulses)
// ---------------------------------------------------------------------------
#ifndef UI_CANVAS_H
#define UI_CANVAS_H

#include <string>
#include <ostream>
#include <cstddef>

namespace ui {

// A "colour" here is the character used when the buffer is drawn to the
// terminal.  When we move to a real graphics window this becomes an RGB value
// and none of the algorithms need to change.
typedef char Pixel;

const Pixel PX_EMPTY  = ' ';
const Pixel PX_WIRE   = '.';   // idle wire
const Pixel PX_ACTIVE = '#';   // wire carrying data this cycle
const Pixel PX_BORDER = '+';
const Pixel PX_FILL   = ':';

struct Point {
    int x, y;
    Point() : x(0), y(0) {}
    Point(int px, int py) : x(px), y(py) {}
};

// The clipping window used by Cohen-Sutherland.
struct ClipWindow {
    int xmin, ymin, xmax, ymax;
    ClipWindow() : xmin(0), ymin(0), xmax(0), ymax(0) {}
    ClipWindow(int a, int b, int c, int d) : xmin(a), ymin(b), xmax(c), ymax(d) {}
};

class Canvas {
private:
    int    width_;
    int    height_;
    Pixel* buffer_;

    // Cohen-Sutherland region codes
    enum { INSIDE = 0, LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8 };
    int  regionCode(int x, int y, const ClipWindow& w) const;

public:
    Canvas(int width, int height);
    Canvas(const Canvas& other);
    Canvas& operator=(const Canvas& other);
    ~Canvas();

    int width()  const { return width_; }
    int height() const { return height_; }

    void  clear(Pixel p = PX_EMPTY);
    void  setPixel(int x, int y, Pixel p);
    Pixel getPixel(int x, int y) const;
    bool  inBounds(int x, int y) const;

    // -- CGL Practical 2 -----------------------------------------------------
    void drawLineBresenham(int x0, int y0, int x1, int y1, Pixel p);
    void drawLineDDA(int x0, int y0, int x1, int y1, Pixel p);
    void drawCircleBresenham(int cx, int cy, int radius, Pixel p);

    // -- CGL Practical 5 -----------------------------------------------------
    // Clips the segment to the window, then draws what survives with Bresenham.
    // Returns false if the segment is entirely outside (rejected).
    bool clipAndDrawLine(int x0, int y0, int x1, int y1,
                         const ClipWindow& win, Pixel p);
    static bool cohenSutherlandClip(int& x0, int& y0, int& x1, int& y1,
                                    const ClipWindow& win);

    // -- CGL Practical 3 -----------------------------------------------------
    void drawRect(int x, int y, int w, int h, Pixel p);
    void fillRect(int x, int y, int w, int h, Pixel p);
    // scan-line fill of an arbitrary polygon
    void scanlineFillPolygon(const Point* pts, int count, Pixel p);
    void boundaryFill(int x, int y, Pixel boundary, Pixel fill);
    void floodFill(int x, int y, Pixel target, Pixel fill);

    // -- text drawn into the buffer -----------------------------------------
    void drawText(int x, int y, const std::string& text);
    void drawBox(int x, int y, int w, int h, const std::string& title);

    // -- output --------------------------------------------------------------
    void render(std::ostream& os) const;
};

} // namespace ui

#endif // UI_CANVAS_H
