#ifndef CXALEVEL_H
#define CXALEVEL_H

enum class CxaLevel {
	// TODO in theory I should tweak ABORT, and always force print here the stacktrace generated inside __cxa
	// lot of food for thought
	none, // this will NOT forcefully print the stack on throw
	warn,
	debug,
	critical
};

#endif // CXALEVEL_H
