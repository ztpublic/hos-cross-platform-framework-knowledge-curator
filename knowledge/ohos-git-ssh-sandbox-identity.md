---
{
  "schema_version": 1,
  "id": "ohos-git-ssh-sandbox-identity",
  "title": "Handle uid and passwd mismatch for git and ssh in the OH sandbox",
  "topic_key": "process.ohos-git-ssh-sandbox-identity",
  "area": "process",
  "scope": "common",
  "frameworks": [],
  "summary": "Tools relying on file ownership or passwd need explicit configuration: give ssh its files by option and give git a scoped safe.directory policy in an app-private config.",
  "when_to_use": "git reports detected dubious ownership for repositories in user storage, and ssh ignores HOME, failing to find its config, known hosts or keys.",
  "applicability": "Bundled git and OpenSSH in OH app processes. Per source comments, files in shared user storage (the currentUser directory) have a different uid from the app, and the app uid has no usable passwd entry. SSH_ASKPASS_REQUIRE depends on your OpenSSH version; the reviewed code relies on it with OpenSSH 9.8.",
  "keywords": [
    "dubious ownership",
    "safe.directory",
    "ssh HOME",
    "passwd",
    "GIT_SSH_COMMAND",
    "UserKnownHostsFile",
    "IdentityFile",
    "SSH_ASKPASS_REQUIRE",
    "currentUser storage"
  ],
  "related": [
    "hnp-tool-private-prefix-environment",
    "ohos-exec-helper-permission-section",
    "sandbox-paths-and-app-identity"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Handle uid and passwd mismatch for git and ssh in the OH sandbox

## When to use

git reports detected dubious ownership for repositories in user storage, and ssh ignores HOME, failing to find its config, known hosts or keys.

## Recommended approach

Tools relying on file ownership or passwd need explicit configuration: give ssh its files by option and give git a scoped safe.directory policy in an app-private config.

## Implementation steps

1. Keep a global git config in app-private storage and refer to it with GIT_CONFIG_GLOBAL, so nothing leaks into other tools.
2. Add safe.directory entries for repositories the user has selected. The reviewed code writes directory = *, which turns the ownership check off entirely; do that only with an understanding of the risk.
3. Create ~/.ssh under the app home with an empty config, since ssh -F needs a file.
4. Set GIT_SSH_COMMAND with -F, -o UserKnownHostsFile and -o IdentityFile to absolute paths, as ssh resolves ~ from passwd rather than HOME.
5. Set SSH_ASKPASS_REQUIRE=force and SSH_ASKPASS to your prompt helper, with host-key confirmation handled through it.

## Conditions and alternatives

The wildcard removes protection against malicious repositories with hooks or config in user storage. Selecting directories one by one is safer, at the cost of more prompts. Consider disabling repository hooks if you keep a wildcard.

## Common mistakes

Setting safe.directory=* in a config other tools read. Assuming HOME controls ssh. Never persisting known_hosts, so every connection re-prompts.

## Check it works

Open a repository in user storage, clone and push over SSH with a key and with a password, then with an unknown host and a changed host key.
