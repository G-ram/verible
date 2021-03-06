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

#include "verilog/CST/parameters.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "common/text/concrete_syntax_leaf.h"
#include "common/text/concrete_syntax_tree.h"
#include "common/text/symbol.h"
#include "common/text/text_structure.h"
#include "common/text/token_info.h"
#include "common/util/casts.h"
#include "common/util/logging.h"
#include "verilog/CST/verilog_nonterminals.h"
#include "verilog/analysis/verilog_analyzer.h"
#include "verilog/parser/verilog_token_enum.h"

#undef ASSERT_OK
#define ASSERT_OK(value) ASSERT_TRUE((value).ok())

namespace verilog {
namespace {

using verible::down_cast;

// Tests that the correct amount of kParameterDeclarations are found.
TEST(FindAllParamDeclarationsTest, BasicParams) {
  const std::pair<std::string, int> kTestCases[] = {
      {"", 0},
      {"module foo; endmodule", 0},
      {"module foo (input bar); endmodule", 0},
      {"module foo; localparam Bar = 1; endmodule", 1},
      {"module foo; localparam int Bar = 1; endmodule", 1},
      {"module foo; parameter int Bar = 1; endmodule", 1},
      {"module foo #(parameter int Bar = 1); endmodule", 1},
      {"module foo; localparam int Bar = 1; localparam int BarSecond = 2; "
       "endmodule",
       2},
      {"class foo; localparam int Bar = 1; endclass", 1},
      {"class foo #(parameter int Bar = 1); endclass", 1},
      {"package foo; parameter Bar = 1; endpackage", 1},
      {"package foo; parameter int Bar = 1; endpackage", 1},
      {"parameter int Bar = 1;", 1},
      {"parameter Bar = 1;", 1},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test.first, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations =
        FindAllParamDeclarations(*ABSL_DIE_IF_NULL(root));
    EXPECT_EQ(param_declarations.size(), test.second);
  }
}

// Tests that GetParamKeyword correctly returns that the parameter type is
// localparam.
TEST(GetParamKeywordTest, LocalParamDeclared) {
  const std::pair<std::string, int> kTestCases[] = {
      {"module foo; localparam int Bar = 1; endmodule", 1},
      {"class foo; localparam int Bar = 1; endclass", 1},
      {"module foo; localparam Bar = 1; endmodule", 1},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test.first, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    ASSERT_EQ(param_declarations.size(), test.second);
    const auto& param_node = down_cast<const verible::SyntaxTreeNode&>(
        *param_declarations.front().match);
    const auto param_keyword = GetParamKeyword(param_node);
    EXPECT_EQ(param_keyword, TK_localparam);
  }
}

// Tests that GetParamKeyword correctly returns that the parameter type is
// parameter.
TEST(GetParamKeywordTest, ParameterDeclared) {
  const std::pair<std::string, int> kTestCases[] = {
      {"module foo; parameter int Bar = 1; endmodule", 1},
      {"module foo #(parameter int Bar = 1); endmodule", 1},
      {"module foo #(int Bar = 1); endmodule", 1},
      {"class foo; parameter int Bar = 1; endclass", 1},
      {"class foo #(parameter int Bar = 1); endclass", 1},
      {"class foo #(int Bar = 1); endclass", 1},
      {"package foo; parameter int Bar = 1; endpackage", 1},
      {"package foo; parameter Bar = 1; endpackage", 1},
      {"parameter int Bar = 1;", 1},
      {"parameter Bar = 1;", 1},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test.first, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    ASSERT_EQ(param_declarations.size(), test.second);
    const auto& param_node = down_cast<const verible::SyntaxTreeNode&>(
        *param_declarations.front().match);
    const auto param_keyword = GetParamKeyword(param_node);
    EXPECT_EQ(param_keyword, TK_parameter);
  }
}

// Tests that GetParamKeyword correctly returns the parameter type when multiple
// parameters are defined.
TEST(GetParamKeywordTest, MultipleParamsDeclared) {
  const std::string kTestCases[] = {
      {"module foo; parameter int Bar = 1; localparam int Bar_2 = 2; "
       "endmodule"},
      {"class foo; parameter int Bar = 1; localparam int Bar_2 = 2; endclass"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);

    // Make sure the first one is TK_parameter.
    const auto& param_node =
        down_cast<const verible::SyntaxTreeNode&>(*param_declarations[0].match);
    const auto param_keyword = GetParamKeyword(param_node);
    EXPECT_EQ(param_keyword, TK_parameter);

    // Make sure the second one is TK_localparam.
    const auto& localparam_node =
        down_cast<const verible::SyntaxTreeNode&>(*param_declarations[1].match);
    const auto localparam_keyword = GetParamKeyword(localparam_node);
    EXPECT_EQ(localparam_keyword, TK_localparam);
  }
}

// Tests that GetParamTypeSymbol correctly returns the kParamType node.
TEST(GetParamTypeSymbolTest, BasicTests) {
  const std::string kTestCases[] = {
      {"module foo; parameter Bar = 1; endmodule"},
      {"module foo; parameter int Bar = 1; endmodule"},
      {"module foo #(parameter int Bar = 1); endmodule"},
      {"module foo; localparam int Bar = 1; endmodule"},
      {"class foo; parameter int Bar = 1; endclass"},
      {"class foo; localparam int Bar = 1; endclass"},
      {"package foo; parameter int Bar = 1; endpackage"},
      {"parameter int Bar = 1;"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto* param_type_symbol =
        GetParamTypeSymbol(*param_declarations.front().match);
    const auto t = param_type_symbol->Tag();
    EXPECT_EQ(t.kind, verible::SymbolKind::kNode);
    EXPECT_EQ(NodeEnum(t.tag), NodeEnum::kParamType);
  }
}

// Tests that GetParameterNameToken correctly returns the token of the
// parameter.
TEST(GetParameterNameTokenTest, BasicTests) {
  const std::pair<std::string, absl::string_view> kTestCases[] = {
      {"module foo; parameter Bar = 1; endmodule", "Bar"},
      {"module foo; localparam Bar_1 = 1; endmodule", "Bar_1"},
      {"module foo; localparam int HelloWorld = 1; endmodule", "HelloWorld"},
      {"module foo #(parameter int HelloWorld1 = 1); endmodule", "HelloWorld1"},
      {"class foo; parameter HelloWorld_1 = 1; endclass", "HelloWorld_1"},
      {"class foo; localparam FooBar = 1; endclass", "FooBar"},
      {"class foo; localparam int Bar_1_1 = 1; endclass", "Bar_1_1"},
      {"package foo; parameter BAR = 1; endpackage", "BAR"},
      {"package foo; parameter int HELLO_WORLD = 1; endpackage", "HELLO_WORLD"},
      {"parameter int Bar = 1;", "Bar"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test.first, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto name_token =
        GetParameterNameToken(*param_declarations.front().match);
    EXPECT_EQ(name_token.text, test.second);
  }
}

// Tests that GetSymbolIdentifierFromParamDeclaration correctly returns the
// token of the symbol identifier.
TEST(GetSymbolIdentifierFromParamDeclarationTest, BasicTests) {
  const std::pair<std::string, absl::string_view> kTestCases[] = {
      {"module foo; parameter type Bar; endmodule", "Bar"},
      {"module foo; localparam type Bar_1; endmodule", "Bar_1"},
      {"module foo #(parameter type HelloWorld1); endmodule", "HelloWorld1"},
      {"class foo #(parameter type Bar); endclass", "Bar"},
      {"class foo; parameter type HelloWorld_1; endclass", "HelloWorld_1"},
      {"class foo; localparam type Bar_1_1; endclass", "Bar_1_1"},
      {"package foo; parameter type HELLO_WORLD; endpackage", "HELLO_WORLD"},
      {"parameter type Bar;", "Bar"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test.first, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto name_token = GetSymbolIdentifierFromParamDeclaration(
        *param_declarations.front().match);
    EXPECT_EQ(name_token.text, test.second);
  }
}

// Tests that IsParamTypeDeclaration correctly returns true if the parameter is
// a parameter type declaration.
TEST(IsParamTypeDeclarationTest, BasicTests) {
  const std::pair<std::string, bool> kTestCases[] = {
      {"module foo; parameter type Bar; endmodule", true},
      {"module foo; localparam type Bar_1; endmodule", true},
      {"module foo #(parameter type HelloWorld1); endmodule", true},
      {"class foo #(parameter type Bar); endclass", true},
      {"class foo; parameter type HelloWorld_1; endclass", true},
      {"class foo; localparam type Bar_1_1; endclass", true},
      {"package foo; parameter type HELLO_WORLD; endpackage", true},
      {"parameter type Bar;", true},
      {"module foo; parameter Bar = 1; endmodule", false},
      {"module foo; localparam int HelloWorld = 1; endmodule", false},
      {"module foo #(parameter int HelloWorld1 = 1); endmodule", false},
      {"class foo; parameter HelloWorld_1 = 1; endclass", false},
      {"class foo; localparam FooBar = 1; endclass", false},
      {"package foo; parameter int HELLO_WORLD = 1; endpackage", false},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test.first, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto is_param_type =
        IsParamTypeDeclaration(*param_declarations.front().match);
    EXPECT_EQ(is_param_type, test.second);
  }
}

// Tests that GetTypeAssignmentFromParamDeclaration correctly returns the
// kTypeAssignment node.
TEST(GetTypeAssignmentFromParamDeclarationTests, BasicTests) {
  const std::string kTestCases[] = {
      {"module foo; parameter type Bar = 1; endmodule"},
      {"module foo #(parameter type Bar = 1); endmodule"},
      {"module foo; localparam type Bar = 1; endmodule"},
      {"class foo; parameter type Bar = 1; endclass"},
      {"class foo; localparam type Bar = 1; endclass"},
      {"package foo; parameter type Bar = 1; endpackage"},
      {"parameter type Bar = 1;"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto* type_assignment_symbol = GetTypeAssignmentFromParamDeclaration(
        *param_declarations.front().match);
    const auto t = type_assignment_symbol->Tag();
    EXPECT_EQ(t.kind, verible::SymbolKind::kNode);
    EXPECT_EQ(NodeEnum(t.tag), NodeEnum::kTypeAssignment);
  }
}

// Tests that GetIdentifierLeafFromTypeAssignment correctly returns the
// SyntaxTreeLeaf of the symbol identifier.
TEST(GetIdentifierLeafFromTypeAssignmentTest, BasicTests) {
  const std::pair<std::string, absl::string_view> kTestCases[] = {
      {"module foo; parameter type Bar; endmodule", "Bar"},
      {"module foo; localparam type Bar_1; endmodule", "Bar_1"},
      {"module foo #(parameter type HelloWorld1); endmodule", "HelloWorld1"},
      {"class foo #(parameter type Bar); endclass", "Bar"},
      {"class foo; parameter type HelloWorld_1; endclass", "HelloWorld_1"},
      {"class foo; localparam type Bar_1_1; endclass", "Bar_1_1"},
      {"package foo; parameter type HELLO_WORLD; endpackage", "HELLO_WORLD"},
      {"parameter type Bar;", "Bar"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test.first, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto* type_assignment_symbol = GetTypeAssignmentFromParamDeclaration(
        *param_declarations.front().match);
    const auto* identifier_leaf =
        GetIdentifierLeafFromTypeAssignment(*type_assignment_symbol);
    EXPECT_EQ(identifier_leaf->get().text, test.second);
  }
}

// Tests that GetParamTypeInfoSymbol correctly returns the kTypeInfo node.
TEST(GetParamTypeInfoSymbolTest, BasicTests) {
  const std::string kTestCases[] = {
      {"module foo; parameter Bar = 1; endmodule"},
      {"module foo; parameter int Bar = 1; endmodule"},
      {"module foo #(parameter int Bar = 1); endmodule"},
      {"module foo; localparam int Bar = 1; endmodule"},
      {"class foo; parameter int Bar = 1; endclass"},
      {"class foo; localparam int Bar = 1; endclass"},
      {"package foo; parameter int Bar = 1; endpackage"},
      {"parameter int Bar = 1;"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto* type_info_symbol =
        GetParamTypeInfoSymbol(*param_declarations.front().match);
    const auto t = type_info_symbol->Tag();
    EXPECT_EQ(t.kind, verible::SymbolKind::kNode);
    EXPECT_EQ(NodeEnum(t.tag), NodeEnum::kTypeInfo);
  }
}

TEST(IsTypeInfoEmptyTest, EmptyTests) {
  const std::string kTestCases[] = {
      {"module foo; parameter Bar = 1; endmodule"},
      {"module foo #(parameter Bar = 1); endmodule"},
      {"module foo; localparam Bar = 1; endmodule"},
      {"class foo; parameter Bar = 1; endclass"},
      {"class foo; localparam Bar = 1; endclass"},
      {"package foo; parameter Bar = 1; endpackage"},
      {"parameter Bar = 1;"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto* type_info_symbol =
        GetParamTypeInfoSymbol(*param_declarations.front().match);
    const auto t = type_info_symbol->Tag();
    EXPECT_EQ(t.kind, verible::SymbolKind::kNode);
    EXPECT_EQ(NodeEnum(t.tag), NodeEnum::kTypeInfo);
    EXPECT_TRUE(IsTypeInfoEmpty(*type_info_symbol));
  }
}

TEST(IsTypeInfoEmptyTest, NonEmptyTests) {
  const std::string kTestCases[] = {
      {"module foo; localparam bit Bar = 1; endmodule"},
      {"module foo #(parameter int Bar = 1); endmodule"},
      {"module foo; parameter int Bar = 1; endmodule"},
      {"class foo; parameter string Bar = \"Bar\"; endclass"},
      {"class foo; localparam logic Bar = 1; endclass"},
      {"parameter int Bar = 1;"},
      {"parameter signed Bar = 1;"},
      {"parameter unsigned Bar = 1;"},
      {"parameter int unsigned Bar = 1;"},
      {"parameter Other_t Bar = other_t::kEnum;"},
      {"parameter pkg_p::Other_t Bar = other_t::kEnum;"},
      {"module foo; localparam int signed  Bar = 1; endmodule"},
      {"module foo #(parameter signed Bar = 1); endmodule"},
      {"module foo #(parameter int signed Bar = 1); endmodule"},
      {"module foo #(parameter Other_t Bar); endmodule"},
      {"module foo #(parameter pkg::Other_t Bar); endmodule"},
      {"module foo #(parameter pkg::Other_t Bar = enum_e::value); endmodule"},
      {"class foo #(parameter Other_t Bar); endclass"},
      {"class foo #(parameter pkg::Other_t Bar); endclass"},
      {"class foo #(parameter pkg::Other_t Bar = enum_e::value); endclass"},
  };
  for (const auto& test : kTestCases) {
    VerilogAnalyzer analyzer(test, "");
    ASSERT_OK(analyzer.Analyze());
    const auto& root = analyzer.Data().SyntaxTree();
    const auto param_declarations = FindAllParamDeclarations(*root);
    const auto* type_info_symbol =
        GetParamTypeInfoSymbol(*param_declarations.front().match);
    const auto t = type_info_symbol->Tag();
    EXPECT_EQ(t.kind, verible::SymbolKind::kNode);
    EXPECT_EQ(NodeEnum(t.tag), NodeEnum::kTypeInfo);
    EXPECT_FALSE(IsTypeInfoEmpty(*type_info_symbol));
  }
}

}  // namespace
}  // namespace verilog
