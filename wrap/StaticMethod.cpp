/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file StaticMethod.ccp
 * @author Frank Dellaert
 * @author Andrew Melim
 * @author Richard Roberts
 **/

#include "StaticMethod.h"
#include "utilities.h"
#include "Class.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <fstream>

using namespace std;
using namespace wrap;

/* ************************************************************************* */
void StaticMethod::proxy_header(FileWriter& proxyFile) const {
  string upperName = matlabName();
  upperName[0] = toupper(upperName[0], locale());
  proxyFile.oss << "    function varargout = " << upperName << "(varargin)\n";
}

/* ************************************************************************* */
string StaticMethod::wrapper_call(FileWriter& wrapperFile, Str cppClassName,
    Str matlabUniqueName, const ArgumentList& args) const {
  // check arguments
  // NOTE: for static functions, there is no object passed
  wrapperFile.oss << "  checkArguments(\"" << matlabUniqueName << "." << name_
      << "\",nargout,nargin," << args.size() << ");\n";

  // unwrap arguments, see Argument.cpp
  args.matlab_unwrap(wrapperFile, 0); // We start at 0 because there is no self object

  // call method and wrap result
  // example: out[0]=wrap<bool>(staticMethod(t));
  string expanded = cppClassName + "::" + name_;
  if (templateArgValue_)
    expanded += ("<" + templateArgValue_->qualifiedName("::") + ">");

  return expanded;
}

/* ************************************************************************* */
void StaticMethod::emit_cython_pxd(FileWriter& file) const {
  // don't support overloads for static method :-(
  for(size_t i = 0; i < nrOverloads(); ++i) {
    file.oss << "\t\t@staticmethod\n";
    file.oss << "\t\t";
    returnVals_[i].emit_cython_pxd(file);
    file.oss << name_ << ((i > 0) ? "_" + to_string(i) : "") << " \"" << name_
             << "\""
             << "(";
    argumentList(i).emit_cython_pxd(file);
    file.oss << ")\n";
  }
}

/* ************************************************************************* */
void StaticMethod::emit_cython_pyx(FileWriter& file, const Class& cls) const {
  // don't support overloads for static method :-(
  for(size_t i = 0; i < nrOverloads(); ++i) {
    file.oss << "\t@staticmethod\n";
    file.oss << "\tdef " << name_ << ((i > 0) ? "_" + to_string(i) : "")
             << "(";
    argumentList(i).emit_cython_pyx(file);
    file.oss << "):\n";
    file.oss << "\t\t";
    if (!returnVals_[i].isVoid()) file.oss << "return ";
    //... casting return value
    returnVals_[i].emit_cython_pyx_casting(file);
    if (!returnVals_[i].isVoid()) file.oss << "(";

    file.oss << cls.pyxCythonClass() << "." 
             << name_ << ((i>0)? "_" + to_string(i):"");
    if (templateArgValue_) file.oss << "[" << templateArgValue_->pyxCythonClass() << "]"; 
    file.oss << "(";
    argumentList(i).emit_cython_pyx_asParams(file);
    if (!returnVals_[i].isVoid()) file.oss << ")";
    file.oss << ")\n";
  }
}

/* ************************************************************************* */
