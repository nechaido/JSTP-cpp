//
// Created by nechaido on 31.03.16.
//

#include "jsrm.h"
namespace JSTP {

JSRM::JSRM(const JSRS::string &in, JSRS::string &err) : JSRS(JSRS::parse(in, err)) {
}

JSRS JSRM::getJSRS(const string &in, string &err) {
  JSRS jsrd = JSRS::parse(in, err);
  JSRS jsrs = parse(this, jsrd.array_items(), err);
  return jsrs;
}

JSRS JSRM::parse(const JSRS *jsrm, const array &values, string &err) {
  object_keys keys = jsrm->get_object_keys();
  object o;
  int i = 0;
  for (auto key : keys){
    JSRS val;
    if (values[i].type() == OBJECT)
      err = "Invalid JSRD\n";
    else {
      if (values[i].type() == ARRAY) val = parse(&(jsrm->operator[](*key)), values[i].array_items(), err);
      else val = values[i];
      o.insert(std::make_pair(*key, val));
    }
  }
  return JSRS(o);
}

}

