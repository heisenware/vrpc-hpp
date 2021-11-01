# VRPC - Variadic Remote Procedure Calls

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/heisenware/vrpc-hpp/master/LICENSE)
[![Semver](https://img.shields.io/badge/semver-2.0.0-blue)](https://semver.org/spec/v2.0.0.html)
[![GitHub Releases](https://img.shields.io/github/tag/heisenware/vrpc-hpp.svg)](https://github.com/heisenware/vrpc-hpp/tag)
[![GitHub Issues](https://img.shields.io/github/issues/heisenware/vrpc-hpp.svg)](http://github.com/heisenware/vrpc-hpp/issues)

---
## See: https://vrpc.io
---

## What is VRPC?

VRPC - Variadic Remote Procedure Calls - is an enhancement of the old RPC
(remote procedure calls) idea. Like RPC, it allows to directly call functions
written in any programming language by functions written in any other (or the
same) programming language. Unlike RPC, VRPC furthermore supports:

- remote function calls on many distributed receivers at the same time (one
  client - multiple agents)
- service discovery
- outbound-connection-only network architecture (using MQTT technology)
- isolated (multi-tenant) and shared access modes to remotely available
  resources
- asynchronous language constructs (callbacks, promises, event-loops)
- OOP (classes, objects, member functions) and functional (lambda) patterns
- exception forwarding

VRPC is available for an entire spectrum of programming technologies including
embedded (arduino, header-only C++, etc.), data-science (python, R,
etc.), and web (javascript, react, etc.) technologies.

As a robust and highly performing communication system it can build the
foundation of complex digitization projects in the area of (I)IoT or
Cloud-Computing.

This open-source project is professionally managed and supported by the
[Heisenware GmbH](https://heisenware.com), which is additionally offering a
cloud-based, no-code platform interfacing all VRPC adapted code.
