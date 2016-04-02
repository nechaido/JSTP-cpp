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

namespace jstp {

// Record implementation

Record::Record() {
  value = std::make_shared<JS_undefined>();
}

Record::Record(std::nullptr_t) {
  value = std::make_shared<JS_null>();
}

Record::Record(double val) {
  value = std::make_shared<JS_number>(val);
}

Record::Record(bool val) {
  value = std::make_shared<JS_boolean>(val);
}

Record::Record(const string &val) {
  value = std::make_shared<JS_string>(val);
}

Record::Record(const char *value) {
  this->value = std::make_shared<JS_string>(value);
}

Record::Record(const array &values) {
  value = std::make_shared<JS_array>(values);
}

Record::Record(const object &values) {
  object_keys keys;
  for (auto i = values.begin(); i != values.end(); ++i) {
    keys.push_back(&i->first);
  }
  value = std::make_shared<JS_object>(values, keys);
}

Record::Record(const object &values, const object_keys &keys) {
  value = std::make_shared<JS_object>(values, keys);
}

Record::Type Record::type() const {
  return value->type();
}

bool Record::bool_value() const {
  return value->bool_value();
}

double Record::number_value() const {
  return value->number_value();
}

const Record::string &Record::string_value() const {
  return value->string_value();
}

const Record::array &Record::array_items() const {
  return value->array_items();
}

const Record::object &Record::object_items() const {
  return value->object_items();
}

const Record::object_keys &Record::get_object_keys() const {
  return value->get_object_keys();
}

const Record &Record::operator[](size_t i) const {
  return value->operator[](i);
}

const Record &Record::operator[](const string &key) const {
  return value->operator[](key);
}

Record::string Record::stringify() const {
  string result;
  value->dump(result);
  return result;
}

bool Record::operator==(const Record &rhs) const {
  return this->value->equals(rhs.value.get());
}

bool Record::operator<(const Record &rhs) const {
  return this->value->less(rhs.value.get());
}

bool Record::operator!=(const Record &rhs) const {
  return !this->value->equals(rhs.value.get());
}

bool Record::operator<=(const Record &rhs) const {
  return this->value->equals(rhs.value.get()) || this->value->less(rhs.value.get());
}

bool Record::operator>(const Record &rhs) const {
  return !this->value->equals(rhs.value.get()) && !this->value->less(rhs.value.get());
}

bool Record::operator>=(const Record &rhs) const {
  return this->value->equals(rhs.value.get()) || !this->value->less(rhs.value.get());
}

const std::string *prepare_string(const std::string &str) {
  std::ostringstream writer;
  bool string_mode = false;
  enum COMMENT_MODE { kDisabled = 0, kOneline, kMultiline } comment_mode = kDisabled;
  for (auto i = str.begin(); i != str.end(); ++i) {
    if ((*i == '\"' || *i == '\'') && (i == str.begin() || *(i - 1) != '\\')) {
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
      if ((comment_mode == kOneline1 && (*i == '\n' || *i == '\r')) ||
          (comment_mode == kMultiline && *(i - 1) == '*' && *i == '/')) {
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
                                    Record::Type &type) {
  std::string::const_iterator result;
  auto i = begin;
  bool is_found = false;
  const std::string *t;
  switch (*(i++)) {
    case ',':
    case ']':
      type = Record::Type::UNDEFINED;
      return begin + 1;
    case '{':
      type = Record::Type::OBJECT;
      break;
    case '[':
      type = Record::Type::ARRAY;
      break;
    case '\"':
    case '\'':
      type = Record::Type::STRING;
      break;
    case 't':
    case 'f':
      type = Record::Type::BOOL;
      break;
    case 'n':
      type = Record::Type::NUL;
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
      type = Record::Type::UNDEFINED;
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
        type = Record::Type::NUMBER;
      } else {
        return begin;
      }
  }

  int p = 1;

  for (; i != end && !is_found; ++i) {
    switch (type) {
      case Record::Type::OBJECT:
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
      case Record::Type::ARRAY:
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
      case Record::Type::STRING:
        if ((*i == '\"' || *i == '\'') && *(i - 1) != '\\') {
          result = i + 1;
          is_found = true;
        }
        break;
      case Record::Type::NUMBER:
        if (!isdigit(*i) && *i != '.') {
          result = i;
          is_found = true;
        }
        break;
      case Record::Type::BOOL:
        if (*i == 'e') {
          result = i + 1;
          is_found = true;
        }
        break;
    }
  }

  if (type == Record::Type::NUMBER && !is_found) {
    result = end;
  }

  return result;
}

const Record *parse_bool(const std::string::const_iterator &begin,
                         const std::string::const_iterator &end,
                         std::string *&err) {
  const std::string *str = get_string(begin, end);
  Record *result;
  if (*str == "true") {
    result = new Record(true);
  } else if (*str == "false") {
    result = new Record(false);
  } else {
    err = new std::string("Invalid format: expected boolean");
    result = new Record();
  }
  delete str;
  return result;
}

const Record *parse_number(const std::string::const_iterator &begin,
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
    return new Record(resulting_value);
  } else {
    return new Record();
  }
}

const Record *parse_string(const std::string::const_iterator &begin,
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
    return new Record(writer.str());
  } else {
    return new Record();
  }
}
const Record
    *parse_array(const std::string::const_iterator &begin, const std::string::const_iterator &end, std::string *&err);

const Record *parse_object(const std::string::const_iterator &begin,
                           const std::string::const_iterator &end,
                           std::string *&err) {
  bool key_mode = true;
  std::string current_key;
  Record::Type current_type;
  std::map<std::string, Record> object;
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
        return new Record(object); // In case of empty object
      } else {
        if (!err) {
          err = new std::string("Invalid format in object: key is invalid");
        }
      }
    } else {
      auto e = get_end(i, end, current_type);
      if (e != i) {
        const Record *t;
        switch (current_type) {
          case Record::Type::OBJECT:
            t = parse_object(i, e, err);
            break;
          case Record::Type::ARRAY:
            t = parse_array(i, e, err);
            break;
          case Record::Type::STRING:
            t = parse_string(i, e, err);
            break;
          case Record::Type::NUMBER:
            t = parse_number(i, e, err);
            break;
          case Record::Type::BOOL:
            t = parse_bool(i, e, err);
            break;
          case Record::Type::UNDEFINED:
            t = new Record();
            break;
          case Record::Type::NUL:
            t = new Record(nullptr);
            break;
        }
        auto ins = object.insert(std::make_pair(current_key, Record(*t)));
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
    return new Record();
  }

  return new Record(object, keys);
}

