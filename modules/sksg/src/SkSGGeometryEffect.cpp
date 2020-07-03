/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/sksg/include/SkSGGeometryEffect.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkStrokeRec.h"
#include "include/effects/SkCornerPathEffect.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkTrimPathEffect.h"
#include "modules/sksg/src/SkSGTransformPriv.h"

namespace sksg {

GeometryEffect::GeometryEffect(sk_sp<GeometryNode> child)
    : fChild(std::move(child)) {
    SkASSERT(fChild);

    this->observeInval(fChild);
}

GeometryEffect::~GeometryEffect() {
    this->unobserveInval(fChild);
}

void GeometryEffect::onClip(SkCanvas* canvas, bool antiAlias) const {
    canvas->clipPath(fPath, SkClipOp::kIntersect, antiAlias);
}

void GeometryEffect::onDraw(SkCanvas* canvas, const SkPaint& paint) const {
    canvas->drawPath(fPath, paint);
}

bool GeometryEffect::onContains(const SkPoint& p) const {
    return fPath.contains(p.x(), p.y());
}

SkPath GeometryEffect::onAsPath() const {
    return fPath;
}

SkRect GeometryEffect::onRevalidate(InvalidationController* ic, const SkMatrix& ctm) {
    SkASSERT(this->hasInval());

    fChild->revalidate(ic, ctm);

    fPath = this->onRevalidateEffect(fChild);
    fPath.shrinkToFit();

    return fPath.computeTightBounds();
}

SkPath TrimEffect::onRevalidateEffect(const sk_sp<GeometryNode>& child) {
    SkPath path = child->asPath();

    if (const auto trim = SkTrimPathEffect::Make(fStart, fStop, fMode)) {
        SkStrokeRec rec(SkStrokeRec::kHairline_InitStyle);
        SkAssertResult(trim->filterPath(&path, path, &rec, nullptr));
    }

    return path;
}

GeometryTransform::GeometryTransform(sk_sp<GeometryNode> child, sk_sp<Transform> transform)
    : INHERITED(std::move(child))
    , fTransform(std::move(transform)) {
    SkASSERT(fTransform);
    this->observeInval(fTransform);
}

GeometryTransform::~GeometryTransform() {
    this->unobserveInval(fTransform);
}

SkPath GeometryTransform::onRevalidateEffect(const sk_sp<GeometryNode>& child) {
    fTransform->revalidate(nullptr, SkMatrix::I());
    const auto m = TransformPriv::As<SkMatrix>(fTransform);

    SkPath path = child->asPath();
    path.transform(m);

    return path;
}

namespace  {

sk_sp<SkPathEffect> make_dash(const std::vector<float> intervals, float phase) {
    if (intervals.empty()) {
        return nullptr;
    }

    const auto* intervals_ptr   = intervals.data();
    auto        intervals_count = intervals.size();

    SkSTArray<32, float, true> storage;
    if (intervals_count & 1) {
        intervals_count *= 2;
        storage.resize(intervals_count);
        intervals_ptr = storage.data();

        std::copy(intervals.begin(), intervals.end(), storage.begin());
        std::copy(intervals.begin(), intervals.end(), storage.begin() + intervals.size());
    }

    return SkDashPathEffect::Make(intervals_ptr, SkToInt(intervals_count), phase);
}

} // namespace

SkPath DashEffect::onRevalidateEffect(const sk_sp<GeometryNode>& child) {
    SkPath path = child->asPath();

    if (const auto dash_patheffect = make_dash(fIntervals, fPhase)) {
        SkStrokeRec rec(SkStrokeRec::kHairline_InitStyle);
        dash_patheffect->filterPath(&path, path, &rec, nullptr);
    }

    return path;
}

SkPath RoundEffect::onRevalidateEffect(const sk_sp<GeometryNode>& child) {
    SkPath path = child->asPath();

    if (const auto round = SkCornerPathEffect::Make(fRadius)) {
        SkStrokeRec rec(SkStrokeRec::kHairline_InitStyle);
        SkAssertResult(round->filterPath(&path, path, &rec, nullptr));
    }

    return path;
}

} // namesapce sksg