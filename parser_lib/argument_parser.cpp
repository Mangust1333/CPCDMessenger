#include "argument_parser.h"

namespace ArgumentParser {

    ArgParser::StringArgument& ArgParser::AddStringArgument(const char& short_name, const std::string& name, const std::string& description) {
        CheckArgumentDefinitionName(short_name, name);
        arguments.push_back(new ArgParser::StringArgument(short_name, name, description));
        return *dynamic_cast<ArgParser::StringArgument*>(arguments.back());
    }

    [[maybe_unused]] ArgParser::StringArgument &
    ArgParser::AddStringArgument(const std::string& name, const std::string& description) {
        CheckArgumentDefinitionName('\0', name);
        arguments.push_back(new ArgParser::StringArgument('\0', name, description));
        return *dynamic_cast<StringArgument*>(arguments.back());
    }

    ArgParser::IntArgument& ArgParser::AddIntArgument(const char& short_name, const std::string& name, const std::string& description) {
        CheckArgumentDefinitionName(short_name, name);
        arguments.push_back(new ArgParser::IntArgument(short_name, name, description));
        return *dynamic_cast<IntArgument*>(arguments.back());
    }

    ArgParser::IntArgument &ArgParser::AddIntArgument(const std::string& name, const std::string& description) {
        CheckArgumentDefinitionName('\0', name);
        arguments.push_back(new ArgParser::IntArgument('\0', name, description));
        return *dynamic_cast<IntArgument*>(arguments.back());
    }

    ArgParser::FlagArgument& ArgParser::AddFlag(const char& short_name, const std::string& name, const std::string& description) {
        CheckArgumentDefinitionName(short_name, name);
        arguments.push_back(new ArgParser::FlagArgument(short_name, name, description));
        return *dynamic_cast<FlagArgument*>(arguments.back());
    }

    ArgParser::FlagArgument& ArgParser::AddFlag(const std::string& name, const std::string& description) {
        CheckArgumentDefinitionName('\0', name);
        arguments.push_back(new ArgParser::FlagArgument('\0', name, description));
        return *dynamic_cast<FlagArgument*>(arguments.back());
    }

    void ArgParser::AddHelp(const char& short_name, const std::string& name, const std::string& description) {
        if(FindArgument(short_name, name) != nullptr) {
            throw std::logic_error("arguments must be named with difference");
        }
        helper = HelpCommandNames(short_name, name, description);
    }

    int ArgParser::GetIntValue(const std::string& name_of_value, size_t parameter) {
        Argument* int_argument = FindArgument('\0', name_of_value);
        if(int_argument) {
            return dynamic_cast<IntArgument*>(int_argument)->GetValue(parameter);
        }
        return -1;
    }

    std::string ArgParser::GetStringValue(const std::string& name_of_value, size_t parameter) {
        Argument* string_argument = FindArgument('\0', name_of_value);
        if(string_argument) {
            return dynamic_cast<StringArgument*>(string_argument)->GetValue(parameter);
        }
        return "\0";
    }

    bool ArgParser::GetFlag(const std::string& name_of_value) {
        Argument* flag_argument = FindArgument('\0', name_of_value);
        if(flag_argument) {
            return dynamic_cast<FlagArgument*>(flag_argument)->GetValue();
        }
        return false;
    }

    void ArgParser::CheckArgumentDefinitionName(char short_name, const std::string& name) {
        if(FindArgument(short_name, name) != nullptr || helper.short_name == short_name || helper.name == name) {
            throw std::logic_error("arguments must be named with difference");
        }
    }

    bool ArgParser::Parse(int argc, char **argv) {
        std::vector<std::string_view> parse_args;
        for (int i = 0; i < argc; ++i) {
            parse_args.push_back(argv[i]);
        }
        return Parse(parse_args);
    }

