#include <iostream>
#include <string>
#include <stdexcept>

using std::cout;
using std::endl;
using std::string;

// https://stackoverflow.com/questions/15011674/is-it-possible-to-dereference-variable-ids
// Can wrap any pythonic types...

#include <mex.h>
#include <matrix.h>

#include "func.h"

extern "C" {

void* c_void_p_pass_thru(void* in) {
    // Leverage Python's ctypes marhsalling of py_object is a (hopefully)
    // robust mechanism to pass stuff around.
    return in;
}

void* c_mx_feval_py_raw(void* mx_raw_handle, int nout, void* py_raw_in) {
    mxArray* mx_handle = static_cast<mxArray*>(mx_raw_handle);
    mxArray* mx_nout = mxCreateNumericMatrix(1, 1, nout);
    mxArray* mx_py_raw_in = mxCreateNumericMatrix(1, 1, py_raw_in);

    const int nrhs = 3;
    mxArray* mx_in[nrhs] = {mx_raw_handle, mx_nout, mx_py_raw_in};
    const int nlhs = 1;
    mxArray* mx_out[nlhs] = {NULL};
    mexCallMATLAB("mx_feval_py_raw", ...)
    void* py_raw_out = *static_cast<void**>(mxGetData(mx_out[0]));

    mxFree(mx_nout);
    mxFree(mx_out);

    return py_raw_out;
}

}  // extern "C"

typedef mwSize mx_uint64;

mxArray* mxCreateUint64(mx_uint64** ppvalue) {
  mxArray* mx_out = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
  *ppvalue = static_cast<mx_uint64*>(mxGetData(mx_out));
}

string mxToStdString(mxArray* mx_in) {
  // From: mxmalloc.c
  const int len = mxGetN(mx_in) + 1;
  char* buffer = mxMalloc(len);
  mxGetString(mx_in, buffer, (mwSize)len);
  string out = buffer;
  mxFree(buffer);
  return out;
}

#define ex_assert(cond, msg_expr) \
  if (!(cond)) { \
    ostringstream os; \
    os << #cond << endl; \
    os << msg_expr; \
    throw std::runtime_error(os.str()); \
  }

const string usage =
    "[varargout{:}] = mex_py_proxy(op, varargin)\n"
    "  op\n" \
    "    'mx_to_mx_raw' Convert mxArray* to uint64 to be passed opaquely.\n" \
    "        [mx_raw] = mex_py_proxy('mx_to_mx_raw', mx)\n" \
    "    'mx_raw_to_mx' Unpack opaque value.\n" \
    "        [mx] = mex_py_proxy('mx_raw_to_mx', mx_raw)\n" \
    "    'get_c_func_ptrs' Get C pointers, using opaque types, to pass to Python.\n" \
    "        [c_func_ptrs_struct] = mex_py_proxy('get_c_func_ptrs')\n" \
    "    'help' Show usage.";

// Create MATLAB struct containing raw values pointing to functions.
mxArray* get_c_func_ptrs() {
  // Reference example: mxcreatestructarray.c
  const int n = 3;
  const char* names[n] = {
    "c_py_to_py_raw",
    "c_py_raw_to_py",
    "c_mx_feval_py_raw",
  };
  // TODO: Will this work???
  void* ptrs[n] = {
    &c_void_p_pass_thru,
    &c_void_p_pass_thru,
    &c_mx_feval_py_raw
  };
  mwSize dims[2] = {1, 1};
  mxArray* s = mxCreateStructArray(2, dims, n, names);
  for (int i = 0; i < n; ++i) {
    const char* name = names[i];
    void* ptr = ptrs[i];
    int field_index = mxGetFieldNumber(s, name);
    mx_int64* pvalue;
    mxArray* value = mxCreateUint64(&pvalue);
    *pvalue = ptr;
    mxSetFieldByNumber(s, 0, field_index, value);
  }
  return s;
}

// Wrap MEX function call.
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]) {
  try {
    ex_assert(nrhs >= 1, usage);

    string op = mxToStdString(prhs[0]);
    if (op == "mx_to_mx_raw") {
      // Return a uint64 representing point to mxArray* item.
      ex_assert(nrhs == 2, usage);
      ex_assert(nlhs == 1, usage);
      mxArray* mx = prhs[1];
      mx_uint64* pmx_raw;
      plhs[0] = mxCreateUint64(&pmx_raw);
      *pmx_raw = static_cast<mx_uint64>(mx);
    } else if (op == "mx_raw_to_mx") {
      // Opposite direction.
      ex_assert(nrhs == 2, usage);
      ex_assert(nlhs == 1, usage);
      mxArray* mx_mx_raw = prhs[1];
      mx_uint64 mx_raw = *static_cast<mx_uint64*>(mxGetData(mx_mx_raw));
      plhs[0] = static_cast<mxArray*>(mx_raw);
    } else if (op == "get_c_func_ptrs") {
      ex_assert(nrhs == 1, usage);
      ex_assert(nlhs == 1, usage);
      // Using example: 
      plhs[0] = get_c_func_ptrs();
    } else if (op == "help") {
      mexPrintf("%s\n", usage.c_str());
    }
  }
  catch (const std::exception& e) {
    mexErrMsgTxt(e.what().c_str());
  }
}
