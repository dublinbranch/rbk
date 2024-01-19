#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_NUMBER_DOUBLEOPERATOR_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_NUMBER_DOUBLEOPERATOR_H

namespace RBK {
bool compare(double lhs, double rhs, double ths = 0.0000001);
bool less(double lhs, double rhs, double ths = 0.0000001);
struct Less {
	bool operator()(const double& a, const double& b) const;
};
} // namespace RBK

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_NUMBER_DOUBLEOPERATOR_H
