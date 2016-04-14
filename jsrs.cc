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
#include <cstring>

namespace jstp {

// Record implementation

Record::Record() : value(std::make_shared<JS_undefined>()) { }

Record::Record(std::nullptr_t) : value(std::make_shared<JS_null>()) { }

Record::Record(double val) : value(std::make_shared<JS_number>(val)) { }

Record::Record(bool val) : value(std::make_shared<JS_boolean>(val)) { }

Record::Record(const string &val) : value(std::make_shared<JS_string>(val)) { }

Record::Record(const char *value) : value(std::make_shared<JS_string>(value)) { }

Record::Record(string &&val) : value(std::make_shared<JS_string>(std::move(val))) { }

Record::Record(const array &values) : value(std::make_shared<JS_array>(values)) { }

Record::Record(array &&values) : value(std::make_shared<JS_array>(std::move(values))) { }

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

Record::Record(object &&values) : value(std::make_shared<JS_object>(std::move(values))) { }

Record::Record(object &&values, object_keys &&keys) : value(std::make_shared<JS_object>(std::move(values),
                                                                                        std::move(keys))) { }

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

const Record &Record::operator[](std::size_t i) const {
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

const char *prepare_string(const std::string &str) {
  char *result = new char[str.length() + 1];
  bool string_mode = false;
  enum { kDisabled = 0, kOneline, kMultiline } comment_mode = kDisabled;
  std::size_t j = 0;
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
        result[j++] = *i;
      }
      if ((comment_mode == kOneline && (*i == '\n' || *i == '\r')) ||
          (comment_mode == kMultiline && *(i - 1) == '*' && *i == '/')) {
        comment_mode = kDisabled;
      }
    } else {
      result[j++] = *i;
    }

  }
  result[j] = '\0';
  return result;
}

bool get_type(const char *begin, const char *end, Record::Type &type) {
  bool result = true;
  switch (*begin) {
    case ',':
    case ']':
      type = Record::Type::UNDEFINED;
      break;
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
      if (begin + 4 <= end) {
        result = (std::strncmp(begin, "null", 4) == 0);
      }
      break;
    case 'u':
      type = Record::Type::UNDEFINED;
      if (begin + 9 <= end) {
        result = (std::strncmp(begin, "undefined", 9) == 0);
      }
      break;
    default:
      result = false;
      if (isdigit(*begin) || *begin == '.' || *begin == '+' || *begin == '-') {
        type = Record::Type::NUMBER;
        result = true;
      }
  }
  return result;
}

// Parse functions
const Record *parse_undefined(const char *begin, const char *end, std::size_t &size, std::string *&err);
const Record *parse_null(const char *begin, const char *end, std::size_t &size, std::string *&err);
const Record *parse_bool(const char *begin, const char *end, std::size_t &size, std::string *&err);
const Record *parse_number(const char *begin, const char *end, std::size_t &size, std::string *&err);
const Record *parse_string(const char *begin, const char *end, std::size_t &size, std::string *&err);
const Record *parse_array(const char *begin, const char *end, std::size_t &size, std::string *&err);
const Record *parse_object(const char *begin, const char *end, std::size_t &size, std::string *&err);

const std::size_t kMaxKeyLength = 256;

const Record *(*parse_func[])(const char *, const char *, std::size_t &, std::string *&) =
    {&parse_undefined, &parse_null, &parse_bool, &parse_number, &parse_string, &parse_array, &parse_object};

const Record *parse_undefined(const char *begin, const char *end, std::size_t &size, std::string *&err) {
  const Record *result = new Record();
  if (*begin == ',' || *begin == ']') {
    size = 0;
  } else if (*begin == 'u') {
    size = 9;
  } else {
    err = new std::string("Invalid format of undefined value");
  }
  return result;
}

const Record *parse_null(const char *begin, const char *end, std::size_t &size, std::string *&err) {
  const Record *result = new Record(nullptr);
  size = 4;
  return result;
}

const Record *parse_bool(const char *begin, const char *end, std::size_t &size, std::string *&err) {
  Record *result;
  if (begin + 4 <= end && strncmp(begin, "true", 4) == 0) {
    result = new Record(true);
    size = 4;
  } else if (begin + 5 <= end && strncmp(begin, "false", 5) == 0) {
    result = new Record(false);
    size = 5;
  } else {
    if (!err) {
      err = new std::string("Invalid format: expected boolean");
    }
    result = new Record();
  }
  return result;
}

const Record *parse_number(const char *begin, const char *end, std::size_t &size, std::string *&err) {
  Record *result = new Record(atof(begin));
  size = end - begin;
  std::size_t i = 0;
  while (begin[i] != ',' && begin[i] != '}' && begin[i] != ']' && i < size) i++;
  size = i;
  return result;
}

