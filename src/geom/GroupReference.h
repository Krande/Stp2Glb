//
// Created by Kristoffer on 07/05/2023.
//

#ifndef NANO_OCCT_GROUPREFERENCE_H
#define NANO_OCCT_GROUPREFERENCE_H

class GroupReference {
public:
    int node_id;
    int start;
    int length;

    GroupReference(int node_id, int start, int length);
};

#endif //NANO_OCCT_GROUPREFERENCE_H
