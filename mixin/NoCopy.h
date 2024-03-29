#pragma once

class NoCopy {
	// Note: Scott Meyers mentions in his Effective Modern
	//C++ book, that deleted functions should generally
	//be public as it results in better error messages
	//due to the compilers behavior to check accessibility
	//before deleted status
      public:
	NoCopy()  = default;
	~NoCopy() = default;

	//copy creator ie Miao m(oldValue)
	NoCopy(const NoCopy&) = delete;
	
	//https://en.cppreference.com/w/cpp/language/move_assignment
	//Miao m = oldValue
	NoCopy& operator=(const NoCopy&) = delete;
};
