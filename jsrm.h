//
// Created by nechaido on 31.03.16.
//

#ifndef JSTP_CPP_JSRM_H
#define JSTP_CPP_JSRM_H

#include "jsrs.h"

namespace JSTP {

class JSRM: public JSRS {

 public:
  JSRM(const string &in, string &err);
  JSRS getJSRD(const string &in, string &err);

};

}


#endif //JSTP_CPP_JSRM_H
