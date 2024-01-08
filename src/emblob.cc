/*
 * emblob.cc
 *
 * Author:    Ryan M. Lederman <lederman@gmail.com>
 * Copyright: Copyright (c) 2018-2024
 * Version:   0.2.0
 * License:   The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "emblob.hh"
#include "emblob/cmdline.hh"
#include "emblob/appstate.hh"
#include "emblob/util.hh"
#include "emblob/version.h"

using namespace std;
using namespace emblob;

int main(int argc, char** argv) {
    app_state state;
    command_line cmd_line;

    auto _exit_main = [&](int code) {
        if (code != EXIT_SUCCESS) {
            /* If exiting with an error code, clean up any files created;
               don't want to leave things in a half-assed state. */
            if (state.created_bin_file)
                delete_file_on_unclean_exit(cmd_line.get_bin_output_filename());
            if (state.created_asm_file)
                delete_file_on_unclean_exit(cmd_line.get_asm_output_filename());
            if (state.created_obj_file)
                delete_file_on_unclean_exit(cmd_line.get_obj_output_filename());
        }

        g_logger->debug("exiting with status: %d (%s)", code,
            code == EXIT_SUCCESS ? "success" : "failure");
        return code;
    };

    try {
        int exit_code = EXIT_FAILURE;
        if (!cmd_line.parse_and_validate(argc, argv, exit_code))
            return _exit_main(exit_code);

        std::string compiler = system::detect_c_compiler();
        if (compiler.empty())
            return _exit_main(EXIT_FAILURE);

        version_resource res;
        res.major = cmd_line.get_major_version();
        res.minor = cmd_line.get_minor_version();
        res.patch = cmd_line.get_patch_version();

        std::string suffix = cmd_line.get_suffix();
        if (!suffix.empty())
            strncpy(res.suffix, suffix.c_str(), EMBLOB_MAX_SUFFIX - 1);

        std::string bin_file = cmd_line.get_bin_output_filename();
        g_logger->info("writing version data {%" PRIu16 ", %" PRIu16 ", %" PRIu16 ", '%s'} to %s...",
            res.major, res.minor, res.patch, res.suffix, bin_file.c_str());

        auto openmode = ios::out | ios::binary | ios::trunc;
        auto wrote = system::write_file_contents(bin_file, openmode, [&](ostream& strm) -> void {
            strm.write(reinterpret_cast<const char*>(&res), sizeof(res));
        });

        if (wrote == -1) {
            g_logger->fatal("failed to write %s: %s", bin_file.c_str(),
                system::get_error_message(errno).c_str());
            return _exit_main(EXIT_FAILURE);
        }

        g_logger->info("successfully created %s (%lld bytes)", bin_file.c_str(),
            system::file_size(bin_file));
        state.created_bin_file = true;

#if defined(__MACOS__) || defined(__LINUS__) || defined(__BSD__)
        auto asm_file = cmd_line.get_asm_output_filename();
        openmode = ios::out | ios::trunc;
        wrote = system::write_file_contents(asm_file, openmode, [&](ostream& strm) -> void {
            strm << ".global _version_data" << endl;
            strm << "_version_data:" << endl;
            strm << ".incbin \"" << bin_file << "\"" << endl;
            strm << ".global _sizeof__version_data" << endl;
            strm << ".set _sizeof__version_data, . - _version_data" << endl;
        });

        if (wrote == -1) {
            g_logger->fatal("failed to write %s: %s", asm_file.c_str(),
                system::get_error_message(errno).c_str());
            return _exit_main(EXIT_FAILURE);
        }

        g_logger->info("successfully created %s (%lld bytes)", asm_file.c_str(),
            system::file_size(asm_file));
        state.created_asm_file = true;

        std::string obj_file = cmd_line.get_obj_output_filename();
        auto cmd = fmt_str("%s -c -o %s %s", compiler.c_str(), obj_file.c_str(), asm_file.c_str());
        bool asm_to_obj = system::execute_system_command(cmd);

        if (asm_to_obj) {
            state.created_obj_file = true;
            g_logger->info("successfully created %s (%lld bytes)", obj_file.c_str(),
            system::file_size(obj_file));
        }

        return _exit_main(asm_to_obj ? EXIT_SUCCESS : EXIT_FAILURE);
#else
# error "support for this platform is not implemented. please contact the author."
#endif
    } catch (const exception& ex) {
        g_logger->fatal("caught top-level exception: %s", ex.what());
        return _exit_main(EXIT_FAILURE);
    }

    return _exit_main(EXIT_SUCCESS);
}


void emblob::delete_file_on_unclean_exit(const std::string& fname) {
    if (0 != remove(fname.c_str()))
        g_logger->error("failed to delete '%s': %s", fname.c_str(),
            system::get_error_message(errno).c_str());
    else
        g_logger->info("deleted '%s'", fname.c_str());
}
