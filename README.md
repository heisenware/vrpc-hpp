# VRPC - Variadic Remote Procedure Calls

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/heisenware/vrpc-hpp/master/LICENSE)
[![Semver](https://img.shields.io/badge/semver-2.0.0-blue)](https://semver.org/spec/v2.0.0.html)
[![GitHub Releases](https://img.shields.io/github/tag/heisenware/vrpc-hpp.svg)](https://github.com/heisenware/vrpc-hpp/tag)
[![GitHub Issues](https://img.shields.io/github/issues/heisenware/vrpc-hpp.svg)](http://github.com/heisenware/vrpc-hpp/issues)
![ci](https://github.com/heisenware/vrpc-hpp/actions/workflows/ci.yml/badge.svg)

## Visit our website: [vrpc.io](https://vrpc.io)

## What is VRPC?

VRPC is an enhancement of the old RPC (remote procedure calls) idea. Like RPC,
it allows to directly call functions written in any programming language by
functions written in any other (or the same) programming language. Unlike RPC,
VRPC uses an MQTT broker for message routing, and additionally supports:

- non-intrusive adaption of existing code, making it remotely callable
- remote function calls on many distributed receivers at the same time (one
  client - multiple agents)
- service discovery
- outbound-connection-only network architecture (thanks to MQTT technology)
- isolated (multi-tenant) and shared access modes to remotely available
  resources
- asynchronous language constructs (callbacks, promises, event-loops)
- OOP (classes, objects, member functions) and functional (lambda) patterns
- exception forwarding

VRPC is available for an entire spectrum of programming technologies including
embedded (Arduino, header-only C++, etc.), data-science (Python, R,
etc.), and web (Javascript, React, etc.) technologies.

As a robust and highly performing communication system it can build the
foundation of complex digitization projects in the area of (I)IoT or
Cloud-Computing.

> VRPC is professionally managed and supported by
> [Heisenware GmbH](https://heisenware.com).

## This is VRPC for C++ (as header-only library)

vrpc_hpp requires C++14 and the Boost Libraries 1.67.0 or later.

Understand how to use it by looking at the examples:

- [Simple Agent Example](examples/01-foo/README.md)
- [Advanced Agent Example](examples/02-bar/README.md)

Get all the details by reading the documentation:

- [Adapter](docs/adapter.md)
- [Agent](docs/agent.md)
