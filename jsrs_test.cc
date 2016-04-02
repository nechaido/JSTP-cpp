#include "gtest/gtest.h"
#include "deps.h"

int checksum(std::string s){
  int result = 0;
  for(auto sIt : s){
    if (sIt != ' ' && sIt != '\n'){
      result ^= sIt;
    }
  }
  return result;
}

TEST(jsrs_test, jsrs_test_dump_Test1) {
  jstp::Record t;
  EXPECT_EQ(t.stringify(), "undefined");
}

TEST(jsrs_test, jsrs_test_dump_Test2) {
  jstp::Record t(nullptr);
  EXPECT_EQ(t.stringify(), "null");
}

TEST(jsrs_test, jsrs_test_dump_Test3) {
  jstp::Record t(25.5);
  EXPECT_EQ(t.stringify(), "25.5");
}

TEST(jsrs_test, jsrs_test_dump_Test4) {
  jstp::Record t(true);
  jstp::Record f(false);
  EXPECT_EQ(t.stringify(), "true");
  EXPECT_EQ(f.stringify(), "false");
}

TEST(jsrs_test, jsrs_test_dump_Test5) {
  jstp::Record t("test");
  EXPECT_EQ(t.stringify(), "\"test\"");
}

TEST(jsrs_test, jsrs_test_dump_Test6) {
  char* str;
  str = "test";
  jstp::Record t(str);
  EXPECT_EQ(t.stringify(), "\"test\"");
}

TEST(jsrs_test, jsrs_test_dump_Test7) {
  jstp::Record t1(25.5);
  jstp::Record t2(true);
  std::vector<jstp::Record> v;
  v.push_back(t1);
  v.push_back(t2);
  jstp::Record t(v);
  EXPECT_EQ(t.stringify(), "[25.5,true]");
}

TEST(jsrs_test, jsrs_test_dump_Test8) {
  std::map<std::string, jstp::Record> m;
  jstp::Record t1(25.5);
  jstp::Record t2(true);
  jstp::Record t3("test");
  std::vector<jstp::Record> v;
  v.push_back(t1);
  v.push_back(t2);
  jstp::Record arr(v);
  m["test1"] = t1;
  m["test2"] = t2;
  m["test3"] = t3;
  m["arr"] = arr;
  jstp::Record t(m);
  EXPECT_EQ(t.stringify(), "{arr:[25.5,true],test1:25.5,test2:true,test3:\"test\"}");
}

TEST(jsrs_test, jsrs_test_dump_TestShouldWork) {
  std::vector<std::string> arr = testData::validArray();
  for (auto &iterator : arr) {
    std::string err = "";
    jstp::Record jsrs = jstp::Record::parse(iterator, err);
    EXPECT_EQ("", err);
    if(checksum(jsrs.stringify()) != checksum(iterator)){
      EXPECT_EQ(iterator, jsrs.stringify()) << "Should be equal to: " << iterator;
    }
  }
}

TEST(jsrs_test, jsrs_test_dump_TestShouldNotWork) {
  std::vector<std::string> arr = testData::inValidArray();
  for (auto &iterator : arr) {
    std::string err = "";
    jstp::Record jsrs = jstp::Record::parse(iterator, err);
    EXPECT_NE("", err);
  }
}
