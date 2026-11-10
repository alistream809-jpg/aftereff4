#include "TestHarness.h"
int main() {
    int failedTests = 0;
    for (auto& t : registry()) {
        int before = failureCount();
        try { t.fn(); } catch (const std::exception& e) { ++failureCount(); std::cerr << "  EXCEPTION: " << e.what() << "\n"; }
        const bool ok = failureCount() == before;
        std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << t.name << "\n";
        if (!ok) ++failedTests;
    }
    std::cout << "\n" << registry().size() - failedTests << "/" << registry().size() << " tests passed\n";
    return failedTests == 0 ? 0 : 1;
}
