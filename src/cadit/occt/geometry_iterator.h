//
// Created by Kristoffer on 12.01.2025.
//

#ifndef GEOMETRY_ITERATOR_HPP
#define GEOMETRY_ITERATOR_HPP

#include <stack>
#include <utility>
#include <iterator>
#include "step_tree.h"

class GeometryIterator {
public:
    using value_type = const ProductNode;
    using pointer = const ProductNode*;
    using reference = const ProductNode&;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

    // Constructor: Initialize with root node
    explicit GeometryIterator(pointer root) {
        if (root) {
            stack.emplace(root, 0);
            advanceToNextValid();
        }
    }

    // Default constructor for end iterator
    GeometryIterator() = default;

    // Dereference operator
    reference operator*() const { return *stack.top().first; }

    // Pointer operator
    pointer operator->() const { return stack.top().first; }

    // Pre-increment operator
    GeometryIterator& operator++() {
        stack.top().second++;
        advanceToNextValid();
        return *this;
    }

    // Post-increment operator
    GeometryIterator operator++(int) {
        GeometryIterator temp = *this;
        ++(*this);
        return temp;
    }

    // Equality comparison
    bool operator==(const GeometryIterator& other) const {
        return stack == other.stack;
    }

    // Inequality comparison
    bool operator!=(const GeometryIterator& other) const {
        return !(*this == other);
    }

private:
    std::stack<std::pair<pointer, size_t>> stack;

    // Advance to the next valid node
    void advanceToNextValid() {
        while (!stack.empty()) {
            auto& [currentNode, childIndex] = stack.top();

            if (childIndex == 0 && !currentNode->geometryIndices.empty()) {
                return;
            }

            if (childIndex < currentNode->children.size()) {
                stack.emplace(&currentNode->children[childIndex], 0);
                childIndex++;
            } else {
                stack.pop();
            }
        }
    }
};



// Helper functions for iteration
inline GeometryIterator begin(const ProductNode& root) {
    return GeometryIterator(&root);
}

inline GeometryIterator end(const ProductNode&) {
    return {};
}

class GeometryRange {
public:
    explicit GeometryRange(const std::vector<ProductNode>& roots) {
        for (const auto& root : roots) {
            iterators.emplace_back(GeometryIterator(&root), GeometryIterator());
        }
        updateCurrentIterator();
    }

    GeometryIterator begin() {
        return currentBegin;
    }

    static GeometryIterator end() {
        return {};
    }

private:
    std::vector<std::pair<GeometryIterator, GeometryIterator>> iterators;
    GeometryIterator currentBegin;

    void updateCurrentIterator() {
        for (auto& [it, endIt] : iterators) {
            if (it != endIt) {
                currentBegin = it;
                return;
            }
        }
        currentBegin = GeometryIterator();
    }
};

#endif // GEOMETRY_ITERATOR_HPP