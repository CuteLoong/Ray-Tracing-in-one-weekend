#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include <iostream>

void write_color(std::ostream &out, color pixel_color, int samples_per_pixel)
{
    auto r = pixel_color[0];
    auto b = pixel_color[1];
    auto g = pixel_color[2];

    // calculate color by number of samples
    auto scale = 1.0 / samples_per_pixel;
    r *= scale;
    r = pow(r, 1.0 / 2.2);
    b *= scale;
    b = pow(b, 1.0 / 2.2);
    g *= scale;
    g = pow(g, 1.0 / 2.2);

    out << static_cast<int>(256 * clamp(r, 0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(b, 0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(g, 0, 0.999)) << '\n';
}
#endif