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
  jstp::JSRS t;
  EXPECT_EQ(t.dump(), "undefined");
}

TEST(jsrs_test, jsrs_test_dump_Test2) {
  jstp::JSRS t(nullptr);
  EXPECT_EQ(t.dump(), "null");
}

TEST(jsrs_test, jsrs_test_dump_Test3) {
  jstp::JSRS t(25.5);
  EXPECT_EQ(t.dump(), "25.5");
}

TEST(jsrs_test, jsrs_test_dump_Test4) {
  jstp::JSRS t(true);
  jstp::JSRS f(false);
  EXPECT_EQ(t.dump(), "true");
  EXPECT_EQ(f.dump(), "false");
}

TEST(jsrs_test, jsrs_test_dump_Test5) {
  jstp::JSRS t("test");
  EXPECT_EQ(t.dump(), "\"test\"");
}

TEST(jsrs_test, jsrs_test_dump_Test6) {
  char* str;
  str = "test";
  jstp::JSRS t(str);
  EXPECT_EQ(t.dump(), "\"test\"");
}

TEST(jsrs_test, jsrs_test_dump_Test7) {
  jstp::JSRS t1(25.5);
  jstp::JSRS t2(true);
  std::vector<jstp::JSRS> v;
  v.push_back(t1);
  v.push_back(t2);
  jstp::JSRS t(v);
  EXPECT_EQ(t.dump(), "[25.5,true]");
}

TEST(jsrs_test, jsrs_test_dump_Test8) {
  std::map<std::string, jstp::JSRS> m;
  jstp::JSRS t1(25.5);
  jstp::JSRS t2(true);
  jstp::JSRS t3("test");
  std::vector<jstp::JSRS> v;
  v.push_back(t1);
  v.push_back(t2);
  jstp::JSRS arr(v);
  m["test1"] = t1;
  m["test2"] = t2;
  m["test3"] = t3;
  m["arr"] = arr;
  jstp::JSRS t(m);
  EXPECT_EQ(t.dump(), "{arr:[25.5,true],test1:25.5,test2:true,test3:\"test\"}");
}

TEST(jsrs_test, jsrs_test_dump_TestShouldWork) {
  std::vector<std::string> arr = testData::validArray();
  for (auto &iterator : arr) {
    std::string err = "";
    jstp::JSRS jsrs = jstp::JSRS::parse(iterator, err);
    EXPECT_EQ("", err);
    if(checksum(jsrs.dump()) != checksum(iterator)){
      EXPECT_EQ(iterator, jsrs.dump()) << "Should be equal to: " << iterator;
    }
  }
}

TEST(jsrs_test, jsrs_test_dump_TestShouldNotWork) {
  std::vector<std::string> arr = testData::inValidArray();
  for (auto &iterator : arr) {
    std::string err = "";
    jstp::JSRS jsrs = jstp::JSRS::parse(iterator, err);
    EXPECT_NE("", err);
  }
}
