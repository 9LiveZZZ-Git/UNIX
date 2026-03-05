#include <juce_core/juce_core.h>

int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int numFails = 0;
    int numPasses = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        if (result != nullptr)
        {
            numFails += result->failures;
            numPasses += result->passes;
            std::cout << (result->failures == 0 ? "  PASS " : "  FAIL ")
                      << result->unitTestName << " — "
                      << result->passes << " passed, "
                      << result->failures << " failed\n";
        }
    }

    std::cout << "\n" << numPasses << " passed, " << numFails << " failed.\n";
    if (numFails == 0)
        std::cout << "All tests passed!\n";
    else
        std::cout << numFails << " test(s) FAILED!\n";

    return numFails > 0 ? 1 : 0;
}
