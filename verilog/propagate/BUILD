# This package SystemVerilog-specific code constant propagatating functions.

licenses(["notice"])

package(
    default_visibility = [
        "//verilog/tools/propagate:__pkg__",
    ],
)

# libraries

cc_library(
    name = "propagator",
    srcs = ["propagator.cc"],
    hdrs = ["propagator.h"],
    deps = [
        "//common/text:concrete_syntax_leaf",
        "//common/text:concrete_syntax_tree",
        "//common/text:constants",
        "//common/text:symbol",
        "//common/text:syntax_tree_context",
        "//common/text:text_structure",
        "//common/text:token_info",
        "//common/text:token_stream_view",
        "//common/util:container_iterator_range",
        "//common/util:enum_flags",
        "//common/util:logging",
        "//common/util:value_saver",
        "//verilog/CST:verilog_nonterminals",
        "//verilog/CST:verilog_tree_print", # for debugging
        "//verilog/parser:verilog_parser",  # only for verilog_symbol_name
        "//verilog/parser:verilog_token_enum",
        "//verilog/analysis:verilog_analyzer",
        "@com_google_absl//absl/base:core_headers",
    ],
)