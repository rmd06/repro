#include "tpl_inst.h"

template<>
struct tpl_traits<int> {
    static const char* name() {
        return "custom trait";
    }
};

template<typename T>
void tpl_func(const T& x) {
    std::cout << "template: " << tpl_traits<T>::name() << std::endl;
}

template void tpl_func<int>(const int& x);
template void tpl_func<double>(const double& x);


template<typename T>
void test::tpl_method(const T& x) {
    std::cout << "method template: " << tpl_traits<T>::name() << std::endl;
}
template void test::tpl_method<int>(const int& x);


template<typename ... Args>
void tpl_func_var(Args ... args) {
    std::cout << "variadic" << std::endl;
}
template void tpl_func_var(int x, double y);