const Record *parse_array(const std::string::const_iterator &begin,
                          const std::string::const_iterator &end,
                          std::string *&err) {
  Record::Type current_type;
  std::vector<Record> array;

  for (auto i = begin + 1; i != end && !err; ++i) {

    auto e = get_end(i, end, current_type);

    if (e != i) {
      if (*(e - 2) == '[' && *(e - 1) == ']') { // In case of empty array
        return new Record(array);
      }
      const Record *t;
      switch (current_type) {
        case Record::Type::OBJECT:
          t = parse_object(i, e, err);
          break;
        case Record::Type::ARRAY:
          t = parse_array(i, e, err);
          break;
        case Record::Type::STRING:
          t = parse_string(i, e, err);
          break;
        case Record::Type::NUMBER:
          t = parse_number(i, e, err);
          break;
        case Record::Type::BOOL:
          t = parse_bool(i, e, err);
          break;
        case Record::Type::UNDEFINED:
          t = new Record();
          if (e == i + 1) {
            e--;
          }
          break;
        case Record::Type::NUL:
          t = new Record(nullptr);
          break;
      }
      array.push_back(Record(*t));
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
    return new Record();
  }

  return new Record(array);
}

Record Record::parse(const string &in, string &err) {
  const string *to_parse = prepare_string(in);
  Type type;
  string *error = nullptr;
  auto end = get_end(to_parse->begin(), to_parse->end(), type);
  if (end != to_parse->end()) {
    err = "Invalid format";
    return Record();
  }
  Record result;
  const Record *t;
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
      t = new Record();
      break;
    case NUL:
      t = new Record(nullptr);
      break;
  }
  result = Record(*t);
  delete t;
  delete to_parse;
  if (error) {
    err = *error;
    delete error;
  }
  return result;
}

// end of Record implementation

// JS_value implementation

// Empty values
struct Empty {
  const std::string string;
  const std::vector<Record> vector;
  const std::map<std::string, Record> map;
  const std::vector<const std::string *> keys;
  const Record jsrs;
  Empty() { }
};

static const Empty &empty() {
  static const Empty e;
  return e;
}

bool Record::JS_value::bool_value() const { return false; }

double Record::JS_value::number_value() const { return 0.0; }

const Record::string &Record::JS_value::string_value() const { return empty().string; }

const Record::array &Record::JS_value::array_items() const { return empty().vector; }

const Record::object &Record::JS_value::object_items() const { return empty().map; }

const Record::object_keys &Record::JS_value::get_object_keys() const { return empty().keys; }

const Record &Record::JS_value::operator[](size_t i) const { return empty().jsrs; }

const Record &Record::JS_value::operator[](const std::string &key) const { return empty().jsrs; }
// end of JS_value implementation

// JS_number implementation

Record::JS_number::JS_number(double value) : value(value) { }

Record::Type Record::JS_number::type() const { return Record::Type::NUMBER; }

bool Record::JS_number::equals(const JS_value *other) const {
  return other->type() == this->type() && this->number_value() == other->number_value();
}

