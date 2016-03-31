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
#include <iomanip>
#include <limits>

namespace JSTP {

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
  object_keys keys;
  for (auto i = values.begin(); i != values.end(); ++i) {
    keys.push_back(&i->first);
  }
  value = std::make_shared<JS_object>(values, keys);
}

JSRS::JSRS(const object &values, const object_keys &keys) {
  value = std::make_shared<JS_object>(values, keys);
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

const JSRS::object_keys &JSRS::get_object_keys() const {
  return value->get_object_keys();
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

const std::string *prepare_string(const std::string &str) {
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

  return new std::string(writer.str());
}

const std::string *get_string(const std::string::const_iterator &begin, const std::string::const_iterator &end) {
  std::ostringstream writer;
  std::copy(begin, end, std::ostream_iterator<char>(writer, ""));
  return new std::string(writer.str());
}

std::string::const_iterator get_end(const std::string::const_iterator &begin,
                                    const std::string::const_iterator &end,
                                    JSRS::Type &type) {
  std::string::const_iterator result;
  auto i = begin;
  bool is_found = false;
  const std::string *t;
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
      t = get_string(begin, begin + 4);
      if (begin + 4 <= end && *t == "null") {
        result = begin + 4;
      } else {
        result = begin;
      }
      delete t;
      break;
    case 'u':
      type = JSRS::Type::UNDEFINED;
      is_found = true;
      t = get_string(begin, begin + 9);
      if (begin + 9 <= end && *t == "undefined") {
        result = begin + 9;
      } else {
        result = begin;
      }
      delete t;
      break;
    default:
      if (isdigit(*begin) || *begin == '.' || *begin == '+' || *begin == '-') {
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
        if (!isdigit(*i) && *i != '.') {
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

  if (type == JSRS::Type::NUMBER && !is_found) {
    result = end;
  }

  return result;
}

const JSRS *parse_bool(const std::string::const_iterator &begin,
                       const std::string::const_iterator &end,
                       std::string *&err) {
  const std::string *str = get_string(begin, end);
  JSRS *result;
  if (*str == "true") {
    result = new JSRS(true);
  } else if (*str == "false") {
    result = new JSRS(false);
  } else {
    err = new std::string("Invalid format: expected boolean");
    result = new JSRS();
  }
  delete str;
  return result;
}

const JSRS *parse_number(const std::string::const_iterator &begin,
                         const std::string::const_iterator &end,
                         std::string *&err) {
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
    return new JSRS(resulting_value);
  } else {
    return new JSRS();
  }
}

const JSRS *parse_string(const std::string::const_iterator &begin,
                         const std::string::const_iterator &end,
                         std::string *&err) {
  std::ostringstream writer;
  try {
    std::copy(begin + 1, end - 1, std::ostream_iterator<char>(writer, ""));
  } catch (std::exception &e) {
    err = new std::string("Error while parsing string ");
    *err += e.what();
  }
  if (!err) {
    return new JSRS(writer.str());
  } else {
    return new JSRS();
  }
}
const JSRS
    *parse_array(const std::string::const_iterator &begin, const std::string::const_iterator &end, std::string *&err);

const JSRS *parse_object(const std::string::const_iterator &begin,
                         const std::string::const_iterator &end,
                         std::string *&err) {
  bool key_mode = true;
  std::string current_key;
  JSRS::Type current_type;
  std::map<std::string, JSRS> object;
  std::vector<const std::string *> keys;
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
        return new JSRS(object); // In case of empty object
      } else {
        if (!err) {
          err = new std::string("Invalid format in object: key is invalid");
        }
      }
    } else {
      auto e = get_end(i, end, current_type);
      if (e != i) {
        const JSRS *t;
        switch (current_type) {
          case JSRS::Type::OBJECT:
            t = parse_object(i, e, err);
            break;
          case JSRS::Type::ARRAY:
            t = parse_array(i, e, err);
            break;
          case JSRS::Type::STRING:
            t = parse_string(i, e, err);
            break;
          case JSRS::Type::NUMBER:
            t = parse_number(i, e, err);
            break;
          case JSRS::Type::BOOL:
            t = parse_bool(i, e, err);
            break;
          case JSRS::Type::UNDEFINED:
            t = new JSRS();
            break;
          case JSRS::Type::NUL:
            t = new JSRS(nullptr);
            break;
        }
        auto ins = object.insert(std::make_pair(current_key, JSRS(*t)));
        keys.push_back(&ins.first->first);
        delete t;
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
    return new JSRS();
  }

  return new JSRS(object, keys);
}

const JSRS *parse_array(const std::string::const_iterator &begin,
                        const std::string::const_iterator &end,
                        std::string *&err) {
  JSRS::Type current_type;
  std::vector<JSRS> array;

  for (auto i = begin + 1; i != end && !err; ++i) {

    auto e = get_end(i, end, current_type);

    if (e != i) {
      if (*(e - 2) == '[' && *(e - 1) == ']') { // In case of empty array
        return new JSRS(array);
      }
      const JSRS *t;
      switch (current_type) {
        case JSRS::Type::OBJECT:
          t = parse_object(i, e, err);
          break;
        case JSRS::Type::ARRAY:
          t = parse_array(i, e, err);
          break;
        case JSRS::Type::STRING:
          t = parse_string(i, e, err);
          break;
        case JSRS::Type::NUMBER:
          t = parse_number(i, e, err);
          break;
        case JSRS::Type::BOOL:
          t = parse_bool(i, e, err);
          break;
        case JSRS::Type::UNDEFINED:
          t = new JSRS();
          if (e == i + 1) {
            e--;
          }
          break;
        case JSRS::Type::NUL:
          t = new JSRS(nullptr);
          break;
      }
      array.push_back(JSRS(*t));
      delete t;
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
    return new JSRS();
  }

  return new JSRS(array);
}

JSRS JSRS::parse(const string &in, string &err) {
  const string *to_parse = prepare_string(in);
  Type type;
  string *error = nullptr;
  auto end = get_end(to_parse->begin(), to_parse->end(), type);
  if (end != to_parse->end()) {
    err = "Invalid format";
    return JSRS();
  }
  JSRS result;
  const JSRS *t;
  switch (type) {
    case ARRAY:
      t = parse_array(to_parse->begin(), to_parse->end(), error);
      break;
    case BOOL:
      t = parse_bool(to_parse->begin(), to_parse->end(), error);
      break;
    case OBJECT:
      t = parse_object(to_parse->begin(), to_parse->end(), error);
      break;
    case NUMBER:
      t = parse_number(to_parse->begin(), to_parse->end(), error);
      break;
    case STRING:
      t = parse_string(to_parse->begin(), to_parse->end(), error);
      break;
    case UNDEFINED:
      t = new JSRS();
      break;
    case NUL:
      t = new JSRS(nullptr);
      break;
  }
  result = JSRS(*t);
  delete t;
  delete to_parse;
  if (error) {
    err = *error;
    delete error;
  }
  return result;
}
JSRS::JSRS(const object_keys &keys, const array &values) {
//TODO Make ir work, nigga
}



// end of JSRS implementation

// JS_value implementation

// Empty values
struct Empty {
  const std::string string;
  const std::vector<JSRS> vector;
  const std::map<std::string, JSRS> map;
  const std::vector<const std::string *> keys;
  const JSRS jsrs;
  Empty() { }
};

static const Empty &empty() {
  static const Empty e;
  return e;
}

bool JSRS::JS_value::bool_value() const { return false; }

double JSRS::JS_value::number_value() const { return 0.0; }

const JSRS::string &JSRS::JS_value::string_value() const { return empty().string; }

const JSRS::array &JSRS::JS_value::array_items() const { return empty().vector; }

const JSRS::object &JSRS::JS_value::object_items() const { return empty().map; }

const JSRS::object_keys &JSRS::JS_value::get_object_keys() const { return empty().keys; }

const JSRS &JSRS::JS_value::operator[](size_t i) const { return empty().jsrs; }

const JSRS &JSRS::JS_value::operator[](const std::string &key) const { return empty().jsrs; }
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
  result << std::setprecision(std::numeric_limits<double>::digits10 + 1) << value;
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
  string t, o;
  this->dump(t);
  other->dump(o);
  return other->type() == this->type() && t.compare(o) < 0; // TODO(belochub): Implement better less for arrays
}

void JSRS::JS_array::dump(string &out) const {
  std::ostringstream result;
  result << '[';
  for (auto i = values.begin(); i != values.end(); ++i) {
    if (i != values.begin()) {
      result << ',';
    }
    if (!i->is_undefined()) {
      result << i->dump();
    }
  }
  result << ']';
  out = result.str();
}

const JSRS::array &JSRS::JS_array::array_items() const { return values; }

const JSRS &JSRS::JS_array::operator[](size_t i) const { return values[i]; }
// end of JS_array implementation

// JS_object implementation

JSRS::JS_object::JS_object(const object &value) : values(value) { }

JSRS::JS_object::JS_object(const object &value, const object_keys &keys) {
  for (auto i = keys.begin(); i != keys.end(); ++i) {
    auto ins = values.insert(std::make_pair(**i, value.at(**i)));
    this->keys.push_back(&ins.first->first);
  }
}

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
  string t, o;
  this->dump(t);
  other->dump(o);
  return other->type() == this->type() && t.compare(o) < 0; // TODO(belochub): Implement better less for objects
}

void JSRS::JS_object::dump(string &out) const {
  std::ostringstream result;
  result << '{';
  for (auto i = keys.begin(); i != keys.end(); ++i) {
    if (i != keys.begin()) {
      result << ',';
    }
    result << **i << ':' << values.at(**i).dump();
  }
  result << '}';
  out = result.str();
}

const JSRS::object &JSRS::JS_object::object_items() const { return values; }

const JSRS::object_keys &JSRS::JS_object::get_object_keys() const { return keys; }

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


