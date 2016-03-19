#ifndef __UTILS_H__
#define __UTILS_H__

#define	COUNT_OF(x) sizeof((x)) / sizeof((x[0]))

namespace Utils
{
	template<class Type>
	Type Min(Type a, Type b)
	{
		return (a < b ? a : b);
	}

	template<class Type>
	Type Max(Type a, Type b)
	{
		return (a > b ? a : b);
	}

	template<class Type>
	Type Abs(Type a)
	{
		return (a < 0 ? -a : a);
	}

	const unsigned char BITBOARD_PHYS_SIZE = sizeof(unsigned long long) * 8;

	const unsigned char BITBOARD_PHYS_SIZE_OFFSET = 6; // for division with right shifts
	const unsigned char BITBOARD_PHYS_SIZE_MOD_MASK = 0x40 - 1; // for modulo with bit operations
};

template< class Type >
class DynamicArray
{
public:
	DynamicArray(int arrSize = 20) : arr(nullptr), size(arrSize), count(0)
	{
		arr = new Type[size];
	}

	DynamicArray(const DynamicArray<Type>& copy) : arr(nullptr), size(copy.size), count(copy.count)
	{
		arr = new Type[size];
		for(int i = 0; i < count; ++i)
		{
			arr[i] = copy.arr[i];
		}
	}

	DynamicArray(const Type srcArr[], int srcSize) : arr(nullptr), size(srcSize), count(srcSize)
	{
		arr = new Type[srcSize];
		for(int i = 0; i < srcSize; ++i)
		{
			arr[i] = srcArr[i];
		}
	}

	virtual ~DynamicArray()
	{
		delete[] arr;
		arr = nullptr;
	}

	DynamicArray<Type>& operator=(const DynamicArray<Type>& assign)
	{
		if(this != &assign)
		{
			delete[] arr;
			arr = nullptr;
			size = assign.size;
			count = assign.count;
			arr = new Type[size];
			for(int i = 0; i < count; ++i) {
				arr[i] = assign.arr[i];
			}
		}
		return *this;
	}

	DynamicArray<Type>& Append(const DynamicArray<Type>& rhs)
	{
		if(count + rhs.count > size) {
			Realloc(count + rhs.count + 1); // add just one more element
		}

		for(int i = 0; i < rhs.count; ++i) {
			arr[count] = rhs.arr[i];
			++count;
		}
		return *this;
	}

	// adds an item to the end of the array ( rellocates current size * 2 if needed )
	DynamicArray<Type>& operator+=(Type item)
	{
		if(count >= size) {
			Realloc( size * 2);
		}
		arr[count] = item;
		count++;
		return *this;
	}

	// removes an item with given index by replacing it with the last element
	void RemoveItem( int index) {
		count--;
		arr[index] = arr[count];
	}

	// clear all elements ( not physically )
	void Clear()
	{
		count = 0;
	}

	// get the count of elements
	int Count() const { return count; }

	// element accessor
	Type& operator[](int index)
	{
		return arr[index];
	}

	// const element accessor
	const Type& operator[](int index) const
	{
		return arr[index];
	}

	// sorts the array in place using quicksort for the sorting
	// NOTE: the class of the array must have operator< predefined
	void Sort()
	{
		if(count > 1)
		{
			QuickSort(0, count - 1);
		}
	}

	// insertion sorts the array in place
	// NOTE: this should be faster for nearly sorted array than the quicksort
	void InsertionSort()
	{
		for(int i = 1; i < count; ++i)
		{
			Type x = arr[i];
			int j = i;
			while(j > 0 && x < arr[j - 1])
			{
				arr[j] = arr[j - 1];
				--j;
			}
			arr[j] = x;
		}
	}


	// if there isn't 'n' in size currently free, the array allocates them
	void Alloc(int n)
	{
		if(size - count < n)
		{
			Realloc(size + n);
		}
	}

protected:
	void Realloc(int newSize)
	{
		if( newSize > size)
		{
			size = newSize;
			Type* tmp = new Type[size];
			for(int i = 0; i < count; ++i)
			{
				tmp[i] = arr[i];
			}
			delete[] arr;
			arr = nullptr;
			arr = tmp;
		}
	}

