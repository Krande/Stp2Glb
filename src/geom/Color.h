//
// Created by Kristoffer on 07/05/2023.
//

#ifndef NANO_OCCT_COLOR_H
#define NANO_OCCT_COLOR_H

class Color {
public:
    float r;
    float g;
    float b;
    float a;

    explicit Color(float r=0.5f, float g=0.5f, float b=0.5f, float a = 1.0f);
};

#endif //NANO_OCCT_COLOR_H
