// SPDX-License-Identifier: GPL-2.0-or-later
#include "../native/model2_scene.h"
#include <cassert>
int main() {
    using namespace rchase2;
    NativeFrame f;f.width=496;f.height=384;f.yoffset=128;
    NativePolygon foreign;foreign.count=4;foreign.focus={600,600};foreign.center={248,320};foreign.window=2;
    NativePolygon world=foreign;world.focus={768,768};world.window=1;
    f.polygons={foreign,foreign,foreign,world};
    // Captured Rail Chase 2 optics are stable even when another focused layer
    // outnumbers the world. Polygon count must not change stereo world scale.
    auto lens=native_camera_lens(f);
    assert(lens.fx==768 && lens.fy==768 && lens.cx==248 && lens.cy==192);
    f.polygons.assign(20,foreign);assert(native_camera_lens(f).fx==768);
    f.lens={496,384,400,400,248,191};
    assert(native_camera_lens(f).fx==400 && presentation_lens(f).fx==400);
    f.lens={};f.polygons.clear();assert(presentation_lens(f).fx==768);
}
