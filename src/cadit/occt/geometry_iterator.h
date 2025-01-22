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

    // Constructor: Initialize with a pointer to the root node
    explicit GeometryIterator(pointer root) {
        if (root) {
            stack.emplace(root, 0);
            advanceToNextValid();
        }
    }

    // Default constructor for end iterator
    GeometryIterator() = default;

    // Dereference operator
    reference operator*() const { 
        return *stack.top().first; 
    }

    // Arrow operator
    pointer operator->() const { 
        return stack.top().first; 
    }

    // Pre-increment
    GeometryIterator& operator++() {
        stack.top().second++;
        advanceToNextValid();
        return *this;
    }

    // Post-increment
    GeometryIterator operator++(int) {
        GeometryIterator temp = *this;
        ++(*this);
        return temp;
    }

    // Equality
    bool operator==(const GeometryIterator& other) const {
        return stack == other.stack;
    }

    // Inequality
    bool operator!=(const GeometryIterator& other) const {
        return !(*this == other);
    }

private:
    // Each stack entry: (currentNode, currentChildIndex)
    std::stack<std::pair<pointer, size_t>> stack;

    // Advance to the next node that has geometryIndices, or run out
    void advanceToNextValid() {
        while (!stack.empty()) {
            auto& [currentNode, childIndex] = stack.top();

            // 1) If this is the first time we see this node (childIndex == 0)
            //    and it has geometry, we stop here so operator* sees it.
            if (childIndex == 0 && !currentNode->geometryInstances.empty()) {
                return; 
            }

            // 2) Otherwise, if we still have children to explore
            if (childIndex < currentNode->children.size()) {
                // Get the raw pointer from the unique_ptr
                auto* childPtr = currentNode->children[childIndex].get();
                stack.emplace(childPtr, 0);
                childIndex++;
            }
            else {
                // No more children -> pop
                stack.pop();
            }
        }
    }
};

// Helper functions for iterating a single ProductNode
inline GeometryIterator begin(const ProductNode& root) {
    return GeometryIterator(&root);
}

inline GeometryIterator end(const ProductNode&) {
    return {};
}

class GeometryRange {
public:
    // Suppose we have a vector of unique_ptr<ProductNode> as roots
    explicit GeometryRange(const std::vector<std::unique_ptr<ProductNode>>& roots) {
        for (auto const& rootPtr : roots) {
            // rootPtr is a std::unique_ptr<ProductNode>, so we pass rootPtr.get()
            iterators.emplace_back(GeometryIterator(rootPtr.get()), GeometryIterator());
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
        // Find the first non-empty iterator
        for (auto& [it, endIt] : iterators) {
            if (it != endIt) {
                currentBegin = it;
                return;
            }
        }
        // If all are empty, we produce an end iterator
        currentBegin = GeometryIterator();
    }
};

#endif // GEOMETRY_ITERATOR_HPP
