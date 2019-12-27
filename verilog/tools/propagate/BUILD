# 'verilog_propagate' is a program for propagating constants for Verilog/SystemVerilog syntax.

licenses(["notice"])

cc_binary(
    name = "verilog_propagate",
    srcs = ["verilog_propagate.cc"],
    visibility = ["//visibility:public"],  # for verilog_style_lint.bzl
    deps = [
        "//common/text:concrete_syntax_tree",
        "//common/text:parser_verifier",
        "//common/text:text_structure",
        "//common/text:token_info",
        "//common/util:file_util",
        "//common/util:logging",
        "//common/util:status",
        "//verilog/propagate:propagator",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
    ],
)
