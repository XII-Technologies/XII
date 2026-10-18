# Security policy

## Support policy

XII is under active development. Security fixes target the current `Development`
branch. There is no established long-term support or guaranteed backport policy
for historical tags or binary snapshots. Consumers should evaluate the current
revision and track advisories and release notes.

## Private reporting

Use [GitHub private vulnerability reporting](https://github.com/XII-Technologies/XII/security/advisories/new)
when it is enabled. If that option is unavailable, the project's existing contact
is [contact@xiitechnologies.com](mailto:contact@xiitechnologies.com). Do not open
a public issue containing exploit details, secrets, or sensitive data.

Include the affected commit/version and component, impact, prerequisites, exact
reproduction steps or a minimal test case, platform/toolchain information, and
any mitigation. Tell us whether the report also affects a third-party dependency.
Provide enough information to reproduce the issue without sharing production data.

## Scope and disclosure

Relevant reports include memory corruption, unsafe asset/archive parsing, unintended
code execution, path traversal, security-relevant networking flaws, and compromise
of build, dependency, or release automation. Ordinary crashes, rendering defects,
and unsupported configurations without a security impact belong in the issue tracker.

Please coordinate disclosure while the report is assessed and a remediation or
mitigation is prepared. Avoid accessing other people's data, disrupting services,
or testing systems without authorization. Response and resolution times depend
on available maintainers and complexity; no fixed deadline or bounty is promised.

Confirmed issues can be communicated through
[GitHub security advisories](https://github.com/XII-Technologies/XII/security/advisories),
fix commits, and release notes. Researcher credit is given with consent. A release
archive or successful CI run does not imply a security certification.
