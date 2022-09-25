/***************************************************************************
* Copyright (c) 2022, Alexei Andreanov
*
* Distributed under the terms of the GNU General Public License v3.
*
* The full license is in the file LICENSE, distributed with this software.
****************************************************************************/

#include <string>
#include <vector>
#include <iostream>

//  Julia header
#include <julia.h>

#include "nlohmann/json.hpp"

#include "xeus/xinput.hpp"
#include "xeus/xinterpreter.hpp"
#include "xeus/xhelper.hpp"

#include "xeus-julia/xinterpreter.hpp"

//JULIA_DEFINE_FAST_TLS

namespace nl = nlohmann;

namespace xeus_julia
{
 
    interpreter::interpreter()
    {
        xeus::register_interpreter(this);
    }

    //  Code sent by the client for execution
    nl::json interpreter::execute_request_impl(int execution_counter, // Typically the cell number
                                                      const  std::string & code, // Code to execute
                                                      bool /*silent*/,
                                                      bool /*store_history*/,
                                                      nl::json /*user_expressions*/,
                                                      bool /*allow_stdin*/)
    {
        //  What is this?
        nl::json kernel_res;

/*
        if (code.compare("hello, world") == 0)
        {
            publish_stream("stdout", code);
        }

        if (code.compare("error") == 0)
        {
            publish_stream("stderr", code);
        }
*/
        //  Only question mark in the code
        if (code.compare("?") == 0)
        {
            std::string html_content = R"(<iframe class="xpyt-iframe-pager" src="
                https://xeus.readthedocs.io"></iframe>)";

            auto payload = nl::json::array();
        
            payload = nl::json::array();
            payload[0] = nl::json::object({
                {"data", {
                    {"text/plain", "https://xeus.readthedocs.io"},
                    {"text/html", html_content}}
                },
                {"source", "page"},
                {"start", 0}
            });

            return xeus::create_successful_reply(payload);
        }

        //  Evaluate the Julia code and return a pointer to the result
        jl_value_t *result = jl_eval_string(code);

        //  Has any exception been raised/thrown?
        jl_value_t *error = jl_exception_occurred();

        nl::json pub_data;
        //  Code is to be replaced with the result converted to a string
        //pub_data["text/plain"] = code;
        pub_data["text/plain"] = result_str;

        //  No exception was raised/thrown
        if (error == NULL)
        {
            //  Convert Julia result into the string
            //  This is incorrect: it only converts C string into Julia string
            //string result_str = std::string(jl_string_ptr(result));
            /*
                The options I have:
                    1. use repr("text/plain", result) to convert the result to a string
                    2. are the stdout ahd stderr intercepted by xeus?
            */
            //  Base.repr function
            const static jl_function_t *repr_func = jl_get_function(jl_base_module, "repr");
            //  output MIME type
            const static jl_value_t *mime = jl_eval_string("\"text/plain\"");
            //  execute repr(mime="text/plain", result) in Julia
            jl_value_t *result_jl_string = jl_call2(repr_func, mime, result);
            //  convert Julia string to C++ string
            string result_string = std::string(jl_string_ptr(result_jl_string));

            kernel_res["status"] = "ok";
//            kernel_res["user_expressions"] = result_string;
            
            publish_execution_result(execution_counter,
                std::move(pub_data),
                nl::json::object()
                    );

                //  The original auto-generated piece of code
//                return xeus::create_successful_reply();
        }
        //  An error occurred
        else
        {
            //  This call will appear to do nothing. However, it is possible to check whether an exception was thrown:
            //  if (jl_exception_occurred())
            //      printf("%s \n", jl_typeof_str(jl_exception_occurred()));
            //  If you are using the Julia C API from a language that supports exceptions (e.g. Python, C#, C++),
            //  it makes sense to wrap each call into libjulia with a function that checks whether an exception was thrown,
            //  and then rethrows the exception in the host language.

            //  Provide details about the error
            if (!silent) {
                publish_execution_error(jl_typeof_str(error), jl_(error), jl_(error));
            }

            kernel_res["status"] = "error";
/*            kernel_res["ename"] = ;
            kernel_res["evalue"] = ;
            kernel_red["traceback"] = ;*/
        }

        return kernel_res;
    }

    //  Configure/setup Julia
    void interpreter::configure_impl()
    {
        //  Perform some operations
        //  Required -- sets up the Julia context
        jl_init();
    }

    //  Check that the code in the cell is correct
    nl::json interpreter::is_complete_request_impl(const std::string& code)
    {
        if (code.compare("incomplete") == 0)
        {
            return xeus::create_is_complete_reply("incomplete"/*status*/, "   "/*indent*/);
        }
        else if(code.compare("invalid") == 0)
        {
            return xeus::create_is_complete_reply("invalid"/*status*/);
        }
        else
        {
            return xeus::create_is_complete_reply("complete"/*status*/);
        }
    }

    //  Autocompletion of the Julia code
    nl::json interpreter::complete_request_impl(const std::string&  code,
                                                     int cursor_pos)
    {
        // Code starts with 'H', it could be the following completion
        if (code[0] == 'H')
        {
       
            return xeus::create_complete_reply(
                {
                    std::string("Hello"),
                    std::string("Hey"),
                    std::string("Howdy")
                },          /*matches*/
                5,          /*cursor_start*/
                cursor_pos  /*cursor_end*/
            );
        }

        // No completion result
        else
        {

            return xeus::create_complete_reply(
                nl::json::array(),  /*matches*/
                cursor_pos,         /*cursor_start*/
                cursor_pos          /*cursor_end*/
            );
        }
    }

    //  Code inspection request -- prefixing code with a question mark?
    nl::json interpreter::inspect_request_impl(const std::string& /*code*/,
                                                      int /*cursor_pos*/,
                                                      int /*detail_level*/)
    {
        
        return xeus::create_inspect_reply(true/*found*/,
            {{std::string("text/plain"), std::string("hello!")}}, /*data*/
            {{std::string("text/plain"), std::string("hello!")}}  /*meta-data*/
        );
         
    }

    //  To do when the kernel is being shut down
    void interpreter::shutdown_request_impl() {
        /* This is strongly recommended by the Julia embedding manual:
        */
        jl_atexit_hook(0);

        //  The auto-generated bit of code
//        std::cout << "Bye!!" << std::endl;
    }

    //  Information about the kernel
    nl::json interpreter::kernel_info_request_impl()
    {

        const std::string  protocol_version = "5.3";
        const std::string  implementation = "xjulia";
        const std::string  implementation_version = XEUS_JULIA_VERSION;
        const std::string  language_name = "julia";
        const std::string  language_version = "1.7.1";
        const std::string  language_mimetype = "text/x-juliasrc";
        const std::string  language_file_extension = "jl";
        const std::string  language_pygments_lexer = "";
        const std::string  language_codemirror_mode = "";
        const std::string  language_nbconvert_exporter = "";
        const std::string  banner = "xjulia -- Xeus based Jupyter kernel for Julia";
        const bool         debugger = true;
        const nl::json     help_links = nl::json::array();


        return xeus::create_info_reply(
            protocol_version,
            implementation,
            implementation_version,
            language_name,
            language_version,
            language_mimetype,
            language_file_extension,
            language_pygments_lexer,
            language_codemirror_mode,
            language_nbconvert_exporter,
            banner,
            debugger,
            help_links
        );
    }

}