bool Record::JS_number::less(const JS_value *other) const {
  return other->type() == this->type() && this->number_value() < other->number_value();
}

void Record::JS_number::dump(string &out) const {
  std::ostringstream result;
  result << std::setprecision(std::numeric_limits<double>::digits10 + 1) << value;
  out = result.str();
}

double Record::JS_number::number_value() const { return value; }
// end of JS_number implementation

// JS_boolean implementation

Record::JS_boolean::JS_boolean(bool value) : value(value) { }

Record::Type Record::JS_boolean::type() const { return Record::Type::BOOL; }

bool Record::JS_boolean::equals(const JS_value *other) const {
  return other->type() == this->type() && this->bool_value() == other->bool_value();
}

bool Record::JS_boolean::less(const JS_value *other) const {
  return other->type() == this->type() && this->bool_value() < other->bool_value();
}

void Record::JS_boolean::dump(string &out) const { out = value ? "true" : "false"; }

bool Record::JS_boolean::bool_value() const { return value; }
// end of JS_boolean implementation

// JS_string implementation

Record::JS_string::JS_string(const string &value) : value(value) { }

Record::JS_string::JS_string(const char *value) : value(value) { }

Record::Type Record::JS_string::type() const { return Record::Type::STRING; }

bool Record::JS_string::equals(const JS_value *other) const {
  return other->type() == this->type() && this->string_value().compare(other->string_value()) == 0;
}

bool Record::JS_string::less(const JS_value *other) const {
  return other->type() == this->type() && this->string_value().compare(other->string_value()) < 0;
}

void Record::JS_string::dump(string &out) const {
  std::ostringstream result;
  result << "\"" << value << "\"";
  out = result.str();
}

const Record::string &Record::JS_string::string_value() const { return value; }
// end of JS_string implementation

// JS_array implementation

Record::JS_array::JS_array(const array &values) : values(values) { }

Record::Type Record::JS_array::type() const { return Record::Type::ARRAY; }

bool Record::JS_array::equals(const JS_value *other) const {
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

bool Record::JS_array::less(const JS_value *other) const {
  string t, o;
  this->dump(t);
  other->dump(o);
  return other->type() == this->type() && t.compare(o) < 0; // TODO(belochub): Implement better less for arrays
}

void Record::JS_array::dump(string &out) const {
  std::ostringstream result;
  result << '[';
  for (auto i = values.begin(); i != values.end(); ++i) {
    if (i != values.begin()) {
      result << ',';
    }
    if (!i->is_undefined()) {
      result << i->stringify();
    }
  }
  result << ']';
  out = result.str();
}

const Record::array &Record::JS_array::array_items() const { return values; }

const Record &Record::JS_array::operator[](size_t i) const { return values[i]; }
// end of JS_array implementation

// JS_object implementation

Record::JS_object::JS_object(const object &value) : values(value) { }

Record::JS_object::JS_object(const object &value, const object_keys &keys) {
  for (auto i = keys.begin(); i != keys.end(); ++i) {
    auto ins = values.insert(std::make_pair(**i, value.at(**i)));
    this->keys.push_back(&ins.first->first);
  }
}

Record::Type Record::JS_object::type() const { return Record::Type::OBJECT; }

bool Record::JS_object::equals(const JS_value *other) const {
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

bool Record::JS_object::less(const JS_value *other) const {
  string t, o;
  this->dump(t);
  other->dump(o);
  return other->type() == this->type() && t.compare(o) < 0; // TODO(belochub): Implement better less for objects
}

void Record::JS_object::dump(string &out) const {
  std::ostringstream result;
  result << '{';
  for (auto i = keys.begin(); i != keys.end(); ++i) {
    if (i != keys.begin()) {
      result << ',';
    }
    result << **i << ':' << values.at(**i).stringify();
  }
  result << '}';
  out = result.str();
}

const Record::object &Record::JS_object::object_items() const { return values; }

const Record::object_keys &Record::JS_object::get_object_keys() const { return keys; }

const Record &Record::JS_object::operator[](const std::string &key) const { return values.at(key); }
// end of JS_object implementation

// JS_undefined implementation

Record::Type Record::JS_undefined::type() const { return Record::Type::UNDEFINED; }

bool Record::JS_undefined::equals(const JS_value *other) const {
  return other->type() == this->type();
}

bool Record::JS_undefined::less(const JS_value *other) const {
  return false;
}

void Record::JS_undefined::dump(string &out) const { out = "undefined"; }
// end of JS_undefined implementation

// JS_null implementation

Record::Type Record::JS_null::type() const { return Record::Type::NUL; }

bool Record::JS_null::equals(const JS_value *other) const {
  return other->type() == this->type();
}

bool Record::JS_null::less(const JS_value *other) const {
  return false;
}

void Record::JS_null::dump(string &out) const { out = "null"; }
// end of JS_undefined implementation
}


