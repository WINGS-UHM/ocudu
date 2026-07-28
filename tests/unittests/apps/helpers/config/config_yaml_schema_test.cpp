// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "apps/helpers/config/config_yaml_schema.h"
#include "ocudu/support/cli11_utils.h"
#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

using namespace ocudu;
using namespace ocudu::config;

namespace {

struct cell_item {
  unsigned pci       = 1;
  uint8_t  sector_id = 127;
};

void configure_cell(CLI::App& app, cell_item& item)
{
  add_option(app, "--pci", item.pci, "physical cell id")->capture_default_str();
  add_option(app, "--sector_id", item.sector_id, "sector id")->capture_default_str();
}

class config_yaml_schema_test : public ::testing::Test
{
protected:
  void SetUp() override { registry().reset(); }
  void TearDown() override { registry().reset(); }
};

} // namespace

TEST_F(config_yaml_schema_test, emits_expected_schema)
{
  CLI::App    app("A test application");
  schema_node root;
  register_schema_root(app, root);
  root.description = "A test application";

  int         count = 3;
  bool        flag  = true;
  double      gain  = 0.5;
  std::string name  = "abc";
  int         start = 0;
  add_option(app, "--count", count, "a count")->capture_default_str()->check(CLI::Range(-1, 1024));
  add_option(app, "--flag", flag, "a flag")->capture_default_str();
  add_option(app, "--gain", gain, "a gain")->capture_default_str();
  add_option(app, "--name", name, "a name")->capture_default_str()->check(CLI::IsMember({"abc", "def"}));
  add_option(app, "--start", start, "mandatory start")->required();
  int band = 1;
  add_option(app, "--band", band, "a band")->capture_default_str()->range(1, 78);

  CLI::App* sub  = add_subcommand(app, "section", "a section");
  unsigned  port = 38472;
  add_option(*sub, "--port", port, "a port")->capture_default_str();

  std::vector<cell_item> cells;
  add_option_object_list<cell_item>(app, "--cells", cells, configure_cell, "per-cell config");

  std::vector<int> ids;
  add_option(app, "--ids", ids, "id list")->range(0, 9);

  std::string yaml = app_helpers::generate_yaml_config_schema(root, "Test");

  // Round-trips as valid YAML and ends with a newline.
  ASSERT_FALSE(yaml.empty());
  EXPECT_EQ(yaml.back(), '\n');
  YAML::Node doc = YAML::Load(yaml);
  ASSERT_TRUE(doc.IsMap());

  EXPECT_EQ(doc["$schema"].as<std::string>(), "http://stsci.edu/schemas/yaml-schema/draft-01");
  EXPECT_EQ(doc["title"].as<std::string>(), "Test");
  EXPECT_EQ(doc["type"].as<std::string>(), "object");

  YAML::Node props = doc["properties"];
  ASSERT_TRUE(props.IsMap());

  // Scalar leaves: type + typed default + description.
  EXPECT_EQ(props["count"]["type"].as<std::string>(), "integer");
  EXPECT_EQ(props["count"]["default"].as<int>(), 3);
  EXPECT_EQ(props["count"]["description"].as<std::string>(), "a count");
  // Constraints from check(CLI::Range) (bridge) and from the first-class .range()/enum.
  EXPECT_EQ(props["count"]["minimum"].as<int>(), -1);
  EXPECT_EQ(props["count"]["maximum"].as<int>(), 1024);
  EXPECT_EQ(props["band"]["minimum"].as<int>(), 1);
  EXPECT_EQ(props["band"]["maximum"].as<int>(), 78);
  ASSERT_TRUE(props["name"]["enum"].IsSequence());
  EXPECT_EQ(props["name"]["enum"][0].as<std::string>(), "abc");
  EXPECT_EQ(props["flag"]["type"].as<std::string>(), "boolean");
  EXPECT_EQ(props["flag"]["default"].as<bool>(), true);
  EXPECT_EQ(props["gain"]["type"].as<std::string>(), "number");
  EXPECT_DOUBLE_EQ(props["gain"]["default"].as<double>(), 0.5);
  EXPECT_EQ(props["name"]["type"].as<std::string>(), "string");
  EXPECT_EQ(props["name"]["default"].as<std::string>(), "abc");

  // Required option: listed in required, and carries NO default.
  ASSERT_TRUE(doc["required"].IsSequence());
  ASSERT_EQ(doc["required"].size(), 1u);
  EXPECT_EQ(doc["required"][0].as<std::string>(), "start");
  EXPECT_TRUE(props["start"]);
  EXPECT_FALSE(props["start"]["default"]);

  // Nested section.
  YAML::Node section = props["section"];
  EXPECT_EQ(section["type"].as<std::string>(), "object");
  EXPECT_EQ(section["description"].as<std::string>(), "a section");
  EXPECT_EQ(section["properties"]["port"]["type"].as<std::string>(), "integer");
  EXPECT_EQ(section["properties"]["port"]["default"].as<int>(), 38472);

  // List-of-struct: array of objects; uint8_t default is the number 127, not 0x7f.
  YAML::Node cells_node = props["cells"];
  EXPECT_EQ(cells_node["type"].as<std::string>(), "array");
  YAML::Node items = cells_node["items"];
  EXPECT_EQ(items["type"].as<std::string>(), "object");
  EXPECT_EQ(items["properties"]["pci"]["type"].as<std::string>(), "integer");
  EXPECT_EQ(items["properties"]["sector_id"]["type"].as<std::string>(), "integer");
  EXPECT_EQ(items["properties"]["sector_id"]["default"].as<int>(), 127);

  // Scalar-array (std::vector<T>) option: CLI11 accepts either a single value or a list, so the option is described
  // as oneOf(scalar, array-of-scalar) - not array-only - and the item constraints appear in both branches.
  YAML::Node ids_node = props["ids"];
  EXPECT_FALSE(ids_node["type"]); // described purely via oneOf, no bare type
  ASSERT_TRUE(ids_node["oneOf"].IsSequence());
  ASSERT_EQ(ids_node["oneOf"].size(), 2u);
  EXPECT_EQ(ids_node["oneOf"][0]["type"].as<std::string>(), "integer"); // single value
  EXPECT_EQ(ids_node["oneOf"][0]["minimum"].as<int>(), 0);
  EXPECT_EQ(ids_node["oneOf"][0]["maximum"].as<int>(), 9);
  EXPECT_EQ(ids_node["oneOf"][1]["type"].as<std::string>(), "array"); // list
  EXPECT_EQ(ids_node["oneOf"][1]["items"]["type"].as<std::string>(), "integer");
  EXPECT_EQ(ids_node["oneOf"][1]["items"]["maximum"].as<int>(), 9);

  // No control characters leaked into the document (the 0x7f bug).
  EXPECT_EQ(yaml.find('\x7f'), std::string::npos);
}
