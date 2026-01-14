#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

resource "gitlab_branch_protection" "protected_branches" {
  for_each = var.protected_branches

  project                      = var.ci_project_id
  branch                       = each.key
  allow_force_push             = each.value.allow_force_push
  code_owner_approval_required = each.value.code_owner_approval_required
  merge_access_level           = each.value.merge_access_level
  push_access_level            = each.value.push_access_level
}
