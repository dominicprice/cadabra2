
#include "Compare.hh"
#include "NEvaluator.hh"
#include <cmath>

using namespace cadabra;

// void NEvaluator::find_common_subexpressions(std::vector<Ex *>)
// 	{
// 	// Compute the hash value of every subtree, and collect matches.
// 	// Then compare subtrees with equal hash to find common subtrees.
// 	}

NEvaluator::NEvaluator(const Ex &ex_)
	: ex(ex_)
	{
	}

NTensor NEvaluator::evaluate()
	{
	find_variable_locations();

	const auto n_sin  = name_set.find("\\sin");
	const auto n_cos  = name_set.find("\\cos");
	const auto n_pow  = name_set.find("\\pow");
	const auto n_prod = name_set.find("\\prod");
	const auto n_sum  = name_set.find("\\sum");

	NTensor lastval(0);

	auto it = ex.begin_post();
	while(it != ex.end_post()) {
		// Either this node is known in the subtree value map,
		// or this node is a function which combines the values
		// of child nodes.


		auto fnd = subtree_values.find(Ex::iterator(it));
		if(fnd!=subtree_values.end()) {
			//std::cerr << it << " has value " << fnd->second << std::endl;
			}
		else {
			if(it->name==n_sin) {
				auto arg    = ex.begin(it);
				auto argval = subtree_values.find(arg)->second;
				lastval = argval.apply(std::sin);
				}
			else if(it->name==n_cos) {
				auto arg    = ex.begin(it);
				auto argval = subtree_values.find(arg)->second;
				lastval = argval.apply(std::cos);
				}
			else if(it->name==n_prod) {
				for(auto cit = ex.begin(it); cit!=ex.end(it); ++cit) {
					auto cfnd = subtree_values.find(Ex::iterator(cit));
					if(cfnd==subtree_values.end())
						throw std::logic_error("Inconsistent value tree.");
					if(cit==ex.begin(it))
						lastval = cfnd->second;
					else
						lastval *= cfnd->second;
					}
				}
			else if(it->name==n_sum) {
				for(auto cit = ex.begin(it); cit!=ex.end(it); ++cit) {
					auto cfnd = subtree_values.find(Ex::iterator(cit));
					if(cfnd==subtree_values.end())
						throw std::logic_error("Inconsistent value tree.");
					if(cit==ex.begin(it))
						lastval = cfnd->second;
					else
						lastval += cfnd->second;
					}
				}
			else if(it->name==n_pow) {
				//std::cerr << "cannot find " << *it << std::endl;

				throw std::logic_error("Value unknown for subtree special function.");
				}
			else {
				// Try variable substitution rules.
				bool found=false;
				for(const auto& var: variable_values) {
					// std::cerr << "Comparing " << var.first << " with " << *it << std::endl;
					if(var.variable == *it) {
						subtree_values.insert(std::make_pair(it, var.values));
						lastval = var.values;
						// std::cerr << "We know the value of " << *it << std::endl;
						found=true;
						break;
						}
					}
				if(!found)
					throw std::logic_error("Value unknown for subtree.");
				}
			subtree_values.insert(std::make_pair(it, lastval));
			}

		++it;
		}

	return lastval;
	}

void NEvaluator::set_variable(const Ex& var, const NTensor& val)
	{
	variable_values.push_back( VariableValues({var, val}) );
	}

void NEvaluator::find_variable_locations()
	{
	// FIXME: we don't really need this anymore, as we do everything
	// with broadcasting.
	for(auto& var: variable_values) {
		auto it = ex.begin_post();
		while(it != ex.end_post()) {
			if(var.variable == *it)
				var.locations.push_back(it);
			++it;
			}
		// std::cerr << "Variable " << var.variable << " at " << var.locations.size() << " places" << std::endl;
		}

	// Now insert subtree values which are such that for every
	// variable node we have an NTensor which is broadcast to the
	// shape of the full variable set NTensor.

	// std::cerr << "full shape = ";
	std::vector<size_t> fullshape;
	for(const auto& var: variable_values) {
		assert(var.values.shape.size()==1);
		fullshape.push_back(var.values.shape[0]);
		std::cerr << var.values.shape[0] << ", ";
		}
	// std::cerr << std::endl;

	for(size_t v=0; v<variable_values.size(); ++v) {
		const auto& var = variable_values[v];
		for(const auto& it: var.locations) {
			subtree_values.insert(std::make_pair(it, var.values.broadcast( fullshape, v ) ) );
			}
		}

	// std::cerr << "ready" << std::endl;
	}
