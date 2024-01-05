# Refreshing code coverage report

The latest code coverage report generated from integration test run is available at
[Development/Code coverage](https://bluechi.readthedocs.io/en/latest/). This report needs to be uploaded manually using
following steps:

1. Checkout the relevant commit from the [main branch](https://github.com/eclipse-bluechi/bluechi)
1. Create new branch to post PR with updated (for example `update-code-coverage`):

   ```shell
   git checkout -b update-code-coverage
   ```

1. Store git hash of the current commit into `doc/code-coverage/commit-hash.txt`:

   ```shell
   git rev-parse --short HEAD > doc/code-coverage/commit-hash.txt
   ```

1. Run integration tests with code coverage enabled for the current commit as described at
   [Creating code coverage report from integration tests execution](../../tests#creating-code-coverage-report-from-integration-tests-execution)
1. Fetch code coverage results from integration tests (let's assume that integration tests result are stored in
   `/var/tmp/tmt/run-001`) and save it into `doc/code-coverage/merged.info`

   ```shell
   cp /var/tmp/tmt/run-001/plans/tier0/report/default-0/merged.info doc/code-coverage/merged.info
   ```

1. Create a PR with updated `commit-hash.txt` and `merged.info`

Once this PR is merged code coverage report should be updated.
