# Security Policy

## Supported Versions

Security fixes for **QTools** are provided in new releases. Please make sure
that you can reproduce the problem with the latest release before
reporting it.

| Version          | Supported          |
| ---------------- | ------------------ |
| latest release   | :white_check_mark: |
| older releases   | :x: (please upgrade) |

Commercial licensees receive security fixes as specified in their license
and support agreement.

QTools are host-based tools (QSPY, QUTest, QView, and others). Problems in
third-party programs bundled with QTools (such as the GNU toolchains) should
be reported to their maintainers; if you are not sure, report them to us and
we will forward them.

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub
issues, discussions, or pull requests.**

Instead, report them privately in one of these ways:

- email **security@state-machine.com**, or
- use GitHub's **"Report a vulnerability"** button on this repository's
  **Security** tab.

Please include the QTools version affected, the target (CPU, compiler, port,
or host OS) if relevant, a description of the problem and its impact, and
the steps to reproduce it (or a proof of concept).

## What to Expect

- acknowledgement of your report within **5 business days**
- confirmation (or not) of the vulnerability, usually within **30 days**
- a fixed release and a GitHub security advisory, crediting you if you wish

Please give us reasonable time to fix the problem before disclosing it
publicly. Our full vulnerability disclosure policy is at
**https://www.state-machine.com/security/**
