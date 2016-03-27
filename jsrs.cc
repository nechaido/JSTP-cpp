//
// Created by nechaido on 25.03.16.
//

#include "jsrs.h"

namespace jstp {

// JS_value implementation

bool JSRS::JS_value::bool_value() const { return false; }

double JSRS::JS_value::number_value() const { return 0; }

const string& JSRS::JS_value::string_value() const { return string(""); }

const array& JSRS::JS_value::array_items() const { return array(); }

const object& JSRS::JS_value::object_items() const { return object(); }

const JSRS& JSRS::JS_value::operator[](size_t i) const { return JSRS(); }

const JSRS& JSRS::JS_value::operator[](const std::string &key) const { return JSRS(); }
// end of JS_value implementation

// JS_number implementation

JSRS::JS_number::JS_number(double value) : value(value) {  }

Type JSRS::JS_number::type() const { return JSRS::Type::NUMBER; }

bool JSRS::JS_number::equals(const JS_value *other) const {
  return other->type() == this->type() && other->number_value() == this->number_value();
}

bool JSRS::JS_number::less(const JS_value *other) const {
  return other->type() == this->type() && other->number_value() < this->number_value();
}

void JSRS::JS_number::dump(string &out) const { out = std::to_string(this->value); }

double JSRS::JS_number::number_value() const { return this->value; }
// end of JS_number implementation

// JS_boolean implementation

JSRS::JS_boolean::JS_boolean(bool value) : value(value) { }

Type JSRS::JS_boolean::type() const { return JSRS::Type::NUMBER; }

bool JSRS::JS_boolean::equals(const JS_value *other) const {
  return other->type() == this->type() && other->number_value() == this->number_value();
}

bool JSRS::JS_boolean::less(const JS_value *other) const {
  return other->type() == this->type() && other->number_value() < this->number_value();
}

void JSRS::JS_boolean::dump(string &out) const { out = std::to_string(this->value); }

bool JSRS::JS_boolean::bool_value() const { return value; }
// end of JS_boolean implementation
}


