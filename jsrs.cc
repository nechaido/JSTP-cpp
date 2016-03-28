/*
The MIT License (MIT)

Copyright (c) 2016 Dmytro Nechai, Nikolai Belochub

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include "jsrs.h"

#include <sstream>
#include <iterator>

namespace jstp {

// JSRS implementation

JSRS::JSRS() {
  value = std::make_shared<JS_undefined>();
}

JSRS::JSRS(std::nullptr_t) {
  value = std::make_shared<JS_null>();
}

JSRS::JSRS(double val) {
  value = std::make_shared<JS_number>(val);
}

JSRS::JSRS(bool val) {
  value = std::make_shared<JS_boolean>(val);
}

JSRS::JSRS(const string &val) {
  value = std::make_shared<JS_string>(val);
}

JSRS::JSRS(const char *value) {
  this->value = std::make_shared<JS_string>(value);
}

JSRS::JSRS(const array &values) {
  value = std::make_shared<JS_array>(values);
}

JSRS::JSRS(const object &values) {
  value = std::make_shared<JS_object>(values);
}

JSRS::Type JSRS::type() const {
  return value->type();
}

bool JSRS::bool_value() const {
  return value->bool_value();
}

double JSRS::number_value() const {
  return value->number_value();
}

const JSRS::string &JSRS::string_value() const {
  return value->string_value();
}

const JSRS::array &JSRS::array_items() const {
  return value->array_items();
}

const JSRS::object &JSRS::object_items() const {
  return value->object_items();
}

const JSRS &JSRS::operator[](size_t i) const {
  return value->operator[](i);
}

const JSRS &JSRS::operator[](const string &key) const {
  return value->operator[](key);
}

JSRS::string JSRS::dump() const {
  string result;
  value->dump(result);
  return result;
}

bool JSRS::operator==(const JSRS &rhs) const {
  return this->value->equals(rhs.value.get());
}

bool JSRS::operator<(const JSRS &rhs) const {
  return this->value->less(rhs.value.get());
}

bool JSRS::operator!=(const JSRS &rhs) const {
  return !this->value->equals(rhs.value.get());
}

bool JSRS::operator<=(const JSRS &rhs) const {
  return this->value->equals(rhs.value.get()) || this->value->less(rhs.value.get());
}

bool JSRS::operator>(const JSRS &rhs) const {
  return !this->value->equals(rhs.value.get()) && !this->value->less(rhs.value.get());
}

bool JSRS::operator>=(const JSRS &rhs) const {
  return this->value->equals(rhs.value.get()) || !this->value->less(rhs.value.get());
}

const std::string &prepare_string(const std::string &str) {
  std::ostringstream writer;
  bool string_mode = false;
  enum COMMENT_MODE { kDisabled = 0, kOneline, kMultiline } comment_mode = kDisabled;
  for (auto i = str.begin(); i != str.end(); ++i) {
    if (*i == '\'' || *i == '\"') {
      string_mode = !string_mode;
    }
    if (!string_mode) {
      if (!comment_mode && *i == '/') {
        switch (*(i + 1)) {
          case '/':
            comment_mode = kOneline;
            break;
          case '*':
            comment_mode = kMultiline;
            break;
        }
      }
      if (!comment_mode && !isspace(*i)) {
        writer << *i;
      }
      if ((comment_mode == 1 && (*i == '\n' || *i == '\r')) ||
          (comment_mode == 2 && *(i - 1) == '*' && *i == '/')) {
        comment_mode = kDisabled;
      }
    } else {
      writer << *i;
    }

  }
  return *new std::string(writer.str());
}

const std::string &get_string(const std::string::iterator &begin, const std::string::iterator &end) {
  std::ostringstream writer;
  std::copy(begin, end, std::ostream_iterator<char>(writer, ""));
  return *new std::string(writer.str());
}

std::string::iterator get_end(const std::string::iterator &begin, const std::string::iterator &end, JSRS::Type &type) {
  std::string::iterator result;
  auto i = begin;
  bool is_found = false;
  switch (*(i++)) {
    case ',':
    case ']':
      type = JSRS::Type::UNDEFINED;
      return begin + 1;
    case '{':
      type = JSRS::Type::OBJECT;
      break;
    case '[':
      type = JSRS::Type::ARRAY;
      break;
    case '\"':
    case '\'':
      type = JSRS::Type::STRING;
      break;
    case 't':
    case 'f':
      type = JSRS::Type::BOOL;
      break;
    case 'n':
      type = JSRS::Type::NUL;
      is_found = true;
      if (begin + 4 < end && get_string(begin, begin + 4) == "null") {
        result = begin + 4;
      } else {
        result = begin;
      }
      break;
    case 'u':
      type = JSRS::Type::UNDEFINED;
      is_found = true;
      if (begin + 9 < end && get_string(begin, begin + 9) == "undefined") {
        result = begin + 9;
      } else {
        result = begin;
      }
      break;
    default:
      if (std::isdigit(*begin) || *begin == '.' || *begin == '+' || *begin == '-') {
        type = JSRS::Type::NUMBER;
      } else {
        return begin;
      }
  }

  int p = 1;

  for (; i != end && !is_found; ++i) {
    switch (type) {
      case JSRS::Type::OBJECT:
        if (*i == '{') {
          p++;
        }
        if (*i == '}') {
          p--;
          if (p == 0) {
            result = i + 1;
            is_found = true;
          }
        }
        break;
      case JSRS::Type::ARRAY:
        if (*i == '[') {
          p++;
        }
        if (*i == ']') {
          p--;
          if (p == 0) {
            result = i + 1;
            is_found = true;
          }
        }
        break;
      case JSRS::Type::STRING:
        if (*i == '\"' || *i == '\'') {
          result = i + 1;
          is_found = true;
        }
        break;
      case JSRS::Type::NUMBER:
        if (!std::isdigit(*i) && *i != '.') {
          result = i;
          is_found = true;
        }
        break;
      case JSRS::Type::BOOL:
        if (*i == 'e') {
          result = i + 1;
          is_found = true;
        }
        break;
    }
  }

  return result;
}

const JSRS &parse_bool(const std::string::iterator &begin, const std::string::iterator &end, std::string *&err) {
  std::string result = get_string(begin, end);
  if (result == "true") {
    return *new JSRS(true);
  } else if (result == "false") {
    return *new JSRS(false);
  } else {
    err = new std::string("Invalid format: expected boolean");
    return *new JSRS();
  }
}

const JSRS &parse_number(const std::string::iterator &begin, const std::string::iterator &end, std::string *&err) {
  std::ostringstream writer;
  std::copy(begin, end, std::ostream_iterator<char>(writer, ""));
  std::string result = writer.str();
  double resulting_value;
  try {
    resulting_value = std::stod(result);
  } catch (std::exception &e) {
    err = new std::string("Invalid format: number exception ");
    *err += e.what();
  }
  if (!err) {
    return *new JSRS(resulting_value);
  } else {
    return *new JSRS();
  }
}

const JSRS &parse_string(const std::string::iterator &begin, const std::string::iterator &end, std::string *&err) {
  std::ostringstream writer;
  try {
    std::copy(begin + 1, end - 1, std::ostream_iterator<char>(writer, ""));
  } catch (std::exception &e) {
    err = new std::string("Error while parsing string ");
    *err += e.what();
  }
  if (!err) {
    return *new JSRS(writer.str());
  } else {
    return *new JSRS();
  }
}
const JSRS &parse_array(const std::string::iterator &begin, const std::string::iterator &end, std::string *&err);

const JSRS &parse_object(const std::string::iterator &begin, const std::string::iterator &end, std::string *&err) {
  bool key_mode = true;
  std::string current_key;
  JSRS::Type current_type;
  std::map<std::string, JSRS> object;
  std::ostringstream writer;
  for (auto i = begin + 1; i != end && !err; ++i) {
    if (key_mode) {
      if (*i == ':') {
        key_mode = false;
        current_key = writer.str();
        writer.str("");
      } else if (isalnum(*i) || *i == '_') {
        writer << *i;
      } else if (*i == '}') {
        return *new JSRS(object); // In case of empty object
      } else {
        if (!err) {
          err = new std::string("Invalid format in object: key is invalid");
        }
      }
    } else {
      auto e = get_end(i, end, current_type);
      if (e != i) {
        switch (current_type) {
          case JSRS::Type::OBJECT:
            object[current_key] = parse_object(i, e, err);
            break;
          case JSRS::Type::ARRAY:
            object[current_key] = parse_array(i, e, err);
            break;
          case JSRS::Type::STRING:
            object[current_key] = parse_string(i, e, err);
            break;
          case JSRS::Type::NUMBER:
            object[current_key] = parse_number(i, e, err);
            break;
          case JSRS::Type::BOOL:
            object[current_key] = parse_bool(i, e, err);
            break;
          case JSRS::Type::UNDEFINED:
            object[current_key] = JSRS();
            break;
          case JSRS::Type::NUL:
            object[current_key] = JSRS(nullptr);
            break;
        }
        i = e;
        if (*i != ',' && *i != '}') {
          if (!err) {
            err = new std::string("Invalid format in object: missed semicolon");
          }
        }
        current_key = "";
        key_mode = true;
      } else {
        if (!err) {
          err = new std::string("Invalid format in object");
        }
      }
    }
  }
  if (err) {
    return *new JSRS();
  }
  JSRS *result = new JSRS(object);
  return *result;
}

const JSRS &parse_array(const std::string::iterator &begin, const std::string::iterator &end, std::string *&err) {
  JSRS::Type current_type;
  std::vector<JSRS> array;

  for (auto i = begin + 1; i != end && !err; ++i) {

    auto e = get_end(i, end, current_type);

    if (e != i) {
      if (*(e - 2) == '[' && *(e - 1) == ']') { // In case of empty array
        return *new JSRS(array);
      }
      switch (current_type) {
        case JSRS::Type::OBJECT:
          array.push_back(parse_object(i, e, err));
          break;
        case JSRS::Type::ARRAY:
          array.push_back(parse_array(i, e, err));
          break;
        case JSRS::Type::STRING:
          array.push_back(parse_string(i, e, err));
          break;
        case JSRS::Type::NUMBER:
          array.push_back(parse_number(i, e, err));
          break;
        case JSRS::Type::BOOL:
          array.push_back(parse_bool(i, e, err));
          break;
        case JSRS::Type::UNDEFINED:
          array.push_back(JSRS());
          if (e == i + 1) {
            e--;
          }
          break;
        case JSRS::Type::NUL:
          array.push_back(JSRS(nullptr));
          break;
      }
      i = e;
      if (*i != ',' && *i != ']') {
        if (!err) {
          err = new std::string("Invalid format in array: missed semicolon");
        }
      }
    } else {
      if (!err) {
        err = new std::string("Invalid format in array");
      }
    }
  }
  if (err) {
    return *new JSRS();
  }
  JSRS *result = new JSRS(array);
  return *result;
}

JSRS JSRS::parse(const string &in, string &err) {
  string to_parse = prepare_string(in);
  Type type;
  string *error = nullptr;
  auto end = get_end(to_parse.begin(), to_parse.end(), type);
  if (end != to_parse.end()) {
    err = "Invalid format";
    return *new JSRS();
  }
  JSRS result;
  switch (type) {
    case ARRAY:
      result = JSRS(parse_array(to_parse.begin(), to_parse.end(), error));
      break;
    case BOOL:
      result = JSRS(parse_bool(to_parse.begin(), to_parse.end(), error));
      break;
    case OBJECT:
      result = JSRS(parse_object(to_parse.begin(), to_parse.end(), error));
      break;
    case NUMBER:
      result = JSRS(parse_number(to_parse.begin(), to_parse.end(), error));
      break;
    case STRING:
      result = JSRS(parse_string(to_parse.begin(), to_parse.end(), error));
      break;
    case UNDEFINED:
      result = JSRS();
      break;
    case NUL:
      result = JSRS(nullptr);
      break;
  }
  if (error) {
    err = *error;
  }
  return result;
}

// end of JSRS implementation

// JS_value implementation

bool JSRS::JS_value::bool_value() const { return false; }

double JSRS::JS_value::number_value() const { return 0.0; }

const JSRS::string &JSRS::JS_value::string_value() const { return *new string(""); }

const JSRS::array &JSRS::JS_value::array_items() const { return *new array(); }

const JSRS::object &JSRS::JS_value::object_items() const { return *new object(); }

const JSRS &JSRS::JS_value::operator[](size_t i) const { return *new JSRS(); }

const JSRS &JSRS::JS_value::operator[](const std::string &key) const { return *new JSRS(); }
// end of JS_value implementation

// JS_number implementation

JSRS::JS_number::JS_number(double value) : value(value) { }

JSRS::Type JSRS::JS_number::type() const { return JSRS::Type::NUMBER; }

bool JSRS::JS_number::equals(const JS_value *other) const {
  return other->type() == this->type() && this->number_value() == other->number_value();
}

bool JSRS::JS_number::less(const JS_value *other) const {
  return other->type() == this->type() && this->number_value() < other->number_value();
}

void JSRS::JS_number::dump(string &out) const {
  std::ostringstream result;
  result << value;
  out = result.str();
}

double JSRS::JS_number::number_value() const { return value; }
// end of JS_number implementation

// JS_boolean implementation

JSRS::JS_boolean::JS_boolean(bool value) : value(value) { }

JSRS::Type JSRS::JS_boolean::type() const { return JSRS::Type::BOOL; }

bool JSRS::JS_boolean::equals(const JS_value *other) const {
  return other->type() == this->type() && this->bool_value() == other->bool_value();
}

bool JSRS::JS_boolean::less(const JS_value *other) const {
  return other->type() == this->type() && this->bool_value() < other->bool_value();
}

void JSRS::JS_boolean::dump(string &out) const { out = value ? "true" : "false"; }

bool JSRS::JS_boolean::bool_value() const { return value; }
// end of JS_boolean implementation

// JS_string implementation

JSRS::JS_string::JS_string(const string &value) : value(value) { }

JSRS::JS_string::JS_string(const char *value) : value(value) { }

JSRS::Type JSRS::JS_string::type() const { return JSRS::Type::STRING; }

bool JSRS::JS_string::equals(const JS_value *other) const {
  return other->type() == this->type() && this->string_value().compare(other->string_value()) == 0;
}

bool JSRS::JS_string::less(const JS_value *other) const {
  return other->type() == this->type() && this->string_value().compare(other->string_value()) < 0;
}

void JSRS::JS_string::dump(string &out) const {
  std::ostringstream result;
  result << "\"" << value << "\"";
  out = result.str();
}

const JSRS::string &JSRS::JS_string::string_value() const { return value; }
// end of JS_string implementation

// JS_array implementation

JSRS::JS_array::JS_array(const array &values) : values(values) { }

JSRS::Type JSRS::JS_array::type() const { return JSRS::Type::ARRAY; }

bool JSRS::JS_array::equals(const JS_value *other) const {
  bool result = other->type() == this->type() && this->array_items().size() == other->array_items().size();
  if (result) {
    for (size_t i = 0; i < this->array_items().size(); i++) {
      if (this->array_items()[i] != other->array_items()[i]) {
        result = false;
        break;
      }
    }
  }
  return result;
}

bool JSRS::JS_array::less(const JS_value *other) const {
  return false; // TODO(belochub): Implement less for arrays
  //return other->type() == this->type();
}

void JSRS::JS_array::dump(string &out) const {
  std::ostringstream result;
  result << '[';
  for (auto i = values.begin(); i != values.end(); ++i) {
    if (i != values.begin()) {
      result << ',';
    }
    result << i->dump();
  }
  result << ']';
  out = result.str();
}

const JSRS::array &JSRS::JS_array::array_items() const { return values; }

const JSRS &JSRS::JS_array::operator[](size_t i) const { return values[i]; }
// end of JS_array implementation

// JS_object implementation

JSRS::JS_object::JS_object(const object &value) : values(value) { }

JSRS::Type JSRS::JS_object::type() const { return JSRS::Type::OBJECT; }

bool JSRS::JS_object::equals(const JS_value *other) const {
  bool result = other->type() == this->type() && this->object_items().size() == other->object_items().size();
  if (result) {
    auto i = this->object_items().begin();
    auto j = other->object_items().begin();
    for (; i != this->object_items().end(); ++i, ++j) {
      if (i->first != j->first || i->second != j->second) {
        result = false;
        break;
      }
    }
  }
  return result;
}

bool JSRS::JS_object::less(const JS_value *other) const {
  return false; // TODO(belochub): Implement less for objects
  //return other->type() == this->type();
}

void JSRS::JS_object::dump(string &out) const {
  std::ostringstream result;
  result << '{';
  for (auto i = values.begin(); i != values.end(); ++i) {
    if (i != values.begin()) {
      result << ',';
    }
    result << i->first << ':' << i->second.dump();
  }
  result << '}';
  out = result.str();
}

const JSRS::object &JSRS::JS_object::object_items() const { return values; }

const JSRS &JSRS::JS_object::operator[](const std::string &key) const { return values.at(key); }
// end of JS_object implementation

// JS_undefined implementation

JSRS::Type JSRS::JS_undefined::type() const { return JSRS::Type::UNDEFINED; }

bool JSRS::JS_undefined::equals(const JS_value *other) const {
  return other->type() == this->type();
}

bool JSRS::JS_undefined::less(const JS_value *other) const {
  return false;
}

void JSRS::JS_undefined::dump(string &out) const { out = "undefined"; }
// end of JS_undefined implementation

// JS_null implementation

JSRS::Type JSRS::JS_null::type() const { return JSRS::Type::NUL; }

bool JSRS::JS_null::equals(const JS_value *other) const {
  return other->type() == this->type();
}

bool JSRS::JS_null::less(const JS_value *other) const {
  return false;
}

void JSRS::JS_null::dump(string &out) const { out = "null"; }
// end of JS_undefined implementation
}


