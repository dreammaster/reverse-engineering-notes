// A stand-in for the one part of the glm vector maths library the way system
// uses (circleLineSegmentIntersection() takes glm::tvec2<float> arguments); the
// names are glm's own, as with WxStub.h and SdlStub.h.
#pragma once

namespace glm {

template<typename T>
struct tvec2 {
	T x, y;
};

typedef tvec2<float> vec2;

} // End of namespace glm
