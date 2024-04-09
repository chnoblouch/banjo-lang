#include "pass_tester.hpp"

int main(int argc, char *argv[]) {
    return PassTester(argv[1], argv[2]).run();
}