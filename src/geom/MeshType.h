//
// Created by Kristoffer on 07/05/2023.
//

#ifndef NANO_OCCT_MESHTYPE_H
#define NANO_OCCT_MESHTYPE_H

enum class MeshType {
    POINTS = 0,
    LINES = 1,
    LINE_LOOP = 2,
    LINE_STRIP = 3,
    TRIANGLES = 4,
    TRIANGLE_STRIP = 5,
    TRIANGLE_FAN = 6
};

MeshType from_int(int value);

#endif //NANO_OCCT_MESHTYPE_H
