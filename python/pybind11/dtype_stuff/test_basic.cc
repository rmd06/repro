#include <cmath>

#include <iostream>

using std::pow;
using std::cout;
using std::endl;

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/operators.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include <numpy/ufuncobject.h>
#include <numpy/ndarraytypes.h>

#include "ufunc_utility.h"

namespace py = pybind11;

class Custom {
public:
    using Self = Custom;

    Custom() {}
    Custom(double value) : value_{value} {}
    Custom(const Custom&) = default;
    Custom& operator=(const Custom&) = default;
    double value() const { return value_; }

    operator double() const { return value_; }

    Self operator==(const Self& rhs) const { return value_ + rhs.value_; }
    Self operator<(const Self& rhs) const { return value_ * 10 * rhs.value_; }
    Custom operator*(const Custom& rhs) const {
        return value_ * rhs.value_;
    }
    Custom operator-() const { return -value_; }

private:
    double value_{};
};

Custom pow(Custom a, Custom b) {
  return pow(a.value(), b.value());
}

// Could increase param count?
#define CHECK_EXPR(struct_name, name, expr, lambda_expr) \
    template <typename A_, typename B = A_> \
    struct struct_name { \
      template <typename A = A_> \
      static std::true_type check(decltype(expr)*); \
      template <typename> \
      static std::false_type check(...); \
      static constexpr bool value = decltype(check<A_>(nullptr))::value; \
      template <typename A = A_> \
      static auto get_lambda() { return lambda_expr; } \
      static const char* get_name() { return name; } \
    };

// https://docs.scipy.org/doc/numpy/reference/routines.math.html
CHECK_EXPR(check_add, "add", A{} + B{},
           [](const A& a, const B& b) { return a + b; });
CHECK_EXPR(check_negative, "negative", -A{},
           [](const A& a) { return -a; });
CHECK_EXPR(check_multiply, "multiply", A{} * B{},
           [](const A& a, const B& b) { return a * b; });
CHECK_EXPR(check_divide, "divide", A{} / B{},
           [](const A& a, const B& b) { return a / b; });
// TODO(eric.cousineau): Figger out non-operator things...
CHECK_EXPR(check_power, "power", pow(A{}, B{}),
           [](const A& a, const B& b) { return pow(a, b); });
CHECK_EXPR(check_subtract, "subtract", A{} - B{},
           [](const A& a, const B& b) { return a - b; });
// https://docs.scipy.org/doc/numpy/reference/routines.logic.html
CHECK_EXPR(check_greater, "greater", A{} > B{},
           [](const A& a, const B& b) { return a > b; });
CHECK_EXPR(check_greater_equal, "greater_equal", A{} >= B{},
           [](const A& a, const B& b) { return a >= b; });
CHECK_EXPR(check_less, "less", A{} < B{},
           [](const A& a, const B& b) { return a < b; });
CHECK_EXPR(check_less_equal, "less_equal", A{} <= B{},
           [](const A& a, const B& b) { return a <= b; });
CHECK_EXPR(check_equal, "equal", A{} == B{},
           [](const A& a, const B& b) { return a == b; });
CHECK_EXPR(check_not_equal, "not_equal", A{} != B{},
           [](const A& a, const B& b) { return a != b; });
// Casting?
CHECK_EXPR(check_double, "er", double{A{}},
           [](const A& a) { return double{a}; });

template <typename Result, typename Func>
void run_if(Func&& func) {
  using Pack = type_pack<Result>;
  type_visit_impl<visit_with_default, Func>::
      template runner<Pack, Result::value>::run(func);
}

template <template <typename...> class Check, typename Class, typename ... Args>
auto maybe_ufunc(type_pack<Args...> = {}) {
  using Result = typename type_pack<Args...>::template bind<Check>;
  constexpr int N = sizeof...(Args);
  auto defer = [](auto) {
    py::module numpy = py::module::import("numpy");
    auto ufunc = (PyUFuncObject*)numpy.attr(Result::get_name()).ptr();
    RegisterUFunc<Class>(ufunc, Result::get_lambda(), const_int<N>{});
  };
  run_if<Result>(defer);
}

void module(py::module m) {}

int npy_rational{-1};

