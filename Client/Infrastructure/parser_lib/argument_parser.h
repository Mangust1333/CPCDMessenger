#pragma once

#include <iostream>
#include <cstring>
#include <cstdint>
#include <utility>
#include <vector>
#include <sstream>
#include <charconv>
#include <string>
#include <utility>


namespace ArgumentParser {

    struct HelpCommandNames {
        char short_name;
        std::string name;
        std::string program_description;
        bool is_help_requested = false;

        HelpCommandNames(char short_name, std::string name, std::string program_description = "") : short_name(short_name), name(std::move(name)), program_description(std::move(program_description)) {}
        ~HelpCommandNames()noexcept=default;
        HelpCommandNames(const HelpCommandNames& other) : short_name(other.short_name), name(other.name), program_description(other.program_description) {}
        HelpCommandNames& operator=(const HelpCommandNames& other) {
            if(this == &other) {
                return *this;
            }
            short_name = other.short_name;
            name = other.name;
            program_description = other.program_description;
            return *this;
        }
        HelpCommandNames(HelpCommandNames&& other) noexcept : short_name(other.short_name), name(std::exchange(other.name, "")), program_description(std::exchange(other.program_description, "")) {}
        HelpCommandNames& operator=(HelpCommandNames&& other) noexcept {
            short_name = other.short_name;
            name = std::move(other.name);
            program_description = std::move(other.program_description);
            return *this;
        }
    };

    class ArgParser {
    public:
        [[maybe_unused]] int GetIntValue(const std::string& name_of_value, size_t parameter = 0);

        [[maybe_unused]] std::string GetStringValue(const std::string& name_of_value, size_t parameter = 0);

        [[maybe_unused]] bool GetFlag(const std::string& name_of_value);

        [[maybe_unused]] bool Parse(int argc, char **argv);

        [[maybe_unused]] bool Parse(const std::vector<std::string_view>& argv);

        [[maybe_unused]] void AddHelp(const char& short_name, const std::string& name, const std::string& description = "");

        [[maybe_unused]] bool Help();

        [[maybe_unused]] std::stringstream HelpDescription();

    private:

        class Argument {
        public:
            Argument(const char& short_name, std::string name, std::string description)
                    : short_name(short_name),
                      name(std::move(name)),
                      description(std::move(description)) { min_value_cnt = 0; }
            virtual ~Argument()=default;

            bool IsMultivalue() const { return is_multivalue; }
            bool IsDefaulted() const { return is_defaulted; }
            size_t GetMinValueCnt() const { return min_value_cnt; };
            char GetShortName() const { return short_name; };
            std::string GetName() const { return name; };
            std::string GetDescription() const { return description; };

        protected:
            bool is_multivalue = false;
            bool is_defaulted = false;
            char short_name;
            std::string name;
            std::string description;
            int min_value_cnt = 0;
            void* store_value = nullptr;
        };

        template<class IdiomClass, typename IdiomType>
        class AllArgument : public Argument {
        public:
            AllArgument(const char& short_name, std::string name, std::string description) : Argument(short_name, std::move(name), std::move(description)) {}
            virtual ~AllArgument()=default;
            [[maybe_unused]] virtual IdiomClass& Multivalue(size_t min_value) { is_multivalue = true; min_value_cnt = min_value; return *reinterpret_cast<IdiomClass*>(this); };
            [[maybe_unused]] virtual IdiomClass& StoreValue(IdiomType& value_ptr) { store_value = &value_ptr; return *reinterpret_cast<IdiomClass*>(this); };
            [[maybe_unused]] virtual IdiomClass& StoreValue(std::vector<IdiomType>& value_ptr) { store_value = &value_ptr; return *reinterpret_cast<IdiomClass*>(this); };
            [[maybe_unused]] virtual IdiomClass& Default(IdiomType default_value) {
                is_defaulted = true;
                defaulted_value = default_value;
                value.push_back(default_value);
                if(store_value != nullptr) {
                    if(is_multivalue) {
                        (*reinterpret_cast<std::vector<IdiomType>*>(store_value)).push_back(default_value);
                    } else {
                        *reinterpret_cast<IdiomType*>(store_value) = default_value;
                    }
                }
                return *reinterpret_cast<IdiomClass*>(this);
            }