    bool ArgParser::Parse(const std::vector<std::string_view>& argv) {
        Argument* positional_argument = nullptr;
        for(auto i = ++argv.begin(); i != argv.end(); ++i) {
            std::string_view term = *i;
            if(term.starts_with("--")) {
                positional_argument = nullptr;
                Argument* parameter;
                size_t position_of_equal = term.find('=');
                std::string_view parameter_value = term.substr(position_of_equal + 1, term.size() - position_of_equal);
                if(position_of_equal == -1) {
                    parameter = FindArgument('\0', std::string(term.substr(2, term.size() - 2)));
                } else {
                    parameter = FindArgument('\0', std::string(term.substr(2, position_of_equal - 2)));
                }
                if(parameter) {
                    if(typeid(*parameter) == typeid(ArgParser::FlagArgument)) {
                        dynamic_cast<FlagArgument*>(parameter)->SetValue(true);
                    } else if(typeid(*parameter) == typeid(ArgParser::IntArgument)) {
                        dynamic_cast<IntArgument*>(parameter)->SetValue(std::stoi(std::string(parameter_value)));
                    } else {
                        dynamic_cast<StringArgument*>(parameter)->SetValue(std::string(parameter_value));
                    }
                } else if(term.substr(2, term.size() - 2) == helper.name) {
                    helper.is_help_requested = true;
                } else {
                    std::cerr << "a non-existent parameter " << term[1] << '\n';
                    return false;
                }
            } else if(term.starts_with("-")) {
                Argument* parameter = FindArgument(term[1]);
                if(parameter) {
                    if(typeid(*parameter) == typeid(ArgParser::FlagArgument)) {
                        dynamic_cast<FlagArgument*>(parameter)->SetValue(true);
                    } else {
                        positional_argument = parameter;
                    }
                } else if(term[1] == helper.short_name) {
                    helper.is_help_requested = true;
                } else {
                    std::cerr << "a non-existent parameter " << term[1] << '\n';
                    return false;
                }
            } else {
                if(positional_argument) {
                    if(typeid(*positional_argument) == typeid(ArgParser::IntArgument)) {
                        dynamic_cast<IntArgument*>(positional_argument)->SetValue(std::stoi(std::string(term)));
                    } else {
                        dynamic_cast<StringArgument*>(positional_argument)->SetValue(std::string(term));
                    }
                } else {
                    std::cerr << "positional argument must be not empty\n";
                    return false;
                }
            }
        }
        for(auto i : arguments) {
            if(typeid(*i) != typeid(ArgParser::FlagArgument)) {
                if(i->GetMinValueCnt() > 0) {
                    std::cerr << "Too few parameters for argument " << i->GetName() << '\n';
                    return false;
                }
            }
        }
        return true;
    }

    ArgParser::Argument* ArgParser::FindArgument(char short_name, const std::string &name) {
        for(auto i : arguments) {
            if(short_name == i->GetShortName() && short_name != '\0' || name == i->GetName() && name != "") {
                return i;
            }
        }
        return nullptr;
    }

    bool ArgParser::Help() {
        return helper.is_help_requested;
    }

    std::stringstream ArgParser::HelpDescription() {
        std::stringstream help_text;
        help_text << parser_name << '\n';
        help_text << helper.program_description << '\n';
        help_text << '\n';
        for(auto i : arguments) {
            (i->GetShortName() != '\0') ? help_text << '-' << i->GetShortName() << ",\t" : help_text << '\t';
            if(typeid(*i) == typeid(ArgParser::FlagArgument)) {
                help_text << "--" << i->GetName() << ",\t" << i->GetDescription();
                if(i->IsDefaulted()) {
                    help_text << " [default = " << reinterpret_cast<FlagArgument*>(i)->GetDefaultValue() << "]";
                }
                help_text << '\n';
            } else if(typeid(*i) == typeid(ArgParser::StringArgument)) {
                help_text << "--" << i->GetName() << "=<string>,\t" << i->GetDescription();
                if(i->IsDefaulted()) {
                    help_text << " [default = " << reinterpret_cast<StringArgument*>(i)->GetDefaultValue() << "]";
                }
                if(i->IsMultivalue()) {
                    help_text << " [repeated, min args = " << reinterpret_cast<StringArgument*>(i)->GetMinValueCnt() << "]";
                }
                help_text << '\n';
            } else if(typeid(*i) == typeid(ArgParser::IntArgument)) {
                help_text << "--" << i->GetName() << "=<int>,\t" << i->GetDescription();
                if(i->IsDefaulted()) {
                    help_text << " [default = " << reinterpret_cast<IntArgument*>(i)->GetDefaultValue() << "]";
                }
                if(i->IsMultivalue()) {
                    help_text << " [repeated, min args = " << reinterpret_cast<IntArgument*>(i)->GetMinValueCnt() << "]";
                }
                help_text << '\n';
            }
        }
        help_text << '\n';
        help_text << '-' << helper.short_name << ",\t--" << helper.name << "\tDisplay this help and exit\n";
        return help_text;
    }

}