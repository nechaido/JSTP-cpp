//
// Created by nechaido on 31.03.16.
//

#include "jsrm.h"
namespace JSTP {

JSRM::JSRM(const JSRS::string &in, JSRS::string &err) : JSRS(JSRS::parse(in, err)) {
}

JSRS JSRM::getJSRD(const string &in, string &err) {
  JSRS jsrd = parse(in, err);
  JSRS jsrs = JSRS("{}");
  object_keys keys = get_object_keys();
  jsrs = JSRS(keys, jsrd.array_items());
  return jsrs;
}

}