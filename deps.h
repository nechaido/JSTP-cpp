//
// Created by nechaido on 29.03.16.
//

#include "jsrs.h"

class testData {
 public:
  static std::vector<std::string> validArray(){
    std::vector<std::string> arr;
    arr.push_back("{\n"
                      "  name:\"Marcus Aurelius\",\n"
                      "  passport:\"AE127095\",\n"
                      "  birth:{\n"
                      "    date:\"1990-02-15\",\n"
                      "    place:\"Rome\"\n"
                      "  },\n"
                      "  contacts:{\n"
                      "    email:\"marcus@aurelius.it\",\n"
                      "    phone:\"+380505551234\",\n"
                      "    address:{\n"
                      "      country:\"Ukraine\",\n"
                      "      city:\"Kiev\",\n"
                      "      zip:\"03056\",\n"
                      "      street:\"Pobedy\",\n"
                      "      building:\"37\",\n"
                      "      floor:\"1\",\n"
                      "      room:\"158\"\n"
                      "    }\n"
                      "  }\n"
                      "}");
    arr.push_back("{\n"
                      "  obj:{\n"
                      "    obj:{\n"
                      "      obj:{\n"
                      "        obj:{\n"
                      "          obj:{\n"
                      "            obj:{\n"
                      "              obj:{\n"
                      "                obj:{\n"
                      "                  obj:{\n"
                      "                    obj:{\n"
                      "                      obj:{\n"
                      "                        obj:{\n"
                      "                          obj:{\n"
                      "                            obj:{\n"
                      "                              name: \"name\"\n"
                      "                            },\n"
                      "                              name: \"name\"\n"
                      "                          },\n"
                      "                            name: \"name\"\n"
                      "                        },\n"
                      "                          name: \"name\"\n"
                      "                      },\n"
                      "                        name: \"name\"\n"
                      "                    },\n"
                      "                      name: \"name\"\n"
                      "                  },\n"
                      "                    name: \"name\"\n"
                      "                },\n"
                      "                  name: \"name\"\n"
                      "              },\n"
                      "              name: \"name\"\n"
                      "            },\n"
                      "            name: \"name\"\n"
                      "          },\n"
                      "          name: \"name\"\n"
                      "        },\n"
                      "        name: \"name\"\n"
                      "      },\n"
                      "      name: \"name\"\n"
                      "    },\n"
                      "    name: \"name\"\n"
                      "  },\n"
                      "  name : null\n"
                      "}");
    arr.push_back("{\n"
                      "  me: {\n"
                      "    you1 : [[\"Yep\", \"Yep\", \"Yup\"],, {here_it_is : 42},, 3, \"it`s\"],\n"
                      "    you2 : [[\"Yep\", \"Yep\", \"Yup\"],, {here_it_is : 42},, 3, \"it`s\"]\n"
                      "  },\n"
                      "  you : [[\"Yep\", \"Yep\", \"Yup\"],, {here_it_is : 42},, 3, \"it`s\"]\n"
                      "}");
    arr.push_back("{\n"
                      "  me : [,\"me\",]\n"
                      "}");
    arr.push_back("{}");
    arr.push_back("[]");
    return arr;
  }

  static std::vector<std::string> inValidArray(){
    std::vector<std::string> arr;
    arr.push_back("{n: nult}");
    arr.push_back("{n:}");
    return arr;
  }
};
