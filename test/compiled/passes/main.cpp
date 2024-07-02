#include "pass_tester.hpp"

int main(int argc, char *argv[]) {
    using namespace banjo;
    
    return PassTester(argv[1], argv[2]).run();
}