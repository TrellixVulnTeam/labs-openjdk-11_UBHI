/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#ifndef SHARE_VM_JVMCI_JVMCI_COMPILER_HPP
#define SHARE_VM_JVMCI_JVMCI_COMPILER_HPP

#include "compiler/abstractCompiler.hpp"

class JVMCICompiler : public AbstractCompiler {
 public:
  // Code installation specific statistics.
  class CodeInstallStats {
   private:
    elapsedTimer _timer;
    volatile int _count;
    volatile int _codeBlobs_size;
    volatile int _codeBlobs_code_size;
   public:
    CodeInstallStats() :
      _count(0),
      _codeBlobs_size(0),
      _codeBlobs_code_size(0)
    {}

    elapsedTimer* timer() { return &_timer; }
    void print_on(outputStream* st, const char* prefix) const;

    // Notifies this object that `cb` has just been
    // installed in the code cache.
    void on_install(CodeBlob* cb);
  };

 private:
  bool _bootstrapping;

  /**
   * True if we have seen a bootstrap compilation request.
   */
  volatile bool _bootstrap_compilation_request_handled;

  /**
   * Number of methods successfully compiled by a call to
   * JVMCICompiler::compile_method().
   */
  volatile int _methods_compiled;

  // Incremented periodically by JVMCI compiler threads
  // to indicate JVMCI compilation activity.
  volatile int _global_compilation_ticks;

  static JVMCICompiler* _instance;

  CodeInstallStats _jit_code_installs;     // CompileBroker compilations
  CodeInstallStats _hosted_code_installs;  // Non-CompileBroker compilations

  /**
   * Exits the VM due to an unexpected exception.
   */
  static void exit_on_pending_exception(oop exception, const char* message);

public:
  JVMCICompiler();

  static JVMCICompiler* instance(bool require_non_null, TRAPS) {
    if (!EnableJVMCI) {
      THROW_MSG_NULL(vmSymbols::java_lang_InternalError(), "JVMCI is not enabled")
    }
    if (_instance == NULL && require_non_null) {
      THROW_MSG_NULL(vmSymbols::java_lang_InternalError(), "The JVMCI compiler instance has not been created");
    }
    return _instance;
  }

  virtual const char* name() { return UseJVMCINativeLibrary ? "JVMCI-native" : "JVMCI"; }

  virtual bool supports_native()                 { return true; }
  virtual bool supports_osr   ()                 { return true; }

  bool is_jvmci()                                { return true; }
  bool is_c1   ()                                { return false; }
  bool is_c2   ()                                { return false; }

  bool needs_stubs            () { return false; }

  // Initialization
  virtual void initialize();

  /**
   * Initialize the compile queue with the methods in java.lang.Object and
   * then wait until the queue is empty.
   */
  void bootstrap(TRAPS);

  // Should force compilation of method at CompLevel_simple?
  bool force_comp_at_level_simple(Method* method);

  bool is_bootstrapping() const { return _bootstrapping; }

  void set_bootstrap_compilation_request_handled() {
    _instance->_bootstrap_compilation_request_handled = true;
  }

  // Compilation entry point for methods
  virtual void compile_method(ciEnv* env, ciMethod* target, int entry_bci, DirectiveSet* directive);

  virtual void stopping_compiler_thread(CompilerThread* current);

  virtual void on_empty_queue(CompileQueue* queue, CompilerThread* thread);

  // Print compilation timers and statistics
  virtual void print_timers();

  // Gets the number of methods that have been successfully compiled by
  // a call to JVMCICompiler::compile_method().
  int methods_compiled() { return _methods_compiled; }
  void inc_methods_compiled();

  // Gets a value indicating JVMCI compilation activity on any thread.
  // If successive calls to this method return a different value, then
  // some degree of JVMCI compilation occurred between the calls.
  int global_compilation_ticks() const { return _global_compilation_ticks; }
  void inc_global_compilation_ticks();

  CodeInstallStats* code_install_stats(bool hosted) {
    if (!hosted) {
      return &_jit_code_installs;
    } else {
      return &_hosted_code_installs;
    }
  }
};

#endif // SHARE_VM_JVMCI_JVMCI_COMPILER_HPP
