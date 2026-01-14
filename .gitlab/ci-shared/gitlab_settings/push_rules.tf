#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

resource "gitlab_project_push_rules" "this" {
  project                 = var.ci_project_id
  prevent_secrets         = var.push_rules.prevent_secrets
  commit_message_regex    = var.push_rules.commit_message_regex
  reject_unsigned_commits = var.push_rules.reject_unsigned_commits
  reject_non_dco_commits  = var.push_rules.reject_non_dco_commits
  deny_delete_tag         = var.push_rules.deny_delete_tag
}
