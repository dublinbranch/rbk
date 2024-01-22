#ifndef ROWSWAPPABLE_H
#define ROWSWAPPABLE_H

//add in your type this, no need to expose in the describe
//Boostjson use a different approach, but I do not know very well it yet
//RowSwappableType rowSwappable;

struct RowSwappableType {};

template<typename T>
concept isRowSwappableType = requires { T::rowSwappable; };

#endif // ROWSWAPPABLE_H