namespace pybind11 { namespace detail {

template <>
struct npy_format_descriptor<Custom> {
    static pybind11::dtype dtype() {
        if (auto ptr = npy_api::get().PyArray_DescrFromType_(npy_rational))
            return reinterpret_borrow<pybind11::dtype>(ptr);
        pybind11_fail("Unsupported buffer format!");
    }
};

} }  // namespace detail } namespace pybind11

int main() {
    py::scoped_interpreter guard;

    py::module m("__main__");

    using Class = Custom;
    py::class_<Class> cls(m, "Custom");
    cls
        .def(py::init<double>())
        .def(py::self == Class{})
        .def(py::self * Class{})
        .def("value", &Class::value)
        .def("__repr__", [](const Class* self) {
            return py::str("Custom({})").format(self->value());
        });

    // Register thing.

    _import_array();
    _import_umath();

    py::module numpy = py::module::import("numpy");
    auto py_type = (PyTypeObject*)cls.ptr();

    typedef struct { char c; Class r; } align_test;

    static PyArray_ArrFuncs npyrational_arrfuncs;
    
    static PyArray_Descr npyrational_descr = {
        PyObject_HEAD_INIT(0)
        py_type,                /* typeobj */
        'V',                    /* kind (V = arbitrary) */
        'r',                    /* type */
        '=',                    /* byteorder */
        /*
         * For now, we need NPY_NEEDS_PYAPI in order to make numpy detect our
         * exceptions.  This isn't technically necessary,
         * since we're careful about thread safety, and hopefully future
         * versions of numpy will recognize that.
         */
        NPY_NEEDS_PYAPI | NPY_USE_GETITEM | NPY_USE_SETITEM, /* flags */
        0,                      /* type_num */
        sizeof(Class),       /* elsize */
        offsetof(align_test,r), /* alignment */
        0,                      /* subarray */
        0,                      /* fields */
        0,                      /* names */
        &npyrational_arrfuncs,  /* f */
    };

    PyArray_InitArrFuncs(&npyrational_arrfuncs);
    npyrational_arrfuncs.getitem = [](void* in, void* arr) -> PyObject* {
        return py::cast(*(const Class*)in).release().ptr();
    };
    npyrational_arrfuncs.setitem = [](PyObject* in, void* out, void* arr) {
        *(Class*)out = *py::handle(in).cast<Class*>();
        return 0;
    };
    npyrational_arrfuncs.copyswap = [](void* dst, void* src, int swap, void* arr) {
        if (!src) return;
        Class* r_dst = (Class*)dst;
        Class* r_src = (Class*)src;
        if (swap) {
            std::swap(*r_dst, *r_src);
        } else {
            *r_dst = *r_src;
        }
    };
    Py_TYPE(&npyrational_descr) = &PyArrayDescr_Type;
    npy_rational = PyArray_RegisterDataType(&npyrational_descr);
    cls.attr("dtype") = py::reinterpret_borrow<py::object>(
        py::handle((PyObject*)&npyrational_descr));

    using Unary = type_pack<Class>;
    using Binary = type_pack<Class, Class>;
    // Arithmetic.
    maybe_ufunc<check_add, Class>(Binary{});
    maybe_ufunc<check_negative, Class>(Unary{});
    maybe_ufunc<check_multiply, Class>(Binary{});
    maybe_ufunc<check_divide, Class>(Binary{});
    maybe_ufunc<check_power, Class>(Binary{});
    maybe_ufunc<check_subtract, Class>(Binary{});
    // Comparison.
    maybe_ufunc<check_greater, Class>(Binary{});
    maybe_ufunc<check_greater_equal, Class>(Binary{});
    maybe_ufunc<check_less, Class>(Binary{});
    maybe_ufunc<check_less_equal, Class>(Binary{});
    maybe_ufunc<check_equal, Class>(Binary{});
    maybe_ufunc<check_not_equal, Class>(Binary{});

    // TODO(eric.cousineau): Casting...

    py::str file = "python/pybind11/dtype_stuff/test_basic.py";
    py::print(file);
    py::eval_file(file);

    py::exec(R"""(
x = np.array([Custom(1)])
)""");

    return 0;
}