const Record *parse_string(const char *begin, const char *end, std::size_t &size, std::string *&err) {
  size = end - begin;
  enum { kApostrophe = 0, kQMarks} string_mode = (*begin == '\'') ? kApostrophe : kQMarks;
  bool is_ended = false;
  for (std::size_t i = 1; i < size; ++i) {
    if ((string_mode == kQMarks && begin[i] == '\"') || (string_mode == kApostrophe && begin[i] == '\'') && begin[i - 1] != '\\') {
      is_ended = true;
      size = i - 1;
      break;
    }
  }
  if (!is_ended) {
    err = new std::string("Error while parsing string");
  }
  if (!err) {
    char *str = new char[size + 1];
    strncpy(str, begin + 1, size);
    str[size] = '\0';
    size += 2;
    const Record *result = new Record(str);
    delete[] str;
    return result;
  } else {
    return new Record();
  }
}

const Record *parse_object(const char *begin, const char *end, std::size_t &size, std::string *&err) {
  bool key_mode = true;
  size = end - begin;
  char current_key[kMaxKeyLength];
  std::size_t current_length = 0;
  Record::Type current_type;
  std::map<std::string, Record> object;
  std::vector<const std::string *> keys;
  const Record *t;
  for (std::size_t i = 1; i < size && !err; ++i) {
    if (key_mode) {
      if (begin[i] == ':') {
        key_mode = false;
        strncpy(current_key, begin + i - current_length, current_length);
        current_key[current_length] = '\0';
        current_length = 0;
      } else if (isalnum(begin[i]) || begin[i] == '_') {
        current_length++;
      } else if (begin[i] == '}') {
        return new Record(std::move(object)); // In case of empty object
      } else {
        if (!err) {
          err = new std::string("Invalid format in object: key is invalid");
        }
      }
    } else {
      bool valid = get_type(begin + i, end, current_type);
      if (valid) {
        t = (parse_func[current_type])(begin + i, end, current_length, err);
        auto ins = object.insert(std::move(std::make_pair(current_key, std::move(Record(*t)))));
        keys.push_back(&ins.first->first);
        delete t;
        i += current_length;
        if (begin[i] != ',' && begin[i] != '}') {
          if (!err) {
            err = new std::string("Invalid format in object: missed semicolon");
          }
        } else if (begin[i] == '}') {
          size = i + 1;
          break;
        }
        current_key[0] = '\0';
        current_length = 0;
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

  return new Record(std::move(object), std::move(keys));
}

const Record *parse_array(const char *begin, const char *end, std::size_t &size, std::string *&err) {
  Record::Type current_type;
  std::vector<Record> array;
  std::size_t current_length = 0;
  size = end - begin;
  if (*begin == '[' && *(begin + 1) == ']') { // In case of empty array
    return new Record(std::move(array));
  }
  const Record *t;
  for (std::size_t i = 1; i < size && !err; ++i) {
    bool valid = get_type(begin + i, end, current_type);
    if (valid) {
      t = (parse_func[current_type])(begin + i, end, current_length, err);
      array.push_back(std::move(Record(*t)));
      delete t;
      i += current_length;
      current_length = 0;
      if (begin[i] != ',' && begin[i] != ']') {
        if (!err) {
          err = new std::string("Invalid format in array: missed semicolon");
        }
      } else if (begin[i] == ']') {
        size = i + 1;
        break;
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

  return new Record(std::move(array));
}
// End of parse functions

Record Record::parse(const string &in, string &err) {
  const char *to_parse = prepare_string(in);
  Type type;
  string *error = nullptr;
  std::size_t size = strlen(to_parse);
  if (!get_type(to_parse, to_parse + size, type)) {
    err = "Invalid type";
    return Record();
  }
  Record result;
  const Record *t = (parse_func[type])(to_parse, to_parse + size, size, error);
  result = Record(*t);
  delete t;
  if (size != strlen(to_parse)) {
    error = new string("Invalid format");
  }
  delete[] to_parse;
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

const Record &Record::JS_value::operator[](std::size_t i) const { return empty().jsrs; }

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

Record::JS_string::JS_string(string &&value) : value(std::move(value)) { }

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

Record::JS_array::JS_array(array &&values) : values(std::move(values)) { }

Record::Type Record::JS_array::type() const { return Record::Type::ARRAY; }

bool Record::JS_array::equals(const JS_value *other) const {
  bool result = other->type() == this->type() && this->array_items().size() == other->array_items().size();
  if (result) {
    for (std::size_t i = 0; i < this->array_items().size(); i++) {
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

const Record &Record::JS_array::operator[](std::size_t i) const { return values[i]; }
// end of JS_array implementation

// JS_object implementation

Record::JS_object::JS_object(const object &value) : values(value) { }

Record::JS_object::JS_object(object &&value) : values(std::move(value)) { }

Record::JS_object::JS_object(const object &value, const object_keys &keys) {
  for (auto i = keys.begin(); i != keys.end(); ++i) {
    auto ins = values.insert(std::make_pair(**i, value.at(**i)));
    this->keys.push_back(&ins.first->first);
  }
}

Record::JS_object::JS_object(object &&value, object_keys &&keys) : values(std::move(value)), keys(std::move(keys)) { }

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


