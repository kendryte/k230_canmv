#include "micropython-wrap/classwrapper.h"
#include "micropython-wrap/functionwrapper.h"
#include "micropython-wrap/variable.h"
#include "micropython-wrap/util.h"

// function we want to call from within a MicroPython script
std::vector<std::string> SomeFunction(std::vector<std::string> vec)
{
  for (auto &v : vec)
    v += "TRANSFORM";
  return vec;
}

// function names are declared in structs
struct FunctionNames
{
  func_name_def(TransformList)
};

extern "C"
{
  void doinit_audio_module(mp_obj_dict_t *mod)
  {
    // Note this one must stay because this function can get called directly,
    // so without CreateModule() already calling InitializePyObjectStore().
    upywrap::InitializePyObjectStore(*mod);

    // register our function with the name 'TransformList'
    // conversion of a MicroPython list of strings is done automatically
    upywrap::FunctionWrapper wrapfunc(mod);
    wrapfunc.Def<FunctionNames::TransformList>(SomeFunction);
  }
}
