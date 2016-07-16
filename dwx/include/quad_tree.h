#ifndef __QUAD_TREE_H__
#define __QUAD_TREE_H__

#include "vector2.h"
#include "drect.h"

#include <vector>
#include <memory>

enum QuadTreeOffsets {
	QTO_NONE = 0,
	QTO_NEG_X = 1 << 0, //!< negative x offset
	QTO_NEG_Y = 1 << 1, //!< negative y offset
	QTO_COUNT = QTO_NEG_X + QTO_NEG_Y + 1,
};

template<class T>
class QuadTree {
public:
	// a single element in the quad tree defined by a position and data
	struct QuadTreeElement {
		QuadTreeElement(Vector2 _position = Vector2(), const T& _element = T())
			: position(_position)
			, element(_element)
		{}
		QuadTreeElement(const QuadTreeElement&) = default;
		QuadTreeElement& operator=(const QuadTreeElement&) = default;

		Vector2 position;
		T element;
	};

	QuadTree(int _level, float _halfDimension, Vector2 _c = Vector2(0.0f, 0.0f))
		: level(_level)
		, halfDimension(_halfDimension)
		, c(_c)
		, subtrees{ nullptr }
	{}

	~QuadTree() {
		for (int i = 0; i < QuadTreeOffsets::QTO_COUNT; ++i) {
			delete subtrees[i];
			subtrees[i] = nullptr;
		}
	}

	QuadTree(const QuadTree& copy)
		: level(copy.level)
		, halfDimension(copy.halfDimension)
		, c(copy.c)
		, subtrees{ nullptr }
	{
		copyTree(copy);
	}

	QuadTree& operator=(const QuadTree& assign) {
		if (this != &assign) {
			copyTree(assign);
		}
		return *this;
	}

	// returns the number of levels required to have the bottom level dimension as specified
	// provided the dimension of the top level
	static int getLevels(float minDimension, float dimension) {
		return static_cast<int>(std::ceil(log2f(dimension / minDimension)));
	}

	// returns the bounding box of the quad tree
	inline Rect getBoundingBox() const noexcept {
		return Rect(
			c.x - halfDimension,
			c.y - halfDimension,
			halfDimension * 2.0f,
			halfDimension * 2.0f
		);
	}

	inline void addElement(const Vector2& pos, const T& element) {
		if (0 == level) {
			if (!elements) {
				elements.reset(new std::vector<QuadTreeElement>());
			}
			elements->push_back(QuadTreeElement(pos, element));
		} else {
			QuadTree * subtree = getSubtree(pos, true);
			subtree->addElement(pos, element);
		}
	}

	// returns the count of elements in the provided bounding box that are stored in the quad tree
	inline int getElementCount(const Rect& bbox = getBoundingBox()) const noexcept {
		int count = 0;
		if (0 == level) {
			// if on the bottom level of the tree - iterate the elemnts
			if (elements) {
				for (const auto & e : *elements) {
					count += (bbox.inside(e.position) ? 1 : 0);
				}
			}
		} else {
			// otherwise check subtrees, but only if they intersect the bbox
			for (int i = 0; i < QuadTreeOffsets::QTO_COUNT; ++i) {
				const QuadTree * subt = subtrees[i];
				if (nullptr != subt && bbox.intersects(subt->getBoundingBox())) {
					count += subt->getElementCount(bbox);
				}
			}
		}
		return count;
	}

	// returns all the elements that are contained in the provided bounding box
	inline void getElements(std::vector<QuadTreeElement>& output, const Rect& bbox) const noexcept {
		if (0 == level) {
			// get all the elemnts that are inside the bounding box
			if (elements) {
				for (const auto & e : *elements) {
					if (bbox.inside(e.position)) {
						output.push_back(e);
					}
				}
			}
		} else {
			// otherwise recursively gather the elements from the subtrees if their bboxes intersect the provided
			for (int i = 0; i < QuadTreeOffsets::QTO_COUNT; ++i) {
				const QuadTree * subt = subtrees[i];
				if (nullptr != subt && bbox.intersects(subt->getBoundingBox())) {
					subt->getElements(output, bbox);
				}
			}
		}
	}

	// returns true if the quad tree contains elements where this point would be placed
	inline bool containsElements(const Vector2& p) const noexcept {
		if (0 == level) {
			return (elements && 0 < elements->size());
		} else {
			const QuadTree * subt = getSubtree(p, false);
			return (nullptr != subt && subt->containsElements(p));
		}
	}

	// returns vector of all quad tree pointers that contain at least one element
	inline void getBottomTrees(std::vector<const QuadTree*>& output) const noexcept {
		if (0 == level) {
			// if this tree was created, it must contain an element or is the top tree
			output.push_back(this);
		} else {
			// iterate over all subtrees
			for (int i = 0; i < QuadTreeOffsets::QTO_COUNT; ++i) {
				const QuadTree * subt = subtrees[i];
				if (subt) {
					subt->getBottomTrees(output);
				}
			}
		}
	}

	inline const std::vector<QuadTreeElement> * getTreeElements() const noexcept {
		return elements.get();
	}

	inline Vector2 getTreeCenter() const noexcept {
		return c;
	}

private:
	inline QuadTree * getSubtree(const Vector2& p, bool create) {
		const int subtreeIndex =
			(p.x < c.x ? QuadTreeOffsets::QTO_NEG_X : 0) +
			(p.y < c.y ? QuadTreeOffsets::QTO_NEG_Y : 0);
		if (nullptr == subtrees[subtreeIndex] && create) {
			// create the subtree
			const float subHalfDim = halfDimension * 0.5f;
			const Vector2 subCenter(
				c.x + (p.x < c.x ? -subHalfDim : subHalfDim),
				c.y + (p.y < c.y ? -subHalfDim : subHalfDim)
			);
			subtrees[subtreeIndex] = new QuadTree(level - 1, subHalfDim, subCenter);
		}
		return subtrees[subtreeIndex];
	}

	inline void copyTree(const QuadTree& cpy) {
		c = cpy.c;
		halfDimension = cpy.halfDimension;
		level = cpy.level;
		for (int i = 0; i < QuadTreeOffsets::QTO_COUNT; ++i) {
			if (nullptr != subtrees[i]) {
				delete subtrees[i];
				subtrees[i] = nullptr;
			}
			if (nullptr != cpy.subtrees[i]) {
				const QuadTree& subt = cpy.subtrees[i];
				subtrees[i] = new QuadTree(subt.level, subt.halfDimension, subt.c);
				subtrees[i]->copyTree(subt);
			} else {
				subtrees[i] = nullptr;
			}
		}
		if (cpy.elements) {
			elements.reset(new std::vector<QuadTreeElement>(*(cpy.elements)));
		}
	}

	Vector2 c; //!< the center of this quad tree
	float halfDimension; //!< the half dimension of the quad tree (it has width and height of 2 * halfDimension)
	int level; //!< the level of depth upwards from the current tree relative to its subtrees
	QuadTree * subtrees[QuadTreeOffsets::QTO_COUNT]; //!< the subtrees of this tree (may be nullptr)

	std::unique_ptr<std::vector<QuadTreeElement> > elements; //!< the elements the subtree saves on its 0 level
};

#endif
