#ifndef LBS_TARGET_H
#define LBS_TARGET_H

#include <string>
#include <vector>

struct Target {
    enum Kind {
        UNKNOWN,
        GENERIC,
        LIBRARY,
        EXECUTABLE,
    } kind;

    const std::string name;
    std::string language;
    std::vector<std::string> sources;
    std::vector<std::string> include_directories;
    std::vector<std::string> linked_libraries;
    std::vector<std::string> flags;
    std::vector<std::string> defines;

    struct Requisite {
        enum Kind {
            COMMAND,
            COPY,
            DEPENDENCY,
        } kind;

        std::string text;
        std::vector<std::string> arguments;
        std::string destination;

        static void Print(const Requisite& requisite) {
            switch (requisite.kind) {
            case DEPENDENCY:
                printf("build dependency %s", requisite.text.data());
                break;
            case COMMAND:
                printf("%s", requisite.text.data());
                for (const auto& arg : requisite.arguments)
                    printf(" %s", arg.data());
                break;
            case COPY:
                printf(
                    "copy %s %s", requisite.text.data(),
                    requisite.destination.data()
                );
                break;
            }
        }
    };

    std::vector<Requisite> requisites;

    Target() = delete;

    static auto
    NamedTarget(Target::Kind kind, std::string name, std::string language)
        -> Target {
        return Target{
            kind, std::move(name), std::move(language), {}, {}, {}, {}, {}, {}};
    }

    static void Print(const Target& target) {
        switch (target.kind) {
        case UNKNOWN: printf("UNKNOWN-KIND TARGET "); break;
        case GENERIC: printf("TARGET "); break;
        case LIBRARY: printf("LIBRARY "); break;
        case EXECUTABLE:
            printf("EXECUTABLE ");
            break;
            break;
        }
        printf("%s\n", target.name.data());
        if (target.sources.size()) {
            printf("Sources:\n");
            for (const auto& source : target.sources)
                printf("- %s\n", source.data());
        }
        if (target.include_directories.size()) {
            printf("Include Directories:\n");
            for (const auto& include_dir : target.include_directories)
                printf("- %s\n", include_dir.data());
        }
        if (target.linked_libraries.size()) {
            printf("Linked Libraries:\n");
            for (const auto& library : target.linked_libraries)
                printf("- %s\n", library.data());
        }
        if (target.requisites.size()) {
            printf("Requisites:\n");
            for (const auto& requisite : target.requisites) {
                printf("- ");
                Requisite::Print(requisite);
                printf("\n");
            }
        }
    }
};

#endif /* LBS_TARGET_H */
