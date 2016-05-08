//
// Created by coaxmetal on 16-5-4.
//

#ifndef MEDITATION_MEDITATION_DRIVER_H
#define MEDITATION_MEDITATION_DRIVER_H
namespace clang{
class CompilerInstance;
}

namespace meditation{
clang::CompilerInstance* createCompilerInstance(int argc, const char **argv);
}
#endif //MEDITATION_MEDITATION_DRIVER_H
