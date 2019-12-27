#include "verilog/propagate/propagator.h"

#include <iostream>
#include <chrono> 

#include "verilog/CST/verilog_tree_print.h"
#include "verilog/CST/verilog_nonterminals.h"
#include "verilog/parser/verilog_parser.h"
#include "verilog/parser/verilog_token_enum.h"

namespace verilog {
namespace propagate {

const verible::SyntaxTreeNode *convert2Node(const verible::SymbolPtr& symbol) {
	return verible::down_cast<const verible::SyntaxTreeNode*>(symbol.get()); 
}

verible::SyntaxTreeNode *convert2Node(verible::SymbolPtr*& symbol) {
	return verible::down_cast<verible::SyntaxTreeNode*>(symbol->get()); 
}

const verible::SyntaxTreeLeaf *convert2Leaf(const verible::SymbolPtr& symbol) {
	return verible::down_cast<const verible::SyntaxTreeLeaf*>(symbol.get()); 
}

verible::SyntaxTreeLeaf *convert2Leaf(verible::SymbolPtr*& symbol) {
	return verible::down_cast<verible::SyntaxTreeLeaf*>(symbol->get()); 
}

int Propagator::parse(const std::string &_content) {
	if(_content.size() == 0) return 0;
	content = _content;
	analyzer = verilog::VerilogAnalyzer::AnalyzeAutomaticMode(content, filename);
	const auto lex_status = analyzer->LexStatus();
	const auto parse_status = analyzer->ParseStatus();

	if (!lex_status.ok() || !parse_status.ok()) {
		const std::vector<std::string> syntax_error_messages(
		analyzer->LinterTokenErrorMessages());
		for (const auto& message : syntax_error_messages) {
			std::cerr << message << std::endl;
		}
		return 1;
	}
	return 0;
}

void Propagator::DependencyVisitor::Visit(const verible::SyntaxTreeNode& node) {
	for(const auto& child : node.children()) {
		if(child != nullptr) {
			if(NodeEnum(node.Tag().tag) == NodeEnum::kPreprocessorInclude) {
				auto child_ptr = convert2Leaf(child);
				if(child_ptr->get().token_enum == yytokentype::TK_StringLiteral) {
					std::string filename(child_ptr->get().text);
					filename.erase(std::remove(filename.begin(), filename.end(), '\"' ), filename.end());
					dependencies.insert(filename);
				}
			}
			child->Accept(this);
		}
	}
}

void Propagator::propagate() {
	ResolveDependencyVisitor resolve_dependency_visitor(this);
	analyzer->Data().SyntaxTree()->Accept(
		&resolve_dependency_visitor,
		&analyzer->MutableData().MutableSyntaxTree()); 

	// verilog::PrettyPrintVerilogTree(*analyzer->Data().SyntaxTree(), analyzer->Data().Contents(),
	// 	&std::cout);	

	ConstantVisitor constant_visitor;
	// auto start = std::chrono::high_resolution_clock::now(); 
	analyzer->Data().SyntaxTree()->Accept(
		&constant_visitor, &analyzer->MutableData().MutableSyntaxTree()); 
	// auto stop = std::chrono::high_resolution_clock::now(); 
	// auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
	// std::cout << "Took: " << duration.count() << std::endl;
}

class PropagatorNode : public verible::SyntaxTreeLeaf {
public:
	explicit PropagatorNode(const verible::TokenInfo& token, Propagator *_propagator) 
		: verible::SyntaxTreeLeaf(token), propagator(_propagator) {}
	Propagator *getPropagator() { return propagator; }
private:
	Propagator *propagator = nullptr;
};

void Propagator::ResolveDependencyVisitor::Visit(
	const verible::SyntaxTreeNode& node, verible::SymbolPtr *symbol_ptr) {
	verible::SyntaxTreeNode *mutable_node = convert2Node(symbol_ptr);
	for(verible::SymbolPtr& child : mutable_node->mutable_children()) {
		if(child != nullptr) {
			child->Accept(this, &child);
			if(NodeEnum(node.Tag().tag) == NodeEnum::kPreprocessorInclude) {
				auto child_ptr = convert2Leaf(child);
				if(child_ptr->get().token_enum == yytokentype::TK_StringLiteral) {
					std::string filename(child_ptr->get().text);
					filename.erase(
						std::remove(filename.begin(), filename.end(), '\"' ), filename.end());
					auto it = propagator->dependencies.find(filename);
					if(it != propagator->dependencies.end()) {
						child = std::unique_ptr<PropagatorNode>(
							new PropagatorNode(child_ptr->get(), it->second));
					} else {
						std::cerr << "Could not find " << filename << std::endl;
					}
				}
			}
		}
	}
}

void Propagator::ConstantVisitor::Visit(
	const verible::SyntaxTreeNode& node, verible::SymbolPtr *symbol_ptr) {
	verible::SyntaxTreeNode *mutable_node = convert2Node(symbol_ptr);
	for(verible::SymbolPtr& child : mutable_node->mutable_children()) {
		if(child != nullptr) {
			child->Accept(this, &child);
		}
	}

	switch(NodeEnum(node.Tag().tag)) {
		case NodeEnum::kBinaryExpression: {
			break;
		}
		default: ;
	}

}

}
}