            virtual void SetValue(IdiomType new_value) {
                if(is_multivalue) {
                    if(is_defaulted) {
                        is_defaulted = false;
                        value.clear();
                        if(store_value != nullptr) {
                            (*reinterpret_cast<std::vector<IdiomType>*>(store_value)).clear();
                        }
                    }
                    if(store_value != nullptr) {
                        (*reinterpret_cast<std::vector<IdiomType>*>(store_value)).push_back(new_value);
                    }
                    (min_value_cnt > 0) ? --min_value_cnt : min_value_cnt;
                    value.push_back(new_value);
                } else {
                    value.resize(1);
                    if(store_value != nullptr) {
                        *reinterpret_cast<IdiomType*>(store_value) = new_value;
                    }
                    value[0] = new_value;
                }
            };
            IdiomType GetValue(size_t parameter = 0) { return (parameter < value.size()) ? value[parameter] : IdiomType(); }
            IdiomType GetDefaultValue() { return defaulted_value; }
        protected:
            std::vector<IdiomType> value;
            IdiomType defaulted_value;
        };

        class FlagArgument : public AllArgument<FlagArgument, bool> {
        public:
            FlagArgument(const char& short_name, std::string name, std::string description) : AllArgument(short_name, std::move(name), std::move(description)) {}
            ~FlagArgument()=default;
            FlagArgument& Default(bool default_value) { is_defaulted = true; value = default_value; return *this; }
            void SetValue(bool new_value) {
                value = new_value;
                if(store_value != nullptr) {
                    *reinterpret_cast<bool*>(store_value) = new_value;
                }
            };
            bool GetValue() { return value; }
        private:
            bool value{};
        };

        class IntArgument : public AllArgument<IntArgument, int> {
        public:
            IntArgument(const char& short_name, std::string name, std::string description) : AllArgument(short_name, std::move(name), std::move(description)) {}
            ~IntArgument()=default;
        };

        class StringArgument : public AllArgument<StringArgument, std::string> {
        public:
            StringArgument(const char& short_name, std::string name, std::string description) : AllArgument(short_name, std::move(name), std::move(description)) {}
            ~StringArgument()=default;
        };

        void CheckArgumentDefinitionName(char short_name = '\0', const std::string& name = "");

        Argument* FindArgument(char short_name = '\0', const std::string& name = "");

    public:
        explicit ArgParser(std::string name = "new parser") : parser_name(std::move(name)) {}
        ~ArgParser()noexcept=default;
        ArgParser(const ArgParser& other) : parser_name(other.parser_name), arguments(other.arguments) {}
        ArgParser& operator=(const ArgParser& other) {
            if(this == &other) {
                return *this;
            }
            parser_name = other.parser_name;
            arguments = other.arguments;
            return *this;
        }
        ArgParser(ArgParser&& other)  noexcept : parser_name(std::exchange(other.parser_name, "")), arguments(std::move(other.arguments)) {}
        ArgParser& operator=(ArgParser&& other)  noexcept {
            parser_name = std::move(other.parser_name);
            arguments = std::move(other.arguments);
            return *this;
        }

        [[maybe_unused]] StringArgument& AddStringArgument(const std::string& name, const std::string& description = "");

        [[maybe_unused]] StringArgument& AddStringArgument(const char& short_name, const std::string& name, const std::string& description = "");

        [[maybe_unused]] IntArgument& AddIntArgument(const std::string& name, const std::string& program_description = "");

        [[maybe_unused]] IntArgument& AddIntArgument(const char& short_name, const std::string& name, const std::string& description = "");

        [[maybe_unused]] FlagArgument& AddFlag(const std::string& name, const std::string& description = "");

        [[maybe_unused]] FlagArgument& AddFlag(const char& short_name, const std::string& name, const std::string& description = "");

    private:
        std::string parser_name;
        std::vector<Argument*> arguments;
        HelpCommandNames helper{'h', "Help", "show Help description"};
    };
}   // namespace ArgumentParser

inline std::ostream& operator<<(std::ostream& output, std::stringstream ss) {
    output << ss.str();
    return output;
}