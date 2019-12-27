#ifndef VERIBLE_VERILOG_PROPAGATE_PROPAGATOR_H_
#define VERIBLE_VERILOG_PROPAGATE_PROPAGATOR_H_

#include <set>
#include <string>
#include <iostream>

#include "common/text/concrete_syntax_leaf.h"
#include "common/text/concrete_syntax_tree.h"
#include "common/text/symbol.h"
#include "common/text/visitors.h"
#include "common/text/text_structure.h"
#include "verilog/analysis/verilog_analyzer.h"

namespace verilog {
namespace propagate {

class Propagator {
protected:
	class DependencyVisitor : public verible::SymbolVisitor {
	public:
		void Visit(const verible::SyntaxTreeLeaf& leaf) {}
		void Visit(const verible::SyntaxTreeNode& node);	
		std::set<std::string> getDependencies() { return dependencies; }
	protected:
		std::set<std::string> dependencies;
	};
	class ResolveDependencyVisitor : public verible::MutableTreeVisitorRecursive {
	public:
		ResolveDependencyVisitor(Propagator *_propagator) 
			: propagator(_propagator) {}
		void Visit(const verible::SyntaxTreeLeaf& leaf, 
			verible::SymbolPtr *symbol_ptr) {}
		void Visit(const verible::SyntaxTreeNode& node, 
			verible::SymbolPtr *symbol_ptr);
	private:
		Propagator *propagator;
	};
	class ConstantVisitor : public verible::MutableTreeVisitorRecursive {
	public:
		void Visit(const verible::SyntaxTreeLeaf& leaf, 
			verible::SymbolPtr *symbol_ptr) {}
		void Visit(const verible::SyntaxTreeNode& node, 
			verible::SymbolPtr *symbol_ptr);
	};
public:
	Propagator(const std::string& _filename)
		: filename(_filename) {}
	Propagator(const Propagator& _propagator) 
		: filename(_propagator.getFilename()),
		content(_propagator.getContent()),
		dependencies(_propagator.getResolvedDependencies()) {}
	const std::string& getFilename() const { return filename; }
	const std::string& getContent() const { return content; }
	Propagator *clone() {
		Propagator *_propagator = new Propagator(*this);
		_propagator->parse();
		return _propagator;
	}
	int parse(const std::string& _content="");
	std::set<std::string> getDependencies() { 
		if(got_dependencies) return dependency_visitor.getDependencies();
		analyzer->Data().SyntaxTree()->Accept(&dependency_visitor); 
		got_dependencies = true;
		return dependency_visitor.getDependencies();	
	}
	verible::ConcreteSyntaxTree& getSyntaxTree() {
		return analyzer->MutableData().MutableSyntaxTree();
	}
	void resolveDependency(const std::string& name, Propagator *propagator) {
		dependencies.insert(std::make_pair(name, propagator));
	}
	const std::map<std::string, Propagator*> getResolvedDependencies() const {
		return dependencies;
	}
	void propagate();
	void dump(const std::string& destination) {}
protected:
	std::string filename;
	std::string content;
	std::unique_ptr<verilog::VerilogAnalyzer> analyzer;
protected:
	// Dependencies
	bool got_dependencies = false;
	DependencyVisitor dependency_visitor;
	std::map<std::string, Propagator*> dependencies;
};

}
}

#endif