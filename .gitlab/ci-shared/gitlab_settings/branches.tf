# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


resource "gitlab_branch_protection" "protected_branches" {
  for_each = var.protected_branches

  project                      = var.ci_project_id
  branch                       = each.key
  allow_force_push             = each.value.allow_force_push
  code_owner_approval_required = each.value.code_owner_approval_required
  merge_access_level           = each.value.merge_access_level
  push_access_level            = each.value.push_access_level
}
