#ifndef LBS_PARSER_H
#define LBS_PARSER_H

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <cnote/target.h>
#include <cnote/build_scenario.h>

auto parse(std::string_view source) -> BuildScenario;

#endif /* LBS_PARSER_H */
