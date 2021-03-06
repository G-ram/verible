// Copyright 2017-2019 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VERIBLE_VERILOG_CST_FUNCTIONS_H_
#define VERIBLE_VERILOG_CST_FUNCTIONS_H_

// See comment at the top
// verilog/CST/verilog_treebuilder_utils.h that explains use
// of std::forward in Make* helper functions.

#include <utility>
#include <vector>

#include "common/analysis/syntax_tree_search.h"
#include "common/text/concrete_syntax_tree.h"
#include "common/text/symbol.h"
#include "verilog/CST/verilog_nonterminals.h"
#include "verilog/CST/verilog_treebuilder_utils.h"

namespace verilog {

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5>
verible::SymbolPtr MakeFunctionHeader(T0&& qualifiers, T1&& function_start,
                                      T2&& lifetime, T3&& return_type_id,
                                      T4&& ports, T5&& semicolon) {
  ExpectString(function_start, "function");
  ExpectString(semicolon, ";");
  return verible::MakeTaggedNode(
      NodeEnum::kFunctionHeader, std::forward<T0>(qualifiers),
      std::forward<T1>(function_start), std::forward<T2>(lifetime),
      std::forward<T3>(
          return_type_id) /* flattens to separate type and id nodes */,
      std::forward<T4>(ports), std::forward<T5>(semicolon));
}

template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9>
verible::SymbolPtr MakeFunctionDeclaration(T0&& qualifiers, T1&& function_start,
                                           T2&& lifetime, T3&& return_type_id,
                                           T4&& ports, T5&& semicolon,
                                           T6&& function_items, T7&& body,
                                           T8&& function_end, T9&& label) {
  ExpectString(function_end, "endfunction");
  return verible::MakeTaggedNode(
      NodeEnum::kFunctionDeclaration,
      verible::MakeTaggedNode(
          NodeEnum::kFunctionHeader, std::forward<T0>(qualifiers),
          std::forward<T1>(function_start), std::forward<T2>(lifetime),
          std::forward<T3>(
              return_type_id) /* flattens to separate type and id nodes */,
          std::forward<T4>(ports), std::forward<T5>(semicolon)),
      std::forward<T6>(function_items), std::forward<T7>(body),
      std::forward<T8>(function_end), std::forward<T9>(label));
}

// Find all function declarations, including class method declarations.
std::vector<verible::TreeSearchMatch> FindAllFunctionDeclarations(
    const verible::Symbol&);

// Returns the function declaration header (return type, id, ports)
const verible::SyntaxTreeNode& GetFunctionHeader(const verible::Symbol&);

// Returns the function lifetime of the node.
const verible::Symbol* GetFunctionLifetime(const verible::Symbol&);

// Returns the id of the function declaration.
const verible::Symbol* GetFunctionId(const verible::Symbol&);

}  // namespace verilog

#endif  // VERIBLE_VERILOG_CST_FUNCTIONS_H_
