#ifndef LBS_PARSER_H
#define LBS_PARSER_H

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <cnote/build_scenario.h>
#include <cnote/target.h>

auto parse(std::string_view source) -> BuildScenario;

#endif /* LBS_PARSER_H */
