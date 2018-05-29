
/*
 * Copyright 2008 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


//#include "SkMathPriv.h"
#include "SkPoint.h"


SkScalar SkPoint::Length(SkScalar dx, SkScalar dy) {
    float mag2 = dx * dx + dy * dy;
    if (SkScalarIsFinite(mag2)) {
        return sk_float_sqrt(mag2);
    } else {
        double xx = dx;
        double yy = dy;
        return (float)sqrt(xx * xx + yy * yy);
    }
}

