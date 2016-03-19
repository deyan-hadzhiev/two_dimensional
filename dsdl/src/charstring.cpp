#include <cstdio>
#include <string.h>
#include "charstring.h"
#include "utils.h"

CharString::CharString(const char * src)
	:
	data(nullptr),
	size(0)
{
	if(src)
	{
		int n = (int) strlen(src) + 1;
		if(n > 1)
		{
			data = new char[n];
			if(data)
			{
				strncpy(data, src, n);
				size = n;
			}
		}
	}
	else
	{
		size = 1;
		data = new char[size];
		data[0] = '\0';
	}

}

CharString::CharString(const CharString& copy)
	:
	data(nullptr),
	size(copy.size)
{
	if(size && copy.data)
	{
		data = new char[size];
		if(data)
		{
			strncpy(data, copy.data, size);
		}
	}
}

CharString::CharString(int number)
	:
	data(nullptr),
	size(0)
{
	char numberStr[MAX_INTEGER_STRING_LENGTH] = "";
	sprintf(numberStr, "%d", number);
	size = (int) strnlen(numberStr, COUNT_OF(numberStr)) + 1;
	if(size)
	{
		data = new char[size];
		if(data)
		{
			strncpy(data, numberStr, size);
		}
	}
}

CharString::~CharString()
{
	delete[] data;
	data = nullptr;
	size = 0;
}

CharString& CharString::operator=(const CharString& assign)
{
	if(this != &assign)
	{
		delete[] data;
		data = nullptr;
		size = assign.size;
		data = new char[size];
		if(data)
		{
			strncpy(data, assign.data, size);
		}
	}
	return *this;
}

const CharString& CharString::operator+=(const CharString& rhs)
{
	int totalLength = size + rhs.size - 1;
	char * tmp = new char[totalLength];
	if(tmp)
	{
		strncpy(tmp, data, size);
		strncat(tmp, rhs.data, totalLength - size + 1);

		delete[] data;
		data = nullptr;
		data = tmp;
		size = totalLength;
	}
	return *this;
}

const CharString operator+(const CharString& lhs, const CharString& rhs)
{
	CharString retval;
	delete[] retval.data;
	retval.size = lhs.size + rhs.size - 1;
	retval.data = new char[retval.size];
	if(retval.data)
	{
		strncpy(retval.data, lhs.data, retval.size);
		strncat(retval.data, rhs.data, retval.size - lhs.size + 1);
	}
	return retval;
}

const char& CharString::operator[](int index) const
{
	return data[index];
}

char& CharString::operator[](int index)
{
	return data[index];
}

int CharString::Length() const
{
	return size - 1;
}

CharString::operator const char * () const
{
	return data;
}

CharString::operator char * ()
{
	return data;
}

const char* CharString::GetPtr() const
{
	return data;
}

char* CharString::GetPtr()
{
	return data;
}