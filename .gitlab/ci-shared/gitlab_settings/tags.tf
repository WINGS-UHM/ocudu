#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

resource "gitlab_tag_protection" "protected_tags" {
  for_each = var.protected_tags

  project             = var.ci_project_id
  tag                 = each.key
  create_access_level = each.value.create_access_level
}
