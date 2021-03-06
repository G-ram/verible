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

#include "verilog/analysis/checkers/void_cast_rule.h"

#include <set>
#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "common/analysis/citation.h"
#include "common/analysis/lint_rule_status.h"
#include "common/analysis/matcher/bound_symbol_manager.h"
#include "common/text/concrete_syntax_leaf.h"
#include "common/text/concrete_syntax_tree.h"
#include "common/text/symbol.h"
#include "common/text/syntax_tree_context.h"
#include "common/text/token_info.h"
#include "common/text/tree_utils.h"
#include "verilog/analysis/descriptions.h"
#include "verilog/analysis/lint_rule_registry.h"

namespace verilog {
namespace analysis {

using verible::GetVerificationCitation;
using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;
using verible::SyntaxTreeLeaf;
using verible::SyntaxTreeNode;

// Register VoidCastRule
VERILOG_REGISTER_LINT_RULE(VoidCastRule);

absl::string_view VoidCastRule::Name() { return "void-cast"; }
const char VoidCastRule::kTopic[] = "void-casts";

std::string VoidCastRule::GetDescription(DescriptionType description_type) {
  return absl::StrCat(
      "Checks that void casts do not contain certain function/method calls. "
      "See ",
      GetVerificationCitation(kTopic), ".");
}

const std::set<std::string>& VoidCastRule::BlacklistedFunctionsSet() {
  static const auto* blacklisted_functions =
      new std::set<std::string>({"uvm_hdl_read"});
  return *blacklisted_functions;
}

void VoidCastRule::HandleSymbol(const verible::Symbol& symbol,
                                const SyntaxTreeContext& context) {
  // Check for blacklisted function names
  verible::matcher::BoundSymbolManager manager;
  if (blacklisted_function_matcher_.Matches(symbol, &manager)) {
    if (auto function_id = manager.GetAs<verible::SyntaxTreeLeaf>("id")) {
      const auto& bfs = BlacklistedFunctionsSet();
      if (bfs.find(std::string(function_id->get().text)) != bfs.end()) {
        violations_.insert(LintViolation(function_id->get(),
                                         FormatReason(*function_id), context));
      }
    }
  }

  // Check for blacklisted calls to randomize
  manager.Clear();
  if (randomize_matcher_.Matches(symbol, &manager)) {
    if (auto randomize_node = manager.GetAs<verible::SyntaxTreeNode>("id")) {
      auto leaf_ptr = verible::GetLeftmostLeaf(*randomize_node);
      const verible::TokenInfo token = ABSL_DIE_IF_NULL(leaf_ptr)->get();
      violations_.insert(LintViolation(
          token, "randomize() is forbidden within void casts", context));
    }
  }
}

LintRuleStatus VoidCastRule::Report() const {
  return LintRuleStatus(violations_, Name(), GetVerificationCitation(kTopic));
}

std::string VoidCastRule::FormatReason(
    const verible::SyntaxTreeLeaf& leaf) const {
  return std::string(leaf.get().text) +
         " is an invalid call within this void cast";
}

}  // namespace analysis
}  // namespace verilog
