#ifndef CONTROLFLOWMACRO_H
#define CONTROLFLOWMACRO_H

#define BREAK_IF_TRUE(Func) \
	if (Func) {         \
		break;      \
	}

#define RETURN_IF_TRUE(func) \
	if (func) {          \
		return;      \
	}

#endif // CONTROLFLOWMACRO_H
