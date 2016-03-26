//
// Created by nechaido on 25.03.16.
//

#ifndef JSTP_CPP_JSRS_H
#define JSTP_CPP_JSRS_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace jstp {

class JSRS {

  typedef std::string string;
  typedef std::vector<JSRS> array;
  typedef std::map<std::string, JSRS> object;

 public:

  enum Type {
    UNDEFINED, NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT
  };

  JSRS();                          // UNDEFINED
  JSRS(std::nullptr_t);            // NUL

  JSRS(double val);                // NUMBER
  JSRS(bool val);                  // BOOL

  JSRS(const string &val);         // STRING
  JSRS(const char *value);         // STRING

  JSRS(const array &values);       // ARRAY

  JSRS(const object &values);      // OBJECT



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


  /**
   * Return a reference to arr[i] if this is an array, UNDEFINED JSTP otherwise.
   */
  const JSRS &operator[](size_t i) const;

  /*
   * Return a reference to obj[key] if this is an object, UNDEFINED JSTP otherwise.
   */
  const JSRS &operator[](const string &key) const;

  /**
   * Serializator
   */
  string dump() const;

  /*
   * Parser of a Record Serialization
   */
  static JSRS parse(const string &in, string &err);

  bool operator==(const JSRS &rhs) const;
  bool operator<(const JSRS &rhs) const;
  bool operator!=(const JSRS &rhs) const;
  bool operator<=(const JSRS &rhs) const;
  bool operator>(const JSRS &rhs) const;
  bool operator>=(const JSRS &rhs) const;


 private:

  std::shared_ptr<JS_value> value;

  /**
   * Inner class for storing values
   * Behaviour of methods is similar to JSRS one`s
   */
  class JS_value {
    friend class JSRS;
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

    virtual const JSRS &operator[](size_t i) const;
    virtual const JSRS &operator[](const std::string &key) const;

    virtual ~JS_value() { }
  };

  class JS_number : JS_value {
   public:
    JS_number(double value);
   private:
    double value;
  };

  class JS_boolean : JS_value {
   public:
    JS_boolean(bool value);
   private:
    bool value;
  };

  class JS_string : JS_value {
   public:
    JS_string(const string &value);
   private:
    string value;
  };

  class JS_array : JS_value {
   public:
    JS_array(const array &values);
   private:
    array values;
  };

  class JS_object : JS_value {
   public:
    JS_object(const object &value);
   private:
    object values;
  };

  class JS_undefined : JS_value { };

  class JS_null : JS_value { };

};

}


#endif //JSTP_CPP_JSRS_H
