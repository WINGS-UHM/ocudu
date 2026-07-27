// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "ocudu/support/cli11_utils.h"
#include "ocudu/support/config_parsers.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace ocudu;
using namespace ocudu::config;

namespace {

class cli11_utils_schema_test : public ::testing::Test
{
protected:
  void SetUp() override { registry().reset(); }
  void TearDown() override { registry().reset(); }

  static void setup_yaml(CLI::App& app)
  {
    app.config_formatter(create_yaml_config_parser());
    app.allow_config_extras(CLI::config_extras_mode::capture);
  }
};

struct cell_item {
  unsigned pci       = 1;
  uint8_t  sector_id = 127; // uint8_t default must be captured as a number, not 0x7f
};

void configure_cell(CLI::App& app, cell_item& item)
{
  add_option(app, "--pci", item.pci, "physical cell id")->capture_default_str();
  add_option(app, "--sector_id", item.sector_id, "sector id")->capture_default_str();
}

} // namespace

TEST_F(cli11_utils_schema_test, free_functions_capture_when_root_registered)
{
  CLI::App    app;
  schema_node root;
  register_schema_root(app, root);

  int  count = 3;
  bool flag  = false;
  add_option(app, "--count", count, "a count")->capture_default_str();
  add_option(app, "--flag", flag, "a flag")->capture_default_str();

  CLI::App* sub = add_subcommand(app, "section", "a section");
  int       x   = 5;
  add_option(*sub, "--x", x, "x value")->capture_default_str();

  ASSERT_EQ(root.children.size(), 3u);
  EXPECT_EQ(root.children[0]->name, "count");
  EXPECT_EQ(root.children[0]->type, leaf_type::integer);
  EXPECT_EQ(root.children[1]->name, "flag");
  EXPECT_EQ(root.children[1]->type, leaf_type::boolean);

  schema_node* grp = root.children[2].get();
  EXPECT_EQ(grp->kind, node_kind::group);
  EXPECT_EQ(grp->name, "section");
  ASSERT_EQ(grp->children.size(), 1u);
  EXPECT_EQ(grp->children[0]->name, "x");
}

TEST_F(cli11_utils_schema_test, passthrough_leaves_parsing_unchanged_without_root)
{
  // No schema root registered: capture is a no-op, parsing still works.
  CLI::App app;
  setup_yaml(app);
  int count = 0;
  add_option(app, "--count", count, "a count")->capture_default_str();

  std::istringstream ss("count: 7\n");
  app.parse_from_stream(ss);
  EXPECT_EQ(count, 7);
  EXPECT_EQ(registry().lookup(&app), nullptr);
}

TEST_F(cli11_utils_schema_test, option_pointer_exposes_required_flag)
{
  CLI::App    app;
  schema_node root;
  register_schema_root(app, root);

  int start = 0;
  add_option(app, "--start", start, "range start")->required();

  ASSERT_EQ(root.children.size(), 1u);
  ASSERT_NE(root.children[0]->option, nullptr);
  EXPECT_TRUE(root.children[0]->option->get_required());
}

TEST_F(cli11_utils_schema_test, option_group_options_recorded_at_parent_level)
{
  CLI::App    app;
  schema_node root;
  register_schema_root(app, root);

  // Options added to an option group share the parent's config namespace, so they must appear as the parent's
  // properties, not be dropped.
  CLI::App* group = add_option_group(app, "display_only");
  int       x     = 7;
  add_option(*group, "--grouped", x, "a grouped option")->capture_default_str();

  ASSERT_EQ(root.children.size(), 1u);
  EXPECT_EQ(root.children[0]->name, "grouped");
  EXPECT_EQ(root.children[0]->type, leaf_type::integer);
}

TEST_F(cli11_utils_schema_test, add_option_function_records_typed_leaf)
{
  CLI::App    app;
  schema_node root;
  register_schema_root(app, root);

  // A custom-parsed option over a non-string type must compile and record with the type of T (integer), no default.
  unsigned captured = 0;
  add_option_function<unsigned>(
      app, "--freq", [&captured](unsigned v) { captured = v; }, "a frequency")
      ->check(CLI::Range(0u, 100u));

  // A string-parsed option (enum name style) records as a string.
  std::string mode;
  add_option_function<std::string>(app, "--mode", [&mode](const std::string& v) { mode = v; }, "a mode");

  // A custom-parsed list of strings records as a scalar array.
  std::vector<std::string> masks;
  add_option_function<std::vector<std::string>>(
      app, "--masks", [&masks](const std::vector<std::string>& v) { masks = v; }, "a list of masks");

  ASSERT_EQ(root.children.size(), 3u);
  EXPECT_EQ(root.children[0]->name, "freq");
  EXPECT_EQ(root.children[0]->type, leaf_type::integer);
  EXPECT_FALSE(root.children[0]->dflt.present);
  EXPECT_EQ(root.children[1]->name, "mode");
  EXPECT_EQ(root.children[1]->type, leaf_type::string);
  EXPECT_EQ(root.children[2]->name, "masks");
  EXPECT_EQ(root.children[2]->type, leaf_type::string);
  EXPECT_TRUE(root.children[2]->is_scalar_array);
}

TEST_F(cli11_utils_schema_test, add_option_cell_captures_shape_and_parses)
{
  CLI::App root_app;
  setup_yaml(root_app);
  schema_node root;
  register_schema_root(root_app, root);

  std::vector<cell_item> cells;
  add_option_cell<cell_item>(root_app, "--cells", cells, configure_cell, "per-cell config");

  // Schema: array node with the element's fields, including the uint8_t captured as a number.
  ASSERT_EQ(root.children.size(), 1u);
  schema_node* arr = root.children[0].get();
  EXPECT_EQ(arr->kind, node_kind::array);
  EXPECT_EQ(arr->name, "cells");
  ASSERT_EQ(arr->children.size(), 2u);
  EXPECT_EQ(arr->children[0]->name, "pci");
  EXPECT_EQ(arr->children[1]->name, "sector_id");
  EXPECT_EQ(arr->children[1]->type, leaf_type::integer);
  ASSERT_TRUE(arr->children[1]->dflt.present);
  EXPECT_EQ(std::get<std::uint64_t>(arr->children[1]->dflt.values[0]), 127u);

  // Parse behaviour: a 2-element list is parsed into the target exactly like the hand-written lambdas.
  std::istringstream ss(R"(
cells:
  - pci: 10
  - pci: 20
    sector_id: 3
)");
  root_app.parse_from_stream(ss);
  ASSERT_EQ(cells.size(), 2u);
  EXPECT_EQ(cells[0].pci, 10u);
  EXPECT_EQ(cells[0].sector_id, 127); // default kept
  EXPECT_EQ(cells[1].pci, 20u);
  EXPECT_EQ(cells[1].sector_id, 3);
}

TEST_F(cli11_utils_schema_test, add_option_cell_prepare_element_seeds_defaults)
{
  CLI::App root_app;
  setup_yaml(root_app);
  schema_node root;
  register_schema_root(root_app, root);

  std::vector<cell_item> cells;
  add_option_cell<cell_item>(root_app, "--cells", cells, configure_cell, "per-cell", [](cell_item& c) { c.pci = 99; });

  std::istringstream ss(R"(
cells:
  - sector_id: 1
)");
  root_app.parse_from_stream(ss);
  ASSERT_EQ(cells.size(), 1u);
  EXPECT_EQ(cells[0].pci, 99u); // seeded by prepare_element, not overridden by the blob
  EXPECT_EQ(cells[0].sector_id, 1);
}
