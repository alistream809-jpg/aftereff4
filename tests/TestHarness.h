#pragma once
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
struct TestCase { std::string name; std::function<void()> fn; };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
struct Registrar { Registrar(const char* n, std::function<void()> f) { registry().push_back({n, std::move(f)}); } };
inline int& failureCount() { static int n = 0; return n; }
#define TEST(name) static void name(); static Registrar reg_##name(#name, name); static void name()
#define CHECK(expr) do { if (!(expr)) { ++failureCount(); std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #expr "\n"; } } while (0)
#define CHECK_NEAR(a, b, eps) do { double _a = (a), _b = (b); if (!(std::fabs(_a - _b) <= (eps))) { ++failureCount(); std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #a " = " << _a << " expected " << _b << "\n"; } } while (0)
