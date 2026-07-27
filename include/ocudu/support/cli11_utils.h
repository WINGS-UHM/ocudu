// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/support/config_parsers.h"
#include "ocudu/support/config_schema.h"
#include "ocudu/support/string_parsing_utils.h"
#include "CLI/CLI11.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <sstream>

namespace ocudu {

using cli11_cell = std::vector<std::string>;

/// \brief Extracts the first option name from a comma-separated list of option names.
///
/// CLI11 allows specifying multiple names for an option using commas (e.g., "--addrs,--addr").
/// This function returns only the first name, which is needed for get_option_no_throw lookup.
///
/// \param option_name Single option name or list of comma-separated option name aliases (e.g. "--addrs,--addr").
/// \return The first option name to be used for option lookup.
inline std::string get_first_option_name(const std::string& option_name)
{
  auto pos = option_name.find(',');
  if (pos != std::string::npos) {
    return option_name.substr(0, pos);
  }
  return option_name;
}

/// \brief Adds a subcommand to the given application using the given subcommand name and description.
///
/// If the subcommand already exists in the application, returns a pointer to it.
///
/// \param app Application where the subcommand will be added.
/// \param name Subcommand name.
/// \param desc Human readable description of the subcommand.
/// \return A pointer to the subcommand added to the application.
inline CLI::App* add_subcommand(CLI::App& app, const std::string& name, const std::string& desc)
{
  CLI::App* subcommand = app.get_subcommand_no_throw(name);
  if (!subcommand) {
    subcommand = app.add_subcommand(name, desc)->configurable();
  }
  config::record_subcommand(app, *subcommand, name, desc);
  return subcommand;
}

/// \brief Adds an option to the given application.
///
/// This function adds an option to the given application using the given parameters. If the option is already present
/// in the application, it is removed and a new option is added that will call the callback of the deleted callback
/// and the conversion of the result for the given parameter. By doing this, it allows to add multiple parameters for
/// one option, so one option will be present in the configuration but the result will be written in all the
/// parameters registered for that option.
///
/// \param app Application where the option will be added.
/// \param option_name Option name.
/// \param param Parameter where the option value will be stored after parsing.
/// \param desc Human readable description of the option.
/// \return A pointer to the option added to the application.
template <typename T>
CLI::Option* add_option(CLI::App& app, const std::string& option_name, T& param, const std::string& desc)
{
  auto*        existing = app.get_option_no_throw(get_first_option_name(option_name));
  CLI::Option* opt      = nullptr;
  if (!existing) {
    opt = app.add_option(option_name, param, desc);
  } else {
    // Option was found. Get the callback and create new option.
    auto callbck = existing->get_callback();
    app.remove_option(existing);

    opt = app.add_option(
                 option_name,
                 [&param, callback = std::move(callbck)](const CLI::results_t& res) {
                   callback(res);
                   return CLI::detail::lexical_conversion<T, T>(res, param);
                 },
                 desc,
                 false,
                 [&param]() -> std::string { return CLI::detail::checked_to_string<T, T>(param); })
              ->run_callback_for_default();
  }
  config::record_option(app, option_name, param, desc, opt);
  return opt;
}

/// Specialization for bools than changes the default function for capture the default string.
template <>
inline CLI::Option* add_option(CLI::App& app, const std::string& option_name, bool& param, const std::string& desc)
{
  auto*        existing = app.get_option_no_throw(get_first_option_name(option_name));
  CLI::Option* opt      = nullptr;
  if (!existing) {
    opt = app.add_option(option_name, param, desc)->default_function([&param]() -> std::string {
      return param ? "true" : "false";
    });
  } else {
    // Option was found. Get the callback and create new option.
    auto callbck = existing->get_callback();
    app.remove_option(existing);

    opt = app.add_option(
                 option_name,
                 [&param, callback = std::move(callbck)](const CLI::results_t& res) {
                   callback(res);
                   return CLI::detail::lexical_conversion<bool, bool>(res, param);
                 },
                 desc,
                 false,
                 [&param]() -> std::string { return param ? "true" : "false"; })
              ->run_callback_for_default();
  }
  config::record_option(app, option_name, param, desc, opt);
  return opt;
}

/// \brief Adds an option group to the application and records it in the configuration schema.
///
/// Thin wrapper over \c CLI::App::add_option_group. Option groups share the parent's configuration namespace, so
/// their options are recorded as properties of the parent app in the schema (rather than being skipped, as a bare
/// CLI11 option group would be).
///
/// \param app Application where the option group will be added.
/// \param name Option group name.
/// \param desc Human readable description of the option group.
/// \return A pointer to the option group added to the application.
inline CLI::App* add_option_group(CLI::App& app, const std::string& name, const std::string& desc = "")
{
  CLI::App* group = app.add_option_group(name, desc);
  config::record_option_group(app, *group);
  return group;
}

/// \brief Adds a boolean flag to the given application and records it in the configuration schema.
///
/// Thin wrapper over \c CLI::App::add_flag that additionally records the flag as a boolean option in the schema
/// (a no-op when the app is not registered against a schema root).
///
/// \param app Application where the flag will be added.
/// \param option_name Flag name.
/// \param param Boolean parameter where the flag value will be stored after parsing.
/// \param desc Human readable description of the flag.
/// \return A pointer to the flag option added to the application.
inline CLI::Option* add_flag(CLI::App& app, const std::string& option_name, bool& param, const std::string& desc)
{
  CLI::Option* opt = app.add_flag(option_name, param, desc);
  config::record_flag(app, option_name, desc, opt);
  return opt;
}

/// \brief Adds an option function to the given application.
///
/// This function adds an option function to the given application using the given parameters. If the option is
/// already present in the application, it is removed and a new option is added that will contain the given function
/// and deleted callback as function. By doing this, it allows to add multiple parameters for one option, so one
/// option will be present in the configuration and the all the functions registered for that option will be called.
///
/// \param app Application where the option will be added.
/// \param option_name Option name.
/// \param func Function to execute during parsing.
/// \param desc Human readable description of the option.
/// \return A pointer to the option added to the application.
template <typename T>
CLI::Option* add_option_function(CLI::App&                            app,
                                 const std::string&                   option_name,
                                 const std::function<void(const T&)>& func,
                                 const std::string&                   desc)
{
  auto*        existing = app.get_option_no_throw(get_first_option_name(option_name));
  CLI::Option* opt      = nullptr;
  if (!existing) {
    opt = app.add_option_function<T>(option_name, func, desc)->run_callback_for_default();
  } else {
    // Option was found. Chain the previous callback with the new function. Generic over T: the results callback
    // runs the previous option's callback with the raw results, then converts them to T and calls the function.
    auto callbck = existing->get_callback();
    app.remove_option(existing);

    opt = app.add_option(
                 option_name,
                 [func, callback = std::move(callbck)](const CLI::results_t& res) -> bool {
                   callback(res);
                   T value{};
                   if (!CLI::detail::lexical_conversion<T, T>(res, value)) {
                     return false;
                   }
                   func(value);
                   return true;
                 },
                 desc)
              ->run_callback_for_default();
  }
  config::record_function_option<T>(app, option_name, desc, opt);
  return opt;
}

/// \brief Adds an option of type cell to the given application.
///
/// \param app Application where the option will be added.
/// \param option_name Option name.
/// \param func Function to execute during parsing.
/// \param desc Human readable description of the option.
/// \return A pointer to the option added to the application.
inline CLI::Option* add_option_cell(CLI::App&                                     app,
                                    const std::string&                            option_name,
                                    const std::function<void(const cli11_cell&)>& func,
                                    const std::string&                            desc)
{
  auto* opt = app.get_option_no_throw(get_first_option_name(option_name));
  if (!opt) {
    return app.add_option_function<std::vector<std::string>>(option_name, func, desc);
  }

  // Option was found. Get the callback and create new option.
  auto callbck = opt->get_callback();
  app.remove_option(opt);

  return app
      .add_option_function<cli11_cell>(
          option_name,
          [func, callback = std::move(callbck)](const cli11_cell& value) {
            func(value);
            callback(value);
          },
          desc)
      ->run_callback_for_default();
}

/// \brief Adds a list-of-struct option to the given application.
///
/// Parses each element blob into \c target[i] using \c configure, resizing \c target beforehand, reproducing the
/// semantics of the hand-written cell lambdas exactly. Additionally captures the element schema: \c configure is run
/// once against a default-constructed exemplar element, on a throwaway CLI::App bound to the array's item shape, so
/// the schema records the element structure through the same recording path as ordinary options.
///
/// \param app Application where the option will be added.
/// \param option_name Option name.
/// \param target Vector where the parsed elements will be stored.
/// \param configure Function that registers the element options on a per-element subapp.
/// \param desc Human readable description of the option.
/// \param prepare_element Optional function run on every element after the resize and before parsing (e.g. to seed
/// elements from a common configuration).
/// \return A pointer to the option added to the application.
/// \brief Records the element schema of a list-of-struct option \c option_name without registering a parser.
///
/// Runs \c configure once against a default-constructed exemplar element, on a throwaway CLI::App bound to the
/// array's item shape, so the schema records the element structure. Use this alongside a hand-written parse lambda
/// for options that cannot use the \ref add_option_cell overload (e.g. a map target, or a configurator that does
/// more than resize+configure+parse). A no-op when the app is not registered against a schema root.
template <typename T>
void declare_cell_schema(CLI::App&                                 app,
                         const std::string&                        option_name,
                         const std::function<void(CLI::App&, T&)>& configure,
                         const std::string&                        desc)
{
  auto exemplar_app  = std::make_shared<CLI::App>();
  auto exemplar_elem = std::make_shared<T>();
  config::record_array(app, option_name, desc, exemplar_app);
  configure(*exemplar_app, *exemplar_elem);
  // Anchor the exemplar element's lifetime to the exemplar app (its options bind to the element by reference); the
  // callback is never invoked as the exemplar app is never parsed.
  exemplar_app->parse_complete_callback([exemplar_elem]() {});
}

template <typename T>
CLI::Option* add_option_cell(CLI::App&                                 app,
                             const std::string&                        option_name,
                             std::vector<T>&                           target,
                             const std::function<void(CLI::App&, T&)>& configure,
                             const std::string&                        desc,
                             const std::function<void(T&)>&            prepare_element = nullptr)
{
  // Capture the element shape once, on a throwaway exemplar app bound to the array's item shape.
  declare_cell_schema<T>(app, option_name, configure, desc);

  return add_option_cell(
      app,
      option_name,
      [&target, configure, prepare_element, desc](const cli11_cell& values) {
        target.resize(values.size());
        if (prepare_element) {
          for (auto& element : target) {
            prepare_element(element);
          }
        }
        for (unsigned i = 0, e = values.size(); i != e; ++i) {
          CLI::App subapp(desc, "item #" + std::to_string(i));
          subapp.config_formatter(create_yaml_config_parser());
          subapp.allow_config_extras(CLI::config_extras_mode::capture);
          configure(subapp, target[i]);
          std::istringstream ss(values[i]);
          subapp.parse_from_stream(ss);
        }
      },
      desc);
}

/// Parse string into optional type.
template <typename T>
bool lexical_cast(const std::string& in, std::optional<T>& output)
{
  expected<T, std::string> result;

  if constexpr (std::is_integral_v<T>) {
    result = parse_int<T>(in);
  } else if constexpr (std::is_same_v<T, float>) {
    result = parse_float(in);
  } else {
    result = parse_double(in);
  }

  if (!result.has_value()) {
    return false;
  }

  output = result.value();
  return true;
}

/// Parsing an integer with additional option "auto" into an optional of an enum type.
template <typename Param>
void add_auto_enum_option(CLI::App&             app,
                          const std::string&    option_name,
                          std::optional<Param>& param,
                          const std::string&    desc)
{
  add_option_function<std::string>(
      app,
      option_name,
      [&param](const std::string& in) -> void {
        if (in.empty() or in == "auto") {
          return;
        }
        std::stringstream             ss(in);
        std::underlying_type_t<Param> val;
        ss >> val;
        param = (Param)val;
      },
      desc)
      ->check([](const std::string& in_str) -> std::string {
        if (in_str == "auto" or in_str.empty()) {
          return "";
        }
        // Check for a valid integer number;
        CLI::TypeValidator<int> IntegerValidator("INTEGER");
        return IntegerValidator(in_str);
      })
      ->default_str("auto");
}

} // namespace ocudu
