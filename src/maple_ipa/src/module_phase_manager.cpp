/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#include "module_phase_manager.h"
#include "class_hierarchy.h"
#include "class_init.h"
#include "option.h"
#include "bin_mpl_export.h"
#include "mpl_timer.h"
#include "clone.h"
#include "call_graph.h"
#if MIR_JAVA
#include "native_stub_func.h"
#include "vtable_analysis.h"
#include "reflection_analysis.h"
#include "annotation_analysis.h"
#include "vtable_impl.h"
#include "java_intrn_lowering.h"
#include "java_eh_lower.h"
#include "native_stub_func.h"
#include "muid_replacement.h"
#include "gen_check_cast.h"
#include "coderelayout.h"
#include "constantfold.h"
#endif  // ~MIR_JAVA

namespace {
constexpr char kDotStr[] = ".";
constexpr char kDotMplStr[] = ".mpl";
}

namespace maple {
void ModulePhaseManager::RegisterModulePhases() {
#define MODAPHASE(id, modphase)                                                    \
  do {                                                                             \
    MemPool *memPool = GetMemPool();                                               \
    ModulePhase *phase = new (memPool->Malloc(sizeof(modphase(id)))) modphase(id); \
    CHECK_NULL_FATAL(phase);                                                       \
    RegisterPhase(id, *phase);                                                     \
    arModuleMgr->AddAnalysisPhase(id, phase);                                      \
  } while (0);

#define MODTPHASE(id, modPhase)                                                    \
  do {                                                                             \
    MemPool *memPool = GetMemPool();                                               \
    ModulePhase *phase = new (memPool->Malloc(sizeof(modPhase(id)))) modPhase(id); \
    CHECK_NULL_FATAL(phase);                                                       \
    RegisterPhase(id, *phase);                                                     \
  } while (0);
#include "module_phases.def"
#undef MODAPHASE
#undef MODTPHASE
}

void ModulePhaseManager::AddModulePhases(const std::vector<std::string> &phases) {
  for (std::string const &phase : phases) {
    AddPhase(phase);
  }
}

void ModulePhaseManager::Run() {
  int phaseIndex = 0;
  for (auto it = PhaseSequenceBegin(); it != PhaseSequenceEnd(); ++it, ++phaseIndex) {
    PhaseID id = GetPhaseId(it);
    auto *p = static_cast<ModulePhase*>(GetPhase(id));
    MIR_ASSERT(p);
    // if we need to skip after certain pass
    if (Options::skipFrom.compare(p->PhaseName()) == 0) {
      break;
    }
    if (!Options::quiet) {
      LogInfo::MapleLogger() << "---Run Module Phase [ " << p->PhaseName() << " ]---\n";
    }
    MPLTimer timer;
    if (timePhases) {
      timer.Start();
    }
    p->Run(&mirModule, arModuleMgr);
    if (timePhases) {
      timer.Stop();
      phaseTimers[phaseIndex] += timer.ElapsedMicroseconds();
    }
    if (Options::skipAfter.compare(p->PhaseName()) == 0) {
      break;
    }
  }
}

void ModulePhaseManager::Emit(const std::string &passName) {
  std::string outFileName;
  std::string moduleFileName = mirModule.GetFileName();
  std::string::size_type lastDot = moduleFileName.find_last_of(kDotStr);
  if (lastDot == std::string::npos) {
    outFileName = moduleFileName + kDotStr;
  } else {
    outFileName = moduleFileName.substr(0, lastDot) + kDotStr;
  }
  outFileName = outFileName.append(passName).append(kDotMplStr);
  mirModule.DumpToFile(outFileName);
}
}  // namespace maple
