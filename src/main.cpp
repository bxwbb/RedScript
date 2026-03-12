#include <iostream>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "./generation.h"

namespace fs = std::filesystem;

constexpr size_t HEAP_MAX_SIZE = 1024 * 1;

bool create_folder_in_current_dir(const std::string &folder_name, const bool overwrite = true) {
    try {
        const fs::path target_path = fs::current_path() / folder_name;
        if (fs::exists(target_path)) {
            if (fs::is_directory(target_path)) {
                if (overwrite) {
                    // std::cout << "Folder already exists: " << target_path << ", skip creation\n";
                    return true;
                }
                throw std::runtime_error("Folder already exists: " + target_path.string());
            }
            throw std::runtime_error(
                "File with the same name exists, cannot create folder: " + target_path.string());
        }
        const bool created = fs::create_directories(target_path);
        if (created) {
            // std::cout << "Folder created successfully: " << target_path << "\n";
        }
        return created;
    } catch (const fs::filesystem_error &e) {
        throw std::runtime_error("Failed to create folder: " + std::string(e.what()));
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to create folder: " + std::string(e.what()));
    }
}

void create_basic_environment(const std::string &project_name) {
    create_folder_in_current_dir(project_name);
    std::fstream mcmeta(project_name + "/pack.mcmeta", std::ios::out);
    mcmeta << "{\n"
            << "\t\"pack\": {\n"
            << "\t\t\"description\": \"Build by RedScript.\",\n"
            << "\t\t\"pack_format\": 71,\n"
            << "\t\t\"supported_formats\": [48, 81],\n"
            << "\t\t\"min_format\": 48,\n"
            << "\t\t\"max_format\": [88, 0]\n"
            << "\t}\n"
            << "}";
    mcmeta.close();
    create_folder_in_current_dir(project_name + "/data");
    create_folder_in_current_dir(project_name + "/data/" + project_name);
    create_folder_in_current_dir(project_name + "/data/" + project_name + "/function");
    create_folder_in_current_dir(project_name + "/data/" + project_name + "/function/__util");
    create_folder_in_current_dir(project_name + "/data/" + project_name + "/tags");
    create_folder_in_current_dir(project_name + "/data/" + project_name + "/tags/function");
    create_folder_in_current_dir(project_name + "/data/" + "minecraft");
    create_folder_in_current_dir(project_name + "/data/" + "minecraft/tags");
    create_folder_in_current_dir(project_name + "/data/" + "minecraft/tags/function");
    std::fstream get_var_func(project_name + "/data/" + project_name + "/function/__util/get_data.mcfunction",
                              std::ios::out);
    get_var_func << "$execute store result storage minecraft:__" << project_name <<
            " $(ret) int 1 run data get storage minecraft:__" << project_name << " __stack[$(index)] 1";
    get_var_func.close();
    std::fstream get_heap_var(project_name + "/data/" + project_name + "/function/__util/get_data.mcfunction",
                              std::ios::out);
    get_heap_var << "$execute store result storage minecraft:__" << project_name <<
            " $(ret) int 1 run data get storage minecraft:__" << project_name << " __stack[$(index)] 1";
    get_heap_var.close();
    std::fstream reset_heap(project_name + "/data/" + project_name + "/function/__util/reset_heap.mcfunction",
                              std::ios::out);
    for (int i = 0; i < HEAP_MAX_SIZE; ++i) {
        reset_heap << "data modify storage minecraft:__" << project_name << " __heap append value 0\n";
    }
    reset_heap.close();
    std::fstream get_heap_value(project_name + "/data/" + project_name + "/function/__util/get_heap_value.mcfunction",
                              std::ios::out);
    get_heap_value << "$return run data get storage minecraft:__" + project_name + " __heap[$(index)] 1\n";
    get_heap_value.close();
    std::fstream set_heap_value(project_name + "/data/" + project_name + "/function/__util/set_heap_value.mcfunction",
                              std::ios::out);
    set_heap_value << "$data modify storage minecraft:__" + project_name + " __heap[$(index)] set value $(value)\n";
    set_heap_value.close();
    std::fstream wait_func(project_name + "/data/" + project_name + "/function/__util/wait_func.mcfunction",
                              std::ios::out);
    wait_func << "$schedule function $(func) $(time)t append";
    wait_func.close();
    std::fstream time_add_func(project_name + "/data/" + project_name + "/function/__util/time_add.mcfunction",
                              std::ios::out);
    time_add_func << "scoreboard players add __time __" << project_name << " 1";
    time_add_func.close();
    std::fstream tick_func_tag(project_name + "/data/" + "minecraft/tags/function/tick.json", std::ios::out);
    tick_func_tag << "{\n"
            << "\t\"values\": [\n"
            << "\t\t\"test:__util/time_add\"\n"
            << "\t]\n"
            << "}";
    tick_func_tag.close();
    std::fstream tool_func_tag(project_name + "/data/" + project_name + "/tags/function/__util.json", std::ios::out);
    tool_func_tag << "{\n"
            << "\t\"values\": [\n"
            << "\t\t\"test:__util/get_data\"\n"
            << "\t]\n"
            << "}";
    tool_func_tag.close();
    std::fstream all_func_tag(project_name + "/data/" + project_name + "/tags/function/__all.json", std::ios::out);
    all_func_tag << "{\n"
            << "\t\"values\": [\n"
            << "\t\t\"#test:__util\",\n"
            << "\t\t\"test:main\"\n"
            << "\t]\n"
            << "}";
    all_func_tag.close();
}

std::string get_file_name(const std::string &path) {
    const size_t sep_pos = path.find_last_of("/\\");
    std::string filename_with_ext;
    if (sep_pos == std::string::npos) {
        filename_with_ext = path;
    } else {
        filename_with_ext = path.substr(sep_pos + 1);
    }
    const size_t dot_pos = filename_with_ext.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return filename_with_ext;
    }
    return filename_with_ext.substr(0, dot_pos);
}

int main(const int argc, char *argv[]) {
    if (argc == 2) {
        if (std::strcmp(argv[1], "--version") == 0) {
            std::cout << "Version: " << VERSION << std::endl;
            std::cout << "Delta: nothing." << std::endl;
        } else if (std::strcmp(argv[1], "-v") == 0) {
            std::cout << "Version: " << VERSION << std::endl;
        } else {
            std::string contents;
            {
                std::stringstream contents_stream;
                std::fstream input(argv[1], std::ios::in);
                contents_stream << input.rdbuf();
                contents = contents_stream.str();
            }

            Tokenizer tokenizer(contents);
            std::vector<Token> tokens = tokenizer.tokenize();
            Parser parser(tokens, argv[1]);
            std::optional<NodeProg> prog = parser.parse_prog();

            if (!prog.has_value()) {
                print_error("Invalid program");
            }

            create_basic_environment(get_file_name(argv[1]));

            Generator generator(prog.value(), get_file_name(argv[1]), get_file_name(argv[1]) + "/data/" + get_file_name(argv[1]) + "/function/");

            generator.gen_prog();
        }
    } else {
        std::cerr << "Usage: " << argv[0] << " [--version]" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
