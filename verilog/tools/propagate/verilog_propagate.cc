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

// verilog_syntax is a simple command-line utility to check Verilog syntax
// for the given file.
//
// Example usage:
// verilog_syntax --verilog_trace_parser files...

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>  // IWYU pragma: keep  // for ostringstream
#include <string>   // for string, allocator, etc
#include <map>
#include <vector>

#include "common/text/concrete_syntax_tree.h"
#include "common/text/parser_verifier.h"
#include "common/text/text_structure.h"
#include "common/text/token_info.h"
#include "common/util/file_util.h"
#include "common/util/logging.h"  // for operator<<, LOG, LogMessage, etc
#include "common/util/status.h"
#include "verilog/analysis/verilog_analyzer.h"
#include "verilog/parser/verilog_parser.h"
#include "verilog/propagate/propagator.h"

struct args_t {
	std::string output;
	std::map<std::string, std::string> macros;
	std::vector<std::string> files;
};

std::pair<std::string, std::string> parseMacro(const std::string &macro) {
	size_t equal_index = macro.find("="); 
	if(equal_index == std::string::npos) {
		return std::make_pair(macro, "");
	}
	return std::make_pair(
		macro.substr(0, equal_index), 
		macro.substr(equal_index + 1, macro.size() - equal_index - 1));
}

args_t parseArgs(int argc, char** argv) {
	args_t args;
	for(int i = 1; i < argc; i++) {
		std::string token = argv[i];

		if(token.substr(0, 2).compare("-D") == 0) {
			if(token.size() > 2) {
				args.macros.insert(
					parseMacro(token.substr(2, token.size() - 2)));
			} else {
				std::string next_token = argv[++i];
				args.macros.insert(parseMacro(next_token));
			}
		} else if(token.substr(0, 2).compare("-o") == 0) {
			args.output = argv[++i];		
		} else {
			args.files.push_back(token);
		}
	}
	return args;
}

std::string synthesizeMacro(const std::map<std::string, std::string>& macros) {
	std::string macros_str = "`ifndef MACRO_GENERATED\n";
	macros_str.append("`define MACRO_GENERATED\n");
	for(const auto& m : macros) {
		macros_str.append("`define ");
		macros_str.append(m.first);
		macros_str.append(" ");
		macros_str.append(m.second);
		macros_str.append("\n");
	}
	macros_str.append("`endif\n\n");
	return macros_str;
}

int main(int argc, char** argv) {
	const args_t args = parseArgs(argc, argv);
	std::string macros = synthesizeMacro(args.macros);

	std::map<std::string, verilog::propagate::Propagator*> parsed;
	std::vector<std::string> order;

	for(const std::string& filename : args.files) {
		std::string content;
		if (!verible::file::GetContents(filename, &content)) {
			std::cerr << "Could not get contents of " << filename << std::endl;
			return 1;
		}
		content.insert(0, macros);
		parsed.insert(std::make_pair(filename, 
			new verilog::propagate::Propagator(filename)));
		if(parsed[filename]->parse(content) != 0) {
			return 1;
		}
		order.push_back(filename);
	}

	// Resolve dependencies
	for(const std::string& filename : order) {
		for(const std::string& dependency : parsed[filename]->getDependencies()) {
			for(const std::string& other : order) {
				if(filename.compare(other) != 0 && 
					dependency.compare(other) == 0) {
					parsed[filename]->resolveDependency(other, parsed[filename]);
				}
			}
		}
	}

	// Propagate
	for(const std::string& filename : order) {
		parsed[filename]->propagate();
	}

	// Dump
	for(const std::string& filename : order) {
		std::cout << "-----------------------------" << std::endl;
		std::cout << filename << std::endl;
		parsed[filename]->dump("");
		std::cout << "-----------------------------" << std::endl;
	}

	return 0;
}
