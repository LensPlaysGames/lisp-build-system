#ifndef LBS_PARSER_H
#define LBS_PARSER_H

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <lbs/build_scenario.h>
#include <lbs/target.h>

auto parse(std::string_view source, std::string language) -> BuildScenario;

#endif /* LBS_PARSER_H */
