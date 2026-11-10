#pragma once
#include "core/Frame.h"
#include "core/FrameSource.h"
#include "project/Project.h"
namespace at {
// CPU reference compositor (milestone 1).
// Draws all visible layers bottom-to-top with transform + opacity into 'out'.
// divisor: 1 = full resolution, 2 = half, ... (preview quality).
void renderComposition(const Project& project, const Composition& comp, double time,
                       IFrameSource& source, Frame& out, int divisor = 1);
}
