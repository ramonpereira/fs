
#include <utils/binding_iterator.hxx>
#include <utils/cartesian_iterator.hxx>
#include <problem_info.hxx>

namespace fs0 { namespace utils {

const ObjectIdxVector binding_iterator::NIL{0};

binding_iterator::binding_iterator(const Signature& types, const ProblemInfo& info) :
	_valid(), // To avoid two iterations, this will get initialized in the call to 'generate_values' below
	_iterator(new cartesian_iterator(generate_values(types, info)))
{}

binding_iterator::~binding_iterator() {
	delete _iterator;
}

std::vector<const ObjectIdxVector*> binding_iterator::generate_values(const Signature& types, const ProblemInfo& info) {
	std::vector<const ObjectIdxVector*> values;
	
	for (TypeIdx type:types) {
		if (type == INVALID_TYPE) {
			values.push_back(&NIL);
			_valid.push_back(false);
		} else {
			values.push_back(&info.getTypeObjects(type));
			_valid.push_back(true);
		}
	}
	
	return values;
}

unsigned binding_iterator::num_bindings() const { return _iterator->size(); }

//! Generates a fresh binding each time it is invoked
Binding binding_iterator::operator*() const { 
	return Binding(*(*_iterator), _valid);
}

const binding_iterator& binding_iterator::operator++() {
	++(*_iterator);
	return *this;
}
const binding_iterator binding_iterator::operator++(int) {binding_iterator tmp(*this); operator++(); return tmp;}

bool binding_iterator::ended() const { return _iterator->ended(); }

} } // namespaces
