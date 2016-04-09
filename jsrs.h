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

#ifndef JSTP_CPP_JSRS_H
#define JSTP_CPP_JSRS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <utility>

namespace jstp {

class Record {

  typedef std::string string;
  typedef std::vector<Record> array;
  typedef std::map<std::string, Record> object;
  typedef std::vector<const string *> object_keys;

 public:

  enum Type {
    UNDEFINED, NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT
  };

  Record();                          // UNDEFINED
  Record(std::nullptr_t);            // NUL

  Record(double val);                // NUMBER
  Record(bool val);                  // BOOL

  Record(const string &val);         // STRING
  Record(const char *value);         // STRING
  Record(string &&val);              // STRING

  Record(const array &values);       // ARRAY
  Record(array &&values);            // ARRAY

  Record(const object &values);      // OBJECT
  Record(const object &values, const object_keys &keys);      // ORDERED_OBJECT
  Record(object &&values);           // OBJECT
  Record(object &&values, object_keys &&keys);      // ORDERED_OBJECT



  Type type() const;

  bool is_undefined() const { return type() == UNDEFINED; };
  bool is_null() const { return type() == NUL; };
  bool is_bool() const { return type() == BOOL; };
  bool is_number() const { return type() == NUMBER; };
  bool is_string() const { return type() == STRING; };
  bool is_array() const { return type() == ARRAY; };
  bool is_object() const { return type() == OBJECT; };

  /**
   * Returns the enclosed value if this is a boolean, false otherwise
   */
  bool bool_value() const;

  /**
   * Returns the enclosed value if this is a number, 0 otherwise
   */
  double number_value() const;

  /**
   * Returns the enclosed value if this is a string, '' otherwise
   */
  const string &string_value() const;

  /**
   * Return the enclosed std::vector if this is an array, or an empty vector otherwise.
   */
  const array &array_items() const;
  /**
   * Return the enclosed std::map if this is an object, or an empty map otherwise.
   */
  const object &object_items() const;
  const object_keys &get_object_keys() const;

  /**
   * Return a reference to arr[i] if this is an array, UNDEFINED JSTP otherwise.
   */
  const Record &operator[](std::size_t i) const;

  /*
   * Return a reference to obj[key] if this is an object, UNDEFINED JSTP otherwise.
   */
  const Record &operator[](const string &key) const;

  /**
   * Serializator
   */
  string stringify() const;

  /*
   * Parser of a Record Serialization
   */
  static Record parse(const string &in, string &err);

  bool operator==(const Record &rhs) const;
  bool operator<(const Record &rhs) const;
  bool operator!=(const Record &rhs) const;
  bool operator<=(const Record &rhs) const;
  bool operator>(const Record &rhs) const;
  bool operator>=(const Record &rhs) const;


 private:

  /**
   * Inner class for storing values
   * Behaviour of methods is similar to Record one`s
   */
  class JS_value {
    friend class Record;
   protected:
    virtual Type type() const = 0;

    virtual bool equals(const JS_value *other) const = 0;
    virtual bool less(const JS_value *other) const = 0;

    virtual void dump(string &out) const = 0;
    virtual bool bool_value() const;
    virtual double number_value() const;
    virtual const string &string_value() const;
    virtual const array &array_items() const;
    virtual const object &object_items() const;
    virtual const object_keys &get_object_keys() const;

    virtual const Record &operator[](std::size_t i) const;
    virtual const Record &operator[](const std::string &key) const;

    virtual ~JS_value() { }
  };

  std::shared_ptr<JS_value> value;

  class JS_number: public JS_value {
   public:
    JS_number(double value);

    Type type() const;

    bool equals(const JS_value *other) const;
    bool less(const JS_value *other) const;

    void dump(string &out) const;

    double number_value() const;
   private:
    const double value;
  };

  class JS_boolean: public JS_value {
   public:
    JS_boolean(bool value);

    Type type() const;

    bool equals(const JS_value *other) const;
    bool less(const JS_value *other) const;

    void dump(string &out) const;

    bool bool_value() const;
   private:
    const bool value;
  };

  class JS_string: public JS_value {
   public:
    JS_string(const string &value);
    JS_string(const char *value);
    JS_string(string &&value);

    Type type() const;

    bool equals(const JS_value *other) const;
    bool less(const JS_value *other) const;

    void dump(string &out) const;

    const string &string_value() const;
   private:
    const string value;
  };

  class JS_array: public JS_value {
   public:
    JS_array(const array &values);
    JS_array(array &&values);

    Type type() const;

    bool equals(const JS_value *other) const;
    bool less(const JS_value *other) const;

    void dump(string &out) const;

    const array &array_items() const;
    const Record &operator[](std::size_t i) const;
   private:
    const array values;
  };

  class JS_object: public JS_value {
   public:
    JS_object(const object &value);
    JS_object(object &&value);
    JS_object(const object &value, const object_keys &keys);
    JS_object(object &&value, object_keys &&keys);

    Type type() const;

    bool equals(const JS_value *other) const;
    bool less(const JS_value *other) const;

    void dump(string &out) const;

    const object &object_items() const;
    const object_keys &get_object_keys() const;
    const Record &operator[](const std::string &key) const;
   private:
    object values;
    object_keys keys;
  };

  class JS_undefined: public JS_value {
    Type type() const;

    bool equals(const JS_value *other) const;
    bool less(const JS_value *other) const;

    void dump(string &out) const;
  };

  class JS_null: public JS_value {
    Type type() const;

    bool equals(const JS_value *other) const;
    bool less(const JS_value *other) const;

    void dump(string &out) const;
  };

};

}


#endif //JSTP_CPP_JSRS_H
