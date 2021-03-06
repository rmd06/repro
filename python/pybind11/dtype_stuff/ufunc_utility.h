#pragma once

// taken from: numpy/core/src/umath/test_rational.c.src
// see ufuncobject.h


// Goal: Define functions that need no capture...


#include <array>
#include <string>

#include <fmt/format.h>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "cpp/wrap_function.h"

namespace py = pybind11;
using py::detail::npy_format_descriptor;

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ufuncobject.h>

#define PY_ASSERT_EX(expr, ...) \
    if (!(expr)) throw py::cast_error(fmt::format(__VA_ARGS__));

template <typename T>
void* heapit(const T& x) { return new T(x); }
void* heapit(std::nullptr_t) { return nullptr; }

template <typename Type, typename ... Args>
void RegisterUFunc(
        PyUFuncObject* py_ufunc,
        PyUFuncGenericFunction func,
        void* data) {
    constexpr int N = sizeof...(Args);
    int dtype = npy_format_descriptor<Type>::dtype().num();
    int dtype_args[] = {npy_format_descriptor<Args>::dtype().num()...};
    PY_ASSERT_EX(
        N == py_ufunc->nargs, "Argument mismatch, {} != {}",
        N, py_ufunc->nargs);
    PY_ASSERT_EX(
        PyUFunc_RegisterLoopForType(
            py_ufunc, dtype, func, dtype_args, data) >= 0,
        "Failed to regstiser ufunc");
}

template <typename Return, typename ... Args>
using Func = Return (*)(Args...);

template <int N>
using const_int = std::integral_constant<int, N>;

// Unary.
template <typename Type, int N = 1, typename Func = void>
void RegisterUFunc(PyUFuncObject* py_ufunc, Func func, const_int<1>) {
    auto info = detail::infer_function_info(func);
    using Info = decltype(info);
    using Arg0 = std::decay_t<typename Info::Args::template type_at<0>>;
    using Out = std::decay_t<typename Info::Return>;
    auto ufunc = [](
            char** args, npy_intp* dimensions, npy_intp* steps, void* data) {
        Func& func = *(Func*)data;
        int step_0 = steps[0];
        int step_out = steps[1];
        int n = *dimensions;
        char *in_0 = args[0], *out = args[1];
        for (int k = 0; k < n; k++) {
            // TODO(eric.cousineau): Support pointers being changed.
            *(Out*)out = func(*(Arg0*)in_0);
            in_0 += step_0;
            out += step_out;
        }
    };
    RegisterUFunc<Type, Arg0, Out>(py_ufunc, ufunc, new Func(func));
};

// Binary.
template <typename Type, int N = 2, typename Func = void>
void RegisterUFunc(PyUFuncObject* py_ufunc, Func func, const_int<2>) {
    auto info = detail::infer_function_info(func);
    using Info = decltype(info);
    using Arg0 = std::decay_t<typename Info::Args::template type_at<0>>;
    using Arg1 = std::decay_t<typename Info::Args::template type_at<1>>;
    using Out = std::decay_t<typename Info::Return>;
    auto ufunc = [](char** args, npy_intp* dimensions, npy_intp* steps, void* data) {
        Func& func = *(Func*)data;
        int step_0 = steps[0];
        int step_1 = steps[1];
        int step_out = steps[2];
        int n = *dimensions;
        char *in_0 = args[0], *in_1 = args[1], *out = args[2];
        for (int k = 0; k < n; k++) {
            // TODO(eric.cousineau): Support pointers being fed in.
            *(Out*)out = func(*(Arg0*)in_0, *(Arg1*)in_1);
            in_0 += step_0;
            in_1 += step_1;
            out += step_out;
        }
    };
    RegisterUFunc<Type, Arg0, Arg1, Out>(py_ufunc, ufunc, new Func(func));
};
