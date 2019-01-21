OpenSBI Contributing Guidelines
===============================

All contributions to OpenSBI should be sent as GitHub Pull Requests (PRs) to the main repository avaliable at: https://github.com/riscv/opensbi

As well as that all work must follow the following rules:
1. We follow the [Linux coding style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html). This means that ALL patches must include the Signed-off-by line.
2. Commit subject must have at least one ":" prefix. Common ones are "lib:", "platform:", "firmware:", "docs:" and "top:".
3. Maintainer should use "Rebase and Merge" when using GitHub to merge pull requests to avoid unnecessary merge commits.
4. Maintainer should avoid creating branches directly in main riscv/opensbi repository. Instead prefer fork of GitHub riscv/opensbi repository and create branches in your own fork for pull requests.
5. Maintainer cannot merge his own pull requests on GitHub.
6. A pull request must get at least one review from relevant maintainer.
7. A pull request must spend at least 24 hours in review so that everyone gets a chance to review.
