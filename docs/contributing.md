Contrubuting to OpenSBI
========================

All contributions to OpenSBI should be sent as GitHub Pull Requests (PRs) to the main repo avaliable at: https://github.com/riscv/opensbi

As well as that all work must follow the following rules:
1. We follow the [Linux coding style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html). This means that ALL patches must include the Signed-off-by line.
2. Commit subject must have atleast one ":" prefix. Common ones are "lib:", "platform:", "firmware:", "docs:" and "top:".
3. Maintainer should prefer "Rebase and Merge" when using GitHub to merge pull requests. Avoid create extra merge commits.
4. Maintainer should avoid creating branches directly in main riscv/opensbi repo. Instead prefer fork of GitHub riscv/opensbi repo and create branches in your own fork for pull requests.
5. Maintainer cannot merge his own pull requests on GitHub.
6. A pull request must get atleast one review from relevant maintainer.
7. A pull request must spend atleast 24 hours in review so that everyone gets a chance to review.
