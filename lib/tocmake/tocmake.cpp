#include <tocmake/tocmake.h>

#include <cstdio>

#include <cnote/target.h>
#include <cnote/build_scenario.h>

std::string tocmake_target(const Target& target) {
    std::string out{};

    switch (target.kind) {
    case Target::GENERIC:
        std::printf("TODO: tocmake: handle generic target (sorry)\n");
        return "";

    case Target::LIBRARY: {
        out += "\nadd_library(";
        out += target.name;
        out += ")\n";
    } break;
    case Target::EXECUTABLE: {
        out += "\nadd_executable(";
        out += target.name;
        out += ")\n";
    } break;

    case Target::UNKNOWN:
    default:
        // UNREACHABLE
        exit(2);
    }

    if (target.sources.size()) {
        out += "target_sources(";
        out += target.name;
        out += " PRIVATE";
        for (const auto& source : target.sources) {
            out += ' ';
            out += source;
        }
        out += ")\n";
    }

    if (target.include_directories.size()) {
        out += "target_include_directories(";
        out += target.name;
        out += " PRIVATE";
        for (const auto& include_dir : target.include_directories) {
            out += ' ';
            out += include_dir;
        }
        out += ")\n";
    }

    if (target.linked_libraries.size()) {
        out += "target_link_libraries(";
        out += target.name;
        for (const auto& lib : target.linked_libraries) {
            out += ' ';
            out += lib;
        }
        out += ")\n";
    }

    if (target.defines.size()) {
        out += "target_compile_definitions(";
        out += target.name;
        out += " PRIVATE";
        for (const auto& define : target.defines) {
            out += ' ';
            out += define;
        }
        out += ")\n";
    }

    if (target.flags.size()) {
        out += "target_compile_options(";
        out += target.name;
        out += " PRIVATE";
        for (const auto& flag : target.flags) {
            out += ' ';
            out += flag;
        }
        out += ")\n";
    }

    return out;
}

std::string tocmake(const BuildScenario& build_scenario) {
    std::string header =
        "cmake_minimum_required(VERSION 3.14)\n"
        "project(lbs-autogen)\n";

    std::string targets{};
    for (const auto& target : build_scenario.targets)
        targets += tocmake_target(target);

    return header + targets;
}
