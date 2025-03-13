#ifndef OPTIONALV2_H
#define OPTIONALV2_H

#include <optional>

template <typename T>
class OptionalV2 : private std::optional<T> {
      public:
	// Inherit constructors from std::optional<T>
	using std::optional<T>::optional;

	// Expose member functions you want to support
	using std::optional<T>::operator*;
	using std::optional<T>::operator->;
	using std::optional<T>::reset;
	using std::optional<T>::emplace;
	using std::optional<T>::value;

	// Provide a named function for checking presence.
	[[nodiscard]] bool has_value() const noexcept {
		return std::optional<T>::has_value();
	}

	// Do not expose operator bool, effectively suppressing it.
};

#endif // OPTIONALV2_H
