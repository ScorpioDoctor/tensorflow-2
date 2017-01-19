
#include "libpython.hpp"

#ifndef _WIN32
#include <dlfcn.h>
#endif

#include <string>
#include <iostream>

namespace {

void lastDLErrorMessage(std::string* pError)
{
#ifdef _WIN32
  LPVOID lpMsgBuf;
  DWORD dw = ::GetLastError();

  DWORD length = ::FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL );

  if (length != 0)
  {
    std::string msg((LPTSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
    pError->assign(msg);
  }
  else
  {
    pError->assign("(Unknown error)");
  }
#else
  const char* msg = ::dlerror();
  if (msg != NULL)
    pError->assign(msg);
  else
    pError->assign("(Unknown error)");
#endif
}

bool loadLibrary(const std::string& libPath, void** ppLib, std::string* pError)
{
  *ppLib = NULL;
#ifdef _WIN32
  *ppLib = (void*)::LoadLibraryEx(libPath.c_str(), NULL, 0);
#else
  *ppLib = ::dlopen(libPath.c_str(), RTLD_NOW|RTLD_GLOBAL);
#endif
  if (*ppLib == NULL)
  {
    lastDLErrorMessage(pError);
    *pError = libPath + " - " + *pError;
    return false;
  }
  else
  {
    return true;
  }
}

bool loadSymbol(void* pLib, const std::string& name, void** ppSymbol, std::string* pError)
{
  *ppSymbol = NULL;
#ifdef _WIN32
  *ppSymbol = (void*)::GetProcAddress((HINSTANCE)pLib, name.c_str());
#else
  *ppSymbol = ::dlsym(pLib, name.c_str());
#endif
  if (*ppSymbol == NULL)
  {
    lastDLErrorMessage(pError);
    *pError = name + " - " + *pError;
    return false;
  }
  else
  {
    return true;
  }
}

bool closeLibrary(void* pLib, std::string* pError)
{
#ifdef _WIN32
  if (!::FreeLibrary((HMODULE)pLib))
#else
  if (::dlclose(pLib) != 0)
#endif
  {
    lastDLErrorMessage(pError);
    return false;
  }
  else
  {
    return true;
  }
}

} // anonymous namespace

void (*_Py_Initialize)();

void (*_Py_IncRef)(PyObject *);
void (*_Py_DecRef)(PyObject *);

PyObject* (*__PyObject_Str)(PyObject *);

PyObject* (*_PyObject_Dir)(PyObject *);

PyObject* (*_PyObject_GetAttrString)(PyObject *, const char *);
int (*_PyObject_HasAttrString)(PyObject*, const char *);

Py_ssize_t (*_PyTuple_Size)(PyObject *);
PyObject* (*_PyTuple_GetItem)(PyObject *, Py_ssize_t);

PyObject* (*_PyList_New)(Py_ssize_t size);
Py_ssize_t (*_PyList_Size)(PyObject *);
PyObject* (*_PyList_GetItem)(PyObject *, Py_ssize_t);
int (*_PyList_SetItem)(PyObject *, Py_ssize_t, PyObject *);

void (*_PyErr_Fetch)(PyObject **, PyObject **, PyObject **);
void (*_PyErr_NormalizeException)(PyObject**, PyObject**, PyObject**);

int (*_PyCallable_Check)(PyObject*);

PyObject* (*_PyRun_StringFlags)(const char *, int, PyObject*, PyObject*, void*);
int (*_PyRun_SimpleFileExFlags)(FILE *, const char *, int, void *);


#define LOAD_PYTHON_SYMBOL_AS(name, as)             \
if (!loadSymbol(pLib_, #name, (void**)&as, pError)) \
  return false;

#define LOAD_PYTHON_SYMBOL(name)                                \
if (!loadSymbol(pLib_, #name, (void**)&_##name, pError)) \
  return false;

bool LibPython::load(const std::string& libPath, bool python3, std::string* pError)
{
  if (!loadLibrary(libPath, &pLib_, pError))
    return false;

  LOAD_PYTHON_SYMBOL(Py_Initialize)
  LOAD_PYTHON_SYMBOL(Py_IncRef)
  LOAD_PYTHON_SYMBOL(Py_DecRef)
  LOAD_PYTHON_SYMBOL(PyObject_GetAttrString)
  LOAD_PYTHON_SYMBOL(PyObject_HasAttrString)
  LOAD_PYTHON_SYMBOL(PyTuple_Size)
  LOAD_PYTHON_SYMBOL(PyTuple_GetItem)
  LOAD_PYTHON_SYMBOL(PyList_New)
  LOAD_PYTHON_SYMBOL(PyList_Size)
  LOAD_PYTHON_SYMBOL(PyList_GetItem)
  LOAD_PYTHON_SYMBOL(PyList_SetItem)
  LOAD_PYTHON_SYMBOL(PyErr_Fetch)
  LOAD_PYTHON_SYMBOL(PyErr_NormalizeException)
  LOAD_PYTHON_SYMBOL_AS(PyObject_Str, __PyObject_Str)
  LOAD_PYTHON_SYMBOL(PyObject_Dir)
  LOAD_PYTHON_SYMBOL(PyCallable_Check)
  LOAD_PYTHON_SYMBOL(PyRun_StringFlags)

  return true;
}

bool LibPython::unload(std::string* pError)
{
  if (pLib_ != NULL)
    return closeLibrary(pLib_, pError);
  else
    return true;
}




