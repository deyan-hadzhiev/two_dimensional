#ifndef __CHARSTRING_H__
#define __CHARSTRING_H__

static const int MAX_INTEGER_STRING_LENGTH = 16;

class CharString
{
public:
	CharString(const char * src = nullptr);
	CharString(const CharString& copy);
	CharString(int number);
	~CharString();

	CharString& operator=(const CharString& assign);

	const CharString& operator+=(const CharString& rhs);

	friend const CharString operator+(const CharString& lhs, const CharString& rhs);

	// element accessor (const)
	const char& operator[](int index) const;
	// element accessor
	char& operator[](int index);

	int Length() const;

	operator const char* () const;
	operator char* ();
	const char* GetPtr() const;
	char* GetPtr();

private:
	char * data;
	int size;
};

#endif // __CHARSTRING_H__