	// quicksorts the array in place
	void QuickSort(int left, int right)
	{
		if(left < right)
		{
			int pivotIndex = (left + right) >> 1; // (left + right) / 2
			// get the new pivot index after partitioning
			pivotIndex = Partition(left, right, pivotIndex);

			// recursively quicksort the other two parts
			QuickSort(left, pivotIndex - 1);
			QuickSort(pivotIndex + 1, right);
		}
	}

	// used by the quicksort algorithm for partitioning
	int Partition(int left, int right, int pivotIndex)
	{
		Type pivotValue = arr[pivotIndex];
		arr[pivotIndex] = arr[right];
		arr[right] = pivotValue;
		int storeIndex = left;
		for(int i = left; i < right; ++i)
		{
			if(arr[i] < pivotValue)
			{
				Type tmp = arr[i];
				arr[i] = arr[storeIndex];
				arr[storeIndex] = tmp;
				++storeIndex;
			}
		}
		arr[right] = arr[storeIndex];
		arr[storeIndex] = pivotValue;
		return storeIndex;
	}

	Type* arr;
	int size;
	int count;
};

template<class Type>
class DynamicStack : protected DynamicArray<Type>
{
public:
	DynamicStack(int stackSize = 40) : DynamicArray<Type>(stackSize) {}

	DynamicStack(const DynamicArray<Type>& copy) : DynamicArray<Type>(copy) {}

	DynamicStack(const DynamicStack<Type>& copy) : DynamicArray<Type>(copy) {}

	DynamicStack<Type>& operator=(const DynamicStack<Type>& assing)
	{
		if(this != &assign)
		{
			delete[] arr;
			arr = nullptr;
			size = assign.size;
			count = assign.count;
			arr = new Type[size];
			for(int i = 0; i < count; ++i)
			{
				arr[i] = assign.arr[i];
			}
		}
		return *this;
	}

	void Push(Type element)
	{
		if(count >= size)
		{
			Realloc(size * 2);
		}
		arr[count] = element;
		count++;
	};

	Type Pop()
	{
		if(count)
		{
			count--;
			return arr[count];
		}
		return Type();
	}

	Type& Top()
	{
		if(count)
		{
			return arr[count - 1];
		}
	}

	const Type& Top() const
	{
		if(count)
		{
			return arr[count - 1];
		}
	}

	bool Empty() const
	{
		return count == 0;
	}

	void Clear()
	{
		count = 0;
	}
};

class Rect
{
public:
	int x, y, width, height;

	Rect() : x(0), y(0), width(0), height(0) {}
	Rect(int x, int y) : x(x), y(y), width(0), height(0) {}
	Rect(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
	Rect(const Rect& copy) : x(copy.x), y(copy.y), width(copy.width), height(copy.height) {}
	Rect(const int vec[4]) : x(vec[0]), y(vec[1]), width(vec[2]), height(vec[3]) {}

	inline Rect& operator=(const Rect& assign)
	{
		x = assign.x;
		y = assign.y;
		width = assign.width;
		height = assign.height;
		return *this;
	}

	inline bool Inside(int sx, int sy) const {
		return sx >= x && sx < (width + x) && sy >= y && sy < (height + y);
	}

};

inline bool operator<(const Rect& lhs, const Rect& rhs) {
	return (rhs.x < lhs.x) && (rhs.y < lhs.y) && ((rhs.x + rhs.width) >(lhs.x + lhs.width)) && ((rhs.y + rhs.height) >(lhs.y + lhs.height));
}

inline bool operator<=(const Rect& lhs, const Rect& rhs) {
	return (rhs.x <= lhs.x) && (rhs.y <= lhs.y) && ((rhs.x + rhs.width) >=(lhs.x + lhs.width)) && ((rhs.y + rhs.height) >=(lhs.y + lhs.height));
}

inline bool operator>(const Rect& lhs, const Rect& rhs) {
	return rhs < lhs;
}

inline bool operator>=(const Rect& lhs, const Rect& rhs) {
	return rhs <= lhs;
}

#endif // __UTILS_H__