#include "TestHarness.h"
#include "core/Json.h"
#include "core/Timecode.h"
#include "core/Id.h"
#include "core/FileUtil.h"
#include <set>
using namespace at;

TEST(json_roundtrip_basic) {
    Json j = Json::object();
    j["a"] = 1; j["b"] = 2.5; j["c"] = "hi \"there\"\n"; j["d"] = true; j["e"] = Json();
    Json arr = Json::array(); arr.push(1); arr.push("x"); j["f"] = arr;
    Json r = Json::parse(j.dump(2));
    CHECK(r.getNumber("a", 0) == 1);
    CHECK_NEAR(r.getNumber("b", 0), 2.5, 0);
    CHECK(r.getString("c", "") == "hi \"there\"\n");
    CHECK(r.getBool("d", false));
    CHECK(r.get("e").isNull());
    CHECK(r.get("f").asArray().size() == 2);
    CHECK(Json::parse(j.dump(0)).dump(0) == j.dump(0));
}
TEST(json_numbers_exact) {
    Json j = Json::object();
    j["x"] = 0.1; j["y"] = 1.0 / 3.0; j["z"] = -1234567.0; j["w"] = 1e-9;
    Json r = Json::parse(j.dump());
    CHECK(r.getNumber("x", 0) == 0.1);
    CHECK(r.getNumber("y", 0) == 1.0 / 3.0);
    CHECK(r.getNumber("z", 0) == -1234567.0);
    CHECK(r.getNumber("w", 0) == 1e-9);
}
TEST(json_unicode) {
    Json r = Json::parse("{\"s\":\"\\u00e9\\u4e2d\\ud83d\\ude00\"}");
    CHECK(r.getString("s", "") == "\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80");
    Json k = Json::object(); k["p"] = "C:\\Videos\\\xD0\x92\xD0\xB8\xD0\xB4\xD0\xB5\xD0\xBE.mp4";
    CHECK(Json::parse(k.dump()).getString("p", "") == k.getString("p", ""));
}
TEST(json_errors) {
    const char* bad[] = {"", "{", "{\"a\":}", "[1,]", "tru", "{\"a\":1} x", "\"abc", "{'a':1}"};
    for (const char* b : bad) {
        bool threw = false;
        try { Json::parse(b); } catch (const std::runtime_error&) { threw = true; }
        CHECK(threw);
    }
}
TEST(timecode) {
    CHECK(formatTimecode(0, 30) == "00:00:00:00");
    CHECK(formatTimecode(1.5, 30) == "00:00:01:15");
    CHECK(formatTimecode(3661.0, 24) == "01:01:01:00");
    CHECK_NEAR(snapToFrame(0.51, 30), 15.0 / 30.0, 1e-12);
}
TEST(ids_unique) {
    std::set<std::string> s;
    for (int i = 0; i < 1000; ++i) s.insert(newId());
    CHECK(s.size() == 1000);
}
TEST(file_atomic_write_and_read) {
    std::string path = (std::filesystem::temp_directory_path() / "at_test_file.txt").string();
    std::string err, back;
    CHECK(writeTextFileAtomic(path, "hello", err));
    CHECK(writeTextFileAtomic(path, "hello world", err));   // overwrite
    CHECK(readTextFile(path, back, err));
    CHECK(back == "hello world");
    std::filesystem::remove(path);
    CHECK(!fileExists(path));
}
