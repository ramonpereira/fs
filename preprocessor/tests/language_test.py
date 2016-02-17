"""
 This tests the parsing of particular FSTRIPS language elements and its
 mapping into appropriate JSON data structures.
"""
import pytest

from .common import process_types, generate_objects, MockTask
# This should be imported from a custom-set PYTHONPATH containing the path to Fast Downward's PDDL parser
from pddl.conditions import Conjunction, Atom
from pddl.f_expression import FunctionalTerm
from language_processor import FormulaProcessor


@pytest.fixture(scope="module")
def bw_task():
    """ This effectively generates a single BW instance per test module and allows it to be reused in different tests
        simply by declaring that the test has a 'bw_task' parameter. See https://pytest.org/latest/fixture.html.
    """
    return generate_base_bw_instance()


def check_type(element, type_):
    assert element['type'] == type_


def unwrap_conjunction(element):
    check_type(element['conditions'], 'conjunction')
    return element['conditions']['elements']


def unwrap_predicate(element):
    check_type(element, 'atom')
    return element['symbol'], element['negated'], element['elements']


def unwrap_function(element):
    check_type(element, 'function')
    return element['symbol'], element['subterms']


def generate_base_bw_instance():
    types, supertypes = process_types([('block', 'object')])
    objects = generate_objects(dict(b1='block', b2='block'))
    return MockTask(types=types, supertypes=supertypes, objects=objects, fluent_symbols=['on', 'loc', 'val'])


def test_predicate(bw_task):
    atom = Atom('on', ['b1', 'b2'])
    processed = FormulaProcessor(bw_task, atom).process()

    elements = unwrap_conjunction(processed)  # We're also testing that the atom is wrapped as a conjunction
    assert len(elements) == 1

    symbol, negated, elements = unwrap_predicate(elements[0])
    assert symbol == 'on' and len(elements) == 2 and all(elem['type'] == 'constant' for elem in elements)


def test_conjunction(bw_task):
    conjunction = Conjunction([Atom('on', ['b1', 'b2']), Atom('on', ['b2', 'b1'])])
    processed = FormulaProcessor(bw_task, conjunction).process()

    elements = unwrap_conjunction(processed)
    assert len(elements) == 2

    symbol0, negated0, elements0 = unwrap_predicate(elements[0])
    symbol1, negated1, elements1 = unwrap_predicate(elements[1])
    assert symbol0 == symbol1 == 'on'
    assert len(elements0) == len(elements1) == 2 and all(elem['type'] == 'constant' for elem in elements1)


def test_function(bw_task):
    conjunction = Atom('=', [FunctionalTerm('loc', ['b1']), 'b2'])
    processed = FormulaProcessor(bw_task, conjunction).process()

    elements = unwrap_conjunction(processed)
    assert len(elements) == 1
    symbol, negated, elements = unwrap_predicate(elements[0])
    assert symbol == '=' and len(elements) == 2 and not negated

    symbol, subterms = unwrap_function(elements[0])  # loc(a)
    check_type(elements[1], 'constant')  # b
    assert symbol == 'loc' and check_type(elements[0], 'constant')


def test_relational_expression(bw_task):
    subterms = [FunctionalTerm('val', ['b1']), FunctionalTerm('val', ['b2'])]
    atom = Atom('<', subterms)
    processed = FormulaProcessor(bw_task, atom).process()

    elements = unwrap_conjunction(processed)  # We're also testing that the atom is wrapped as a conjunction
    assert len(elements) == 1

    symbol, negated, elements = unwrap_predicate(elements[0])
    assert symbol == '<' and len(elements) == 2
    _, _ = unwrap_function(elements[0])
    _, _ = unwrap_function(elements[1])


def test_existential():
    assert False

