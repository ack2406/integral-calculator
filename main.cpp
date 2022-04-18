#include <cstdio>
#include <windows.h>
#include <cmath>
#include <chrono>
#include <utility>
#include <vector>
#include "argparse.cpp"

class PolynomialData {
public:
    std::vector<double> xs;
    std::pair<double, double> scope;

    double *pArea;

    CRITICAL_SECTION *criticalSection;
    double shift;

    PolynomialData(std::vector<double> xs, std::pair<double, double> scope, double *pArea,
                   CRITICAL_SECTION *criticalSection, double shift) : xs(std::move(xs)), scope(std::move(scope)),
                                                                      pArea(pArea),
                                                                      criticalSection(criticalSection), shift(shift) {}
};

DWORD WINAPI calculateIntegral(void *args) {
    auto *pData = (PolynomialData *) args;

    double area = 0;

    double shiftsAmount = (pData->scope.second - pData->scope.first) / pData->shift;

    for (int i = 0; i < (int) round(shiftsAmount); i++) {
        double midPoint = (pData->scope.first + pData->shift * i + pData->scope.first + pData->shift * (i + 1)) / 2;
        double height = 0;

        for (int j = 0; j < pData->xs.size(); j++) {
            height += pow(midPoint, j) * pData->xs[j];
        }
        area += height * pData->shift;
    }
    EnterCriticalSection(pData->criticalSection);

    (*pData->pArea) += area;

    LeaveCriticalSection(pData->criticalSection);

    delete pData;
    return 0;
}

int main(int argc, char **argv) {
    printf("XD");
    argparse::ArgumentParser program("Integral Calculator");

    program.add_argument("-t", "--threads")
            .help("amount of threads for calculations")
            .default_value(5)
            .scan<'d', int>();

    program.add_argument("-S", "--shift")
            .help("width of rectangles for calculations")
            .default_value(0.00001)
            .scan<'g', double>();

    program.add_argument("-s", "--scope")
            .help("scope of polynomial from first to second value")
            .nargs(2)
            .scan<'g', double>();

    program.add_argument("-p", "--polynomial")
        .help("values of polynomial sorted from x^0 to x^n")
        .remaining()
        .scan<'g', double>();


    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }
    //15523320449163134.000
    //15523320449157700
    CRITICAL_SECTION criticalSection;
    InitializeCriticalSection(&criticalSection);

    int threadAmount = program.get<int>("--threads");

    auto polynomial = program.get<std::vector<double>>("--polynomial");
    auto scopeVector = program.get<std::vector<double>>("--scope");
    auto fullScope = std::pair<double, double>{scopeVector[0], scopeVector[1]};

    auto start = std::chrono::steady_clock::now();
    std::vector<HANDLE> threads;
    double area = 0;
    auto shift = program.get<double>("--shift");

    DWORD id;

    std::pair<double, double> scope;

    double scopeShift = (fullScope.second - fullScope.first) / (double) threadAmount;

    for (int i = 0; i < threadAmount; i++) {
        scope.first = fullScope.first + scopeShift * i;
        scope.second = fullScope.second + scopeShift * (i + 1);

        auto *data = new PolynomialData(polynomial, scope, &area, &criticalSection, shift);

        threads.emplace_back(CreateThread(nullptr, 0, calculateIntegral, (void *) data, 0, &id));
        if (threads[i] != INVALID_HANDLE_VALUE) {
            SetThreadPriority(threads[i], THREAD_PRIORITY_NORMAL);
        }
    }
    for (int i = 0; i < threadAmount; i++) {
        WaitForSingleObject(threads[i], INFINITE);
    }
    auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration<double>(end - start).count();
    printf("threads: %d, area: %.3f,\ttime: %.3f s\n", threadAmount, area, time);

    DeleteCriticalSection(&criticalSection);

    return 0;
}

